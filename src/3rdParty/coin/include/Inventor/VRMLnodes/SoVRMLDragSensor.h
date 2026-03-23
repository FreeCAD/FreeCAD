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

#ifndef COIN_SOVRMLDRAGSENSOR_H
#define COIN_SOVRMLDRAGSENSOR_H

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/VRMLnodes/SoVRMLSensor.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbViewVolume.h>

class SbMatrix;

class COIN_DLL_API SoVRMLDragSensor : public SoVRMLSensor {
  typedef SoVRMLSensor inherited;

  SO_NODE_ABSTRACT_HEADER(SoVRMLDragSensor);

public:
  SoSFVec3f trackPoint_changed;
  SoSFBool autoOffset;

  //  virtual void GLRender(SoGLRenderAction * action);
  virtual void handleEvent(SoHandleEventAction * action);
  static void initClass(void);

protected:
  
  virtual SbBool dragStart(void) = 0;
  virtual void drag(void) = 0;
  virtual void dragFinish(void) = 0;

  SoVRMLDragSensor(void);
  virtual ~SoVRMLDragSensor();

  const SbVec3f & getLocalStartingPoint(void) const;
  const SbMatrix & getLocalToWorldMatrix(void) const;
  const SbMatrix & getWorldToLocalMatrix(void) const;
  const SbViewVolume & getViewVolume(void) const;
  const SbVec2f & getNormalizedLocaterPosition(void) const;

private:
  // FIXME: move to private class
  SbVec3f hitpt;
  SbMatrix obj2world;
  SbMatrix world2obj;
  SbVec2s mousepos;
  SbVec2f normpos;
  SbViewVolume viewvolume;
};

#endif // ! COIN_SOVRMLDRAGSENSOR_H
