#ifndef COIN_SOMFVEC4UB_H
#define COIN_SOMFVEC4UB_H

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

#include <Inventor/fields/SoMField.h>
#include <Inventor/fields/SoSubField.h>
#include <Inventor/SbVec4ub.h>

class COIN_DLL_API SoMFVec4ub : public SoMField {
  typedef SoMField inherited;

  SO_MFIELD_HEADER(SoMFVec4ub, SbVec4ub, SbVec4ub);

  SO_MFIELD_SETVALUESPOINTER_HEADER(SbVec4ub);
  SO_MFIELD_SETVALUESPOINTER_HEADER(uint8_t);

public:
  static void initClass(void);

  void setValues(int start, int num, const uint8_t xyzw[][4]);
  void set1Value(int idx, uint8_t x, uint8_t y, uint8_t z, uint8_t w);
  void set1Value(int idx, const uint8_t xyzw[4]);
  void setValue(uint8_t x, uint8_t y, uint8_t z, uint8_t w);
  void setValue(const uint8_t xyzw[4]);

}; // SoMFVec4ub

#endif // !COIN_SOMFVEC4UB_H
