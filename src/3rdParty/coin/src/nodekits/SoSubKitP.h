#ifndef COIN_SOSUBKITP_H
#define COIN_SOSUBKITP_H

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

// The macro definitions in this file are used internally by Coin
// classes, and mirrors some of the public macros available in
// SoSubKit.h with a few modifications so they are suited for the
// built-in classes.
//
// The macros in this file are not made visible for use by the
// application programmer.


#ifndef COIN_INTERNAL
#error this is a private header file
#endif // !COIN_INTERNAL

#include "nodes/SoSubNodeP.h"

// Note for developers: match changes to this macro with the same
// changes to SoBaseKit::initClass().
#define SO_KIT_INTERNAL_INIT_CLASS(_class_, _fileformats_) \
  do { \
    SO_NODE_INTERNAL_INIT_CLASS(_class_, _fileformats_); \
    _class_::parentcatalogptr = inherited::getClassNodekitCatalogPtr(); \
  } while (0)


#define SO_KIT_INTERNAL_CONSTRUCTOR(_class_) \
  do { \
    SO_NODE_INTERNAL_CONSTRUCTOR(_class_); \
    SoBase::staticDataLock(); \
    if (_class_::classcatalog == NULL) { \
      SoType mytype = SoType::fromName(SO__QUOTE(_class_)); \
      if (_class_::parentcatalogptr) \
        _class_::classcatalog = (*_class_::parentcatalogptr)->clone(mytype); \
      else \
        _class_::classcatalog = new SoNodekitCatalog; \
      cc_coin_atexit_static_internal(reinterpret_cast<coin_atexit_f*>(_class_::atexit_cleanupkit)); \
    } \
    SoBase::staticDataUnlock(); \
  } while (0)

#endif // !COIN_SOSUBKITP_H
