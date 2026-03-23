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

#ifndef COIN_SOVRMLMACROS_H
#define COIN_SOVRMLMACROS_H

#include <Inventor/nodes/SoSubNode.h>

#define SO_VRMLNODE_INTERNAL_CONSTRUCTOR(_class_) \
  SO_NODE_INTERNAL_CONSTRUCTOR(_class_); \
  this->setNodeType(SoNode::VRML2);

#define SO_VRMLNODE_ADD_EVENT_IN(_field_) \
  do { \
    this->_field_.setFieldType(SoField::EVENTIN_FIELD); \
    this->_field_.setContainer(this); \
    fieldData->addField(this, SO__QUOTE(_field_), &this->_field_);\
  } WHILE_0

#define SO_VRMLNODE_ADD_EVENT_OUT(_field_) \
  do { \
    this->_field_.setFieldType(SoField::EVENTOUT_FIELD); \
    this->_field_.setContainer(this); \
    fieldData->addField(this, SO__QUOTE(_field_), &this->_field_);\
  } WHILE_0

#define SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(_field_) \
  do { \
    this->_field_.setFieldType(SoField::EXPOSED_FIELD); \
    this->_field_.setContainer(this); \
    fieldData->addField(this, SO__QUOTE(_field_), &this->_field_);\
  } WHILE_0

#define SO_VRMLNODE_ADD_EMPTY_MFIELD(_field_) \
  do { \
    this->_field_.setContainer(this); \
    fieldData->addField(this, SO__QUOTE(_field_), &this->_field_);\
  } WHILE_0


#define SO_VRMLNODE_ADD_FIELD(_field_, _defaultval_) \
  SO_NODE_ADD_FIELD(_field_, _defaultval_)

#define SO_VRMLNODE_ADD_EXPOSED_FIELD(_field_, _defaultval_) \
  this->_field_.setFieldType(SoField::EXPOSED_FIELD); \
  SO_NODE_ADD_FIELD(_field_, _defaultval_)

#define SO_VRML97_NODE_TYPE (SoNode::VRML2|SoNode::COIN_2_0)


#endif // COIN_SOVRMLMACROS_H
