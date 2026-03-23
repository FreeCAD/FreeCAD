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
  \class SoTransformVec3f SoTransformVec3f.h Inventor/engines/SoTransformVec3f.h
  \brief The SoTransformVec3f class transforms 3D vectors by a matrix.

  \ingroup coin_engines
*/

#include <Inventor/engines/SoTransformVec3f.h>
#include <Inventor/lists/SoEngineOutputList.h>

#include "engines/SoSubEngineP.h"

/*!
  \var SoMFVec3f SoTransformVec3f::vector
  Set of 3D vector inputs to transform.
*/
/*!
  \var SoMFMatrix SoTransformVec3f::matrix
  Set of transformation matrices to use on the vectors.
*/
/*!
  \var SoEngineOutput SoTransformVec3f::point
  (SoMFVec3f) Transformed points.
*/
/*!
  \var SoEngineOutput SoTransformVec3f::direction
  (SoMFVec3f) Transformed vector directions.
*/
/*!
  \var SoEngineOutput SoTransformVec3f::normalDirection
  (SoMFVec3f) Normalized transformed vector directions.
*/

SO_ENGINE_SOURCE(SoTransformVec3f);

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTransformVec3f::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoTransformVec3f);
}

/*!
  Default constructor.
*/
SoTransformVec3f::SoTransformVec3f(void)
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoTransformVec3f);

  SO_ENGINE_ADD_INPUT(vector, (0.0f, 0.0f, 0.0f));
  SO_ENGINE_ADD_INPUT(matrix, (SbMatrix::identity()));

  SO_ENGINE_ADD_OUTPUT(point, SoMFVec3f);
  SO_ENGINE_ADD_OUTPUT(direction, SoMFVec3f);
  SO_ENGINE_ADD_OUTPUT(normalDirection, SoMFVec3f);
}

/*!
  Destructor is protected because explicit destruction of engines is
  not allowed.
*/
SoTransformVec3f::~SoTransformVec3f()
{
}

// doc in parent
void
SoTransformVec3f::evaluate(void)
{
  int numvec = this->vector.getNum();
  int nummatrices = this->matrix.getNum();

  int numoutputs = SbMax(numvec, nummatrices);

  SO_ENGINE_OUTPUT(point, SoMFVec3f, setNum(numoutputs));
  SO_ENGINE_OUTPUT(direction, SoMFVec3f, setNum(numoutputs));
  SO_ENGINE_OUTPUT(normalDirection, SoMFVec3f, setNum(numoutputs));

  SbVec3f pt, dir, ndir;

  for (int i = 0; i < numoutputs; i++) {
    const SbVec3f & v = this->vector[SbMin(i, numvec-1)];
    const SbMatrix & m = this->matrix[SbMin(i, nummatrices-1)];
    m.multVecMatrix(v, pt);
    m.multDirMatrix(v, dir);
    ndir = dir;
    (void) ndir.normalize(); // null vector is ok
    SO_ENGINE_OUTPUT(point, SoMFVec3f, set1Value(i, pt));
    SO_ENGINE_OUTPUT(direction, SoMFVec3f, set1Value(i, dir));
    SO_ENGINE_OUTPUT(normalDirection, SoMFVec3f, set1Value(i, ndir));
  }
}
