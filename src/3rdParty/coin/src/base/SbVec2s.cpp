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
  \class SbVec2s SbVec2s.h Inventor/SbVec2s.h
  \brief The SbVec2s class is a 2 dimensional vector with short integer 
  coordinates.

  \ingroup coin_base

  This vector class is used by many other classes in
  Coin. It provides storage for a vector in 2 dimensions
  as well as simple integer arithmetic operations.

  \sa SbVec2f, SbVec2d, SbVec3s, SbVec3f, SbVec3d, SbVec4f, SbVec4d.
*/

#include <Inventor/SbVec2s.h>

#include <limits>
#include <cassert>

#include <Inventor/SbVec2us.h>
#include <Inventor/SbVec2b.h>
#include <Inventor/SbVec2i32.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec2d.h>
#include <Inventor/fields/SoSFVec2s.h>

#include "coinString.h"
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  \fn SbVec2s::SbVec2s(void)

  The default constructor does nothing. The vector coordinates will be
  uninitialized until you do a setValue().
*/

/*!
  \fn SbVec2s::SbVec2s(const short v[2])

  Constructs an SbVec2s instance with initial values from \a v.
*/

/*!
  \fn SbVec2s::SbVec2s(short x, short y)

  Constructs an SbVec2s instance with the initial vector endpoints from
  \a x and \a y.
*/

/*!
  \fn SbVec2s::SbVec2s(const SbVec2us & v)

  Constructs an SbVec2s instance from the value in an SbVec2us instance.

  \since Coin 2.5
*/

/*!
  \fn SbVec2s::SbVec2s(const SbVec2b & v)

  Constructs an SbVec2s instance from the value in an SbVec2b instance.

  \since Coin 2.5
*/

/*!
  \fn SbVec2s::SbVec2s(const SbVec2i32 & v)

  Constructs an SbVec2s instance from the value in an SbVec2i32 instance.

  \since Coin 2.5
*/

/*!
  \fn SbVec2s::SbVec2s(const SbVec2f & v)

  Constructs an SbVec2s instance from the value in an SbVec2f instance.

  \since Coin 2.5
*/

/*!
  \fn SbVec2s::SbVec2s(const SbVec2d & v)

  Constructs an SbVec2s instance from the value in an SbVec2d instance.

  \since Coin 2.5
*/

/*!
  \fn int32_t SbVec2s::dot(const SbVec2s & v) const

  Calculates and returns the result of taking the dot product of this
  vector and \a v.
*/

/*!
  \fn const short * SbVec2s::getValue(void) const

  Returns a pointer to an array of two floats containing the x and y
  coordinates of the vector.

  \sa setValue().
*/

/*!
  \fn void SbVec2s::getValue(short & x, short & y) const

  Returns the x and y coordinates of the vector.

  \sa setValue().
*/

/*!
  \fn void SbVec2s::negate(void)

  Negate the vector (i.e. point it in the opposite direction).
*/

/*!
  \fn SbVec2s & SbVec2s::setValue(const short v[2])

  Set new x and y coordinates for the vector from \a v. Returns reference to
  self.

  \sa getValue().
*/

/*!
  \fn SbVec2s & SbVec2s::setValue(short x, short y)

  Set new x and y coordinates for the vector. Returns reference to self.

  \sa getValue().
*/

/*!
  \since Coin 2.5
*/

SbVec2s &
SbVec2s::setValue(const SbVec2us & v)
{
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  return *this;
}

/*!
  \since Coin 2.5
*/

SbVec2s &
SbVec2s::setValue(const SbVec2b & v)
{
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  return *this;
}

/*!
  \since Coin 2.5
*/

SbVec2s &
SbVec2s::setValue(const SbVec2i32 & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<short>::max() || v[0] < -std::numeric_limits<short>::max() ||
      v[1] > std::numeric_limits<short>::max() || v[1] < -std::numeric_limits<short>::max()) {
    SoDebugError::post("SbVec2s::setValue", "SbVec2i32 argument out of range for SbVec2s");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  return *this;
}

/*!
  \since Coin 2.5
*/

SbVec2s &
SbVec2s::setValue(const SbVec2f & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<short>::max() || v[0] < -std::numeric_limits<short>::max() ||
      v[1] > std::numeric_limits<short>::max() || v[1] < -std::numeric_limits<short>::max()) {
    SoDebugError::post("SbVec2s::setValue", "SbVec2f argument out of range for SbVec2s");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  return *this;
}

/*!
  \since Coin 2.5
*/

SbVec2s &
SbVec2s::setValue(const SbVec2d & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<short>::max() || v[0] < -std::numeric_limits<short>::max() ||
      v[1] > std::numeric_limits<short>::max() || v[1] < -std::numeric_limits<short>::max()) {
    SoDebugError::post("SbVec2s::setValue", "SbVec2d argument out of range for SbVec2s");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  return *this;
}

/*!
  \fn short & SbVec2s::operator [] (int i)

  Index operator. Returns modifiable x or y coordinate.

  \sa getValue() and setValue().
*/

/*!
  \fn const short & SbVec2s::operator [](int i) const

  Index operator. Returns x or y coordinate.

  \sa getValue().
*/

/*!
  \fn SbVec2s & SbVec2s::operator *= (int d)

  Multiply components of vector with value \a d. Returns reference to self.
*/

/*!
  Multiply components of vector with value \a d. Returns reference to self.
*/

SbVec2s &
SbVec2s::operator *= (double d)
{
  vec[0] = static_cast<short>(vec[0] * d);
  vec[1] = static_cast<short>(vec[1] * d);
  return *this;
}

/*!
  \fn SbVec2s & SbVec2s::operator /= (int d)

  Divides components of vector with value \a d. Returns reference to self.
*/


/*!
  \fn SbVec2s & SbVec2s::operator /= (double d)

  Divides components of vector with value \a d. Returns reference to self.
*/

/*!
  \fn SbVec2s & SbVec2s::operator += (const SbVec2s & v)

  Adds this vector and vector \a v. Returns reference to self.
*/

/*!
  \fn SbVec2s & SbVec2s::operator -= (const SbVec2s & v)

  Subtracts vector \a v from this vector. Returns reference to self.
*/

/*!
  \fn SbVec2s SbVec2s::operator - (void) const

  Non-destructive negation operator. Returns a new SbVec2s instance which
  points in the opposite direction of this vector.

  \sa negate().
*/

/*!
  \fn SbVec2s operator * (const SbVec2s & v, int d)
  \relates SbVec2s

  Returns an SbVec2s instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec2s operator * (const SbVec2s & v, double d)
  \relates SbVec2s

  Returns an SbVec2s instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec2s operator * (int d, const SbVec2s & v)
  \relates SbVec2s

  Returns an SbVec2s instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec2s operator * (double d, const SbVec2s & v)
  \relates SbVec2s

  Returns an SbVec2s instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec2s operator / (const SbVec2s & v, int d)
  \relates SbVec2s

  Returns an SbVec2s instance which is the components of vector \a v
  divided on \a d.
*/

/*!
  \fn SbVec2s operator / (const SbVec2s & v, double d)
  \relates SbVec2s

  Returns an SbVec2s instance which is the components of vector \a v
  divided on \a d.
*/

/*!
  \fn SbVec2s operator + (const SbVec2s & v1, const SbVec2s & v2)
  \relates SbVec2s

  Returns an SbVec2s instance which is the sum of vectors \a v1 and \a v2.
*/

/*!
  \fn SbVec2s operator - (const SbVec2s & v1, const SbVec2s & v2)
  \relates SbVec2s

  Returns an SbVec2s instance which is vector \a v2 subtracted from
  vector \a v1.
*/

/*!
  \fn int operator == (const SbVec2s & v1, const SbVec2s & v2)
  \relates SbVec2s

  Returns \a 1 if \a v1 and \a v2 are equal, \a 0 otherwise.
*/

/*!
  \fn int operator != (const SbVec2s & v1, const SbVec2s & v2)
  \relates SbVec2s

  Returns \a 1 if \a v1 and \a v2 are not equal, \a 0 if they are equal.
*/

/*!
  Return a string representation of this object
*/
SbString
SbVec2s::toString() const
{
  return CoinInternal::ToString(*this);
}

/*!
  Convert from a string representation, return whether this is a valid conversion
*/
SbBool
SbVec2s::fromString(const SbString & str)
{
  SbBool conversionOk;
  *this = CoinInternal::FromString<SbVec2s>(str,&conversionOk);
  return conversionOk;
}

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
*/
void
SbVec2s::print(FILE * fp) const
{
#if COIN_DEBUG
  fputs(this->toString().getString(),fp);
#endif // COIN_DEBUG
}
