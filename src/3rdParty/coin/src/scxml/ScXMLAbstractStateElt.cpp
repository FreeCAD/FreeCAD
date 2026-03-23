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

#include <Inventor/scxml/ScXMLAbstractStateElt.h>

/*!
  \class ScXMLAbstractStateElt ScXMLAbstractStateElt.h Inventor/scxml/ScXMLAbstractStateElt
  \brief abstract base class for the SCXML 'state' elements.

  \ingroup coin_scxml
  \since Coin 3.1
*/

#include <cassert>

#include <Inventor/errors/SoDebugError.h>
#include "scxml/ScXMLCommonP.h"
#include "SbBasicP.h"

// *************************************************************************

class ScXMLAbstractStateElt::PImpl {
public:
};

#define PRIVATE

SCXML_ELEMENT_ABSTRACT_SOURCE(ScXMLAbstractStateElt);


void
ScXMLAbstractStateElt::initClass(void)
{
  SCXML_OBJECT_INIT_ABSTRACT_CLASS(ScXMLAbstractStateElt, ScXMLElt, "ScXMLElt");
}

void
ScXMLAbstractStateElt::cleanClass(void)
{
  ScXMLAbstractStateElt::classTypeId = SoType::badType();
}

ScXMLAbstractStateElt::ScXMLAbstractStateElt(void)
: id(NULL)
{
}

ScXMLAbstractStateElt::~ScXMLAbstractStateElt(void)
{
  this->setIdAttribute(NULL);
}

void
ScXMLAbstractStateElt::setIdAttribute(const char * idstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->id, "id", idstr);
  // FIXME: validate id for lexcal conformance?
}

SbBool
ScXMLAbstractStateElt::handleXMLAttributes(void)
{
  if (!inherited::handleXMLAttributes()) {
    return FALSE;
  }

  this->setIdAttribute(this->getXMLAttribute("id"));

  // validate
  if ((this->id == NULL) || (std::strcmp(this->id, "") == 0)) {
    SoDebugError::postInfo("ScXMLAbstractStateElt::handleXMLAttributes",
                           "state element must have 'id' attribute");
    return FALSE;
  }

  return TRUE;
}

void
ScXMLAbstractStateElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  const ScXMLAbstractStateElt * orig = coin_assert_cast<const ScXMLAbstractStateElt *>(rhs);
  this->setIdAttribute(orig->getIdAttribute());
}

const ScXMLElt *
ScXMLAbstractStateElt::search(const char * attrname, const char * attrvalue) const
{
  const ScXMLElt * hit = inherited::search(attrname, attrvalue);
  if (hit) {
    return hit;
  }
  if (strcmp(attrname, "id") == 0) {
    if (this->id && strcmp(attrvalue, this->id) == 0) {
      return this;
    }
  }
  return NULL;
}

#undef PRIVATE
