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
  \class SbCylinder SbCylinder.h Inventor/SbCylinder.h
  \brief The SbCylinder class is a representation of a cylinder.

  \ingroup coin_base

  This class is used within other classes in Coin. It contains data to
  represent a cylinder by an axis and a radius. The cylinder has no
  length/height value, which means it is treated as of infinite
  length.

  \sa SbLine, SbSphere */

#include <cassert>
#include <Inventor/SbCylinder.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  The default constructor will make a cylinder of radius 1, center axis
  going through origo in the parallel direction of the positive y-axis.
 */
SbCylinder::SbCylinder(void)
{
  this->setValue(SbLine(SbVec3f(0.0f, 0.0f, 0.0f), SbVec3f(0.0f, 1.0f, 0.0f)),
                 1.0f);
}

/*!
  Construct and initialize an SbCylinder instance with the given axis \a a
  and radius \a r.
 */
SbCylinder::SbCylinder(const SbLine& a, const float r)
{
  this->setValue(a, r);
}

/*!
  Set the axis and radius of the cylinder.

  \sa getAxis(), getRadius().
 */
void
SbCylinder::setValue(const SbLine& a, const float r)
{
  this->setAxis(a);
  this->setRadius(r);
}

/*!
  Set the center axis of the cylinder.

  \sa setRadius(), getAxis().
 */
void
SbCylinder::setAxis(const SbLine& a)
{
#if COIN_DEBUG
  if (!(a.getDirection().length() > 0.0f))
    SoDebugError::postWarning("SbCylinder::setAxis",
                              "Axis has zero length => undefined");
#endif // COIN_DEBUG

  this->axis = a;
}

/*!
  Set the radius of the cylinder.

  \sa setAxis(), getRadius().
 */
void
SbCylinder::setRadius(const float r)
{
#if COIN_DEBUG
  if (!(r>=0.0f))
    SoDebugError::postWarning("SbCylinder::setRadius",
                              "Radius (%f) should be positive.",r);
#endif // COIN_DEBUG

  this->radius = r;
}

/*!
  Return the line which is the center axis of the cylinder.

  \sa setAxis(), getRadius().
 */
const SbLine&
SbCylinder::getAxis(void) const
{
  return this->axis;
}

/*!
  Return radius of cylinder.

  \sa setRadius(), getAxis().
 */
float
SbCylinder::getRadius(void) const
{
  return this->radius;
}

/*!
  Finds the intersection point of the ray \a l on the cylinder where
  the ray enters, and return this point in \a intersection.

  If the ray does not intersect the cylinder, \a FALSE is returned,
  otherwise we will return \a TRUE.
 */
SbBool
SbCylinder::intersect(const SbLine& l, SbVec3f& intersection) const
{
  SbVec3f dummy;
  return this->intersect(l, intersection, dummy);
}

/*!
  Find the intersection points of the ray \a l on the cylinder
  and return these in \a enter and \a exit. If the ray just "grazes"
  the cylinder, the \a enter and \a exit points have equal values.

  If the ray does not intersect the cylinder, \a FALSE is returned,
  otherwise we will return \a TRUE.
 */
SbBool
SbCylinder::intersect(const SbLine& l, SbVec3f& enter, SbVec3f& exit) const
{
#if COIN_DEBUG
  if (!(l.getDirection().length()>0.0f))
    SoDebugError::postWarning("SbCylinder::intersect",
                              "The ray 'l' has zero length => undefined");
#endif // COIN_DEBUG
  //
  // What seems to be the immediately most simple, gung-ho method of
  // solving this - solving for   |Qr - Qc| = r    - won't work because
  // there are possibly many points which match this equation that are
  // not part of the solution we're after (think about it).
  //
  // So, to solve for the intersection between a ray and a cylinder, we
  // move from 3D to 2D by setting up a right-handed orthonormal
  // coordinate system where one of the axis is identical to the cylinder
  // direction vector (U2), and the other two axis (U0 and U1) are
  // perpendicular to the first. A point in this coordinate system can
  // be written as
  //                     Qc = Pc + x*U0 + y*U1 + z*U2
  //
  // The points on the cylinder is then those matching x²+y²=r².
  //
  // For the ray, we project it into the coordinate system by
  // rewriting the line equation   Qr = Pr + t*Dr  as
  //
  //    Qr = Pc + x0*U0+y0*U1+z0*U2 + t*(x1*U0+y1*U1+z1*U2)
  //
  // The intersection is then determined by
  //
  //    (x0 + t*x1)² + (y0 + t*y1)² = r²
  //
  // => (x1²+y1²)*t² + 2*(x0*x1+y0*y1)*t + (x1²+y1²-r²) = 0
  //
  // Which can be solved by:
  // t = (-b ± sqrt(b² - 4ac))/2a, a = x1² + y1², b = 2*(x0*x1+y0*y1),
  //                               c = (x1²+y1²-r²),
  //
  // ...
  //                                                        19980824 mortene.


  SbVec3f cv = l.getPosition() - this->axis.getPosition();

  // If the ray is not parallel with the cylinder, we'll need this cross
  // product anyhow, so use this instead of the dot product to determine
  // if they are parallel or not.
  SbVec3f n = l.getDirection().cross(this->axis.getDirection());
  float len = n.normalize();

  if (len == 0.0f) {
    // They are parallel, check if we're inside or outside the cylinder
    // by getting the distance between the cylinder axis and the ray.
#if 0
    float dist = cv.dot(this->axis.getDirection());
    dist = (cv - dist*this->axis.getDirection()).length();
#endif
    if((this->axis.getClosestPoint(l.getPosition()) -
        l.getPosition()).length() <= radius) {
      // We're inside the cylinder, set enter and exit points to be
      // very large values (the cylinder is conceptually of infinite
      // length).
      float offset = 14530000; // TODO: find out what define OI uses
      enter = l.getPosition() + -offset*l.getDirection();
      exit = l.getPosition() + offset*l.getDirection();
      return TRUE;
    }

    return FALSE;
  }

  // Check the closest distance from the ray to the cylinder axis. If
  // this distance is larger than the radius of the cylinder, there's
  // of course no intersection.
  float d = static_cast<float>(fabs(cv.dot(n)));
  if (d > this->radius) return FALSE;

  // There's an intersection, now find the parameter for the plane
  // equation.
  SbVec3f tmp = cv.cross(this->axis.getDirection());
  float t = -tmp.dot(n)/len;
  tmp = n.cross(this->axis.getDirection());
  // we know that n is not parallel with axis, just normalize
  (void) tmp.normalize();

  float s = static_cast<float>(fabs(sqrt(this->radius*this->radius - d*d) /
                        l.getDirection().dot(tmp)));

  float enterparam = t - s;
  float exitparam = t + s;
  if (enterparam > exitparam) SbSwap(enterparam, exitparam);

  enter = l.getPosition() + enterparam * l.getDirection();
  exit = l.getPosition() + exitparam * l.getDirection();

  return TRUE;
}

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
 */
void
SbCylinder::print(FILE * fp) const
{
#if COIN_DEBUG
  fprintf( fp, "axis: " );
  this->getAxis().print(fp);
  fprintf( fp, "  radius: %f ", this->getRadius() );
#endif // COIN_DEBUG
}
