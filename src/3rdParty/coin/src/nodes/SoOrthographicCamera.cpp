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
  \class SoOrthographicCamera SoOrthographicCamera.h Inventor/nodes/SoOrthographicCamera.h
  \brief The SoOrthographicCamera class defines a camera node with orthographic rendering.

  \ingroup coin_nodes

  Orthographic rendering will not give a particularly realistic
  impression of the scene, but non-realistic rendering is for various
  reasons widely used in applications for e.g. Computer Aided Design.

  Also, a simple technique for drawing overlay / HUD style graphics
  (often appearing to be in 2D) can be implemented by setting up a
  "sub scene graph" with an SoOrthographicCamera and the geometry.  As
  a simple demonstration of this concept, load this file into e.g. the
  ExaminerViewer:

  \verbatim
  #Inventor V2.1 ascii
  
  Separator {
     PerspectiveCamera { position 0 0 5 }
     Cone { }
  
     Separator {
        OrthographicCamera {
           position -0.75 -0.75 2
        }
        BaseColor { rgb 1 1 0 }
        Sphere { radius 0.2 }
     }
  }
  \endverbatim

  You will likely encounter z-buffer issues with this technique which
  makes the overlay / HUD graphics end up interspersed with the "real"
  geometry. If so, this can be solved by e.g. inserting an SoCallback
  node in the sub-scene, where you let the callback disable the depth
  buffer with glDisable(GL_DEPTH_TEST).

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    OrthographicCamera {
        viewportMapping ADJUST_CAMERA
        position 0 0 1
        orientation 0 0 1  0
        nearDistance 1
        farDistance 10
        aspectRatio 1
        focalDistance 5
        height 2
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoOrthographicCamera.h>

#include <Inventor/SbSphere.h>
#include <Inventor/errors/SoDebugError.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFFloat SoOrthographicCamera::height
  Height of viewport in world space scale. Defaults to 2.0 units.
*/

// *************************************************************************

SO_NODE_SOURCE(SoOrthographicCamera);

/*!
  Constructor.
*/
SoOrthographicCamera::SoOrthographicCamera()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoOrthographicCamera);

  SO_NODE_ADD_FIELD(height, (2.0f));
}

/*!
  Destructor.
*/
SoOrthographicCamera::~SoOrthographicCamera()
{
}

// Doc in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoOrthographicCamera::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoOrthographicCamera, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

/*!
  Scale SoOrthographicCamera::height by multiplying with \a
  scalefactor.
*/
void
SoOrthographicCamera::scaleHeight(float scalefactor)
{
  this->height.setValue(this->height.getValue() * scalefactor);
}

// Doc in superclass.
SbViewVolume
SoOrthographicCamera::getViewVolume(float useaspectratio) const
{
  SbViewVolume volume;

  if (useaspectratio == 0.0f) useaspectratio = this->aspectRatio.getValue();
  float halfheight = this->height.getValue() * 0.5f;
  float halfwidth = halfheight * useaspectratio;
  volume.ortho(-halfwidth, halfwidth,
               -halfheight, halfheight,
               this->nearDistance.getValue(), this->farDistance.getValue());
  
  volume.rotateCamera(this->orientation.getValue());
  volume.translateCamera(this->position.getValue());
  return volume;
}

// Doc in superclass.
void
SoOrthographicCamera::viewBoundingBox(const SbBox3f & box,
                                      float aspect, float slack)
{
#if COIN_DEBUG
  if (box.isEmpty()) {
    SoDebugError::postWarning("SoOrthographicCamera::viewBoundingBox",
                              "bounding box empty");
    return;
  }
#endif // COIN_DEBUG

  // Get the radius of the bounding sphere.
  SbSphere bs;
  bs.circumscribe(box);
  float radius = bs.getRadius();

  // We want to move the camera in such a way that it is pointing
  // straight at the center of the scene bounding box -- but without
  // modifying the rotation value (so we can't use SoCamera::pointAt()),
  // and positioned at the edge of the bounding sphere.
  SbVec3f cameradirection;
  this->orientation.getValue().multVec(SbVec3f(0, 0, -1), cameradirection);
  this->position.setValue(box.getCenter() + cameradirection * -radius);

  // Set up the clipping planes tangent to the bounding sphere of the scene.
  this->nearDistance = radius * ( -slack + 1);
  this->farDistance = radius * ( slack + 1);

  // The focal distance is simply the distance from the camera to the
  // scene midpoint. This field is not used in rendering, its just
  // provided to make it easier for the user to do calculations based
  // on the distance between the camera and the scene.
  this->focalDistance = radius;

  // Make sure that everything will still be inside the viewing volume
  // even if the aspect ratio "favorizes" width over height, and take the
  // slack factor into account.
  if (aspect < 1.0f)
    this->height = 2 * radius / aspect;
  else
    this->height = 2 * radius;
}
