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

#ifndef COIN_SOSUBENGINEP_H
#define COIN_SOSUBENGINEP_H

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

#include <Inventor/engines/SoSubEngine.h>
#include "tidbitsp.h"

// Be aware that any changes to the SO_ENGINE_INTERNAL_CONSTRUCTOR
// macro should be matched by similar changes to the constructor in
// the SO_INTERPOLATE_SOURCE macro (which have to use
// SO_ENGINE_CONSTRUCTOR because it is "public").

#define SO_ENGINE_INTERNAL_CONSTRUCTOR(_class_) \
  do { \
    SO_ENGINE_CONSTRUCTOR(_class_); \
    /* Restore value of isBuiltIn flag (which is set to FALSE */ \
    /* in the SO_ENGINE_CONSTRUCTOR() macro). */ \
    this->isBuiltIn = TRUE; \
  } while (0)


#define SO_ENGINE_INTERNAL_INIT_CLASS(_class_) \
  do { \
    const char * classname = SO__QUOTE(_class_); \
    PRIVATE_COMMON_ENGINE_INIT_CODE(_class_, &classname[2], &_class_::createInstance, inherited); \
  } while (0)


#define SO_ENGINE_INTERNAL_INIT_ABSTRACT_CLASS(_class_) \
  do { \
    const char * classname = SO__QUOTE(_class_); \
    PRIVATE_COMMON_ENGINE_INIT_CODE(_class_, &classname[2], NULL, inherited); \
  } while (0)


#define SO_INTERPOLATE_INTERNAL_INIT_CLASS(_class_) \
 \
/*! \
  Sets up initialization for data common to all instances of \
  this class, like submitting necessary information to the Coin type \
  system. \
*/ \
void \
_class_::initClass(void) \
{ \
  SO_ENGINE_INTERNAL_INIT_CLASS(_class_); \
}


#define SO_INTERPOLATE_INTERNAL_SOURCE(_class_, _type_, _valtype_, _default0_, _default1_, _interpexp_) \
 \
SO_ENGINE_SOURCE(_class_); \
 \
_class_::_class_(void) \
{ \
  PRIVATE_SO_INTERPOLATE_CONSTRUCTOR(_class_, _type_, _valtype_, _default0_, _default1_); \
  this->isBuiltIn = TRUE; \
} \
 \
PRIVATE_SO_INTERPOLATE_DESTRUCTOR(_class_) \
PRIVATE_SO_INTERPOLATE_EVALUATE(_class_, _type_, _valtype_, _interpexp_)


#define SO_INTERNAL_ENGINE_SOURCE_DYNAMIC_IO(_class_) \
PRIVATE_ENGINE_TYPESYSTEM_SOURCE(_class_); \
unsigned int _class_::classinstances = 0; \
SoFieldData * _class_::inputdata = NULL; \
const SoFieldData ** _class_::parentinputdata = NULL; \
SoEngineOutputData * _class_::outputdata = NULL; \
const SoEngineOutputData ** _class_::parentoutputdata = NULL; \
 \
/*! \
  Returns the SoFieldData class which holds information about inputs \
  in this engine. \
*/ \
const SoFieldData ** _class_::getInputDataPtr(void) \
{ \
  assert(0 && "function not in use for _class_"); \
  return NULL; \
} \
 \
/*! \
  Returns the SoFieldData class which holds information about inputs \
  in this engine. \
*/ \
const SoFieldData * _class_::getFieldData(void) const \
{ \
  return this->dynamicinput; \
} \
 \
/*! \
  Returns the SoEngineOutputData class which holds information about \
  the outputs in this engine. \
*/ \
const SoEngineOutputData ** _class_::getOutputDataPtr(void) \
{ \
  assert(0 && "function not in use for _class_"); \
  return NULL; \
} \
 \
/*! \
  Returns the SoEngineOutputData class which holds information about \
  the outputs in this engine. \
*/ \
const SoEngineOutputData * _class_::getOutputData(void) const \
{ \
  return this->dynamicoutput; \
} \
 \
/*! \
  Creates a new instance of the class type corresponding to the SoType object. \
*/ \
void * _class_::createInstance(void) \
{ \
  return new _class_; \
} \
 \
void \
_class_::atexit_cleanup(void) \
{ \
  delete _class_::inputdata; \
  delete _class_::outputdata; \
  _class_::inputdata = NULL; \
  _class_::outputdata = NULL; \
  _class_::parentinputdata = NULL; \
  _class_::parentoutputdata = NULL; \
  _class_::classTypeId STATIC_SOTYPE_INIT; \
  _class_::classinstances = 0; \
}

#endif // !COIN_SOSUBENGINEP_H
