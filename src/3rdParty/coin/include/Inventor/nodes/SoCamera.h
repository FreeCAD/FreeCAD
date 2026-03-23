#ifndef COIN_SOCAMERA_H
#define COIN_SOCAMERA_H

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
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFFloat.h>

#include <Inventor/SbVec3f.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbBox3f.h>

#define SO_ASPECT_SQUARE        1.0f
#define SO_ASPECT_VIDEO         (4.0f/3.0f)
#define SO_ASPECT_35mm_ACADEMY  1.371
#define SO_ASPECT_16mm          1.369
#define SO_ASPECT_35mm_FULL     1.33333
#define SO_ASPECT_70mm          2.287
#define SO_ASPECT_CINEMASCOPE   2.35
#define SO_ASPECT_HDTV          (16.0f/9.0f)
#define SO_ASPECT_PANAVISION    2.361
#define SO_ASPECT_35mm          (3.0f/2.0f)
#define SO_ASPECT_VISTAVISION   2.301

class SoPath;

class SoCameraP;

class COIN_DLL_API SoCamera : public SoNode {
  typedef SoNode inherited;

  SO_NODE_ABSTRACT_HEADER(SoCamera);

public:
  static void initClass(void);

  enum ViewportMapping {
    CROP_VIEWPORT_FILL_FRAME,
    CROP_VIEWPORT_LINE_FRAME,
    CROP_VIEWPORT_NO_FRAME,
    ADJUST_CAMERA,
    LEAVE_ALONE
  };

  SoSFEnum viewportMapping;
  SoSFVec3f position;
  SoSFRotation orientation;
  SoSFFloat aspectRatio;
  SoSFFloat nearDistance;
  SoSFFloat farDistance;
  SoSFFloat focalDistance;

  SbViewVolume getViewVolume(const SbViewportRegion & vp,
                             SbViewportRegion & resultvp, 
                             const SbMatrix & mm = SbMatrix::identity()) const;
  
  void pointAt(const SbVec3f & targetpoint);
  void pointAt(const SbVec3f & targetpoint, const SbVec3f & upvector);
  virtual void scaleHeight(float scalefactor) = 0;
  virtual SbViewVolume getViewVolume(float useaspectratio = 0.0f) const = 0;
  void viewAll(SoNode * const sceneroot, const SbViewportRegion & vpregion,
               const float slack = 1.0f);
  void viewAll(SoPath * const path, const SbViewportRegion & vpregion,
               const float slack = 1.0f);
  SbViewportRegion getViewportBounds(const SbViewportRegion & region) const;

  enum StereoMode {
    MONOSCOPIC,
    LEFT_VIEW,
    RIGHT_VIEW
  };

  void setStereoMode(StereoMode mode);
  StereoMode getStereoMode(void) const;

  void setStereoAdjustment(float adjustment);
  float getStereoAdjustment(void) const;
  void setBalanceAdjustment(float adjustment);
  float getBalanceAdjustment(void) const;

  virtual void doAction(SoAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void audioRender(SoAudioRenderAction *action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void getMatrix(SoGetMatrixAction * action);
  virtual void handleEvent(SoHandleEventAction * action);
  virtual void rayPick(SoRayPickAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);
  virtual void viewBoundingBox(const SbBox3f & box, float aspect,
                               float slack) = 0;
protected:
  SoCamera(void);
  virtual ~SoCamera();

  virtual void jitter(int numpasses, int curpass,
                      const SbViewportRegion & vpreg,
                      SbVec3f & jitteramount) const;

private:
  void getView(SoAction * action, SbViewVolume & resultvv,
               SbViewportRegion & resultvp,
               const SbBool considermodelmatrix = TRUE);

  void drawCroppedFrame(SoGLRenderAction * action,
                        const int viewportmapping,
                        const SbViewportRegion & oldvp,
                        const SbViewportRegion & newvp);

  void lookAt(const SbVec3f & dir, const SbVec3f & up);

  StereoMode stereomode;
  float stereoadjustment;
  float balanceadjustment;
private:
  SoCameraP *pimpl;
  friend class SoCameraP;
};

#endif // !COIN_SOCAMERA_H
