/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "position_decoder.hpp"

#include <libpml/empty_parameter_config.hpp>
#include <libpml/listener_position.hpp>
#include <libpml/string_parameter.hpp>

#include <libvisr/parameter_type.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>

namespace visr
{
namespace rcl
{

  PositionDecoder::PositionDecoder( SignalFlowContext const & context,
                                    char const * name,
                                    CompositeComponent * parent,
                                    panning::XYZ const &offsetKinect,
                                    float qw /*=1.0f*/,
                                    float qx /*= 0.0f*/,
                                    float qy /*= 0.0f*/,
                                    float qz /*= 0.0f*/ )
  : AtomicComponent( context, name, parent )
  , mDatagramInput( "messageInput", *this, pml::EmptyParameterConfig() )
  , mPositionOutput( "positionOutput", *this, pml::EmptyParameterConfig() )
{
    mOffsetKinect = offsetKinect;
    mQw = qw;
    mQx = qx;
    mQy = qy;
    mQz = qz;
}

PositionDecoder::~PositionDecoder()
{
}


namespace // unnamed
{
/**
 * Parse a listener ID from a JSON representation
 * @param inputStream The test stream to be parsed.
 * @param pos [out] Object returning the parsed position data.
 */
void parseJSON(std::istream &  inputStream, pml::ListenerPosition & pos)
{
  namespace pt = boost::property_tree;

  pt::ptree tree;
  pt::read_json( inputStream, tree );

  pos.setTimeNs( tree.get<pml::ListenerPosition::TimeType>( "nTime" ) * 100 );
  pos.setFaceID( tree.get<pml::ListenerPosition::IdType>( "iFace" ) );
  pos.set( tree.get<pml::ListenerPosition::Coordinate>( "headJoint.X" ),
          tree.get<pml::ListenerPosition::Coordinate>( "headJoint.Y" ),
          tree.get<pml::ListenerPosition::Coordinate>( "headJoint.Z" ) );
  // TODO: Implement parsing of orientation
  pos.setOrientation( 0.0f, 0.0f, 0.0f );
}

}

void PositionDecoder::process()
{
  pml::ListenerPosition newPos;
  pml::ListenerPosition foundPos;
  pml::ListenerPosition::IdType smallestFaceId = std::numeric_limits<unsigned int>::max();
  pml::ListenerPosition::TimeType latestTimeStamp = 0;
  while( !mDatagramInput.empty() )
  {
    char const * nextMsg = mDatagramInput.front().str();
    std::stringstream msgStream( nextMsg );
    try
    {
      parseJSON( msgStream, newPos );
      // within each iteration, use only the position with the samllest timestamp (i.e., the face which has been within the view 
      // of the tracker for the longest time.
      if( newPos.faceID() <= smallestFaceId )
      {
        // for a given face ID, update the position only if the timestamp is not older than the previously received timestamp.
        if( newPos.timeNs() >= latestTimeStamp )
        {
          foundPos = newPos;
          smallestFaceId = foundPos.faceID();
          latestTimeStamp = foundPos.timeNs();
        }
      }
    }
    catch( std::exception const & ex )
    {
      // Don't abort the program when receiving a corrupted message.
      std::cerr << "PositionDecoder: Error while decoding a position message: " << ex.what() << std::endl;
    }
    mDatagramInput.pop();
  }
  // Did we actually receive a valid message?
  if( smallestFaceId != std::numeric_limits<unsigned int>::max() )
  {
    mPositionOutput.data() = translatePosition( foundPos );
    mPositionOutput.swapBuffers();
  }
}

pml::ListenerPosition PositionDecoder::translatePosition( const pml::ListenerPosition &pos )
{
#if 0
  {
    std::array <float, 3> u{ { mQx, mQy, mQz } }; // the ijk components of the quaternion

    std::array <float, 3> v{ { pos.x(), pos.y(), pos.z() } }; //the vector to rotate


    // Extract the scalar part of the quaternion
    float s = mQw; //the quaternion w components

    // Do the math
    2.0f * dot(u, v) * u + (s*s - dot(u, u)) * v + 2.0f * s * cross(u, v);
  }
#endif

  return pml::ListenerPosition( -pos.z() + mOffsetKinect.x, -pos.x() + mOffsetKinect.y, pos.y() + mOffsetKinect.z );
}

} // namespace rcl
} // namespace visr
