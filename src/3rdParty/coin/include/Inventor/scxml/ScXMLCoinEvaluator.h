#ifndef COIN_SCXMLCOINEVALUATOR_H
#define COIN_SCXMLCOINEVALUATOR_H

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
#include <Inventor/tools/SbPimplPtr.h>

class COIN_DLL_API ScXMLCoinEvaluator : public ScXMLEvaluator {
  typedef ScXMLEvaluator inherited;
  SCXML_OBJECT_HEADER(ScXMLCoinEvaluator)

public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLCoinEvaluator(void);
  virtual ~ScXMLCoinEvaluator(void);

  virtual void setStateMachine(ScXMLStateMachine *sm);

  virtual ScXMLDataObj * evaluate(const char * expression) const;

  virtual SbBool setAtLocation(const char * location, ScXMLDataObj * obj);
  virtual ScXMLDataObj * locate(const char * location) const;

  virtual void clearTemporaryVariables(void);
  void dumpTemporaries(void);

private:
  class PImpl;
  SbPimplPtr<PImpl> pimpl;

}; // ScXMLCoinEvaluator

class COIN_DLL_API ScXMLCoinEqualsOpExprDataObj : public ScXMLEqualsOpExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLCoinEqualsOpExprDataObj)
  typedef ScXMLEqualsOpExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj * & pointer) const;

};

class COIN_DLL_API ScXMLCoinAddOpExprDataObj : public ScXMLAddOpExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLCoinAddOpExprDataObj)
  typedef ScXMLAddOpExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj * & pointer) const;

};

class COIN_DLL_API ScXMLCoinSubtractOpExprDataObj : public ScXMLSubtractOpExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLCoinSubtractOpExprDataObj)
  typedef ScXMLSubtractOpExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(ScXMLDataObj * lhs, ScXMLDataObj * rhs);

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj * & pointer) const;

};

class COIN_DLL_API ScXMLCoinMultiplyOpExprDataObj : public ScXMLMultiplyOpExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLCoinMultiplyOpExprDataObj)
  typedef ScXMLMultiplyOpExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj * & pointer) const;

};

class COIN_DLL_API ScXMLCoinDivideOpExprDataObj : public ScXMLDivideOpExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLCoinDivideOpExprDataObj)
  typedef ScXMLDivideOpExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj * & pointer) const;

};

class COIN_DLL_API ScXMLCoinLengthFuncExprDataObj : public ScXMLExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLCoinLengthFuncExprDataObj)
  typedef ScXMLExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(ScXMLDataObj * obj);

  ScXMLCoinLengthFuncExprDataObj(void);
  ScXMLCoinLengthFuncExprDataObj(ScXMLDataObj * obj);
  virtual ~ScXMLCoinLengthFuncExprDataObj(void);

  void setExpr(ScXMLDataObj * obj);
  ScXMLDataObj * getExpr(void) const { return this->expr; }

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj * & pointer) const;

  ScXMLDataObj * expr;

};

#endif // !COIN_SCXMLCOINEVALUATOR_H
