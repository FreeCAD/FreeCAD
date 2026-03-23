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

#include <Inventor/scxml/ScXMLElt.h>
#include "coindefs.h"

/*!
  \class ScXMLElt ScXMLElt.h Inventor/scxml/ScXMLElt.h
  \brief base class for all SCXML elements.

  This is a common base class for all SCXML document element classes.
  It manages XML attributes at the generic level, has utilities for
  checking document containment, and provides some infrastructure hooks.

  \since Coin 3.1
  \ingroup coin_scxml
*/

/*!
  \class ScXMLEltReader ScXMLElt.h Inventor/scxml/ScXMLElt.h
  \brief base class for element reader objects

  \since Coin 3.1
  \ingroup coin_scxml
*/

#ifdef _MSC_VER
#pragma warning(disable:4786) // symbol truncated
#endif // _MSC_VER

#include <cstring>
#include <cassert>
#include <map>

#include <Inventor/SbName.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/C/XML/attribute.h>
#include <Inventor/scxml/ScXML.h>

#include "scxml/ScXMLP.h"

// *************************************************************************

ScXMLEltReader::ScXMLEltReader(const char * eltname)
: element(eltname)
{
}

ScXMLEltReader::~ScXMLEltReader(void)
{
}

void
ScXMLEltReader::setXMLAttributes(ScXMLElt * scxmlelt, cc_xml_elt * xmlelt)
{
  const int numattrs = cc_xml_elt_get_num_attributes(xmlelt);
  const cc_xml_attr ** attrs = cc_xml_elt_get_attributes(xmlelt);
  for (int c = 0; c < numattrs; ++c) {
    const char * attrname = cc_xml_attr_get_name(attrs[c]);
    const char * attrvalue = cc_xml_attr_get_value(attrs[c]);
    scxmlelt->setXMLAttribute(attrname, attrvalue);
  }
}

// *************************************************************************

class ScXMLElt::PImpl {
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

SCXML_ELEMENT_ABSTRACT_SOURCE(ScXMLElt);

void
ScXMLElt::initClass(void)
{
  SCXML_OBJECT_INIT_ABSTRACT_CLASS(ScXMLElt, ScXMLObject, "ScXMLObject");
}

void
ScXMLElt::cleanClass(void)
{
  ScXMLElt::classTypeId = SoType::badType();
}

ScXMLElt::ScXMLElt(void)
: containerptr(NULL)
{
}

ScXMLElt::~ScXMLElt(void)
{
  this->containerptr = NULL;
}

/*!
  This method associates a value with an attribute name.  This is
  used for parameter passing while reading by the SCXML reader, but
  can be used as a general-purpose attribute dictionary outside of
  that.

  If NULL is passed as the value, the attribute is removed.
*/
void
ScXMLElt::setXMLAttribute(const char * attribute, const char * value)
{
  assert(attribute);
  const SbName attrname(attribute); // uniqify on string pointer
  PImpl::AttributeMap::iterator it =
    PRIVATE(this)->attributemap.find(attrname.getString());
  if (it == PRIVATE(this)->attributemap.end()) {
    if (value) {
      char * valuedup = new char [ strlen(value) + 1 ];
      strcpy(valuedup, value);
      PRIVATE(this)->attributemap.insert(
        PImpl::AttributeEntry(attrname.getString(), valuedup));
    }
  } else {
    delete [] it->second;
    it->second = NULL;
    if (!value) {
      PRIVATE(this)->attributemap.erase(it);
    } else {
      it->second = new char [ strlen(value) + 1 ];
      strcpy(it->second, value);
    }
  }
}

/*!
  This method returns the string value set for an attribute, or
  NULL if not set.
*/
const char *
ScXMLElt::getXMLAttribute(const char * attribute) const
{
  const SbName attrname(attribute);
  PImpl::AttributeMap::const_iterator it =
    PRIVATE(this)->attributemap.find(attrname.getString());
  if (it != PRIVATE(this)->attributemap.end()) {
    return it->second;
  }
  return NULL;
}

/*!
  This method is called when the file reader has set all the XML
  attributes on the object, and wants the object to handle them and
  report if the values were OK or if a read error should be produced.

  \return TRUE if the attributes are OK, and FALSE on error.

  This base class implementation does nothing and just returns TRUE.
*/
SbBool
ScXMLElt::handleXMLAttributes(void)
{
  return TRUE;
}

/*!
  This method searches the SCXML structure for an element with the
  given attribute value for the given attribute.

  Returns NULL if nothing was found.

  This function needs to be reimplemented to traverse child elements.
*/
const ScXMLElt *
ScXMLElt::search(const char * attrname, const char * attrvalue) const
{
  assert(attrname && attrvalue);
  if (strstr(attrname, ":")) { // namespace attribute
    const char * val = this->getXMLAttribute(attrname);
    if (val != NULL) {
      if (strcmp(val, attrvalue) == 0) {
        return this;
      }
    }
  }
  return NULL;
}

/*!
  Returns a deep copy of the given SCXML element.
*/
ScXMLElt *
ScXMLElt::clone(void) const
{
  SoType elementtype(getTypeId());
  if (!elementtype.canCreateInstance()) { return NULL; }
  ScXMLElt * copy = static_cast<ScXMLElt *>(elementtype.createInstance());
  copy->copyContents(this);
  return copy;
}

void
ScXMLElt::copyContents(const ScXMLElt * COIN_UNUSED_ARG(rhs))
{
  // FIXME: copy XMLattribute map, otherwise xmlns attrs will be lost.
}

/*!
  Set the pointer to the parent ScXML object.
*/
void
ScXMLElt::setContainer(ScXMLElt * container)
{
  this->containerptr = container;
}

/*!
  This method returns TRUE if the object is contained within the given
  \a object argument. Also if they are the same, TRUE will be returned.
  Otherwise, FALSE is returned.
*/
SbBool
ScXMLElt::isContainedIn(const ScXMLElt * element) const
{
  const ScXMLElt * lineageobj = this;
  while (lineageobj) {
    if (lineageobj == element) return TRUE;
    lineageobj = lineageobj->getContainer();
  }
  return FALSE;
}

#undef PRIVATE
