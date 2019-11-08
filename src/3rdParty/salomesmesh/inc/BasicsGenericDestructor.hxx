// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

//  SALOME Basics : general SALOME definitions and tools (C++ part - no CORBA)
//  File   : BasicGenericDestructor.hxx
//  Author : Antoine YESSAYAN, Paul RASCLE, EDF
//  Module : SALOME
//  $Header$
//
#ifndef _BASICGENERICDESTRUCTOR_HXX_
#define _BASICGENERICDESTRUCTOR_HXX_

#include "SALOME_Basics.hxx"

#include <list>
#include <algorithm>
#include <cassert>
#include <typeinfo>
#include <iostream>
#include <cstdlib>
#include <pthread.h>

//#define _DEVDEBUG_

#ifdef _DEVDEBUG_
#define MYDEVTRACE {std::cerr << __FILE__ << " [" << __LINE__ << "] : ";}
#define DEVTRACE(msg) {MYDEVTRACE; std::cerr<<msg<<std::endl<<std::flush;}
#else
#define MYDEVTRACE
#define DEVTRACE(msg)
#endif

#ifdef WIN32
#pragma warning(disable:4251) // Warning DLL Interface ...
#endif

// ============================================================================
/*!
 * The PROTECTED_DELETE base class provides a protected destructor. 
 * The only way to use PROTECTED_DELETE is inheritance:
 *   example: class LocalTraceBufferPool : public PROTECTED_DELETE
 * Herited class destructor must stay protected.
 * Normal use is:
 * - create an instance of herited class on the heap (new),
 * - use addObj(instance) to store the instance on the static list _objList,
 * - delete instance with deleteInstance(instance)
 *
 * This class is utilised with GENERIC_DESTRUCTOR and DESTRUCTOR_OF, 
 * to program automatic deletion of objects at the end of the process, while
 * keeping the possibility of an early destruction, if required. This is used
 * for unit testing and trace mecanism.
 */ 
// ============================================================================

class BASICS_EXPORT PROTECTED_DELETE
{
public:
  static void deleteInstance(PROTECTED_DELETE *anObject);
  static void addObj(PROTECTED_DELETE *anObject);

protected:
  virtual ~PROTECTED_DELETE();
  static std::list<PROTECTED_DELETE*> _objList;

private:
  static pthread_mutex_t _listMutex;
};

// ============================================================================
/*!
 * The GENERIC_DESTRUCTOR abstract class describes the comportement of any
 * destruction object. This type is used to create a list of miscellaneous
 * destruction objects.
 *
 * The only way to use the GENERIC_DESTRUCTOR class is inheritance:
 *   class SPECIFIC_DESTRUCTOR : public GENERIC_DESTRUCTOR
 * 
 * A generic destructor provides two methods:
 * -# a static method to add a destruction (object) to be performed:
 *    GENERIC_DESTRUCTOR::Add(GENERIC_DESTRUCTOR &anObject);
 *    The Destruction object is stored in a list of pointer to
 *    GENERIC_DESTRUCTOR objects.
 * -# an object method to execute the destruction : operator()().
 */ 
// ============================================================================

class BASICS_EXPORT GENERIC_DESTRUCTOR
{
public :
  static std::list<GENERIC_DESTRUCTOR*> *Destructors;

  virtual ~GENERIC_DESTRUCTOR() {};
  static const int Add(GENERIC_DESTRUCTOR &anObject);
  virtual void operator()(void) = 0;
};

// ============================================================================
/*! 
 * The DESTRUCTOR_OF class allows the user to program - at any moment - the
 * destruction of an object at the end of the process.
 *
 * Example: the POINT ptrPoint will be destroyed at the end of the process
 * (atexit).
 *
 * POINT *ptrPoint = new POINT ;
 *
 * DESTRUCTOR_OF<POINT> *ptrDestruct = new DESTRUCTOR_OF<POINT>(*ptrPoint);
 * 
 * Note that neither ptrPoint, nor ptrDestruct should be destroyed by the user.
 * 
 * The destruction object must be created dynamically because it suscribes
 * itself in the list of destruction to be performed at the end of the process.
 */ 
// ============================================================================

template <class TYPE> class DESTRUCTOR_OF : public GENERIC_DESTRUCTOR
{
public:
  /*!
    Programs the destruction at the end of the process, of the object anObject.
    This method records in _objectPtr the address of an object to be destroyed 
    at the end of the process
  */
  DESTRUCTOR_OF(TYPE &anObject):
    _objectPtr(&anObject)
  {
    DEVTRACE(" DESTRUCTOR_OF " << typeid(anObject).name() 
             << " " << _objectPtr << " " << this );
    PROTECTED_DELETE::addObj(_objectPtr);
    assert(GENERIC_DESTRUCTOR::Add(*this) >= 0);
  }

  /*!
    Performs the destruction of the object.
    This method really destroys the object pointed by _objectPtr. 
    It should be called at the end of the process (i.e. at exit).
  */
  virtual void operator()(void)
  {
    if (_objectPtr)
      {
        DEVTRACE("DESTRUCTOR_OF<>::operator() " << _objectPtr);
        if (_objectPtr) PROTECTED_DELETE::deleteInstance(_objectPtr);
        _objectPtr = NULL;
      }
  }

  virtual ~DESTRUCTOR_OF()
  {
    DEVTRACE("~DESTRUCTOR_OF() " << this);
    assert(!_objectPtr);
  }

private:
  TYPE *_objectPtr;
};

# endif
