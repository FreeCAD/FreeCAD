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

#include <Inventor/SbVec2us.h>

#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec2ub.h>
#include <Inventor/SbVec2ui32.h>

/*!
  \class SbVec2us SbVec2us.h Inventor/SbVec2us.h

  \since Coin 2.5
*/

SbVec2us &
SbVec2us::setValue(const SbVec2s & v)
{
  vec[0] = static_cast<unsigned short>(v[0]);
  vec[1] = static_cast<unsigned short>(v[1]);
  return *this;
}

SbVec2us &
SbVec2us::setValue(const SbVec2ub & v)
{
  vec[0] = static_cast<unsigned short>(v[0]);
  vec[1] = static_cast<unsigned short>(v[1]);
  return *this;
}

SbVec2us &
SbVec2us::setValue(const SbVec2ui32 & v)
{
  vec[0] = static_cast<unsigned short>(v[0]);
  vec[1] = static_cast<unsigned short>(v[1]);
  return *this;
}

void
SbVec2us::negate(void)
{
  vec[0] = static_cast<unsigned short>(-vec[0]);
  vec[1] = static_cast<unsigned short>(-vec[1]);
}

SbVec2us &
SbVec2us::operator *= (double d)
{
  vec[0] = static_cast<unsigned short>(vec[0] * d);
  vec[1] = static_cast<unsigned short>(vec[1] * d);
  return *this;
}
