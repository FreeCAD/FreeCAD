#ifndef COIN_SOSUBELEMENT_H
#define COIN_SOSUBELEMENT_H

/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

/*
 * This file contains macro definitions with common declarations and
 * definitions used in Coin elements.
 *
 * FIXME: document macros and how they are used to set up a new
 * user-extension skeleton element class (or just point to the example
 * code in examples/advanced/elements/). 19991209 mortene.
 *
 */

// *************************************************************************

#include <Inventor/SbBasic.h> // for SO__QUOTE() definition
#include <Inventor/SbName.h>
#include <Inventor/C/tidbits.h>
#include <cassert>
// Include SoElement.h to be Open Inventor compatible at compile-time.
#include <Inventor/elements/SoElement.h>

// *************************************************************************

#define SO_ELEMENT_ABSTRACT_HEADER(_class_) \
public: \
  static SoType getClassTypeId(void); \
  static int getClassStackIndex(void); \
protected: \
  _class_(void); \
private: \
  /* classStackIndex is protected in the OIV UNIX distribution in */ \
  /* SoSubElement.h and private in the Win32 distribution. Since */ \
  /* there is a getClassStackIndex() access method, it seems more */ \
  /* sensible to keep it private.  20000808 mortene. */ \
  static int classStackIndex; \
  static SoType classTypeId; \
  static void cleanupClass(void) { SoType::removeType(_class_::classTypeId.getName()); _class_::classTypeId STATIC_SOTYPE_INIT; }

// *************************************************************************

#define SO_ELEMENT_HEADER(_class_) \
  SO_ELEMENT_ABSTRACT_HEADER(_class_); \
public: \
  static void * createInstance(void)

// *************************************************************************

#define PRIVATE_SOELEMENT_VARIABLES(_class_) \
int _class_::classStackIndex; \
SoType _class_::classTypeId STATIC_SOTYPE_INIT; \
 \
/*! \
  This static method returns the SoType object associated with \
  objects of this class. \
*/ \
SoType _class_::getClassTypeId(void) { return _class_::classTypeId; } \
 \
/*!
  This static method returns the state stack index for the class. \
*/ \
int _class_::getClassStackIndex(void) { return _class_::classStackIndex; }


#define SO_ELEMENT_ABSTRACT_SOURCE(_class_) \
PRIVATE_SOELEMENT_VARIABLES(_class_) \
_class_::_class_(void) { }

#define SO_ELEMENT_SOURCE(_class_) \
PRIVATE_SOELEMENT_VARIABLES(_class_) \
_class_::_class_(void) { this->setTypeId(_class_::classTypeId); \
                         this->setStackIndex(_class_::classStackIndex); } \
/*! \COININTERNAL \
 \
  Creates a new instance of the class type corresponding to the SoType object. \
*/ \
void * _class_::createInstance(void) { return new _class_; }

/*
  Specific to Coin. Added 2003-10-27.
*/
#define SO_ELEMENT_CUSTOM_CONSTRUCTOR_SOURCE(_class_) \
PRIVATE_SOELEMENT_VARIABLES(_class_) \
/*! \COININTERNAL \
 \
  Creates a new instance of the class type corresponding to the SoType object. \
*/ \
void * _class_::createInstance(void) { return new _class_; }

// *************************************************************************

#define PRIVATE_SOELEMENT_INIT(_class_, _parent_, _instantiate_) \
  do { \
    assert(_class_::classTypeId == SoType::badType()); \
    assert(_parent_::getClassTypeId() != SoType::badType()); \
    _class_::classTypeId = SoType::createType(_parent_::getClassTypeId(), \
                                              SO__QUOTE(_class_), \
                                              _instantiate_); \
    if (_parent_::getClassStackIndex() < 0) _class_::classStackIndex = _class_::createStackIndex(_class_::classTypeId); \
    else _class_::classStackIndex = _parent_::getClassStackIndex(); \
    cc_coin_atexit_static_internal(reinterpret_cast<coin_atexit_f*>(_class_::cleanupClass)); \
  } WHILE_0


#define SO_ELEMENT_INIT_ABSTRACT_CLASS(_class_, _parent_) \
  PRIVATE_SOELEMENT_INIT(_class_, _parent_, NULL)

#define SO_ELEMENT_INIT_CLASS(_class_, _parent_) \
  PRIVATE_SOELEMENT_INIT(_class_, _parent_, &_class_::createInstance)

// *************************************************************************

#endif // !COIN_SOSUBELEMENT_H
