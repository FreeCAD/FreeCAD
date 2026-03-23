#ifndef COIN_SBCOLOR4F_H
#define COIN_SBCOLOR4F_H

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

#include <Inventor/system/inttypes.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/SbColor.h>

class SbVec4f;

class COIN_DLL_API SbColor4f : public SbVec4f {
public:
  SbColor4f(void);
  SbColor4f(const SbColor &rgb, const float alpha = 1.0f);
  SbColor4f(const SbVec4f& v);
  SbColor4f(const float* const rgba);
  SbColor4f(const float r, const float g, const float b, const float a = 1.0f);

  void setValue(const float r, const float g, const float b,
                const float a = 1.0f);
  void setValue(const float col[4]);
  const float *getValue() const;
  void getValue(float &r, float &g, float &b, float &a);

  SbColor4f& setRGB(const SbColor &col);
  void getRGB(SbColor &color);
  SbColor4f& setHSVValue(float h, float s, float v, float a = 1.0f);
  SbColor4f& setHSVValue(const float hsv[3], float alpha = 1.0f);
  void getHSVValue(float &h, float &s, float &v) const;
  void getHSVValue(float hsv[3]) const;
  SbColor4f& setPackedValue(const uint32_t rgba);
  uint32_t getPackedValue() const;

  float operator[](const int idx) const;
  float &operator[](const int idx);

  SbColor4f &operator*=(const float d);
  SbColor4f &operator/=(const float d);
  SbColor4f &operator+=(const SbColor4f &c);
  SbColor4f &operator-=(const SbColor4f &c);

  friend COIN_DLL_API SbColor4f operator *(const SbColor4f &c, const float d);
  friend COIN_DLL_API SbColor4f operator *(const float d, const SbColor4f &c);
  friend COIN_DLL_API SbColor4f operator /(const SbColor4f &c, const float d);
  friend COIN_DLL_API SbColor4f operator +(const SbColor4f &v1, const SbColor4f &v2);
  friend COIN_DLL_API SbColor4f operator -(const SbColor4f &v1, const SbColor4f &v2);
  friend COIN_DLL_API int operator ==(const SbColor4f &v1, const SbColor4f &v2);
  friend COIN_DLL_API int operator !=(const SbColor4f &v1, const SbColor4f &v2);

private:
  float red() const { return this->vec[0]; }
  float green() const { return this->vec[1]; }
  float blue() const { return this->vec[2]; }
  float alpha() const { return this->vec[3]; }
};

COIN_DLL_API SbColor4f operator *(const SbColor4f &c, const float d);
COIN_DLL_API SbColor4f operator *(const float d, const SbColor4f &c);
COIN_DLL_API SbColor4f operator /(const SbColor4f &c, const float d);
COIN_DLL_API SbColor4f operator +(const SbColor4f &v1, const SbColor4f &v2);
COIN_DLL_API SbColor4f operator -(const SbColor4f &v1, const SbColor4f &v2);
COIN_DLL_API int operator ==(const SbColor4f &v1, const SbColor4f &v2);
COIN_DLL_API int operator !=(const SbColor4f &v1, const SbColor4f &v2);

typedef class SbColor4f SbColorRGBA; // TGS compatibility

#endif // !COIN_SBCOLOR4F_H
