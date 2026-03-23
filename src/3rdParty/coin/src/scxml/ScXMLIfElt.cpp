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

#include <Inventor/scxml/ScXMLIfElt.h>

/*!
  \class ScXMLIfElt ScXMLIfElt.h Inventor/scxml/ScXMLIfElt.h
  \brief implements the &lt;if&gt; SCXML element.

  \ingroup coin_scxml
*/

#include <cassert>
#include <vector>
#include <algorithm>

#include <memory>

#include <Inventor/C/XML/element.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/scxml/ScXMLAssignElt.h>
#include <Inventor/scxml/ScXMLElseElt.h>
#include <Inventor/scxml/ScXMLElseIfElt.h>
#include <Inventor/scxml/ScXMLEvaluator.h>
#include <Inventor/scxml/ScXMLEventElt.h>
#include <Inventor/scxml/ScXMLInvokeElt.h>
#include <Inventor/scxml/ScXMLLogElt.h>
#include <Inventor/scxml/ScXMLScriptElt.h>
#include <Inventor/scxml/ScXMLSendElt.h>
#include <Inventor/scxml/ScXMLStateMachine.h>
#include <Inventor/scxml/ScXMLValidateElt.h>
#include <Inventor/scxml/ScXMLEvaluator.h>
#include <Inventor/scxml/ScXMLStateMachine.h>

#include "scxml/ScXMLCommonP.h"
#include "SbBasicP.h"

// *************************************************************************

class ScXMLIfEltReader : public ScXMLEltReader {
public:
  ScXMLIfEltReader(void);
  virtual ScXMLElt * read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm);
};

ScXMLIfEltReader::ScXMLIfEltReader(void)
: ScXMLEltReader("if")
{
}

ScXMLElt *
ScXMLIfEltReader::read(ScXMLElt * container, cc_xml_elt * xmlelt, ScXMLDocument * doc, ScXMLStateMachine * sm)
{
  assert(container && xmlelt);
  ScXMLIfElt * ifelt = new ScXMLIfElt;
  ifelt->setContainer(container);
  this->setXMLAttributes(ifelt, xmlelt);

  // handle XML attributes
  if (unlikely(!ifelt->handleXMLAttributes())) {
    delete ifelt;
    return NULL;
  }

  ScXMLExecutableElt * conditional = ifelt;

  const int numchildren = cc_xml_elt_get_num_children(xmlelt);
  for (int c = 0; c < numchildren; ++c) {
    cc_xml_elt * element = cc_xml_elt_get_child(xmlelt, c);
    const char * elementtype = cc_xml_elt_get_type(element);

    if (strcmp(elementtype, COIN_XML_CDATA_TYPE) == 0) {
      // ignore CDATA
      continue;
    }

    if (strcmp(elementtype, "elseif") == 0) {
      if (unlikely(ifelt->getElse())) {
        SoDebugError::post("ScXMLIfEltReader::read",
                           "<if> cannot contain <elseif> after an <else> element");
        delete ifelt;
        return NULL;
      }
      ScXMLEltReader * elseifreader = ScXMLElseIfElt::getElementReader();
      ScXMLElt * elseifelt = elseifreader->read(ifelt, element, doc, sm);
      if (unlikely(!elseifelt)) {
        delete ifelt;
        return NULL;
      }
      assert(elseifelt->isOfType(ScXMLElseIfElt::getClassTypeId()));
      ifelt->addElseIf(static_cast<ScXMLElseIfElt *>(elseifelt));
      conditional = static_cast<ScXMLElseIfElt *>(elseifelt);
    }

    else if (strcmp(elementtype, "else") == 0) {
      if (unlikely(ifelt->getElse())) {
        SoDebugError::post("ScXMLIfEltReader::read",
                           "<if> cannot contain multiple <else> elements");
        delete ifelt;
        return NULL;
      }
      ScXMLEltReader * elsereader = ScXMLElseElt::getElementReader();
      ScXMLElt * elseelt = elsereader->read(ifelt, element, doc, sm);
      if (unlikely(!elseelt)) {
        delete ifelt;
        return NULL;
      }
      assert(elseelt->isOfType(ScXMLElseElt::getClassTypeId()));
      ifelt->setElse(static_cast<ScXMLElseElt *>(elseelt));
      conditional = static_cast<ScXMLElseElt *>(elseelt);
    }

    else if (strcmp(elementtype, "if") == 0) {
      ScXMLEltReader * executablereader = ScXMLIfElt::getElementReader();
      ScXMLElt * executableelt = executablereader->read(ifelt, element, doc, sm);
      if (unlikely(!executableelt)) {
        delete ifelt;
        return NULL;
      }
      assert(executableelt->isOfType(ScXMLIfElt::getClassTypeId()));
      ifelt->addExecutable(conditional, static_cast<ScXMLExecutableElt *>(executableelt));
    }
    else if (strcmp(elementtype, "script") == 0) {
      ScXMLEltReader * scriptreader = ScXMLScriptElt::getElementReader();
      ScXMLElt * executableelt = scriptreader->read(ifelt, element, doc, sm);
      if (unlikely(!executableelt)) {
        delete ifelt;
        return NULL;
      }
      assert(executableelt->isOfType(ScXMLScriptElt::getClassTypeId()));
      ifelt->addExecutable(conditional, static_cast<ScXMLExecutableElt *>(executableelt));
    }
    else if (strcmp(elementtype, "log") == 0) {
      ScXMLEltReader * logreader = ScXMLLogElt::getElementReader();
      ScXMLElt * executableelt = logreader->read(ifelt, element, doc, sm);
      if (unlikely(!executableelt)) {
        delete ifelt;
        return NULL;
      }
      assert(executableelt->isOfType(ScXMLLogElt::getClassTypeId()));
      ifelt->addExecutable(conditional, static_cast<ScXMLExecutableElt *>(executableelt));
    }
    else if (strcmp(elementtype, "assign") == 0) {
      ScXMLEltReader * assignreader = ScXMLAssignElt::getElementReader();
      ScXMLElt * executableelt = assignreader->read(ifelt, element, doc, sm);
      if (unlikely(!executableelt)) {
        delete ifelt;
        return NULL;
      }
      assert(executableelt->isOfType(ScXMLAssignElt::getClassTypeId()));
      ifelt->addExecutable(conditional, static_cast<ScXMLExecutableElt *>(executableelt));
    }
    else if (strcmp(elementtype, "event") == 0) {
      ScXMLEltReader * eventreader = ScXMLEventElt::getElementReader();
      ScXMLElt * executableelt = eventreader->read(ifelt, element, doc, sm);
      if (unlikely(!executableelt)) {
        delete ifelt;
        return NULL;
      }
      assert(executableelt->isOfType(ScXMLEventElt::getClassTypeId()));
      ifelt->addExecutable(conditional, static_cast<ScXMLExecutableElt *>(executableelt));
    }
    else if (strcmp(elementtype, "invoke") == 0) {
      ScXMLEltReader * invokereader = ScXMLInvokeElt::getElementReader();
      ScXMLElt * executableelt = invokereader->read(ifelt, element, doc, sm);
      if (unlikely(!executableelt)) {
        delete ifelt;
        return NULL;
      }
      assert(executableelt->isOfType(ScXMLInvokeElt::getClassTypeId()));
      ifelt->addExecutable(conditional, static_cast<ScXMLExecutableElt *>(executableelt));
    }
    else if (strcmp(elementtype, "send") == 0) {
      ScXMLEltReader * sendreader = ScXMLSendElt::getElementReader();
      ScXMLElt * executableelt = sendreader->read(ifelt, element, doc, sm);
      if (unlikely(!executableelt)) {
        delete ifelt;
        return NULL;
      }
      assert(executableelt->isOfType(ScXMLSendElt::getClassTypeId()));
      ifelt->addExecutable(conditional, static_cast<ScXMLExecutableElt *>(executableelt));
    }
    else if (strcmp(elementtype, "validate") == 0) {
      ScXMLEltReader * validatereader = ScXMLValidateElt::getElementReader();
      ScXMLElt * executableelt = validatereader->read(ifelt, element, doc, sm);
      if (unlikely(!executableelt)) {
        delete ifelt;
        return NULL;
      }
      assert(executableelt->isOfType(ScXMLValidateElt::getClassTypeId()));
      ifelt->addExecutable(conditional, static_cast<ScXMLExecutableElt *>(executableelt));
    }

    else {
      SoDebugError::post("ScXMLIfEltReader::read",
                         "<if> contains unexpected <%s> element", elementtype);
      delete ifelt;
      return NULL;
    }
  }

  return ifelt;
}

// *************************************************************************

class ScXMLIfElt::PImpl {
public:
  PImpl(void) : pub(NULL)
  {
  }

  ~PImpl(void)
  {
    SCXML__CLEAR_STD_VECTOR(this->elseiflist, ScXMLElseIfElt *);
  }

  int findIdx(const ScXMLExecutableElt * conditional) const;
  void assureArrayForIdx(int idx);
  std::vector<ScXMLExecutableElt *> * getArrayForIdx(int idx) const;

  ScXMLIfElt * pub;

  std::vector<ScXMLElseIfElt *> elseiflist;
  std::unique_ptr<ScXMLElseElt> elseelt;
  std::vector< std::vector<ScXMLExecutableElt *> * > executables;
};

#define PUBLIC(obj) ((obj)->pub)

int
ScXMLIfElt::PImpl::findIdx(const ScXMLExecutableElt * conditional) const
{
  assert(conditional);

  if (conditional == this->pub) { return 0; }

  for (int idx = 0; idx < static_cast<int>(this->elseiflist.size()); ++idx) {
    if (conditional == this->elseiflist.at(idx-1)) {
      return (idx + 1);
    }
  }

  if (conditional == PUBLIC(this)->getElse()) {
    return int(this->elseiflist.size() + 1);
  }

  return -1;
}

void
ScXMLIfElt::PImpl::assureArrayForIdx(int idx)
{
  while (int(this->executables.size()) < idx) {
    this->executables.push_back(NULL);
  }
  if (this->executables.at(idx) == NULL) {
    this->executables.at(idx) = new std::vector<ScXMLExecutableElt *>;
  }
}

std::vector<ScXMLExecutableElt *> *
ScXMLIfElt::PImpl::getArrayForIdx(int idx) const
{
  if (idx < int(this->executables.size())) {
    return this->executables.at(idx);
  }
  return NULL;
}

#undef PUBLIC
#define PRIVATE(obj) ((obj)->pimpl)

SCXML_ELEMENT_SOURCE(ScXMLIfElt);

void
ScXMLIfElt::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLIfElt, ScXMLExecutableElt, "ScXMLExecutableElt");
  SCXML_ELEMENT_REGISTER_READER(ScXMLIfElt, "if", ScXMLIfEltReader);
}

void
ScXMLIfElt::cleanClass(void)
{
  SCXML_ELEMENT_UNREGISTER_READER(ScXMLIfElt);
  ScXMLIfElt::classTypeId = SoType::badType();
}

ScXMLIfElt::ScXMLIfElt(void)
: cond(NULL)
{
  pimpl->pub = this;
}

ScXMLIfElt::~ScXMLIfElt(void)
{
  this->setCondAttribute(NULL);
}

void
ScXMLIfElt::setCondAttribute(const char * condstr)
{
  SCXML__SET_ATTRIBUTE_VALUE(this->cond, "cond", condstr);
}

// const char * getCondAttribute(void) const { return this->cond; }

SbBool
ScXMLIfElt::handleXMLAttributes(void)
{
  if (!inherited::handleXMLAttributes()) {
    return FALSE;
  }

  this->setCondAttribute(this->getXMLAttribute("cond"));

  return TRUE;
}

void
ScXMLIfElt::copyContents(const ScXMLElt * rhs)
{
  inherited::copyContents(rhs);
  const ScXMLIfElt * orig = coin_assert_cast<const ScXMLIfElt *>(rhs);
  this->setCondAttribute(orig->getCondAttribute());

  int c = 0, i = 0;
  for (i = 0; i < orig->getNumExecutables(orig); ++i) {
    ScXMLExecutableElt * executable =
      coin_assert_cast<ScXMLExecutableElt *>(orig->getExecutable(orig, i)->clone());
    this->addExecutable(this, executable);
  }

  for (c = 0; c < orig->getNumElseIfs(); ++c) {
    ScXMLElseIfElt * origelseif = orig->getElseIf(c);
    ScXMLElseIfElt * elseif =
      coin_assert_cast<ScXMLElseIfElt *>(origelseif->clone());
    this->addElseIf(elseif);
    for (i = 0; i < orig->getNumExecutables(origelseif); ++i) {
      ScXMLExecutableElt * executable =
        coin_assert_cast<ScXMLExecutableElt *>(orig->getExecutable(origelseif, i)->clone());
      this->addExecutable(elseif, executable);
    }
  }

  if (orig->getElse()) {
    ScXMLElseElt * origelse = orig->getElse();
    ScXMLElseElt * elseelt =
      coin_assert_cast<ScXMLElseElt *>(origelse->clone());
    this->setElse(elseelt);
    for (i = 0; i < orig->getNumExecutables(origelse); ++i) {
      ScXMLExecutableElt * executable =
        coin_assert_cast<ScXMLExecutableElt *>(orig->getExecutable(origelse, i)->clone());
      this->addExecutable(elseelt, executable);
    }
  }
}

const ScXMLElt *
ScXMLIfElt::search(const char * attrname, const char * attrvalue) const
{
  const ScXMLElt * hit = inherited::search(attrname, attrvalue);
  if (hit) {
    return hit;
  }
  if (strcmp(attrname, "cond") == 0) {
    if (this->cond && strcmp(attrvalue, this->cond) == 0) {
      return this;
    }
  }
  std::vector<ScXMLElseIfElt *>::const_iterator it = PRIVATE(this)->elseiflist.begin();
  while (it != PRIVATE(this)->elseiflist.end()) {
    hit = (*it)->search(attrname, attrvalue);
    if (hit) {
      return hit;
    }
    ++it;
  }
  if (PRIVATE(this)->elseelt.get()) {
    hit = PRIVATE(this)->elseelt->search(attrname, attrvalue);
    if (hit) {
      return hit;
    }
  }
  return NULL;
}

void
ScXMLIfElt::execute(ScXMLStateMachine * statemachine) const
{
  ScXMLEvaluator * evaluator = statemachine->getEvaluator();
  assert(evaluator);

  ScXMLDataObj * res = NULL;
  ScXMLBoolDataObj * boolres = NULL;
  res = evaluator->evaluate(this->getCondAttribute());
  if (res) {
    if (res->isOfType(ScXMLBoolDataObj::getClassTypeId())) {
      boolres = static_cast<ScXMLBoolDataObj *>(res);
      if (boolres->getBool()) {
        for (int i = 0; i < this->getNumExecutables(this); ++i) {
          ScXMLExecutableElt * executable = this->getExecutable(this, i);
          executable->execute(statemachine);
        }
        return;
      }
    }
    else {
      statemachine->queueInternalEvent("error.eval.minimum.IfCond.NOT_A_TRUTH_EXPRESSION");
      return;
    }
  }
  else {
    statemachine->queueInternalEvent("error.eval.minimum.IfCond.NO_VALID_EXPRESSION");
    return;
  }

  for (int i = 0; i < this->getNumElseIfs(); ++i) {
    ScXMLElseIfElt * elseif = this->getElseIf(i);
    res = evaluator->evaluate(elseif->getCondAttribute());
    if (res) {
      if (res->isOfType(ScXMLBoolDataObj::getClassTypeId())) {
        boolres = static_cast<ScXMLBoolDataObj *>(res);
        if (boolres->getBool()) {
          for (int j = 0; j < this->getNumExecutables(elseif); ++j) {
            ScXMLExecutableElt * executable = this->getExecutable(elseif, j);
            executable->execute(statemachine);
          }
          return;
        }
      }
      else {
        statemachine->queueInternalEvent("error.eval.minimum.ElseIfCond.NOT_A_TRUTH_EXPRESSION");
        return;
      }
    }
    else {
      statemachine->queueInternalEvent("error.eval.minimum.ElseIfCond.NO_VALID_EXPRESSION");
      return;
    }
  }

  if (this->getElse()) {
    ScXMLElseElt * elseelt = this->getElse();
    for (int i = 0; i < this->getNumExecutables(elseelt); ++i) {
      ScXMLExecutableElt * executable = this->getExecutable(elseelt, i);
      executable->execute(statemachine);
    }
    return;
  }
}

SCXML_LIST_OBJECT_API_IMPL(ScXMLIfElt, ScXMLElseIfElt, PRIVATE(this)->elseiflist, ElseIf, ElseIfs);

SCXML_SINGLE_OBJECT_API_IMPL(ScXMLIfElt, ScXMLElseElt, PRIVATE(this)->elseelt, Else);

int
ScXMLIfElt::getNumExecutables(const ScXMLExecutableElt * conditional) const
{
  int idx = PRIVATE(this)->findIdx(conditional);
  if (unlikely(idx == -1)) {
    assert(0 && "given conditional not found");
    return -1;
  }
  std::vector<ScXMLExecutableElt *> * array = PRIVATE(this)->getArrayForIdx(idx);
  if (array) {
    return int(array->size());
  }
  return 0;
}

ScXMLExecutableElt *
ScXMLIfElt::getExecutable(const ScXMLExecutableElt * conditional, int idx) const
{
  int condidx = PRIVATE(this)->findIdx(conditional);
  if (unlikely(condidx == -1)) {
    assert(0 && "given conditional not found");
    return NULL;
  }
  std::vector<ScXMLExecutableElt *> * array = PRIVATE(this)->getArrayForIdx(condidx);
  if (unlikely(!array)) {
    assert(0 && "given conditional has no executables");
    return NULL;
  }
  if (unlikely(idx >= int(array->size()))) {
    assert(0 && "given idx too large for given conditional");
    return NULL;
  }
  return array->at(idx);
}

void
ScXMLIfElt::addExecutable(ScXMLExecutableElt * conditional, ScXMLExecutableElt * executable)
{
  int idx = PRIVATE(this)->findIdx(conditional);
  if (unlikely(idx == -1)) {
    assert(0 && "given conditional not found");
    return;
  }
  PRIVATE(this)->assureArrayForIdx(idx);
  std::vector<ScXMLExecutableElt *> * array = PRIVATE(this)->getArrayForIdx(idx);
  assert(array);
  array->push_back(executable);
}

void
ScXMLIfElt::removeExecutable(ScXMLExecutableElt * conditional, ScXMLExecutableElt * executable)
{
  int idx = PRIVATE(this)->findIdx(conditional);
  if (unlikely(idx == -1)) {
    assert(0 && "given conditional not found");
    return;
  }
  std::vector<ScXMLExecutableElt *> * array = PRIVATE(this)->getArrayForIdx(idx);
  if (unlikely(!array)) {
    assert(0 && "given conditional has no executables");
    return;
  }
  std::vector<ScXMLExecutableElt *>::iterator it =
    std::find(array->begin(), array->end(), executable);
  if (unlikely(it == array->end())) {
    assert(0 && "given conditional does not have given executable");
    return;
  }
  array->erase(it);
}

void
ScXMLIfElt::clearAllExecutables(ScXMLExecutableElt * conditional)
{
  int condidx = PRIVATE(this)->findIdx(conditional);
  if (unlikely(condidx == -1)) {
    assert(0 && "given conditional not found");
    return;
  }
  std::vector<ScXMLExecutableElt *> * array = PRIVATE(this)->getArrayForIdx(condidx);
  if (!array) {
    return;
  }
  for (int idx = 0; idx < int(array->size()); ++idx) {
    delete array->at(idx);
  }
  delete array;
  PRIVATE(this)->executables.at(condidx) = NULL;
}

#undef PRIVATE
