#ifndef COIN_SOTABPLANEDRAGGER_H
#define COIN_SOTABPLANEDRAGGER_H

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

#include <Inventor/draggers/SoDragger.h>
#include <Inventor/tools/SbLazyPimplPtr.h>
#include <Inventor/fields/SoSFVec3f.h>

class SoSensor;
class SoFieldSensor;
class SbLineProjector;
class SbPlaneProjector;
class SoTabPlaneDraggerP;

class COIN_DLL_API SoTabPlaneDragger : public SoDragger {
  typedef SoDragger inherited;

  SO_KIT_HEADER(SoTabPlaneDragger);
  SO_KIT_CATALOG_ENTRY_HEADER(cornerScaleCoords);
  SO_KIT_CATALOG_ENTRY_HEADER(cornerScaleTab0);
  SO_KIT_CATALOG_ENTRY_HEADER(cornerScaleTab1);
  SO_KIT_CATALOG_ENTRY_HEADER(cornerScaleTab2);
  SO_KIT_CATALOG_ENTRY_HEADER(cornerScaleTab3);
  SO_KIT_CATALOG_ENTRY_HEADER(edgeScaleCoords);
  SO_KIT_CATALOG_ENTRY_HEADER(edgeScaleTab0);
  SO_KIT_CATALOG_ENTRY_HEADER(edgeScaleTab1);
  SO_KIT_CATALOG_ENTRY_HEADER(edgeScaleTab2);
  SO_KIT_CATALOG_ENTRY_HEADER(edgeScaleTab3);
  SO_KIT_CATALOG_ENTRY_HEADER(planeSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(scaleTabHints);
  SO_KIT_CATALOG_ENTRY_HEADER(scaleTabMaterial);
  SO_KIT_CATALOG_ENTRY_HEADER(scaleTabMaterialBinding);
  SO_KIT_CATALOG_ENTRY_HEADER(scaleTabNormal);
  SO_KIT_CATALOG_ENTRY_HEADER(scaleTabNormalBinding);
  SO_KIT_CATALOG_ENTRY_HEADER(scaleTabs);
  SO_KIT_CATALOG_ENTRY_HEADER(translator);

public:
  static void initClass(void);
  SoTabPlaneDragger(void);

  SoSFVec3f translation;
  SoSFVec3f scaleFactor;

  void adjustScaleTabSize(void);

protected:
  virtual ~SoTabPlaneDragger(void);

  virtual void GLRender(SoGLRenderAction * action);

  virtual SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE);
  virtual void setDefaultOnNonWritingFields(void);

  void reallyAdjustScaleTabSize(SoGLRenderAction * action);

  void getXYScreenLengths(SbVec2f & lengths, const SbMatrix & localtoscreen,
                           const SbVec2s & winsize);

  void dragStart(void);
  void drag(void);
  void dragFinish(void);

  // Lots of public/protected methods and members were removed from
  // this class as they clearly should have been private.
  // Let us know if we've removed something that you need.
  // pederb, 20000226

private:

  // static methods moved from public to private
  static void startCB(void * f, SoDragger * d);
  static void motionCB(void * f, SoDragger * d);
  static void finishCB(void * f, SoDragger * d);
  static void metaKeyChangeCB(void * f, SoDragger * d);
  static void fieldSensorCB(void * f, SoSensor * s);
  static void valueChangedCB(void * f, SoDragger * d);

  void createPrivateParts(void);
  SoNode *getNodeFieldNode(const char *fieldname);

  SoFieldSensor * scaleFieldSensor;
  SoFieldSensor * translFieldSensor;
  SbLineProjector *lineProj;
  SbPlaneProjector *planeProj;
  int whatkind;
  int constraintState;
  float prevsizex;
  float prevsizey;
  SbBool adjustTabs;
  SbVec3f worldRestartPt;
  SbVec3f scaleCenter;

private:
  SbLazyPimplPtr<SoTabPlaneDraggerP> pimpl;

  // NOT IMPLEMENTED:
  SoTabPlaneDragger(const SoTabPlaneDragger & rhs);
  SoTabPlaneDragger & operator = (const SoTabPlaneDragger & rhs);
}; // SoTabPlaneDragger

#endif // !COIN_SOTABPLANEDRAGGER_H
