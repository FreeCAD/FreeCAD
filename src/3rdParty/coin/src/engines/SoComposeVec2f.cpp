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
  \class SoComposeVec2f SoComposeVec2f.h Inventor/engines/SoComposeVec2f.h
  \brief The SoComposeVec2f class is used to compose 2D vectors from two floats.

  \ingroup coin_engines
*/

#include <Inventor/engines/SoComposeVec2f.h>
#include <Inventor/lists/SoEngineOutputList.h>

#include "engines/SoSubEngineP.h"

SO_ENGINE_SOURCE(SoComposeVec2f);

/*!
  \var SoMFFloat SoComposeVec2f::x
  First coordinates of the output vectors.
*/
/*!
  \var SoMFFloat SoComposeVec2f::y
  Second coordinates of the output vectors.
*/
/*!
  \var SoEngineOutput SoComposeVec2f::vector
  (SoMFVec2f) 2D vectors.
*/


#ifndef DOXYGEN_SKIP_THIS // No need to document these.

SoComposeVec2f::SoComposeVec2f(void)
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoComposeVec2f);

  SO_ENGINE_ADD_INPUT(x,(0.0f));
  SO_ENGINE_ADD_INPUT(y,(0.0f));

  SO_ENGINE_ADD_OUTPUT(vector,SoMFVec2f);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoComposeVec2f::initClass(void)
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoComposeVec2f);
}

//
// private members
//
SoComposeVec2f::~SoComposeVec2f()
{
}

// Documented in superclass.
void
SoComposeVec2f::evaluate(void)
{
  int numX=x.getNum();
  int numY=y.getNum();

  int numOut=numX>numY?numX:numY;

  SO_ENGINE_OUTPUT(vector,SoMFVec2f,setNum(numOut));

  int i;
  float xVal,yVal;
  for (i=0;i<numOut;i++) {
    xVal=i<numX?x[i]:x[numX-1];
    yVal=i<numY?y[i]:y[numY-1];

    SO_ENGINE_OUTPUT(vector,SoMFVec2f,set1Value(i,xVal,yVal));
  }
}

#endif // !DOXYGEN_SKIP_THIS
