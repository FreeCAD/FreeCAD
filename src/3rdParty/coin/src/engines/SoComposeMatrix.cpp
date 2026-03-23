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
  \class SoComposeMatrix SoComposeMatrix.h Inventor/engines/SoComposeMatrix.h
  \brief The SoComposeMatrix class is used to compose a matrix from miscellaneous transformations.

  \ingroup coin_engines
*/

#include <Inventor/engines/SoComposeMatrix.h>
#include <Inventor/lists/SoEngineOutputList.h>
#include <Inventor/fields/SoMFMatrix.h>

#include "engines/SoSubEngineP.h"

SO_ENGINE_SOURCE(SoComposeMatrix);

/*!
  \var SoMFVec3f SoComposeMatrix::translation
  Input field with set of translation vectors for the output matrices.
*/
/*!
  \var SoMFRotation SoComposeMatrix::rotation
  Input field with set of rotations for the output matrices.
*/
/*!
  \var SoMFVec3f SoComposeMatrix::scaleFactor
  Input field with set of scale vectors for the output matrices.
*/
/*!
  \var SoMFRotation SoComposeMatrix::scaleOrientation
  Input field with set of scale orientations for the output matrices.
*/
/*!
  \var SoMFVec3f SoComposeMatrix::center
  Input field with set of center positions for the output matrices.
*/
/*!
  \var SoEngineOutput SoComposeMatrix::matrix

  (SoMFMatrix) A set of matrices calculated from the input fields.
  The matrices is calculated by the SbMatrix::setTransform() function.
*/

#ifndef DOXYGEN_SKIP_THIS // No need to document these.

/*!
  Constructor.
*/
SoComposeMatrix::SoComposeMatrix()
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoComposeMatrix);

  SO_ENGINE_ADD_INPUT(translation, (0.0f, 0.0f, 0.0f));
  SO_ENGINE_ADD_INPUT(rotation, (0.0f, 0.0f, 0.0f, 1.0f));
  SO_ENGINE_ADD_INPUT(scaleFactor, (1.0f, 1.0f, 1.0f));
  SO_ENGINE_ADD_INPUT(scaleOrientation, (0.0f, 0.0f, 0.0f, 1.0f));
  SO_ENGINE_ADD_INPUT(center, (0.0f, 0.0f, 0.0f));

  SO_ENGINE_ADD_OUTPUT(matrix,SoMFMatrix);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoComposeMatrix::initClass()
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoComposeMatrix);
}

//
// private members
//
/*!
  Destructor.
*/
SoComposeMatrix::~SoComposeMatrix()
{
}

// Documented in superclass.
void
SoComposeMatrix::evaluate()
{
  int numTranslation = this->translation.getNum();
  int numRotation = this->rotation.getNum();
  int numScaleFactor = this->scaleFactor.getNum();
  int numScaleOrientation = this->scaleOrientation.getNum();
  int numCenter = this->center.getNum();

  int numOut = numTranslation > numRotation? numTranslation:numRotation;
  int numOut2 =
    numScaleFactor>numScaleOrientation?numScaleFactor:numScaleOrientation;
  numOut2 = numOut2>numCenter?numOut2:numCenter;
  numOut = numOut>numOut2?numOut:numOut2;

  SO_ENGINE_OUTPUT(matrix,SoMFMatrix,setNum(numOut));

  int i;

  for (i=0;i<numOut;i++) {
    const SbVec3f translationVal=
      i<numTranslation?this->translation[i]:this->translation[numTranslation-1];
    const SbVec3f scaleFactorVal=
      i<numScaleFactor?this->scaleFactor[i]:this->scaleFactor[numScaleFactor-1];
    const SbVec3f centerVal=i<numCenter?this->center[i]:this->center[numCenter-1];
    const SbRotation rotationVal=
      i<numRotation?this->rotation[i]:this->rotation[numRotation-1];
    const SbRotation scaleOrientationVal=
      i<numScaleOrientation?
      this->scaleOrientation[i]:this->scaleOrientation[numScaleOrientation-1];

    SbMatrix mat;
    mat.setTransform(translationVal,rotationVal,scaleFactorVal,
                     scaleOrientationVal,centerVal);
    SO_ENGINE_OUTPUT(matrix,SoMFMatrix,set1Value(i,mat));
  }
}

#endif // !DOXYGEN_SKIP_THIS
