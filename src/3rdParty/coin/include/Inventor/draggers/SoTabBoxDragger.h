#ifndef COIN_SOTABBOXDRAGGER_H
#define COIN_SOTABBOXDRAGGER_H

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
class SoTabBoxDraggerP;

class COIN_DLL_API SoTabBoxDragger : public SoDragger {
  typedef SoDragger inherited;

  SO_KIT_HEADER(SoTabBoxDragger);
  SO_KIT_CATALOG_ENTRY_HEADER(boxGeom);
  SO_KIT_CATALOG_ENTRY_HEADER(surroundScale);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane1);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane1Sep);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane1Xf);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane2);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane2Sep);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane2Xf);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane3);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane3Sep);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane3Xf);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane4);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane4Sep);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane4Xf);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane5);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane5Sep);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane5Xf);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane6);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane6Sep);
  SO_KIT_CATALOG_ENTRY_HEADER(tabPlane6Xf);

public:
  static void initClass(void);
  SoTabBoxDragger(void);

  SoSFVec3f translation;
  SoSFVec3f scaleFactor;

  void adjustScaleTabSize(void);

protected:
  virtual ~SoTabBoxDragger(void);
  virtual SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE);
  virtual void setDefaultOnNonWritingFields(void);

  static void invalidateSurroundScaleCB(void * f, SoDragger * d);
  static void fieldSensorCB(void * f, SoSensor * s);
  static void valueChangedCB(void * f, SoDragger * d);

  SoFieldSensor * translFieldSensor;
  SoFieldSensor * scaleFieldSensor;

private:
  void initTransformNodes(void);

private:
  SbLazyPimplPtr<SoTabBoxDraggerP> pimpl;

  // NOT IMPLEMENTED:
  SoTabBoxDragger(const SoTabBoxDragger & rhs);
  SoTabBoxDragger & operator = (const SoTabBoxDragger & rhs);
}; // SoTabBoxDragger

#endif // !COIN_SOTABBOXDRAGGER_H
