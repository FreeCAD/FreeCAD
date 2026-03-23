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

#ifndef COIN_SOSUBNODEENGINEP_H
#define COIN_SOSUBNODEENGINEP_H

// The macro definitions in this file are used internally by Coin
// classes, and mirrors some of the public macros available in
// SoSubEngine.h with a few modifications so they are suited for the
// built-in classes.
//
// The macros in this file are not made visible for use by the
// application programmer.

#ifndef COIN_INTERNAL
#error this is a private header file
#endif // !COIN_INTERNAL

#include <Inventor/engines/SoSubNodeEngine.h>

#include "tidbitsp.h"

// Be aware that any changes to the SO_ENGINE_INTERNAL_CONSTRUCTOR
// macro should be matched by similar changes to the constructor in
// the SO_INTERPOLATE_SOURCE macro (which have to use
// SO_ENGINE_CONSTRUCTOR because it is "public").

// Update 2006-08-11, pederb:
// The INIT_CLASS macros will now set the node type to VRML97. All
// node-engines in Coin are VRML97 nodes. If we add node engines that
// are not VRML97 nodes we need to add some new macros.


#define SO_NODEENGINE_INTERNAL_CONSTRUCTOR(_class_) \
  do { \
    SO_NODEENGINE_CONSTRUCTOR(_class_); \
    /* Restore value of isBuiltIn flag (which is set to FALSE */ \
    /* in the SO_ENGINE_CONSTRUCTOR() macro). */ \
    this->isBuiltIn = TRUE; \
  } while (0)

#define SO_NODEENGINE_INTERNAL_INIT_CLASS(_class_) \
  do { \
    const char * classname = SO__QUOTE(_class_); \
    PRIVATE_COMMON_NODEENGINE_INIT_CODE(_class_, &classname[2], &_class_::createInstance, inherited); \
    SoNode::setCompatibilityTypes(_class_::getClassTypeId(), SO_VRML97_NODE_TYPE); \
    coin_atexit((coin_atexit_f*)_class_::atexit_cleanupnodeengine, CC_ATEXIT_NORMAL); \
    coin_atexit((coin_atexit_f*)_class_::atexit_cleanup, CC_ATEXIT_NORMAL); \
  } while (0)


#define SO_NODEENGINE_INTERNAL_INIT_ABSTRACT_CLASS(_class_) \
  do { \
    const char * classname = SO__QUOTE(_class_); \
    PRIVATE_COMMON_NODEENGINE_INIT_CODE(_class_, &classname[2], NULL, inherited); \
    SoNode::setCompatibilityTypes(_class_::getClassTypeId(), SO_VRML97_NODE_TYPE); \
    coin_atexit((coin_atexit_f*)_class_::atexit_cleanupnodeengine, CC_ATEXIT_NORMAL); \
    coin_atexit((coin_atexit_f*)_class_::atexit_cleanup, CC_ATEXIT_NORMAL); \
  } while (0)


#endif // COIN_SOSUBNODEENGINEP_H
