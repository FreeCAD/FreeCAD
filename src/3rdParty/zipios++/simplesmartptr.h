#ifndef SIMPLESMARTPTR_H
#define SIMPLESMARTPTR_H

#include "zipios-config.h"

namespace zipios {

/** SimpleSmartPointer is a simple reference counting smart pointer
    template. The type pointed to must keep a reference count that is
    accessible through the two methods void ref() const and unsigned
    int unref() const. The type must also handle the reference count
    properly. The easiest way to do that is to use the ReferenceCount
    template class. */
template< class Type >
class SimpleSmartPointer {
public:
  Type *operator-> () const { return _p ;  }

  Type &operator* ()  const { return *_p ; }

  SimpleSmartPointer( Type *p = 0 ) : _p( p ) { ref() ; }

  template< class T2 > SimpleSmartPointer( const SimpleSmartPointer< T2 > &src ) 
    : _p( src.get() ) { ref() ; }

  SimpleSmartPointer( const SimpleSmartPointer &src ) : _p( src.get() ) { 
    ref() ; 
  }

  ~SimpleSmartPointer () { if ( unref() == 0 ) deleteIt() ; }

  template< class T2 > 
  SimpleSmartPointer &operator= ( const SimpleSmartPointer< T2 > &src ) {
    ref( src.get() ) ;
    if ( unref() == 0 )
      deleteIt() ;
    _p = src.get() ;
    return *this ;
  }

  SimpleSmartPointer &operator= ( const SimpleSmartPointer &src ) {
    ref( src.get() ) ;
    if ( unref() == 0 )
      deleteIt() ;
    _p = src.get() ;
    return *this ;
  }

  SimpleSmartPointer &operator=( Type *src ) {
    _p = src ;
    ref() ; 
    return *this ;
  }

  bool operator== ( const Type *p )                const { return _p == p     ; }
  bool operator!= ( const Type *p )                const { return _p != p     ; }
  bool operator== ( const SimpleSmartPointer &sp ) const { return _p == sp.get() ; }
  bool operator!= ( const SimpleSmartPointer &sp ) const { return _p != sp.get() ; }
  bool operator!  ()                               const { return ! _p        ; }
  // This next method is inspired by iostream, and is for use with 
  // if ( some_smart_pointer ).
  operator void*() const { return _p ? (void *)(-1) : (void *)(0) ; }

  Type *get() const { return _p ; }

  /** Returns the reference count - For debugging purposes. */
  unsigned int getReferenceCount() const { return _p->getReferenceCount(); }


private:
  template< class T2 >
  void ref( const T2 *ptr ) { if ( ptr ) ptr->ref() ; }

  void ref() const { if ( _p ) _p->ref() ; }
  unsigned int unref() const {
    if ( _p )
      return _p->unref();
    else
      return 0 ;
  }
  void deleteIt() {
//      if( _p )
//        cerr << "SimpleSmartPointer: Deleting object!" << endl ;
    delete _p ;
  }
  Type *_p ;
};


/** ReferenceCount is useful to ensure proper handling of the
    reference count for (objects of) classes handled through a
    SimpleSmartPointer. Subclassing ReferenceCount is all a class
    needs to become ready for being handled by
    SimpleSmartPointer. Another way is to add a ReferenceCount member
    variable to a class and write two methods 'void ref() const' and
    'unsigned int unref() const' that invoke the same methods in the
    ReferenceCount variable. */
template< class Type >
class ReferenceCount {
  /** SimpleSmartPointer needs to be a friend to invoke the private
      ref() and unref() methods.  */
  friend class SimpleSmartPointer< Type > ;
  friend class SimpleSmartPointer< const Type > ;
  /** Type also needs to be a friend to invoke the private ref() and
      unref() methods, in case Type doesn't want to inherit
      ReferenceCount and thus needs to invoke ref() and unref()
      through forwarding member functions. */
  //
  //  Originally the following template parameter was made a friend.
  //  This is not allowed by the standard so comment it out:
  //
  // friend Type ;
  //
  //  Initially hack things by making the necessary classes friends
  //  even though we don't know really which they are.  This is an
  //  Hideous Hack.
  friend class FileEntry ;
  friend class Bogus ;
  
public:
  /** Constructor initializes count to zero. */
  ReferenceCount() : _ref_count( 0 ) {}

  /** Copy-constructor initializes count to zero. It doesn't copy it
      from src. */
  ReferenceCount( const ReferenceCount & /*src*/ ) : _ref_count( 0 ) {}

  /** The assignment operator doesn't copy the reference count, it
      leaves it unchanged.  */
  const ReferenceCount &operator= ( const ReferenceCount & /*src*/ ) { return *this; }
private:

  /** Increases the reference count. */
  void ref() const           { ++_ref_count ;        }

  /** Decreases the reference count. */
  unsigned int unref() const { return --_ref_count ; }

  /** Returns the reference count - For debugging purposes. */
  unsigned int getReferenceCount() const { return _ref_count; }

  /** Holds the actual reference count */
  mutable unsigned short _ref_count ;
};



} // namespace

#endif

/** \file
    Header file that defines SimpleSmartPointer and ReferenceCount.
*/

/*
  Zipios++ - a small C++ library that provides easy access to .zip files.
  Copyright (C) 2000  Thomas Søndergaard
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/
