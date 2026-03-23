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

#ifndef COIN_SOINTERPOLATE_H
#define COIN_SOINTERPOLATE_H


#include <Inventor/engines/SoSubEngine.h>
#include <Inventor/engines/SoEngineOutput.h>
#include <Inventor/fields/SoSFFloat.h>

class COIN_DLL_API SoInterpolate : public SoEngine {
  typedef SoEngine inherited;
  SO_ENGINE_ABSTRACT_HEADER(SoInterpolate);

public:
  static void initClass(void);
  static void initClasses(void);

  SoSFFloat alpha;
  SoEngineOutput output; // type varies for subclasses

protected:
  SoInterpolate();
  virtual ~SoInterpolate(void);
};



//// Start macros ////////////////////////////////////////////////////////


#define SO_INTERPOLATE_HEADER(_class_) \
  SO_ENGINE_HEADER(_class_); \
  public: \
    _class_(); \
    static void initClass(); \
  protected: \
    virtual ~_class_(); \
  private: \
    virtual void evaluate()


#define PRIVATE_SO_INTERPOLATE_CONSTRUCTOR(_class_, _type_, _valtype_, _default0_, _default1_) \
  SO_ENGINE_CONSTRUCTOR(_class_); \
  SO_ENGINE_ADD_INPUT(alpha, (0.0f)); \
  SO_ENGINE_ADD_INPUT(input0, _default0_); \
  SO_ENGINE_ADD_INPUT(input1, _default1_); \
  SO_ENGINE_ADD_OUTPUT(output, _type_)

#define PRIVATE_SO_INTERPOLATE_DESTRUCTOR(_class_) \
_class_::~_class_() \
{ \
}

#define PRIVATE_SO_INTERPOLATE_EVALUATE(_class_, _type_, _valtype_, _interpexp_) \
void \
_class_::evaluate(void) \
{ \
  int n0 = this->input0.getNum(); \
  int n1 = this->input1.getNum(); \
  float a = this->alpha.getValue(); \
  for (int i = SbMax(n0, n1) - 1; i >= 0; i--) { \
    _valtype_ v0 = this->input0[SbMin(i, n0-1)]; \
    _valtype_ v1 = this->input1[SbMin(i, n1-1)]; \
    SO_ENGINE_OUTPUT(output, _type_, set1Value(i, _interpexp_)); \
  } \
}


// Considering the number of lines of code needed to implement
// the evaluate() method in each class, I'm amazed it is defined in
// a macro and not simply implemented for each class. But, I guess
// we'll have to supply this macro to keep the OIV compatibility,
// so here it is. Check the interpolator classes for examples on
// how to use it.
//                                               pederb, 20000309
//
// There's another version of this macro for internal use in the
// SoSubEngineP.h file, so match any changes you do here with that
// macro also -- if applicable.
//
//                                               mortene, 20000505

#define SO_INTERPOLATE_SOURCE(_class_, _type_, _valtype_, _default0_, _default1_, _interpexp_) \
 \
SO_ENGINE_SOURCE(_class_); \
 \
_class_::_class_(void) \
{ \
  PRIVATE_SO_INTERPOLATE_CONSTRUCTOR(_class_, _type_, _valtype_, _default0_, _default1_); \
  this->isBuiltIn = FALSE; \
} \
 \
PRIVATE_SO_INTERPOLATE_DESTRUCTOR(_class_) \
PRIVATE_SO_INTERPOLATE_EVALUATE(_class_, _type_, _valtype_, _interpexp_)


#define SO_INTERPOLATE_INITCLASS(_class_, _classname_) \
 \
void \
_class_::initClass(void) \
{ \
  SO_ENGINE_INIT_CLASS(_class_, SoInterpolate, "SoInterpolate"); \
}


//// End macros //////////////////////////////////////////////////////////


#ifndef COIN_INTERNAL
// Include these header files for better Open Inventor compatibility.
#include <Inventor/engines/SoInterpolateFloat.h>
#include <Inventor/engines/SoInterpolateVec2f.h>
#include <Inventor/engines/SoInterpolateVec3f.h>
#include <Inventor/engines/SoInterpolateVec4f.h>
#include <Inventor/engines/SoInterpolateRotation.h>
#endif // !COIN_INTERNAL


#endif // !COIN_SOINTERPOLATE_H
