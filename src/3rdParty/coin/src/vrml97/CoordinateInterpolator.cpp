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
  \class SoVRMLCoordinateInterpolator SoVRMLCoordinateInterpolator.h Inventor/VRMLnodes/SoVRMLCoorinateInterpolator.h
  \brief The SoVRMLCoordinateInterpolator class is used to interpolate 3D coordinates.

  \ingroup coin_VRMLnodes
  
  \WEB3DCOPYRIGHT

  \verbatim
  CoordinateInterpolator {
    eventIn      SFFloat set_fraction        # (-inf, inf)
    exposedField MFFloat key           []    # (-inf, inf)
    exposedField MFVec3f keyValue      []    # (-inf, inf)
    eventOut     MFVec3f value_changed
  }
  \endverbatim

  This node linearly interpolates among a list of MFVec3f values. The
  number of coordinates in the keyValue field shall be an integer
  multiple of the number of keyframes in the key field. That integer
  multiple defines how many coordinates will be contained in the
  value_changed events.  4.6.8, Interpolator nodes
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.8>),
  contains a more detailed discussion of interpolators.

*/
#include <Inventor/VRMLnodes/SoVRMLCoordinateInterpolator.h>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/lists/SbList.h>

#include "engines/SoSubNodeEngineP.h"

#ifndef DOXYGEN_SKIP_THIS

class SoVRMLCoordinateInterpolatorP {
public:
  SbList <SbVec3f> tmplist;
};

#endif // DOXYGEN_SKIP_THIS


SO_NODEENGINE_SOURCE(SoVRMLCoordinateInterpolator);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLCoordinateInterpolator::initClass(void) // static
{
  SO_NODEENGINE_INTERNAL_INIT_CLASS(SoVRMLCoordinateInterpolator);
}

#define PRIVATE(obj) ((obj)->pimpl)

SoVRMLCoordinateInterpolator::SoVRMLCoordinateInterpolator(void)
{
  PRIVATE(this) = new SoVRMLCoordinateInterpolatorP;

  SO_NODEENGINE_INTERNAL_CONSTRUCTOR(SoVRMLCoordinateInterpolator);

  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(keyValue);
  SO_NODEENGINE_ADD_OUTPUT(value_changed, SoMFVec3f);
}

SoVRMLCoordinateInterpolator::~SoVRMLCoordinateInterpolator()
{
  delete PRIVATE(this);
}

void
SoVRMLCoordinateInterpolator::evaluate(void)
{
  if (!this->value_changed.isEnabled()) return;

  float interp;
  int i, idx = this->getKeyValueIndex(interp, this->keyValue.getNum());
  if (idx < 0) return;

  PRIVATE(this)->tmplist.truncate(0);
  const int numkeys = this->key.getNum();
  const int numcoords = this->keyValue.getNum() / numkeys;

  const SbVec3f * c0 = this->keyValue.getValues(idx*numcoords);
  const SbVec3f * c1 = c0;
  if (interp > 0.0f) c1 = this->keyValue.getValues((idx+1)*numcoords);

  for (i = 0; i < numcoords; i++) {
    PRIVATE(this)->tmplist.append(c0[i] + (c1[i]-c0[i]) * interp);
  }

  const SbVec3f * coords = PRIVATE(this)->tmplist.getArrayPtr();

  SO_ENGINE_OUTPUT(value_changed, SoMFVec3f, setNum(numcoords));
  SO_ENGINE_OUTPUT(value_changed, SoMFVec3f, setValues(0, numcoords, coords));
}

#undef PRIVATE

#endif // HAVE_VRML97
