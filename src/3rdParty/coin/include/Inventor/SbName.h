#ifndef COIN_SBNAME_H
#define COIN_SBNAME_H

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

#include <Inventor/SbBasic.h>

class SbString;

class COIN_DLL_API SbName {
public:
  SbName(void);
  SbName(const char * namestring);
  SbName(const SbString & str);
  SbName(const SbName & name);
  ~SbName();

  const char * getString(void) const;
  int getLength(void) const;

  int operator!(void) const;
  friend COIN_DLL_API int operator==(const SbName & lhs, const char * rhs);
  friend COIN_DLL_API int operator==(const char * lhs, const SbName & rhs);
  friend COIN_DLL_API int operator==(const SbName & lhs, const SbName & rhs);
  friend COIN_DLL_API int operator!=(const SbName & lhs, const char * rhs);
  friend COIN_DLL_API int operator!=(const char * lhs, const SbName & rhs);
  friend COIN_DLL_API int operator!=(const SbName & lhs, const SbName & rhs);

  operator const char * (void) const;

  // The following 4 functions shouldn't be in this class, they are
  // related solely to .iv/.wrl file I/O (and so would likely be
  // better placed in SoOutput). This is a design flaw, but we can't
  // fix it without breaking API compatibility with the original SGI
  // Inventor.
  static SbBool isIdentStartChar(const char c);
  static SbBool isIdentChar(const char c);
  static SbBool isBaseNameStartChar(const char c);
  static SbBool isBaseNameChar(const char c);

  static const SbName & empty(void);

private:
  const char * permaaddress;
};

COIN_DLL_API int operator==(const SbName & lhs, const char * rhs);
COIN_DLL_API int operator==(const char * lhs, const SbName & rhs);
COIN_DLL_API int operator==(const SbName & lhs, const SbName & rhs);
COIN_DLL_API int operator!=(const SbName & lhs, const char * rhs);
COIN_DLL_API int operator!=(const char * lhs, const SbName & rhs);
COIN_DLL_API int operator!=(const SbName & lhs, const SbName & rhs);

#endif // !COIN_SBNAME_H
