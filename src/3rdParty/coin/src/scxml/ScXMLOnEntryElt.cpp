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

#include <Inventor/scxml/ScXMLOnEntryElt.h>

/*!
  \class ScXMLOnEntryElt ScXMLOnEntryElt.h Inventor/scxml/ScXMLOnEntryElt.h
  \brief implements the &lt;onentry&gt; SCXML element.

  \since Coin 3.0
  \ingroup coin_scxml
*/

#include <cassert>
#include <algorithm>
#include <vector>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/ScXMLEventElt.h>
#include <Inventor/scxml/ScXMLIfElt.h>
#include <Inventor/scxml/ScXMLInvokeElt.h>
#include <Inventor/scxml/ScXMLLogElt.h>
#include <Inventor/scxml/ScXMLScriptElt.h>
#include <Inventor/scxml/ScXMLSendElt.h>
#include <Inventor/scxml/ScXMLAssignElt.h>

#include "scxml/ScXMLCommonP.h"

// *************************************************************************

class ScXMLOnEntryEltReader : public ScXMLEltReader {
public:
  ScXMLOnEntryEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);
};

ScXMLOnEntryEltReader::ScXMLOnEntryEltReader(void)
: ScXMLEltReader("onentry")
{
}

ScXMLElt *
ScXMLOnEntryEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm)
{
  assert(container && xmlelt);
  ScXMLOnEntryElt * onentry = new ScXMLOnEntryElt;
  onentry->setContainer(container);
  this->setXMLAttributes(onentry, xmlelt);

  // handle XML attributes
  if (unlikely(!onentry->handleXMLAttributes())) {
    delete onentry;
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

    if (strcmp(elementtype, "log") == 0) {
      // <log> - zero or more times
      ScXMLEltReader * logreader = ScXMLLogElt::getElementReader();
      assert(logreader);
      ScXMLElt * logobj = logreader->read(onentry, element, doc, sm);
      if (unlikely(!logobj)) {
        delete onentry;
        return NULL;
      }
      assert(logobj->isOfType(ScXMLLogElt::getClassTypeId()));
      onentry->addExecutable(static_cast<ScXMLLogElt *>(logobj));
    }
    else if (strcmp(elementtype, "event") == 0) {
      // <event> - zero or more times
      ScXMLEltReader * eventreader = ScXMLEventElt::getElementReader();
      assert(eventreader);
      ScXMLElt * eventobj = eventreader->read(onentry, element, doc, sm);
      if (unlikely(!eventobj)) {
        delete onentry;
        return NULL;
      }
      assert(eventobj->isOfType(ScXMLEventElt::getClassTypeId()));
      onentry->addExecutable(static_cast<ScXMLEventElt *>(eventobj));
    }
    else if (strcmp(elementtype, "assign") == 0) {
      // <assign> - zero or more times
      ScXMLEltReader * assignreader = ScXMLAssignElt::getElementReader();
      assert(assignreader);
      ScXMLElt * assignobj = assignreader->read(onentry, element, doc, sm);
      if (unlikely(!assignobj)) {
        delete onentry;
        return NULL;
      }
      assert(assignobj->isOfType(ScXMLAssignElt::getClassTypeId()));
      onentry->addExecutable(static_cast<ScXMLAssignElt *>(assignobj));
    }
    else if (strcmp(elementtype, "send") == 0) {
      // <send> - zero or more times
      ScXMLEltReader * sendreader = ScXMLSendElt::getElementReader();
      assert(sendreader);
      ScXMLElt * sendobj = sendreader->read(onentry, element, doc, sm);
      if (unlikely(!sendobj)) {
        delete onentry;
        return NULL;
      }
      assert(sendobj->isOfType(ScXMLSendElt::getClassTypeId()));
      onentry->addExecutable(static_cast<ScXMLSendElt *>(sendobj));
    }
    else if (strcmp(elementtype, "if") == 0) {
      // <if> - zero or more times
      ScXMLEltReader * ifreader = ScXMLIfElt::getElementReader();
      assert(ifreader);
      ScXMLElt * ifobj = ifreader->read(onentry, element, doc, sm);
      if (unlikely(!ifobj)) {
        delete onentry;
        return NULL;
      }
      assert(ifobj->isOfType(ScXMLIfElt::getClassTypeId()));
      onentry->addExecutable(static_cast<ScXMLIfElt *>(ifobj));
    }
    else if (strcmp(elementtype, "script") == 0) {
      // <script> - zero or more times
      ScXMLEltReader * scriptreader = ScXMLScriptElt::getElementReader();
      assert(scriptreader);
      ScXMLElt * scriptobj = scriptreader->read(onentry, element, doc, sm);
      if (unlikely(!scriptobj)) {
        delete onentry;
        return NULL;
      }
      assert(scriptobj->isOfType(ScXMLScriptElt::getClassTypeId()));
      onentry->addExecutable(static_cast<ScXMLScriptElt *>(scriptobj));
    }
    else if (strcmp(elementtype, "invoke") == 0) {
      // <invoke> - zero or more times
      ScXMLEltReader * invokereader = ScXMLInvokeElt::getElementReader();
      assert(invokereader);
      ScXMLElt * invokeobj = invokereader->read(onentry, element, doc, sm);
      if (unlikely(!invokeobj)) {
        delete onentry;
        return NULL;
      }
      assert(invokeobj->isOfType(ScXMLInvokeElt::getClassTypeId()));
      onentry->addExecutable(static_cast<ScXMLInvokeElt *>(invokeobj));
    }
    else {
      SoDebugError::post("ScXMLTransitionEltReader::read",
                         "unexpected XML element '<%s>' found in <onentry>",
                         elementtype);
      delete onentry;
      return NULL;
    }
  }
  return onentry;
} // read()

// *************************************************************************

class ScXMLOnEntryElt::PImpl {
public:
  ~PImpl(void)
  {
    SCXML__CLEAR_STD_VECTOR(this->executablelist, ScXMLExecutableElt *);
  }

  std::vector<ScXMLExecutableElt *> executablelist;
};

#define PRIVATE(obj) ((obj)->pimpl)

SCXML_ELEMENT_SOURCE(ScXMLOnEntryElt);

void
ScXMLOnEntryElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLOnEntryElt, ScXMLElt, "ScXMLElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLOnEntryElt, "onentry", ScXMLOnEntryEltReader);
}

void
ScXMLOnEntryElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLOnEntryElt);
  ScXMLOnEntryElt::classTypeId = SoType::badType();
}

ScXMLOnEntryElt::ScXMLOnEntryElt(void)
{
}

ScXMLOnEntryElt::~ScXMLOnEntryElt(void)
{
}

void
ScXMLOnEntryElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  const ScXMLOnEntryElt * orig = coin_assert_cast<const ScXMLOnEntryElt *>(rhs);
  for (int c = 0; c < orig->getNumExecutables(); ++c) {
    ScXMLExecutableElt * executable =
      coin_assert_cast<ScXMLExecutableElt *>(orig->getExecutable(c)->clone());
    this->addExecutable(executable);
  }
}

const ScXMLElt *
ScXMLOnEntryElt::search(const char * attrname, const char * attrvalue) const
{
  const ScXMLElt * hit = inherited::search(attrname, attrvalue);
  if (hit) {
    return hit;
  }
  return NULL;
}

// executable content

SCXML_LIST_OBJECT_API_IMPL(ScXMLOnEntryElt, ScXMLExecutableElt, PRIVATE(this)->executablelist, Executable, Executables);

/*!
*/
void
ScXMLOnEntryElt::execute(ScXMLStateMachine * statemachine) const
{
  std::vector<ScXMLExecutableElt *>::const_iterator it =
    PRIVATE(this)->executablelist.begin();
  while (it != PRIVATE(this)->executablelist.end()) {
    (*it)->execute(statemachine);
    ++it;
  }
}

#undef PRIVATE
