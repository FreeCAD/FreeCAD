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

#include <Inventor/scxml/ScXMLAnchorElt.h>

/*!
  \class ScXMLAnchorElt ScXMLAnchorElt.h Inventor/scxml/ScXMLAnchorElt.h
  \brief implements the &lt;anchor&gt; SCXML element.

  The &lt;anchor&gt; element is not supported yet other than as a
  dummy state.

  \since Coin 3.0
  \ingroup coin_scxml
*/

#include <cassert>
#include <cstring>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/scxml/ScXML.h>

#include "scxml/ScXMLCommonP.h"
#include "SbBasicP.h"

// *************************************************************************

class ScXMLAnchorEltReader : public ScXMLEltReader {
public:
  ScXMLAnchorEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);
};

ScXMLAnchorEltReader::ScXMLAnchorEltReader(void)
: ScXMLEltReader("anchor")
{
}

ScXMLElt *
ScXMLAnchorEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * COIN_UNUSED_ARG(doc), ScXMLStateMachine * COIN_UNUSED_ARG(sm))
{
  assert(container && xmlelt);
  ScXMLAnchorElt * anchor = new ScXMLAnchorElt;
  anchor->setContainer(container);
  this->setXMLAttributes(anchor, xmlelt);

  // handle XML attributes
  if (unlikely(!anchor->handleXMLAttributes())) {
    delete anchor;
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

    SoDebugError::post("ScXMLAnchorEltReader::read",
                       "unexpected XML element '<%s>' found in <anchor>",
                       elementtype);
    delete anchor;
    return NULL;
  }

  return anchor;
}

// *************************************************************************

class ScXMLAnchorElt::PImpl {
public:
};

#define PRIVATE

SCXML_ELEMENT_SOURCE(ScXMLAnchorElt);

void
ScXMLAnchorElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLAnchorElt, ScXMLElt, "ScXMLElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLAnchorElt, "anchor", ScXMLAnchorEltReader);
}

void
ScXMLAnchorElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLAnchorElt);
  ScXMLAnchorElt::classTypeId = SoType::badType();
}

ScXMLAnchorElt::ScXMLAnchorElt(void)
: type(NULL),
  snapshot(NULL)
{
}

ScXMLAnchorElt::~ScXMLAnchorElt(void)
{
  this->setTypeAttribute(NULL);
  this->setSnapshotAttribute(NULL);
}

void
ScXMLAnchorElt::setTypeAttribute(const char * typestr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->type, "type", typestr);
}

void
ScXMLAnchorElt::setSnapshotAttribute(const char * snapshotstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->snapshot, "snapshot", snapshotstr);
}

SbBool
ScXMLAnchorElt::handleXMLAttributes(void)
{
  if (!inherited::handleXMLAttributes()) return FALSE;

  this->setTypeAttribute(this->getXMLAttribute("type"));
  this->setSnapshotAttribute(this->getXMLAttribute("snapshot"));

  if (this->type == NULL) {
    return FALSE;
  }

  return TRUE;
}

void
ScXMLAnchorElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  const ScXMLAnchorElt * orig = coin_assert_cast<const ScXMLAnchorElt *>(rhs);
  this->setTypeAttribute(orig->getTypeAttribute());
  this->setSnapshotAttribute(orig->getSnapshotAttribute());
}

const ScXMLElt *
ScXMLAnchorElt::search(const char * attrname, const char * attrvalue) const
{
  const ScXMLElt * hit = inherited::search(attrname, attrvalue);
  if (hit) {
    return hit;
  }
  if (strcmp(attrname, "type") == 0) {
    if (this->type && strcmp(attrvalue, this->type) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "snapshot") == 0) {
    if (this->snapshot && strcmp(attrvalue, this->snapshot) == 0) {
      return this;
    }
  }
  return NULL;
}

#undef PRIVATE
