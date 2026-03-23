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

#ifndef COIN_SOSUBNODEENGINE_H
#define COIN_SOSUBNODEENGINE_H

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/engines/SoSubEngine.h>

#define SO_NODEENGINE_ABSTRACT_HEADER(_class_) \
  SO_NODE_ABSTRACT_HEADER(_class_); \
protected: \
  static const SoEngineOutputData ** getOutputDataPtr(void); \
public: \
  virtual const SoEngineOutputData * getOutputData(void) const; \
private: \
  static void atexit_cleanupnodeengine(void); \
  static SoEngineOutputData * outputdata; \
  static const SoEngineOutputData ** parentoutputdata

#define SO_NODEENGINE_HEADER(_class_) \
  SO_NODEENGINE_ABSTRACT_HEADER(_class_); \
  public: \
    static void * createInstance(void)

#define SO_NODEENGINE_ABSTRACT_SOURCE(_class_) \
SO_NODE_ABSTRACT_SOURCE(_class_); \
SoEngineOutputData * _class_::outputdata = NULL; \
const SoEngineOutputData ** _class_::parentoutputdata = NULL; \
 \
const SoEngineOutputData ** \
_class_::getOutputDataPtr(void) \
{ \
  return (const SoEngineOutputData**)&_class_::outputdata; \
} \
 \
const SoEngineOutputData * \
_class_::getOutputData(void) const \
{ \
  return _class_::outputdata; \
} \
 \
void \
_class_::atexit_cleanupnodeengine(void) { \
  delete _class_::outputdata; \
  _class_::outputdata = NULL; \
  _class_::parentoutputdata = NULL; \
  _class_::classTypeId STATIC_SOTYPE_INIT; \
}

#define SO_NODEENGINE_SOURCE(_class_) \
SO_NODEENGINE_ABSTRACT_SOURCE(_class_); \
 \
void * \
_class_::createInstance(void) \
{ \
  return new _class_; \
}

#define SO_NODEENGINE_CONSTRUCTOR(_class_) \
  do { \
    SoBase::staticDataLock(); \
    _class_::classinstances++; \
    /* Catch attempts to use an engine class which has not been initialized. */ \
    assert(_class_::classTypeId != SoType::badType()); \
    /* Initialize a inputdata container for the class only once. */ \
    if (!_class_::fieldData) { \
      _class_::fieldData = \
        new SoFieldData(_class_::parentFieldData ? \
                        *_class_::parentFieldData : NULL); \
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

#define PRIVATE_COMMON_NODEENGINE_INIT_CODE(_class_, _classname_, _createfunc_, _parentclass_) \
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
    _class_::parentoutputdata = _parentclass_::getOutputDataPtr(); \
  } WHILE_0

#define SO_NODEENGINE_INIT_CLASS(_class_, _parentclass_, _parentname_) \
  do { \
    const char * classname = SO__QUOTE(_class_); \
    PRIVATE_COMMON_INIT_CODE(_class_, classname, &_class_::createInstance, _parentclass_); \
  } WHILE_0

#define SO_NODEENGINE_EXIT_CLASS(_class_) \
  _class_::atexit_cleanupnodeengine();

#define SO_NODEENGINE_INIT_ABSTRACT_CLASS(_class_, _parentclass_, _parentname_) \
  do { \
    const char * classname = SO__QUOTE(_class_); \
    PRIVATE_COMMON_INIT_CODE(_class_, classname, NULL, _parentclass_); \
  } WHILE_0

#define SO_NODEENGINE_ADD_OUTPUT(_output_, _type_) \
  do { \
    outputdata->addOutput(this, SO__QUOTE(_output_), \
                          &this->_output_, \
                          _type_::getClassTypeId()); \
    this->_output_.setNodeContainer(this); \
  } WHILE_0

#endif // COIN_SOSUBNODEENGINE_H
