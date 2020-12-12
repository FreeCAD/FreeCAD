/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef FC_DIRECTIONAL_LIGHT
#define FC_DIRECTIONAL_LIGHT

#include <Inventor/annex/FXViz/nodes/SoShadowDirectionalLight.h>
#include <Inventor/draggers/SoDirectionalLightDragger.h>

class SoChildList;
class SoDragger;
class SoFieldSensor;
class SoSensor;
class SbVec3f;
class SbMatrix;
class SoFCDirectionalLightP;
class SoSwitch;

namespace Gui
{

/** SoFCDirectionalLight implements a directional light node with embedded dragger */
class GuiExport SoFCDirectionalLight : public SoShadowDirectionalLight {
  typedef SoShadowDirectionalLight inherited;

  SO_NODE_HEADER(SoFCDirectionalLight);

public:
  static void initClass(void);
  SoFCDirectionalLight(void);

  SoSFBool showDragger;

  SoDragger * getDragger(void);
  SoSwitch  * getDraggerSwitch(void);

  virtual void doAction(SoAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void getMatrix(SoGetMatrixAction * action);
  virtual void handleEvent(SoHandleEventAction * action);
  virtual void pick(SoPickAction * action);
  virtual void search(SoSearchAction * action);
  virtual void notify(SoNotList * nl);

  virtual SoChildList * getChildren(void) const;

protected:
  virtual ~SoFCDirectionalLight(void);

  void setDragger(SoDragger * newdragger);

  virtual void copyContents(const SoFieldContainer * fromfc, SbBool copyconnections);
  static void transferFieldValues(const SoDirectionalLight * from, SoDirectionalLight * to);

  static void valueChangedCB(void * f, SoDragger * d);
  static void fieldSensorCB(void * f, SoSensor * d);

  SoFieldSensor * directionFieldSensor;
  SoFieldSensor * colorFieldSensor;
  SoChildList * children;

private:
  void attachSensors(const SbBool onoff);

private:
  SoFCDirectionalLightP *pimpl;

  SoFCDirectionalLight(const SoFCDirectionalLight & rhs);
  SoFCDirectionalLight & operator = (const SoFCDirectionalLight & rhs);
};


/** Provides a directional light dragger that auto scales according to the current viewport */
class GuiExport SoFCDirectionalLightDragger : public SoDirectionalLightDragger {
  typedef SoDirectionalLightDragger inherited;

  SO_NODE_HEADER(SoFCDirectionalLightDragger);

public:
  static void initClass(void);
  SoFCDirectionalLightDragger(void);

  SoSFVec3f scaleFactor;

  virtual void notify(SoNotList * nl);
  virtual void doAction(SoAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void pick(SoPickAction * action);
};

}

#endif // FC_DIRECTIONAL_LIGHT

// vim: noai:ts=2:sw=2
