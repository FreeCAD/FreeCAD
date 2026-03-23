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
  \class SoDecomposeRotation SoDecomposeRotation.h Inventor/engines/SoDecomposeRotation.h
  \brief The SoDecomposeRotation class is used to decompose a rotation into angle and axis.

  \ingroup coin_engines
*/

#include <Inventor/engines/SoDecomposeRotation.h>
#include <Inventor/lists/SoEngineOutputList.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoMFFloat.h>

#include "engines/SoSubEngineP.h"

SO_ENGINE_SOURCE(SoDecomposeRotation);

/*!
  \var SoMFRotation SoDecomposeRotation::rotation
  Input rotations to decompose into axis + angle values.
*/
/*!
  \var SoEngineOutput SoDecomposeRotation::axis
  (SoMFVec3f) The axis settings of the input rotations.
*/
/*!
  \var SoEngineOutput SoDecomposeRotation::angle
  (SoMFFloat) The angle values of the input rotations.
*/

#ifndef DOXYGEN_SKIP_THIS // No need to document these.

// Default constructor.
SoDecomposeRotation::SoDecomposeRotation()
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoDecomposeRotation);
  
  SO_ENGINE_ADD_INPUT(rotation, (0.0f, 0.0f, 0.0f, 1.0f));
  
  SO_ENGINE_ADD_OUTPUT(axis, SoMFVec3f);
  SO_ENGINE_ADD_OUTPUT(angle, SoMFFloat);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoDecomposeRotation::initClass()
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoDecomposeRotation);
}

//
// private members
//
SoDecomposeRotation::~SoDecomposeRotation()
{
}

// Documented in superclass.
void
SoDecomposeRotation::evaluate()
{
  int num = this->rotation.getNum();

  SO_ENGINE_OUTPUT(axis,SoMFVec3f,setNum(num));
  SO_ENGINE_OUTPUT(angle,SoMFFloat,setNum(num));

  int i;
  float angleVal;
  SbVec3f axisVal;
  for (i = 0; i < num; i++) {
    this->rotation[i].getValue(axisVal,angleVal);
    SO_ENGINE_OUTPUT(axis,SoMFVec3f,set1Value(i,axisVal));
    SO_ENGINE_OUTPUT(angle,SoMFFloat,set1Value(i,angleVal));
  }
}

#endif // !DOXYGEN_SKIP_THIS
