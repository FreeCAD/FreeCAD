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

#include <Inventor/scxml/ScXMLContentElt.h>

/*!
  \class ScXMLContentElt Inventor/scxml/ScXMLContentElt.h
  \brief implements the &lt;content&gt; SCXML element.

  \ingroup coin_scxml
*/

#include <cassert>
#include <cstring>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/XML/element.h>

#include "coindefs.h"
#include "scxml/ScXMLCommonP.h"
#include "SbBasicP.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::strcmp;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

// *************************************************************************

class ScXMLContentEltReader : public ScXMLEltReader {
public:
  ScXMLContentEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);
};

ScXMLContentEltReader::ScXMLContentEltReader(void)
: ScXMLEltReader("content")
{
}

ScXMLElt *
ScXMLContentEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * COIN_UNUSED_ARG(doc), ScXMLStateMachine * COIN_UNUSED_ARG(sm))
{
  assert(container && xmlelt);
  ScXMLContentElt * content = new ScXMLContentElt;
  content->setContainer(container);
  this->setXMLAttributes(content, xmlelt);

  // handle XML attributes
  if (unlikely(!content->handleXMLAttributes())) {
    delete content;
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

    SoDebugError::post("ScXMLContentEltReader::read",
                       "unexpected XML element '<%s>' found in <content>",
                       elementtype);
    delete content;
    return NULL;
  }


  return content;
}

// *************************************************************************

class ScXMLContentElt::PImpl {
public:
};

#define PRIVATE

SCXML_ELEMENT_SOURCE(ScXMLContentElt);

void
ScXMLContentElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLContentElt, ScXMLElt, "ScXMLElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLContentElt, "content", ScXMLContentEltReader);
}

void
ScXMLContentElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLContentElt);
  ScXMLContentElt::classTypeId = SoType::badType();
}

ScXMLContentElt::ScXMLContentElt(void)
{
}

ScXMLContentElt::~ScXMLContentElt(void)
{
}

void
ScXMLContentElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  /*const ScXMLContentElt * orig = */coin_assert_cast<const ScXMLContentElt *>(rhs);
}

#undef PRIVATE
