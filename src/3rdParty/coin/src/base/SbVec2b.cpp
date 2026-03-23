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

#include <Inventor/SbVec2b.h>

#include <limits>

#include <Inventor/SbVec2ub.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec2i32.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec2d.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  \class SbVec2b SbVec2b.h Inventor/SbVec2b.h
  \brief a vector class for containing two byte integers.

  \sa SbVec2ub
  \since Coin 2.5
*/

SbVec2b &
SbVec2b::setValue(const SbVec2ub & v)
{
  vec[0] = static_cast<int8_t>(v[0]);
  vec[1] = static_cast<int8_t>(v[1]);
  return *this;
}

SbVec2b &
SbVec2b::setValue(const SbVec2s & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<int8_t>::max() || v[0] < -std::numeric_limits<int8_t>::max() ||
      v[1] > std::numeric_limits<int8_t>::max() || v[1] < -std::numeric_limits<int8_t>::max()) {
    SoDebugError::post("SbVec2b::setValue", "SbVec2s argument out of range for SbVec2b");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<int8_t>(v[0]);
  vec[1] = static_cast<int8_t>(v[1]);
  return *this;
}

SbVec2b &
SbVec2b::setValue(const SbVec2i32 & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<int8_t>::max() || v[0] < -std::numeric_limits<int8_t>::max() ||
      v[1] > std::numeric_limits<int8_t>::max() || v[1] < -std::numeric_limits<int8_t>::max()) {
    SoDebugError::post("SbVec2b::setValue", "SbVec2i32 argument out of range for SbVec2b");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<int8_t>(v[0]);
  vec[1] = static_cast<int8_t>(v[1]);
  return *this;
}

SbVec2b &
SbVec2b::setValue(const SbVec2f & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<int8_t>::max() || v[0] < -std::numeric_limits<int8_t>::max() ||
      v[1] > std::numeric_limits<int8_t>::max() || v[1] < -std::numeric_limits<int8_t>::max()) {
    SoDebugError::post("SbVec2b::setValue", "SbVec2f argument out of range for SbVec2b");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<int8_t>(v[0]);
  vec[1] = static_cast<int8_t>(v[1]);
  return *this;
}

SbVec2b &
SbVec2b::setValue(const SbVec2d & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<int8_t>::max() || v[0] < -std::numeric_limits<int8_t>::max() ||
      v[1] > std::numeric_limits<int8_t>::max() || v[1] < -std::numeric_limits<int8_t>::max()) {
    SoDebugError::post("SbVec2b::setValue", "SbVec2d argument out of range for SbVec2b");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<int8_t>(v[0]);
  vec[1] = static_cast<int8_t>(v[1]);
  return *this;
}

SbVec2b &
SbVec2b::operator *= (double d)
{
  vec[0] = static_cast<int8_t>(vec[0] * d);
  vec[1] = static_cast<int8_t>(vec[1] * d);
  return *this;
}
