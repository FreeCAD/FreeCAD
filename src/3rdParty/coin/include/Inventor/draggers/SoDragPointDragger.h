#ifndef COIN_SODRAGPOINTDRAGGER_H
#define COIN_SODRAGPOINTDRAGGER_H

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
class SoDragPointDraggerP;

class COIN_DLL_API SoDragPointDragger : public SoDragger {
  typedef SoDragger inherited;

  SO_KIT_HEADER(SoDragPointDragger);
  SO_KIT_CATALOG_ENTRY_HEADER(noRotSep);
  SO_KIT_CATALOG_ENTRY_HEADER(planeFeedbackSep);
  SO_KIT_CATALOG_ENTRY_HEADER(planeFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(planeFeedbackTranslation);
  SO_KIT_CATALOG_ENTRY_HEADER(rotX);
  SO_KIT_CATALOG_ENTRY_HEADER(rotXSep);
  SO_KIT_CATALOG_ENTRY_HEADER(rotY);
  SO_KIT_CATALOG_ENTRY_HEADER(rotYSep);
  SO_KIT_CATALOG_ENTRY_HEADER(rotZ);
  SO_KIT_CATALOG_ENTRY_HEADER(rotZSep);
  SO_KIT_CATALOG_ENTRY_HEADER(xFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(xFeedbackSep);
  SO_KIT_CATALOG_ENTRY_HEADER(xFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(xFeedbackTranslation);
  SO_KIT_CATALOG_ENTRY_HEADER(xTranslator);
  SO_KIT_CATALOG_ENTRY_HEADER(xTranslatorSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(xyFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(xyTranslator);
  SO_KIT_CATALOG_ENTRY_HEADER(xyTranslatorSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(xzFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(xzTranslator);
  SO_KIT_CATALOG_ENTRY_HEADER(xzTranslatorSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(yFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(yFeedbackSep);
  SO_KIT_CATALOG_ENTRY_HEADER(yFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(yFeedbackTranslation);
  SO_KIT_CATALOG_ENTRY_HEADER(yTranslator);
  SO_KIT_CATALOG_ENTRY_HEADER(yTranslatorSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(yzFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(yzTranslator);
  SO_KIT_CATALOG_ENTRY_HEADER(yzTranslatorSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(zFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(zFeedbackSep);
  SO_KIT_CATALOG_ENTRY_HEADER(zFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(zFeedbackTranslation);
  SO_KIT_CATALOG_ENTRY_HEADER(zTranslator);
  SO_KIT_CATALOG_ENTRY_HEADER(zTranslatorSwitch);

public:
  static void initClass(void);
  SoDragPointDragger(void);

  SoSFVec3f translation;

  void setJumpLimit(const float limit);
  float getJumpLimit(void) const;
  void showNextDraggerSet(void);

protected:
  virtual ~SoDragPointDragger(void);
  virtual SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE);
  virtual void setDefaultOnNonWritingFields(void);

  void dragStart(void);
  void drag(void);
  void dragFinish(void);

  static void startCB(void * f, SoDragger * d);
  static void motionCB(void * f, SoDragger * d);
  static void finishCB(void * f, SoDragger * d);
  static void metaKeyChangeCB(void * f, SoDragger * d);
  static void fieldSensorCB(void * f, SoSensor * s);
  static void valueChangedCB(void * f, SoDragger * d);

  SoFieldSensor * fieldSensor;

private:
  void registerDragger(SoDragger *dragger);
  void unregisterDragger(const char *name);
  void updateSwitchNodes();
  int currAxis;
  float jumpLimit;

private:
  SbLazyPimplPtr<SoDragPointDraggerP> pimpl;

  // NOT IMPLEMENTED:
  SoDragPointDragger(const SoDragPointDragger & rhs);
  SoDragPointDragger & operator = (const SoDragPointDragger & rhs);
}; // SoDragPointDragger

#endif // !COIN_SODRAGPOINTDRAGGER_H
