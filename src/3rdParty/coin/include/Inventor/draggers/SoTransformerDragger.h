#ifndef COIN_SOTRANSFORMERDRAGGER_H
#define COIN_SOTRANSFORMERDRAGGER_H

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
#include <Inventor/tools/SbPimplPtr.h>
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/lists/SoNodeList.h>

class SoSensor;
class SoFieldSensor;
class SoTransformerDraggerP;

class COIN_DLL_API SoTransformerDragger : public SoDragger {
  typedef SoDragger inherited;

  SO_KIT_HEADER(SoTransformerDragger);

  SO_KIT_CATALOG_ENTRY_HEADER(axisFeedbackLocation);
  SO_KIT_CATALOG_ENTRY_HEADER(axisFeedbackSep);
  SO_KIT_CATALOG_ENTRY_HEADER(circleFeedbackAntiSquish);
  SO_KIT_CATALOG_ENTRY_HEADER(circleFeedbackSep);
  SO_KIT_CATALOG_ENTRY_HEADER(circleFeedbackTransform);
  SO_KIT_CATALOG_ENTRY_HEADER(circleFeedbackTransformSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(negXRoundWallFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(negXWallFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(negXWallFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(negYRoundWallFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(negYWallFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(negYWallFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(negZRoundWallFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(negZWallFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(negZWallFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(overallStyle);
  SO_KIT_CATALOG_ENTRY_HEADER(posXRoundWallFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(posXWallFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(posXWallFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(posYRoundWallFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(posYWallFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(posYWallFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(posZRoundWallFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(posZWallFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(posZWallFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(radialFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(radialFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator1);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator1Active);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator1LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator1Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator2);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator2Active);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator2LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator2Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator3);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator3Active);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator3LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator3Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator4);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator4Active);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator4LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator4Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator5);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator5Active);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator5LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator5Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator6);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator6Active);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator6LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator6Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(rotatorSep);
  SO_KIT_CATALOG_ENTRY_HEADER(scale1);
  SO_KIT_CATALOG_ENTRY_HEADER(scale1Active);
  SO_KIT_CATALOG_ENTRY_HEADER(scale1LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(scale1Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(scale2);
  SO_KIT_CATALOG_ENTRY_HEADER(scale2Active);
  SO_KIT_CATALOG_ENTRY_HEADER(scale2LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(scale2Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(scale3);
  SO_KIT_CATALOG_ENTRY_HEADER(scale3Active);
  SO_KIT_CATALOG_ENTRY_HEADER(scale3LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(scale3Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(scale4);
  SO_KIT_CATALOG_ENTRY_HEADER(scale4Active);
  SO_KIT_CATALOG_ENTRY_HEADER(scale4LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(scale4Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(scale5);
  SO_KIT_CATALOG_ENTRY_HEADER(scale5Active);
  SO_KIT_CATALOG_ENTRY_HEADER(scale5LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(scale5Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(scale6);
  SO_KIT_CATALOG_ENTRY_HEADER(scale6Active);
  SO_KIT_CATALOG_ENTRY_HEADER(scale6LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(scale6Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(scale7);
  SO_KIT_CATALOG_ENTRY_HEADER(scale7Active);
  SO_KIT_CATALOG_ENTRY_HEADER(scale7LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(scale7Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(scale8);
  SO_KIT_CATALOG_ENTRY_HEADER(scale8Active);
  SO_KIT_CATALOG_ENTRY_HEADER(scale8LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(scale8Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(scaleBoxFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(scaleBoxFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(scaleSep);
  SO_KIT_CATALOG_ENTRY_HEADER(surroundScale);
  SO_KIT_CATALOG_ENTRY_HEADER(translateBoxFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(translateBoxFeedbackRotation);
  SO_KIT_CATALOG_ENTRY_HEADER(translateBoxFeedbackSep);
  SO_KIT_CATALOG_ENTRY_HEADER(translateBoxFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(translator1);
  SO_KIT_CATALOG_ENTRY_HEADER(translator1Active);
  SO_KIT_CATALOG_ENTRY_HEADER(translator1LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(translator1Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(translator2);
  SO_KIT_CATALOG_ENTRY_HEADER(translator2Active);
  SO_KIT_CATALOG_ENTRY_HEADER(translator2LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(translator2Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(translator3);
  SO_KIT_CATALOG_ENTRY_HEADER(translator3Active);
  SO_KIT_CATALOG_ENTRY_HEADER(translator3LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(translator3Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(translator4);
  SO_KIT_CATALOG_ENTRY_HEADER(translator4Active);
  SO_KIT_CATALOG_ENTRY_HEADER(translator4LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(translator4Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(translator5);
  SO_KIT_CATALOG_ENTRY_HEADER(translator5Active);
  SO_KIT_CATALOG_ENTRY_HEADER(translator5LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(translator5Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(translator6);
  SO_KIT_CATALOG_ENTRY_HEADER(translator6Active);
  SO_KIT_CATALOG_ENTRY_HEADER(translator6LocateGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(translator6Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(translatorSep);
  SO_KIT_CATALOG_ENTRY_HEADER(xAxisFeedbackActive);
  SO_KIT_CATALOG_ENTRY_HEADER(xAxisFeedbackSelect);
  SO_KIT_CATALOG_ENTRY_HEADER(xAxisFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(xCircleFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(xCircleFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(xCrosshairFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(yAxisFeedbackActive);
  SO_KIT_CATALOG_ENTRY_HEADER(yAxisFeedbackSelect);
  SO_KIT_CATALOG_ENTRY_HEADER(yAxisFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(yCircleFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(yCircleFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(yCrosshairFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(zAxisFeedbackActive);
  SO_KIT_CATALOG_ENTRY_HEADER(zAxisFeedbackSelect);
  SO_KIT_CATALOG_ENTRY_HEADER(zAxisFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(zCircleFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(zCircleFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(zCrosshairFeedback);

public:
  static void initClass(void);
  SoTransformerDragger(void);

  SoSFRotation rotation;
  SoSFVec3f translation;
  SoSFVec3f scaleFactor;
  SoSFFloat minDiscRotDot;

  enum State {
    INACTIVE,

    RIT_X_ROTATE,
    TOP_Y_ROTATE,
    FNT_Z_ROTATE,
    LFT_X_ROTATE,
    BOT_Y_ROTATE,
    BAK_Z_ROTATE,

    PX_PY_PZ_3D_SCALE,
    PX_PY_NZ_3D_SCALE,
    PX_NY_PZ_3D_SCALE,
    PX_NY_NZ_3D_SCALE,
    NX_PY_PZ_3D_SCALE,
    NX_PY_NZ_3D_SCALE,
    NX_NY_PZ_3D_SCALE,
    NX_NY_NZ_3D_SCALE,

    RIT_TRANSLATE,
    TOP_TRANSLATE,
    FNT_TRANSLATE,
    LFT_TRANSLATE,
    BOT_TRANSLATE,
    BAK_TRANSLATE
  };

  State getCurrentState(void);

  void unsquishKnobs(void);

  SbBool isLocateHighlighting(void);
  void setLocateHighlighting(SbBool onoff);

  static void setColinearThreshold(int newval);
  static int getColinearThreshold(void);

  SbVec3f getBoxPointInWorldSpace(const SbVec3f & pointonunitbox);
  SbVec3f getBoxDirInWorldSpace(const SbVec3f & dironunitbox);
  SbVec3f getWorldPointInBoxSpace(const SbVec3f & pointinworldspace);
  SbVec2f getWorldPointInPixelSpace(const SbVec3f & thepoint);

  SbVec3f getInteractiveCenterInBoxSpace(void);

protected:
  virtual ~SoTransformerDragger(void);
  virtual SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE);
  virtual void setDefaultOnNonWritingFields(void);

  static void startCB(void * f, SoDragger * d);
  static void motionCB(void * f, SoDragger * d);
  static void finishCB(void * f, SoDragger * d);
  static void metaKeyChangeCB(void *, SoDragger *);
  static void fieldSensorCB(void * f, SoSensor * s);
  static void valueChangedCB(void * f, SoDragger * d);

  void dragStart(void);
  void drag(void);
  void dragFinish(void);

  void updateAntiSquishList(void);
  void setAllPartSwitches(int scalewhich, int rotatewhich, int translatewhich);
  int getMouseGestureDirection(SbBool x_ok, SbBool y_ok, SbBool z_ok);
  static int getIgnoreAxis(SbVec2f axis[3][2],
                           SbBool x_ok, SbBool y_ok, SbBool z_ok);
  static void makeMinorAxisPerpendicularIfColinear(SbVec2f origin, SbVec2f axisends[3][2], int index_a, int index_b);
  static SbBool isColinear(SbVec2f a1[2], SbVec2f a2[2], int pixels);

  SoFieldSensor * translFieldSensor;
  SoFieldSensor * scaleFieldSensor;
  SoFieldSensor * rotateFieldSensor;
  SoNodeList antiSquishList;

private:
  void getSurroundScaleMatrices(SbMatrix &matrix, SbMatrix &inv);
  SoNode *getNodeFieldNode(const char *fieldname);

  void build_catalog1(void);
  void build_catalog2(void);
  void build_catalog3(void);
  void build_catalog4(void);
  void build_catalog5(void);
  void build_catalog6(void);

  SbMatrix getWorkingToWorldMatrix(void);
  SbMatrix getWorldToWorkingMatrix(void);
  SbVec3f localToWorking(const SbVec3f &v);
  SbVec3f workingToLocal(const SbVec3f &v);
  SbVec3f calcCtrlOffset(const SbVec3f &startpt);
  void setSwitchValue(const char *str, const int which);

  SbBool setDynamicTranslatorSwitches(const SoEvent *event);
  SbBool setDynamicRotatorSwitches(const SoEvent *event);
  SbBool setDynamicScaleSwitches(const SoEvent *event);

  void dragTranslate();
  void dragScale();
  void dragRotate();

  class SbPlaneProjector *planeProj;
  class SbLineProjector *lineProj;
  class SbSphereProjector *sphereProj;
  class SbCylinderProjector *cylProj;

  State state;

private:
  SbPimplPtr<SoTransformerDraggerP> pimpl;
  friend class SoTransformerDraggerP;

  // NOT IMPLEMENTED:
  SoTransformerDragger(const SoTransformerDragger & rhs);
  SoTransformerDragger & operator = (const SoTransformerDragger & rhs);
}; // SoTransformerDragger

#endif // !COIN_SOTRANSFORMERDRAGGER_H
