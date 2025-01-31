/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_MAXMSPEXTERNALS_MATRIX_CONVOLVER_INIT_FILTER_MATRIX_HPP_INCLUDED
#define VISR_MAXMSPEXTERNALS_MATRIX_CONVOLVER_INIT_FILTER_MATRIX_HPP_INCLUDED

#include <string>
#include <vector>

namespace visr
{
// Forward declaration
namespace efl
{
template< typename DataType >
class BasicMatrix;
}
namespace rbbl
{
class IndexSequence;
}

namespace maxmsp
{
namespace matrix_convolver
{

/**
 * Initialise the matrix of impulse responses from a set of parameters.
 * @param filterList A comma-separated string containing the filter files(WAVs) 
 * to be read. Empty or blank if no files are to be loaded.
 * @param maxFilterLength The length of the filters. If provided, the WAV files are checked against this value, 
 * and padded with zeros. To ignore this value, use std::numeric_limits<std::size_t>::max(). In this case, the 
 * filter length is set to the length of the longest WAV file.
 * @param maxFilterEntries The number of filters in the returned matrix. An error is generated if the highest filter index exceeds this value.
 * Use std::numeric_limits<std::size_t>::max() to ignore this parameter. In this case, the number of filters 
 * is determined from the highest position index of the impulse responses.
 * @param indexOffsets Starting indices for the irs from the WAV files. If specified, it must has the same number of values 
 * as the number of files in \p  filterList. To ignore, pass an empty sequence. In this case, the IRs care stored consecutively in the output matrix.
 * @param [out] matrix The resized and filled impulse reesponse matrix.
 * @throw std::invalid_argument In case of inconsistent parameters.
 */
template< typename DataType >
void initFilterMatrix( std::string const & filterList,
                       std::size_t maxFilterLength,
                       std::size_t maxFilterEntries,
                       rbbl::IndexSequence const & indexOffsets,
                       efl::BasicMatrix<DataType> & matrix );

} // namespace matrix_convolver
} // namespace maxmsp
} // namespace visr

#endif // #ifndef VISR_MAXMSPEXTERNALS_MATRIX_CONVOLVER_INIT_FILTER_MATRIX_HPP_INCLUDED
