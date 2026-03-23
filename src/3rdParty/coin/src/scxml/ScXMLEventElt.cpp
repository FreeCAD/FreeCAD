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

#include <Inventor/scxml/ScXMLEventElt.h>

/*!
  \class ScXMLEventElt ScXMLEventElt.h Inventor/scxml/ScXMLEventElt.h
  \brief implements the &lt;event&gt; SCXML element.

  \since Coin 3.1
  \ingroup coin_scxml
*/

#include <cassert>
#include <cstring>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/ScXMLEvent.h>
#include <Inventor/scxml/ScXMLStateMachine.h>

#include "scxml/ScXMLCommonP.h"
#include "SbBasicP.h"

// *************************************************************************

class ScXMLEventEltReader : public ScXMLEltReader {
public:
  ScXMLEventEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);
};

ScXMLEventEltReader::ScXMLEventEltReader(void)
: ScXMLEltReader("event")
{
}

ScXMLElt *
ScXMLEventEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * COIN_UNUSED_ARG(doc), ScXMLStateMachine * COIN_UNUSED_ARG(sm))
{
  assert(container && xmlelt);
  ScXMLEventElt * event = new ScXMLEventElt;
  event->setContainer(container);
  this->setXMLAttributes(event, xmlelt);

  // handle XML attributes
  if (unlikely(!event->handleXMLAttributes())) {
    delete event;
    return NULL;
  }

  const int numchildren = cc_xml_elt_get_num_children(xmlelt);
  for (int c = 0; c < numchildren; ++c) {
    cc_xml_elt * element = cc_xml_elt_get_child(xmlelt, c);
    const char * elementtype = cc_xml_elt_get_type(element);

    if (strcmp(elementtype, COIN_XML_CDATA_TYPE) == 0) {
      // ignore CDATA
      continue;
    }

    SoDebugError::post("ScXMLEventEltReader::read",
                       "<event> contains unexpected <%s> element", elementtype);
    delete event;
    return NULL;
  }

  return event;
}

// *************************************************************************

class ScXMLEventElt::PImpl {
public:
};

#define PRIVATE


SCXML_ELEMENT_SOURCE(ScXMLEventElt);

void
ScXMLEventElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLEventElt, ScXMLExecutableElt, "ScXMLExecutableElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLEventElt, "event", ScXMLEventEltReader);
}

void
ScXMLEventElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLEventElt);
  ScXMLEventElt::classTypeId = SoType::badType();
}

ScXMLEventElt::ScXMLEventElt(void)
: name(NULL)
{
}

ScXMLEventElt::~ScXMLEventElt(void)
{
  this->setNameAttribute(NULL);
}

void
ScXMLEventElt::setNameAttribute(const char * namestr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->name, "name", namestr);
}

// const char * ScXMLEventElt::getNameAttribute(void) const

SbBool
ScXMLEventElt::handleXMLAttributes(void)
{
  if (!inherited::handleXMLAttributes()) {
    return FALSE;
  }

  this->setNameAttribute(this->getXMLAttribute("name"));

  return TRUE;
}

void
ScXMLEventElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  const ScXMLEventElt * orig = coin_assert_cast<const ScXMLEventElt *>(rhs);
  this->setNameAttribute(orig->getNameAttribute());
}


const ScXMLElt *
ScXMLEventElt::search(const char * attrname, const char * attrvalue) const
{
  const ScXMLElt * hit = inherited::search(attrname, attrvalue);
  if (hit) {
    return hit;
  }
  if (strcmp(attrname, "name") == 0) {
    if (this->name && (strcmp(attrvalue, this->name) == 0)) {
      return this;
    }
  }
  return NULL;
}

ScXMLEvent *
ScXMLEventElt::createEvent(ScXMLEventTarget * COIN_UNUSED_ARG(host)) const
{
  ScXMLEvent * event = new ScXMLEvent;
  event->setEventName(this->getNameAttribute());
  return event;
}

void
ScXMLEventElt::execute(ScXMLStateMachine * statemachine) const
{
  inherited::execute(statemachine);
  statemachine->sendInternalEvent(this);
}

#undef PRIVATE
