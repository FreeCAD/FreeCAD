#ifndef COIN_SOIMAGE_H
#define COIN_SOIMAGE_H

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

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFImage.h>
#include <Inventor/fields/SoSFString.h>

class SoSensor;
class SoFieldSensor;
class SbImage;

class COIN_DLL_API SoImage : public SoShape {
  typedef SoShape inherited;

  SO_NODE_HEADER(SoImage);

public:
  static void initClass(void);
  SoImage(void);

  enum VertAlignment {
    BOTTOM,
    HALF,
    TOP
  };

  enum HorAlignment {
    LEFT,
    CENTER,
    RIGHT
  };

  SoSFInt32 width;
  SoSFInt32 height;
  SoSFEnum vertAlignment;
  SoSFEnum horAlignment;
  SoSFImage image;
  SoSFString filename;

  virtual void GLRender(SoGLRenderAction * action);
  virtual void rayPick(SoRayPickAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);

protected:
  virtual ~SoImage();

  virtual void generatePrimitives(SoAction * action);
  virtual void computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center);

  virtual SbBool readInstance(SoInput * in, unsigned short flags);
  virtual void notify(SoNotList * list);
  int getReadStatus(void);
  void setReadStatus(SbBool flag);

private:
  SbVec2s getSize(void) const;
  static SbVec3f getNilpoint(SoState *state);
  void getQuad(SoState *state, SbVec3f &v0, SbVec3f &v1,
               SbVec3f &v2, SbVec3f &v3);

  const unsigned char * getImage(SbVec2s & size, int & nc);
  SbBool loadFilename(void);
  SbBool readstatus;
  SbImage * resizedimage;
  SbBool resizedimagevalid;
  class SoFieldSensor * filenamesensor;
  SbBool transparency;
  SbBool testtransparency;
  void testTransparency(void);
  static void filenameSensorCB(void *, SoSensor *);
};

#endif // !COIN_SOIMAGE_H
