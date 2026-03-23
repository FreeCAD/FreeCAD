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
  \class SbSphereSectionProjector SbSphereSectionProjector.h Inventor/projectors/SbSphereSectionProjector.h
  \brief The SbSphereSectionProjector projects 2D points to a sliced sphere.

  \ingroup coin_projectors

  The projection sphere for this class is sliced by a clipping
  plane.  Projections will be mapped to the remaining sphere part.

  \sa SbCylinderSectionProjector
 */

#include <Inventor/projectors/SbSphereSectionProjector.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*! \var SbSphereSectionProjector::tolerance
  Tolerance value, deciding how much of the half-sphere to do
  projections against.
*/
/*! \var SbSphereSectionProjector::tolDist
  Tolerance value multiplied with the sphere radius.
*/
/*! \var SbSphereSectionProjector::tolPlane
  Defines the plane cutting the sphere into a projection part.
*/
/*! \var SbSphereSectionProjector::radialFactor
  The radial factor value decides how to calculate rotations outside of
  the sphere.
  \sa setRadialFactor()
*/
/*! \var SbSphereSectionProjector::planePoint
  A point in the plane where the center of the sphere is located.
*/
/*! \var SbSphereSectionProjector::planeDir
  Direction of cutting plane.
*/
/*! \var SbSphereSectionProjector::planeDist
  Distance from plane to sphere center.
*/


/*!
  Default constructor. See SbSphereProjector::SbSphereProjector().

  The \a edgetol value should be within <0, 1], and specifies how much
  of the sphere is used as a projection surface. 1.0 means the full
  front half is used.
*/
SbSphereSectionProjector::SbSphereSectionProjector(const float edgetol,
                                                   const SbBool orienttoeye)
  : inherited(orienttoeye),
    tolerance(edgetol),
    radialFactor(0.0f)
{
  // needSetup will be TRUE, and other members will be initialized later
}

/*!
  Constructor with explicit setting of the projection sphere.
*/
SbSphereSectionProjector::SbSphereSectionProjector(const SbSphere & sph,
                                                   const float edgetol,
                                                   const SbBool orienttoeye)
  : inherited(sph, orienttoeye),
    tolerance(edgetol),
    radialFactor(0.0f)
{
  // needSetup will be TRUE, and other members will be initialized later
}

// Documented in superclass.
SbProjector *
SbSphereSectionProjector::copy(void) const
{
  return new SbSphereSectionProjector(*this);
}

// Documented in superclass.
SbVec3f
SbSphereSectionProjector::project(const SbVec2f & point)
{
  if (this->needSetup) this->setupTolerance();

  SbLine projline = this->getWorkingLine(point);
  SbVec3f projpt;

  SbBool tst = this->intersectSphereFront(projline, projpt);
  if (!tst || !this->isWithinTolerance(projpt)) {
    if (!this->tolPlane.intersect(projline, projpt)) {
#if COIN_DEBUG
      SoDebugError::postWarning("SbSphereSectionProjector::project",
                                "working line is perpendicular to plane direction.");
#endif // COIN_DEBUG
      // set to 0, 0, 0 to avoid crazy rotations. lastPoint will then
      // never change, and there will be no rotation in getRotation()
      projpt = SbVec3f(0.0f, 0.0f, 0.0f);
    }
    else {
      SbLine myLine(projpt, this->sphere.getCenter());
      if (!this->sphere.intersect(myLine, projpt)) {
        // shouldn't happen, but be robust if it does
        projpt = SbVec3f(0.0f, 0.0f, 0.0f);
      }
    }
  }
  this->lastPoint = projpt;
  return projpt;
}

// Documented in superclass.
SbRotation
SbSphereSectionProjector::getRotation(const SbVec3f & point1,
                                      const SbVec3f & point2)
{
  // FIXME: incorporate the effect of the radialFactor.  pederb,
  // 19991209
  const SbVec3f & c = this->sphere.getCenter();
  return SbRotation(point1-c, point2-c);
}

/*!
  The \a edgetol value decides how much of the surface of the sphere
  is used for projection. 1.0 means the full sphere half is used.
*/
void
SbSphereSectionProjector::setTolerance(const float edgetol)
{
  this->needSetup = TRUE;
  this->tolerance = edgetol;
}

/*!
  Returns edge tolerance for the sphere half.
*/
float
SbSphereSectionProjector::getTolerance(void) const
{
  return this->tolerance;
}

/*!
  Set up the controlling value for how the rotation calculation will
  be done "outside" the sphere.

  A radialFactor equal to 0.0 will make the rotations be done like if
  they were done by dragging the sphere, while a value of 1.0 will
  make the rotation be around the axis from the viewpoint towards the
  sphere.
*/
void
SbSphereSectionProjector::setRadialFactor(const float rad)
{
  this->radialFactor = rad;
}

/*!
  Returns value of radialFactor.

  \sa setRadialFactor()
*/
float
SbSphereSectionProjector::getRadialFactor(void) const
{
  return this->radialFactor;
}

/*!
  Check if \a point is within the part of the sphere used for
  projections.
*/
SbBool
SbSphereSectionProjector::isWithinTolerance(const SbVec3f & point)
{
  if (this->needSetup) this->setupTolerance();
  return (planePoint-point).sqrLength() <= this->sqrtoldist;
}

/*!
  Recalculate the internal projection surface settings. Needs to be
  done if any of the parameters influencing the projection surface
  have been changed from subclasses without using the access methods.
*/
void
SbSphereSectionProjector::setupTolerance(void)
{
  if (this->orientToEye) {
    this->planeDir = -this->viewVol.getProjectionDirection();
    this->worldToWorking.multDirMatrix(this->planeDir, this->planeDir);
    this->planeDir.normalize();
  }
  else {
    this->planeDir.setValue(0.0f, 0.0f, 1.0f);
  }
  if (!this->intersectFront) this->planeDir = -this->planeDir;

  float radius = this->sphere.getRadius();
  this->tolDist = this->tolerance * radius;
  this->sqrtoldist = this->tolDist * this->tolDist;
  this->planeDist = (float)sqrt(radius * radius - this->tolDist * this->tolDist);
  this->planePoint = this->sphere.getCenter() +
    this->planeDir * this->planeDist;
  this->tolPlane = SbPlane(this->planeDir, this->planePoint);
  this->needSetup = FALSE;
}
