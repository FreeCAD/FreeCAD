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
  \class SbCylinderProjector SbCylinderProjector.h Inventor/projectors/SbCylinderProjector.h
  \brief The SbCylinderProjector class is the abstract base class for mapping to cylindrical surfaces.

  \ingroup coin_projectors

  The cylinder projectors map 2D points to various surface types based
  on cylindrical shapes.

  \sa SbSphereProjector
 */

#include <Inventor/projectors/SbCylinderProjector.h>
#include <Inventor/SbRotation.h>

/*!
  \var SbCylinderProjector::intersectFront

  Flag which says whether or not we should map to the outside or
  inside of the cylinder surface.
*/
/*!
  \var SbCylinderProjector::cylinder

  Specification of the projection cylinder.
*/
/*!
  \var SbCylinderProjector::orientToEye

  Which direction the cylindrical surface is oriented.
*/
/*!
  \var SbCylinderProjector::needSetup

  Set to \c TRUE whenever the projection surface needs to be
  recalculated according to the setting of the
  SbCylinderProjector::orientToEye flag.
*/
/*!
  \var SbCylinderProjector::lastPoint

  Stores the previously projected 3D point.
*/
/*!
  \fn virtual SbRotation SbCylinderProjector::getRotation(const SbVec3f & point1, const SbVec3f & point2)

  Returns rotation on the projection surface which reorients \a
  point1 to \a point2.
*/



/*!
  Default constructor sets up a cylinder along the Y-axis with height
  1.
*/
SbCylinderProjector::SbCylinderProjector(const SbBool orienttoeye)
  : intersectFront(TRUE),
    cylinder(SbLine(SbVec3f(0.0f, 0.0f, 0.0f), SbVec3f(0.0f, 1.0f, 0.0f)), 1.0f),
    orientToEye(orienttoeye),
    needSetup(TRUE)
{
}

/*!
  Constructor taking an explicit \a cylinder projection definition.
*/
SbCylinderProjector::SbCylinderProjector(const SbCylinder & cylinder,
                                         const SbBool orienttoeye)
  : intersectFront(TRUE),
    cylinder(cylinder),
    orientToEye(orienttoeye),
    needSetup(TRUE)
{
}

/*!
  Project the 2D point to a 3D coordinate on the cylindrical surface,
  and find the rotation from the last projection to this one.

  \sa project(), getRotation()
*/
SbVec3f
SbCylinderProjector::projectAndGetRotation(const SbVec2f & point,
                                           SbRotation & rot)
{
  SbVec3f lastpt = this->lastPoint;
  SbVec3f newpt = this->project(point);
  this->lastPoint = newpt;
  rot = this->getRotation(lastpt, newpt);
  return newpt;
}

/*!
  Set \a cylinder to project onto.
*/
void
SbCylinderProjector::setCylinder(const SbCylinder & cylinderref)
{
  this->cylinder = cylinderref;
  this->needSetup = TRUE;
}

/*!
  Returns projection cylinder.
*/
const SbCylinder &
SbCylinderProjector::getCylinder(void) const
{
  return this->cylinder;
}

/*!
  Sets whether or not the projection surface should be oriented
  towards the eye of the viewer.
*/
void
SbCylinderProjector::setOrientToEye(const SbBool orienttoeye)
{
  if (this->orientToEye != orienttoeye) {
    this->orientToEye = orienttoeye;
    this->needSetup = TRUE;
  }
}

/*!
  Returns the state of the cylinder orientation flag.
*/
SbBool
SbCylinderProjector::isOrientToEye(void) const
{
  return this->orientToEye;
}

/*!
  Set whether to intersect with the outside of the cylinder (\a
  isfront equal to \c TRUE), or the inside.
*/
void
SbCylinderProjector::setFront(const SbBool infront)
{
  if (this->intersectFront != infront) {
    this->intersectFront = infront;
    this->needSetup = TRUE;
  }
}

/*!
  Returns value of the flag which decides whether to intersect with
  the outside or inside of the cylinder.
*/
SbBool
SbCylinderProjector::isFront(void) const
{
  return this->intersectFront;
}

/*!
  Check if \a point is on the front side or the back side of the
  cylinder.
*/
SbBool
SbCylinderProjector::isPointInFront(const SbVec3f & point) const
{
  const SbViewVolume & vv = this->getViewVolume();
  SbVec3f camdir;
  if (vv.getProjectionType() == SbViewVolume::PERSPECTIVE) {
    SbVec3f campos;
    this->worldToWorking.multVecMatrix(vv.getProjectionPoint(), campos);
    camdir = campos - this->cylinder.getAxis().getClosestPoint(point);

    // projection point for reverse perspective view volume lies behind the scene
    if (vv.getNearDist() < 0.0f) camdir *= -1.0f;
  } else {
    this->worldToWorking.multDirMatrix( vv.zVector(), camdir);
  }
  SbVec3f ptdir = point - this->cylinder.getAxis().getClosestPoint(point);
  return camdir.dot(ptdir) >= 0.0f;
}

/*!
  Intersect \a line with the SbCylinderProjector::cylinder and place
  the intersection point (if any) in \a result. Considers setFront()
  settings.

  Returns \c TRUE if \a line actually hits the cylinder, \c FALSE if
  it doesn't intersect with it.
*/
SbBool
SbCylinderProjector::intersectCylinderFront(const SbLine & line,
                                            SbVec3f & result)
{
  SbVec3f i0, i1;
  SbBool isect = this->cylinder.intersect(line, i0, i1);
  if (isect) {
    if (this->isFront()) result = i0;
    else result = i1;
  }
  return isect;
}

// Documented in superclass.
void
SbCylinderProjector::setWorkingSpace(const SbMatrix & space)
{
  this->needSetup = TRUE;
  inherited::setWorkingSpace(space);
}
