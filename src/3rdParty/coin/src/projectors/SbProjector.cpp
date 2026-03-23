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
  \class SbProjector SbProjector.h Inventor/projectors/SbProjector.h
  \brief The SbProjector class is the abstract base projector class.

  \ingroup coin_projectors

  Projectors are used in the Coin library for mapping 2D coordinates
  (typically from the position of the mouse cursor in the rendering
  window) to 3D "world" coordinates.

  Mapping 2D coordinates to 3D coordinates is something which is done
  extensively in the dragger classes, to provide the user with a
  convenient and natural way of interacting with the 3D geometry of
  scenes.

  For a usage example, see the class documentation for
  SbSphereSheetProjector.

  The application programmer should normally not need to care about
  the projector classes, unless there are special needs in the
  application.

  \sa SoDragger
*/


#include <Inventor/projectors/SbProjector.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbPlane.h>
#include <cassert>
#include "coindefs.h"

/*!
  \fn SbProjector::~SbProjector()

  Destructor is protected, as this is an abstract class.
 */

/*!
  \fn virtual SbVec3f SbProjector::project(const SbVec2f & point)

  Project the 2D \a point from normalized viewport coordinates to a 3D
  point. The mapping will be done in accordance with the type of the
  projector.
 */
/*!
  \fn virtual SbProjector * SbProjector::copy(void) const

  Construct and return a copy of this projector. The caller is
  responsible for destructing the new instance.

  \DANGEROUS_ALLOC_RETURN
 */

/*!
  \var SbProjector::viewVol

  The viewing volume definition.
*/
/*!
  \var SbProjector::worldToWorking

  The matrix which converts from world coordinates to coordinates in
  the projector's local coordinate system.
*/
/*!
  \var SbProjector::workingToWorld

  The matrix which converts from coordinates in the projector's local
  coordinate system to world coordinates.
*/


/*!
  The constructor initializes the working space matrix to an identity
  matrix.
 */
SbProjector::SbProjector(void)
{
  this->worldToWorking.makeIdentity();
  this->workingToWorld.makeIdentity();
}

/*!
  Set the viewing volume the projections will take place in.

  \sa getViewVolume()
 */
void
SbProjector::setViewVolume(const SbViewVolume & vol)
{
  this->viewVol = vol;
}

/*!
  Return the current viewing volume used by the projections.

  \sa setViewVolume()
 */
const SbViewVolume &
SbProjector::getViewVolume(void) const
{
  return this->viewVol;
}

/*!
  Sets the matrix used for converting from the projector's coordinate
  system to the world coordinate system.
 */
void
SbProjector::setWorkingSpace(const SbMatrix & space)
{
  this->workingToWorld = space;
  this->worldToWorking = space.inverse();
}

/*!
  Returns projector-to-world matrix.

  \sa setWorkingSpace()
 */
const SbMatrix &
SbProjector::getWorkingSpace(void) const
{
  return this->workingToWorld;
}

/*!
  From the 2D \a point in normalized screen space coordinates,
  calculate the line passing through the scene.

  Typically used for tracking intersection points for the mouse
  cursor.
 */
SbLine
SbProjector::getWorkingLine(const SbVec2f & point) const
{
  SbLine l;
  this->viewVol.projectPointToLine(point, l);
  this->worldToWorking.multLineMatrix(l, l);
  return l;
}

/*!
  Finds the unit cube vanishing distance for the current projector
  view volume.  The view volume must be a perspective view
  volume.

  This method was not part of the Inventor v2.1 API, and is an
  extension specific to Coin.

  \since Coin 1.1
*/
float
SbProjector::findVanishingDistance(void) const
{
  const SbViewVolume & vv = this->viewVol;

  // FIXME: find a proper algorithm to calculate the vanishing
  // distance. Now we just use an incremental algorithm to detect when
  // the projected box is less than one pixel on a screen with height
  // = 512 pixels. pederb, 2001-10-11
  assert(vv.getProjectionType() == SbViewVolume::PERSPECTIVE);
  float depth = vv.getHeight();

  // used to break out if we get too many iterations. Will probably
  // never happen, but it's here just in case something bad happens.
  int cnt = 0;

  float unit = depth * 0.25f;

  SbBox3f unitbox(-unit, -unit, -unit, unit, unit, unit);
  SbVec3f projdir = this->viewVol.getProjectionDirection();
  SbMatrix m;
  m.setTranslate(projdir * depth);
  SbBox3f box = unitbox;
  box.transform(m);

  SbVec2f siz = vv.projectBox(box);
  while (cnt < 64 && (siz[1] > (1.0f / 512.0f))) {
    depth *= 2.0f;
    m.setTranslate(projdir * depth);
    SbBox3f box = unitbox;
    box.transform(m);
    siz = vv.projectBox(box);
    cnt++;
  }
  return depth;
}

/*!
  Verifies that \a projpt is a valid projection for the current view
  volume. For perspective view volumes, it does this by checking that
  the projection point is in front of the eye plane. For orthographic
  projections, this method always returns \e TRUE.

  This method was not part of the Inventor v2.1 API, and is an
  extension specific to Coin.

  \since Coin 1.1
*/
SbBool
SbProjector::verifyProjection(const SbVec3f & projpt) const
{
  if (this->viewVol.getProjectionType() == SbViewVolume::PERSPECTIVE) {
    SbPlane eyeplane = this->viewVol.getPlane(0.0f);
    SbVec3f wrld;
    this->workingToWorld.multVecMatrix(projpt, wrld);
    if (this->viewVol.getNearDist() > 0.0f) {
      if (eyeplane.isInHalfSpace(wrld)) return FALSE;
    } else {
      // eye plane for reverse perspective view volume lies behind the scene
      if (!eyeplane.isInHalfSpace(wrld)) return FALSE;
    }
  }
  return TRUE;
}

/*!
  Try projecting the 2D \a point from normalized viewport coordinates to a 3D
  point. The mapping will be done in accordance with the type of the
  projector.

  If the projection can't be done safely (for instance when the
  projection plane or line is parallel to the view volume projection),
  this function should return FALSE.
  
  Default implementation will call project() and always return TRUE,
  but subclasses can override this behavior to support safe
  projections.

  \since Coin 3.0
*/
SbBool 
SbProjector::tryProject(const SbVec2f & point, const float COIN_UNUSED_ARG(epsilon), SbVec3f & result)
{
  result = this->project(point);
  return TRUE;
}
