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

#include <Inventor/scxml/ScXMLScriptElt.h>

/*!
  \class ScXMLScriptElt ScXMLScriptElt.h Inventor/scxml/ScXMLScriptElt.h
  \brief implements the &lt;script&gt; SCXML element.

  \ingroup coin_scxml
  \since Coin 3.1
*/

#include <cassert>
#include <cstring>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/XML/element.h>

#include "coindefs.h"
#include "SbBasicP.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::strcmp;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

// *************************************************************************

class ScXMLScriptEltReader : public ScXMLEltReader {
public:
  ScXMLScriptEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);
};

ScXMLScriptEltReader::ScXMLScriptEltReader(void)
: ScXMLEltReader("script")
{
}

ScXMLElt *
ScXMLScriptEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * COIN_UNUSED_ARG(doc), ScXMLStateMachine * COIN_UNUSED_ARG(sm))
{
  assert(container && xmlelt);
  ScXMLScriptElt * script = new ScXMLScriptElt;
  script->setContainer(container);
  this->setXMLAttributes(script, xmlelt);

  // handle XML attributes
  if (unlikely(!script->handleXMLAttributes())) {
    delete script;
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

    SoDebugError::post("ScXMLScriptEltReader::read",
                       "<script> contains unexpected <%s> element", elementtype);
    delete script;
    return NULL;
   }

  return script;
}

// *************************************************************************

class ScXMLScriptElt::PImpl {
public:
};

#define PRIVATE

SCXML_ELEMENT_SOURCE(ScXMLScriptElt);

void
ScXMLScriptElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLScriptElt, ScXMLExecutableElt, "ScXMLExecutableElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLScriptElt, "script", ScXMLScriptEltReader);
}

void
ScXMLScriptElt::cleanClass(void)
{
}

ScXMLScriptElt::ScXMLScriptElt(void)
{
}

ScXMLScriptElt::~ScXMLScriptElt(void)
{
}

void
ScXMLScriptElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  /*const ScXMLScriptElt * orig = */coin_assert_cast<const ScXMLScriptElt *>(rhs);
}

void
ScXMLScriptElt::execute(ScXMLStateMachine * COIN_UNUSED_ARG(statemachine)) const
{
  // get evaluator (profile-determined) from statemachine
  // evaluate script
}

#undef PRIVATE
