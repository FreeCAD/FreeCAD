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
  \class SbRotation SbRotation.h Inventor/SbRotation.h
  \brief The SbRotation class represents a rotation in 3D space.

  \ingroup coin_base

  SbRotation is used extensively throughout the Coin library.

  An SbRotation is stored internally as a quaternion for speed and
  storage reasons, but inquiries can be done to get and set axis and
  angle values for convenience.


  Note that there is one \e very common mistake that is easy to make
  when setting the value of an SbRotation, and that is to
  inadvertently use the wrong SbRotation constructor. This example
  should clarify the problem:

  \code
  SbRotation rotation(0, 0, 1, 1.5707963f);
  \endcode

  The programmer clearly tries to set a PI/2 rotation around the Z
  axis, but this will fail, as the SbRotation constructor invoked
  above is the one that takes as arguments the 4 floats of a \e
  quaternion. What the programmer almost certainly wanted to do was to
  use the SbRotation constructor that takes a rotation vector and a
  rotation angle, which is invoked like this:

  \code
  SbRotation rotation(SbVec3f(0, 0, 1), 1.5707963f);
  \endcode


  Another common problem is to set the rotation value to exactly 0.0,
  while wanting to store just the information about a rotation \e
  angle: rotations are internally handled as quaternions, and when
  converting from an angle and a rotation value to a quaternion
  representation, the information about the angle "gets lost" if there
  is no actual rotation.

  \sa SbMatrix
*/


#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/fields/SoSFRotation.h>
#include <cassert>
#include <cfloat>
#include "coinString.h"

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  \fn float SbRotation::operator[](size_t n) const

  \brief returns the n'th quaternion of this rotation
*/

/*!
  The default constructor just initializes a valid rotation. The
  actual value is unspecified, and you should not depend on it.
*/
SbRotation::SbRotation(void)
  // This translates to zero rotation around the positive Z-axis.
  : quat(0.0f, 0.0f, 0.0f, 1.0f)
{
}

/*!
  Construct a new SbRotation object initialized with the given
  axis-of-rotation and rotation angle.
 */
SbRotation::SbRotation(const SbVec3f & axis, const float radians)
{
#if COIN_DEBUG
  if (axis.length()==0.0f)
    SoDebugError::postWarning("SbRotation::SbRotation",
                              "axis parameter has zero length => "
                              "undefined axis.");
#endif // COIN_DEBUG
  this->setValue(axis, radians);
}

/*!
  Construct a new SbRotation object initialized with the given quaternion
  components.

  The array must be ordered as follows:

  q[0] = x, q[1] = y, q[2] = z and q[3] = w, where the quaternion is
  specified by q = w + xi + yj + zk.
 */
SbRotation::SbRotation(const float q[4])
{
  this->setValue(q);
}

/*!
  Construct a new SbRotation object initialized with the given quaternion
  components.
 */
SbRotation::SbRotation(const float q0, const float q1,
                       const float q2, const float q3)
{
  this->setValue(q0, q1, q2, q3);
}

/*!
  Construct a new SbRotation object initialized with the given rotation
  matrix.
 */
SbRotation::SbRotation(const SbMatrix & m)
{
  this->setValue(m);
}

/*!
  Construct a rotation which is the minimum rotation necessary to make
  vector \a rotateFrom point in the direction of vector \a rotateTo.

  Example:

  \code
  #include <Inventor/SbRotation.h>
  #include <Inventor/SbVec3f.h>
  #include <cstdio>

  int
  main(void)
  {
    SbVec3f from(10, 0, 0);
    SbVec3f to(0, 10, 0);

    SbRotation rot(from, to);

    SbVec3f axis;
    float angle;
    rot.getValue(axis, angle);
    axis.print(stdout);
    printf("  angle==%f\n", angle);

    return 0;
  }
  \endcode
*/
SbRotation::SbRotation(const SbVec3f & rotateFrom, const SbVec3f & rotateTo)
{
  // Parameters are checked in setValue().

  this->setValue(rotateFrom, rotateTo);
}

/*!
  Return pointer to an array with the rotation expressed as four
  quaternion values.

  \sa setValue().
*/
const float *
SbRotation::getValue(void) const
{
  return &this->quat[0];
}

/*!
  Return the four quaternion components representing the rotation.

  \sa setValue().
 */
void
SbRotation::getValue(float & q0, float & q1, float & q2, float & q3) const
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
SbRotation &
SbRotation::setValue(const float q0, const float q1,
                     const float q2, const float q3)
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
SbRotation::getValue(SbVec3f & axis, float & radians) const
{
  if((this->quat[3] >= -1.0f) && (this->quat[3] <= 1.0f)) {
    radians = float(acos(this->quat[3])) * 2.0f;
    float scale = static_cast<float>(sin(radians / 2.0f));

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
SbRotation::getValue(SbMatrix & matrix) const
{
  // From:
  // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm
  float l = this->quat.length();
  float x,y,z,w;
  if (l > FLT_EPSILON) {
    // normalize it
    x = this->quat[0] / l;
    y = this->quat[1] / l;
    z = this->quat[2] / l;
    w = this->quat[3] / l;
  }
  else {
    // identity
    x = y = z = 0.0f;
    w = 1.0f;
  }
  matrix[0][0] = 1.0f - 2.0f * (y * y + z * z);
  matrix[0][1] = 2.0f * (x * y + z * w);
  matrix[0][2] = 2.0f * (z * x - y * w);
  matrix[0][3] = 0.0f;

  matrix[1][0] = 2.0f * (x * y - z * w);
  matrix[1][1] = 1.0f - 2.0f * (z * z + x * x);
  matrix[1][2] = 2.0f * (y * z + x * w);
  matrix[1][3] = 0;

  matrix[2][0] = 2.0f * (z * x + y * w);
  matrix[2][1] = 2.0f * (y * z - x * w);
  matrix[2][2] = 1.0f - 2.0f * (y * y + x * x);
  matrix[2][3] = 0.0f;

  matrix[3][0] = 0.0f;
  matrix[3][1] = 0.0f;
  matrix[3][2] = 0.0f;
  matrix[3][3] = 1.0f;
}

/*!
  Invert the rotation. Returns reference to self.

  \sa inverse()
 */
SbRotation &
SbRotation::invert(void)
{
  float length = this->quat.length();
#if COIN_DEBUG
  if (length==0.0f)
    SoDebugError::postWarning("SbRotation::invert",
                              "Quarternion has zero length => "
                              "undefined rotation.");
#endif // COIN_DEBUG

  // Optimize by doing 1 div and 4 muls instead of 4 divs.
  float inv = 1.0f / length;

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
SbRotation
SbRotation::inverse(void) const
{
  float length = this->quat.length();
#if COIN_DEBUG
  if (length==0.0f)
    SoDebugError::postWarning("SbRotation::inverse",
                              "Quaternion has zero length => "
                              "undefined rotation.");
#endif // COIN_DEBUG

  // Optimize by doing 1 div and 4 muls instead of 4 divs.
  float inv = 1.0f / length;

  SbRotation rot;
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
SbRotation&
SbRotation::setValue(const float q[4])
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
SbRotation &
SbRotation::setValue(const SbMatrix & m)
{
  float scalerow = m[0][0] + m[1][1] + m[2][2];

  if (scalerow > 0.0f) {
    float s = static_cast<float>(sqrt(scalerow + m[3][3]));
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

    float s = static_cast<float>(sqrt((m[i][i] - (m[j][j] + m[k][k])) + m[3][3]));

    this->quat[i] = s * 0.5f;
    s = 0.5f / s;

    this->quat[3] = (m[j][k] - m[k][j]) * s;
    this->quat[j] = (m[i][j] + m[j][i]) * s;
    this->quat[k] = (m[i][k] + m[k][i]) * s;
  }

  if (m[3][3] != 1.0f) this->operator*=(1.0f/static_cast<float>(sqrt(m[3][3])));
  return *this;
}

/*!
  Reset rotation with the given axis-of-rotation and rotation angle.
  Returns reference to self.

  Make sure \a axis is not the null vector when calling this method.

  \sa getValue().
 */
SbRotation &
SbRotation::setValue(const SbVec3f & axis, const float radians)
{
#if COIN_DEBUG
  if (axis.length()==0.0f)
    SoDebugError::postWarning("SbRotation::setValue",
                              "axis parameter has zero length.");
#endif // COIN_DEBUG

  // From <http://www.automation.hut.fi/~jaro/thesis/hyper/node9.html>.

  this->quat[3] = static_cast<float>(cos(radians/2));

  const float sineval = static_cast<float>(sin(radians/2));
  SbVec3f a = axis;
  // we test for a null vector above
  (void) a.normalize();
  this->quat[0] = a[0] * sineval;
  this->quat[1] = a[1] * sineval;
  this->quat[2] = a[2] * sineval;
  return *this;
}

/*!
  Construct a rotation which is the minimum rotation necessary to make
  vector \a rotateFrom point in the direction of vector \a rotateTo.

  Returns reference to self.

  See SbRotation constructor with corresponding input arguments for a
  simple code example.

  \sa getValue().
 */
SbRotation &
SbRotation::setValue(const SbVec3f & rotateFrom, const SbVec3f & rotateTo)
{
#if COIN_DEBUG
  // Check if the vectors are valid.
  if (rotateFrom.length()==0.0f) {
    SoDebugError::postWarning("SbRotation::setValue",
                              "rotateFrom argument has zero length.");
  }
  if (rotateTo.length()==0.0f) {
    SoDebugError::postWarning("SbRotation::setValue",
                              "rotateTo argument has zero length.");
  }
#endif // COIN_DEBUG
  SbVec3f from(rotateFrom);
  // we test for a null vector above
  (void) from.normalize();
  SbVec3f to(rotateTo);
  // we test for a null vector above
  (void) to.normalize();

  const float dot = from.dot(to);
  SbVec3f crossvec = from.cross(to);
  const float crosslen = crossvec.normalize();

  if (crosslen == 0.0f) { // Parallel vectors
    // Check if they are pointing in the same direction.
    if (dot > 0.0f) {
      this->setValue(0.0f, 0.0f, 0.0f, 1.0f);
    }
    // Ok, so they are parallel and pointing in the opposite direction
    // of each other.
    else {
      // Try crossing with X-axis.
      SbVec3f t = from.cross(SbVec3f(1.0f, 0.0f, 0.0f));
      // If not ok, cross with Y-axis.
      if (t.normalize() == 0.0f) {
        t = from.cross(SbVec3f(0.0f, 1.0f, 0.0f));
        (void) t.normalize();
      }
      this->setValue(t[0], t[1], t[2], 0.0f);
    }
  }
  else { // Vectors are not parallel
    // The fabs() wrapping is to avoid problems when `dot' "overflows"
    // a tiny wee bit, which can lead to sqrt() returning NaN.
    crossvec *= static_cast<float>(sqrt(0.5f * fabs(1.0f - dot)));
    // The fabs() wrapping is to avoid problems when `dot' "underflows"
    // a tiny wee bit, which can lead to sqrt() returning NaN.
    this->setValue(crossvec[0], crossvec[1], crossvec[2],
                   static_cast<float>(sqrt(0.5 * fabs(1.0 + dot))));
  }

  return *this;
}

/*!
  Multiplies the quaternions.

  Note that order is important when combining quaternions with the
  multiplication operator.
 */
SbRotation &
SbRotation::operator*=(const SbRotation & q)
{
  // Formula from <http://www.lboro.ac.uk/departments/ma/gallery/quat/>

  float tx, ty, tz, tw;
  this->getValue(tx, ty, tz, tw);
  float qx, qy, qz, qw;
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
SbRotation &
SbRotation::operator*=(const float s)
{
  this->quat *= s;
  return *this;
}

/*!
  \relates SbRotation

  Check if the two rotations are equal.

  \sa equals().
 */
int
operator==(const SbRotation & q1, const SbRotation & q2)
{
  return (q1.quat == q2.quat);
}

/*!
  \relates SbRotation

  Check if the two rotations are not equal.

  \sa equals().
 */
int
operator!=(const SbRotation & q1, const SbRotation & q2)
{
  return !(q1 == q2);
}

/*!
  Check the internal quaternion representation vectors for equality
  within the given tolerance.
 */
SbBool
SbRotation::equals(const SbRotation & r, float tolerance) const
{
  return this->quat.equals(r.quat, tolerance);
}

/*!
  \relates SbRotation

  Multiplies the two rotations and returns the result.

  Note that order is important when combining quaternions with the
  multiplication operator.
*/
SbRotation
operator*(const SbRotation & q1, const SbRotation & q2)
{
  SbRotation q(q1);
  q *= q2;
  return q;
}

/*!
  Rotate the \a src vector and put the result in \a dst.

  It is safe to let src and dst be the same SbVec3f instance.
*/
void
SbRotation::multVec(const SbVec3f & src, SbVec3f & dst) const
{
  SbVec3f qv(this->quat[0], this->quat[1], this->quat[2]);
  float r = this->quat[3];

  SbVec3f a = qv.cross(src);
  SbVec3f b = qv.cross(a);

  dst = src + 2.0f * (r * a + b);
}

/*!
  Scale the angle of rotation by \a scaleFactor.
 */
void
SbRotation::scaleAngle(const float scaleFactor)
{
  SbVec3f axis;
  float rad;

  this->getValue(axis, rad);
  this->setValue(axis, rad * scaleFactor);
}

/*!
  \relates SbRotation

  Interpolates along the shortest path between the two rotation
  positions (from \a rot0 to \a rot1).

  Returns the SbRotation which will rotate \a rot0 the given part \a t
  of the spherical distance towards \a rot1, where \a t=0 will yield \a rot0
  and \a t=1 will yield \a rot1.

  \a t should be in the interval [0, 1].
 */
SbRotation
SbRotation::slerp(const SbRotation & rot0, const SbRotation & rot1, float t)
{
#if COIN_DEBUG
  if (t<0.0f || t>1.0f) {
    SoDebugError::postWarning("SbRotation::slerp",
                              "The t parameter (%f) is out of bounds [0,1]. "
                              "Clamping to bounds.", t);
    if (t<0.0f) t=0.0f;
    else if (t>1.0f) t=1.0f;
  }
#endif // COIN_DEBUG

  SbRotation from = rot0;
  SbRotation to = rot1;

  float dot = from.quat.dot(to.quat);

  // Find the correct direction of the interpolation.
  if(dot < 0.0f) {
    dot = -dot;
    to.quat.negate();
  }

  // fallback to linear interpolation, in case we run out of floating
  // point precision
  float scale0 = 1.0f - t;
  float scale1 = t;

  if ((1.0f - dot) > FLT_EPSILON) {
    float angle = static_cast<float>(acos(dot));
    float sinangle = static_cast<float>(sin(angle));
    if (sinangle > FLT_EPSILON) {
      // calculate spherical interpolation
      scale0 = float(sin((1.0 - t) * angle)) / sinangle;
      scale1 = float(sin(t * angle)) / sinangle;
    }
  }
  SbVec4f vec = (scale0 * from.quat) + (scale1 * to.quat);
  return SbRotation(vec[0], vec[1], vec[2], vec[3]);
}

/*!
  Returns an identity rotation.
 */
SbRotation
SbRotation::identity(void)
{
  return SbRotation(0.0f, 0.0f, 0.0f, 1.0f);
}

/*!
  Return a string representation of this object
*/
SbString
SbRotation::toString() const
{
  return CoinInternal::ToString(*this);
}

/*!
  Convert from a string representation, return whether this is a valid conversion
*/
SbBool
SbRotation::fromString(const SbString & str)
{
  SbBool conversionOk;
  *this = CoinInternal::FromString<SbRotation>(str,&conversionOk);
  return conversionOk;
}

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
 */
void
SbRotation::print(FILE * fp) const
{
#if COIN_DEBUG
  this->quat.print(fp);
#endif // COIN_DEBUG
}

#ifdef COIN_TEST_SUITE
#include <Inventor/SbTypeInfo.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec4f.h>
#include <cassert>
#include <cstdio>

typedef SbRotation ToTest;
BOOST_AUTO_TEST_CASE(operatorBrackets)
{
  const int FLOAT_SENSITIVITY = 1;
  const float SQRT2 = sqrt(2.f)/2.f;
  SbRotation rot(0,-SQRT2,0,SQRT2);

  for (int i=0;i<4;++i) {
    int premultiply = ((i&0x1)*((i&0x2)-1));
    float testVal = premultiply*SQRT2;
    BOOST_CHECK_MESSAGE(
                        floatEquals(testVal,rot[i],FLOAT_SENSITIVITY),
                        std::string("Wrong value when trying to access value #")
                        + ::CoinTest::stringify(i)
                        + ": "
                        + ::CoinTest::stringify(rot[i]) +
                        " == "
                        + ::CoinTest::stringify(testVal)
                        );
  }
}

BOOST_AUTO_TEST_CASE(toString) {
  ToTest val(SbVec3f(0, -1, 0),  1);
  SbString str("0 -1 0  1");
  SbVec4f expected(0.f, -1.f, 0.f, 1.f), actual;
  sscanf(val.toString().getString(), "%f %f %f  %f", &actual[0], &actual[1], &actual[2], &actual[3]);
  BOOST_CHECK_MESSAGE(actual.equals(expected, 0.000001f),
                      std::string("Mismatch between ") +  val.toString().getString() + " and control string " + str.getString());

}

BOOST_AUTO_TEST_CASE(fromString) {
  ToTest foo;
  SbString test = "0 -1 0 1";
  ToTest trueVal(SbVec3f(0, -1, 0),  1);
  SbBool conversionOk = foo.fromString(test);
  BOOST_CHECK_MESSAGE(conversionOk && trueVal == foo,
                      std::string("Mismatch between ") +  foo.toString().getString() + " and control " + trueVal.toString().getString());
}

BOOST_AUTO_TEST_CASE(fromInvalidString) {
  ToTest foo;
  SbString test = "2.- 2 3 4";
  ToTest trueVal(1,2,3,4);
  SbBool conversionOk = foo.fromString(test);
  BOOST_CHECK_MESSAGE(conversionOk == FALSE,
                      std::string("Able to convert from ") + test.getString() + " which is not a valid " + SbTypeInfo<ToTest>::getTypeName() + " representation");
}

#endif //COIN_TEST_SUITE
