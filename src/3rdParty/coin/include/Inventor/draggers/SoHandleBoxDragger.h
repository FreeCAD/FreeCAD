#ifndef COIN_SOHANDLEBOXDRAGGER_H
#define COIN_SOHANDLEBOXDRAGGER_H

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
class SbPlaneProjector;
class SbLineProjector;
class SoHandleBoxDraggerP;

class COIN_DLL_API SoHandleBoxDragger : public SoDragger {
  typedef SoDragger inherited;

  SO_KIT_HEADER(SoHandleBoxDragger);
  SO_KIT_CATALOG_ENTRY_HEADER(arrow1);
  SO_KIT_CATALOG_ENTRY_HEADER(arrow1Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(arrow2);
  SO_KIT_CATALOG_ENTRY_HEADER(arrow2Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(arrow3);
  SO_KIT_CATALOG_ENTRY_HEADER(arrow3Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(arrow4);
  SO_KIT_CATALOG_ENTRY_HEADER(arrow4Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(arrow5);
  SO_KIT_CATALOG_ENTRY_HEADER(arrow5Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(arrow6);
  SO_KIT_CATALOG_ENTRY_HEADER(arrow6Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(arrowTranslation);
  SO_KIT_CATALOG_ENTRY_HEADER(drawStyle);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder1);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder1Active);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder1Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder2);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder2Active);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder2Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder3);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder3Active);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder3Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder4);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder4Active);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder4Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder5);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder5Active);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder5Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder6);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder6Active);
  SO_KIT_CATALOG_ENTRY_HEADER(extruder6Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(surroundScale);
  SO_KIT_CATALOG_ENTRY_HEADER(translator1);
  SO_KIT_CATALOG_ENTRY_HEADER(translator1Active);
  SO_KIT_CATALOG_ENTRY_HEADER(translator1Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(translator2);
  SO_KIT_CATALOG_ENTRY_HEADER(translator2Active);
  SO_KIT_CATALOG_ENTRY_HEADER(translator2Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(translator3);
  SO_KIT_CATALOG_ENTRY_HEADER(translator3Active);
  SO_KIT_CATALOG_ENTRY_HEADER(translator3Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(translator4);
  SO_KIT_CATALOG_ENTRY_HEADER(translator4Active);
  SO_KIT_CATALOG_ENTRY_HEADER(translator4Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(translator5);
  SO_KIT_CATALOG_ENTRY_HEADER(translator5Active);
  SO_KIT_CATALOG_ENTRY_HEADER(translator5Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(translator6);
  SO_KIT_CATALOG_ENTRY_HEADER(translator6Active);
  SO_KIT_CATALOG_ENTRY_HEADER(translator6Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform1);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform1Active);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform1Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform2);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform2Active);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform2Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform3);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform3Active);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform3Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform4);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform4Active);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform4Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform5);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform5Active);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform5Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform6);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform6Active);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform6Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform7);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform7Active);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform7Switch);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform8);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform8Active);
  SO_KIT_CATALOG_ENTRY_HEADER(uniform8Switch);

public:
  static void initClass(void);
  SoHandleBoxDragger(void);

  SoSFVec3f scaleFactor;
  SoSFVec3f translation;

protected:
  virtual ~SoHandleBoxDragger(void);
  virtual SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE);
  virtual void setDefaultOnNonWritingFields(void);

  static void startCB(void * f, SoDragger * d);
  static void motionCB(void * f, SoDragger * d);
  static void finishCB(void * f, SoDragger * d);
  static void metaKeyChangeCB(void * f, SoDragger * d);
  static void fieldSensorCB(void * f, SoSensor * s);
  static void valueChangedCB(void * f, SoDragger * d);

  void dragStart(void);
  void drag(void);
  void dragFinish(void);
  void setAllPartsActive(SbBool onoroff);

  SoFieldSensor * translFieldSensor;
  SoFieldSensor * scaleFieldSensor;

private:
  void updateSwitches();
  void updateArrows();
  SoNode *getNodeFieldNode(const char *fieldname);
  SbPlaneProjector *planeProj;
  SbLineProjector *lineProj;
  int whatkind;
  int whatnum;
  int constraintState;
  SbBool ctrlDown;
  SbVec3f worldRestartPt;
  SbVec3f ctrlOffset;

  void getSurroundScaleMatrices(SbMatrix &matrix, SbMatrix &inv);
  SbVec3f getDraggerCenter();
  SbVec3f calcCtrlOffset(const SbVec3f startpt);

private:
  SbLazyPimplPtr<SoHandleBoxDraggerP> pimpl;

  // NOT IMPLEMENTED:
  SoHandleBoxDragger(const SoHandleBoxDragger & rhs);
  SoHandleBoxDragger & operator = (const SoHandleBoxDragger & rhs);
}; // SoHandleBoxDragger

#endif // !COIN_SOHANDLEBOXDRAGGER_H
