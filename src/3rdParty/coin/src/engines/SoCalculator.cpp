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
  \class SoCalculator Inventor/engines/SoCalculator.h
  \brief General purpose calculator for floats and 3D float vectors.

  \ingroup coin_engines

  The SoCalculator uses the values from the input fields (which are
  either single floating point values or vectors) as variables in the
  free-form engine expressions and places the results on the output
  fields.

  The engine has sixteen input fields; eight scalar inputs (\e a, \e
  b, \e c, \e d, \e e, \e f, \e g, and \e h), and eight vector inputs (\e A,
  \e B, \e C, \e D, \e E, \e F, \e G, and \e H).

  There are eight output fields; four scalar outputs (\e oa, \e ob, \e
  oc, and \e od), and four vector outputs (\e oA, \e oB, \e oC, and \e
  oD).

  The expression syntax is quite similar to C/C++, with a very limited
  set of keywords and functions.

  An example:

  \code

  oa = a * (0.5 + b) / c

  \endcode

  Will multiply the value in \e a with the value in \e b plus 0.5,
  divide that result with \e c, and place the result in \e oa. Since
  this is an engine, the expression will only be evaluated when
  someone attempts to read the value in \e oa, not every time an input
  in changed.

  All inputs are multi-fields, and if there are several values in an
  input, the expression will be evaluated once for every input field
  value, and the output will create as many values as there are input
  field values.

  If there is more than one input field, and the input fields do not
  have the same number of values, the engine will create as many
  output values as the input field with the biggest number of
  values. When the index get out of bounds for some other input field,
  the last field value will be used.

  Vector expressions are similar to scalar expression. An example:

  \code

  oA = A + vec3f(1.0, 0.0, 0.0) * B

  \endcode

  Will take the vector in \e A, add the value in \e B multiplied with
  (1,0,0), and place the result in \e oA.

  In addition to regular arithmetics, the SoCalculator syntax also
  includes some functions.

  Scalar functions:

  \li cos(x) - cosine function (x in radians)
  \li sin(x) - sinus function
  \li tan(x) - tangent function
  \li acos(x) - arc cosine function
  \li asin(x) - arc sinus function
  \li atan(x) - arc tangent function
  \li atan2(y, x) - arc tangent function of two variables (y, x).
  \li cosh(x) - hyperbolic cosine function
  \li sinh(x) - hyperbolic sinus function
  \li tanh(x) - hyperbolic tangent function
  \li sqrt(x) - square root function
  \li pow(x,y) - x raised to the power of y
  \li exp(x) - e to the power of x
  \li log(x) - natural logarithm of x
  \li log10() - base-10 logarithm of x
  \li ceil(x) - rounds x upwards to the nearest integer
  \li floor(x) - rounds x downwards to the nearest integer
  \li fabs(x) - absolute value
  \li fmod(x, y) - remainder of dividing x by y
  \li rand(x) - pseudo-random value between 0 and 1

  Vector functions:

  \li cross(x, y) - cross product of x and y
  \li dot(x,y) - dot product of x and y (returns scalar value)
  \li length(x) - length of x (returns scalar value)
  \li normalize(x) - returns normalized version of x
  \li x[y] - access components in x (y should be a scalar value in the range [0,2])

  There are also some named constants that can be used:

  \li MAXFLOAT
  \li MINFLOAT
  \li M_E
  \li M_LOG2E
  \li M_LOG10E
  \li M_LN2
  \li M_PI
  \li M_SQRT2 - sqrt(2)
  \li M_SQRT1_2 - sqrt(1/2)

  The only control flow available is the \e ? operator. An example:

  \code

  oa = (a > b) ? (a * 0.5) : (b * c)

  \endcode

  (The parentheses are not necessary, they're there just to make the
  example easier to read)

  In addition to the standard comparison operators (\e <, \e >, \e <=, \e >=,
  \e ==, \e !=), you can also use && (AND) and || (OR) to
  combine expression, and the unary ! (NOT) operator.

  One final thing worth mentioning are the temporary variables. There
  exist sixteen temporary variables that can be used in expressions.
  \e ta, \e tb, \e tc, \e td, \e te, \e tf, \e tg, and \e th are scalar
  variables, and \e tA, \e tB, \e tC, \e tD, \e tE, \e tF, \e tG, and \e tH
  are vector variables. They are usually used when you have more than
  one expression that should be evaluated in order.

  An example with three expressions:

  \code

  ta = a * b; tb = c + d; tc = e - f
  tA = vec3f(ta, tb, tc) + A
  oA = tA * B

  \endcode

  The example just shows how temporary variables can be used to make
  your expressions easier to read. Please note that it's possible to
  have several statements in one expression. You just separate them
  with semicolons.

  Here is a simple example of how an SoCalculator engine may be used
  in an .iv file:

  \code

  DEF mycamera PerspectiveCamera {
    orientation 1 0 0 1.57
  }

  DEF headlight DirectionalLight {
    intensity 0.8
    direction 0 0 1
  }

  Separator {
    # Render a cube not affected by lighting
    LightModel { model BASE_COLOR }
    BaseColor { rgb = Calculator {
                        a = USE headlight . intensity
                        expression [ "oA = vec3f( a, a, a)" ]
                      } . oA }
    Cube {}
  }

  \endcode

  In the example, the color of the Cube is a function of the intensity
  of the DirectionalLight, even though the Cube is rendered without
  lighting because of the BASE_COLOR LightModel.

*/

#include <Inventor/engines/SoCalculator.h>

#include "SbBasicP.h"

#include <cassert>

#include <Inventor/lists/SoEngineOutputList.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "engines/evaluator.h"
#include "engines/SoSubEngineP.h"

/*!
  \var SoMFFloat SoCalculator::a
  Input floating point value for the expressions.
*/
/*!
  \var SoMFFloat SoCalculator::b
  Input floating point value for the expressions.
*/
/*!
  \var SoMFFloat SoCalculator::c
  Input floating point value for the expressions.
*/
/*!
  \var SoMFFloat SoCalculator::d
  Input floating point value for the expressions.
*/
/*!
  \var SoMFFloat SoCalculator::e
  Input floating point value for the expressions.
*/
/*!
  \var SoMFFloat SoCalculator::f
  Input floating point value for the expressions.
*/
/*!
  \var SoMFFloat SoCalculator::g
  Input floating point value for the expressions.
*/
/*!
  \var SoMFFloat SoCalculator::h
  Input floating point value for the expressions.
*/
/*!
  \var SoMFVec3f SoCalculator::A
  Input vector with three floating point values for the expressions.
*/
/*!
  \var SoMFVec3f SoCalculator::B
  Input vector with three floating point values for the expressions.
*/
/*!
  \var SoMFVec3f SoCalculator::C
  Input vector with three floating point values for the expressions.
*/
/*!
  \var SoMFVec3f SoCalculator::D
  Input vector with three floating point values for the expressions.
*/
/*!
  \var SoMFVec3f SoCalculator::E
  Input vector with three floating point values for the expressions.
*/
/*!
  \var SoMFVec3f SoCalculator::F
  Input vector with three floating point values for the expressions.
*/
/*!
  \var SoMFVec3f SoCalculator::G
  Input vector with three floating point values for the expressions.
*/
/*!
  \var SoMFVec3f SoCalculator::H
  Input vector with three floating point values for the expressions.
*/
/*!
  \var SoMFString SoCalculator::expression
  Mathematical expressions for the calculator.
*/
/*!
  \var SoEngineOutput SoCalculator::oa
  (SoMFFloat) Output value with result from the calculations.
*/
/*!
  \var SoEngineOutput SoCalculator::ob
  (SoMFFloat) Output value with result from the calculations.
*/
/*!
  \var SoEngineOutput SoCalculator::oc
  (SoMFFloat) Output value with result from the calculations.
*/
/*!
  \var SoEngineOutput SoCalculator::od
  (SoMFFloat) Output value with result from the calculations.
*/
/*!
  \var SoEngineOutput SoCalculator::oA
  (SoMFVec3f) Output value with result from the calculations.
*/
/*!
  \var SoEngineOutput SoCalculator::oB
  (SoMFVec3f) Output value with result from the calculations.
*/
/*!
  \var SoEngineOutput SoCalculator::oC
  (SoMFVec3f) Output value with result from the calculations.
*/
/*!
  \var SoEngineOutput SoCalculator::oD
  (SoMFVec3f) Output value with result from the calculations.
*/

class SoCalculatorP {
public:
  float ta_th[8];
  SbVec3f tA_tH[8];

  float a_h[8];
  SbVec3f A_H[8];
  float oa_od[4];
  SbVec3f oA_oD[4];
  SbList <struct so_eval_node*> evaluatorList;
};

#define PRIVATE(thisp) (thisp->pimpl)
#define THISP(POINTER) static_cast<SoCalculator *>(POINTER)

SO_ENGINE_SOURCE(SoCalculator);

/*!
  Constructor.
*/
SoCalculator::SoCalculator(void)
{
  PRIVATE(this) = new SoCalculatorP;

  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoCalculator);

  SO_ENGINE_ADD_INPUT(a, (0.0f));
  SO_ENGINE_ADD_INPUT(b, (0.0f));
  SO_ENGINE_ADD_INPUT(c, (0.0f));
  SO_ENGINE_ADD_INPUT(d, (0.0f));
  SO_ENGINE_ADD_INPUT(e, (0.0f));
  SO_ENGINE_ADD_INPUT(f, (0.0f));
  SO_ENGINE_ADD_INPUT(g, (0.0f));
  SO_ENGINE_ADD_INPUT(h, (0.0f));
  SO_ENGINE_ADD_INPUT(A, (SbVec3f(0.0f, 0.0f, 0.0f)));
  SO_ENGINE_ADD_INPUT(B, (SbVec3f(0.0f, 0.0f, 0.0f)));
  SO_ENGINE_ADD_INPUT(C, (SbVec3f(0.0f, 0.0f, 0.0f)));
  SO_ENGINE_ADD_INPUT(D, (SbVec3f(0.0f, 0.0f, 0.0f)));
  SO_ENGINE_ADD_INPUT(E, (SbVec3f(0.0f, 0.0f, 0.0f)));
  SO_ENGINE_ADD_INPUT(F, (SbVec3f(0.0f, 0.0f, 0.0f)));
  SO_ENGINE_ADD_INPUT(G, (SbVec3f(0.0f, 0.0f, 0.0f)));
  SO_ENGINE_ADD_INPUT(H, (SbVec3f(0.0f, 0.0f, 0.0f)));
  SO_ENGINE_ADD_INPUT(expression, (""));

  SO_ENGINE_ADD_OUTPUT(oa, SoMFFloat);
  SO_ENGINE_ADD_OUTPUT(ob, SoMFFloat);
  SO_ENGINE_ADD_OUTPUT(oc, SoMFFloat);
  SO_ENGINE_ADD_OUTPUT(od, SoMFFloat);
  SO_ENGINE_ADD_OUTPUT(oA, SoMFVec3f);
  SO_ENGINE_ADD_OUTPUT(oB, SoMFVec3f);
  SO_ENGINE_ADD_OUTPUT(oC, SoMFVec3f);
  SO_ENGINE_ADD_OUTPUT(oD, SoMFVec3f);

  // initialize temporary registers (ta-th, tA-tH)
  int i;
  for (i = 0; i < 8; i++) {
    PRIVATE(this)->ta_th[i] = 0.0f;
    PRIVATE(this)->tA_tH[i].setValue(0.0f, 0.0f, 0.0f);
  }
}

/*!
  Destructor.
*/
SoCalculator::~SoCalculator(void)
{
  for (int i = 0; i < PRIVATE(this)->evaluatorList.getLength(); i++) {
    so_eval_delete(PRIVATE(this)->evaluatorList[i]);
  }
  delete PRIVATE(this);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoCalculator::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoCalculator);
}

// Documented in superclass.
void
SoCalculator::evaluate(void)
{
  int i, j;

  if (this->expression.getNum() == 0 ||
      this->expression[0].getLength() == 0) return;

  if (PRIVATE(this)->evaluatorList.getLength() == 0) {
    for (i = 0; i < this->expression.getNum(); i++) {
      const SbString &s = this->expression[i];
      if (s.getLength()) {
        PRIVATE(this)->evaluatorList.append(so_eval_parse(s.getString()));
#if COIN_DEBUG
        if (so_eval_error()) {
          SoDebugError::postWarning("SoCalculator::evaluateExpression",
                                    "%s", so_eval_error());
        }
#endif // COIN_DEBUG
      }
      else PRIVATE(this)->evaluatorList.append(NULL);
    }
  }


  // find all fields used in all expressions
  int maxnum = 0;
  char inused[16]; /* a-h and A-H */
  char outused[8]; /* a-d and A-D */
  for (i = 0; i < 16; i++) inused[i] = 0;
  for (i = 0; i < 8; i++) outused[i] = 0;

  for (i = 0; i < PRIVATE(this)->evaluatorList.getLength(); i++) {
    this->findUsed(PRIVATE(this)->evaluatorList[i], inused, outused);
  }

  // find max number of values in used input fields
  char fieldname[2];
  fieldname[1] = 0;
  for (i = 0; i < 16; i++) {
    if (inused[i]) {
      if (i < 8) {
        fieldname[0] = 'a' + i;
      }
      else {
        fieldname[0] = 'A' + (i-8);
      }
      SoMField * field = coin_assert_cast<SoMField*>(this->getField(fieldname));
      maxnum = SbMax(maxnum, field->getNum());
    }
  }
  if (maxnum == 0) maxnum = 1; // in case only temporary registers were used

  if (outused[0]) { SO_ENGINE_OUTPUT(oa, SoMFFloat, setNum(maxnum)); }
  if (outused[1]) { SO_ENGINE_OUTPUT(ob, SoMFFloat, setNum(maxnum)); }
  if (outused[2]) { SO_ENGINE_OUTPUT(oc, SoMFFloat, setNum(maxnum)); }
  if (outused[3]) { SO_ENGINE_OUTPUT(od, SoMFFloat, setNum(maxnum)); }

  if (outused[4]) { SO_ENGINE_OUTPUT(oA, SoMFVec3f, setNum(maxnum)); }
  if (outused[5]) { SO_ENGINE_OUTPUT(oB, SoMFVec3f, setNum(maxnum)); }
  if (outused[6]) { SO_ENGINE_OUTPUT(oC, SoMFVec3f, setNum(maxnum)); }
  if (outused[7]) { SO_ENGINE_OUTPUT(oD, SoMFVec3f, setNum(maxnum)); }

  // loop through all fieldindices and evaluate
  for (i = 0; i < maxnum; i++) {
    // just initialize output registers to default values
    // (in case an expression reads from an output before setting its value)
    PRIVATE(this)->oA_oD[0] = SbVec3f(0.0f, 0.0f, 0.0f);
    PRIVATE(this)->oA_oD[1] = SbVec3f(0.0f, 0.0f, 0.0f);
    PRIVATE(this)->oA_oD[2] = SbVec3f(0.0f, 0.0f, 0.0f);
    PRIVATE(this)->oA_oD[3] = SbVec3f(0.0f, 0.0f, 0.0f);
    PRIVATE(this)->oa_od[0] = 0.0f;
    PRIVATE(this)->oa_od[1] = 0.0f;
    PRIVATE(this)->oa_od[2] = 0.0f;
    PRIVATE(this)->oa_od[3] = 0.0f;

    // evaluate all expressions for this fieldidx
    for (j = 0; j < PRIVATE(this)->evaluatorList.getLength(); j++) {
      if (PRIVATE(this)->evaluatorList[j]) {
        this->evaluateExpression(PRIVATE(this)->evaluatorList[j], i);
      }
    }
  }
}

// "extern C" wrapper and C-function typedefs are needed with the
// OSF1/cxx compiler (probably a bug in the compiler, but it doesn't
// seem to hurt to do this anyway).
extern "C" {
  typedef void(*C_func_read)(const char *, float *, void *);
  typedef void(*C_func_write)(const char *, float *, int, void *);
}

// evaluates a single expression from/into fieldidx
void
SoCalculator::evaluateExpression(struct so_eval_node *node, const int fieldidx)
{
  int i;

  char fieldname[2];
  fieldname[1] = 0;
  char inused[16]; /* a-h and A-H */
  char outused[8]; /* oa-od and oA-oD */

  so_eval_cbdata cbdata;
  cbdata.readfieldcb = reinterpret_cast<C_func_read>(SoCalculator::readfieldcb);
  cbdata.writefieldcb = reinterpret_cast<C_func_write>(SoCalculator::writefieldcb);
  cbdata.userdata = this;

  for (i = 0; i < 16; i++) inused[i] = 0;
  for (i = 0; i < 8; i++) outused[i] = 0;

  this->findUsed(node, inused, outused);

  // copy values from fields to temporary "registers" while evaluating
  for (i = 0; i < 8; i++) {
    if (inused[i]) {
      fieldname[0] = 'a' + i;
      SoMFFloat * field = coin_assert_cast<SoMFFloat *>(this->getField(fieldname));
      int num = field->getNum();
      if (num) PRIVATE(this)->a_h[i] = field->getValues(0)[SbMin(fieldidx, num-1)];
      else PRIVATE(this)->a_h[i] = 0.0f;
    }
  }
  for (i = 0; i < 8; i++) {
    if (inused[i+8]) {
      fieldname[0] = 'A' + i;
      SoMFVec3f * field = coin_assert_cast<SoMFVec3f *>(this->getField(fieldname));
      int num = field->getNum();
      if (num) PRIVATE(this)->A_H[i] = field->getValues(0)[SbMin(fieldidx, num-1)];
      else PRIVATE(this)->A_H[i] = SbVec3f(0.0f, 0.0f, 0.0f);
    }
  }
  so_eval_evaluate(node, &cbdata);

  // copy the output values from "registers" to engine output
  if (outused[0]) { SO_ENGINE_OUTPUT(oa, SoMFFloat, set1Value(fieldidx, PRIVATE(this)->oa_od[0])); }
  if (outused[1]) { SO_ENGINE_OUTPUT(ob, SoMFFloat, set1Value(fieldidx, PRIVATE(this)->oa_od[1])); }
  if (outused[2]) { SO_ENGINE_OUTPUT(oc, SoMFFloat, set1Value(fieldidx, PRIVATE(this)->oa_od[2])); }
  if (outused[3]) { SO_ENGINE_OUTPUT(od, SoMFFloat, set1Value(fieldidx, PRIVATE(this)->oa_od[3])); }

  if (outused[4]) { SO_ENGINE_OUTPUT(oA, SoMFVec3f, set1Value(fieldidx, PRIVATE(this)->oA_oD[0])); }
  if (outused[5]) { SO_ENGINE_OUTPUT(oB, SoMFVec3f, set1Value(fieldidx, PRIVATE(this)->oA_oD[1])); }
  if (outused[6]) { SO_ENGINE_OUTPUT(oC, SoMFVec3f, set1Value(fieldidx, PRIVATE(this)->oA_oD[2])); }
  if (outused[7]) { SO_ENGINE_OUTPUT(oD, SoMFVec3f, set1Value(fieldidx, PRIVATE(this)->oA_oD[3])); }
}



//
// find all input and output fields that are used in the expression(s)
// inused 0-7   => a-h
// inused 8-15  => A-H
// outused 0-3  => oa-od
// outused 4-7  => oA-oD
//
// inused and outused must be cleared before calling this method
//
// FIXME: this becomes a bottleneck if there are many SoCalculator
// engines in the scene graph which are updated all the time. See the
// SoGuiExamples/coin-competitions/SIM-20010914/kaos.cpp.in for some
// great test-code to use while profiling.  Could be solved by caching
// the set of expressions found.  20010917 mortene.
void
SoCalculator::findUsed(struct so_eval_node *node, char *inused, char *outused)
{
  if (node == NULL) return;

  if (node->id == ID_ASSIGN_FLT || node->id == ID_ASSIGN_VEC) {
    this->findUsed(node->child2, inused, outused); // traverse rhs
    // inspect lhs
    node = node->child1;
    if (node->regname[0] == 'o') { // only consider engine outputs
      if ((node->regname[1] >= 'A') && (node->regname[1] <= 'D')) {
        outused[node->regname[1]-'A'+4] = 1;
      }
      else {
        assert((node->regname[1] >= 'a') && (node->regname[1] <= 'd'));
        outused[node->regname[1]-'a'] = 1;
      }
    }
  }
  else {
    if (node->child1) this->findUsed(node->child1, inused, outused);
    if (node->child2) this->findUsed(node->child2, inused, outused);
    if (node->child3) this->findUsed(node->child3, inused, outused);
  }
  if (node->id == ID_FLT_REG) {
    if ((node->regname[0] >= 'a') && (node->regname[0] <= 'h')) {
      inused[node->regname[0]-'a'] = 1;
    }
  }
  else if (node->id == ID_VEC_REG || node->id == ID_VEC_REG_COMP) {
    if ((node->regname[0] >= 'A') && (node->regname[0] <= 'H')) {
      inused[node->regname[0]-'A'+8] = 1;
    }
  }
}

// Documented in superclass.
void
SoCalculator::inputChanged(SoField *which)
{
  // if expression changes we have to rebuild the eval tree structure
  if (which == &this->expression) {
    for (int i = 0; i < PRIVATE(this)->evaluatorList.getLength(); i++) {
      so_eval_delete(PRIVATE(this)->evaluatorList[i]);
    }
    PRIVATE(this)->evaluatorList.truncate(0);
  }
}

// callback from evaluator. Reads values from temporary registers
void
SoCalculator::readfieldcb(const char *fieldname, float *data, void *userdata)
{
  SoCalculator * thisp = THISP(userdata);
  if (fieldname[0] == 'o') {
    //
    // FIXME: I'm not quite sure if it should be legal to read from an
    // output field. Investigate. pederb, 20000307
    //

    // this will work if output was set in an earlier expression
    if ((fieldname[1] >= 'A') && (fieldname[1] <= 'D')) {
      int idx = fieldname[1] - 'A';
      data[0] = PRIVATE(thisp)->oA_oD[idx][0];
      data[1] = PRIVATE(thisp)->oA_oD[idx][1];
      data[2] = PRIVATE(thisp)->oA_oD[idx][2];
    }
    else {
      assert((fieldname[1] >= 'a') && (fieldname[1] <= 'd'));
      int idx = fieldname[1] - 'a';
      data[0] = PRIVATE(thisp)->oa_od[idx];
    }
  }
  else if (fieldname[0] == 't') {
    if ((fieldname[1] >= 'A') && (fieldname[1] <= 'H')) {
      int idx = fieldname[1] - 'A';
      data[0] = PRIVATE(thisp)->tA_tH[idx][0];
      data[1] = PRIVATE(thisp)->tA_tH[idx][1];
      data[2] = PRIVATE(thisp)->tA_tH[idx][2];
    }
    else {
      assert((fieldname[1] >= 'a') && (fieldname[1] <= 'h'));
      int idx = fieldname[1] - 'a';
      data[0] = PRIVATE(thisp)->ta_th[idx];
    }
  }
  else if ((fieldname[0] >= 'A') && (fieldname[0] <= 'H')) {
    int idx = fieldname[0] - 'A';
    data[0] = PRIVATE(thisp)->A_H[idx][0];
    data[1] = PRIVATE(thisp)->A_H[idx][1];
    data[2] = PRIVATE(thisp)->A_H[idx][2];
  }
  else {
    assert((fieldname[0] >= 'a') && (fieldname[0] <= 'h'));
    int idx = fieldname[0] - 'a';
    data[0] = PRIVATE(thisp)->a_h[idx];
  }
}

// callback from evaluator. Writes values into temporary registers
void
SoCalculator::writefieldcb(const char *fieldname, float *data,
                           int comp, void *userdata)
{
  SoCalculator * thisp = THISP(userdata);
  if (fieldname[0] == 'o') {
    if ((fieldname[1] >= 'A') && (fieldname[1] <= 'D')) {
      int idx = fieldname[1] - 'A';
      if (comp >= 0) {
        PRIVATE(thisp)->oA_oD[idx][comp] = data[0];
      }
      else {
        PRIVATE(thisp)->oA_oD[idx][0] = data[0];
        PRIVATE(thisp)->oA_oD[idx][1] = data[1];
        PRIVATE(thisp)->oA_oD[idx][2] = data[2];
      }
    }
    else {
      assert((fieldname[1] >= 'a') && (fieldname[1] <= 'd'));
      int idx = fieldname[1] - 'a';
      PRIVATE(thisp)->oa_od[idx] = data[0];
    }
  }
  else if (fieldname[0] == 't') {
    if ((fieldname[1] >= 'A') && (fieldname[1] <= 'H')) {
      int idx = fieldname[1] - 'A';
      if (comp >= 0) {
        PRIVATE(thisp)->tA_tH[idx][comp] = data[0];
      }
      else {
        PRIVATE(thisp)->tA_tH[idx][0] = data[0];
        PRIVATE(thisp)->tA_tH[idx][1] = data[1];
        PRIVATE(thisp)->tA_tH[idx][2] = data[2];
      }
    }
    else {
      assert((fieldname[1] >= 'a') && (fieldname[1] <= 'h'));
      int idx = fieldname[1] - 'a';
      PRIVATE(thisp)->ta_th[idx] = data[0];
    }
  }
  else {
    assert(0 && "should not happen");
  }
}

#undef THISP
#undef PRIVATE
