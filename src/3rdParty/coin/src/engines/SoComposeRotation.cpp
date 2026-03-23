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
  \class SoComposeRotation SoComposeRotation.h Inventor/engines/SoComposeRotation.h
  \brief The SoComposeRotation class is used to compose rotations from angle and axis.

  \ingroup coin_engines

  Simple usage example:

  \code
  #Inventor V2.1 ascii
  
  Separator {
     Transform {
        rotation =
        ComposeRotation { axis 0 1 0  angle =
           ElapsedTime { }.timeOut
        }.rotation
     }
     Cube { }
  }
  \endcode
*/

#include <Inventor/engines/SoComposeRotation.h>
#include <Inventor/lists/SoEngineOutputList.h>
#include <Inventor/fields/SoMFRotation.h>

#include "engines/SoSubEngineP.h"

// *************************************************************************

/*!
  \var SoMFVec3f SoComposeRotation::axis
  Set of axis vectors for the output rotations. Default value is (0.0f, 0.0f, 1.0f).
*/
/*!
  \var SoMFFloat SoComposeRotation::angle
  Set of scalar rotation values for the output rotations. Default value is 0.0.
*/
/*!
  \var SoEngineOutput SoComposeRotation::rotation

  (SoMFRotation) Rotations generated from the angle and axis input
  fields.
*/

// *************************************************************************

SO_ENGINE_SOURCE(SoComposeRotation);

// *************************************************************************

SoComposeRotation::SoComposeRotation()
{
  SO_ENGINE_INTERNAL_CONSTRUCTOR(SoComposeRotation);

  SO_ENGINE_ADD_INPUT(axis,(0.0f,0.0f,1.0f));
  SO_ENGINE_ADD_INPUT(angle,(0.0f));

  SO_ENGINE_ADD_OUTPUT(rotation,SoMFRotation);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoComposeRotation::initClass()
{
  SO_ENGINE_INTERNAL_INIT_CLASS(SoComposeRotation);
}

//
// private members
//
SoComposeRotation::~SoComposeRotation()
{
}

// *************************************************************************

// Documented in superclass.
void
SoComposeRotation::evaluate()
{
  int numAxis=axis.getNum();
  int numAngle=angle.getNum();

  int numOut=numAxis>numAngle?numAxis:numAngle;

  SO_ENGINE_OUTPUT(rotation,SoMFRotation,setNum(numOut));

  int i;

  float angleVal;
  for (i=0;i<numOut;i++) {
    const SbVec3f axisVal=i<numAxis?axis[i]:axis[numAxis-1];
    angleVal=i<numAngle?angle[i]:angle[numAngle-1];

    SO_ENGINE_OUTPUT(rotation,SoMFRotation,set1Value(i,axisVal,angleVal));
  }
}

// *************************************************************************
