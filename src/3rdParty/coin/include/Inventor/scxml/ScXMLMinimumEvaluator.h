#ifndef COIN_SCXMLMINIMUMEVALUATOR_H
#define COIN_SCXMLMINIMUMEVALUATOR_H

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

#include <Inventor/scxml/ScXMLEvaluator.h>
#include <Inventor/tools/SbLazyPimplPtr.h>

class COIN_DLL_API ScXMLMinimumEvaluator : public ScXMLEvaluator {
  typedef ScXMLEvaluator inherited;
  SCXML_OBJECT_HEADER(ScXMLMinimumEvaluator)

public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLMinimumEvaluator(void);
  virtual ~ScXMLMinimumEvaluator(void);

  virtual void setStateMachine(ScXMLStateMachine *);

  virtual ScXMLDataObj * evaluate(const char * expression) const;

  virtual SbBool setAtLocation(const char * location, ScXMLDataObj * obj);
  virtual ScXMLDataObj * locate(const char * location) const;

  virtual void clearTemporaryVariables(void);

private:
  class PImpl;
  SbLazyPimplPtr<PImpl> pimpl;

}; // ScXMLMinimumEvaluator


class COIN_DLL_API ScXMLMinimumExprDataObj : public ScXMLExprDataObj {
  SCXML_OBJECT_ABSTRACT_HEADER(ScXMLMinimumExprDataObj)
  typedef ScXMLExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

}; // ScXMLMinimumDataObj


class COIN_DLL_API ScXMLInExprDataObj : public ScXMLMinimumExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLMinimumExprDataObj)
  typedef ScXMLExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(const char * stateid);

  ScXMLInExprDataObj(void);
  ScXMLInExprDataObj(const char * stateid);
  virtual ~ScXMLInExprDataObj(void);

  void setStateId(const char * stateid);
  const char * getStateId(void) const { return this->stateid; }

protected:
  char * stateid;

  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const;

}; // ScXMLMinimumDataObj


class COIN_DLL_API ScXMLAppendOpExprDataObj : public ScXMLExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLAppendOpExprDataObj)
  typedef ScXMLExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(ScXMLDataObj * lhs, ScXMLDataObj * rhs);

  ScXMLAppendOpExprDataObj(void);
  ScXMLAppendOpExprDataObj(ScXMLDataObj * lhs, ScXMLDataObj * rhs);
  virtual ~ScXMLAppendOpExprDataObj(void);

  void setLHS(ScXMLDataObj * lhs);
  const ScXMLDataObj * getLHS(void) const { return this->lhs; }
  void setRHS(ScXMLDataObj * rhs);
  const ScXMLDataObj * getRHS(void) const { return this->rhs; }

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const;

private:
  ScXMLDataObj * lhs, * rhs;

};

#endif // !COIN_SCXMLMINIMUMEVALUATOR_H
