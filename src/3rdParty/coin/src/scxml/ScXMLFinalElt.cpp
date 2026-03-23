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

#include <Inventor/scxml/ScXMLFinalElt.h>

/*!
  \class ScXMLFinalElt ScXMLFinalElt.h Inventor/scxml/ScXMLFinalElt.h
  \brief implements the &lt;final&gt; SCXML element.

  A &lt;final&gt; element takes an "id" attribute (required), and can
  have one child of type &lt;onentry&gt; and &lt;onexit&gt;.

  \since Coin 3.0
  \ingroup coin_scxml
*/

#include <cassert>
#include <cstring>

#include <memory>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/ScXMLOnEntryElt.h>
#include <Inventor/scxml/ScXMLOnExitElt.h>

#include "scxml/ScXMLCommonP.h"

// *************************************************************************

class ScXMLFinalEltReader : public ScXMLEltReader {
public:
  ScXMLFinalEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);

};

ScXMLFinalEltReader::ScXMLFinalEltReader(void)
: ScXMLEltReader("final")
{
}

ScXMLElt *
ScXMLFinalEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm)
{
  assert(container && xmlelt);

  ScXMLFinalElt * final = new ScXMLFinalElt;
  final->setContainer(container);
  this->setXMLAttributes(final, xmlelt);

  // handle XML attributes
  if (unlikely(!final->handleXMLAttributes())) {
    delete final;
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

    if (strcmp(elementtype, "onentry") == 0) {
      if (unlikely(final->getOnEntry())) {
        SoDebugError::post("ScXMLFinalEltReader::read",
                           "<final> element can only contain one <onentry> element");
        delete final;
        return NULL;
      }

      ScXMLEltReader * onentryreader = ScXMLOnEntryElt::getElementReader();
      assert(onentryreader);
      ScXMLElt * onentryelt = onentryreader->read(final, element, doc, sm);

      if (unlikely(!onentryelt)) {
        delete final;
        return NULL;
      }
      assert(final->isOfType(ScXMLOnEntryElt::getClassTypeId()));
      final->setOnEntry(static_cast<ScXMLOnEntryElt *>(onentryelt));
    }

    else if (strcmp(elementtype, "onexit") == 0) {
      if (unlikely(final->getOnExit())) {
        SoDebugError::post("ScXMLFinalEltReader::read",
                           "<final> element can only contain one <onexit> element");
        delete final;
        return NULL;
      }

      ScXMLEltReader * onexitreader = ScXMLOnExitElt::getElementReader();
      assert(onexitreader);
      ScXMLElt * onexitelt = onexitreader->read(final, element, doc, sm);

      if (unlikely(!onexitelt)) {
        delete final;
        return NULL;
      }
      assert(onexitelt->isOfType(ScXMLOnExitElt::getClassTypeId()));
      final->setOnExit(static_cast<ScXMLOnExitElt *>(onexitelt));
    }

    else {
      SoDebugError::post("ScXMLFinalEltReader::read",
                         "unexpected XML element '<%s>' found in <final>",
                         elementtype);
      delete final;
      return NULL;
    }
  }

  return final;
} // readScXMLFinal

// *************************************************************************

class ScXMLFinalElt::PImpl {
public:
  PImpl(void) { }

  std::unique_ptr<ScXMLOnEntryElt> onentry;
  std::unique_ptr<ScXMLOnExitElt> onexit;

};

#define PRIVATE(obj) ((obj)->pimpl)

SCXML_ELEMENT_SOURCE(ScXMLFinalElt);

void
ScXMLFinalElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLFinalElt, ScXMLAbstractStateElt, "ScXMLAbstractStateElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLFinalElt, "final", ScXMLFinalEltReader);
}

void
ScXMLFinalElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLFinalElt);
  ScXMLFinalElt::classTypeId = SoType::badType();
}

ScXMLFinalElt::ScXMLFinalElt(void)
{
}

ScXMLFinalElt::~ScXMLFinalElt(void)
{
}

void
ScXMLFinalElt::setOnEntry(ScXMLOnEntryElt * onentry)
{
  PRIVATE(this)->onentry.reset(onentry);
}

ScXMLOnEntryElt *
ScXMLFinalElt::getOnEntry(void) const
{
  return PRIVATE(this)->onentry.get();
}

void
ScXMLFinalElt::setOnExit(ScXMLOnExitElt * onexit)
{
  PRIVATE(this)->onexit.reset(onexit);
}

ScXMLOnExitElt *
ScXMLFinalElt::getOnExit(void) const
{
  return PRIVATE(this)->onexit.get();
}

void
ScXMLFinalElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  const ScXMLFinalElt * orig = coin_assert_cast<const ScXMLFinalElt *>(rhs);
  if (orig->getOnEntry()) {
    ScXMLOnEntryElt * onentry =
      coin_assert_cast<ScXMLOnEntryElt *>(orig->getOnEntry()->clone());
    this->setOnEntry(onentry);
  }
  if (orig->getOnExit()) {
    ScXMLOnExitElt * onexit =
      coin_assert_cast<ScXMLOnExitElt *>(orig->getOnExit()->clone());
    this->setOnExit(onexit);
  }
}

const ScXMLElt *
ScXMLFinalElt::search(const char * attrname, const char * attrvalue) const
{
  const ScXMLElt * hit = inherited::search(attrname, attrvalue);
  if (hit) {
    return hit;
  }
  if (PRIVATE(this)->onentry.get()) {
    hit = PRIVATE(this)->onentry->search(attrname, attrvalue);
    if (hit) {
      return hit;
    }
  }
  if (PRIVATE(this)->onexit.get()) {
    hit = PRIVATE(this)->onexit->search(attrname, attrvalue);
    if (hit) {
      return hit;
    }
  }
  return NULL;
}

#undef PRIVATE
