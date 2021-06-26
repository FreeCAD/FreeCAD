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

#include "PreCompiled.h"

#include "SoFCShapeInfo.h"

SO_NODE_SOURCE(SoFCShapeInfo)

SoFCShapeInfo::SoFCShapeInfo()
{
  SO_NODE_CONSTRUCTOR(SoFCShapeInfo);
  SO_NODE_ADD_FIELD(partCount,  (0));
  SO_NODE_ADD_FIELD(shapeType,  (0));
  SO_NODE_DEFINE_ENUM_VALUE(ShapeType, UNKNOWN_SHAPE_TYPE);
  SO_NODE_DEFINE_ENUM_VALUE(ShapeType, SOLID);
}

SoFCShapeInfo::~SoFCShapeInfo()
{
}

void
SoFCShapeInfo::initClass(void)
{
  SO_NODE_INIT_CLASS(SoFCShapeInfo,SoNode,"FCShapeInfo");
}

///////////////////////////////////////////////////////////
SO_NODE_SOURCE(SoFCShapeInstance)

SoFCShapeInstance::SoFCShapeInstance()
{
  SO_NODE_CONSTRUCTOR(SoFCShapeInstance);
  SO_NODE_ADD_FIELD(partIndex,  (-1));
  SO_NODE_ADD_FIELD(transform,  (SbMatrix()));
  SO_NODE_ADD_FIELD(shapeInfo,  (0));
}

SoFCShapeInstance::~SoFCShapeInstance()
{
}

void
SoFCShapeInstance::initClass(void)
{
  SO_NODE_INIT_CLASS(SoFCShapeInstance,SoNode,"FCShapeInstance");
}
// vim: noai:ts=2:sw=2
