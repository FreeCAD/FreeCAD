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
  \class SbDPRotation SbDPRotation.h Inventor/SbDPRotation.h
  \brief The SbDPRotation class represents a rotation in 3D space
  using double precision data.

  \ingroup coin_base

  SbDPRotation is used extensively throughout the Coin library.

  An SbDPRotation is stored internally as a quaternion for speed and
  storage reasons, but inquiries can be done to get and set axis and
  angle values for convenience.

  \sa SbDPMatrix
*/

#include <Inventor/SbDPRotation.h>

#include <Inventor/SbVec3d.h>
#include <Inventor/SbDPMatrix.h>
#include <cassert>
#include <cfloat>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  The default constructor just initializes a valid rotation. The
  actual value is unspecified, and you should not depend on it.
*/
SbDPRotation::SbDPRotation(void)
  // This translates to zero rotation around the positive Z-axis.
  : quat(0.0f, 0.0f, 0.0f, 1.0f)
{
}

/*!
  Construct a new SbDPRotation object initialized with the given
  axis-of-rotation and rotation angle.
 */
SbDPRotation::SbDPRotation(const SbVec3d & axis, const double radians)
{
#if COIN_DEBUG
  if (axis.length()==0.0f)
    SoDebugError::postWarning("SbDPRotation::SbDPRotation",
                              "axis parameter has zero length => "
                              "undefined axis.");
#endif // COIN_DEBUG
  this->setValue(axis, radians);
}

/*!
  Construct a new SbDPRotation object initialized with the given quaternion
  components.

  The array must be ordered as follows:

  q[0] = x, q[1] = y, q[2] = z and q[3] = w, where the quaternion is
  specified by q = w + xi + yj + zk.
 */
SbDPRotation::SbDPRotation(const double q[4])
{
  this->setValue(q);
}

/*!
  Construct a new SbDPRotation object initialized with the given quaternion
  components.
 */
SbDPRotation::SbDPRotation(const double q0, const double q1,
                       const double q2, const double q3)
{
  this->setValue(q0, q1, q2, q3);
}

/*!
  Construct a new SbDPRotation object initialized with the given rotation
  matrix.
 */
SbDPRotation::SbDPRotation(const SbDPMatrix & m)
{
  this->setValue(m);
}

/*!
  Construct a rotation which is the minimum rotation necessary to make vector
  \a rotateFrom point in the direction of vector \a rotateTo.
 */
SbDPRotation::SbDPRotation(const SbVec3d & rotateFrom, const SbVec3d & rotateTo)
{
  // Parameters are checked in setValue().

  this->setValue(rotateFrom, rotateTo);
}

/*!
  Return pointer to an array with the rotation expressed as four
  quaternion values.

  \sa setValue().
*/
const double *
SbDPRotation::getValue(void) const
{
  return &this->quat[0];
}

/*!
  Return the four quaternion components representing the rotation.

  \sa setValue().
 */
void
SbDPRotation::getValue(double & q0, double & q1, double & q2, double & q3) const
{
  q0 = this->quat[0];
  q1 = this->quat[1];
  q2 = this->quat[2];
  q3 = this->quat[3];
}

/*!
  Set the rotation.

  \sa getValue().
*/
SbDPRotation &
SbDPRotation::setValue(const double q0, const double q1,
                     const double q2, const double q3)
{
  this->quat[0] = q0;
  this->quat[1] = q1;
  this->quat[2] = q2;
  this->quat[3] = q3;
  if (this->quat.normalize() == 0.0f) {
#if COIN_DEBUG
    SoDebugError::postWarning("SbRotation::setValue",
                              "Quarternion has zero length => "
                              "undefined rotation.");
#endif // COIN_DEBUG
  }
  return *this;
}

/*!
  Return the rotation in the form of an axis-of-rotation and a rotation
  angle.

  \sa setValue().
 */
void
SbDPRotation::getValue(SbVec3d & axis, double & radians) const
{
  if((this->quat[3] >= -1.0f) && (this->quat[3] <= 1.0f)) {
    radians = double(acos(this->quat[3])) * 2.0f;
    double scale = static_cast<double>(sin(radians / 2.0f));

    if(scale != 0.0f) {
      axis[0] = this->quat[0] / scale;
      axis[1] = this->quat[1] / scale;
      axis[2] = this->quat[2] / scale;
      // FIXME: why not just flip the sign on each component according
      // to "scale" and normalize the axis instead? 20010111 mortene.
      return;
    }
  }

  // Quaternion can't be converted to axis and rotation angle, so we just
  // set up values to indicate this.
  axis.setValue(0.0f, 0.0f, 1.0f);
  radians = 0.0f;
}

/*!
  Return this rotation in the form of a matrix.

  \sa setValue().
 */
void
SbDPRotation::getValue(SbDPMatrix & matrix) const
{
  const double x = this->quat[0];
  const double y = this->quat[1];
  const double z = this->quat[2];
  const double w = this->quat[3];

  matrix[0][0] = w*w + x*x - y*y - z*z;
  matrix[0][1] = 2*x*y + 2*w*z;
  matrix[0][2] = 2*x*z - 2*w*y;
  matrix[0][3] = 0.0f;

  matrix[1][0] = 2*x*y-2*w*z;
  matrix[1][1] = w*w - x*x + y*y - z*z;
  matrix[1][2] = 2*y*z + 2*w*x;
  matrix[1][3] = 0.0f;

  matrix[2][0] = 2*x*z + 2*w*y;
  matrix[2][1] = 2*y*z - 2*w*x;
  matrix[2][2] = w*w - x*x - y*y + z*z;
  matrix[2][3] = 0.0f;

  matrix[3][0] = 0.0f;
  matrix[3][1] = 0.0f;
  matrix[3][2] = 0.0f;
  matrix[3][3] = w*w + x*x + y*y + z*z;
}

/*!
  Invert the rotation. Returns reference to self.

  \sa inverse()
 */
SbDPRotation &
SbDPRotation::invert(void)
{
  double length = this->quat.length();
#if COIN_DEBUG
  if (length==0.0f)
    SoDebugError::postWarning("SbDPRotation::invert",
                              "Quarternion has zero length => "
                              "undefined rotation.");
#endif // COIN_DEBUG

  // Optimize by doing 1 div and 4 muls instead of 4 divs.
  double inv = 1.0f / length;

  this->quat[0] = -this->quat[0] * inv;
  this->quat[1] = -this->quat[1] * inv;
  this->quat[2] = -this->quat[2] * inv;
  this->quat[3] = this->quat[3] * inv;
  return *this;
}

/*!
  Non-destructively inverses the rotation and returns the result.

  \sa invert()
 */
SbDPRotation
SbDPRotation::inverse(void) const
{
  double length = this->quat.length();
#if COIN_DEBUG
  if (length==0.0f)
    SoDebugError::postWarning("SbDPRotation::inverse",
                              "Quaternion has zero length => "
                              "undefined rotation.");
#endif // COIN_DEBUG

  // Optimize by doing 1 div and 4 muls instead of 4 divs.
  double inv = 1.0f / length;

  SbDPRotation rot;
  rot.quat[0] = -this->quat[0] * inv;
  rot.quat[1] = -this->quat[1] * inv;
  rot.quat[2] = -this->quat[2] * inv;
  rot.quat[3] = this->quat[3] * inv;

  return rot;
}

/*!
  Reset the rotation by the four quaternions in the array.

  \sa getValue().
 */
SbDPRotation&
SbDPRotation::setValue(const double q[4])
{
  this->quat[0] = q[0];
  this->quat[1] = q[1];
  this->quat[2] = q[2];
  this->quat[3] = q[3];
  if (this->quat.normalize() == 0.0f) {
#if COIN_DEBUG
    SoDebugError::postWarning("SbRotation::setValue",
                              "Quarternion has zero length => "
                              "undefined rotation.");
#endif // COIN_DEBUG
  }
  return *this;
}

/*!
  Set the rotation from the components of the given matrix. Returns
  reference to self.

  \sa getValue().
 */
SbDPRotation &
SbDPRotation::setValue(const SbDPMatrix & m)
{
  double scalerow = m[0][0] + m[1][1] + m[2][2];

  if (scalerow > 0.0f) {
    double s = static_cast<double>(sqrt(scalerow + m[3][3]));
    this->quat[3] = s * 0.5f;
    s = 0.5f / s;

    this->quat[0] = (m[1][2] - m[2][1]) * s;
    this->quat[1] = (m[2][0] - m[0][2]) * s;
    this->quat[2] = (m[0][1] - m[1][0]) * s;
  }
  else {
    int i = 0;
    if (m[1][1] > m[0][0]) i = 1;
    if (m[2][2] > m[i][i]) i = 2;

    int j = (i+1)%3;
    int k = (j+1)%3;

    double s = static_cast<double>(sqrt((m[i][i] - (m[j][j] + m[k][k])) + m[3][3]));

    this->quat[i] = s * 0.5f;
    s = 0.5f / s;

    this->quat[3] = (m[j][k] - m[k][j]) * s;
    this->quat[j] = (m[i][j] + m[j][i]) * s;
    this->quat[k] = (m[i][k] + m[k][i]) * s;
  }

  if (m[3][3] != 1.0f) this->operator*=(1.0f/static_cast<double>(sqrt(m[3][3])));
  return *this;
}

/*!
  Reset rotation with the given axis-of-rotation and rotation angle.
  Returns reference to self.

  Make sure \a axis is not the null vector when calling this method.

  \sa getValue().
 */
SbDPRotation &
SbDPRotation::setValue(const SbVec3d & axis, const double radians)
{
#if COIN_DEBUG
  if (axis.length()==0.0f)
    SoDebugError::postWarning("SbDPRotation::setValue",
                              "axis parameter has zero length.");
#endif // COIN_DEBUG

  // From <http://www.automation.hut.fi/~jaro/thesis/hyper/node9.html>.

  this->quat[3] = static_cast<double>(cos(radians/2));

  const double sineval = static_cast<double>(sin(radians/2));
  SbVec3d a = axis;
  // we test for a null vector above
  (void) a.normalize();
  this->quat[0] = a[0] * sineval;
  this->quat[1] = a[1] * sineval;
  this->quat[2] = a[2] * sineval;
  return *this;
}

/*!
  Construct a rotation which is the minimum rotation necessary to make vector
  \a rotateFrom point in the direction of vector \a rotateTo.

  Returns reference to self.

  \sa getValue().
 */
SbDPRotation &
SbDPRotation::setValue(const SbVec3d & rotateFrom, const SbVec3d & rotateTo)
{
#if COIN_DEBUG
  // Check if the vectors are valid.
  if (rotateFrom.length()==0.0f) {
    SoDebugError::postWarning("SbDPRotation::setValue",
                              "rotateFrom argument has zero length.");
  }
  if (rotateTo.length()==0.0f) {
    SoDebugError::postWarning("SbDPRotation::setValue",
                              "rotateTo argument has zero length.");
  }
#endif // COIN_DEBUG

  SbVec3d from(rotateFrom);
  // we test for a null vector above
  (void) from.normalize();
  SbVec3d to(rotateTo);
  // we test for a null vector above
  (void) to.normalize();

  const double dot = from.dot(to);
  SbVec3d crossvec = from.cross(to);
  const double crosslen = crossvec.normalize();

  if (crosslen == 0.0f) { // Parallel vectors
    // Check if they are pointing in the same direction.
    if (dot > 0.0) {
      this->setValue(0.0, 0.0, 0.0, 1.0);
    }
    // Ok, so they are parallel and pointing in the opposite direction
    // of each other.
    else {
      // Try crossing with X-axis.
      SbVec3d t = from.cross(SbVec3d(1.0f, 0.0f, 0.0f));
      // If not ok, cross with Y-axis.
      if (t.normalize() == 0.0) {
        t = from.cross(SbVec3d(0.0f, 1.0f, 0.0f));
        (void) t.normalize();
      }
      this->setValue(t[0], t[1], t[2], 0.0f);
    }
  }
  else { // Vectors are not parallel
    // The fabs() wrapping is to avoid problems when `dot' "overflows"
    // a tiny wee bit, which can lead to sqrt() returning NaN.
    crossvec *= static_cast<double>(sqrt(0.5f * fabs(1.0f - dot)));
    // The fabs() wrapping is to avoid problems when `dot' "underflows"
    // a tiny wee bit, which can lead to sqrt() returning NaN.
    this->setValue(crossvec[0], crossvec[1], crossvec[2],
                   static_cast<double>(sqrt(0.5 * fabs(1.0 + dot))));
  }

  return *this;
}

/*!
  Multiplies the quaternions.

  Note that order is important when combining quaternions with the
  multiplication operator.
 */
SbDPRotation &
SbDPRotation::operator*=(const SbDPRotation & q)
{
  // Formula from <http://www.lboro.ac.uk/departments/ma/gallery/quat/>

  double tx, ty, tz, tw;
  this->getValue(tx, ty, tz, tw);
  double qx, qy, qz, qw;
  q.getValue(qx, qy, qz, qw);

  this->setValue(qw*tx + qx*tw + qy*tz - qz*ty,
                 qw*ty - qx*tz + qy*tw + qz*tx,
                 qw*tz + qx*ty - qy*tx + qz*tw,
                 qw*tw - qx*tx - qy*ty - qz*tz);
  return *this;
}

/*!
  Multiplies components of quaternion with scalar value \a s.
  Returns reference to self.
 */
SbDPRotation &
SbDPRotation::operator*=(const double s)
{
  this->quat *= s;
  return *this;
}

/*!
  \relates SbDPRotation

  Check if the two rotations are equal.

  \sa equals().
 */
int
operator==(const SbDPRotation & q1, const SbDPRotation & q2)
{
  return (q1.quat == q2.quat);
}

/*!
  \relates SbDPRotation

  Check if the two rotations are not equal.

  \sa equals().
 */
int
operator!=(const SbDPRotation & q1, const SbDPRotation & q2)
{
  return !(q1 == q2);
}

/*!
  Check the internal quaternion representation vectors for equality
  within the given tolerance.
 */
SbBool
SbDPRotation::equals(const SbDPRotation & r, double tolerance) const
{
  return this->quat.equals(r.quat, tolerance);
}

/*!
  \relates SbDPRotation

  Multiplies the two rotations and returns the result.

  Note that order is important when combining quaternions with the
  multiplication operator.
*/
SbDPRotation
operator*(const SbDPRotation & q1, const SbDPRotation & q2)
{
  SbDPRotation q(q1);
  q *= q2;
  return q;
}

/*!
  Rotate the \a src vector and put the result in \a dst.
 */
void
SbDPRotation::multVec(const SbVec3d & src, SbVec3d & dst) const
{
  // FIXME: this looks amazingly ineffective. Should
  // optimize. 20010907 mortene.

  SbDPMatrix mat;
  mat.setRotate(*this);
  mat.multVecMatrix(src, dst);
}

/*!
  Scale the angle of rotation by \a scaleFactor.
 */
void
SbDPRotation::scaleAngle(const double scaleFactor)
{
  SbVec3d axis;
  double rad;

  this->getValue(axis, rad);
  this->setValue(axis, rad * scaleFactor);
}

/*!
  \relates SbDPRotation

  Interpolates along the shortest path between the two rotation
  positions (from \a rot0 to \a rot1).

  Returns the SbDPRotation which will rotate \a rot0 the given part \a t
  of the spherical distance towards \a rot1, where \a t=0 will yield \a rot0
  and \a t=1 will yield \a rot1.

  \a t should be in the interval [0, 1].
 */
SbDPRotation
SbDPRotation::slerp(const SbDPRotation & rot0, const SbDPRotation & rot1, double t)
{
#if COIN_DEBUG
  if (t<0.0f || t>1.0f) {
    SoDebugError::postWarning("SbDPRotation::slerp",
                              "The t parameter (%f) is out of bounds [0,1]. "
                              "Clamping to bounds.", t);
    if (t<0.0f) t=0.0f;
    else if (t>1.0f) t=1.0f;
  }
#endif // COIN_DEBUG

  SbDPRotation from = rot0;
  SbDPRotation to = rot1;

  double dot = from.quat.dot(to.quat);

  // Find the correct direction of the interpolation.
  if(dot < 0.0f) {
    dot = -dot;
    to.quat.negate();
  }

  // fallback to linear interpolation, in case we run out of floating
  // point precision
  double scale0 = 1.0 - t;
  double scale1 = t;

  if ((1.0f - dot) > FLT_EPSILON) {
    double angle = static_cast<double>(acos(dot));
    double sinangle = static_cast<double>(sin(angle));
    if (sinangle > FLT_EPSILON) {
      // calculate spherical interpolation
      scale0 = double(sin((1.0 - t) * angle)) / sinangle;
      scale1 = double(sin(t * angle)) / sinangle;
    }
  }
  SbVec4d vec = (scale0 * from.quat) + (scale1 * to.quat);
  return SbDPRotation(vec[0], vec[1], vec[2], vec[3]);
}

/*!
  Returns an identity rotation.
 */
SbDPRotation
SbDPRotation::identity(void)
{
  return SbDPRotation(0.0f, 0.0f, 0.0f, 1.0f);
}

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
 */
void
SbDPRotation::print(FILE * fp) const
{
#if COIN_DEBUG
  this->quat.print(fp);
#endif // COIN_DEBUG
}

#ifdef COIN_TEST_SUITE
#include <Inventor/SbVec3d.h>

BOOST_AUTO_TEST_CASE(tgsCompliance) {
  SbDPRotation v = SbRotationd(SbVec3d(0,1,2),3);
}
#endif //COIN_TEST_SUITE
