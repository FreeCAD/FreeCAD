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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97

/*!
  \class SoVRMLPositionInterpolator SoVRMLPositionInterpolator.h Inventor/VRMLnodes/SoVRMLPositionInterpolator.h
  \brief The SoVRMLPositionInterpolator class is used to interpolate 3D points.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  PositionInterpolator {
    eventIn      SFFloat set_fraction        # (-,)
    exposedField MFFloat key           []    # (-,)
    exposedField MFVec3f keyValue      []    # (-,)
    eventOut     SFVec3f value_changed
  }
  \endverbatim
  
  The PositionInterpolator node linearly interpolates among a list of
  3D vectors. The keyValue field shall contain exactly as many values
  as in the key field.  4.6.8, Interpolator nodes
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.8>),
  contains a more detailed discussion of interpolators.

*/

/*!
  \var SoMFVec3f SoVRMLPositionInterpolator::keyValue
  The keyValue vector.
*/

/*!
  \var SoEngineOutput SoVRMLPositionInterpolator::value_changed
  The eventOut which is sent every time the interpolator has calculated a new value.
*/

#include <Inventor/VRMLnodes/SoVRMLPositionInterpolator.h>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>

#include "engines/SoSubNodeEngineP.h"

SO_NODEENGINE_SOURCE(SoVRMLPositionInterpolator);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLPositionInterpolator::initClass(void) // static
{
  SO_NODEENGINE_INTERNAL_INIT_CLASS(SoVRMLPositionInterpolator);
}

/*!
  Constructor.
*/
SoVRMLPositionInterpolator::SoVRMLPositionInterpolator(void)
{
  SO_NODEENGINE_INTERNAL_CONSTRUCTOR(SoVRMLPositionInterpolator);

  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(keyValue);
  SO_NODEENGINE_ADD_OUTPUT(value_changed, SoSFVec3f);
}

/*!
  Destructor.
*/
SoVRMLPositionInterpolator::~SoVRMLPositionInterpolator()
{
}

// Doc in parent
void
SoVRMLPositionInterpolator::evaluate(void)
{
  float interp;
  int idx = this->getKeyValueIndex(interp, this->keyValue.getNum());
  if (idx < 0) return;
  
  const SbVec3f * v = this->keyValue.getValues(0);

  SbVec3f v0 = v[idx];
  if (interp > 0.0f) {
    SbVec3f v1 = v[idx+1];
    v0 = v0 + (v1-v0)*interp;
  }
  SO_ENGINE_OUTPUT(value_changed, SoSFVec3f, setValue(v0));
}

#endif // HAVE_VRML97
