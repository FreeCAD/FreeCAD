#ifndef COIN_SOGLYPH_H
#define COIN_SOGLYPH_H

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
#include <Inventor/SbBox2f.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/misc/SoState.h>

class SbName;
class SoGlyphP;

class COIN_DLL_API SoGlyph {
public:

  enum Fonttype {
    FONT2D = 1,
    FONT3D = 2
  };
  
  void unref(void) const;
  
  static const SoGlyph * getGlyph(const char character, const SbName & font);
  
  const SbVec2f * getCoords(void) const;
  const int * getFaceIndices(void) const;
  const int * getEdgeIndices(void) const;
  const int * getNextCWEdge(const int edgeidx) const;
  const int * getNextCCWEdge(const int edgeidx) const;

  float getWidth(void) const;
  const SbBox2f & getBoundingBox(void) const;
  
  static const SoGlyph * getGlyph(SoState * state,
                                  const unsigned int character, 
                                  const SbVec2s & size,
                                  const float angle);
  SbVec2s getAdvance(void) const;
  SbVec2s getKerning(const SoGlyph & rightglyph) const;
  unsigned char * getBitmap(SbVec2s & size, SbVec2s & pos, const SbBool antialiased) const;

protected:
  SoGlyph(void);
  ~SoGlyph();

#if (COIN_MAJOR_VERSION == 2)
#error Reminder: when copying this file over to Coin-2, next 3 functions
#error must be changed back to take a non-const first argument -- to keep
#error ABI-compatibility. Do not change any other interfaces because of this,
#error but handle by doing work-around casting internally in SoGlyph.cpp.
#endif
  void setCoords(const SbVec2f * coords, int numcoords = -1);
  void setFaceIndices(const int * indices, int numindices = -1);
  void setEdgeIndices(const int * indices, int numindices = -1);

private:
  static void unrefGlyph(SoGlyph * glyph);
  void setFontType(Fonttype type) const;


  friend class SoGlyphP;
  SoGlyphP * pimpl;
};

#endif // !COIN_SOGLYPH_H
