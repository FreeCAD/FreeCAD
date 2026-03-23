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

/*!
  \class SoBoolOperation SoBoolOperation.h Inventor/engines/SoBoolOperation.h
  \brief The SoBoolOperation engine evaluates expressions of boolean logic.

  \ingroup coin_engines

  The multivalue fields SoBoolOperation::a and SoBoolOperation::b are
  combined according to the operations set in
  SoBoolOperation::operation, with the resulting \c TRUE or \c FALSE
  value set on SoBoolOperation::output.
*/

/*!
  \enum SoBoolOperation::Operation
  Enumeration of available boolean logic operators.
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::CLEAR
  Always set SoBoolOperation::output to \c FALSE, no matter the input
  values.
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::SET
  Always set SoBoolOperation::output to \c TRUE, no matter the input
  values.
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::A
  Output result = SoBoolOperation::a
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::NOT_A
  Output result = ! SoBoolOperation::a
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::B
  Output result = SoBoolOperation::b
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::NOT_B
  Output result = ! SoBoolOperation::b
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::A_OR_B
  r = a || b
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::NOT_A_OR_B
  r = !a || b
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::A_OR_NOT_B
  r = a || !b
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::NOT_A_OR_NOT_B
  r = !a || !b
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::A_AND_B
  r = a && b
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::NOT_A_AND_B
  r = !a && b
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::A_AND_NOT_B
  r = a && !b
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::NOT_A_AND_NOT_B
  r = !a && !b
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::A_EQUALS_B
  r = a==b
*/
/*!
  \var SoBoolOperation::Operation SoBoolOperation::A_NOT_EQUALS_B
  r = ! a==b
*/

/*!
  \var SoMFBool SoBoolOperation::a
  First set of boolean input "signal" values.
*/
/*!
  \var SoMFBool SoBoolOperation::b
  Second set of boolean input "signal" values.
*/
/*!
  \var SoMFEnum SoBoolOperation::operation

  Set of boolean logic expressions. Each of these are used to combine
  SoBoolOperation::a with SoBoolOperation::b (for each index value
  from 0 to the last value) to produce the results on the
  SoBoolOperation::output field.
*/

/*!
  \var SoEngineOutput SoBoolOperation::output
  (SoMFBool) The result of each (a[i] operation[i] b[i]) expression.
*/
/*!
  \var SoEngineOutput SoBoolOperation::inverse
  (SoMFBool) The set of inverse results.
*/

#include <Inventor/engines/SoBoolOperation.h>
#include <Inventor/lists/SoEngineOutputList.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "engines/SoSubEngineP.h"

SO_ENGINE_SOURCE(SoBoolOperation);

/*!
  Default constructor.
*/
SoBoolOperation::SoBoolOperation(void)
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoBoolOperation);

  SO_ENGINE_ADD_INPUT(a,(FALSE));
  SO_ENGINE_ADD_INPUT(b,(FALSE));
  SO_ENGINE_ADD_INPUT(operation,(SoBoolOperation::A));

  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, CLEAR);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, SET);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, A);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, NOT_A);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, B);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, NOT_B);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, A_OR_B);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, NOT_A_OR_B);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, A_OR_NOT_B);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, NOT_A_OR_NOT_B);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, A_AND_B);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, NOT_A_AND_B);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, A_AND_NOT_B);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, NOT_A_AND_NOT_B);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, A_EQUALS_B);
  SO_ENGINE_DEFINE_ENUM_VALUE(Operation, A_NOT_EQUALS_B);

  SO_ENGINE_SET_MF_ENUM_TYPE(operation, Operation);

  SO_ENGINE_ADD_OUTPUT(output, SoMFBool);
  SO_ENGINE_ADD_OUTPUT(inverse, SoMFBool);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoBoolOperation::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoBoolOperation);
}

/*!
  Destructor.
*/
SoBoolOperation::~SoBoolOperation()
{
}

// doc from parent
void
SoBoolOperation::evaluate(void)
{
  int numA = this->a.getNum();
  int numB = this->b.getNum();
  int numOp = this->operation.getNum();

  int numOut = SbMax(SbMax(numA, numB), numOp);

  SO_ENGINE_OUTPUT(output,SoMFBool,setNum(numOut));
  SO_ENGINE_OUTPUT(inverse,SoMFBool,setNum(numOut));

  for (int i = 0; i < numOut; i++) {
    SbBool tmp_a = numA==0 ? FALSE : (i < numA ? this->a[i] : this->a[numA-1]);
    SbBool tmp_b = numB==0 ? FALSE : (i < numB ? this->b[i] : this->b[numB-1]);
    int tmp_op = numOp==0 ? SoBoolOperation::CLEAR :
      (i < numOp ? this->operation[i] : this->operation[numOp-1]);

    SbBool val;

    switch (tmp_op) {
    case SoBoolOperation::CLEAR:
      val = FALSE;
      break;
    case SoBoolOperation::SET:
      val = TRUE;
      break;
    case SoBoolOperation::A:
      val = tmp_a;
      break;
    case SoBoolOperation::NOT_A:
      val = !tmp_a;
      break;
    case SoBoolOperation::B:
      val = tmp_b;
      break;
    case SoBoolOperation::NOT_B:
      val = !tmp_b;
      break;
    case SoBoolOperation::A_OR_B:
      val = tmp_a || tmp_b;
      break;
    case SoBoolOperation::NOT_A_OR_B:
      val = !tmp_a || tmp_b;
      break;
    case SoBoolOperation::A_OR_NOT_B:
      val = tmp_a || !tmp_b;
      break;
    case SoBoolOperation::NOT_A_OR_NOT_B:
      val = !tmp_a || !tmp_b;
      break;
    case SoBoolOperation::A_AND_B:
      val = tmp_a && tmp_b;
      break;
    case SoBoolOperation::NOT_A_AND_B:
      val = !tmp_a && tmp_b;
      break;
    case SoBoolOperation::A_AND_NOT_B:
      val = tmp_a && !tmp_b;
      break;
    case SoBoolOperation::NOT_A_AND_NOT_B:
      val = !tmp_a && !tmp_b;
      break;
    case SoBoolOperation::A_EQUALS_B:
      val = (tmp_a==tmp_b);
      break;
    case SoBoolOperation::A_NOT_EQUALS_B:
      val = (tmp_a != tmp_b);
      break;
    default:
#if COIN_DEBUG
      SoDebugError::postInfo("SoBoolOperation::evaluate",
                             "unknown bool operation");
#endif // COIN_DEBUG
      val = TRUE; // avoid compiler warning
      break;
    }

    // FIXME: this seems extremely inefficient -- won't there be heaps
    // of notifications if there are many values in (at least) one of
    // the input fields? 20000915 mortene.
    SO_ENGINE_OUTPUT(output, SoMFBool, set1Value(i, val));
    SO_ENGINE_OUTPUT(inverse, SoMFBool, set1Value(i, !val));
  }
}
