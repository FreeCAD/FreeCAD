#ifndef COIN_SCXMLSUBOBJECT_H
#define COIN_SCXMLSUBOBJECT_H

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

#include <Inventor/SbName.h>

/*
 * This file contains macros for setting up the classes in the ScXML
 * subsystem (Statechart XML) in Coin. They loosely follow the macros
 * design for nodes and nodekits and engines, etc.
 */

// *************************************************************************

#define SCXML_OBJECT_HEADER(classname)                          \
public:                                                         \
  static SoType getClassTypeId(void);                           \
  virtual SoType getTypeId(void) const;                         \
  static void * createInstance(void);                           \
private:                                                        \
  static SoType classTypeId;

// *************************************************************************

#define SCXML_OBJECT_ABSTRACT_HEADER(classname)                 \
public:                                                         \
  static SoType getClassTypeId(void);                           \
  virtual SoType getTypeId(void) const = 0;                     \
private:                                                        \
  static SoType classTypeId;

// *************************************************************************

#define SCXML_ELEMENT_ABSTRACT_HEADER(classname)                \
  SCXML_OBJECT_ABSTRACT_HEADER(classname)

// *************************************************************************

// FIXME: element-reader must be reenatrant/threadsafe
#define SCXML_ELEMENT_HEADER(classname)                         \
public:                                                         \
  static ScXMLEltReader * getElementReader(void);               \
private:                                                        \
  static ScXMLEltReader * elementReader;                        \
  SCXML_OBJECT_HEADER(classname)

// *************************************************************************

#define SCXML_OBJECT_SOURCE(classname)                          \
                                                                \
SoType classname::classTypeId = SoType::badType();              \
                                                                \
SoType                                                          \
classname::getClassTypeId(void)                                 \
{                                                               \
  return classname::classTypeId;                                \
}                                                               \
                                                                \
SoType                                                          \
classname::getTypeId(void) const                                \
{                                                               \
  return classname::classTypeId;                                \
}                                                               \
                                                                \
void *                                                          \
classname::createInstance(void)                                 \
{                                                               \
  return new classname;                                         \
}

// *************************************************************************

#define SCXML_OBJECT_ABSTRACT_SOURCE(classname)                 \
                                                                \
SoType classname::classTypeId = SoType::badType();              \
                                                                \
SoType                                                          \
classname::getClassTypeId(void)                                 \
{                                                               \
  return classname::classTypeId;                                \
}

#define SCXML_ELEMENT_ABSTRACT_SOURCE(classname)                \
  SCXML_OBJECT_ABSTRACT_SOURCE(classname)

#define SCXML_ELEMENT_SOURCE(classname)                         \
ScXMLEltReader * classname::elementReader = NULL;               \
                                                                \
ScXMLEltReader *                                                \
classname::getElementReader(void)                               \
{                                                               \
  return classname::elementReader;                              \
}                                                               \
                                                                \
  SCXML_OBJECT_SOURCE(classname)

// *************************************************************************

#define SCXML_OBJECT_INIT_CLASS(thisclass, parentclass, parentname)     \
  do {                                                                  \
    SoType parenttype = SoType::fromName(SO__QUOTE(parentclass));       \
    assert(parenttype != SoType::badType());                            \
    thisclass::classTypeId =                                            \
      SoType::createType(parenttype,                                    \
                         SbName(SO__QUOTE(thisclass)),                  \
                         thisclass::createInstance);                    \
 /* ScXMLObject::registerClassType(xmlns, xmlclass,                     \
                                   thisclass::classTypeId); */          \
  } while ( FALSE )


#define SCXML_ELEMENT_REGISTER_READER(thisclass, xmlelement, classname) \
  do {                                                                  \
    thisclass::elementReader = new classname;                           \
  } while ( FALSE )

#define SCXML_ELEMENT_UNREGISTER_READER(thisclass) \
  do {                                                                  \
    delete thisclass::elementReader;                                    \
    thisclass::elementReader = NULL;                                    \
  } while ( FALSE )

// *************************************************************************

#define SCXML_OBJECT_INIT_ABSTRACT_CLASS(thisclass, parentclass, parentname) \
  do {                                                                  \
    SoType parenttype = SoType::fromName(SO__QUOTE(parentclass));       \
    assert(parenttype != SoType::badType());                            \
    thisclass::classTypeId =                                            \
      SoType::createType(parenttype, SbName(SO__QUOTE(thisclass)));     \
  } while ( FALSE )

// *************************************************************************

#define SCXML_INVOKE_INIT_CLASS(thisclass, parentclass, xmlns, targettype, source) \
  do {                                                                  \
    SoType parenttype = SoType::fromName(SO__QUOTE(parentclass));       \
    assert(parenttype != SoType::badType());                            \
    thisclass::classTypeId =                                            \
      SoType::createType(parenttype,                                    \
                         SbName(SO__QUOTE(thisclass)),                  \
                         thisclass::createInstance);                    \
    ScXMLObject::registerInvokeClassType(xmlns, targettype, source,     \
                                         thisclass::classTypeId);       \
  } while ( FALSE )

// *************************************************************************


#endif // !COIN_SCXMLSUBOBJECT_H
