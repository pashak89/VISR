/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#include "object_vector.hpp"

#include <utility> // for std::make_pair

namespace visr
{
namespace objectmodel
{

ObjectVector::ObjectVector() = default;

ObjectVector::~ObjectVector() = default;

ObjectVector::ObjectVector( ObjectVector && rhs ) = default;

ObjectVector& ObjectVector::operator=( ObjectVector const & rhs ) = default;

/**
* Explicit definition of move assignment operator.
*/
ObjectVector& ObjectVector::operator=( ObjectVector && rhs ) = default;


void ObjectVector::assign( ObjectVector const & rhs )
{
  mObjects.clear();
  for( ObjectContainer::value_type const & v : rhs.mObjects )
  {
    mObjects.insert( v );
  }
}

void ObjectVector::swap( ObjectVector & rhs )
{
  mObjects.swap( rhs.mObjects );
}


Object const & ObjectVector::at( ObjectId id ) const
{
  ObjectContainer::const_iterator findIt = mObjects.find( id );
  if( findIt == mObjects.end() )
  {
    throw std::invalid_argument( "An audio object with this id does not exist." );
  }
  return *(findIt->mVal);
}

Object & ObjectVector::at( ObjectId id )
{
  ObjectContainer::iterator findIt = mObjects.find( id );
  if( findIt == mObjects.end() )
  {
    throw std::invalid_argument( "An audio object with this id does not exist." );
  }
  return *(findIt->mVal);
}

void ObjectVector::insert( Object const &  obj )
{
  ObjectContainer::iterator findIt = mObjects.find( obj.id() );
  if( findIt != mObjects.end() )
  {
    mObjects.erase( findIt );
  }
  mObjects.insert( obj.clone() );
}

void ObjectVector::insert( std::unique_ptr<Object> &&  obj )
{
  ObjectContainer::iterator findIt = mObjects.find( obj->id() );
  if( findIt != mObjects.end() )
  {
    mObjects.erase( findIt );
  }
  mObjects.insert( Containee( std::move(obj)) );
}

void ObjectVector::remove( ObjectId id )
{
  ObjectContainer::iterator findIt = mObjects.find( id );
  if( findIt == mObjects.end() )
  {
    throw std::invalid_argument( "An audio object with this id does not exist." );
  }
  mObjects.erase( findIt );
}
  
void ObjectVector::clear()
{
  mObjects.clear();
}
  
} // namespace objectmodel
} // namespace visr
