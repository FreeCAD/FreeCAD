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
  \class SoFrustumCamera SoFrustumCamera.h Inventor/nodes/SoFrustumCamera.h
  \brief The SoFrustumCamera class defines a camera with a generic frustum..

  The SoFrustumCamera class makes it possible to specify a frustum in
  the same manner as the OpenGL glFrustum() function. It has four new
  fields (left, right, top, bottom), and will use
  SoCamera::nearDistance and SoCamera::farDistance for the two last
  glFrustum() parameters.

  This camera can be useful in applications that require full control
  over the view frustum, such as in CAVE or other multipipe
  applications.

  \ingroup coin_nodes
  \since Coin 2.5
*/

/*!
  \var SoSFFloat SoFrustumCamera::left

  The left clipping plane position. Default value is -0.5.
*/

/*!
  \var SoSFFloat SoFrustumCamera::right

  The right clipping plane position. Default value is 0.5
*/

/*!
  \var SoSFFloat SoFrustumCamera::bottom

  The bottom clipping plane position. Default value is -0.5.
*/

/*!
  \var SoSFFloat SoFrustumCamera::top

  The top clipping plane position. Default value is 0.5.
*/

#include <Inventor/nodes/SoFrustumCamera.h>

#include <cmath>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SbSphere.h>

#include "nodes/SoSubNodeP.h"

SO_NODE_SOURCE(SoFrustumCamera);

/*!
  Constructor.
*/
SoFrustumCamera::SoFrustumCamera(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoFrustumCamera);

  SO_NODE_ADD_FIELD(top, (0.5f));
  SO_NODE_ADD_FIELD(bottom, (-0.5f));
  SO_NODE_ADD_FIELD(left, (-0.5f));
  SO_NODE_ADD_FIELD(right, (0.5f));
}

/*!
  Destructor.
*/
SoFrustumCamera::~SoFrustumCamera(void)
{
}

// Doc in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoFrustumCamera::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoFrustumCamera, SoNode::COIN_2_5);
}

// Doc in superclass.
void
SoFrustumCamera::scaleHeight(float scalefactor)
{
  this->top = this->top.getValue() * scalefactor;
  this->bottom = this->bottom.getValue() * scalefactor;
}

// Doc in superclass.
SbViewVolume
SoFrustumCamera::getViewVolume(float useaspectratio) const
{
  SbViewVolume vv;

  if (useaspectratio == 0.0f) { // LEAVE_ALONE viewportMapping
    vv.frustum(this->left.getValue(),
               this->right.getValue(),
               this->bottom.getValue(),
               this->top.getValue(),
               this->nearDistance.getValue(),
               this->farDistance.getValue());
  }
  else {
    // calculate left and right based on height
    float cx = (this->left.getValue() + this->right.getValue()) * 0.5f;
    float h_2 = (this->top.getValue() - this->bottom.getValue()) * 0.5f;

    vv.frustum(cx - h_2 * useaspectratio,
               cx + h_2 * useaspectratio,
               this->bottom.getValue(),
               this->top.getValue(),
               this->nearDistance.getValue(),
               this->farDistance.getValue());
  }
  vv.rotateCamera(this->orientation.getValue());
  vv.translateCamera(this->position.getValue());
  return vv;
}

// Doc in superclass.
void
SoFrustumCamera::viewBoundingBox(const SbBox3f & box, float aspect, float slack)
{

#if COIN_DEBUG
  // Only check for "flagged" emptiness, and don't use
  // SbBox3f::hasVolume(), as we *can* handle flat boxes.
  if (box.isEmpty()) {
    SoDebugError::postWarning("Frustum::viewBoundingBox",
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

  // just calculate a heightangle so that we can reuse code from
  // SoPerspectiveCamera
  float nearv = this->nearDistance.getValue();
  SbVec3f tvec = SbVec3f(0.0f, this->top.getValue(), nearv);
  SbVec3f bvec = SbVec3f(0.0f, this->bottom.getValue(), nearv);

  (void) tvec.normalize();
  (void) bvec.normalize();

  float heightangle = (float) acos((double)SbClamp(tvec.dot(bvec), 0.0f, 1.0f));

  // Move the camera to the edge of the bounding sphere, while still
  // pointing at the scene.
  SbVec3f direction = this->position.getValue() - box.getCenter();
  direction.normalize();
  float movelength =
    aspectradius + (aspectradius/float(tan(heightangle / 2.0)));
  this->position.setValue(box.getCenter() + direction * movelength);

  // Set up the far clipping plane according to the slack value (a
  // value of 1.0 will yield a far clipping plane that is tangent to
  // the bounding sphere of the scene).
  float distance_to_midpoint =
    (this->position.getValue() - box.getCenter()).length();
  this->farDistance = distance_to_midpoint + radius * slack;

  // The focal distance is simply the distance from the camera to the
  // scene midpoint. This field is not used in rendering, its just
  // provided to make it easier for the user to do calculations based
  // on the distance between the camera and the scene.
  this->focalDistance = distance_to_midpoint;
}
