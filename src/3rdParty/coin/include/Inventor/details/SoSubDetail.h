#ifndef COIN_SOSUBDETAIL_H
#define COIN_SOSUBDETAIL_H

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

#include <Inventor/details/SoDetail.h>
#include <Inventor/C/tidbits.h> // for cc_coin_atexit()
#include <Inventor/SbName.h> // for implicit char* -> SbName cast in
                             // SoType::createType()
#include <cassert>

// *************************************************************************

#define SO_DETAIL_HEADER(_class_) \
public: \
  virtual SoType getTypeId(void) const; \
  static SoType getClassTypeId(void); \
private: \
  static SoType classTypeId; \
public: \
  /*! \
    This static method cleans up static data of the class. \
  */ \
  static void cleanupClass(void) { _class_::classTypeId STATIC_SOTYPE_INIT; }

// *************************************************************************

#define SO_DETAIL_SOURCE(_class_) \
SoType _class_::classTypeId STATIC_SOTYPE_INIT; \
 \
/*!
  Returns the type identification of an object derived from a \
  class inheriting SoDetail. This is used for runtime type checking and \
  "downward" casting. \
*/ \
SoType _class_::getTypeId(void) const { return _class_::classTypeId; } \
 \
/*! \
  This static method returns the SoType object associated with \
  objects of this class. \
*/ \
SoType _class_::getClassTypeId(void) { return _class_::classTypeId; }

// *************************************************************************

#define SO_DETAIL_INIT_CLASS(_class_, _parentclass_) \
  do { \
    /* Make sure we only initialize once. */ \
    assert(_class_::classTypeId == SoType::badType()); \
    /* Make sure superclass get initialized before subclass. */ \
    assert(_parentclass_::getClassTypeId() != SoType::badType()); \
    \
    _class_::classTypeId = \
           SoType::createType(_parentclass_::getClassTypeId(), \
                              SO__QUOTE(_class_)); \
    /* FIXME: internal code should not use this function, but use the coin_atexit() function */ \
    /* with priority set to CC_ATEXIT_NORMAL. As it is now, the clean-up functions for */ \
    /* these classes will always be run before all other Coin at-exit clean-ups. 20070126 mortene */ \
    cc_coin_atexit(reinterpret_cast<coin_atexit_f *>(_class_::cleanupClass)); \
  } WHILE_0

// *************************************************************************

#endif // !COIN_SOSUBDETAIL_H
