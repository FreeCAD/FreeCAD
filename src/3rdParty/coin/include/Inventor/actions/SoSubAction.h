#ifndef COIN_SOSUBACTION_H
#define COIN_SOSUBACTION_H

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

#ifndef COIN_INTERNAL
// Added to be Inventor compliant.
#include <Inventor/SbString.h>
#include <Inventor/actions/SoAction.h>
#endif // COIN_INTERNAL
#include <Inventor/C/tidbits.h>

// *************************************************************************

#define SO_ACTION_ADD_METHOD(_nodeclass_, _method_) \
  do { \
    addMethod(_nodeclass_::getClassTypeId(), (SoActionMethod)_method_); \
  } WHILE_0

// *************************************************************************

#define SO_ACTION_CONSTRUCTOR(_classname_) \
  do { \
    _classname_::traversalMethods = this->methods; \
  } WHILE_0

// *************************************************************************

#define SO_ACTION_HEADER(_classname_) \
public: \
  virtual SoType getTypeId(void) const; \
  static SoType getClassTypeId(void); \
  /*! \COININTERNAL */ \
  static void addMethod(const SoType type, SoActionMethod method); \
  /*! \COININTERNAL */ \
  static void enableElement(const SoType type, const int stackindex); \
 \
protected: \
  virtual const SoEnabledElementsList & getEnabledElements(void) const; \
  /* These two methods are not available in the original OIV API. */ \
  /* They have been added as a work-around for Win32 DLL headaches. */ \
  /* See further explanation below. 20000808 mortene. */ \
  static SoEnabledElementsList * getClassEnabledElements(void); \
  static SoActionMethodList * getClassActionMethods(void); \
 \
private: \
  /* The enabledElements and methods variables are protected in */ \
  /* the original OIV API. This is not such a good idea, since */ \
  /* exposed static class member variables is a major grievance */ \
  /* with regard to Win32 DLLs. */ \
  static void atexit_cleanup(void); \
  static SoEnabledElementsList * enabledElements; \
  static SoActionMethodList * methods; \
  static SoType classTypeId

// *************************************************************************

#define SO_ACTION_SOURCE(_classname_) \
SoEnabledElementsList * _classname_::enabledElements = NULL; \
SoActionMethodList * _classname_::methods = NULL; \
/*! \
  Returns list of enabled elements for this class. \
 \
  The enabledElements and methods variables are protected in \
  the original OIV API. This is not such a good idea, since \
  exposed static class member variables is a major grievance \
  with regard to Win32 DLLs. \
  \COIN_FUNCTION_EXTENSION \
*/ \
SoEnabledElementsList * _classname_::getClassEnabledElements(void) { return _classname_::enabledElements; } \
/*! \
  Returns list of action methods for this class. \
 \
  The enabledElements and methods variables are protected in \
  the original OIV API. This is not such a good idea, since \
  exposed static class member variables is a major grievance \
  with regard to Win32 DLLs. \
  \COIN_FUNCTION_EXTENSION \
*/ \
SoActionMethodList * _classname_::getClassActionMethods(void) { return _classname_::methods; } \
SoType _classname_::classTypeId STATIC_SOTYPE_INIT; \
 \
/*! \
  This static method returns the SoType object associated with \
  objects of this class. \
*/ \
SoType _classname_::getClassTypeId(void) { return _classname_::classTypeId; } \
 \
/*!
  Returns the type identification of an object derived from a \
  class inheriting SoAction. This is used for runtime type checking and \
  "downward" casting. \
*/ \
SoType _classname_::getTypeId(void) const { return _classname_::classTypeId; } \
/*! \
  Returns list of enabled elements. \
*/ \
const SoEnabledElementsList & _classname_::getEnabledElements(void) const \
{ \
  assert(_classname_::enabledElements); \
  return *_classname_::enabledElements; \
} \
void \
_classname_::addMethod(const SoType type, SoActionMethod method) \
{ \
  assert(_classname_::methods); \
  _classname_::methods->addMethod(type, method); \
} \
void \
_classname_::enableElement(const SoType type, const int stackindex) \
{ \
  assert(_classname_::enabledElements); \
  _classname_::enabledElements->enable(type, stackindex); \
} \
void \
_classname_::atexit_cleanup(void) \
{ \
  delete _classname_::enabledElements; \
  _classname_::enabledElements = NULL; \
  delete _classname_::methods; \
  _classname_::methods = NULL; \
  _classname_::classTypeId STATIC_SOTYPE_INIT; \
}

// *************************************************************************

#define SO_ACTION_INIT_CLASS(_classname_, _parentclassname_) \
  do { \
    assert(_classname_::getClassTypeId() == SoType::badType()); \
    assert(_parentclassname_::getClassTypeId() != SoType::badType()); \
    _classname_::classTypeId = SoType::createType(_parentclassname_::getClassTypeId(), SO__QUOTE(_classname_)); \
    _classname_::enabledElements = new SoEnabledElementsList(_parentclassname_::getClassEnabledElements()); \
    _classname_::methods = new SoActionMethodList(_parentclassname_::getClassActionMethods()); \
    cc_coin_atexit_static_internal(reinterpret_cast<coin_atexit_f *>(_classname_::atexit_cleanup));  \
  } WHILE_0

// *************************************************************************

#endif // !COIN_SOSUBACTION_H
