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
  \class SoComposeRotationFromTo SoComposeRotationFromTo.h Inventor/engines/SoComposeRotationFromTo.h
  \brief The SoComposeRotationFromTo class is used to compose rotations based on from and to vectors.

  \ingroup coin_engines
*/

#include <Inventor/engines/SoComposeRotationFromTo.h>
#include <Inventor/lists/SoEngineOutputList.h>
#include <Inventor/fields/SoMFRotation.h>

#include "engines/SoSubEngineP.h"

SO_ENGINE_SOURCE(SoComposeRotationFromTo);

/*!
  \var SoMFVec3f SoComposeRotationFromTo::from
  Set of input vectors to rotate \e from.
*/
/*!
  \var SoMFVec3f SoComposeRotationFromTo::to
  Set of input vectors to rotate \e to.
*/
/*!
  \var SoEngineOutput SoComposeRotationFromTo::rotation

  (SoMFRotation) Set of output rotations, where each element is the
  rotation you need to apply to the \e from vector (of the same index)
  to make it point in the \e to direction.
*/

#ifndef DOXYGEN_SKIP_THIS // No need to document these.

// Default constructor.
SoComposeRotationFromTo::SoComposeRotationFromTo(void)
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoComposeRotationFromTo);

  SO_ENGINE_ADD_INPUT(from,(0.0f,0.0f,1.0f));
  SO_ENGINE_ADD_INPUT(to,(0.0f,0.0f,1.0f));

  SO_ENGINE_ADD_OUTPUT(rotation,SoMFRotation);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoComposeRotationFromTo::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoComposeRotationFromTo);
}

// private destructor
SoComposeRotationFromTo::~SoComposeRotationFromTo()
{
}

// doc in parent
void
SoComposeRotationFromTo::evaluate(void)
{
  int numFrom = from.getNum();
  int numTo = to.getNum();
  int numOut = SbMax(numFrom, numTo);

  SO_ENGINE_OUTPUT(rotation, SoMFRotation, setNum(numOut));

  for (int i = 0; i < numOut; i++) {
    const SbVec3f fromVec = this->from[SbMin(i, numFrom-1)];
    const SbVec3f toVec = this->to[SbMin(i, numTo-1)];
    SO_ENGINE_OUTPUT(rotation, SoMFRotation, set1Value(i, SbRotation(fromVec, toVec)));
  }
}

#endif // !DOXYGEN_SKIP_THIS
