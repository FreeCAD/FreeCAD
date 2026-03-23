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
  \class SoVRMLOrientationInterpolator SoVRMLOrientationInterpolator.h Inventor/VRMLnodes/SoVRMLOrientationInterpolator.h
  \brief The SoVRMLOrientationInterpolator class is used to interpolate orientations.

  \ingroup coin_VRMLnodes
  
  \WEB3DCOPYRIGHT

  \verbatim
  OrientationInterpolator {
    eventIn      SFFloat    set_fraction      # (-,)
    exposedField MFFloat    key           []  # (-,)
    exposedField MFRotation keyValue      []  # [-1,1],(-,)
    eventOut     SFRotation value_changed
  }
  \endverbatim
  
  The OrientationInterpolator node interpolates among a list of
  rotation values specified in the keyValue field. These rotations are
  absolute in object space and therefore are not cumulative. The
  keyValue field shall contain exactly as many rotations as there are
  keyframes in the key field.  An orientation represents the final
  position of an object after a rotation has been applied. An
  OrientationInterpolator interpolates between two orientations by
  computing the shortest path on the unit sphere between the two
  orientations.  The interpolation is linear in arc length along this
  path. The results are undefined if the two orientations are
  diagonally opposite.  If two consecutive keyValue values exist such
  that the arc length between them is greater than , the interpolation
  will take place on the arc complement. For example, the
  interpolation between the orientations (0, 1, 0, 0) and (0, 1, 0,
  5.0) is equivalent to the rotation between the orientations (0, 1,
  0, 2) and (0, 1, 0, 5.0).  A more detailed discussion of
  interpolators is contained in 4.6.8, Interpolator nodes
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.8>).

*/

/*!
  \var SoMFRotation SoVRMLOrientationInterpolator::keyValue
  The keyValue vector.
*/

/*!
  \var SoEngineOutput SoVRMLOrientationInterpolator::value_changed
  The eventOut which is sent every time the interpolator has calculated a new value.
*/

#include <Inventor/VRMLnodes/SoVRMLOrientationInterpolator.h>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>

#include "engines/SoSubNodeEngineP.h"

SO_NODEENGINE_SOURCE(SoVRMLOrientationInterpolator);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLOrientationInterpolator::initClass(void)
{
  SO_NODEENGINE_INTERNAL_INIT_CLASS(SoVRMLOrientationInterpolator);
}

/*!
  Constructor.
*/
SoVRMLOrientationInterpolator::SoVRMLOrientationInterpolator(void)
{
  SO_NODEENGINE_INTERNAL_CONSTRUCTOR(SoVRMLOrientationInterpolator);

  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(keyValue);
  SO_NODEENGINE_ADD_OUTPUT(value_changed, SoSFRotation);
}

/*!
  Destructor.
*/
SoVRMLOrientationInterpolator::~SoVRMLOrientationInterpolator(void)
{
}

// Doc in parent
void
SoVRMLOrientationInterpolator::evaluate(void)
{
  float interp;
  const int idx = this->getKeyValueIndex(interp, this->keyValue.getNum());
  if (idx < 0) return;

  const SbRotation * v = this->keyValue.getValues(0);
  SbRotation v0 = v[idx];
  if (interp > 0.0f) {
    SbRotation v1 = v[idx+1];
    v0 = SbRotation::slerp(v0, v1, interp);
  }
  SO_ENGINE_OUTPUT(value_changed, SoSFRotation, setValue(v0));
}

#endif // HAVE_VRML97
