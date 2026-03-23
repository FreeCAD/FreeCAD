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

#include <Inventor/SbVec4f.h>

#include <limits>
#include <cassert>

#include <Inventor/SbVec4d.h>
#include <Inventor/SbVec4b.h>
#include <Inventor/SbVec4s.h>
#include <Inventor/SbVec4i32.h>
#include <Inventor/SbVec3f.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "tidbitsp.h" // coin_debug_normalize()

/*!
  \class SbVec4f SbVec4f.h Inventor/SbVec4f.h
  \brief The SbVec4f class is a 4 dimensional vector with floating point coordinates.

  \ingroup coin_base

  This vector class is used by many other classes in
  Coin. It provides storage for a 3 dimensional homogeneous
  vector (with the 4 components usually referred to as <x, y, z, w>)
  as well as simple floating point arithmetic operations.

  \sa SbVec2s, SbVec2f, SbVec2d, SbVec3s, SbVec3f, SbVec3d, SbVec4d.
*/

/*!
  \fn SbVec4f::SbVec4f(void)

  The default constructor does nothing. The vector coordinates will be
  uninitialized until a call the setValue().
*/

/*!
  \fn SbVec4f::SbVec4f(const float v[4])
  Constructs an SbVec4f instance with initial values from \a v.
*/

/*!
  \fn SbVec4f::SbVec4f(float x, float y, float z, float w)

  Constructs an SbVec4f instance with the initial homogeneous vector
  set to \a <x,y,z,w>.
*/

/*!
  \fn SbVec4f::SbVec4f(const SbVec4d & v)

  Constructs an SbVec4f instance from an SbVec4d instance.
*/

/*!
  \fn SbVec4f::SbVec4f(const SbVec4b & v)

  Constructs an SbVec4f instance from an SbVec4b instance.

  \since Coin 2.5
*/

/*!
  \fn SbVec4f::SbVec4f(const SbVec4s & v)

  Constructs an SbVec4f instance from an SbVec4s instance.

  \since Coin 2.5
*/

/*!
  \fn SbVec4f::SbVec4f(const SbVec4i32 & v)

  Constructs an SbVec4f instance from an SbVec4i32 instance.

  \since Coin 2.5
*/

/*!
  \fn float SbVec4f::dot(const SbVec4f & v) const

  Calculates and returns the result of taking the dot product of this
  vector and \a v.
*/

/*!
  Compares the vector with \a v and returns \c TRUE if the distance
  between the vectors is smaller or equal to the square root of
  \a tolerance.

  The comparison is done in 4D-space, i.e. the \a w component of the
  vector is \e not used to make x, y and z into Cartesian coordinates
  first.
*/
SbBool
SbVec4f::equals(const SbVec4f& v, float tolerance) const
{
#if COIN_DEBUG
  if(!(tolerance >= 0.0f))
    SoDebugError::postWarning("SbVec4f::equals","Tolerance should be >= 0.0f");
#endif // COIN_DEBUG

  float xdist = this->vec[0] - v[0];
  float ydist = this->vec[1] - v[1];
  float zdist = this->vec[2] - v[2];
  float wdist = this->vec[3] - v[3];

  if((xdist*xdist + ydist*ydist + zdist*zdist + wdist*wdist) <= tolerance)
    return TRUE;
  return FALSE;
}

/*!
  Returns the vector as a Cartesian 3D vector in \a v. This means that
  the 3 first components x, y and z will be divided by the fourth, w.
*/
void
SbVec4f::getReal(SbVec3f & v) const
{
#if COIN_DEBUG
  if(!(this->vec[3] != 0.0f))
    SoDebugError::postWarning("SbVec4f::getReal",
                              "The 4th vector component is zero => "
                              "division by zero");
#endif // COIN_DEBUG

  v.setValue(vec[0]/vec[3], vec[1]/vec[3], vec[2]/vec[3]);
}

/*!
  \fn const float * SbVec4f::getValue(void) const
  Returns a pointer to an array of four floats containing the
  x, y, z and w coordinates of the vector.

  \sa setValue().
*/

/*!
  \fn void SbVec4f::getValue(float & x, float & y, float & z, float & w) const
  Returns the x, y, z and w coordinates of the vector.

  \sa setValue().
*/

/*!
  Return the length of the vector in 4D space.
 */
float
SbVec4f::length(void) const
{
  return static_cast<float>(sqrt(this->sqrLength()));
}

/*!
  \fn float SbVec4f::sqrLength(void) const
  Return the square of the length of the vector in 4D space.
*/

/*!
  \fn void SbVec4f::negate(void)
  Negate the vector.
*/

/*!
  Normalize the vector to unit length. Return value is the original
  length of the vector before normalization.
*/

float
SbVec4f::normalize(void)
{
  float len = this->length();
  //This number is found by doing some testing, but I suspect it to be a bit to high. BFG
  static const float NORMALIZATION_TOLERANCE = 1.0/16777216;

  if (len > 0.0f) {
    //We don't want to normalize if we are close enough, as we are
    //probably just going to make things worse
    if (SbAbs(len-1.0) > NORMALIZATION_TOLERANCE) {
      operator/=(len);
    }
  }
#if COIN_DEBUG
  else if (coin_debug_normalize()) {
    SoDebugError::postWarning("SbVec4f::normalize",
                              "The length of the vector should be > 0.0f "
                              "to be able to normalize.");
  }
#endif // COIN_DEBUG

  return len;
}

/*!
  \fn SbVec4f & SbVec4f::setValue(const float v[4])
  Set new coordinates for the vector from \a v. Returns reference to
  self.

  \sa getValue().
*/

/*!
  \fn SbVec4f & SbVec4f::setValue(float x, float y, float z, float w)

  Set new coordinates for the vector. Returns reference to self.

  \sa getValue().
*/

/*!
  Sets the value from an SbVec4d instance.
*/

SbVec4f &
SbVec4f::setValue(const SbVec4d & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<float>::max() || v[0] < -std::numeric_limits<float>::max() || 
      v[1] > std::numeric_limits<float>::max() || v[1] < -std::numeric_limits<float>::max() || 
      v[2] > std::numeric_limits<float>::max() || v[2] < -std::numeric_limits<float>::max()) {
    SoDebugError::post("SbVec4f::setValue", "SbVec4d argument out of range for SbVec4f storage");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<float>(v[0]);
  vec[1] = static_cast<float>(v[1]);
  vec[2] = static_cast<float>(v[2]);
  vec[3] = static_cast<float>(v[3]);
  return *this;
}

/*!
  \since Coin 2.5
*/

SbVec4f &
SbVec4f::setValue(const SbVec4b & v)
{
  vec[0] = static_cast<float>(v[0]);
  vec[1] = static_cast<float>(v[1]);
  vec[2] = static_cast<float>(v[2]);
  vec[3] = static_cast<float>(v[3]);
  return *this;
}

/*!
  \since Coin 2.5
*/

SbVec4f &
SbVec4f::setValue(const SbVec4s & v)
{
  vec[0] = static_cast<float>(v[0]);
  vec[1] = static_cast<float>(v[1]);
  vec[2] = static_cast<float>(v[2]);
  vec[3] = static_cast<float>(v[3]);
  return *this;
}

/*!
  \since Coin 2.5
*/

SbVec4f &
SbVec4f::setValue(const SbVec4i32 & v)
{
  vec[0] = static_cast<float>(v[0]);
  vec[1] = static_cast<float>(v[1]);
  vec[2] = static_cast<float>(v[2]);
  vec[3] = static_cast<float>(v[3]);
  return *this;
}

/*!
  \fn float & SbVec4f::operator [] (int i)

  Index operator. Returns modifiable x, y, z or w component of vector.

  \sa getValue() and setValue().
*/

/*!
  \fn const float & SbVec4f::operator [] (int i) const

  Index operator. Returns x, y, z or w component of vector.

  \sa getValue() and setValue().
*/

/*!
  \fn SbVec4f & SbVec4f::operator *= (float d)

  Multiply components of vector with value \a d. Returns reference to self.
*/

/*!
  \fn SbVec4f & SbVec4f::operator /= (float d)
  Divides components of vector with value \a d. Returns reference to self.
*/

/*!
  \fn SbVec4f & SbVec4f::operator += (const SbVec4f & v)

  Adds this vector and vector \a v. Returns reference to self.
*/

/*!
  \fn SbVec4f & SbVec4f::operator -= (const SbVec4f & v)

  Subtracts vector \a v from this vector. Returns reference to self.
*/

/*!
  \fn SbVec4f SbVec4f::operator - (void) const

  Non-destructive negation operator. Returns a new SbVec4f instance which
  has all components negated.

  \sa negate().
*/

/*!
  \fn SbVec4f operator * (const SbVec4f & v, float d)
  \relates SbVec4f

  Returns an SbVec4f instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec4f operator * (float d, const SbVec4f & v)
  \relates SbVec4f

  Returns an SbVec4f instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec4f operator / (const SbVec4f & v, float d)
  \relates SbVec4f

  Returns an SbVec4f instance which is the components of vector \a v
  divided on the scalar factor \a d.
*/

/*!
  \fn SbVec4f operator + (const SbVec4f & v1, const SbVec4f & v2)
  \relates SbVec4f

  Returns an SbVec4f instance which is the sum of vectors \a v1 and \a v2.
*/

/*!
  \fn SbVec4f operator - (const SbVec4f & v1, const SbVec4f & v2)
  \relates SbVec4f

  Returns an SbVec4f instance which is vector \a v2 subtracted from
  vector \a v1.
*/

/*!
  \fn int operator == (const SbVec4f & v1, const SbVec4f & v2)
  \relates SbVec4f

  Returns \a 1 if \a v1 and \a v2 are equal, \a 0 otherwise.

  \sa equals().
*/

/*!
  \fn int operator != (const SbVec4f & v1, const SbVec4f & v2)
  \relates SbVec4f

  Returns \a 1 if \a v1 and \a v2 are not equal, \a 0 if they are equal.

  \sa equals().
*/

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
 */
void
SbVec4f::print(FILE * fp) const
{
#if COIN_DEBUG
  fprintf( fp, "<%f, %f, %f, %f>", this->vec[0], this->vec[1], this->vec[2],
    this->vec[3] );
#endif // COIN_DEBUG
}

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(noNormalizingNormalized)
{
  const float SQRT2 = sqrt(2.f)/2.f;
  SbVec4f vec(0,-SQRT2,0,SQRT2);

  vec.normalize();
  for (int i=0;i<4;++i) {
    int premultiply = ((i&0x1)*((i&0x2)-1));
    float testVal = premultiply*SQRT2;
    BOOST_CHECK_MESSAGE(
                        testVal==vec[i],
                        std::string("Wrong value when trying to access value #")
                        + ::CoinTest::stringify(i)
                        + ": "
                        + ::CoinTest::stringify(vec[i]) +
                        " == "
                        + ::CoinTest::stringify(testVal)
                        );
  }

}

BOOST_AUTO_TEST_CASE(normalizingDeNormalized)
{
  const int FLOAT_SENSITIVITY = 1;
  const float SQRT2 = sqrt(2.f)/2.f;
  SbVec4f vec(0,-1,0,1);

  vec.normalize();
  for (int i=0;i<4;++i) {
    int premultiply = ((i&0x1)*((i&0x2)-1));
    float testVal = premultiply*SQRT2;
    BOOST_CHECK_MESSAGE(
                        floatEquals(testVal,vec[i],FLOAT_SENSITIVITY),
                        std::string("Wrong value when trying to access value #")
                        + ::CoinTest::stringify(i)
                        + ": "
                        + ::CoinTest::stringify(vec[i]) +
                        " == "
                        + ::CoinTest::stringify(testVal)
                        );
  }

}

#endif //COIN_TEST_SUITE
