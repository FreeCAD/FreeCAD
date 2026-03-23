#ifndef COIN_SOSUBNODEP_H
#define COIN_SOSUBNODEP_H

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
// SoSubNode.h with a few modifications so they are suited for the
// built-in classes.
//
// The macros in this file are not made visible for use by the
// application programmer.

#ifndef COIN_INTERNAL
#error this is a private header file
#endif // !COIN_INTERNAL

// only internal nodes can use this macro and pass "inherited" as arg #4
#define PRIVATE_INTERNAL_COMMON_INIT_CODE(_class_, _classname_, _createfunc_, _parentclass_) \
  do { \
    /* Make sure we only initialize once. */ \
    assert(_class_::classTypeId == SoType::badType() && "don't init() twice!"); \
    /* Make sure superclass gets initialized before subclass. */ \
    assert(_parentclass_::getClassTypeId() != SoType::badType() && "you forgot init() on parentclass!"); \
 \
    /* Set up entry in the type system. */ \
    _class_::classTypeId = \
      SoType::createType(_parentclass_::getClassTypeId(), \
                         _classname_, \
                         _createfunc_, \
                         SoNode::getNextActionMethodIndex()); \
    SoNode::incNextActionMethodIndex(); \
 \
    /* Store parent's fielddata pointer for later use in the constructor. */ \
    _class_::parentFieldData = _parentclass_::getFieldDataPtr(); \
    cc_coin_atexit_static_internal(reinterpret_cast<coin_atexit_f *>(_class_::atexit_cleanup)); \
  } while (0)

#define SO_NODE_INTERNAL_CONSTRUCTOR(_class_) \
  do { \
    SoBase::staticDataLock(); \
    SO_NODE_CONSTRUCTOR_NOLOCK(_class_); \
    /* Restore value of isBuiltIn flag (which is set to FALSE */ \
    /* in the SO_NODE_CONSTRUCTOR() macro. */ \
    this->isBuiltIn = TRUE; \
    SoBase::staticDataUnlock(); \
  } while (0)


#define SO_NODE_INTERNAL_INIT_CLASS(_class_, _fileformats_) \
  do { \
    const char * classname = SO__QUOTE(_class_); \
    PRIVATE_INTERNAL_COMMON_INIT_CODE(_class_, &classname[2], &_class_::createInstance, inherited); \
    SoNode::setCompatibilityTypes(_class_::getClassTypeId(), _fileformats_); \
  } while (0)


#define SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(_class_, _fileformats_) \
  do { \
    const char * classname = SO__QUOTE(_class_); \
    PRIVATE_INTERNAL_COMMON_INIT_CODE(_class_, &classname[2], NULL, inherited); \
    SoNode::setCompatibilityTypes(_class_::getClassTypeId(), _fileformats_); \
  } while (0)


// *************************************************************************

// Convenience defines to use for the second parameter of the
// SO_NODE_INTERNAL_INIT_CLASS() macro (for internal node classes).
//
// It's is handy to use these defines instead of the enum flags
// directly, as these can be updated in this one central location,
// instead of having to update all node source files on each new Coin
// major release.

#define SO_FROM_COIN_4_0 \
  (SoNode::COIN_4_0)

#define SO_FROM_COIN_3_0 \
  (SoNode::COIN_3_0|SO_FROM_COIN_4_0)

#define SO_FROM_COIN_2_5 \
  (SoNode::COIN_2_5|SO_FROM_COIN_3_0)

#define SO_FROM_COIN_2_4 \
  (SoNode::COIN_2_4|SO_FROM_COIN_2_5)

#define SO_FROM_COIN_2_3 \
  (SoNode::COIN_2_3|SO_FROM_COIN_2_4)

#define SO_FROM_COIN_2_2 \
  (SoNode::COIN_2_2|SO_FROM_COIN_2_3)

#define SO_FROM_COIN_2_0 \
  (SoNode::COIN_2_0|SO_FROM_COIN_2_2)

#define SO_FROM_COIN_1_0 \
  (SoNode::COIN_1_0|SO_FROM_COIN_2_0)

// *************************************************************************

#define SO_FROM_INVENTOR_6_0 \
  (SoNode::INVENTOR_6_0)

#define SO_FROM_INVENTOR_5_0 \
  (SoNode::INVENTOR_5_0)

// Several releases missing in-between here, but we don't seem to have
// any nodes from those yet...  -mortene.

#define SO_FROM_INVENTOR_2_6 \
  (SoNode::INVENTOR_2_6|SO_FROM_INVENTOR_5_0)

#define SO_FROM_INVENTOR_2_5 \
  (SoNode::INVENTOR_2_5|SO_FROM_INVENTOR_2_6)

#define SO_FROM_INVENTOR_2_1 \
  (SoNode::INVENTOR_2_1|SO_FROM_INVENTOR_2_5|SO_FROM_COIN_1_0)

#define SO_FROM_INVENTOR_2_0 \
  (SoNode::INVENTOR_2_0|SO_FROM_INVENTOR_2_1)

#define SO_FROM_INVENTOR_1 \
  (SoNode::INVENTOR_1|SO_FROM_INVENTOR_2_0)

// *************************************************************************

#endif // !COIN_SOSUBNODEP_H
