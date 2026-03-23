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

#include <Inventor/scxml/ScXMLStateElt.h>

/*!
  \class ScXMLStateElt ScXMLStateElt.h Inventor/scxml/ScXMLStateElt.h
  \brief implements the &lt;state&gt; SCXML element.

  \since Coin 3.0
  \ingroup coin_scxml
*/

#include <cassert>
#include <cstring>
#include <algorithm>
#include <vector>

#include <memory>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/C/XML/element.h>
#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/ScXMLDocument.h>
#include <Inventor/scxml/ScXMLInvokeElt.h>
#include <Inventor/scxml/ScXMLParallelElt.h>
#include <Inventor/scxml/ScXMLOnExitElt.h>
#include <Inventor/scxml/ScXMLOnEntryElt.h>
#include <Inventor/scxml/ScXMLTransitionElt.h>
#include <Inventor/scxml/ScXMLInitialElt.h>
#include <Inventor/scxml/ScXMLFinalElt.h>
#include <Inventor/scxml/ScXMLHistoryElt.h>
#include <Inventor/scxml/ScXMLAnchorElt.h>
#include <Inventor/scxml/ScXMLDataModelElt.h>
#include <Inventor/scxml/ScXMLScxmlElt.h>

#include "scxml/ScXMLCommonP.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::strcmp;
using std::strcpy;
using std::strlen;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

// *************************************************************************

class ScXMLStateEltReader : public ScXMLEltReader {
public:
  ScXMLStateEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);
};

ScXMLStateEltReader::ScXMLStateEltReader(void)
: ScXMLEltReader("state")
{
}

namespace { namespace ScXMLStateNS {

template <class Type>
Type *
clone(Type * objptr)
{
  return coin_assert_cast<Type *>(objptr->clone());
}

} }

ScXMLElt *
ScXMLStateEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm)
{
  assert(container && xmlelt);

  ScXMLStateElt * state = new ScXMLStateElt;
  state->setContainer(container);
  this->setXMLAttributes(state, xmlelt);

  // handle XML attributes
  if (unlikely(!state->handleXMLAttributes())) {
    SoDebugError::post("ScXMLStateEltReader::read",
                       "invalid XML attributes");
    delete state;
    return NULL;
  }

  // handle external reference
  const char * extref = state->getSrcAttribute();
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
            state->setInitial(initialelt);
          }
#endif
          for (c = 0; c < parent->getNumStates(); ++c) {
            ScXMLStateElt * stateelt = ::ScXMLStateNS::clone(parent->getState(c));
//            coin_assert_cast<ScXMLStateElt *>(parent->getState(c)->clone());
            state->addState(stateelt);
          }
          for (c = 0; c < parent->getNumParallels(); ++c) {
            ScXMLParallelElt * parallelelt =
              coin_assert_cast<ScXMLParallelElt *>(parent->getParallel(c)->clone());
            state->addParallel(parallelelt);
          }
          for (c = 0; c < parent->getNumFinals(); ++c) {
            ScXMLFinalElt * finalelt =
              coin_assert_cast<ScXMLFinalElt *>(parent->getFinal(c)->clone());
            state->addFinal(finalelt);
          }
          if (parent->getDataModel()) {
            ScXMLDataModelElt * datamodelelt =
              coin_assert_cast<ScXMLDataModelElt *>(parent->getDataModel()->clone());
            state->setDataModel(datamodelelt);
          }
#if 0
          for (c = 0; c < parent->getNumScripts(); ++c) {
            ScXMLScriptElt * scriptelt =
              coin_assert_cast<ScXMLScriptElt *>(parent->getScript(c)->clone());
            state->addScript(scriptelt);
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
            state->setOnEntry(onentryelt);
          }
          if (parent->getOnExit()) {
            ScXMLOnExitElt * onexitelt =
              coin_assert_cast<ScXMLOnExitElt *>(parent->getOnExit()->clone());
            state->setOnExit(onexitelt);
          }
          for (c = 0; c < parent->getNumTransitions(); ++c) {
            ScXMLTransitionElt * transitionelt =
              coin_assert_cast<ScXMLTransitionElt *>(parent->getTransition(c)->clone());
            state->addTransition(transitionelt);
          }
#if 0
          if (parent->getInitial()) {
            ScXMLInitialElt * initialelt =
              coin_assert_cast<ScXMLInitialElt *>(parent->getInitial()->clone());
            state->setInitial(initialelt);
          }
#endif
          for (c = 0; c < parent->getNumStates(); ++c) {
            ScXMLStateElt * stateelt =
              coin_assert_cast<ScXMLStateElt *>(parent->getState(c)->clone());
            state->addState(stateelt);
          }
          for (c = 0; c < parent->getNumParallels(); ++c) {
            ScXMLParallelElt * parallelelt =
              coin_assert_cast<ScXMLParallelElt *>(parent->getParallel(c)->clone());
            state->addParallel(parallelelt);
          }
          for (c = 0; c < parent->getNumFinals(); ++c) {
            ScXMLFinalElt * finalelt =
              coin_assert_cast<ScXMLFinalElt *>(parent->getFinal(c)->clone());
            state->addFinal(finalelt);
          }
          for (c = 0; c < parent->getNumHistories(); ++c) {
            ScXMLHistoryElt * historyelt =
              coin_assert_cast<ScXMLHistoryElt *>(parent->getHistory(c)->clone());
            state->addHistory(historyelt);
          }
          for (c = 0; c < parent->getNumAnchors(); ++c) {
            ScXMLAnchorElt * anchorelt =
              coin_assert_cast<ScXMLAnchorElt *>(parent->getAnchor(c)->clone());
            state->addAnchor(anchorelt);
          }
          if (parent->getDataModel()) {
            ScXMLDataModelElt * datamodelelt =
              coin_assert_cast<ScXMLDataModelElt *>(parent->getDataModel()->clone());
            state->setDataModel(datamodelelt);
          }
        }
        else if (parentelt->isOfType(ScXMLParallelElt::getClassTypeId())) {
          ScXMLParallelElt * parent =
            coin_assert_cast<ScXMLParallelElt *>(parentelt);
          int c = 0;
          if (parent->getOnEntry()) {
            ScXMLOnEntryElt * onentryelt =
              coin_assert_cast<ScXMLOnEntryElt *>(parent->getOnEntry()->clone());
            state->setOnEntry(onentryelt);
          }
          if (parent->getOnExit()) {
            ScXMLOnExitElt * onexitelt =
              coin_assert_cast<ScXMLOnExitElt *>(parent->getOnExit()->clone());
            state->setOnExit(onexitelt);
          }
          for (c = 0; c < parent->getNumTransitions(); ++c) {
            ScXMLTransitionElt * transitionelt =
              coin_assert_cast<ScXMLTransitionElt *>(parent->getTransition(c)->clone());
            state->addTransition(transitionelt);
          }
#if 0
          if (parent->getInitial()) {
            ScXMLInitialElt * initialelt =
              coin_assert_cast<ScXMLInitialElt *>(parent->getInitial()->clone());
            state->setInitial(initialelt);
          }
#endif
          for (c = 0; c < parent->getNumStates(); ++c) {
            ScXMLStateElt * stateelt =
              coin_assert_cast<ScXMLStateElt *>(parent->getState(c)->clone());
            state->addState(stateelt);
          }
          for (c = 0; c < parent->getNumParallels(); ++c) {
            ScXMLParallelElt * parallelelt =
              coin_assert_cast<ScXMLParallelElt *>(parent->getParallel(c)->clone());
            state->addParallel(parallelelt);
          }
          for (c = 0; c < parent->getNumFinals(); ++c) {
            ScXMLFinalElt * finalelt =
              coin_assert_cast<ScXMLFinalElt *>(parent->getFinal(c)->clone());
            state->addFinal(finalelt);
          }
          for (c = 0; c < parent->getNumHistories(); ++c) {
            ScXMLHistoryElt * historyelt =
              coin_assert_cast<ScXMLHistoryElt *>(parent->getHistory(c)->clone());
            state->addHistory(historyelt);
          }
          for (c = 0; c < parent->getNumAnchors(); ++c) {
            ScXMLAnchorElt * anchorelt =
              coin_assert_cast<ScXMLAnchorElt *>(parent->getAnchor(c)->clone());
            state->addAnchor(anchorelt);
          }
          if (parent->getDataModel()) {
            ScXMLDataModelElt * datamodelelt =
              coin_assert_cast<ScXMLDataModelElt *>(parent->getDataModel()->clone());
            state->setDataModel(datamodelelt);
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

    if (strcmp(elementtype, "state") == 0) {
      // <state> - zero or more times
      ScXMLEltReader * statereader = ScXMLStateElt::getElementReader();
      assert(statereader);
      ScXMLElt * stateobj = statereader->read(state, element, doc, sm);
      if (unlikely(!stateobj)) {
        delete state;
        return NULL;
      }

      assert(stateobj->isOfType(ScXMLStateElt::getClassTypeId()));
      state->addState(static_cast<ScXMLStateElt *>(stateobj));
    }

    else if (strcmp(elementtype, "onentry") == 0) {
      // <onentry> - zero or one time
      if (unlikely(state->getOnEntry())) {
        SoDebugError::post("ScXMLStateEltReader::read",
                           "<state> elements can only contain one <onentry> element");
        delete state;
        return NULL;
      }

      ScXMLEltReader * onentryreader = ScXMLOnEntryElt::getElementReader();
      assert(onentryreader);
      ScXMLElt * onentryobj = onentryreader->read(state, element, doc, sm);
      if (unlikely(!onentryobj)) {
        delete state;
        return NULL;
      }

      assert(onentryobj->isOfType(ScXMLOnEntryElt::getClassTypeId()));
      state->setOnEntry(static_cast<ScXMLOnEntryElt *>(onentryobj));
    }

    else if (strcmp(elementtype, "onexit") == 0) {
      // <onexit> - zero or one time
      if (unlikely(state->getOnExit())) {
        SoDebugError::post("ScXMLStateEltReader::read",
                           "<state> elements can only contain one <onexit> element");
        delete state;
        return NULL;
      }

      ScXMLEltReader * onexitreader = ScXMLOnExitElt::getElementReader();
      assert(onexitreader);
      ScXMLElt * onexitobj = onexitreader->read(state, element, doc, sm);
      if (unlikely(!onexitobj)) {
        delete state;
        return NULL;
      }

      assert(onexitobj->isOfType(ScXMLOnExitElt::getClassTypeId()));
      state->setOnExit(static_cast<ScXMLOnExitElt *>(onexitobj));
    }

    else if (strcmp(elementtype, "transition") == 0) {
      // <transition> - zero or more times
      ScXMLEltReader * transitionreader = ScXMLTransitionElt::getElementReader();
      assert(transitionreader);
      ScXMLElt * transitionobj = transitionreader->read(state, element, doc, sm);
      if (unlikely(!transitionobj)) {
        delete state;
        return NULL;
      }

      assert(transitionobj->isOfType(ScXMLTransitionElt::getClassTypeId()));
      state->addTransition(static_cast<ScXMLTransitionElt *>(transitionobj));
    }

    else if (strcmp(elementtype, "initial") == 0) {
      // <initial> - must occur iff states+parallels >= 1
      // or if initial attribute is not set
      ScXMLEltReader * initialreader = ScXMLInitialElt::getElementReader();
      assert(initialreader);
      ScXMLElt * initialobj = initialreader->read(state, element, doc, sm);
      if (unlikely(!initialobj)) {
        delete state;
        return NULL;
      }

      assert(initialobj->isOfType(ScXMLInitialElt::getClassTypeId()));
      state->setInitial(static_cast<ScXMLInitialElt *>(initialobj));
    }

    else if (strcmp(elementtype, "parallel") == 0) {
      // <parallel> - zero or more times
      ScXMLEltReader * parallelreader = ScXMLParallelElt::getElementReader();
      assert(parallelreader);
      ScXMLElt * parallelobj = parallelreader->read(state, element, doc, sm);
      if (unlikely(!parallelobj)) {
        delete state;
        return NULL;
      }

      assert(parallelobj->isOfType(ScXMLParallelElt::getClassTypeId()));
      state->addParallel(static_cast<ScXMLParallelElt *>(parallelobj));
    }

    else if (strcmp(elementtype, "final") == 0) {
      // <final> - zero or more times
      ScXMLEltReader * finalreader = ScXMLFinalElt::getElementReader();
      assert(finalreader);
      ScXMLElt * finalobj = finalreader->read(state, element, doc, sm);
      if (unlikely(!finalobj)) {
        delete state;
        return NULL;
      }

      assert(finalobj->isOfType(ScXMLFinalElt::getClassTypeId()));
      state->addFinal(static_cast<ScXMLFinalElt *>(finalobj));
    }

    else if (strcmp(elementtype, "history") == 0) {
      // <history> - zero or more times
      ScXMLEltReader * historyreader = ScXMLHistoryElt::getElementReader();
      assert(historyreader);
      ScXMLElt * historyobj = historyreader->read(state, element, doc, sm);
      if (unlikely(!historyobj)) {
        delete state;
        return NULL;
      }

      assert(historyobj->isOfType(ScXMLHistoryElt::getClassTypeId()));
      state->addHistory(static_cast<ScXMLHistoryElt *>(historyobj));
    }

    else if (strcmp(elementtype, "anchor") == 0) {
      // <anchor> - zero or more times
      ScXMLEltReader * anchorreader = ScXMLAnchorElt::getElementReader();
      assert(anchorreader);
      ScXMLElt * anchorobj = anchorreader->read(state, element, doc, sm);
      if (unlikely(!anchorobj)) {
        delete state;
        return NULL;
      }

      assert(anchorobj->isOfType(ScXMLAnchorElt::getClassTypeId()));
      state->addAnchor(static_cast<ScXMLAnchorElt *>(anchorobj));
    }

    else if (strcmp(elementtype, "datamodel") == 0) {
      // <datamodel> - zero or one time
      if (unlikely(state->getDataModel())) {
        SoDebugError::post("ScXMLStateEltReader::read",
                           "<state> elements can only contain one <datamodel> element");
        delete state;
        return NULL;
      }

      ScXMLEltReader * datamodelreader = ScXMLDataModelElt::getElementReader();
      assert(datamodelreader);
      ScXMLElt * datamodelobj = datamodelreader->read(state, element, doc, sm);
      if (unlikely(!datamodelobj)) {
        delete state;
        return NULL;
      }

      assert(datamodelobj->isOfType(ScXMLDataModelElt::getClassTypeId()));
      state->setDataModel(static_cast<ScXMLDataModelElt *>(datamodelobj));
    }

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
    else {
      SoDebugError::post("ScXMLStateEltReader::read",
                         "unexpected XML element '<%s>' found in <state>",
                         elementtype);
      delete state;
      return NULL;
    }
  }
  return state;
}

// *************************************************************************

class ScXMLStateElt::PImpl {
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

SCXML_ELEMENT_SOURCE(ScXMLStateElt);

/*!
*/
void
ScXMLStateElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLStateElt, ScXMLAbstractStateElt, "ScXMLAbstractStateElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLStateElt, "state", ScXMLStateEltReader);
}

/*!
*/
void
ScXMLStateElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLStateElt);
  ScXMLStateElt::classTypeId = SoType::badType();
}

/*!
  Constructor.
*/
ScXMLStateElt::ScXMLStateElt(void)
: src(NULL),
  initial(NULL)
{
}

/*!
  Destructor.
*/
ScXMLStateElt::~ScXMLStateElt(void)
{
  this->setSrcAttribute(NULL);
  this->setInitialAttribute(NULL);
}

/*!
  Sets the XML 'src' attribute for the SCXML &lt;state&gt; element.
  To get the attribute value interpreted and taken action upon, you will
  also need to [FIXME]
*/
void
ScXMLStateElt::setSrcAttribute(const char * srcstr)
{
  if (this->src && strcmp(this->src, "") != 0) {
    // FIXME: remove externally sources states?
  }

  SCXML__SET_ATTRIBUTE_VALUE(this->src, "src", srcstr);

  if ((this->src != NULL) && (strcmp(this->src, "") != 0)) {
    // FIXME: scan string for #
    // FIXME: load/import [externally] referenced states
  }
}

/*!
  \fn const char * ScXMLStateElt::getSrcAttribute(void) const

  Returns the XML 'src' attribute for this SCXML &lt;state&gt; element.
*/

/*!
  Sets the XML 'initial' attribute for the SCXML &lt;state&gt; element.
  If this value is set, the state should not contain an &lt;initial&gt;
  element, and vice versa.

  When set, this value must be the value of a descendant an identifier for
  a child state.

  \sa ScXMLInitialElt
*/
void
ScXMLStateElt::setInitialAttribute(const char * initialstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->initial, "initial", initialstr);
}

/*!
  \fn const char * ScXMLStateElt::getInitialAttribute(void) const

  Returns the XML 'initial' attribute for the SCXML &lt;state&gt; element.
*/

// Doc in parent
SbBool
ScXMLStateElt::handleXMLAttributes(void)
{
  if (!inherited::handleXMLAttributes()) return FALSE;

  this->setInitialAttribute(this->getXMLAttribute("initial"));
  this->setSrcAttribute(this->getXMLAttribute("src"));

  return TRUE;
}

void
ScXMLStateElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  const ScXMLStateElt * orig = coin_assert_cast<const ScXMLStateElt *>(rhs);
  this->setInitialAttribute(orig->getInitialAttribute());
  this->setSrcAttribute(orig->getSrcAttribute());
  int c = 0;
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
ScXMLStateElt::search(const char * attrname, const char * attrvalue) const
{
  const ScXMLElt * hit = inherited::search(attrname, attrvalue);
  if (hit) {
    return hit;
  }
  if (strcmp(attrname, "initial") == 0) { // 'initialstate' is dropped
    if (this->initial && strcmp(attrvalue, this->initial) == 0) {
      return this;
    }
  }
  else if (strcmp(attrname, "src") == 0) {
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

SCXML_SINGLE_OBJECT_API_IMPL(ScXMLStateElt, ScXMLOnEntryElt,
                             PRIVATE(this)->onentryptr, OnEntry);

SCXML_SINGLE_OBJECT_API_IMPL(ScXMLStateElt, ScXMLOnExitElt,
                             PRIVATE(this)->onexitptr, OnExit);

SCXML_LIST_OBJECT_API_IMPL(ScXMLStateElt, ScXMLTransitionElt,
                           PRIVATE(this)->transitionlist,
                           Transition, Transitions);

SCXML_SINGLE_OBJECT_API_IMPL(ScXMLStateElt, ScXMLInitialElt,
                             PRIVATE(this)->initialptr, Initial);

SCXML_LIST_OBJECT_API_IMPL(ScXMLStateElt, ScXMLStateElt,
                           PRIVATE(this)->statelist, State, States);

SCXML_LIST_OBJECT_API_IMPL(ScXMLStateElt, ScXMLParallelElt,
                           PRIVATE(this)->parallellist, Parallel, Parallels);

SCXML_LIST_OBJECT_API_IMPL(ScXMLStateElt, ScXMLFinalElt,
                           PRIVATE(this)->finallist, Final, Finals);

SCXML_LIST_OBJECT_API_IMPL(ScXMLStateElt, ScXMLHistoryElt,
                           PRIVATE(this)->historylist, History, Histories);

SCXML_LIST_OBJECT_API_IMPL(ScXMLStateElt, ScXMLAnchorElt,
                           PRIVATE(this)->anchorlist, Anchor, Anchors);

SCXML_SINGLE_OBJECT_API_IMPL(ScXMLStateElt, ScXMLDataModelElt,
                             PRIVATE(this)->datamodelptr, DataModel);

// SCXML_SINGLE_OBJECT_API_IMPL(ScXMLStateElt, ScXMLInvokeElt, PRIVATE(this)->invokeptr, Invoke);

/*!
  Returns TRUE if this is an "atomic state", which means that it has no
  sub-states but contains executable content.
*/
SbBool
ScXMLStateElt::isAtomicState(void) const
{
  return ((PRIVATE(this)->statelist.size() == 0) &&
          (PRIVATE(this)->parallellist.size() == 0) &&
          (PRIVATE(this)->invokeptr.get() != NULL));
}

#undef PRIVATE
