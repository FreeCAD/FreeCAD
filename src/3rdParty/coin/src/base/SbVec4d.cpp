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

#include <Inventor/SbVec4d.h>

#include <limits>
#include <cassert>

#include <Inventor/SbVec4f.h>
#include <Inventor/SbVec4b.h>
#include <Inventor/SbVec4s.h>
#include <Inventor/SbVec4i32.h>
#include <Inventor/SbVec3d.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "tidbitsp.h" // coin_debug_normalize()

/*!
  \class SbVec4d SbVec4d.h Inventor/SbVec4d.h
  \brief The SbVec4d class is a 4 dimensional vector with double precision
  floating point coordinates.

  \ingroup coin_base

  This vector class is not by many other classes in
  Coin. It provides storage for a 3 dimensional homogeneous
  vector (with the 4 components usually referred to as <x, y, z, w>)
  as well as simple double precision floating point arithmetic operations.

  \sa SbVec2s, SbVec2f, SbVec2d, SbVec3s, SbVec3f, SbVec3d, SbVec4f.
*/

/*!
  \fn SbVec4d::SbVec4d(void)
  The default constructor does nothing. The vector coordinates will be
  uninitialized until a call the setValue().
*/

/*!
  \fn SbVec4d::SbVec4d(const double v[4])
  Constructs an SbVec4d instance with initial values from \a v.
*/

/*!
  \fn SbVec4d::SbVec4d(double x, double y, double z, double w)

  Constructs an SbVec4d instance with the initial homogeneous vector
  set to \a <x,y,z,w>.
*/

/*!
  \fn SbVec4d::SbVec4d(const SbVec4f & v)

  Constructs an SbVec4d instance from an SbVec4f instance.
*/

/*!
  \fn SbVec4d::SbVec4d(const SbVec4b & v)

  Constructs an SbVec4d instance from an SbVec4b instance.

  \since Coin 2.5
*/

/*!
  \fn SbVec4d::SbVec4d(const SbVec4s & v)

  Constructs an SbVec4d instance from an SbVec4s instance.

  \since Coin 2.5
*/

/*!
  \fn SbVec4d::SbVec4d(const SbVec4i32 & v)

  Constructs an SbVec4d instance from an SbVec4i32 instance.

  \since Coin 2.5
*/

/*!
  \fn double SbVec4d::dot(const SbVec4d & v) const

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
SbVec4d::equals(const SbVec4d & v, double tolerance) const
{
#if COIN_DEBUG
  if(!(tolerance >= 0.0f))
    SoDebugError::postWarning("SbVec4d::equals","Tolerance should be >= 0.0f");
#endif // COIN_DEBUG

  double xdist = this->vec[0] - v[0];
  double ydist = this->vec[1] - v[1];
  double zdist = this->vec[2] - v[2];
  double wdist = this->vec[3] - v[3];

  if((xdist*xdist + ydist*ydist + zdist*zdist + wdist*wdist) <= tolerance)
    return TRUE;
  return FALSE;
}

/*!
  Returns the vector as a Cartesian 3D vector in \a v. This means that
  the 3 first components x, y and z will be divided by the fourth, w.
*/
void
SbVec4d::getReal(SbVec3d & v) const
{
#if COIN_DEBUG
  if(!(this->vec[3] != 0.0f))
    SoDebugError::postWarning("SbVec4d::getReal",
                              "The 4th vector component is zero => "
                              "division by zero");
#endif // COIN_DEBUG

  v.setValue(vec[0]/vec[3], vec[1]/vec[3], vec[2]/vec[3]);
}

/*!
  \fn const double * SbVec4d::getValue(void) const

  Returns a pointer to an array of four doubles containing the
  x, y, z and w coordinates of the vector.

  \sa setValue().
*/

/*!
  \fn void SbVec4d::getValue(double & x, double & y, double & z, double & w) const

  Returns the x, y, z and w coordinates of the vector.

  \sa setValue().
*/

/*!
  Return the length of the vector in 4D space.
*/
double
SbVec4d::length(void) const
{
  return static_cast<double>(sqrt(this->sqrLength()));
}

/*!
  \fn double SbVec4d::sqrLength(void) const

  Return the square of the length of the vector in 4D space.
*/

/*!
  \fn void SbVec4d::negate(void)
  Negate the vector.
*/

/*!
  Normalize the vector to unit length. Return value is the original
  length of the vector before normalization.
*/

double
SbVec4d::normalize(void)
{
  double len = this->length();

  if (len > 0.0) {
    operator/=(len);
  }
#if COIN_DEBUG
  else if (coin_debug_normalize()) {
    SoDebugError::postWarning("SbVec4d::normalize",
                              "The length of the vector should be > 0.0 "
                              "to be able to normalize.");
  }
#endif // COIN_DEBUG
  return len;
}

/*!
  \fn SbVec4d & SbVec4d::setValue(const double v[4])

  Set new coordinates for the vector from \a v. Returns reference to
  self.

  \sa getValue().
*/

/*!
  \fn SbVec4d & SbVec4d::setValue(double x, double y, double z, double w)

  Set new coordinates for the vector. Returns reference to self.

  \sa getValue().
*/

/*!
  \since Coin 2.5
*/

SbVec4d &
SbVec4d::setValue(const SbVec4f & v)
{
  vec[0] = static_cast<double>(v[0]);
  vec[1] = static_cast<double>(v[1]);
  vec[2] = static_cast<double>(v[2]);
  vec[3] = static_cast<double>(v[3]);
  return *this;
}

/*!
  \since Coin 2.5
*/

SbVec4d &
SbVec4d::setValue(const SbVec4b & v)
{
  vec[0] = static_cast<double>(v[0]);
  vec[1] = static_cast<double>(v[1]);
  vec[2] = static_cast<double>(v[2]);
  vec[3] = static_cast<double>(v[3]);
  return *this;
}

/*!
  \since Coin 2.5
*/

SbVec4d &
SbVec4d::setValue(const SbVec4s & v)
{
  vec[0] = static_cast<double>(v[0]);
  vec[1] = static_cast<double>(v[1]);
  vec[2] = static_cast<double>(v[2]);
  vec[3] = static_cast<double>(v[3]);
  return *this;
}

/*!
  \since Coin 2.5
*/

SbVec4d &
SbVec4d::setValue(const SbVec4i32 & v)
{
  vec[0] = static_cast<double>(v[0]);
  vec[1] = static_cast<double>(v[1]);
  vec[2] = static_cast<double>(v[2]);
  vec[3] = static_cast<double>(v[3]);
  return *this;
}

/*!
  \fn double & SbVec4d::operator [] (int i)

  Index operator. Returns modifiable x, y, z or w component of vector.

  \sa getValue() and setValue().
*/

/*!
  \fn const double & SbVec4d::operator [] (int i) const
  Index operator. Returns x, y, z or w component of vector.

  \sa getValue() and setValue().
*/

/*!
  \fn SbVec4d & SbVec4d::operator *= (double d)

  Multiply components of vector with value \a d. Returns reference to self.
*/

/*!
  \fn SbVec4d & SbVec4d::operator /= (double d)

  Divides components of vector with value \a d. Returns reference to self.
*/

/*!
  \fn SbVec4d & SbVec4d::operator += (const SbVec4d & v)

  Adds this vector and vector \a v. Returns reference to self.
*/

/*!
  \fn SbVec4d & SbVec4d::operator -= (const SbVec4d & v)

  Subtracts vector \a v from this vector. Returns reference to self.
*/

/*!
  \fn SbVec4d SbVec4d::operator - (void) const

  Non-destructive negation operator. Returns a new SbVec4d instance which
  has all components negated.

  \sa negate().
*/

/*!
  \fn SbVec4d operator *(const SbVec4d & v, double d)
  \relates SbVec4d

  Returns an SbVec4d instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec4d operator * (double d, const SbVec4d & v)
  \relates SbVec4d

  Returns an SbVec4d instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec4d operator / (const SbVec4d & v, double d)
  \relates SbVec4d

  Returns an SbVec4d instance which is the components of vector \a v
  divided on the scalar factor \a d.
*/

/*!
  \fn SbVec4d operator + (const SbVec4d & v1, const SbVec4d & v2)
  \relates SbVec4d

  Returns an SbVec4d instance which is the sum of vectors \a v1 and \a v2.
*/

/*!
  \fn SbVec4d operator - (const SbVec4d & v1, const SbVec4d & v2)
  \relates SbVec4d

  Returns an SbVec4d instance which is vector \a v2 subtracted from
  vector \a v1.
*/

/*!
  \fn int operator == (const SbVec4d & v1, const SbVec4d & v2)
  \relates SbVec4d

  Returns \a 1 if \a v1 and \a v2 are equal, \a 0 otherwise.

  \sa equals().
*/

/*!
  \fn int operator != (const SbVec4d & v1, const SbVec4d & v2)
  \relates SbVec4d

  Returns \a 1 if \a v1 and \a v2 are not equal, \a 0 if they are equal.

  \sa equals().
*/

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
 */
void
SbVec4d::print(FILE * fp) const
{
#if COIN_DEBUG
  fprintf( fp, "<%f, %f, %f, %f>", this->vec[0], this->vec[1], this->vec[2],
    this->vec[3] );
#endif // COIN_DEBUG
}
