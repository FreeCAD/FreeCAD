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

#ifndef COIN_SOVRMLSOUND_H
#define COIN_SOVRMLSOUND_H

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/SbTime.h>

class SoVRMLSoundP;
class SoPath;

class COIN_DLL_API SoVRMLSound : public SoNode
{
  typedef SoNode inherited;
  SO_NODE_HEADER(SoVRMLSound);

public:
  static void initClass(void);
  SoVRMLSound(void);

  SoSFNode source;
  SoSFFloat intensity;
  SoSFFloat priority;
  SoSFVec3f location;
  SoSFVec3f direction;
  SoSFFloat minFront;
  SoSFFloat maxFront;
  SoSFFloat minBack;
  SoSFFloat maxBack;
  SoSFBool spatialize;
  SoSFFloat dopplerFactor;
  SoSFFloat dopplerVelocity;

  void setDopplerVelocity(float velocity);
  float getDopplerVelocity(void) const;
  void setDopplerFactor(float factor);
  float getDopplerFactor(void) const;

  void startPlaying(SoPath *path, void *userdataptr);
  void stopPlaying(SoPath *path, void *userdataptr);

  static void setDefaultBufferingProperties(int bufferLength, int numBuffers, 
                                            SbTime sleepTime);
  void setBufferingProperties(int bufferLength, int numBuffers, 
                              SbTime sleepTime);
  void getBufferingProperties(int &bufferLength, int &numBuffers, 
                              SbTime &sleepTime);

  virtual void audioRender(SoAudioRenderAction *action);

protected:
  virtual ~SoVRMLSound(void);

private:
  SoVRMLSoundP *pimpl;
  friend class SoVRMLSoundP;
};

#endif // ! COIN_SOVRMLSOUND_H
