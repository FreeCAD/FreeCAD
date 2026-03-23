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
  \class SoDecomposeMatrix SoDecomposeMatrix.h Inventor/engines/SoDecomposeMatrix.h
  \brief The SoDecomposeMatrix class is used to decompose a matrix into simple transformations.

  \ingroup coin_engines
*/

#include <Inventor/engines/SoDecomposeMatrix.h>
#include <Inventor/lists/SoEngineOutputList.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoMFRotation.h>

#include "engines/SoSubEngineP.h"

SO_ENGINE_SOURCE(SoDecomposeMatrix);

/*!
  \var SoMFMatrix SoDecomposeMatrix::matrix
  Set of transformation matrices to decompose into their
  translation/rotation/scale parts.
*/
/*!
  \var SoMFVec3f SoDecomposeMatrix::center
  Center points of transform matrices.
*/
/*!
  \var SoEngineOutput SoDecomposeMatrix::translation
  (SoMFVec3f) Translation parts of input matrices.
*/
/*!
  \var SoEngineOutput SoDecomposeMatrix::rotation
  (SoMFRotation) Rotation parts of input matrices.
*/
/*!
  \var SoEngineOutput SoDecomposeMatrix::scaleFactor
  (SoMFVec3f) Scale vectors of input matrices.
*/
/*!
  \var SoEngineOutput SoDecomposeMatrix::scaleOrientation
  (SoMFRotation) Scale orientation values of the input matrices.
*/


#ifndef DOXYGEN_SKIP_THIS // No need to document these.

// Default constructor.
SoDecomposeMatrix::SoDecomposeMatrix()
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoDecomposeMatrix);

  SO_ENGINE_ADD_INPUT(matrix,(SbMatrix()));
  SO_ENGINE_ADD_INPUT(center,(SbVec3f()));

  SO_ENGINE_ADD_OUTPUT(translation,SoMFVec3f);
  SO_ENGINE_ADD_OUTPUT(rotation,SoMFRotation);
  SO_ENGINE_ADD_OUTPUT(scaleFactor,SoMFVec3f);
  SO_ENGINE_ADD_OUTPUT(scaleOrientation,SoMFRotation);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoDecomposeMatrix::initClass()
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoDecomposeMatrix);
}

//
// private members
//
SoDecomposeMatrix::~SoDecomposeMatrix()
{
}

// Documented in superclass.
void
SoDecomposeMatrix::evaluate()
{
  int num = this->matrix.getNum();

  SO_ENGINE_OUTPUT(translation,SoMFVec3f,setNum(num));
  SO_ENGINE_OUTPUT(rotation,SoMFRotation,setNum(num));
  SO_ENGINE_OUTPUT(scaleFactor,SoMFVec3f,setNum(num));
  SO_ENGINE_OUTPUT(scaleOrientation,SoMFRotation,setNum(num));

  int i;
  SbVec3f translationVal,scaleFactorVal;
  SbRotation rotationVal,scaleOrientationVal;
  for (i = 0; i < num; i++) {
    SbVec3f c = (i < center.getNum()) ? center[i] : SbVec3f(0.0f, 0.0f, 0.0f);
    this->matrix[i].getTransform(translationVal,rotationVal,scaleFactorVal,
                                 scaleOrientationVal, c);
    SO_ENGINE_OUTPUT(translation,SoMFVec3f,set1Value(i,translationVal));
    SO_ENGINE_OUTPUT(rotation,SoMFRotation,set1Value(i,rotationVal));
    SO_ENGINE_OUTPUT(scaleFactor,SoMFVec3f,set1Value(i,scaleFactorVal));
    SO_ENGINE_OUTPUT(scaleOrientation,SoMFRotation,
                     set1Value(i,scaleOrientationVal));
  }
}

#endif // !DOXYGEN_SKIP_THIS
