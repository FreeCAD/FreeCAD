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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoRotation.h>
#endif

#include "FemGuiTools.h"


namespace FemGui::GuiTools
{

#define PLACEMENT_CHILDREN 2
#define CONE_CHILDREN 2

void createPlacement(SoSeparator* sep, const SbVec3f& base, const SbRotation& r)
{
    SoTranslation* trans = new SoTranslation();
    trans->translation.setValue(base);
    sep->addChild(trans);
    SoRotation* rot = new SoRotation();
    rot->rotation.setValue(r);
    sep->addChild(rot);
}

void updatePlacement(const SoSeparator* sep,
                     const int idx,
                     const SbVec3f& base,
                     const SbRotation& r)
{
    SoTranslation* trans = static_cast<SoTranslation*>(sep->getChild(idx));
    trans->translation.setValue(base);
    SoRotation* rot = static_cast<SoRotation*>(sep->getChild(idx + 1));
    rot->rotation.setValue(r);
}

void createCone(SoSeparator* sep, const double height, const double radius)
{
    // Adjust cone so that the tip is on base
    SoTranslation* trans = new SoTranslation();
    trans->translation.setValue(SbVec3f(0, -height / 2, 0));
    sep->addChild(trans);
    SoCone* cone = new SoCone();
    cone->height.setValue(height);
    cone->bottomRadius.setValue(radius);
    sep->addChild(cone);
}

SoSeparator* createCone(const double height, const double radius)
{
    // Create a new cone node
    SoSeparator* sep = new SoSeparator();
    createCone(sep, height, radius);
    return sep;
}

void updateCone(const SoNode* node, const int idx, const double height, const double radius)
{
    const SoSeparator* sep = static_cast<const SoSeparator*>(node);
    SoTranslation* trans = static_cast<SoTranslation*>(sep->getChild(idx));
    trans->translation.setValue(SbVec3f(0, -height / 2, 0));
    SoCone* cone = static_cast<SoCone*>(sep->getChild(idx + 1));
    cone->height.setValue(height);
    cone->bottomRadius.setValue(radius);
}

void createCylinder(SoSeparator* sep, const double height, const double radius)
{
    SoCylinder* cyl = new SoCylinder();
    cyl->height.setValue(height);
    cyl->radius.setValue(radius);
    sep->addChild(cyl);
}

SoSeparator* createCylinder(const double height, const double radius)
{
    // Create a new cylinder node
    SoSeparator* sep = new SoSeparator();
    createCylinder(sep, height, radius);
    return sep;
}

void updateCylinder(const SoNode* node, const int idx, const double height, const double radius)
{
    const SoSeparator* sep = static_cast<const SoSeparator*>(node);
    SoCylinder* cyl = static_cast<SoCylinder*>(sep->getChild(idx));
    cyl->height.setValue(height);
    cyl->radius.setValue(radius);
}

void createCube(SoSeparator* sep, const double width, const double length, const double height)
{
    SoCube* cube = new SoCube();
    cube->width.setValue(width);
    cube->depth.setValue(length);
    cube->height.setValue(height);
    sep->addChild(cube);
}

SoSeparator* createCube(const double width, const double length, const double height)
{
    SoSeparator* sep = new SoSeparator();
    createCube(sep, width, length, height);
    return sep;
}

void updateCube(const SoNode* node,
                const int idx,
                const double width,
                const double length,
                const double height)
{
    const SoSeparator* sep = static_cast<const SoSeparator*>(node);
    SoCube* cube = static_cast<SoCube*>(sep->getChild(idx));
    cube->width.setValue(width);
    cube->depth.setValue(length);
    cube->height.setValue(height);
}

void createArrow(SoSeparator* sep, const double length, const double radius)
{
    createCone(sep, radius, radius / 2);
    createPlacement(sep, SbVec3f(0, -radius / 2 - (length - radius) / 2, 0), SbRotation());
    createCylinder(sep, length - radius, radius / 5);
}

SoSeparator* createArrow(const double length, const double radius)
{
    SoSeparator* sep = new SoSeparator();
    createArrow(sep, length, radius);
    return sep;
}

void updateArrow(const SoNode* node, const int idx, const double length, const double radius)
{
    const SoSeparator* sep = static_cast<const SoSeparator*>(node);
    updateCone(sep, idx, radius, radius / 2);
    updatePlacement(sep,
                    idx + CONE_CHILDREN,
                    SbVec3f(0, -radius / 2 - (length - radius) / 2, 0),
                    SbRotation());
    updateCylinder(sep, idx + CONE_CHILDREN + PLACEMENT_CHILDREN, length - radius, radius / 5);
}

void createFixed(SoSeparator* sep, const double height, const double width, const bool gap)
{
    createCone(sep, height - width / 4, height - width / 4);
    createPlacement(
        sep,
        SbVec3f(0, -(height - width / 4) / 2 - width / 8 - (gap ? 1.0 : 0.1) * width / 8, 0),
        SbRotation());
    createCube(sep, width, width, width / 4);
}

SoSeparator* createFixed(const double height, const double width, const bool gap)
{
    SoSeparator* sep = new SoSeparator();
    createFixed(sep, height, width, gap);
    return sep;
}

void updateFixed(const SoNode* node,
                 const int idx,
                 const double height,
                 const double width,
                 const bool gap)
{
    const SoSeparator* sep = static_cast<const SoSeparator*>(node);
    updateCone(sep, idx, height - width / 4, height - width / 4);
    updatePlacement(
        sep,
        idx + CONE_CHILDREN,
        SbVec3f(0, -(height - width / 4) / 2 - width / 8 - (gap ? 1.0 : 0.0) * width / 8, 0),
        SbRotation());
    updateCube(sep, idx + CONE_CHILDREN + PLACEMENT_CHILDREN, width, width, width / 4);
}

}  // namespace FemGui::GuiTools
