#ifndef COIN_SOSUBEVENT_H
#define COIN_SOSUBEVENT_H

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

#include <Inventor/C/tidbits.h>

// *************************************************************************

#define SO_EVENT_HEADER() \
private: \
  static SoType classTypeId; \
  /*! \
    This static method cleans up static data of the class. \
  */ \
  static void cleanupClass(void) { classTypeId STATIC_SOTYPE_INIT; }; \
  static void * createInstance(void); \
public: \
  static SoType getClassTypeId(void); \
  virtual SoType getTypeId(void) const

#define SO_EVENT_ABSTRACT_HEADER() \
private: \
  static SoType classTypeId; \
  /*! \
    This static method cleans up static data of the class. \
  */ \
  static void cleanupClass(void) { classTypeId STATIC_SOTYPE_INIT; }; \
public: \
  static SoType getClassTypeId(void); \
  virtual SoType getTypeId(void) const

// *************************************************************************

#define SO_EVENT_ABSTRACT_SOURCE(_class_) \
 \
/*! \
  This static method returns the SoType object associated with \
  objects of this class. \
*/ \
SoType _class_::getClassTypeId(void) { return _class_::classTypeId; } \
 \
/*!
  Returns the type identification of an object derived from a \
  class inheriting SoEvent. This is used for runtime type checking and \
  "downward" casting. \
*/ \
SoType _class_::getTypeId(void) const { return _class_::classTypeId; } \
SoType _class_::classTypeId STATIC_SOTYPE_INIT

#define SO_EVENT_SOURCE(_class_) \
 \
/*! \
  This static method returns the SoType object associated with \
  objects of this class. \
*/ \
SoType _class_::getClassTypeId(void) { return _class_::classTypeId; } \
 \
/*!
  Returns the type identification of an object derived from a \
  class inheriting SoEvent. This is used for runtime type checking and \
  "downward" casting. \
*/ \
SoType _class_::getTypeId(void) const { return _class_::classTypeId; } \
/*! \COININTERNAL \
 \
  Creates a new instance of the class type corresponding to the SoType object. \
*/ \
void * _class_::createInstance(void) { return new _class_; } \
SoType _class_::classTypeId STATIC_SOTYPE_INIT

// *************************************************************************

#define SO_EVENT_INIT_ABSTRACT_CLASS(_class_, _parentclass_) \
  do { \
    /* Make sure we only initialize once. */ \
    assert(_class_::classTypeId == SoType::badType()); \
    /* Make sure superclass get initialized before subclass. */ \
    assert(_parentclass_::getClassTypeId() != SoType::badType()); \
    \
    _class_::classTypeId = \
      SoType::createType(_parentclass_::getClassTypeId(), SO__QUOTE(_class_)); \
    /* FIXME: internal code should not use this function, but use the coin_atexit() function */ \
    /* with priority set to CC_ATEXIT_NORMAL. As it is now, the clean-up functions for */ \
    /* these classes will always be run before all other Coin at-exit clean-ups. 20070126 mortene */ \
    cc_coin_atexit(reinterpret_cast<coin_atexit_f *>(cleanupClass)); \
  } WHILE_0

#define SO_EVENT_INIT_CLASS(_class_, _parentclass_) \
  do { \
    /* Make sure we only initialize once. */ \
    assert(_class_::classTypeId == SoType::badType()); \
    /* Make sure superclass get initialized before subclass. */ \
    assert(_parentclass_::getClassTypeId() != SoType::badType()); \
    \
    _class_::classTypeId = \
      SoType::createType(_parentclass_::getClassTypeId(), SO__QUOTE(_class_), _class_::createInstance); \
    /* FIXME: internal code should not use this function, but use the coin_atexit() function */ \
    /* with priority set to CC_ATEXIT_NORMAL. As it is now, the clean-up functions for */ \
    /* these classes will always be run before all other Coin at-exit clean-ups. 20070126 mortene */ \
    cc_coin_atexit(reinterpret_cast<coin_atexit_f *>(cleanupClass)); \
  } WHILE_0

// *************************************************************************

#endif // !COIN_SOSUBEVENT_H
