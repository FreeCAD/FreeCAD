#ifndef COIN_SOCENTERBALLDRAGGER_H
#define COIN_SOCENTERBALLDRAGGER_H

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
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/fields/SoSFVec3f.h>

class SoSensor;
class SoFieldSensor;
class SoCenterballDraggerP;

class COIN_DLL_API SoCenterballDragger : public SoDragger {
  typedef SoDragger inherited;

  SO_KIT_HEADER(SoCenterballDragger);

  SO_KIT_CATALOG_ENTRY_HEADER(XAxis);
  SO_KIT_CATALOG_ENTRY_HEADER(XAxisSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(XCenterChanger);
  SO_KIT_CATALOG_ENTRY_HEADER(XRotator);
  SO_KIT_CATALOG_ENTRY_HEADER(YAxis);
  SO_KIT_CATALOG_ENTRY_HEADER(YAxisSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(YCenterChanger);
  SO_KIT_CATALOG_ENTRY_HEADER(YRotator);
  SO_KIT_CATALOG_ENTRY_HEADER(ZAxis);
  SO_KIT_CATALOG_ENTRY_HEADER(ZAxisSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(ZCenterChanger);
  SO_KIT_CATALOG_ENTRY_HEADER(ZRotator);
  SO_KIT_CATALOG_ENTRY_HEADER(antiSquish);
  SO_KIT_CATALOG_ENTRY_HEADER(lightModel);
  SO_KIT_CATALOG_ENTRY_HEADER(rot2X90);
  SO_KIT_CATALOG_ENTRY_HEADER(rotX90);
  SO_KIT_CATALOG_ENTRY_HEADER(rotY90);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator);
  SO_KIT_CATALOG_ENTRY_HEADER(surroundScale);
  SO_KIT_CATALOG_ENTRY_HEADER(translateToCenter);

public:
  static void initClass(void);
  SoCenterballDragger(void);

  virtual void saveStartParameters(void);

  SoSFRotation rotation;
  SoSFVec3f center;

protected:
  virtual ~SoCenterballDragger(void);
  void transferCenterDraggerMotion(SoDragger * childdragger);
  void setSwitches(SoDragger * activechild);
  virtual SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE);
  virtual void setDefaultOnNonWritingFields(void);

  static void fieldSensorCB(void * f, SoSensor * s);
  static void valueChangedCB(void * f, SoDragger * d);
  static void kidStartCB(void * f, SoDragger * d);
  static void kidFinishCB(void * f, SoDragger * d);

  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void getMatrix(SoGetMatrixAction * action);

  SoFieldSensor * rotFieldSensor;
  SoFieldSensor * centerFieldSensor;

private:
  SbVec3f savedtransl;
  SbVec3f savedcenter;
  void addChildDragger(SoDragger *child);
  void removeChildDragger(const char *childname);

private:
  SbLazyPimplPtr<SoCenterballDraggerP> pimpl;

  // NOT IMPLEMENTED:
  SoCenterballDragger(const SoCenterballDragger & rhs);
  SoCenterballDragger & operator & (const SoCenterballDragger & rhs);
}; // SoCenterballDragger

#endif // !COIN_SOCENTERBALLDRAGGER_H
