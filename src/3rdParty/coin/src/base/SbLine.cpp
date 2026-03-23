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
  \class SbLine SbLine.h Inventor/SbLine.h
  \brief The SbLine class represents a line in 3D space.

  \ingroup coin_base

  SbLine provides a way of specifying a directed line, through a 3D
  point (origin) and a vector direction in 3D space.

  It is context dependent whether or not an SbLine is considered as a
  \e ray extending from the starting point in only one direction, or
  as a \e line intersecting the origin point and extending infinitely
  from it in both directions.

  SbLine is used by many other classes in Coin.

  \sa SbVec3f
*/


#include <cassert>

#include <Inventor/SbLine.h>
#include <Inventor/errors/SoDebugError.h>


/*!
  The default constructor does nothing. The line will be uninitialized
  until the first assignment or setValue() call.
*/
SbLine::SbLine(void)
{
}

/*!
  Constructor with \a origin specifying the line origin point, and \a point
  specifying another point on the line that is used to determine the line's
  direction.  \a point should not be the same as \a origin, as this will
  lead to the line having a null vector as the direction vector.  This will
  cause division by zero problems in some of the other methods on this class.

  Be aware that the direction vector will be normalized and not be the same
  as \a point - \a origin.

  \sa setValue, setPosDir, getOrigin, getDirection
*/
SbLine::SbLine(const SbVec3f& origin, const SbVec3f& point)
{
  this->setValue(origin, point);
}

/*!
  Set new position and direction of the line by specifying line origin
  and another point on the line that is used to determine the line's
  direction. \a point should not be the same as \a origin, as this
  will lead to having a null vector as the direction vector.  This will
  cause division by zero problems in some of the other methods on this
  class.

  Be aware that the direction vector will be normalized and not be the same
  as \a point - \a origin.

  Method <em>setPosDir(position, direction)</em> should be preferred over a
  call to <em>setValue(position, position + direction)</em>, because the
  latter causes numerical problems when \a position has a very large magnitude
  and \a direction is a normalized direction vector.

  \sa setPosDir, getOrigin, getDirection
*/
void
SbLine::setValue(const SbVec3f& origin, const SbVec3f& point)
{
  this->pos = origin;
  this->dir = point - origin;

#if COIN_DEBUG
  if(origin == point) {
    SoDebugError::postWarning("SbLine::setValue",
                              "The two points defining the line is "
                              "equal => line is invalid.");
    return;
  }
#endif // COIN_DEBUG

  // we test for a null vector above, just normalize
  (void) this->dir.normalize();
}

/*!
  Set position and direction.

  Be aware that the direction vector will be normalized and not be the same
  as provided to this method.

  \sa setValue, getOrigin, getDirection
  \since Coin 4.0
*/
void
SbLine::setPosDir(const SbVec3f & position, const SbVec3f & direction)
{
  this->pos = position;
  this->dir = direction;
  this->dir.normalize();
}

/*!
  Returns the two closest points on the lines.  If the lines are
  parallel, all points are equally close and we return \c FALSE. If
  the lines are not parallel, the point positions will be stored in \a
  ptOnThis and \a ptOnLine2, and we'll return \c TRUE.

  Note that both SbLine instances are considered to be infinite in
  both directions from their definition points, as far as this
  function is concerned.

  \sa getClosestPoint().
*/
SbBool
SbLine::getClosestPoints(const SbLine& line2,
                         SbVec3f& ptOnThis, SbVec3f& ptOnLine2) const
{
#if 1 
  // new optimized version based on formulas from from Boyko Bantchev

  // p1 = point on line 1
  // p2 = point on line 2
  // d1 = line 1 direction
  // d2 = line 2 direction
  // q1 = closest point on line 1
  // q2 = closest point on line 2

  // The solution (q1 and q2) must be on their respective 
  // lines:
  //
  // q1 = p1 + t1 * d1                               (0)
  // q2 = p2 + t2 * d2
  //
  // we set u = p2 - p1, and get:
  //
  // q2 - q1 = u + t2*d2 - t1*d1                     (1)
  //
  // the solution line q2 - q1 is orthogonal to d1 and d2 
  // (or a null vector if the lines intersect), which yields:
  //
  // (u + t2*d2 - t1*d1) · d1 = 0                    (2)
  // (u + t2*d2 - t1*d1) · d2 = 0
  //
  // we know |d1| and |d2| == 1, and set d1 · d2 = t
  //
  // t1 - t*t2 = u · d1
  // t*t1 - t2 = u · d2
  //
  // Solve for t1, and find q1 using (0):
  //
  // t1 = (u·d1 - t * (u·d2))/ (1 - t^2)
  //
  // just find q2 by using line2.getClosestPoint(q1)

  SbVec3f p1 = this->pos;
  SbVec3f p2 = line2.pos;
  
  SbVec3f d1 = this->dir;
  SbVec3f d2 = line2.dir;

  SbVec3f u = p2-p1;
  float t = d1.dot(d2);

  const float eps = 1.0e-06f;
  if (t < -1.0f + eps || t > 1.0f-eps) {
    // lines are parallel
    return FALSE;
  }
  t = (u.dot(d1) - t * u.dot(d2)) / (1-t*t);
  ptOnThis = p1 + t * d1;
  ptOnLine2 = line2.getClosestPoint(ptOnThis);
  return TRUE;
#else // old version

#if COIN_DEBUG
  if(this->getDirection().length() == 0.0)
    SoDebugError::postWarning("SbLine::getClosestPoints",
                              "This line has no direction (zero vector).");
  if(line2.getDirection().length() == 0.0)
    SoDebugError::postWarning("SbLine::getClosestPoints",
                              "argument line has no direction (zero vector).");
#endif // COIN_DEBUG

  // Check if the lines are parallel.
  // FIXME: should probably use equals() here.
  if(line2.dir == this->dir) return FALSE;
  else if(line2.dir == -this->dir) return FALSE;


  // From the discussion on getClosestPoint(), we know that the point
  // we wish to find on a line can be expressed as:
  //
  //                  (Q1-P0)·D0
  //   Q0 = P0 + D0 * ----------
  //                     |D0|
  //
  // ...where P0 is a point on the first line, D0 is the direction
  // vector and Q1 is the "closest point" on the other line. From this
  // we get two equations with two unknowns. By substituting for
  // Q1 we get a new equation with a single unknown, Q0:
  //
  //                   (         (Q0 - P1)·D1    )
  //                   (P1 + D1 * ------------ - P0) · D0
  //                   (             |D1|        )
  //   Q0 = P0 + D0 * ------------------------------------
  //                                |D0|
  //
  // Which obviously is bloody hard (perhaps impossible?) to solve
  // analytically. Damn. Back to the pen and pencil stuff.
  //
  // Ok, new try. Since we're looking for the minimum distance between the
  // two lines, we should be able to solve it by expressing the distance
  // between the points we want to find as a parametrized function and
  // take the derivative:
  //
  //   f(t0, t1) = |Q1 - Q0| = |P1+D1*t1 - (P0+D0*t0)|
  //
  //                         (t1*D1 - P0)·D0
  // t0 can be expressed as  ---------------  which gives us
  //                               |D0|
  //
  //   f(t) = |P1 + D1*t - P0 - D0N * ((t*D1 - P0)·D0)|, t = t1
  //                                                     D0N = D0 normalized
  //                               _____________
  // ..which is eual to   f(t) = \/Þ² + ß² + ð²  , where Þ, ß, and ð
  // is the full expression above with the x, y, and z components
  // of the vectors.
  //
  // Since we're looking for the minimum value of the function, we can just
  // ignore the square root. We'll do the next parts of the math on a
  // general components case, since it's the same for the x, y and z parts.
  //
  // Expanding any of the Þ, ß, or ð expressions, we get this:
  //   (P1[i] - D1[i]*t - P0[i] - D0N[i]*D0[x]*D1[x]*t + D0N[i]*D0[x]*P0[x]
  //      - D0N[i]*D0[y]*D1[y]*t + D0N[i]*D0[y]*P0[y] - D0N[i]*D0[z]*D1[z]*t
  //      + D0N[i]*D0[z]*P0[z])² ,
  // where i=[x|y|z].
  //
  // Derivating this by using the chain rule (i.e. g(t)² =
  // 2*g(t)*g'(t)), we'll get this equation for finding the t yielding
  // the minimum distance between two points Q0 and Q1 on the lines:
  //
  //      -(cx*dx+cy*dy+cz*dz)
  //  t = --------------------
  //        dx² + dy² + dz²
  //
  //  di = D1[i] - D0N[i] * (D0[x]*D1[x] + D0[y]*D1[y] + D0[z]*D1[z])
  // and
  //  ci = P1[i] - P0[i] + D0N[i] * (D0[x]*P0[x] + D0[y]*P0[y] + D0[z]*P0[z])
  // where i=[x|y|z].
  //
  // Now we'll substitute t back in for t1 in   Q1 = P1 + D1*t1, which'll
  // also let us find Q0 by an invocation of getClosestPoint().
  //
  // That's it. I can't believe this took me 4 hours to complete. Code worked
  // on the first run, though. :-)
  //                                                           19980815 mortene.

  SbVec3f P0 = this->pos;
  SbVec3f P1 = line2.pos;
  SbVec3f D0 = this->dir;
  SbVec3f D1 = line2.dir;
  SbVec3f D0N = D0;

  // we warn about lines with no direction above, just normalize
  (void) D0N.normalize();

  float c[3], d[3];

  for (int i=0; i < 3; i++) {
    d[i] =
      D1[i] - D0N[i]*(D0[0]*D1[0] + D0[1]*D1[1] + D0[2]*D1[2]);
    c[i] =
      P1[i] - P0[i] + D0N[i]*(D0[0]*P0[0] + D0[1]*P0[1] + D0[2]*P0[2]);
  }

  float t = -(c[0]*d[0]+c[1]*d[1]+c[2]*d[2]) / (d[0]*d[0]+d[1]*d[1]+d[2]*d[2]);

  ptOnLine2 = line2.pos + line2.dir * t;
  ptOnThis = this->getClosestPoint(ptOnLine2);

  return TRUE;
#endif
}

/*!
  Returns the point on the line which is closest to \a point:

  \verbatim
                      _
               Q      D
      SP x-----x------->
          \    |
           \   |
            \  |
             \ |
              \|
               x P
  

   P = argument point            _
   SP = line starting point, and D = line direction of this line
   Q = point to find and return from function

  \endverbatim

  The vector defined by \a point and the returned point will be normal
  to the line.

  Note that the SbLine instance is considered to be infinite in both
  directions from its definition point, as far as this function is
  concerned.

  \sa getClosestPoints().
*/
SbVec3f
SbLine::getClosestPoint(const SbVec3f & point) const
{
  //
  //             Q      D
  //    SP x-----x------->
  //        \    |
  //         \   |
  //          \  |
  //           \ |
  //            \|
  //             x P
  //
  // P = argument point, SP = line starting point, D = line direction,
  // Q = point to find.
  //
  // Solved by:
  //                         a·b
  //             comp_b(a) = ---   , a = P-SP, b = D, comp_b(a) = |Q-SP|
  //                         |b|
  //
  //  ==>   Q = SP + comp_b(a)*D
  //                                        19980815 mortene.

  // No use warning about a zero length line here. The user will get a
  // warning when the line is constructed. Also, we don't need to
  // account for the length of the direction vector, since this->dir
  // is always normalized (or a null-vector). The result will actually
  // be correct when the line has zero length, since the line starting
  // point will then be the closest point. 2005-02-24 pederb, with
  // thanks to Boyko Bantchev for useful suggestions.
  return this->pos + this->dir * (point - this->pos).dot(this->dir);
}

/*!
  Return a vector representing the origin point on the line.

  \sa setValue
 */
const SbVec3f&
SbLine::getPosition(void) const
{
  return this->pos;
}

/*!
  Return a vector representing the direction of the line. The direction
  vector will always be normalized.
 */
const SbVec3f&
SbLine::getDirection(void) const
{
  return this->dir;
}

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
 */
void
SbLine::print(FILE * fp) const
{
#if COIN_DEBUG
  fprintf( fp, "p: " );
  this->getPosition().print(fp);
  fprintf( fp, "d: " );
  this->getDirection().print(fp);
#endif // COIN_DEBUG
}
