#ifndef COIN_SOOFFSCREENRENDERER_H
#define COIN_SOOFFSCREENRENDERER_H

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

#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbColor.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/lists/SbPList.h>
#include <Inventor/SbString.h>
#include <Inventor/SbName.h>

#include <cstdio>

class SoBase;
class SoGLRenderAction;
class SoNode;
class SoPath;

// This shouldn't strictly be necessary, but the OSF1/cxx compiler
// complains if this is left out, while using the "friend class
// SoExtSelectionP" statement in the class definition.
class SoOffscreenRendererP;


class COIN_DLL_API SoOffscreenRenderer {
public:
  enum Components {
    LUMINANCE = 1,
    LUMINANCE_TRANSPARENCY = 2,
    RGB = 3,
    RGB_TRANSPARENCY = 4
  };

  SoOffscreenRenderer(const SbViewportRegion & viewportregion);
  SoOffscreenRenderer(SoGLRenderAction * action);
  ~SoOffscreenRenderer();

  static float getScreenPixelsPerInch(void);
  static SbVec2s getMaximumResolution(void);
  void setComponents(const Components components);
  Components getComponents(void) const;
  void setViewportRegion(const SbViewportRegion & region);
  const SbViewportRegion & getViewportRegion(void) const;
  void setBackgroundColor(const SbColor & color);
  const SbColor & getBackgroundColor(void) const;
  void setGLRenderAction(SoGLRenderAction * action);
  SoGLRenderAction * getGLRenderAction(void) const;
  SbBool render(SoNode * scene);
  SbBool render(SoPath * scene);
  unsigned char * getBuffer(void) const;
  const void * const & getDC(void) const;

  SbBool writeToRGB(FILE * fp) const;
  SbBool writeToPostScript(FILE * fp) const;
  SbBool writeToPostScript(FILE * fp, const SbVec2f & printsize) const;

  SbBool writeToRGB(const char * filename) const;
  SbBool writeToPostScript(const char * filename) const;
  SbBool writeToPostScript(const char * filename, const SbVec2f & printsize) const;

  SbBool isWriteSupported(const SbName & filetypeextension) const;
  int getNumWriteFiletypes(void) const;
  void getWriteFiletypeInfo(const int idx,
                            SbPList & extlist,
                            SbString & fullname,
                            SbString & description);
  SbBool writeToFile(const SbString & filename, const SbName & filetypeextension) const;

  void setPbufferEnable(SbBool enable);
  SbBool getPbufferEnable(void) const;

private:
  friend class SoOffscreenRendererP;
  class SoOffscreenRendererP * pimpl;
};

#endif // !COIN_SOOFFSCREENRENDERER_H
