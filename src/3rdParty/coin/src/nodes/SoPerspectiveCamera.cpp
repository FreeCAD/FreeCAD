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
  \class SoPerspectiveCamera SoPerspectiveCamera.h Inventor/nodes/SoPerspectiveCamera.h
  \brief The SoPerspectiveCamera class defines a camera node with perspective rendering.

  \ingroup coin_nodes

  For realistic looking 3D scene, the geometry should be rendered with
  perspective calculations. Use this camera type to accomplish this.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    PerspectiveCamera {
        viewportMapping ADJUST_CAMERA
        position 0 0 1
        orientation 0 0 1  0
        nearDistance 1
        farDistance 10
        aspectRatio 1
        focalDistance 5
        heightAngle 0.78539819
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoPerspectiveCamera.h>

#include <cassert>

#include <Inventor/SbSphere.h>
#include <Inventor/errors/SoDebugError.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFFloat SoPerspectiveCamera::heightAngle

  The vertical angle of the viewport, also known as "field of view".
  Default value is 45 degrees (note: value is specified in radians).
*/

// *************************************************************************

SO_NODE_SOURCE(SoPerspectiveCamera);

/*!
  Constructor.
*/
SoPerspectiveCamera::SoPerspectiveCamera()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoPerspectiveCamera);

  SO_NODE_ADD_FIELD(heightAngle, (float(M_PI)/4.0f));  // 45 degrees.
}

/*!
  Destructor.
*/
SoPerspectiveCamera::~SoPerspectiveCamera()
{
}

// Doc in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoPerspectiveCamera::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoPerspectiveCamera, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

/*!
  Scale the SoPerspectiveCamera::heightAngle field by multiplying with
  \a scalefactor.
*/
void
SoPerspectiveCamera::scaleHeight(float scalefactor)
{
  float tmp = this->heightAngle.getValue();
  this->heightAngle.setValue(tmp * scalefactor);
}

// Doc in superclass.
SbViewVolume
SoPerspectiveCamera::getViewVolume(float useaspectratio) const
{
  float angle = this->heightAngle.getValue();
  if (useaspectratio == 0.0f) useaspectratio = this->aspectRatio.getValue();
  SbViewVolume volume;
  volume.perspective(angle, useaspectratio,
                     this->nearDistance.getValue(),
                     this->farDistance.getValue());
  volume.rotateCamera(this->orientation.getValue());
  volume.translateCamera(this->position.getValue());
  return volume;
}

// Doc in superclass.
void
SoPerspectiveCamera::viewBoundingBox(const SbBox3f & box, float aspect,
                                     float slack)
{
#if COIN_DEBUG
  // Only check for "flagged" emptiness, and don't use
  // SbBox3f::hasVolume(), as we *can* handle flat boxes.
  if (box.isEmpty()) {
    SoDebugError::postWarning("SoPerspectiveCamera::viewBoundingBox",
                              "bounding box is empty");
    return;
  }
#endif // COIN_DEBUG

  // First, we want to move the camera in such a way that it is
  // pointing straight at the center of the scene bounding box -- but
  // without modifying the rotation value (so we can't use
  // SoCamera::pointAt()).
  SbVec3f cameradirection;
  this->orientation.getValue().multVec(SbVec3f(0, 0, -1), cameradirection);
  this->position.setValue(box.getCenter() + -cameradirection);


  // Get the radius of the bounding sphere.
  SbSphere bs;
  bs.circumscribe(box);
  float radius = bs.getRadius();

  // Make sure that everything will still be inside the viewing volume
  // even if the aspect ratio "favorizes" width over height.
  float aspectradius = radius / (aspect < 1.0f ? aspect : 1.0f);

  // Move the camera to the edge of the bounding sphere, while still
  // pointing at the scene.
  SbVec3f direction = this->position.getValue() - box.getCenter();
  (void) direction.normalize(); // we know this is not a null vector

  // There's a small chance that the frustum will intersect the
  // bounding box when we calculate the movelength like this, but for
  // all normal heightAngles it will yield a much better fit than
  // the 100% safe version which also adds radius to movelength
  float movelength = aspectradius/float(tan(this->heightAngle.getValue() / 2.0));
  this->position.setValue(box.getCenter() + direction * movelength);

  // Set up the clipping planes according to the slack value (a value
  // of 1.0 will yield clipping planes that are tangent to the
  // bounding sphere of the scene).
  float distance_to_midpoint =
    (this->position.getValue() - box.getCenter()).length();
  // make sure nearDistance isn't 0.0 (or too close to 0.0)
  const float EPS = 0.001f;
  this->nearDistance = SbMax(distance_to_midpoint*EPS, distance_to_midpoint - radius * slack);
  this->farDistance = distance_to_midpoint + radius * slack;

  // The focal distance is simply the distance from the camera to the
  // scene midpoint. This field is not used in rendering, its just
  // provided to make it easier for the user to do calculations based
  // on the distance between the camera and the scene.
  this->focalDistance = distance_to_midpoint;
}
