#ifndef COIN_SOTRANSFORMMANIP_H
#define COIN_SOTRANSFORMMANIP_H

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

#include <Inventor/nodes/SoTransform.h>
#include <Inventor/tools/SbLazyPimplPtr.h>
#ifndef COIN_INTERNAL
#include <Inventor/draggers/SoDragger.h>
#endif // !COIN_INTERNAL

class SoChildList;
class SoDragger;
class SoFieldSensor;
class SoPath;
class SoSensor;
class SoTransformManipP;

class COIN_DLL_API SoTransformManip : public SoTransform {
  typedef SoTransform inherited;

  SO_NODE_HEADER(SoTransformManip);

public:
  static void initClass(void);
  SoTransformManip(void);

  SoDragger * getDragger(void);
  SbBool replaceNode(SoPath * p);
  SbBool replaceManip(SoPath * p, SoTransform * newone) const;

  virtual void doAction(SoAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void getMatrix(SoGetMatrixAction * action);
  virtual void handleEvent(SoHandleEventAction * action);
  virtual void pick(SoPickAction * action);
  virtual void search(SoSearchAction * action);

  virtual SoChildList * getChildren(void) const;

protected:
  virtual ~SoTransformManip(void);

  void setDragger(SoDragger * newdragger);

  virtual void copyContents(const SoFieldContainer * fromfc,
                            SbBool copyconnections);

  static void transferFieldValues(const SoTransform * from, SoTransform * to);
  static void valueChangedCB(void * f, SoDragger * d);
  static void fieldSensorCB(void * f, SoSensor * d);

  SoFieldSensor * rotateFieldSensor;
  SoFieldSensor * translFieldSensor;
  SoFieldSensor * scaleFieldSensor;
  SoFieldSensor * centerFieldSensor;
  SoFieldSensor * scaleOrientFieldSensor;
  SoChildList * children;

  void attachSensors(const SbBool onoff);

private:
  SbLazyPimplPtr<SoTransformManipP> pimpl;

  // NOT IMPLEMENTED:
  SoTransformManip(const SoTransformManip & rhs);
  SoTransformManip & operator = (const SoTransformManip & rhs);
}; // SoTransformManip

#endif // !COIN_SOTRANSFORMMANIP_H
