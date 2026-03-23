#ifndef COIN_SOSPOTLIGHTDRAGGER_H
#define COIN_SOSPOTLIGHTDRAGGER_H

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
#include <Inventor/fields/SoSFFloat.h>

class SoSensor;
class SoFieldSensor;
class SbPlaneProjector;
class SoSpotLightDraggerP;

class COIN_DLL_API SoSpotLightDragger : public SoDragger {
  typedef SoDragger inherited;

  SO_KIT_HEADER(SoSpotLightDragger);
  SO_KIT_CATALOG_ENTRY_HEADER(beam);
  SO_KIT_CATALOG_ENTRY_HEADER(beamActive);
  SO_KIT_CATALOG_ENTRY_HEADER(beamPlacement);
  SO_KIT_CATALOG_ENTRY_HEADER(beamScale);
  SO_KIT_CATALOG_ENTRY_HEADER(beamSep);
  SO_KIT_CATALOG_ENTRY_HEADER(beamSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(material);
  SO_KIT_CATALOG_ENTRY_HEADER(rotator);
  SO_KIT_CATALOG_ENTRY_HEADER(translator);
  SO_KIT_CATALOG_ENTRY_HEADER(translatorRotInv);
  SO_KIT_CATALOG_ENTRY_HEADER(translatorSep);

public:
  static void initClass(void);
  SoSpotLightDragger(void);

  SoSFRotation rotation;
  SoSFVec3f translation;
  SoSFFloat angle;

protected:
  virtual ~SoSpotLightDragger(void);
  virtual SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE);
  virtual void setDefaultOnNonWritingFields(void);

  static void startCB(void * f, SoDragger * d);
  static void motionCB(void * f, SoDragger * d);
  static void doneCB(void * f, SoDragger * d);
  static void fieldSensorCB(void * f, SoSensor * s);
  static void valueChangedCB(void * f, SoDragger * d);

  void dragStart(void);
  void drag(void);
  void dragFinish(void);

  void setBeamScaleFromAngle(float beamangle);

  SoFieldSensor * rotFieldSensor;
  SoFieldSensor * translFieldSensor;
  SoFieldSensor * angleFieldSensor;
  SbPlaneProjector * planeProj;

private:
  SbLazyPimplPtr<SoSpotLightDraggerP> pimpl;

  // NOT IMPLEMENTED:
  SoSpotLightDragger(const SoSpotLightDragger & rhs);
  SoSpotLightDragger & operator = (const SoSpotLightDragger & rhs);
}; // SoSpotLightDragger

#endif // !COIN_SOSPOTLIGHTDRAGGER_H
