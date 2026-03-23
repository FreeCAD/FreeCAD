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

#include <Inventor/scxml/ScXMLParallelElt.h>

/*!
  \class ScXMLParallelElt ScXMLParallelElt.h Inventor/scxml/ScXMLParallelElt.h
  \brief implements the &lt;parallel&gt; SCXML element.

  \since Coin 3.0
  \ingroup coin_scxml
*/

#include <cassert>
#include <cstring>
#include <algorithm>
#include <vector>

#include <memory>

#include <Inventor/C/tidbits.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/ScXMLDocument.h>
#include <Inventor/scxml/ScXMLInvokeElt.h>
#include <Inventor/scxml/ScXMLOnExitElt.h>
#include <Inventor/scxml/ScXMLOnEntryElt.h>
#include <Inventor/scxml/ScXMLTransitionElt.h>
#include <Inventor/scxml/ScXMLStateElt.h>
#include <Inventor/scxml/ScXMLInitialElt.h>
#include <Inventor/scxml/ScXMLFinalElt.h>
#include <Inventor/scxml/ScXMLHistoryElt.h>
#include <Inventor/scxml/ScXMLAnchorElt.h>
#include <Inventor/scxml/ScXMLDataModelElt.h>
#include <Inventor/scxml/ScXMLScxmlElt.h>

#include "scxml/ScXMLCommonP.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::strlen;
using std::strcpy;
using std::strcmp;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

// *************************************************************************

class ScXMLParallelEltReader : public ScXMLEltReader {
public:
  ScXMLParallelEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);
};

ScXMLParallelEltReader::ScXMLParallelEltReader(void)
: ScXMLEltReader("parallel")
{
}

namespace { namespace ScXMLParallelNS {

template <class Type>
Type *
clone(Type * objptr)
{
  return coin_assert_cast<Type *>(objptr->clone());
}

} }

ScXMLElt *
ScXMLParallelEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm)
{
  assert(container && xmlelt);
  ScXMLParallelElt * parallel = new ScXMLParallelElt;
  parallel->setContainer(container);
  this->setXMLAttributes(parallel, xmlelt);

  // handle XML attributes
  if (unlikely(!parallel->handleXMLAttributes())) {
    SoDebugError::post("ScXMLParallelEltReader::read",
                       "invalid XML attributes");
    delete parallel;
    return NULL;
  }

  const char * extref = parallel->getSrcAttribute();
  if ((extref != NULL) && (extref[0] != '\0')) {
    SbString reference(extref);
    char * eltnameref = const_cast<char *>(strchr(reference.getString(), '#'));
    if (eltnameref) {
      eltnameref[0] = '\0';
      ++eltnameref;
    }
    ScXMLDocument * refdoc = ScXMLDocument::readFile(reference.getString());
    if (refdoc) {
      ScXMLElt * parentelt = refdoc->getRoot();
      if (eltnameref) {
        parentelt = refdoc->getStateById(SbName(eltnameref));
      }
      if (parentelt) {
        if (parentelt->isOfType(ScXMLScxmlElt::getClassTypeId())) {
          ScXMLScxmlElt * parent =
            coin_assert_cast<ScXMLScxmlElt *>(parentelt);
          int c = 0;
#if 0
          if (parent->getInitial()) {
            ScXMLInitialElt * initialelt =
              coin_assert_cast<ScXMLInitialElt *>(parent->getInitial()->clone());
            parallel->setInitial(initialelt);
          }
#endif
          for (c = 0; c < parent->getNumStates(); ++c) {
            ScXMLStateElt * stateelt = ::ScXMLParallelNS::clone(parent->getState(c));
//            coin_assert_cast<ScXMLStateElt *>(parent->getState(c)->clone());
            parallel->addState(stateelt);
          }
          for (c = 0; c < parent->getNumParallels(); ++c) {
            ScXMLParallelElt * parallelelt =
              coin_assert_cast<ScXMLParallelElt *>(parent->getParallel(c)->clone());
            parallel->addParallel(parallelelt);
          }
          for (c = 0; c < parent->getNumFinals(); ++c) {
            ScXMLFinalElt * finalelt =
              coin_assert_cast<ScXMLFinalElt *>(parent->getFinal(c)->clone());
            parallel->addFinal(finalelt);
          }
          if (parent->getDataModel()) {
            ScXMLDataModelElt * datamodelelt =
              coin_assert_cast<ScXMLDataModelElt *>(parent->getDataModel()->clone());
            parallel->setDataModel(datamodelelt);
          }
#if 0
          for (c = 0; c < parent->getNumScripts(); ++c) {
            ScXMLScriptElt * scriptelt =
              coin_assert_cast<ScXMLScriptElt *>(parent->getScript(c)->clone());
            parallel->addScript(scriptelt);
          }
#endif
        }
        else if (parentelt->isOfType(ScXMLStateElt::getClassTypeId())) {
          ScXMLStateElt * parent =
            coin_assert_cast<ScXMLStateElt *>(parentelt);
          int c = 0;
          if (parent->getOnEntry()) {
            ScXMLOnEntryElt * onentryelt =
              coin_assert_cast<ScXMLOnEntryElt *>(parent->getOnEntry()->clone());
            parallel->setOnEntry(onentryelt);
          }
          if (parent->getOnExit()) {
            ScXMLOnExitElt * onexitelt =
              coin_assert_cast<ScXMLOnExitElt *>(parent->getOnExit()->clone());
            parallel->setOnExit(onexitelt);
          }
          for (c = 0; c < parent->getNumTransitions(); ++c) {
            ScXMLTransitionElt * transitionelt =
              coin_assert_cast<ScXMLTransitionElt *>(parent->getTransition(c)->clone());
            parallel->addTransition(transitionelt);
          }
#if 0
          if (parent->getInitial()) {
            ScXMLInitialElt * initialelt =
              coin_assert_cast<ScXMLInitialElt *>(parent->getInitial()->clone());
            parallel->setInitial(initialelt);
          }
#endif
          for (c = 0; c < parent->getNumStates(); ++c) {
            ScXMLStateElt * stateelt =
              coin_assert_cast<ScXMLStateElt *>(parent->getState(c)->clone());
            parallel->addState(stateelt);
          }
          for (c = 0; c < parent->getNumParallels(); ++c) {
            ScXMLParallelElt * parallelelt =
              coin_assert_cast<ScXMLParallelElt *>(parent->getParallel(c)->clone());
            parallel->addParallel(parallelelt);
          }
          for (c = 0; c < parent->getNumFinals(); ++c) {
            ScXMLFinalElt * finalelt =
              coin_assert_cast<ScXMLFinalElt *>(parent->getFinal(c)->clone());
            parallel->addFinal(finalelt);
          }
          for (c = 0; c < parent->getNumHistories(); ++c) {
            ScXMLHistoryElt * historyelt =
              coin_assert_cast<ScXMLHistoryElt *>(parent->getHistory(c)->clone());
            parallel->addHistory(historyelt);
          }
          for (c = 0; c < parent->getNumAnchors(); ++c) {
            ScXMLAnchorElt * anchorelt =
              coin_assert_cast<ScXMLAnchorElt *>(parent->getAnchor(c)->clone());
            parallel->addAnchor(anchorelt);
          }
          if (parent->getDataModel()) {
            ScXMLDataModelElt * datamodelelt =
              coin_assert_cast<ScXMLDataModelElt *>(parent->getDataModel()->clone());
            parallel->setDataModel(datamodelelt);
          }
        }
        else if (parentelt->isOfType(ScXMLParallelElt::getClassTypeId())) {
          ScXMLParallelElt * parent =
            coin_assert_cast<ScXMLParallelElt *>(parentelt);
          int c = 0;
          if (parent->getOnEntry()) {
            ScXMLOnEntryElt * onentryelt =
              coin_assert_cast<ScXMLOnEntryElt *>(parent->getOnEntry()->clone());
            parallel->setOnEntry(onentryelt);
          }
          if (parent->getOnExit()) {
            ScXMLOnExitElt * onexitelt =
              coin_assert_cast<ScXMLOnExitElt *>(parent->getOnExit()->clone());
            parallel->setOnExit(onexitelt);
          }
          for (c = 0; c < parent->getNumTransitions(); ++c) {
            ScXMLTransitionElt * transitionelt =
              coin_assert_cast<ScXMLTransitionElt *>(parent->getTransition(c)->clone());
            parallel->addTransition(transitionelt);
          }
#if 0
          if (parent->getInitial()) {
            ScXMLInitialElt * initialelt =
              coin_assert_cast<ScXMLInitialElt *>(parent->getInitial()->clone());
            parallel->setInitial(initialelt);
          }
#endif
          for (c = 0; c < parent->getNumStates(); ++c) {
            ScXMLStateElt * stateelt =
              coin_assert_cast<ScXMLStateElt *>(parent->getState(c)->clone());
            parallel->addState(stateelt);
          }
          for (c = 0; c < parent->getNumParallels(); ++c) {
            ScXMLParallelElt * parallelelt =
              coin_assert_cast<ScXMLParallelElt *>(parent->getParallel(c)->clone());
            parallel->addParallel(parallelelt);
          }
          for (c = 0; c < parent->getNumFinals(); ++c) {
            ScXMLFinalElt * finalelt =
              coin_assert_cast<ScXMLFinalElt *>(parent->getFinal(c)->clone());
            parallel->addFinal(finalelt);
          }
          for (c = 0; c < parent->getNumHistories(); ++c) {
            ScXMLHistoryElt * historyelt =
              coin_assert_cast<ScXMLHistoryElt *>(parent->getHistory(c)->clone());
            parallel->addHistory(historyelt);
          }
          for (c = 0; c < parent->getNumAnchors(); ++c) {
            ScXMLAnchorElt * anchorelt =
              coin_assert_cast<ScXMLAnchorElt *>(parent->getAnchor(c)->clone());
            parallel->addAnchor(anchorelt);
          }
          if (parent->getDataModel()) {
            ScXMLDataModelElt * datamodelelt =
              coin_assert_cast<ScXMLDataModelElt *>(parent->getDataModel()->clone());
            parallel->setDataModel(datamodelelt);
          }
        }
        else if (parentelt->isOfType(ScXMLFinalElt::getClassTypeId())) {
          // huh?
        }
      }
      delete refdoc;
    }
  }

  // read in all children, and recurse down
  const int numchildren = cc_xml_elt_get_num_children(xmlelt);
  for (int c = 0; c < numchildren; ++c) {
    cc_xml_elt * element = cc_xml_elt_get_child(xmlelt, c);
    const char * elementtype = cc_xml_elt_get_type(element);

    if (strcmp(elementtype, COIN_XML_CDATA_TYPE) == 0) {
      // ignore CDATA
      continue;
    }

    if (strcmp(elementtype, "onentry") == 0) {
      // <onentry> - zero or one time
      if (unlikely(parallel->getOnEntry())) {
        SoDebugError::post("ScXMLParallelEltReader::read",
                           "<parallel> elements can only have one <onentry> element");
        delete parallel;
        return NULL;
      }
      ScXMLEltReader * onentryreader = ScXMLOnEntryElt::getElementReader();
      ScXMLElt * onentryobj = onentryreader->read(parallel, element, doc, sm);
      if (unlikely(!onentryobj)) {
        delete parallel;
        return NULL;
      }
      assert(onentryobj->isOfType(ScXMLOnEntryElt::getClassTypeId()));
      parallel->setOnEntry(static_cast<ScXMLOnEntryElt *>(onentryobj));
    }

    else if (strcmp(elementtype, "onexit") == 0) {
      // <onexit> - zero or one time
      if (unlikely(parallel->getOnExit())) {
        SoDebugError::post("ScXMLParallelEltReader::read",
                           "<parallel> elements can only have one <onexit> element");
        delete parallel;
        return NULL;
      }
      ScXMLEltReader * onexitreader = ScXMLOnExitElt::getElementReader();
      ScXMLElt * onexitobj = onexitreader->read(parallel, element, doc, sm);
      if (unlikely(!onexitobj)) {
        delete parallel;
        return NULL;
      }
      assert(onexitobj->isOfType(ScXMLOnExitElt::getClassTypeId()));
      parallel->setOnExit(static_cast<ScXMLOnExitElt *>(onexitobj));
    }

    else if (strcmp(elementtype, "transition") == 0) {
      // <transition> - zero or more times
      ScXMLEltReader * transitionreader = ScXMLTransitionElt::getElementReader();
      ScXMLElt * transitionobj = transitionreader->read(parallel, element, doc, sm);
      if (unlikely(!transitionobj)) {
        delete parallel;
        return NULL;
      }
      assert(transitionobj->isOfType(ScXMLTransitionElt::getClassTypeId()));
      parallel->addTransition(static_cast<ScXMLTransitionElt *>(transitionobj));
    }

    else if (strcmp(elementtype, "initial") == 0) {
      // <initial> - must occur iff states+parallels >= 1
      if (unlikely(parallel->getInitial())) {
        SoDebugError::post("ScXMLInitialEltReader::read",
                           "<parallel> elements can contain only one <initial> element");
        delete parallel;
        return NULL;
      }
      ScXMLEltReader * initialreader = ScXMLInitialElt::getElementReader();
      ScXMLElt * initialobj = initialreader->read(parallel, element, doc, sm);
      if (unlikely(!initialobj)) {
        delete parallel;
        return NULL;
      }
      assert(initialobj->isOfType(ScXMLInitialElt::getClassTypeId()));
      parallel->setInitial(static_cast<ScXMLInitialElt *>(initialobj));
    }

    else if (strcmp(elementtype, "state") == 0) {
      // <state> - zero or more times
      ScXMLEltReader * statereader = ScXMLStateElt::getElementReader();
      ScXMLElt * stateobj = statereader->read(parallel, element, doc, sm);
      if (unlikely(!stateobj)) {
        delete parallel;
        return NULL;
      }
      assert(stateobj->isOfType(ScXMLStateElt::getClassTypeId()));
      parallel->addState(static_cast<ScXMLStateElt *>(stateobj));
    }

    else if (strcmp(elementtype, "parallel") == 0) {
      // <parallel> - zero or more times
      ScXMLEltReader * parallelreader = ScXMLParallelElt::getElementReader();
      ScXMLElt * parallelobj = parallelreader->read(parallel, element, doc, sm);
      if (unlikely(!parallelobj)) {
        delete parallel;
        return NULL;
      }
      assert(parallelobj->isOfType(ScXMLParallelElt::getClassTypeId()));
      parallel->addParallel(static_cast<ScXMLParallelElt *>(parallelobj));
    }

    else if (strcmp(elementtype, "final") == 0) {
      // <final> - zero or more times
      ScXMLEltReader * finalreader = ScXMLFinalElt::getElementReader();
      ScXMLElt * finalobj = finalreader->read(parallel, element, doc, sm);
      if (unlikely(!finalobj)) {
        delete parallel;
        return NULL;
      }
      assert(finalobj->isOfType(ScXMLFinalElt::getClassTypeId()));
      parallel->addFinal(static_cast<ScXMLFinalElt *>(finalobj));
    }

    else if (strcmp(elementtype, "history") == 0) {
      // <history> - zero or more times
      ScXMLEltReader * historyreader = ScXMLHistoryElt::getElementReader();
      ScXMLElt * historyobj = historyreader->read(parallel, element, doc, sm);
      if (unlikely(!historyobj)) {
        delete parallel;
        return NULL;
      }
      assert(historyobj->isOfType(ScXMLHistoryElt::getClassTypeId()));
      parallel->addHistory(static_cast<ScXMLHistoryElt *>(historyobj));
    }

    else if (strcmp(elementtype, "anchor") == 0) {
      // <anchor> - zero or more times
      ScXMLEltReader * anchorreader = ScXMLAnchorElt::getElementReader();
      ScXMLElt * anchorobj = anchorreader->read(parallel, element, doc, sm);
      if (unlikely(!anchorobj)) {
        delete parallel;
        return NULL;
      }
      assert(anchorobj->isOfType(ScXMLAnchorElt::getClassTypeId()));
      parallel->addAnchor(static_cast<ScXMLAnchorElt *>(anchorobj));
    }

    else if (strcmp(elementtype, "datamodel") == 0) {
      // <datamodel> - zero or one time
      if (unlikely(parallel->getDataModel())) {
        SoDebugError::post("ScXMLParallelEltReader::read",
                           "<parallel> elements can only have one <datamodel> element");
        delete parallel;
        return NULL;
      }
      ScXMLEltReader * datamodelreader = ScXMLDataModelElt::getElementReader();
      ScXMLElt * datamodelobj = datamodelreader->read(parallel, element, doc, sm);
      if (unlikely(!datamodelobj)) {
        delete parallel;
        return NULL;
      }
      assert(datamodelobj->isOfType(ScXMLDataModelElt::getClassTypeId()));
      parallel->setDataModel(static_cast<ScXMLDataModelElt *>(datamodelobj));
    }

#if 0
#if 0
    // <invoke> - one time iff states+parallel == 0
    else if (strcmp(elementtype, "invoke") == 0) {
      ScXMLObject * invokeobj = ScXMLP::readScXMLInvokeElt(state, element, xmlns);
      if (invokeobj) {
        assert(invokeobj->isOfType(ScXMLInvokeElt::getClassTypeId()));
        state->addInvoke(static_cast<ScXMLInvokeElt *>(invokeobj));
      } else {
        SoDebugError::post("ScXML::readFile", "error reading <%s> element", elementtype);
        delete state;
        return NULL;
      }
    }
#endif
#endif
    else {
      SoDebugError::post("ScXML::readFile",
                         "unexpected XML element '<%s>' found in <parallel>",
                         elementtype);
      delete parallel;
      return NULL;
    }
  }

  return parallel;
}

// *************************************************************************

class ScXMLParallelElt::PImpl {
public:
  PImpl(void) { }

  ~PImpl(void)
  {
    SCXML__CLEAR_STD_VECTOR(this->transitionlist, ScXMLTransitionElt *);
    SCXML__CLEAR_STD_VECTOR(this->statelist, ScXMLStateElt *);
    SCXML__CLEAR_STD_VECTOR(this->parallellist, ScXMLParallelElt *);
    SCXML__CLEAR_STD_VECTOR(this->finallist, ScXMLFinalElt *);
    SCXML__CLEAR_STD_VECTOR(this->historylist, ScXMLHistoryElt *);
    SCXML__CLEAR_STD_VECTOR(this->anchorlist, ScXMLAnchorElt *);
  }

  std::unique_ptr<ScXMLOnEntryElt> onentryptr;
  std::unique_ptr<ScXMLOnExitElt> onexitptr;
  std::vector<ScXMLTransitionElt *> transitionlist;
  std::unique_ptr<ScXMLInitialElt> initialptr;
  std::vector<ScXMLStateElt *> statelist;
  std::vector<ScXMLParallelElt *> parallellist;
  std::vector<ScXMLFinalElt *> finallist;
  std::vector<ScXMLHistoryElt *> historylist;
  std::vector<ScXMLAnchorElt *> anchorlist;
  std::unique_ptr<ScXMLDataModelElt> datamodelptr;
  std::unique_ptr<ScXMLInvokeElt> invokeptr;
  // std::unique_ptr<ScXMLDocument> srcref;
};

#define PRIVATE(obj) ((obj)->pimpl)

SCXML_ELEMENT_SOURCE(ScXMLParallelElt);

/*!
*/
void
ScXMLParallelElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLParallelElt, ScXMLAbstractStateElt, "ScXMLAbstractStateElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLParallelElt, "parallel", ScXMLParallelEltReader);
}

/*!
*/
void
ScXMLParallelElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLParallelElt);
  ScXMLParallelElt::classTypeId = SoType::badType();
}

ScXMLParallelElt::ScXMLParallelElt(void)
: src(NULL)
{
}

ScXMLParallelElt::~ScXMLParallelElt(void)
{
  this->setSrcAttribute(NULL);
}

void
ScXMLParallelElt::setSrcAttribute(const char * srcstr)
{
  if (this->src && strcmp(this->src, "") != 0) {
    // FIXME: remove externally sources states?
  }
  SCXML__SET_ATTRIBUTE_VALUE(this->src, "src", srcstr);
  if ((this->src != NULL) && (strcmp(this->src, "") != 0)) {
    // FIXME: scan string for #
    // FIXME: load externally referenced states
  }
}

// const char * ScXMLParallelElt::getSrcAttribute(void) const

SbBool
ScXMLParallelElt::handleXMLAttributes(void)
{
  if (!inherited::handleXMLAttributes()) {
    return FALSE;
  }

  this->setSrcAttribute(this->getXMLAttribute("src"));

  return TRUE;
}

void
ScXMLParallelElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  const ScXMLParallelElt * orig = coin_assert_cast<const ScXMLParallelElt *>(rhs);
  this->setSrcAttribute(orig->getSrcAttribute());

  int c;
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
  for (c = 0; c < orig->getNumTransitions(); ++c) {
    ScXMLTransitionElt * transition =
      coin_assert_cast<ScXMLTransitionElt *>(orig->getTransition(c)->clone());
    this->addTransition(transition);
  }
  if (orig->getInitial()) {
    ScXMLInitialElt * initial =
      coin_assert_cast<ScXMLInitialElt *>(orig->getInitial()->clone());
    this->setInitial(initial);
  }
  for (c = 0; c < orig->getNumStates(); ++c) {
    ScXMLStateElt * state =
      coin_assert_cast<ScXMLStateElt *>(orig->getState(c)->clone());
    this->addState(state);
  }
  for (c = 0; c < orig->getNumParallels(); ++c) {
    ScXMLParallelElt * parallel =
      coin_assert_cast<ScXMLParallelElt *>(orig->getParallel(c)->clone());
    this->addParallel(parallel);
  }
  for (c = 0; c < orig->getNumFinals(); ++c) {
    ScXMLFinalElt * final =
      coin_assert_cast<ScXMLFinalElt *>(orig->getFinal(c)->clone());
    this->addFinal(final);
  }
  for (c = 0; c < orig->getNumHistories(); ++c) {
    ScXMLHistoryElt * history =
      coin_assert_cast<ScXMLHistoryElt *>(orig->getHistory(c)->clone());
    this->addHistory(history);
  }
  for (c = 0; c < orig->getNumAnchors(); ++c) {
    ScXMLAnchorElt * anchor =
      coin_assert_cast<ScXMLAnchorElt *>(orig->getAnchor(c)->clone());
    this->addAnchor(anchor);
  }
  if (orig->getDataModel()) {
    ScXMLDataModelElt * datamodel =
      coin_assert_cast<ScXMLDataModelElt *>(orig->getDataModel()->clone());
    this->setDataModel(datamodel);
  }
}

const ScXMLElt *
ScXMLParallelElt::search(const char * attrname, const char * attrvalue) const
{
  const ScXMLElt * hit = inherited::search(attrname, attrvalue);
  if (hit) {
    return hit;
  }
  if (strcmp(attrname, "src") == 0) {
    if (this->src && strcmp(attrvalue, this->src) == 0) {
      return this;
    }
  }
  if (PRIVATE(this)->onentryptr.get()) {
    hit = PRIVATE(this)->onentryptr->search(attrname, attrvalue);
    if (hit) {
      return hit;
    }
  }
  if (PRIVATE(this)->onexitptr.get()) {
    hit = PRIVATE(this)->onexitptr->search(attrname, attrvalue);
    if (hit) {
      return hit;
    }
  }
  {
    std::vector<ScXMLTransitionElt *>::const_iterator it = PRIVATE(this)->transitionlist.begin();
    while (it != PRIVATE(this)->transitionlist.end()) {
      hit = (*it)->search(attrname, attrvalue);
      if (hit) {
        return hit;
      }
      ++it;
    }
  }
  if (PRIVATE(this)->initialptr.get()) {
    hit = PRIVATE(this)->initialptr->search(attrname, attrvalue);
    if (hit) {
      return hit;
    }
  }
  {
    std::vector<ScXMLStateElt *>::const_iterator it = PRIVATE(this)->statelist.begin();
    while (it != PRIVATE(this)->statelist.end()) {
      hit = (*it)->search(attrname, attrvalue);
      if (hit) {
        return hit;
      }
      ++it;
    }
  }
  {
    std::vector<ScXMLParallelElt *>::const_iterator it = PRIVATE(this)->parallellist.begin();
    while (it != PRIVATE(this)->parallellist.end()) {
      hit = (*it)->search(attrname, attrvalue);
      if (hit) {
        return hit;
      }
      ++it;
    }
  }
  {
    std::vector<ScXMLFinalElt *>::const_iterator it = PRIVATE(this)->finallist.begin();
    while (it != PRIVATE(this)->finallist.end()) {
      hit = (*it)->search(attrname, attrvalue);
      if (hit) {
        return hit;
      }
      ++it;
    }
  }
  {
    std::vector<ScXMLHistoryElt *>::const_iterator it = PRIVATE(this)->historylist.begin();
    while (it != PRIVATE(this)->historylist.end()) {
      hit = (*it)->search(attrname, attrvalue);
      if (hit) {
        return hit;
      }
      ++it;
    }
  }
  {
    std::vector<ScXMLAnchorElt *>::const_iterator it = PRIVATE(this)->anchorlist.begin();
    while (it != PRIVATE(this)->anchorlist.end()) {
      hit = (*it)->search(attrname, attrvalue);
      if (hit) {
        return hit;
      }
      ++it;
    }
  }
  if (PRIVATE(this)->datamodelptr.get()) {
    hit = PRIVATE(this)->datamodelptr->search(attrname, attrvalue);
    if (hit) {
      return hit;
    }
  }
  return NULL;
}

SCXML_SINGLE_OBJECT_API_IMPL(ScXMLParallelElt, ScXMLOnEntryElt, PRIVATE(this)->onentryptr, OnEntry);

SCXML_SINGLE_OBJECT_API_IMPL(ScXMLParallelElt, ScXMLOnExitElt, PRIVATE(this)->onexitptr, OnExit);

SCXML_LIST_OBJECT_API_IMPL(ScXMLParallelElt, ScXMLTransitionElt, PRIVATE(this)->transitionlist, Transition, Transitions);

SCXML_SINGLE_OBJECT_API_IMPL(ScXMLParallelElt, ScXMLInitialElt, PRIVATE(this)->initialptr, Initial);

SCXML_LIST_OBJECT_API_IMPL(ScXMLParallelElt, ScXMLStateElt, PRIVATE(this)->statelist, State, States);

SCXML_LIST_OBJECT_API_IMPL(ScXMLParallelElt, ScXMLParallelElt, PRIVATE(this)->parallellist, Parallel, Parallels);

SCXML_LIST_OBJECT_API_IMPL(ScXMLParallelElt, ScXMLFinalElt, PRIVATE(this)->finallist, Final, Finals);

SCXML_LIST_OBJECT_API_IMPL(ScXMLParallelElt, ScXMLHistoryElt, PRIVATE(this)->historylist, History, Histories);

SCXML_LIST_OBJECT_API_IMPL(ScXMLParallelElt, ScXMLAnchorElt, PRIVATE(this)->anchorlist, Anchor, Anchors);

SCXML_SINGLE_OBJECT_API_IMPL(ScXMLParallelElt, ScXMLDataModelElt, PRIVATE(this)->datamodelptr, DataModel);

// SCXML_SINGLE_OBJECT_API_IMPL(ScXMLParallelElt, ScXMLInvokeElt, PRIVATE(this)->invokeptr, Invoke);

/*!
  Returns TRUE if this is an "atomic state", which means that it has no
  sub-states but contains executable content.
*/
SbBool
ScXMLParallelElt::isAtomicState(void) const
{
  return ((PRIVATE(this)->statelist.size() == 0) &&
          (PRIVATE(this)->parallellist.size() == 0) &&
          (PRIVATE(this)->invokeptr.get() != NULL));
}

#undef PRIVATE
