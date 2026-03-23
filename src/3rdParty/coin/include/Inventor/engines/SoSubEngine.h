#ifndef COIN_SOSUBENGINE_H
#define COIN_SOSUBENGINE_H

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

#include <cassert>
#include <Inventor/SbName.h>
#include <Inventor/SoType.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/engines/SoEngine.h>
#include <Inventor/engines/SoOutputData.h>
#include <Inventor/fields/SoFieldData.h>

// *************************************************************************

//
// FIXME: document macros. pederb, 20000309
//

#define PRIVATE_ENGINE_TYPESYSTEM_HEADER( ) \
public: \
  static SoType getClassTypeId(void); \
  virtual SoType getTypeId(void) const; \
private: \
  static SoType classTypeId

#define SO_ENGINE_ABSTRACT_HEADER(_classname_) \
  PRIVATE_ENGINE_TYPESYSTEM_HEADER(); \
protected: \
  static const SoFieldData ** getInputDataPtr(void); \
  static const SoEngineOutputData ** getOutputDataPtr(void); \
public: \
  virtual const SoFieldData * getFieldData(void) const; \
  virtual const SoEngineOutputData * getOutputData(void) const; \
private: \
  static unsigned int classinstances; \
  static SoFieldData * inputdata; \
  static const SoFieldData ** parentinputdata; \
  static SoEngineOutputData * outputdata; \
  static const SoEngineOutputData ** parentoutputdata; \
  static void atexit_cleanup(void)

#define SO_ENGINE_HEADER(_classname_) \
    SO_ENGINE_ABSTRACT_HEADER(_classname_); \
  public: \
    static void * createInstance(void)

// *************************************************************************

#define PRIVATE_ENGINE_TYPESYSTEM_SOURCE(_class_) \
 \
/*! \
  This static method returns the SoType object associated with \
  objects of this class. \
*/ \
SoType _class_::getClassTypeId(void) { return _class_::classTypeId; } \
 \
/*!
  Returns the type identification of an object derived from a \
  class inheriting SoBase. This is used for runtime type checking and \
  "downward" casting. \
*/ \
SoType _class_::getTypeId(void) const { return _class_::classTypeId; } \
SoType _class_::classTypeId STATIC_SOTYPE_INIT

#define SO_ENGINE_ABSTRACT_SOURCE(_class_) \
PRIVATE_ENGINE_TYPESYSTEM_SOURCE(_class_); \
 \
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
const SoFieldData ** \
_class_::getInputDataPtr(void) \
{ \
  return const_cast<const SoFieldData **>(&_class_::inputdata); \
} \
 \
/*! \
  Returns the SoFieldData class which holds information about inputs \
  in this engine. \
*/ \
const SoFieldData * \
_class_::getFieldData(void) const \
{ \
  return _class_::inputdata; \
} \
 \
/*! \
  Returns the SoEngineOutputData class which holds information about \
  the outputs in this engine. \
*/ \
const SoEngineOutputData ** \
_class_::getOutputDataPtr(void) \
{ \
  return const_cast<const SoEngineOutputData**>(&_class_::outputdata); \
} \
 \
/*! \
  Returns the SoEngineOutputData class which holds information about \
  the outputs in this engine. \
*/ \
const SoEngineOutputData * \
_class_::getOutputData(void) const \
{ \
  return _class_::outputdata; \
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
  assert(_class_::classTypeId != SoType::badType()); \
  SoType::removeType(_class_::classTypeId.getName()); \
  _class_::classTypeId STATIC_SOTYPE_INIT; \
  _class_::classinstances = 0; \
}

#define SO_ENGINE_SOURCE(_class_) \
SO_ENGINE_ABSTRACT_SOURCE(_class_); \
 \
/*! \
  Creates a new instance of the class type corresponding to the SoType object. \
*/ \
void * \
_class_::createInstance(void) \
{ \
  return new _class_; \
}

// *************************************************************************

#define SO_ENGINE_IS_FIRST_INSTANCE() \
   (classinstances == 1)

#define SO_ENGINE_CONSTRUCTOR(_class_) \
  do { \
    SoBase::staticDataLock(); \
    _class_::classinstances++; \
    /* Catch attempts to use an engine class which has not been initialized. */ \
    assert(_class_::classTypeId != SoType::badType()); \
    /* Initialize a inputdata container for the class only once. */ \
    if (!_class_::inputdata) { \
      _class_::inputdata = \
        new SoFieldData(_class_::parentinputdata ? \
                        *_class_::parentinputdata : NULL); \
      _class_::outputdata = \
        new SoEngineOutputData(_class_::parentoutputdata ? \
                               *_class_::parentoutputdata : NULL); \
    } \
    /* Extension classes from the application programmers should not be */ \
    /* considered native. This is important to get the export code to do */ \
    /* the Right Thing. */ \
    this->isBuiltIn = FALSE; \
    SoBase::staticDataUnlock(); \
  } WHILE_0

// *************************************************************************

#define PRIVATE_COMMON_ENGINE_INIT_CODE(_class_, _classname_, _createfunc_, _parentclass_) \
  do { \
    /* Make sure we only initialize once. */ \
    assert(_class_::classTypeId == SoType::badType()); \
    /* Make sure superclass gets initialized before subclass. */ \
    assert(_parentclass_::getClassTypeId() != SoType::badType()); \
 \
    /* Set up entry in the type system. */ \
    _class_::classTypeId = \
      SoType::createType(_parentclass_::getClassTypeId(), \
                         _classname_, \
                         _createfunc_); \
 \
    /* Store parent's data pointers for later use in the constructor. */ \
    _class_::parentinputdata = _parentclass_::getInputDataPtr(); \
    _class_::parentoutputdata = _parentclass_::getOutputDataPtr(); \
    cc_coin_atexit_static_internal \
      (reinterpret_cast<coin_atexit_f*>(_class_::atexit_cleanup));  \
  } WHILE_0


#define SO_ENGINE_INIT_CLASS(_class_, _parentclass_, _parentname_) \
  do { \
    const char * classname = SO__QUOTE(_class_); \
    PRIVATE_COMMON_ENGINE_INIT_CODE(_class_, classname, &_class_::createInstance, _parentclass_); \
  } WHILE_0

#define SO_ENGINE_INIT_ABSTRACT_CLASS(_class_, _parentclass_, _parentname_) \
  do { \
    const char * classname = SO__QUOTE(_class_); \
    PRIVATE_COMMON_ENGINE_INIT_CODE(_class_, classname, NULL, _parentclass_); \
  } WHILE_0

// *************************************************************************

#define SO_ENGINE_ADD_INPUT(_input_, _defaultval_) \
  do { \
    this->_input_.setValue _defaultval_;\
    this->_input_.setContainer(this); \
    inputdata->addField(this, SO__QUOTE(_input_), &this->_input_);\
  } WHILE_0

#define SO_ENGINE_ADD_OUTPUT(_output_, _type_) \
  do { \
    outputdata->addOutput(this, SO__QUOTE(_output_), \
                          &this->_output_, \
                          _type_::getClassTypeId()); \
    this->_output_.setContainer(this); \
  } WHILE_0

// *************************************************************************

#define SO_ENGINE_DEFINE_ENUM_VALUE(_enumname_, _enumval_) \
  do { \
    inputdata->addEnumValue(SO__QUOTE(_enumname_), \
                            SO__QUOTE(_enumval_), _enumval_); \
  } WHILE_0

#define SO_ENGINE_OUTPUT(_engineout_, _fieldtype_, _writeop_) \
  do { \
    if (_engineout_.isEnabled()) { \
      /* No fields can be added or removed during this loop, as it */ \
      /* is a "closed" operation. (The fields are disabled for */ \
      /* notification while the loop runs). */ \
      int SO_ENGINE_OUTPUT_numconnections = _engineout_.getNumConnections(); \
      /* The reason we use the perverted variable names is to */ \
      /* avoid the possibility of getting _extremely_ hard */ \
      /* to find bugs when _writeop_ contains the same variable */ \
      /* names we are using internally in the macro. */ \
      for (int SO_ENGINE_OUTPUT_i = 0; SO_ENGINE_OUTPUT_i < SO_ENGINE_OUTPUT_numconnections; SO_ENGINE_OUTPUT_i++) { \
        _fieldtype_ * SO_ENGINE_OUTPUT_field = \
           static_cast<_fieldtype_*>(_engineout_[SO_ENGINE_OUTPUT_i]); \
        if (!SO_ENGINE_OUTPUT_field->isReadOnly()) { SO_ENGINE_OUTPUT_field->_writeop_; } \
      } \
      /* paranoid assertion */ \
      assert(_engineout_.getNumConnections() == SO_ENGINE_OUTPUT_numconnections); \
    } \
  } WHILE_0

// *************************************************************************

#define SO_COMPOSE__HEADER(_name_) \
  SO_ENGINE_HEADER(_name_); \
  private: \
    virtual void evaluate(); \
  protected: \
    virtual ~_name_();\
  public: \
   _name_(); \
    static void initClass()

// *************************************************************************

#endif // !COIN_SOSUBENGINE_H
