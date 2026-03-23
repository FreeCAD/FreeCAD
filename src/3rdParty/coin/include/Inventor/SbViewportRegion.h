#ifndef COIN_SBVIEWPORTREGION_H
#define COIN_SBVIEWPORTREGION_H

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

#include <cstdio>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec2f.h>

class COIN_DLL_API SbViewportRegion {
public:
  SbViewportRegion(void);
  SbViewportRegion(short width, short height);
  SbViewportRegion(SbVec2s winSize);
  SbViewportRegion(const SbViewportRegion & vpReg);

  void setWindowSize(short width, short height);
  void setWindowSize(SbVec2s winSize);
  void setViewport(float left, float bottom,
                   float width, float height);
  void setViewport(SbVec2f origin, SbVec2f size);
  void setViewportPixels(short left, short bottom,
                         short width, short height);
  void setViewportPixels(SbVec2s origin, SbVec2s size);
  const SbVec2s & getWindowSize(void) const;
  const SbVec2f & getViewportOrigin(void) const;
  const SbVec2s & getViewportOriginPixels(void) const;
  const SbVec2f & getViewportSize(void) const;
  const SbVec2s & getViewportSizePixels(void) const;
  float getViewportAspectRatio(void) const;
  void scaleWidth(float ratio);
  void scaleHeight(float ratio);
  void setPixelsPerInch(float ppi);
  float getPixelsPerInch(void) const;
  float getPixelsPerPoint(void) const;
  friend COIN_DLL_API int operator ==(const SbViewportRegion & reg1,
                                      const SbViewportRegion & reg2);

  friend COIN_DLL_API int operator !=(const SbViewportRegion & reg1,
                                      const SbViewportRegion & reg2);

  void print(FILE * file) const;

private:
  SbVec2s winsize;
  SbVec2f vporigin;
  SbVec2f vpsize;
  SbVec2s vporigin_s;
  SbVec2s vpsize_s;
  float pixperinch;
};

COIN_DLL_API int operator ==(const SbViewportRegion & reg1, const SbViewportRegion & reg2);
COIN_DLL_API int operator !=(const SbViewportRegion & reg1, const SbViewportRegion & reg2);

#endif // !COIN_SBVIEWPORTREGION_H
