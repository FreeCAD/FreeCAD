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
  \class SoDecomposeVec3f SoDecomposeVec3f.h Inventor/engines/SoDecomposeVec3f.h
  \brief The SoDecomposeVec3f class is used to decompose 3D vectors into three floats.

  \ingroup coin_engines
*/

#include <Inventor/engines/SoDecomposeVec3f.h>
#include <Inventor/lists/SoEngineOutputList.h>

#include "engines/SoSubEngineP.h"

SO_ENGINE_SOURCE(SoDecomposeVec3f);

/*!
  \var SoMFVec3f SoDecomposeVec3f::vector
  Set of input vectors to be decomposed into their coordinate
  elements.
*/
/*!
  \var SoEngineOutput SoDecomposeVec3f::x
  (SoMFFloat) First coordinates of the input vectors.
*/
/*!
  \var SoEngineOutput SoDecomposeVec3f::y
  (SoMFFloat) Second coordinates of the input vectors.
*/
/*!
  \var SoEngineOutput SoDecomposeVec3f::z
  (SoMFFloat) Third coordinates of the input vectors.
*/


#ifndef DOXYGEN_SKIP_THIS // No need to document these.

SoDecomposeVec3f::SoDecomposeVec3f()
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoDecomposeVec3f);

  SO_ENGINE_ADD_INPUT(vector,(0,0,0));

  SO_ENGINE_ADD_OUTPUT(x,SoMFFloat);
  SO_ENGINE_ADD_OUTPUT(y,SoMFFloat);
  SO_ENGINE_ADD_OUTPUT(z,SoMFFloat);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoDecomposeVec3f::initClass()
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoDecomposeVec3f);
}

//
// private members
//
SoDecomposeVec3f::~SoDecomposeVec3f()
{
}

// Documented in superclass.
void
SoDecomposeVec3f::evaluate()
{
  int num = this->vector.getNum();

  SO_ENGINE_OUTPUT(x,SoMFFloat,setNum(num));
  SO_ENGINE_OUTPUT(y,SoMFFloat,setNum(num));
  SO_ENGINE_OUTPUT(z,SoMFFloat,setNum(num));

  int i;
  for (i = 0; i < num; i++) {
    SO_ENGINE_OUTPUT(x,SoMFFloat,set1Value(i,vector[i][0]));
    SO_ENGINE_OUTPUT(y,SoMFFloat,set1Value(i,vector[i][1]));
    SO_ENGINE_OUTPUT(z,SoMFFloat,set1Value(i,vector[i][2]));
  }
}

#endif // !DOXYGEN_SKIP_THIS
