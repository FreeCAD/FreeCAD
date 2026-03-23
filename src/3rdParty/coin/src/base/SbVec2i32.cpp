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
  \class SbVec2i32 SbVec2i32.h Inventor/SbVec2i32.h
  \brief The SbVec2i32 class is a 2 dimensional vector with 32-bit signed 
  integer coordinates.

  \ingroup coin_base

  This vector class is used by many other classes in
  Coin. It provides storage for a vector in 2 dimensions
  as well as simple integer arithmetic operations.

  \sa SbVec2f, SbVec2d, SbVec3s, SbVec3f, SbVec3d, SbVec4f, SbVec4d.
*/

#include <Inventor/SbVec2i32.h>

#include <limits>
#include <cassert>

#include <Inventor/SbVec2ui32.h>
#include <Inventor/SbVec2b.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec2d.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  \fn SbVec2i32::SbVec2i32(void)

  The default constructor does nothing. The vector coordinates will be
  uninitialized until you do a setValue().
*/

/*!
  \fn SbVec2i32::SbVec2i32(const int32_t v[2])

  Constructs an SbVec2i32 instance with initial values from \a v.
*/

/*!
  \fn SbVec2i32::SbVec2i32(int32_t x, int32_t y)

  Constructs an SbVec2i32 instance with the initial vector endpoints from
  \a x and \a y.
*/

/*!
  \fn SbVec2i32::SbVec2i32(const SbVec2ui32 & v)
  
  \since Coin 2.5
*/

/*!
  \fn SbVec2i32::SbVec2i32(const SbVec2b & v)
  
  \since Coin 2.5
*/

/*!
  \fn SbVec2i32::SbVec2i32(const SbVec2s & v)
  
  \since Coin 2.5
*/

/*!
  \fn SbVec2i32::SbVec2i32(const SbVec2f & v)
  
  \since Coin 2.5
*/

/*!
  \fn SbVec2i32::SbVec2i32(const SbVec2d & v)
  
  \since Coin 2.5
*/

/*!
  \fn int32_t SbVec2i32::dot(const SbVec2i32 & v) const

  Calculates and returns the result of taking the dot product of this
  vector and \a v.
*/

/*!
  \fn const int32_t * SbVec2i32::getValue(void) const

  Returns a pointer to an array of two floats containing the x and y
  coordinates of the vector.

  \sa setValue().
*/

/*!
  \fn void SbVec2i32::getValue(int32_t & x, int32_t & y) const

  Returns the x and y coordinates of the vector.

  \sa setValue().
*/

/*!
  \fn void SbVec2i32::negate(void)

  Negate the vector (i.e. point it in the opposite direction).
*/

/*!
  \fn SbVec2i32 & SbVec2i32::setValue(const int32_t v[2])

  Set new x and y coordinates for the vector from \a v. Returns reference to
  self.

  \sa getValue().
*/

/*!
  \fn SbVec2i32 & SbVec2i32::setValue(int32_t x, int32_t y)

  Set new x and y coordinates for the vector. Returns reference to self.

  \sa getValue().
*/

/*!
  \since Coin 2.5
*/

SbVec2i32 &
SbVec2i32::setValue(const SbVec2ui32 & v)
{
  vec[0] = static_cast<int32_t>(v[0]);
  vec[1] = static_cast<int32_t>(v[1]);
  return *this;
}

/*!
  \since Coin 2.5
*/

SbVec2i32 &
SbVec2i32::setValue(const SbVec2b & v)
{
  vec[0] = static_cast<int32_t>(v[0]);
  vec[1] = static_cast<int32_t>(v[1]);
  return *this;
}

/*!
  \since Coin 2.5
*/

SbVec2i32 &
SbVec2i32::setValue(const SbVec2s & v)
{
  vec[0] = static_cast<int32_t>(v[0]);
  vec[1] = static_cast<int32_t>(v[1]);
  return *this;
}

/*!
  \since Coin 2.5
*/

SbVec2i32 &
SbVec2i32::setValue(const SbVec2f & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<int32_t>::max() || v[0] < -std::numeric_limits<int32_t>::max() ||
      v[1] > std::numeric_limits<int32_t>::max() || v[1] < -std::numeric_limits<int32_t>::max()) {
    SoDebugError::post("SbVec2b::setValue", "SbVec2f argument out of range for SbVec2i32");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<int32_t>(v[0]);
  vec[1] = static_cast<int32_t>(v[1]);
  return *this;
}

/*!
  \since Coin 2.5
*/

SbVec2i32 &
SbVec2i32::setValue(const SbVec2d & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<int32_t>::max() || v[0] < -std::numeric_limits<int32_t>::max() ||
      v[1] > std::numeric_limits<int32_t>::max() || v[1] < -std::numeric_limits<int32_t>::max()) {
    SoDebugError::post("SbVec2b::setValue", "SbVec2d argument out of range for SbVec2i32");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<int32_t>(v[0]);
  vec[1] = static_cast<int32_t>(v[1]);
  return *this;
}

/*!
  \fn int32_t & SbVec2i32::operator [] (int i)

  Index operator. Returns modifiable x or y coordinate.

  \sa getValue() and setValue().
*/

/*!
  \fn const int32_t & SbVec2i32::operator [] (int i) const

  Index operator. Returns x or y coordinate.

  \sa getValue().
*/

/*!
  \fn SbVec2i32 & SbVec2i32::operator *= (int d)

  Multiply components of vector with value \a d. Returns reference to self.
*/

/*!
  Multiply components of vector with value \a d. Returns reference to self.
*/

SbVec2i32 &
SbVec2i32::operator *= (double d)
{
  vec[0] = static_cast<int32_t>(vec[0] * d);
  vec[1] = static_cast<int32_t>(vec[1] * d);
  return *this;
}

/*!
  \fn SbVec2i32 & SbVec2i32::operator /= (int d)

  Divides components of vector with value \a d. Returns reference to self.
*/

/*!
  \fn SbVec2i32 & SbVec2i32::operator /= (double d)

  Divides components of vector with value \a d. Returns reference to self.
*/

/*!
  \fn SbVec2i32 & SbVec2i32::operator += (const SbVec2i32 & v)
  Adds this vector and vector \a v. Returns reference to self.
*/

/*!
  \fn SbVec2i32 & SbVec2i32::operator -= (const SbVec2i32 & v)

  Subtracts vector \a u from this vector. Returns reference to self.
*/

/*!
  \fn SbVec2i32 SbVec2i32::operator - (void) const

  Non-destructive negation operator. Returns a new SbVec2i32 instance which
  points in the opposite direction of this vector.

  \sa negate().
*/

/*!
  \fn SbVec2i32 operator * (const SbVec2i32 & v, int d)
  \relates SbVec2i32

  Returns an SbVec2i32 instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec2i32 operator * (const SbVec2i32 & v, double d)
  \relates SbVec2i32

  Returns an SbVec2i32 instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec2i32 operator * (int d, const SbVec2i32 & v)
  \relates SbVec2i32

  Returns an SbVec2i32 instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec2i32 operator * (double d, const SbVec2i32 & v)
  \relates SbVec2i32

  Returns an SbVec2i32 instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec2i32 operator / (const SbVec2i32 & v, int d)
  \relates SbVec2i32

  Returns an SbVec2i32 instance which is the components of vector \a v
  divided on \a d.
*/

/*!
  \fn SbVec2i32 operator / (const SbVec2i32 & v, double d)
  \relates SbVec2i32

  Returns an SbVec2i32 instance which is the components of vector \a v
  divided on \a d.
*/

/*!
  \fn SbVec2i32 operator + (const SbVec2i32 & v1, const SbVec2i32 & v2)
  \relates SbVec2i32

  Returns an SbVec2i32 instance which is the sum of vectors \a v1 and \a v2.
 */

/*!
  \fn SbVec2i32 operator - (const SbVec2i32 & v1, const SbVec2i32 & v2)
  \relates SbVec2i32

  Returns an SbVec2i32 instance which is vector \a v2 subtracted from
  vector \a v1.
*/

/*!
  \fn int operator == (const SbVec2i32 & v1, const SbVec2i32 & v2)
  \relates SbVec2i32

  Returns \a 1 if \a v1 and \a v2 are equal, \a 0 otherwise.
*/

/*!
  \fn int operator != (const SbVec2i32 & v1, const SbVec2i32 & v2)
  \relates SbVec2i32

  Returns \a 1 if \a v1 and \a v2 are not equal, \a 0 if they are equal.
*/

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
 */
void
SbVec2i32::print(FILE * fp) const
{
#if COIN_DEBUG
  fprintf( fp, "<%d, %d>", this->vec[0], this->vec[1] );
#endif // COIN_DEBUG
}
