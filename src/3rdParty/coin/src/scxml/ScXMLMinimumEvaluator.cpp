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

#include <Inventor/scxml/ScXMLMinimumEvaluator.h>

/*!
  \class ScXMLMinimumExprDataObj ScXMLMinimumEvaluator.h Inventor/scxml/ScXMLMinimumEvaluator.h
  \brief implements the data objects for the evaluator for the minimum profile.

  \ingroup coin_scxml
*/

/*!
  \class ScXMLMinimumEvaluator ScXMLMinimumEvaluator.h Inventor/scxml/ScXMLMinimumEvaluator.h
  \brief implements the evaluator for the minimum profile.

  - In()

  \ingroup coin_scxml
*/

#include <cassert>
#include <cstring>

#include <memory>

#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/ScXMLAbstractStateElt.h>
#include <Inventor/scxml/ScXMLStateMachine.h>
#include "scxml/ScXMLCommonP.h"
#include "scxml/eval-minimum.h"

// *************************************************************************

class ScXMLMinimumEvaluator::PImpl {
public:
};

SCXML_OBJECT_SOURCE(ScXMLMinimumEvaluator);

void
ScXMLMinimumEvaluator::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLMinimumEvaluator, ScXMLEvaluator, "ScXMLEvaluator");
  ScXML::registerEvaluatorType(SbName("minimum"), classTypeId);
  ScXML::registerEvaluatorType(SbName("min"), classTypeId); // alias
  ScXML::registerEvaluatorType(SbName::empty(), classTypeId); // default
}

void
ScXMLMinimumEvaluator::cleanClass(void)
{
  ScXML::unregisterEvaluatorType(SbName::empty(), classTypeId); // default
  ScXML::unregisterEvaluatorType(SbName("min"), classTypeId); // alias
  ScXML::unregisterEvaluatorType(SbName("minimum"), classTypeId);
  ScXMLMinimumEvaluator::classTypeId = SoType::badType();
}

ScXMLMinimumEvaluator::ScXMLMinimumEvaluator(void)
{
}

ScXMLMinimumEvaluator::~ScXMLMinimumEvaluator(void)
{
}

void
ScXMLMinimumEvaluator::setStateMachine(ScXMLStateMachine * sm)
{
  inherited::setStateMachine(sm);
  if (sm) {
    SbList<const char *> modules;
    modules.append("core");
    modules.append("externals");
    sm->setEnabledModulesList(modules);
  }
}

/*!
  Returns the result of evaluating the expression.
  The only expression that is allowed is the In({ID}) predicate, and
  the result should, if all things go as they should, be of type
  ScXMLBoolDataObj.
*/
ScXMLDataObj *
ScXMLMinimumEvaluator::evaluate(const char * expression) const
{
  ScXMLDataObj * ret = scxml_minimum_parse(expression);
  return ret;
}

/*!
  Returns FALSE. The minimum profile does not implement the data module.
*/
SbBool
ScXMLMinimumEvaluator::setAtLocation(const char * COIN_UNUSED_ARG(location), ScXMLDataObj * COIN_UNUSED_ARG(obj))
{
  return FALSE;
}

/*!
  Returns NULL. The minimum profile does not implement the data module.
*/
ScXMLDataObj *
ScXMLMinimumEvaluator::locate(const char * COIN_UNUSED_ARG(location)) const
{
  return NULL;
}

/*!
  Does nothing.
*/
void
ScXMLMinimumEvaluator::clearTemporaryVariables(void)
{
}

// *************************************************************************

SCXML_OBJECT_ABSTRACT_SOURCE(ScXMLMinimumExprDataObj);

void
ScXMLMinimumExprDataObj::initClass(void)
{
  SCXML_OBJECT_INIT_ABSTRACT_CLASS(ScXMLMinimumExprDataObj, ScXMLExprDataObj, "ScXMLExprDataObj");
}

void
ScXMLMinimumExprDataObj::cleanClass(void)
{
  ScXMLMinimumExprDataObj::classTypeId = SoType::badType();
}

// *************************************************************************


SCXML_OBJECT_SOURCE(ScXMLInExprDataObj);

void
ScXMLInExprDataObj::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLInExprDataObj, ScXMLMinimumExprDataObj, "ScXMLMinimumExprDataObj");
}

void
ScXMLInExprDataObj::cleanClass(void)
{
  ScXMLInExprDataObj::classTypeId = SoType::badType();
}

ScXMLDataObj *
ScXMLInExprDataObj::createFor(const char * stateid)
{
  return new ScXMLInExprDataObj(stateid);
}

ScXMLInExprDataObj::ScXMLInExprDataObj(void)
: stateid(NULL)
{
}

ScXMLInExprDataObj::ScXMLInExprDataObj(const char * stateidstr)
: stateid(NULL)
{
  this->setStateId(stateidstr);
}

ScXMLInExprDataObj::~ScXMLInExprDataObj(void)
{
}

void
ScXMLInExprDataObj::setStateId(const char * stateidstr)
{
  delete [] this->stateid;
  this->stateid = NULL;
  if (stateidstr) {
    this->stateid = new char [strlen(stateidstr) + 1];
    strcpy(this->stateid, stateidstr);
  }
}

SbBool
ScXMLInExprDataObj::evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const
{
  assert(sm);
  assert(pointer == NULL);
  if (this->stateid) {
    sm->queueInternalEvent("error.eval.minimum.In.NO_STATE");
    return FALSE;
  }
  for (int i = 0; i < sm->getNumActiveStates(); ++i) {
    const ScXMLElt * stateelt = sm->getActiveState(i);
    const ScXMLAbstractStateElt * state =
      coin_assert_cast<const ScXMLAbstractStateElt *>(stateelt);
    const char * activestate = state->getIdAttribute();
    if (strcmp(activestate, this->stateid) == 0) {
      pointer = new ScXMLBoolDataObj(TRUE);
      return TRUE;
    }
  }
  pointer = new ScXMLBoolDataObj(FALSE);
  return TRUE;
}

// *************************************************************************

SCXML_OBJECT_SOURCE(ScXMLAppendOpExprDataObj);

void
ScXMLAppendOpExprDataObj::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLAppendOpExprDataObj, ScXMLExprDataObj, "ScXMLExprDataObj");
}

void
ScXMLAppendOpExprDataObj::cleanClass(void)
{
  ScXMLAppendOpExprDataObj::classTypeId = SoType::badType();
}

ScXMLDataObj *
ScXMLAppendOpExprDataObj::createFor(ScXMLDataObj * lhs, ScXMLDataObj * rhs)
{
  if (lhs->isOfType(ScXMLStringDataObj::getClassTypeId()) &&
      rhs->isOfType(ScXMLStringDataObj::getClassTypeId())) {
    ScXMLStringDataObj * lhsstring = static_cast<ScXMLStringDataObj *>(lhs);
    ScXMLStringDataObj * rhsstring = static_cast<ScXMLStringDataObj *>(rhs);
    std::unique_ptr<char[]> string(new char [strlen(lhsstring->getString()) + strlen(rhsstring->getString()) + 1]);
    strcpy(string.get(), lhsstring->getString());
    strcat(string.get(), rhsstring->getString());
    delete rhsstring;
    lhsstring->setString(string.get());
    return lhsstring;
  }
  return new ScXMLAppendOpExprDataObj(lhs, rhs);
}

ScXMLAppendOpExprDataObj::ScXMLAppendOpExprDataObj(void)
: lhs(NULL), rhs(NULL)
{
}

ScXMLAppendOpExprDataObj::ScXMLAppendOpExprDataObj(ScXMLDataObj * lhsptr, ScXMLDataObj * rhsptr)
: lhs(lhsptr), rhs(rhsptr)
{
}

ScXMLAppendOpExprDataObj::~ScXMLAppendOpExprDataObj(void)
{
  delete this->lhs;
  this->lhs = NULL;
  delete this->rhs;
  this->rhs = NULL;
}

void
ScXMLAppendOpExprDataObj::setLHS(ScXMLDataObj * lhsptr)
{
  this->lhs = lhsptr;
}

void
ScXMLAppendOpExprDataObj::setRHS(ScXMLDataObj * rhsptr)
{
  this->rhs = rhsptr;
}

SbBool
ScXMLAppendOpExprDataObj::evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const
{
  assert(this->lhs && this->rhs);
  ScXMLStringDataObj * lhsevaled = NULL, * rhsevaled = NULL;

  if (this->lhs->isOfType(ScXMLExprDataObj::getClassTypeId())) {
    ScXMLExprDataObj * lhsexpr = static_cast<ScXMLExprDataObj *>(this->lhs);
    ScXMLDataObj * evaled = lhsexpr->evaluate(sm);
    if (!evaled) {
      return FALSE;
    }
    if (!evaled->isOfType(ScXMLStringDataObj::getClassTypeId())) {
      sm->queueInternalEvent("error.eval.minimum.Append.INVALID_LHS");
      return FALSE;
    }
    lhsevaled = static_cast<ScXMLStringDataObj *>(evaled);
  }
  else if (this->lhs->isOfType(ScXMLStringDataObj::getClassTypeId())) {
    lhsevaled = static_cast<ScXMLStringDataObj *>(this->lhs);
  }
  else {
    sm->queueInternalEvent("error.eval.minimum.Append.INVALID_LHS");
    return FALSE;
  }

  if (this->rhs->isOfType(ScXMLExprDataObj::getClassTypeId())) {
    ScXMLExprDataObj * rhsexpr = static_cast<ScXMLExprDataObj *>(this->rhs);
    ScXMLDataObj * evaled = rhsexpr->evaluate(sm);
    if (!evaled) {
      return FALSE;
    }
    if (!evaled->isOfType(ScXMLStringDataObj::getClassTypeId())) {
      sm->queueInternalEvent("error.eval.minimum.Append.INVALID_RHS");
      return FALSE;
    }
    rhsevaled = static_cast<ScXMLStringDataObj *>(evaled);
  }
  else if (this->rhs->isOfType(ScXMLStringDataObj::getClassTypeId())) {
    rhsevaled = static_cast<ScXMLStringDataObj *>(this->rhs);
  }
  else {
    sm->queueInternalEvent("error.eval.minimum.Append.INVALID_RHS");
    return FALSE;
  }

  std::unique_ptr<char[]> string(new char [strlen(lhsevaled->getString()) + strlen(rhsevaled->getString()) + 1]);
  strcpy(string.get(), lhsevaled->getString());
  strcat(string.get(), rhsevaled->getString());

  pointer = new ScXMLStringDataObj(string.get());
  return TRUE;
}

#ifdef COIN_TEST_SUITE

#include <cmath>
#include <cfloat>

#include <Inventor/scxml/ScXMLStateMachine.h>
#include <Inventor/scxml/ScXMLDocument.h>
#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/ScXMLElt.h>
#include <Inventor/SbByteBuffer.h>
#include <Inventor/SbString.h>

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(MimimumExpressions,1);

BOOST_AUTO_TEST_CASE(MimimumExpressions)
{
  std::unique_ptr<ScXMLStateMachine> sm(new ScXMLStateMachine);
  std::unique_ptr<ScXMLEvaluator> evaluator(new ScXMLMinimumEvaluator);

  ScXMLDataObj * res = NULL;
  ScXMLBoolDataObj * boolobj = NULL;

  //FIXME, this test is not finished. BFG 20090831

  static const char foo [] =
    "<scxml version=\"1.0\" profile=\"minimum\" name=\"foo\" initial=\"active\"><state id=\"active\"></state><state id=\"inactive\"></state></scxml>";
  /*
  ScXMLDocument * doc = ScXMLDocument::readBuffer(SbByteBuffer(sizeof(foo),foo));
  assert(doc->getRoot());
  sm->setDescription(doc);
  sm->setName("<memory buffer>");
  sm->initialize();
  assert(sm->isActive() && !sm->isFinished());
  printf("%d\n", sm->getNumActiveStates());
  const ScXMLElt * elt = sm->getActiveState(0);
  printf("%s\n",elt->getXMLAttribute("id"));
  evaluator->setStateMachine(sm.get());
  res = evaluator->evaluate("In('active')");
  */

}

#endif // !COIN_TEST_SUITE
