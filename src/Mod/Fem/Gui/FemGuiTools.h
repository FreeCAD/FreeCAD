/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <Inventor/SbVec3f.h>
#include <Inventor/SbRotation.h>


class SoNode;
class SoSeparator;

namespace FemGui
{

namespace GuiTools
{

void createPlacement(SoSeparator* sep, const SbVec3f& base, const SbRotation& r);
void updatePlacement(const SoSeparator* sep, const int idx, const SbVec3f& base, const SbRotation& r);
void createCone(SoSeparator* sep, const double height, const double radius);
SoSeparator* createCone(const double height, const double radius);
void updateCone(const SoNode* node, const int idx, const double height, const double radius);
void createCylinder(SoSeparator* sep, const double height, const double radius);
SoSeparator* createCylinder(const double height, const double radius);
void updateCylinder(const SoNode* node, const int idx, const double height, const double radius);
void createCube(SoSeparator* sep, const double width, const double length, const double height);
SoSeparator* createCube(const double width, const double length, const double height);
void updateCube(
    const SoNode* node,
    const int idx,
    const double width,
    const double length,
    const double height
);
void createArrow(SoSeparator* sep, const double length, const double radius);
SoSeparator* createArrow(const double length, const double radius);
void updateArrow(const SoNode* node, const int idx, const double length, const double radius);
void createFixed(SoSeparator* sep, const double height, const double width, const bool gap = false);
SoSeparator* createFixed(const double height, const double width, const bool gap = false);
void updateFixed(
    const SoNode* node,
    const int idx,
    const double height,
    const double width,
    const bool gap = false
);

}  // namespace GuiTools

}  // namespace FemGui
