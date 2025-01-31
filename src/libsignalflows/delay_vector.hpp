/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_MAXMSP_SIGNAlFLOWS_DELAY_VECTOR_HPP_INCLUDED
#define VISR_MAXMSP_SIGNAlFLOWS_DELAY_VECTOR_HPP_INCLUDED

#include "export_symbols.hpp"

#include <libvisr/audio_input.hpp>
#include <libvisr/audio_output.hpp>
#include <libvisr/composite_component.hpp>

#include <librcl/delay_vector.hpp>

#include <libefl/basic_vector.hpp>

#include <libpml/double_buffering_protocol.hpp>
#include <libpml/vector_parameter_config.hpp>
#include <libpml/vector_parameter_config.hpp>

namespace visr
{
namespace signalflows
{

class VISR_SIGNALFLOWS_LIBRARY_SYMBOL DelayVector: public CompositeComponent
{
public:
  explicit DelayVector( SignalFlowContext const & context,
                        const char * name,
                        CompositeComponent * parent,
                        std::size_t cNumberOfChannels,
                        std::size_t interpolationPeriod,
                        char const * interpolationMethod );

  ~DelayVector();

private:
  rcl::DelayVector mDelay;

  AudioInput mInput;

  AudioOutput mOutput;

  ParameterInput<pml::DoubleBufferingProtocol, pml::VectorParameter<SampleType> > mGainInput;

  ParameterInput<pml::DoubleBufferingProtocol, pml::VectorParameter<SampleType> > mDelayInput;
};

} // namespace signalflows
} // namespace visr

#endif // #ifndef VISR_SIGNALFLOWS_DELAY_VECTOR_HPP_INCLUDED
