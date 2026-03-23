#ifndef COIN_SOMFCOLORRGBA_H
#define COIN_SOMFCOLORRGBA_H

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
#include <Inventor/SbColor4f.h>

class COIN_DLL_API SoMFColorRGBA : public SoMField {
  typedef SoMField inherited;

  SO_MFIELD_HEADER(SoMFColorRGBA, SbColor4f, const SbColor4f &);

  SO_MFIELD_SETVALUESPOINTER_HEADER(float);
  SO_MFIELD_SETVALUESPOINTER_HEADER(SbColor4f);

public:
  static void initClass(void);

  void setValues(int start, int num, const float rgba[][4]);
  void setHSVValues(int start, int num, const float hsv[][4]);

  void setValue(const SbVec4f & vec);
  void setValue(float r, float g, float b, float a);
  void setValue(const float rgba[4]);

  void setHSVValue(float h, float s, float v, float a);
  void setHSVValue(const float hsva[4]);

  void set1Value(int idx, const SbVec4f & vec);
  void set1Value(int idx, float r, float g, float b, float a);
  void set1Value(int idx, const float rgba[4]);

  void set1HSVValue(int idx, float h, float s, float v, float a);
  void set1HSVValue(int idx, const float hsva[4]);

}; // SoMFColorRGBA

#endif // !COIN_SOMFCOLORRGBA_H
