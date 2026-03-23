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

#include <Inventor/scxml/ScXMLOnExitElt.h>

/*!
  \class ScXMLOnExitElt ScXMLOnExitElt.h Inventor/scxml/ScXMLOnExitElt.h
  \brief implements the &lt;onexit&gt; SCXML element.

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

class ScXMLOnExitEltReader : public ScXMLEltReader {
public:
  ScXMLOnExitEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);
};

ScXMLOnExitEltReader::ScXMLOnExitEltReader(void)
: ScXMLEltReader("onexit")
{
}

ScXMLElt *
ScXMLOnExitEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm)
{
  assert(container && xmlelt);
  ScXMLOnExitElt * onexit = new ScXMLOnExitElt;
  onexit->setContainer(container);
  this->setXMLAttributes(onexit, xmlelt);

  // handle XML attributes
  if (unlikely(!onexit->handleXMLAttributes())) {
    delete onexit;
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
      ScXMLElt * logobj = logreader->read(onexit, element, doc, sm);
      if (unlikely(!logobj)) {
        delete onexit;
        return NULL;
      }
      assert(logobj->isOfType(ScXMLLogElt::getClassTypeId()));
      onexit->addExecutable(static_cast<ScXMLLogElt *>(logobj));
    }
    else if (strcmp(elementtype, "event") == 0) {
      // <event> - zero or more times
      ScXMLEltReader * eventreader = ScXMLEventElt::getElementReader();
      assert(eventreader);
      ScXMLElt * eventobj = eventreader->read(onexit, element, doc, sm);
      if (unlikely(!eventobj)) {
        delete onexit;
        return NULL;
      }
      assert(eventobj->isOfType(ScXMLEventElt::getClassTypeId()));
      onexit->addExecutable(static_cast<ScXMLEventElt *>(eventobj));
    }
    else if (strcmp(elementtype, "assign") == 0) {
      // <assign> - zero or more times
      ScXMLEltReader * assignreader = ScXMLAssignElt::getElementReader();
      assert(assignreader);
      ScXMLElt * assignobj = assignreader->read(onexit, element, doc, sm);
      if (unlikely(!assignobj)) {
        delete onexit;
        return NULL;
      }
      assert(assignobj->isOfType(ScXMLAssignElt::getClassTypeId()));
      onexit->addExecutable(static_cast<ScXMLAssignElt *>(assignobj));
    }
    else if (strcmp(elementtype, "send") == 0) {
      // <send> - zero or more times
      ScXMLEltReader * sendreader = ScXMLSendElt::getElementReader();
      assert(sendreader);
      ScXMLElt * sendobj = sendreader->read(onexit, element, doc, sm);
      if (unlikely(!sendobj)) {
        delete onexit;
        return NULL;
      }
      assert(sendobj->isOfType(ScXMLSendElt::getClassTypeId()));
      onexit->addExecutable(static_cast<ScXMLSendElt *>(sendobj));
    }
    else if (strcmp(elementtype, "if") == 0) {
      // <if> - zero or more times
      ScXMLEltReader * ifreader = ScXMLIfElt::getElementReader();
      assert(ifreader);
      ScXMLElt * ifobj = ifreader->read(onexit, element, doc, sm);
      if (unlikely(!ifobj)) {
        delete onexit;
        return NULL;
      }
      assert(ifobj->isOfType(ScXMLIfElt::getClassTypeId()));
      onexit->addExecutable(static_cast<ScXMLIfElt *>(ifobj));
    }
    else if (strcmp(elementtype, "script") == 0) {
      // <script> - zero or more times
      ScXMLEltReader * scriptreader = ScXMLScriptElt::getElementReader();
      assert(scriptreader);
      ScXMLElt * scriptobj = scriptreader->read(onexit, element, doc, sm);
      if (unlikely(!scriptobj)) {
        delete onexit;
        return NULL;
      }
      assert(scriptobj->isOfType(ScXMLScriptElt::getClassTypeId()));
      onexit->addExecutable(static_cast<ScXMLScriptElt *>(scriptobj));
    }
    else if (strcmp(elementtype, "invoke") == 0) {
      // <invoke> - zero or more times
      ScXMLEltReader * invokereader = ScXMLInvokeElt::getElementReader();
      assert(invokereader);
      ScXMLElt * invokeobj = invokereader->read(onexit, element, doc, sm);
      if (unlikely(!invokeobj)) {
        delete onexit;
        return NULL;
      }
      assert(invokeobj->isOfType(ScXMLInvokeElt::getClassTypeId()));
      onexit->addExecutable(static_cast<ScXMLInvokeElt *>(invokeobj));
    }
    else {
      SoDebugError::post("ScXMLTransitionEltReader::read",
                         "unexpected XML element '<%s>' found in <onexit>",
                         elementtype);
      delete onexit;
      return NULL;
    }
  }
  return onexit;
}

// *************************************************************************

class ScXMLOnExitElt::PImpl {
public:
  ~PImpl(void)
  {
    SCXML__CLEAR_STD_VECTOR(this->executablelist, ScXMLExecutableElt *);
  }

  std::vector<ScXMLExecutableElt *> executablelist;
};

#define PRIVATE(obj) ((obj)->pimpl)

SCXML_ELEMENT_SOURCE(ScXMLOnExitElt);

void
ScXMLOnExitElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLOnExitElt, ScXMLElt, "ScXMLElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLOnExitElt, "onexit", ScXMLOnExitEltReader);
}

void
ScXMLOnExitElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLOnExitElt);
  ScXMLOnExitElt::classTypeId = SoType::badType();
}

ScXMLOnExitElt::ScXMLOnExitElt(void)
{
}

ScXMLOnExitElt::~ScXMLOnExitElt(void)
{
}

void
ScXMLOnExitElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  const ScXMLOnExitElt * orig = coin_assert_cast<const ScXMLOnExitElt *>(rhs);
  for (int c = 0; c < orig->getNumExecutables(); ++c) {
    ScXMLExecutableElt * executable =
      coin_assert_cast<ScXMLExecutableElt *>(orig->getExecutable(c)->clone());
    this->addExecutable(executable);
  }
}

const ScXMLElt *
ScXMLOnExitElt::search(const char * attrname, const char * attrvalue) const
{
  const ScXMLElt * hit = inherited::search(attrname, attrvalue);
  if (hit) {
    return hit;
  }
  return NULL;
}

// *************************************************************************
// executable content

SCXML_LIST_OBJECT_API_IMPL(ScXMLOnExitElt, ScXMLExecutableElt, PRIVATE(this)->executablelist, Executable, Executables);

void
ScXMLOnExitElt::execute(ScXMLStateMachine * statemachine) const
{
  std::vector<ScXMLExecutableElt *>::const_iterator it =
    PRIVATE(this)->executablelist.begin();
  while (it != PRIVATE(this)->executablelist.end()) {
    (*it)->execute(statemachine);
    ++it;
  }
}

#undef PRIVATE
