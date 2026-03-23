#ifndef COIN_SOPRIMITIVEVERTEX_H
#define COIN_SOPRIMITIVEVERTEX_H

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

#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/system/inttypes.h>

class SoDetail;

class COIN_DLL_API SoPrimitiveVertex {
public:
  SoPrimitiveVertex(void);
  SoPrimitiveVertex(const SoPrimitiveVertex & pv);
  ~SoPrimitiveVertex(void);

  SoPrimitiveVertex & operator = (const SoPrimitiveVertex & pv);

  const SbVec3f & getPoint(void) const { return point; }
  const SbVec3f & getNormal(void) const { return normal; }
  const SbVec4f & getTextureCoords(void) const { return textureCoords; }
  int getMaterialIndex(void) const { return materialIndex; }
  uint32_t getPackedColor(void) const { return packedColor; }
  const SoDetail * getDetail(void) const { return detail; }

  void setPoint(const SbVec3f & pt) { point = pt; }
  void setPoint(float x, float y, float z) { point.setValue(x, y, z); }
  void setNormal(const SbVec3f & n) { normal = n; }
  void setNormal(float nx, float ny, float nz) { normal.setValue(nx, ny, nz); }
  void setTextureCoords(const SbVec2f & tex) { textureCoords.setValue(tex[0], tex[1], 0.0f, 1.0f); }
  void setTextureCoords(float tx, float ty) { textureCoords.setValue(tx, ty, 0.0f, 1.0f); }
  void setTextureCoords(const SbVec3f & tex) { textureCoords.setValue(tex[0], tex[1], tex[2], 1.0f); }
  void setTextureCoords(float tx, float ty, float tz) { textureCoords.setValue(tx, ty, tz, 1.0f); }
  void setTextureCoords(const SbVec4f & tex) { textureCoords = tex; }
  void setTextureCoords(float tx, float ty, float tz, float tw) { textureCoords.setValue(tx, ty, tz, tw); }

  void setMaterialIndex(int index) { materialIndex = index; }
  void setPackedColor(uint32_t rgba) { packedColor = rgba; }
  void setDetail(SoDetail * d) { detail = d; }

private:
  SbVec3f point;
  SbVec3f normal;
  SbVec4f textureCoords;
  int materialIndex;
  SoDetail * detail;
  uint32_t packedColor;

}; // SoPrimitiveVertex

#endif // !COIN_SOPRIMITIVEVERTEX_H
