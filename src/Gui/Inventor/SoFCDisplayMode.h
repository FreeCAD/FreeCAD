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

#ifndef FC_SOFCDISPLAYMODE_H
#define FC_SOFCDISPLAYMODE_H

#include <Inventor/nodes/SoNode.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFName.h>
#include <Inventor/fields/SoSFFloat.h>

class GuiExport SoFCDisplayMode: public SoNode {
  typedef SoNode inherited;

  SO_NODE_HEADER(SoFCDisplayMode);

public:
  static void initClass(void);
protected:
  virtual ~SoFCDisplayMode();

public:
  SoSFName displayMode;
  SoSFBool showHiddenLines;
  SoSFColor faceColor;
  SoSFColor lineColor;
  SoSFFloat transparency;

  SoFCDisplayMode();
  virtual void doAction(SoAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void callback(SoCallbackAction * action);
};

#endif // FC_SOFCDISPLAYMOD_H
// vim: noai:ts=2:sw=2
