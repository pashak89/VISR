/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "core_convolver_uniform.hpp"

#include <librbbl/fft_wrapper_factory.hpp>

#include <complex>

namespace visr
{
namespace rbbl
{

template< typename SampleType >
CoreConvolverUniform<SampleType>::
CoreConvolverUniform( std::size_t numberOfInputs,
                              std::size_t numberOfOutputs,
                              std::size_t blockLength,
                              std::size_t maxFilterLength,
                              std::size_t maxFilterEntries,
                              efl::BasicMatrix<SampleType> const & initialFilters,
                              std::size_t alignment /*= 0*/,
                              char const * fftImplementation /*= "default"*/ )
 : mAlignment( alignment )
 , mComplexAlignment( alignment/2 )
 , mNumberOfInputs( numberOfInputs )
 , mNumberOfOutputs( numberOfOutputs )
 , mBlockLength( blockLength )
 , mMaxFilterLength( maxFilterLength )
 , mNumberOfFilterPartitions( calculateNumberOfPartitions( maxFilterLength, blockLength ) )
 , mDftSize( calculateDftSize( blockLength ) )
 , mDftRepresentationSizePadded( calculateDftRepresentationSizePadded( blockLength, mComplexAlignment ) )
 , mInputBuffers( numberOfInputs, mDftSize, alignment )
 , mInputFDL( numberOfInputs, mDftRepresentationSizePadded * mNumberOfFilterPartitions, mComplexAlignment )
 , mFdlCycleOffset( 0 )
 , mTimeDomainTransformBuffer( mDftSize, mAlignment )
 , mFilterPartitionsFrequencyDomain( maxFilterEntries, mDftRepresentationSizePadded * mNumberOfFilterPartitions, mComplexAlignment )
 , mFrequencyDomainAccumulator( mDftRepresentationSizePadded, mComplexAlignment )
 , mFftRepresentation( FftWrapperFactory<SampleType>::create( fftImplementation, mDftSize, alignment ) )
 , mFilterScalingFactor( calculateFilterScalingFactor() )
{
  initFilters( initialFilters );
}

template< typename SampleType >
CoreConvolverUniform<SampleType>::~CoreConvolverUniform() = default;

template< typename SampleType >
void CoreConvolverUniform<SampleType>::
processInputs( SampleType const * const input, std::size_t channelStride, std::size_t alignment )
{
  mInputBuffers.write( input, channelStride, numberOfInputs(), blockLength(), alignment );
  advanceFDL();
  for( std::size_t chIdx( 0 ); chIdx < mNumberOfInputs; ++chIdx )
  {
    mFftRepresentation->forwardTransform( mInputBuffers.getReadPointer( chIdx, mDftSize ), getFdlBlock( chIdx, 0 ) );
  }
}

#if 1
//// 'old' implementation
//template< typename SampleType >
//void CoreConvolverUniform<SampleType>::
//processOutputs( SampleType * const output, std::size_t channelStride, std::size_t alignment )
//{
//  for( std::size_t outputIdx( 0 ); outputIdx < mNumberOfOutputs; ++outputIdx )
//  {
//    // get an iterator pair containing all routings for the current output.
//    // Not that this depends on the comparison function for the map
//    // keys, which orders all routings for a given output cosecutively.
//    typename RoutingTable::const_iterator start = mRoutingTable.lower_bound( RoutingKey( 0, outputIdx ) );
//    typename RoutingTable::const_iterator nextStart = mRoutingTable.lower_bound( RoutingKey( 0, outputIdx + 1 ) );
//
//    bool addFlag = false; 
//    for( typename RoutingTable::const_iterator routingIt( start ); routingIt != nextStart; ++routingIt )
//    {
//      std::size_t const inputIdx = routingIt->first.inputIdx;
//      std::size_t const filterIdx = routingIt->second.filterIdx;
//      SampleType const scaleFactor = routingIt->second.gainLinear;
//
//      processFilter( inputIdx, filterIdx, scaleFactor, mFrequencyDomainSum.data(), addFlag );
//      addFlag = true;
//    } // iterate through all routings for a given output port.
//    // Perform the inverse transformation
//    transformOutput( outputIdx, mFrequencyDomainSum.data(), output + channelStride * outputIdx );
//  }
//}
#else
template< typename SampleType >
void CoreConvolverUniform<SampleType>::
processOutputs( SampleType * const output, std::size_t channelStride, std::size_t alignment )
{
  std::size_t currOut = 0;
  std::size_t filtersAtCurrOutput = 0;
  for( typename RoutingTable::const_iterator tableIt = mRoutingTable.begin(); tableIt != mRoutingTable.end(); ++tableIt )
  {
    std::size_t thisOut = tableIt->outputIdx;
    if( thisOut == currOut )
    {
      bool const addFlag = (filtersAtCurrOutput > 0 );
      processFilter( tableIt->second->inputIdx, tableIt->second->filterIdx, tableIt->second->gainLinear, addFlag );
    }
    else if( thisOut == currOut + 1 )
    {
    }
  }

  typename RoutingTable::const_iterator tableIt = mRoutingTable.begin();
  for( std::size_t outputIdx( 0 ); outputIdx < mNumberOfOutputs; ++outputIdx )
  {
    // get an iterator pair containing all routings for the current output.
    // Not that this depends on the comparison function for the map
    // keys, which orders all routings for a given output cosecutively.
    typename RoutingTable::const_iterator start = mRoutingTable.lower_bound( RoutingKey( 0, outputIdx ) );
    typename RoutingTable::const_iterator nextStart = mRoutingTable.lower_bound( RoutingKey( 0, outputIdx + 1 ) );

    bool addFlag = false;
    for( typename RoutingTable::const_iterator routingIt( start ); routingIt != nextStart; ++routingIt )
    {
      std::size_t const  inputIdx = routingIt->first.inputIdx;
      std::size_t const  filterIdx = routingIt->second.filterIdx;
      SampleType const scaleFactor = routingIt->second.gainLinear;

      processFilter( inputIdx, filterIdx, scaleFactor, mFrequencyDomainSum.data(), addFlag );
      addFlag = true;
    } // iterate through all routings for a given output port.
      // Perform the inverse transformation
    transformOutput( outputIdx, mFrequencyDomainSum.data(), output + channelStride * outputIdx );
  }
}
#endif

template< typename SampleType >
void CoreConvolverUniform<SampleType>::processFilter( std::size_t inputIndex, std::size_t filterIndex, 
                                                      SampleType gain, FrequencyDomainType * result, bool addFlag )
{
  if( efl::vectorMultiply( getFdlBlock( inputIndex, 0 ),
    getFdFilterPartition( filterIndex, 0 ),
    mFrequencyDomainAccumulator.data(),
    mDftRepresentationSizePadded, // slightly more operations, but likely faster due to better use of vectorized operations.
    mComplexAlignment ) != efl::noError )
  {
    throw std::runtime_error( "CoreConvolverUniform::processOutput(): Frequency-domain block convolution failed." );
  }
  for( std::size_t blockIndex( 1 ); blockIndex < mNumberOfFilterPartitions; ++blockIndex )
  {
    if( efl::vectorMultiplyAddInplace( getFdlBlock( inputIndex, blockIndex ),
      getFdFilterPartition( filterIndex, blockIndex ),
      mFrequencyDomainAccumulator.data(),
      mDftRepresentationSizePadded, // slightly more operations, but likely faster due to better use of vectorized operations.
      mComplexAlignment ) != efl::noError )
    {
      throw std::runtime_error( "CoreConvolverUniform::processOutput(): Frequency-domain block convolution failed." );
    }
  }
  // Scale to the output vector.
  // Note that there is no dedicated function for scaling a complex vector by a real-valued constant at the moment, so we use a complex-complex variant.
  // (which is slightly inefficient)
  // Another option would be to cast the complex vectors to real-valued ones, and to do a real-valued scale and add for 2 time the length.
  if( addFlag )
  {
    if( efl::vectorMultiplyConstantAddInplace( static_cast<FrequencyDomainType>( gain ),
      mFrequencyDomainAccumulator.data(),
      result,
      mDftRepresentationSizePadded, // slightly more arithmetic operations than required, but likely faster due to better use of vectorized operations.
      mComplexAlignment ) != efl::noError )
      //if( efl::vectorMultiplyConstantAddInplace<SampleType>( static_cast<SampleType>(gain),
      //  reinterpret_cast<SampleType const * const>(mFrequencyDomainAccumulator.data()),
      //  reinterpret_cast<SampleType *>(mFrequencyDomainSum.data()),
      //  2*mDftRepresentationSizePadded, // slightly more arithmetic operations than required, but likely faster due to better use of vectorized operations.
      //  mComplexAlignment ) != efl::noError )
    {
      throw std::runtime_error( "CoreConvolverUniform::processOutput(): Frequency-domain block convolution failed." );
    }
  }
  else
  {
    if( efl::vectorMultiplyConstant( static_cast<FrequencyDomainType>( gain ),
      mFrequencyDomainAccumulator.data(),
      result,
      mDftRepresentationSizePadded, // slightly more arithmetic operations than required, but likely faster due to better use of vectorized operations.
      mComplexAlignment ) != efl::noError )
      //if( efl::vectorMultiplyConstantAddInplace<SampleType>( static_cast<SampleType>(gain),
      //  reinterpret_cast<SampleType const * const>(mFrequencyDomainAccumulator.data()),
      //  reinterpret_cast<SampleType *>(mFrequencyDomainSum.data()),
      //  2*mDftRepresentationSizePadded, // slightly more arithmetic operations than required, but likely faster due to better use of vectorized operations.
      //  mComplexAlignment ) != efl::noError )
    {
      throw std::runtime_error( "CoreConvolverUniform::processOutput(): Frequency-domain block convolution failed." );
    }
  }
}

template< typename SampleType >
void CoreConvolverUniform<SampleType>::transformOutput( FrequencyDomainType const * fdBlock, SampleType * tdResult )
{
  mFftRepresentation->inverseTransform( fdBlock, mTimeDomainTransformBuffer.data() );
  // discard the time-domain aliasing and copy the remaining samples to the output
  // TODO: Establish a computation for the guaranteed alignment (0 at the moment).
  if( efl::vectorCopy( mTimeDomainTransformBuffer.data() + blockLength(), tdResult, mDftSize - blockLength(), 0 ) != efl::noError )
  {
    throw std::runtime_error( "CoreConvolverUniform::transformOutput(): Copying of output samples failed." );
  }
}

template< typename SampleType >
/*static*/ std::size_t CoreConvolverUniform<SampleType>::
calculateNumberOfPartitions( std::size_t filterLength, std::size_t blockLength )
{
  return static_cast<std::size_t>(std::ceil( static_cast<double>(filterLength) / blockLength ));
}

template< typename SampleType >
/*static*/ std::size_t CoreConvolverUniform<SampleType>::
calculateDftSize( std::size_t blockLength )
{
  return 2 * blockLength;
}

template< typename SampleType>
/*static*/ std::size_t CoreConvolverUniform<SampleType>::
calculateDftRepresentationSize( std::size_t blockLength )
{
  std::size_t const dftSize = calculateDftSize( blockLength );
  if( dftSize % 2 != 0 ) // Cannot happen with the current setting for the DFT size.
  {
    throw std::invalid_argument( "At the moment, the complex representation of the DFT values supports only real-valued DFT sizes." );
  }
  return dftSize/2+1;
}

template< typename SampleType >
/*static*/ std::size_t CoreConvolverUniform<SampleType>::
calculateDftRepresentationSizePadded( std::size_t blockLength, std::size_t alignment )
{
  return efl::nextAlignedSize( calculateDftRepresentationSize( blockLength ), alignment);
}

template< typename SampleType>
SampleType CoreConvolverUniform<SampleType>::calculateFilterScalingFactor() const
{
  if( not mFftRepresentation )
  {
    throw std::logic_error( "CoreConvolverUniform::calculateFilterScalingFactor(): The FFT implementation object has not been initialized." );
  }
  SampleType const fwdConst = mFftRepresentation->forwardScalingFactor();
  SampleType const invConst = mFftRepresentation->inverseScalingFactor( );

  return static_cast<SampleType>(1.0) / (fwdConst*invConst*static_cast<SampleType>(mDftSize));
}

template< typename SampleType>
void CoreConvolverUniform<SampleType>::advanceFDL()
{
  mFdlCycleOffset = (mFdlCycleOffset + mNumberOfFilterPartitions - 1) % mNumberOfFilterPartitions; // The '+ mNumberOfFilterPartitions' is to avoid pecularities of % when the argument is negative.
}

///////////////////////////////////////////////////////////////////////////////
// Manipulation of the routing table

//template< typename SampleType>
//void CoreConvolverUniform<SampleType>::clearRoutingTable( )
//{
//  mRoutingTable.clear();
//}
//
//template< typename SampleType>
//void CoreConvolverUniform<SampleType>::initRoutingTable( RoutingList const & routings )
//{
//  clearRoutingTable();
//  if( routings.size() > maxNumberOfRoutingPoints() )
//  {
//    throw std::invalid_argument( "CoreConvolverUniform:initRoutingTable() exceeds the maximum admissible number of elements " );
//  }
//  for( RoutingEntry const & v : routings )
//  {
//    setRoutingEntry( v );
//  }
//  assert( mRoutingTable.size( ) <= maxNumberOfRoutingPoints( ) );
//}
//
//template< typename SampleType>
//void CoreConvolverUniform<SampleType>::setRoutingEntry( RoutingEntry const & routing )
//{
//  setRoutingEntry( routing.inputIndex, routing.outputIndex, routing.filterIndex, routing.gainLinear );
//}
//
//template< typename SampleType>
//void CoreConvolverUniform<SampleType>::setRoutingEntry( std::size_t inputIdx,
//                                                                std::size_t outputIdx,
//                                                                std::size_t filterIdx,
//                                                                RoutingEntry::GainType gain )
//{
//  assert( mRoutingTable.size() <= maxNumberOfRoutingPoints() );
//  if( inputIdx >= numberOfInputs() )
//  {
//    throw std::invalid_argument( "CoreConvolverUniform::setRoutingEntry(): Input index exceeds the admissible range." );
//  }
//  if( outputIdx >= numberOfOutputs() )
//  {
//    throw std::invalid_argument( "CoreConvolverUniform::setRoutingEntry(): Output index exceeds the admissible range." );
//  }
//  if( filterIdx >= maxNumberOfFilterEntries() )
//  {
//    throw std::invalid_argument( "CoreConvolverUniform::setRoutingEntry(): Filter index exceeds the admissible range." );
//  }
//
//  typename RoutingTable::iterator findIt = mRoutingTable.find( RoutingKey( inputIdx, outputIdx ) );
//  if( findIt != mRoutingTable.end() )
//  {
//    findIt->second = RoutingValue( filterIdx, static_cast<SampleType>(gain) );
//  }
//  else
//  {
//    if( mRoutingTable.size( ) >= maxNumberOfRoutingPoints( ) )
//    {
//      throw std::invalid_argument( "CoreConvolverUniform::setRoutingEntry(): Maximum number of routing points already reached." );
//    }
//    auto res = mRoutingTable.insert( std::make_pair( RoutingKey( inputIdx, outputIdx ),
//      RoutingValue( filterIdx, static_cast<SampleType>(gain) ) ) );
//    if( !res.second )
//    {
//      throw std::invalid_argument( "CoreConvolverUniform::setRoutingEntry(): Adding new routing point failed." );
//    }
//  }
//  assert( mRoutingTable.size( ) <= maxNumberOfRoutingPoints( ) );
//}
//
//template< typename SampleType>
//bool CoreConvolverUniform<SampleType>::removeRoutingEntry( std::size_t inputIdx, std::size_t outputIdx )
//{
//  std::size_t numErased = mRoutingTable.erase( RoutingKey( inputIdx, outputIdx ) );
//  return (numErased > 0);
//}

template< typename SampleType>
void CoreConvolverUniform<SampleType>::clearFilters()
{
  mFilterPartitionsFrequencyDomain.zeroFill();
}

template< typename SampleType >
void CoreConvolverUniform<SampleType>::initFilters( efl::BasicMatrix<SampleType> const & newFilters )
{
  clearFilters();
  if( newFilters.numberOfRows() > maxNumberOfFilterEntries() )
  {
    throw std::invalid_argument( "CoreConvolverUniform::initFilters( ): The initializer exceeds the maximum number of filter entries." );
  }
  if( newFilters.numberOfColumns( ) > maxFilterLength() )
  {
    throw std::invalid_argument( "CoreConvolverUniform::initFilters( ): The initializer exceeds the maximum filter length." );
  }
  std::size_t const numFilters = newFilters.numberOfRows( );
  std::size_t const filterLength = newFilters.numberOfColumns( );
  for( std::size_t filterIdx( 0 ); filterIdx < numFilters; ++filterIdx )
  {
    setImpulseResponse( newFilters.row( filterIdx ), filterLength, filterIdx, newFilters.alignmentElements() );
  }
  for( std::size_t filterIdx( numFilters ); filterIdx < maxNumberOfFilterEntries(); ++filterIdx )
  {
    efl::ErrorCode const res = efl::vectorZero( mFilterPartitionsFrequencyDomain.row( filterIdx ),
      mFilterPartitionsFrequencyDomain.numberOfColumns(), mComplexAlignment );
    if( res != efl::noError )
    {
      throw std::runtime_error( "CoreConvolverUniform::initFilters( ): Zeroing the excess filters failed." );
    }
  }
}

template< typename SampleType >
void CoreConvolverUniform<SampleType>::
transformImpulseResponse( SampleType const * ir, std::size_t irLength, FrequencyDomainType * result, std::size_t alignment /*= 0*/ ) const
{
  if( irLength > maxFilterLength() )
  {
    throw std::invalid_argument( "CoreConvolverUniform::transformImpulseResponse(): impulse response length exceeds maximum admissible values." );
  }
  for( std::size_t partitionIdx( 0 ); partitionIdx < mNumberOfFilterPartitions; ++partitionIdx )
  {
    mTimeDomainTransformBuffer.zeroFill( ); // This ensures correct results also if the the buffer is filled patially (i.e., less than than the admissible number of samples.
    std::size_t const startIdx = partitionIdx * blockLength();
    std::size_t const endIdx = std::min( (partitionIdx + 1) * blockLength(), irLength ); // STL-like 'one past end' index
    if( endIdx > startIdx )
    {
      // The following multiply calls assumes that the blocklength is a multiple of the alignment.
      // Multiply the the filter by the computed scaling constants to compensate for the FFT normalisation.
      if( efl::vectorMultiplyConstant( mFilterScalingFactor, ir + startIdx, mTimeDomainTransformBuffer.data(), endIdx - startIdx, std::min( mAlignment, alignment ) ) != efl::noError )
      {
        std::runtime_error( "CoreConvolverUniform::transformImpulseResponse(): Error while copying data." );
      }
    }
    FrequencyDomainType * blockResult = result + partitionIdx * mDftRepresentationSizePadded;
    // TODO: check error reporting of the wrapper.
    mFftRepresentation->forwardTransform( mTimeDomainTransformBuffer.data(), blockResult );
  }
}

template< typename SampleType >
void CoreConvolverUniform<SampleType>::
setImpulseResponse( SampleType const * ir, std::size_t filterLength, std::size_t filterIdx, std::size_t alignment /*= 0*/ )
{
  if( filterIdx >= maxNumberOfFilterEntries() )
  {
    throw std::invalid_argument( "CoreConvolverUniform::setImpulseResponse(): filter index exceeds number of filters" );
  }
  transformImpulseResponse( ir, filterLength,
                            mFilterPartitionsFrequencyDomain.row( filterIdx ),
                            std::min( mAlignment, alignment ) );
}

template< typename SampleType >
void CoreConvolverUniform<SampleType>::setFilter( FrequencyDomainType const * transformedFilter, std::size_t filterIdx, std::size_t alignment /*= 0*/ )
{
  if( filterIdx >= maxNumberOfFilterEntries() )
  {
    throw std::invalid_argument( "CoreConvolverUniform::setFilter(): filter index exceeds number of filters" );
  }
  efl::vectorCopy( transformedFilter, mFilterPartitionsFrequencyDomain.row( filterIdx ),
                   mFilterPartitionsFrequencyDomain.numberOfColumns(),
                   std::min( alignment, mComplexAlignment ) );
}

// explicit instantiations
template class CoreConvolverUniform<float>;
template class CoreConvolverUniform<double>;

} // namespace rbbl
} // namespace visr
