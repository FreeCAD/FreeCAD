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

#include <Inventor/scxml/ScXMLFinalizeElt.h>

/*!
  \class ScXMLFinalizeElt ScXMLFinalizeElt.h Inventor/scxml/ScXMLFinalizeElt.h
  \brief implements the &lt;finalize&gt; SCXML element.

  \ingroup coin_scxml
*/

#include <cassert>
#include <cstring>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/XML/element.h>

#include "scxml/ScXMLCommonP.h"
#include "SbBasicP.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::strcmp;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

// *************************************************************************

class ScXMLFinalizeEltReader : public ScXMLEltReader {
public:
  ScXMLFinalizeEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);
};

ScXMLFinalizeEltReader::ScXMLFinalizeEltReader(void)
: ScXMLEltReader("finalize")
{
}

ScXMLElt *
ScXMLFinalizeEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * COIN_UNUSED_ARG(doc), ScXMLStateMachine * COIN_UNUSED_ARG(sm))
{
  assert(container && xmlelt);
  ScXMLFinalizeElt * finalize = new ScXMLFinalizeElt;
  finalize->setContainer(container);
  this->setXMLAttributes(finalize, xmlelt);

  // handle XML attributes
  if (unlikely(!finalize->handleXMLAttributes())) {
    delete finalize;
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

    SoDebugError::post("ScXMLFinalizeEltReader::read",
                       "<finalize> contains unexpected <%s> element", elementtype);
    delete finalize;
    return NULL;
  }

  return finalize;
}

// *************************************************************************

class ScXMLFinalizeElt::PImpl {
public:
};

#define PRIVATE

SCXML_ELEMENT_SOURCE(ScXMLFinalizeElt);

void
ScXMLFinalizeElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLFinalizeElt, ScXMLElt, "ScXMLElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLFinalizeElt, "finalize", ScXMLFinalizeEltReader);
}

void
ScXMLFinalizeElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLFinalizeElt);
  ScXMLFinalizeElt::classTypeId = SoType::badType();
}

ScXMLFinalizeElt::ScXMLFinalizeElt(void)
{
}

ScXMLFinalizeElt::~ScXMLFinalizeElt(void)
{
}

void
ScXMLFinalizeElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  /*const ScXMLFinalizeElt * orig = */coin_assert_cast<const ScXMLFinalizeElt *>(rhs);
}

#undef PRIVATE
