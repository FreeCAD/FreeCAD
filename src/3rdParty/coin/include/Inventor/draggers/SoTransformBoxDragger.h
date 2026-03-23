#ifndef COIN_SOTRANSFORMBOXDRAGGER_H
#define COIN_SOTRANSFORMBOXDRAGGER_H

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
class SoTransformBoxDraggerP;

class COIN_DLL_API SoTransformBoxDragger : public SoDragger {
  typedef SoDragger inherited;

  SO_KIT_HEADER(SoTransformBoxDragger);
  SO_KIT_CATALOG_ENTRY_HEADER(antiSquish);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator1);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator1Rot);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator1Sep);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator2);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator2Rot);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator2Sep);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator3);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator3Rot);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator3Sep);
  SO_KIT_CATALOG_ENTRY_HEADER(scaler);
  SO_KIT_CATALOG_ENTRY_HEADER(surroundScale);
  SO_KIT_CATALOG_ENTRY_HEADER(translator1);
  SO_KIT_CATALOG_ENTRY_HEADER(translator1Rot);
  SO_KIT_CATALOG_ENTRY_HEADER(translator1Sep);
  SO_KIT_CATALOG_ENTRY_HEADER(translator2);
  SO_KIT_CATALOG_ENTRY_HEADER(translator2Rot);
  SO_KIT_CATALOG_ENTRY_HEADER(translator2Sep);
  SO_KIT_CATALOG_ENTRY_HEADER(translator3);
  SO_KIT_CATALOG_ENTRY_HEADER(translator3Rot);
  SO_KIT_CATALOG_ENTRY_HEADER(translator3Sep);
  SO_KIT_CATALOG_ENTRY_HEADER(translator4);
  SO_KIT_CATALOG_ENTRY_HEADER(translator4Rot);
  SO_KIT_CATALOG_ENTRY_HEADER(translator4Sep);
  SO_KIT_CATALOG_ENTRY_HEADER(translator5);
  SO_KIT_CATALOG_ENTRY_HEADER(translator5Rot);
  SO_KIT_CATALOG_ENTRY_HEADER(translator5Sep);
  SO_KIT_CATALOG_ENTRY_HEADER(translator6);
  SO_KIT_CATALOG_ENTRY_HEADER(translator6Rot);
  SO_KIT_CATALOG_ENTRY_HEADER(translator6Sep);

public:
  static void initClass(void);
  SoTransformBoxDragger(void);

  SoSFRotation rotation;
  SoSFVec3f translation;
  SoSFVec3f scaleFactor;

protected:
  virtual ~SoTransformBoxDragger(void);
  virtual SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE);
  virtual void setDefaultOnNonWritingFields(void);

  static void invalidateSurroundScaleCB(void * f, SoDragger * d);
  static void fieldSensorCB(void * f, SoSensor * s);
  static void valueChangedCB(void * f, SoDragger * d);

  SoFieldSensor * rotFieldSensor;
  SoFieldSensor * translFieldSensor;
  SoFieldSensor * scaleFieldSensor;

private:
  void addChildDragger(SoDragger * child);
  void removeChildDragger(SoDragger * child);

private:
  SbLazyPimplPtr<SoTransformBoxDraggerP> pimpl;

  // NOT IMPLEMENTED:
  SoTransformBoxDragger(const SoTransformBoxDragger & rhs);
  SoTransformBoxDragger & operator = (const SoTransformBoxDragger & rhs);
}; // SoTransformBoxDragger

#endif // !COIN_SOTRANSFORMBOXDRAGGER_H
