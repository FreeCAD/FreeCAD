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

#include <Inventor/SbVec4ui32.h>

#include <Inventor/SbVec4i32.h>
#include <Inventor/SbVec4ub.h>
#include <Inventor/SbVec4us.h>

/*!
  \class SbVec4ui32 SbVec4ui32.h Inventor/SbVec4ui32.h

  \since Coin 2.5
*/

SbVec4ui32 &
SbVec4ui32::setValue(const SbVec4i32 & v)
{
  vec[0] = static_cast<uint32_t>(v[0]);
  vec[1] = static_cast<uint32_t>(v[1]);
  vec[2] = static_cast<uint32_t>(v[2]);
  vec[3] = static_cast<uint32_t>(v[3]);
  return *this;
}

SbVec4ui32 &
SbVec4ui32::setValue(const SbVec4ub & v)
{
  vec[0] = static_cast<uint32_t>(v[0]);
  vec[1] = static_cast<uint32_t>(v[1]);
  vec[2] = static_cast<uint32_t>(v[2]);
  vec[3] = static_cast<uint32_t>(v[3]);
  return *this;
}

SbVec4ui32 &
SbVec4ui32::setValue(const SbVec4us & v)
{
  vec[0] = static_cast<uint32_t>(v[0]);
  vec[1] = static_cast<uint32_t>(v[1]);
  vec[2] = static_cast<uint32_t>(v[2]);
  vec[3] = static_cast<uint32_t>(v[3]);
  return *this;
}

void
SbVec4ui32::negate(void)
{
  vec[0] = static_cast<uint32_t>(0-vec[0]);
  vec[1] = static_cast<uint32_t>(0-vec[1]);
  vec[2] = static_cast<uint32_t>(0-vec[2]);
  vec[3] = static_cast<uint32_t>(0-vec[3]);
}

SbVec4ui32 &
SbVec4ui32::operator *= (double d)
{
  vec[0] = static_cast<uint32_t>(vec[0] * d);
  vec[1] = static_cast<uint32_t>(vec[1] * d);
  vec[2] = static_cast<uint32_t>(vec[2] * d);
  vec[3] = static_cast<uint32_t>(vec[3] * d);
  return *this;
}
