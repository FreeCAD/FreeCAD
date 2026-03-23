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
  \class SbSphere SbSphere.h Inventor/SbSphere.h
  \brief The SbSphere class is a representation of a sphere.

  \ingroup coin_base

  This class is used within many other classes in Coin. It contains
  the data necessary to represent a sphere (a 3D point and a radius).

  \sa SbCylinder */

#include <cassert>
#include <Inventor/SbSphere.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SbLine.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  The default constructor does nothing. The center point and the radius
  will be uninitialized.
 */
SbSphere::SbSphere(void)
{
}

/*!
  Construct an SbSphere instance with the given center point and radius.
 */
SbSphere::SbSphere(const SbVec3f &centerarg, const float radiusarg)
{
#if COIN_DEBUG
  if (radiusarg<0.0f)
    SoDebugError::postWarning("SbSphere::SbSphere",
                              "Radius should be >= 0.0f.");
#endif // COIN_DEBUG

  this->setValue(centerarg, radiusarg);
}

/*!
  Set the sphere's center point and radius.

  \sa getCenter(), getRadius().
 */
void
SbSphere::setValue(const SbVec3f &centerarg, const float radiusarg)
{
#if COIN_DEBUG
  if (radiusarg<0.0f)
    SoDebugError::postWarning("SbSphere::setValue",
                              "Radius should be >= 0.0f.");
#endif // COIN_DEBUG
  this->setCenter(centerarg);
  this->setRadius(radiusarg);
}

/*!
  Set the sphere's center point.

  \sa setValue(), setRadius() and getCenter().
 */
void
SbSphere::setCenter(const SbVec3f &centerarg)
{
  this->center = centerarg;
}

/*!
  Set the sphere's radius.

  \sa setValue(), setCenter() and getRadius().
 */
void
SbSphere::setRadius(const float radiusarg)
{
#if COIN_DEBUG
  if (radiusarg<0.0f)
    SoDebugError::postWarning("SbSphere::setRadius",
                              "Radius should be >= 0.0f.");
#endif // COIN_DEBUG
  this->radius = radiusarg;
}

/*!
  Returns an SbVec3f with the sphere's center point.

  \sa setCenter(), getRadius().
 */
const SbVec3f &
SbSphere::getCenter(void) const
{
  return this->center;
}

/*!
  Returns the sphere's radius.

  \sa setRadius(), getCenter().
 */
float
SbSphere::getRadius(void) const
{
  return this->radius;
}

/*!
  Make the sphere exactly contain \a box, i.e. the sphere center point
  will be the same as that of the box, and the radius will be the distance
  from the box center point to any of the corners.
 */
void
SbSphere::circumscribe(const SbBox3f &box)
{
#if COIN_DEBUG
  if (box.isEmpty()) {
    SoDebugError::postWarning("SbSphere::circumscribe",
                              "The box is empty.");
    return;
  }
#endif // COIN_DEBUG

  this->setCenter(box.getCenter());

  float dx, dy, dz;
  box.getSize(dx, dy, dz);

  this->setRadius(float(sqrt(dx*dx + dy*dy + dz*dz)) / 2.0f);
}

/*!
  Finds the intersection enter point for the given line \a l
  on the sphere.

  If the line does not intersect the sphere, \a FALSE is returned.
 */
SbBool
SbSphere::intersect(const SbLine &l, SbVec3f &intersection) const
{
  SbVec3f dummy;
  return this->intersect(l, intersection, dummy);
}

/*!
  Find the intersection points of the ray \a l on the sphere and
  return these in \a enter and \a exit. If the ray just "grazes"
  the sphere, the \a enter and \a exit points have equal values.

  If the ray does not intersect the sphere, \a FALSE is returned, otherwise
  we will return \a TRUE.
 */
SbBool
SbSphere::intersect(const SbLine &l, SbVec3f &enter, SbVec3f &exit) const
{
#if COIN_DEBUG
  if (!(l.getDirection().length()>0.0f))
    SoDebugError::postWarning("SbSphere::intersect",
                              "The line 'l' has no direction.");
#endif // COIN_DEBUG

  // Compute point on the line that is closest to the sphere center.
  SbVec3f closestpt = l.getClosestPoint(this->getCenter());

  // Sphere center, closest point on the line and intersection
  // point(s) form a right-angled triangle. The distance between closest point
  // and intersection point(s) can be computed using Pythagoras' theorem.
  float sqrradius = this->getRadius() * this->getRadius();
  float sqrdistcenter = (this->getCenter() - closestpt).sqrLength();
  float sqrdistintersect = sqrradius - sqrdistcenter;

  if (sqrdistintersect < 0) {
    // no intersection of sphere and line exists
    return FALSE;
  }
  else {
    float t = sqrtf(sqrdistintersect);
    enter = closestpt - t * l.getDirection();
    exit  = closestpt + t * l.getDirection();
    return TRUE;
  }
}

/*!
  Returns \a TRUE of the given point \a p lies within the sphere.
 */
SbBool
SbSphere::pointInside(const SbVec3f &p) const
{
  return (p - center).length() < radius;
}

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
 */
void
SbSphere::print(FILE * fp) const
{
#if COIN_DEBUG
  fprintf( fp, "center: " );
  this->getCenter().print(fp);
  fprintf( fp, "  radius: %f ", this->getRadius() );
#endif // COIN_DEBUG
}
