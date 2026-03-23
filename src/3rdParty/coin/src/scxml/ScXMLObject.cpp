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

#ifdef _MSC_VER
#pragma warning(disable:4786) // symbol truncated
#endif // _MSC_VER

#include <Inventor/scxml/ScXMLObject.h>

/*!
  \class ScXMLObject ScXMLObject.h Inventor/scxml/ScXMLObject.h
  \brief Base class for all SCXML objects.

  \since Coin 3.0
  \ingroup coin_scxml
*/

/*!
  \fn ScXMLObject::getTypeId() const

  \brief \copybrief SoBase::getTypeId() const

  \sa SoBase::getTypeId() const
*/

/*!
  \fn ScXMLObject::getClassTypeId()

  \brief \copybrief SoBase::getClassTypeId()

  \sa SoBase::getClassTypeId
*/

#include <cstring>
#include <cassert>
#include <map>

#include <Inventor/SbName.h>
#include <Inventor/scxml/ScXML.h>

#include "scxml/ScXMLP.h"

// *************************************************************************

class ScXMLObject::PImpl {
public:
  typedef std::map<const char *, char *> AttributeMap;
  typedef std::pair<const char *, char *> AttributeEntry;
  AttributeMap attributemap;

  ~PImpl(void)
  {
    AttributeMap::iterator it = this->attributemap.begin();
    while (it != this->attributemap.end()) {
      delete [] it->second;
      ++it;
    }
    this->attributemap.clear();
  }
};

#define PRIVATE(obj) ((obj)->pimpl)

SCXML_OBJECT_ABSTRACT_SOURCE(ScXMLObject);


/*!
  \brief \copybrief SoBase::initClass()

  \sa SoBase::initClass()
*/
void
ScXMLObject::initClass(void)
{
  ScXMLObject::classTypeId =
    SoType::createType(SoType::badType(), SbName("ScXMLObject"));
  // not registered with ScXML
}

/*!
  \brief Does the opposite of initClass, deallocates any class
  specific resources and resets the class to an uninitialized state
  with respect to the Coin type system
 */
void
ScXMLObject::cleanClass(void)
{
  ScXMLObject::classTypeId = SoType::badType();
}

ScXMLObject::ScXMLObject(void)
: containerptr(NULL)
{
}

ScXMLObject::~ScXMLObject(void)
{
  this->containerptr = NULL;
}

/*!
  \brief \copybrief SoBase::isOfType(SoType type) const

  \sa SoBase::isOfType(SoType type) const
*/
SbBool
ScXMLObject::isOfType(SoType type) const
{
  return this->getTypeId().isDerivedFrom(type);
}

// *************************************************************************

/*!
  Internal method.
  Friendly forwarding for ScXMLObject subclasses.
*/
void
ScXMLObject::registerClassType(const char * xmlns, const char * classname, SoType type)
{
  ScXMLP::registerClassType(xmlns, classname, type);
}

/*!
  Internal method.
  Friendly forwarding for ScXMLObject subclasses.
*/
void
ScXMLObject::registerInvokeClassType(const char * xmlns, const char * targettype, const char * source, SoType type)
{
  ScXMLP::registerInvokeClassType(xmlns, targettype, source, type);
}

#undef PRIVATE
