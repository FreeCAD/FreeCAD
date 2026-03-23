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

#ifndef COIN_SOVRMLIMAGETEXTURE_H
#define COIN_SOVRMLIMAGETEXTURE_H

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/VRMLnodes/SoVRMLTexture.h>
#include <Inventor/fields/SoMFString.h>
#include <Inventor/SbImage.h>

class SoVRMLImageTexture;
class SoSensor;
class SbImage;

typedef SbBool VRMLPrequalifyFileCallback(const SbString &, void *,
                                          SoVRMLImageTexture *);

class COIN_DLL_API SoVRMLImageTexture : public SoVRMLTexture
{
  typedef SoVRMLTexture inherited;
  SO_NODE_HEADER(SoVRMLImageTexture);

public:
  static void initClass(void);
  SoVRMLImageTexture(void);

  SoMFString url;

  static void setDelayFetchURL(const SbBool onoff);
  static void setPrequalifyFileCallBack(VRMLPrequalifyFileCallback * cb,
                                        void * closure);
  void allowPrequalifyFile(SbBool enable);

  virtual void doAction(SoAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void rayPick(SoRayPickAction * action);

  void setImage(const SbImage & image);
  const SbImage * getImage(void) const;

  static void setImageDataMaxAge(const uint32_t maxage);

protected:
  virtual ~SoVRMLImageTexture();

  virtual SbBool readInstance(SoInput * in, unsigned short flags);
  int getReadStatus(void) const;
  void setReadStatus(int status);

private:

  SbBool readImage(const SbString & filename);
  SbBool loadUrl(void);
  class SoVRMLImageTextureP * pimpl;
  static void urlSensorCB(void *, SoSensor *);
  static void glimage_callback(void * closure);
  static SbBool image_read_cb(const SbString &, SbImage *, void *);
  static void read_thread(void * closure);
  static SbBool default_prequalify_cb(const SbString & url,  void * closure, 
                                      SoVRMLImageTexture * node);
  static void oneshot_readimage_cb(void *, SoSensor *);

}; // class SoVRMLImageTexture

#endif // ! COIN_SOVRMLIMAGETEXTURE_H
