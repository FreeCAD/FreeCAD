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

#include <Inventor/SbVec4s.h>

#include <limits>

#include <Inventor/SbVec4us.h>
#include <Inventor/SbVec4b.h>
#include <Inventor/SbVec4i32.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/SbVec4d.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  \class SbVec4s SbVec4s.h Inventor/SbVec4s.h

  \since Coin 2.5
*/

SbVec4s &
SbVec4s::setValue(const SbVec4us & v)
{
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  vec[2] = static_cast<short>(v[2]);
  vec[3] = static_cast<short>(v[3]);
  return *this;
}

SbVec4s &
SbVec4s::setValue(const SbVec4b & v)
{
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  vec[2] = static_cast<short>(v[2]);
  vec[3] = static_cast<short>(v[3]);
  return *this;
}

SbVec4s &
SbVec4s::setValue(const SbVec4i32 & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<short>::max() || v[0] < -std::numeric_limits<short>::max() || 
      v[1] > std::numeric_limits<short>::max() || v[1] < -std::numeric_limits<short>::max() || 
      v[2] > std::numeric_limits<short>::max() || v[2] < -std::numeric_limits<short>::max()) {
    SoDebugError::post("SbVec4s::setValue", "SbVec4i32 argument out of range to store in an SbVec4s");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  vec[2] = static_cast<short>(v[2]);
  vec[3] = static_cast<short>(v[3]);
  return *this;
}

SbVec4s &
SbVec4s::setValue(const SbVec4f & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<short>::max() || v[0] < -std::numeric_limits<short>::max() || 
      v[1] > std::numeric_limits<short>::max() || v[1] < -std::numeric_limits<short>::max() || 
      v[2] > std::numeric_limits<short>::max() || v[2] < -std::numeric_limits<short>::max()) {
    SoDebugError::post("SbVec4s::setValue", "SbVec4f argument out of range to store in an SbVec4s");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  vec[2] = static_cast<short>(v[2]);
  vec[3] = static_cast<short>(v[3]);
  return *this;
}

SbVec4s &
SbVec4s::setValue(const SbVec4d & v)
{
#if COIN_DEBUG
  if (v[0] > std::numeric_limits<short>::max() || v[0] < -std::numeric_limits<short>::max() || 
      v[1] > std::numeric_limits<short>::max() || v[1] < -std::numeric_limits<short>::max() || 
      v[2] > std::numeric_limits<short>::max() || v[2] < -std::numeric_limits<short>::max()) {
    SoDebugError::post("SbVec4s::setValue", "SbVec4d argument out of range to store in an SbVec4s");
  }
#endif // COIN_DEBUG
  vec[0] = static_cast<short>(v[0]);
  vec[1] = static_cast<short>(v[1]);
  vec[2] = static_cast<short>(v[2]);
  vec[3] = static_cast<short>(v[3]);
  return *this;
}

SbVec4s &
SbVec4s::operator *= (double d)
{
  vec[0] = static_cast<short>(vec[0] * d);
  vec[1] = static_cast<short>(vec[1] * d);
  vec[2] = static_cast<short>(vec[2] * d);
  vec[3] = static_cast<short>(vec[3] * d); 
  return *this;
}
