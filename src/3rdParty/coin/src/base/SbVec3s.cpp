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

#include <Inventor/SbVec3s.h>

#include <limits>
#include <cassert>

#include <Inventor/SbVec3us.h>
#include <Inventor/SbVec3b.h>
#include <Inventor/SbVec3i32.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec3d.h>
#include <Inventor/fields/SoSFVec3s.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "coinString.h"

/*!
  \class SbVec3s SbVec3s.h Inventor/SbVec3s.h
  \brief The SbVec3s class is a 3 dimensional vector with short integer
  coordinates.

  \ingroup coin_base

  This vector class provides storage for a 3 dimensional vector
  as well as simple integer arithmetic operations.

  \COIN_CLASS_EXTENSION

  \sa SbVec2s, SbVec2f, SbVec2d, SbVec3f, SbVec3d, SbVec4f, SbVec4d.
  \since Coin 2.0
  \since TGS Inventor ?.?
*/

/*!
  \fn SbVec3s::SbVec3s(void)

  The default constructor does nothing. The vector coordinates will be
  uninitialized until you do a setValue() call.
*/

/*!
  \fn SbVec3s::SbVec3s(const short v[3])

  Constructs an SbVec3s instance with initial values from \a v.
*/

/*!
  \fn SbVec3s::SbVec3s(short x, short y, short z)

  Constructs an SbVec3s instance with the initial vector endpoint set to
  \a <x,y,z>.
*/

/*!
  \fn SbVec3s::SbVec3s(const SbVec3us & v)

  \since Coin 2.5
*/

/*!
  \fn SbVec3s::SbVec3s(const SbVec3b & v)

  \since Coin 2.5
*/

/*!
  \fn SbVec3s::SbVec3s(const SbVec3i32 & v)

  \since Coin 2.5
*/

/*!
  \fn SbVec3s::SbVec3s(const SbVec3f & v)

  \since Coin 2.5
*/

/*!
  \fn SbVec3s::SbVec3s(const SbVec3d & v)

  \since Coin 2.5
*/

/*!
  \fn int32_t SbVec3s::dot(const SbVec3s & v) const

  Calculates and returns the result of taking the dot product of this
  vector and \a v.
*/

/*!
  \fn const short * SbVec3s::getValue(void) const

  Returns a pointer to an array of three shorts containing the x, y
  and z coordinates of the vector.

  \sa setValue().
*/

/*!
  \fn void SbVec3s::getValue(short & x, short & y, short & z) const

  Returns the x, y and z coordinates of the vector.

  \sa setValue().
*/

/*!
  \fn void SbVec3s::negate(void)

  Negate the vector (i.e. point it in the opposite direction).
*/

/*!
  \fn SbVec3s & SbVec3s::setValue(const short v[3])

  Set new coordinates for the vector from \a v. Returns reference to
  self.

  \sa getValue().
*/

/*!
  \fn SbVec3s & SbVec3s::setValue(short x, short y, short z)

  Set new coordinates for the vector. Returns reference to self.

  \sa getValue().
*/

/*!
  Sets the value from an SbVec3us instance.

  \since Coin 2.5
*/

SbVec3s &
SbVec3s::setValue(const SbVec3us & v)
{
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  vec[2] = static_cast<short>(v[2]);
  return *this;
}

/*!
  Sets the value from an SbVec3b instance.

  \since Coin 2.5
*/

SbVec3s &
SbVec3s::setValue(const SbVec3b & v)
{
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  vec[2] = static_cast<short>(v[2]);
  return *this;
}

/*!
  Sets the value from an SbVec3i32 instance.

  \since Coin 2.5
*/

SbVec3s &
SbVec3s::setValue(const SbVec3i32 & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<short>::max() || v[0] < -std::numeric_limits<short>::max() ||
      v[1] > std::numeric_limits<short>::max() || v[1] < -std::numeric_limits<short>::max() ||
      v[2] > std::numeric_limits<short>::max() || v[2] < -std::numeric_limits<short>::max()) {
    SoDebugError::post("SbVec3s::setValue", "SbVec3i32 argument out of range for SbVec3s");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  vec[2] = static_cast<short>(v[2]);
  return *this;
}

/*!
  Sets the value from an SbVec3f instance.

  \since Coin 2.5
*/

SbVec3s &
SbVec3s::setValue(const SbVec3f & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<short>::max() || v[0] < -std::numeric_limits<short>::max() ||
      v[1] > std::numeric_limits<short>::max() || v[1] < -std::numeric_limits<short>::max() ||
      v[2] > std::numeric_limits<short>::max() || v[2] < -std::numeric_limits<short>::max()) {
    SoDebugError::post("SbVec3s::setValue", "SbVec3f argument out of range for SbVec3s");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  vec[2] = static_cast<short>(v[2]);
  return *this;
}

/*!
  Sets the value from an SbVec3d instance.

  \since Coin 2.5
*/

SbVec3s &
SbVec3s::setValue(const SbVec3d & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<short>::max() || v[0] < -std::numeric_limits<short>::max() ||
      v[1] > std::numeric_limits<short>::max() || v[1] < -std::numeric_limits<short>::max() ||
      v[2] > std::numeric_limits<short>::max() || v[2] < -std::numeric_limits<short>::max()) {
    SoDebugError::post("SbVec3s::setValue", "SbVec3d argument out of range for SbVec3s");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  vec[2] = static_cast<short>(v[2]);
  return *this;
}

/*!
  \fn short & SbVec3s::operator[](const int i)

  Index operator. Returns modifiable x, y or z coordinate of vector.

  \sa getValue() and setValue().
*/

/*!
  \fn short SbVec3s::operator[](const int i) const
  Index operator. Returns x, y or z coordinate of vector.

  \sa getValue() and setValue().
*/

/*!
  \fn SbVec3s & SbVec3s::operator *= (int d)
  Multiply components of vector with value \a d. Returns reference to self.
*/

/*!
  Multiply components of vector with value \a d. Returns reference to self.
*/

SbVec3s &
SbVec3s::operator *=(double d)
{
  vec[0] = static_cast<short>(vec[0] * d);
  vec[1] = static_cast<short>(vec[1] * d);
  vec[2] = static_cast<short>(vec[2] * d);
  return *this;
}

/*!
  \fn SbVec3s & SbVec3s::operator /= (int d)

  Divides components of vector with value \a d. Returns reference to self.
*/

/*!
  \fn SbVec3s & SbVec3s::operator /= (double d)

  Divides components of vector with value \a d. Returns reference to self.
*/

/*!
  \fn SbVec3s & SbVec3s::operator += (const SbVec3s & v)

  Adds this vector and vector \a v. Returns reference to self.
*/

/*!
  \fn SbVec3s & SbVec3s::operator -= (const SbVec3s & v)
  Subtracts vector \a v from this vector. Returns reference to self.
*/

/*!
  \fn SbVec3s SbVec3s::operator-(void) const

  Non-destructive negation operator. Returns a new SbVec3s instance
  which points in the opposite direction of this vector.

  \sa negate().
*/

/*!
  \fn SbVec3s operator * (const SbVec3s & v, int d)
  \relates SbVec3s

  Returns an SbVec3s instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec3s operator * (const SbVec3s & v, double d)
  \relates SbVec3s

  Returns an SbVec3s instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec3s operator * (int d, const SbVec3s & v)
  \relates SbVec3s

  Returns an SbVec3s instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec3s operator * (double d, const SbVec3s & v)
  \relates SbVec3s

  Returns an SbVec3s instance which is the components of vector \a v
  multiplied with \a d.
*/

/*!
  \fn SbVec3s operator / (const SbVec3s & v, int d)
  \relates SbVec3s

  Returns an SbVec3s instance which is the components of vector \a v
  divided on the scalar factor \a d.
*/

/*!
  \fn SbVec3s operator / (const SbVec3s & v, double d)
  \relates SbVec3s

  Returns an SbVec3s instance which is the components of vector \a v
  divided on \a d.
*/

/*!
  \fn SbVec3s operator + (const SbVec3s & v1, const SbVec3s & v2)
  \relates SbVec3s

  Returns an SbVec3s instance which is the sum of vectors \a v1 and \a v2.
*/

/*!
  \fn SbVec3s operator - (const SbVec3s & v1, const SbVec3s & v2)
  \relates SbVec3s

  Returns an SbVec3s instance which is vector \a v2 subtracted from
  vector \a v1.
*/

/*!
  \fn int operator == (const SbVec3s & v1, const SbVec3s & v2)
  \relates SbVec3s

  Returns \a 1 if \a v1 and \a v2 are \e exactly equal, \a 0 otherwise.
*/

/*!
  \fn int operator != (const SbVec3s & v1, const SbVec3s & v2)
  \relates SbVec3s

  Returns \a 1 if \a v1 and \a v2 are not equal, \a 0 if they are equal.
*/

/*!
  Return a string representation of this object
*/
SbString
SbVec3s::toString() const
{
  return CoinInternal::ToString(*this);
}

/*!
  Convert from a string representation, return whether this is a valid conversion
*/
SbBool
SbVec3s::fromString(const SbString & str)
{
  SbBool conversionOk;
  *this = CoinInternal::FromString<SbVec3s>(str,&conversionOk);
  return conversionOk;
}

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
*/
void
SbVec3s::print(FILE * fp) const
{
#if COIN_DEBUG
  SbString str = "<";
  str+=this->toString();
  str += ">";
  fputs(str.getString(),fp);
#endif // COIN_DEBUG
}

#ifdef COIN_TEST_SUITE
#include <Inventor/SbTypeInfo.h>

typedef SbVec3s ToTest;
BOOST_AUTO_TEST_CASE(toString) {
  ToTest val(1,2,3);
  SbString str("1 2 3");
  BOOST_CHECK_MESSAGE(str == val.toString(),
                      std::string("Mismatch between ") +  val.toString().getString() + " and control string " + str.getString());
}

BOOST_AUTO_TEST_CASE(fromString) {
  ToTest foo;
  SbString test = "1 -2 3";
  ToTest trueVal(1,-2,3);
  foo.fromString(test);
  BOOST_CHECK_MESSAGE(trueVal == foo,
                      std::string("Mismatch between ") +  foo.toString().getString() + " and control " + trueVal.toString().getString());
}

BOOST_AUTO_TEST_CASE(fromInvalidString) {
  ToTest foo;
  SbString test = "a,2,3";
  SbBool conversionOk = foo.fromString(test);
  BOOST_CHECK_MESSAGE(conversionOk == FALSE,
                      std::string("Able to convert from ") + test.getString() + " which is not a valid " + SbTypeInfo<ToTest>::getTypeName() + " representation");
}

#endif //COIN_TEST_SUITE
