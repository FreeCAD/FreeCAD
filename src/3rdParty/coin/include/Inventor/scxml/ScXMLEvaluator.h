#ifndef COIN_SCXMLEVALUATOR_H
#define COIN_SCXMLEVALUATOR_H

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

#include <Inventor/scxml/ScXMLObject.h>
#include <Inventor/tools/SbLazyPimplPtr.h>

class ScXMLDocument;
class ScXMLStateMachine;
class ScXMLDataObj;


class COIN_DLL_API ScXMLEvaluator : public ScXMLObject {
  typedef ScXMLObject inherited;
  SCXML_OBJECT_ABSTRACT_HEADER(ScXMLEvaluator)
public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLEvaluator(void);
  virtual ~ScXMLEvaluator(void);

  virtual void setStateMachine(ScXMLStateMachine * sm);
  ScXMLStateMachine * getStateMachine(void) const { return this->statemachine; }

  virtual ScXMLDataObj * evaluate(const char * expression) const = 0;

  virtual SbBool setAtLocation(const char * location, ScXMLDataObj * obj) = 0;
  virtual ScXMLDataObj * locate(const char * location) const = 0;

  virtual void clearTemporaryVariables(void);

protected:

private:
  ScXMLStateMachine * statemachine;

  class PImpl;
  SbLazyPimplPtr<PImpl> pimpl;

}; // ScXMLEvaluator


// *************************************************************************

class COIN_DLL_API ScXMLDataObj : public ScXMLObject {
  SCXML_OBJECT_ABSTRACT_HEADER(ScXMLDataObj)
  typedef ScXMLObject inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLDataObj(void);
  virtual ~ScXMLDataObj(void);

  void setContainer(ScXMLObject * container);
  ScXMLObject * getContainer(void) const { return this->container; }

private:
  ScXMLObject * container;

}; // ScXMLDataObj

class COIN_DLL_API ScXMLConstantDataObj : public ScXMLDataObj {
  SCXML_OBJECT_ABSTRACT_HEADER(ScXMLConstantDataObj)
  typedef ScXMLDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  virtual ScXMLDataObj * clone(void) const = 0;
  virtual void convertToString(SbString & str) const = 0;

};


class COIN_DLL_API ScXMLStringDataObj : public ScXMLConstantDataObj {
  SCXML_OBJECT_HEADER(ScXMLStringDataObj)
  typedef ScXMLConstantDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(const char * value);

  ScXMLStringDataObj(void);
  ScXMLStringDataObj(const char * value);
  virtual ~ScXMLStringDataObj(void);

  void setString(const char * value);
  const char * getString(void) const { return this->value; }

  virtual ScXMLDataObj * clone(void) const;
  virtual void convertToString(SbString & str) const;

private:
  char * value;

};


class COIN_DLL_API ScXMLRealDataObj : public ScXMLConstantDataObj {
  SCXML_OBJECT_HEADER(ScXMLRealDataObj)
  typedef ScXMLConstantDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(double value);

  ScXMLRealDataObj(void);
  ScXMLRealDataObj(double value);
  virtual ~ScXMLRealDataObj(void);

  void setReal(double value);
  double getReal(void) const { return this->value; }

  virtual ScXMLDataObj * clone(void) const;
  virtual void convertToString(SbString & str) const;

private:
  double value;

};


class COIN_DLL_API ScXMLBoolDataObj : public ScXMLConstantDataObj {
  SCXML_OBJECT_HEADER(ScXMLBoolDataObj)
  typedef ScXMLConstantDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(SbBool value);

  ScXMLBoolDataObj(void);
  ScXMLBoolDataObj(SbBool value);
  virtual ~ScXMLBoolDataObj(void);

  void setBool(SbBool value);
  SbBool getBool(void) const { return this->value; }

  virtual ScXMLDataObj * clone(void) const;
  virtual void convertToString(SbString & str) const;

private:
  SbBool value;

};


class COIN_DLL_API ScXMLSbDataObj : public ScXMLConstantDataObj {
  SCXML_OBJECT_HEADER(ScXMLSbDataObj)
  typedef ScXMLConstantDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(const char * value);

  ScXMLSbDataObj(void);
  ScXMLSbDataObj(const char * value);
  virtual ~ScXMLSbDataObj(void);

  void setSbValue(const char * value);
  const char * getSbValue(void) const { return this->value; }

  virtual ScXMLDataObj * clone(void) const;
  virtual void convertToString(SbString & str) const;

private:
  char * value;

};


class COIN_DLL_API ScXMLXMLDataObj : public ScXMLDataObj {
  SCXML_OBJECT_HEADER(ScXMLXMLDataObj)
  typedef ScXMLDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLXMLDataObj(void);
  virtual ~ScXMLXMLDataObj(void);

  virtual ScXMLDataObj * clone(void) const;

};


class COIN_DLL_API ScXMLExprDataObj : public ScXMLDataObj {
  SCXML_OBJECT_ABSTRACT_HEADER(ScXMLExprDataObj)
  typedef ScXMLDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLExprDataObj(void);
  virtual ~ScXMLExprDataObj(void);

  ScXMLDataObj * evaluate(ScXMLStateMachine * sm);

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const = 0;

private:
  ScXMLDataObj * result;

};


class COIN_DLL_API ScXMLReferenceDataObj : public ScXMLExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLReferenceDataObj)
  typedef ScXMLExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(const char * reference);

  ScXMLReferenceDataObj(void);
  ScXMLReferenceDataObj(const char * reference);
  virtual ~ScXMLReferenceDataObj(void);

  void setReference(const char * reference);
  const char * getReference(void) const { return this->reference; }

  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const;

private:
  char * reference;

};


// *************************************************************************
// logical operators

class COIN_DLL_API ScXMLAndOpExprDataObj : public ScXMLExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLAndOpExprDataObj)
  typedef ScXMLExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(ScXMLDataObj * lhs, ScXMLDataObj * rhs);

  ScXMLAndOpExprDataObj(void);
  ScXMLAndOpExprDataObj(ScXMLDataObj * lhs, ScXMLDataObj * rhs);
  virtual ~ScXMLAndOpExprDataObj(void);

  void setLHS(ScXMLDataObj * lhs);
  const ScXMLDataObj * getLHS(void) const { return this->lhs; }
  void setRHS(ScXMLDataObj * rhs);
  const ScXMLDataObj * getRHS(void) const { return this->rhs; }

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const;

private:
  ScXMLDataObj * lhs, * rhs;

};


class COIN_DLL_API ScXMLOrOpExprDataObj : public ScXMLExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLOrOpExprDataObj)
  typedef ScXMLExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(ScXMLDataObj * lhs, ScXMLDataObj * rhs);

  ScXMLOrOpExprDataObj(void);
  ScXMLOrOpExprDataObj(ScXMLDataObj * lhs, ScXMLDataObj * rhs);
  virtual ~ScXMLOrOpExprDataObj(void);

  void setLHS(ScXMLDataObj * lhs);
  const ScXMLDataObj * getLHS(void) const { return this->lhs; }
  void setRHS(ScXMLDataObj * rhs);
  const ScXMLDataObj * getRHS(void) const { return this->rhs; }

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const;

private:
  ScXMLDataObj * lhs, * rhs;

};


class COIN_DLL_API ScXMLNotOpExprDataObj : public ScXMLExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLNotOpExprDataObj)
  typedef ScXMLExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(ScXMLDataObj * rhs);

  ScXMLNotOpExprDataObj(void);
  ScXMLNotOpExprDataObj(ScXMLDataObj * rhs);
  virtual ~ScXMLNotOpExprDataObj(void);

  void setRHS(ScXMLDataObj * rhs);
  const ScXMLDataObj * getRHS(void) const { return this->rhs; }

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const;

private:
  ScXMLDataObj * rhs;

};


class COIN_DLL_API ScXMLEqualsOpExprDataObj : public ScXMLExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLEqualsOpExprDataObj)
  typedef ScXMLExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(ScXMLDataObj * lhs, ScXMLDataObj * rhs);

  ScXMLEqualsOpExprDataObj(void);
  ScXMLEqualsOpExprDataObj(ScXMLDataObj * lhs, ScXMLDataObj * rhs);
  virtual ~ScXMLEqualsOpExprDataObj(void);

  void setLHS(ScXMLDataObj * lhs);
  const ScXMLDataObj * getLHS(void) const { return this->lhs; }
  void setRHS(ScXMLDataObj * rhs);
  const ScXMLDataObj * getRHS(void) const { return this->rhs; }

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const;

private:
  ScXMLDataObj * lhs, * rhs;

};


// *************************************************************************
// arithmetic operators

class COIN_DLL_API ScXMLAddOpExprDataObj : public ScXMLExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLAddOpExprDataObj)
  typedef ScXMLExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(ScXMLDataObj * lhs, ScXMLDataObj * rhs);

  ScXMLAddOpExprDataObj(void);
  ScXMLAddOpExprDataObj(ScXMLDataObj * lhs, ScXMLDataObj * rhs);
  virtual ~ScXMLAddOpExprDataObj(void);

  void setLHS(ScXMLDataObj * lhs);
  const ScXMLDataObj * getLHS(void) const { return this->lhs; }
  void setRHS(ScXMLDataObj * rhs);
  const ScXMLDataObj * getRHS(void) const { return this->rhs; }

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const;

private:
  ScXMLDataObj * lhs, * rhs;

};


class COIN_DLL_API ScXMLSubtractOpExprDataObj : public ScXMLExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLSubtractOpExprDataObj)
  typedef ScXMLExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(ScXMLDataObj * lhs, ScXMLDataObj * rhs);

  ScXMLSubtractOpExprDataObj(void);
  ScXMLSubtractOpExprDataObj(ScXMLDataObj * lhs, ScXMLDataObj * rhs);
  virtual ~ScXMLSubtractOpExprDataObj(void);

  void setLHS(ScXMLDataObj * lhs);
  const ScXMLDataObj * getLHS(void) const { return this->lhs; }
  void setRHS(ScXMLDataObj * rhs);
  const ScXMLDataObj * getRHS(void) const { return this->rhs; }

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const;

private:
  ScXMLDataObj * lhs, * rhs;

};


class COIN_DLL_API ScXMLMultiplyOpExprDataObj : public ScXMLExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLMultiplyOpExprDataObj)
  typedef ScXMLExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(ScXMLDataObj * lhs, ScXMLDataObj * rhs);

  ScXMLMultiplyOpExprDataObj(void);
  ScXMLMultiplyOpExprDataObj(ScXMLDataObj * lhs, ScXMLDataObj * rhs);
  virtual ~ScXMLMultiplyOpExprDataObj(void);

  void setLHS(ScXMLDataObj * lhs);
  const ScXMLDataObj * getLHS(void) const { return this->lhs; }
  void setRHS(ScXMLDataObj * rhs);
  const ScXMLDataObj * getRHS(void) const { return this->rhs; }

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const;

private:
  ScXMLDataObj * lhs, * rhs;

};


class COIN_DLL_API ScXMLDivideOpExprDataObj : public ScXMLExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLDivideOpExprDataObj)
  typedef ScXMLExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(ScXMLDataObj * lhs, ScXMLDataObj * rhs);

  ScXMLDivideOpExprDataObj(void);
  ScXMLDivideOpExprDataObj(ScXMLDataObj * lhs, ScXMLDataObj * rhs);
  virtual ~ScXMLDivideOpExprDataObj(void);

  void setLHS(ScXMLDataObj * lhs);
  const ScXMLDataObj * getLHS(void) const { return this->lhs; }
  void setRHS(ScXMLDataObj * rhs);
  const ScXMLDataObj * getRHS(void) const { return this->rhs; }

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const;

private:
  ScXMLDataObj * lhs, * rhs;

};


class COIN_DLL_API ScXMLNegateOpExprDataObj : public ScXMLExprDataObj {
  SCXML_OBJECT_HEADER(ScXMLNegateOpExprDataObj)
  typedef ScXMLExprDataObj inherited;
public:
  static void initClass(void);
  static void cleanClass(void);

  static ScXMLDataObj * createFor(ScXMLDataObj * rhs);

  ScXMLNegateOpExprDataObj(void);
  ScXMLNegateOpExprDataObj(ScXMLDataObj * rhs);
  virtual ~ScXMLNegateOpExprDataObj(void);

  void setRHS(ScXMLDataObj * rhs);
  const ScXMLDataObj * getRHS(void) const { return this->rhs; }

protected:
  virtual SbBool evaluateNow(ScXMLStateMachine * sm, ScXMLDataObj *& pointer) const;

private:
  ScXMLDataObj * rhs;

};


#endif // !COIN_SCXMLEVALUATOR_H
