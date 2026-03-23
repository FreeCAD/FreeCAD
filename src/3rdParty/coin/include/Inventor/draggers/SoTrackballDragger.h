#ifndef COIN_SOTRACKBALLDRAGGER_H
#define COIN_SOTRACKBALLDRAGGER_H

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
#include <Inventor/SbVec2f.h>
#include <Inventor/SbTime.h>

class SoSensor;
class SoFieldSensor;
class SbSphereProjector;
class SbCylinderProjector;
class SbLineProjector;
class SoTimerSensor;
class SoTrackballDraggerP;

class COIN_DLL_API SoTrackballDragger : public SoDragger {
  typedef SoDragger inherited;

  SO_KIT_HEADER(SoTrackballDragger);
  SO_KIT_CATALOG_ENTRY_HEADER(XRotator);
  SO_KIT_CATALOG_ENTRY_HEADER(XRotatorActive);
  SO_KIT_CATALOG_ENTRY_HEADER(XRotatorSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(YRotator);
  SO_KIT_CATALOG_ENTRY_HEADER(YRotatorActive);
  SO_KIT_CATALOG_ENTRY_HEADER(YRotatorSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(ZRotator);
  SO_KIT_CATALOG_ENTRY_HEADER(ZRotatorActive);
  SO_KIT_CATALOG_ENTRY_HEADER(ZRotatorSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(antiSquish);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator);
  SO_KIT_CATALOG_ENTRY_HEADER(rotatorActive);
  SO_KIT_CATALOG_ENTRY_HEADER(rotatorSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(surroundScale);
  SO_KIT_CATALOG_ENTRY_HEADER(userAxis);
  SO_KIT_CATALOG_ENTRY_HEADER(userAxisActive);
  SO_KIT_CATALOG_ENTRY_HEADER(userAxisRotation);
  SO_KIT_CATALOG_ENTRY_HEADER(userAxisSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(userRotator);
  SO_KIT_CATALOG_ENTRY_HEADER(userRotatorActive);
  SO_KIT_CATALOG_ENTRY_HEADER(userRotatorSwitch);

public:
  static void initClass(void);
  SoTrackballDragger(void);

  SoSFRotation rotation;
  SoSFVec3f scaleFactor;

  SbBool isAnimationEnabled(void);
  void setAnimationEnabled(SbBool newval);

protected:
  virtual ~SoTrackballDragger(void);
  virtual SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE);
  virtual void setDefaultOnNonWritingFields(void);

  void dragStart(void);
  void drag(void);
  void dragFinish(void);
  void setAllPartsActive(SbBool onoroff);

  static void startCB(void * f, SoDragger * d);
  static void motionCB(void * f, SoDragger * d);
  static void finishCB(void * f, SoDragger * d);
  static void metaKeyChangeCB(void * f, SoDragger * d);
  static void fieldSensorCB(void * f, SoSensor * s);
  static void valueChangedCB(void * f, SoDragger * d);

  SoFieldSensor * rotFieldSensor;
  SoFieldSensor * scaleFieldSensor;

private:
  SoNode * getNodeFieldNode(const char *fieldname);
  void updateUserAxisSwitches(const SbBool setactive = FALSE);
  static void timerSensorCB(void *, SoSensor *);

private:
  SbPimplPtr<SoTrackballDraggerP> pimpl;
  friend class SoTrackballDraggerP;

  // NOT IMPLEMENTED:
  SoTrackballDragger(const SoTrackballDragger & rhs);
  SoTrackballDragger & operator = (const SoTrackballDragger & rhs);
}; // SoTrackballDragger

#endif // !COIN_SOTRACKBALLDRAGGER_H
