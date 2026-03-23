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
  \class SoReversePerspectiveCamera SoReversePerspectiveCamera.h Inventor/nodes/SoReversePerspectiveCamera.h
  \brief The SoReversePerspectiveCamera class defines a camera node with reverse perspective rendering.

  \ingroup coin_nodes

  For realistic looking 3D scene, the geometry should be rendered with
  perspective calculations. Use this camera type to accomplish this.

  Reverse perspective rendering, a special case of perspective rendering, is
  where the projection rays do not converge in the eye of the observer, but
  instead at a point behind the scene. Thus objects farther away from the
  observer appear larger than closer objects. A reverse perspective projection
  is defined by
    - setting the perspective camera position to the projection point behind the scene
    - setting near / far distance to negative values
    - rotating the camera orientation by 180 degrees around the camera Z-axis
  to compensate the image flip induced by the negative near / far values.

  The view volume of the default reverse perspective camera is defined as shown below.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    ReversePerspectiveCamera {
        viewportMapping ADJUST_CAMERA
        position 0 0 -10
        orientation 0 0 1 3.14159
        nearDistance -10
        farDistance -1
        aspectRatio 1
        focalDistance -6
        heightAngle 0.78539819
    }
  \endcode

*/

// *************************************************************************

#include <Inventor/nodes/SoReversePerspectiveCamera.h>

#include <cassert>

#include <Inventor/errors/SoDebugError.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

SO_NODE_SOURCE(SoReversePerspectiveCamera);

/*!
  Constructor.
*/
SoReversePerspectiveCamera::SoReversePerspectiveCamera()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoReversePerspectiveCamera);

  this->position.setValue (0.0f, 0.0f, -10.0f);
  this->position.setDefault (TRUE);

  this->orientation.setValue (SbVec3f(0.0f, 0.0f, 1.0f), 3.14159f);
  this->orientation.setDefault (TRUE);

  this->nearDistance.setValue (-10.0f);
  this->nearDistance.setDefault (TRUE);

  this->farDistance.setValue (-1.0f);
  this->farDistance.setDefault (TRUE);

  this->focalDistance.setValue (-6.0f);
  this->focalDistance.setDefault (TRUE);
}

/*!
  Destructor.
*/
SoReversePerspectiveCamera::~SoReversePerspectiveCamera()
{
}

// Doc in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoReversePerspectiveCamera::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoReversePerspectiveCamera, SO_FROM_COIN_4_0|SoNode::COIN_4_0);
}
