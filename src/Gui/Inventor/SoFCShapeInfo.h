/****************************************************************************
 *   Copyright (c) 2021 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#ifndef FC_SOFCSHAPEINFO_H
#define FC_SOFCSHAPEINFO_H

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFMatrix.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/elements/SoShapeHintsElement.h>

class GuiExport SoFCShapeInfo : public SoNode {
  typedef SoNode inherited;
  SO_NODE_HEADER(SoNode);

public:
  SoSFInt32 partCount;
  SoSFEnum shapeType;

  enum ShapeType {
    UNKNOWN_SHAPE_TYPE = SoShapeHintsElement::UNKNOWN_SHAPE_TYPE,
    SOLID = SoShapeHintsElement::SOLID
  };

  static void initClass(void);
  SoFCShapeInfo(void);

protected:
  virtual ~SoFCShapeInfo();
};

class GuiExport SoFCShapeInstance : public SoNode {
  typedef SoNode inherited;
  SO_NODE_HEADER(SoNode);

public:
  SoSFInt32 partIndex;
  SoSFMatrix transform;
  SoSFNode shapeInfo;

  static void initClass(void);
  SoFCShapeInstance(void);

protected:
  virtual ~SoFCShapeInstance();
};

#endif // FC_SOFCSHAPEINFO_H
// vim: noai:ts=2:sw=2
