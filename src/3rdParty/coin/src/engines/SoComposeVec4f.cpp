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
  \class SoComposeVec4f SoComposeVec4f.h Inventor/engines/SoComposeVec4f.h
  \brief The SoComposeVec4f class is used to compose 4D vectors from four floats.

  \ingroup coin_engines
*/

#include <Inventor/engines/SoComposeVec4f.h>
#include <Inventor/lists/SoEngineOutputList.h>

#include "engines/SoSubEngineP.h"

SO_ENGINE_SOURCE(SoComposeVec4f);

/*!
  \var SoMFFloat SoComposeVec4f::x
  First coordinates of the output vectors.
*/
/*!
  \var SoMFFloat SoComposeVec4f::y
  Second coordinates of the output vectors.
*/
/*!
  \var SoMFFloat SoComposeVec4f::z
  Third coordinates of the output vectors.
*/
/*!
  \var SoMFFloat SoComposeVec4f::w
  Fourth coordinates of the output vectors.
*/
/*!
  \var SoEngineOutput SoComposeVec4f::vector
  (SoMFVec4f) 4D vectors.
*/

#ifndef DOXYGEN_SKIP_THIS // No need to document these.

SoComposeVec4f::SoComposeVec4f()
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoComposeVec4f);

  SO_ENGINE_ADD_INPUT(x,(0.0f));
  SO_ENGINE_ADD_INPUT(y,(0.0f));
  SO_ENGINE_ADD_INPUT(z,(0.0f));
  SO_ENGINE_ADD_INPUT(w,(0.0f));

  SO_ENGINE_ADD_OUTPUT(vector,SoMFVec4f);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoComposeVec4f::initClass()
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoComposeVec4f);
}

//
// private members
//
SoComposeVec4f::~SoComposeVec4f()
{
}

// Documented in superclass.
void
SoComposeVec4f::evaluate()
{
  int numX=x.getNum();
  int numY=y.getNum();
  int numZ=z.getNum();
  int numW=w.getNum();

  int numOut=numX>numY?numX:numY;
  numOut=numZ>numOut?numZ:numOut;
  numOut=numW>numOut?numW:numOut;

  SO_ENGINE_OUTPUT(vector,SoMFVec4f,setNum(numOut));

  int i;
  float xVal,yVal,zVal,wVal;
  for (i=0;i<numOut;i++) {
    xVal=i<numX?x[i]:x[numX-1];
    yVal=i<numY?y[i]:y[numY-1];
    zVal=i<numZ?z[i]:z[numZ-1];
    wVal=i<numW?w[i]:w[numW-1];

    SO_ENGINE_OUTPUT(vector,SoMFVec4f,set1Value(i,xVal,yVal,zVal,wVal));
  }
}

#endif // !DOXYGEN_SKIP_THIS
