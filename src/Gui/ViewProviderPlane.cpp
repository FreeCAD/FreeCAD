/***************************************************************************
 *   Copyright (c) 2012 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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
# include <Inventor/nodes/SoAsciiText.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoFaceSet.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/SbColor.h>
#endif

#include <App/Datums.h>
#include <Gui/ViewParams.h>

#include "ViewProviderPlane.h"
#include "ViewProviderCoordinateSystem.h"


using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderPlane, Gui::ViewProviderDatum)


ViewProviderPlane::ViewProviderPlane()
{
    sPixmap = "Std_Plane";
    lineThickness = 1.0;

    pLabel = new SoAsciiText();
}

ViewProviderPlane::~ViewProviderPlane() = default;

void ViewProviderPlane::attach(App::DocumentObject * obj) {
    ViewProviderDatum::attach(obj);

    std::string role = getRole();

    SoSeparator* sep = getDatumRoot();

    // Setup colors
    // Can't use transparency because of https://github.com/FreeCAD/FreeCAD/issues/18395
    // When this issue is fixed then we can use the below and remove the material here
    // and faceSeparator...
    //ShapeAppearance.setTransparency(0.8);
    auto material = new SoMaterial();
    material->transparency.setValue(0.95f);
    if (!role.empty()) {
        ShapeAppearance.setDiffuseColor(getColor(role));
        SbColor color;
        float alpha = 0.0f;
        color.setPackedValue(getColor(role), alpha);
        material->ambientColor.setValue(color);
        material->diffuseColor.setValue(color);
    }

    static const float size = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")->GetFloat("DatumPlaneSize", 40.0);
    static const float startSize = 0.25 * size; //NOLINT

    SbVec3f verts[4];
    if (role.empty()) {
        verts[0] = SbVec3f(size, size, 0);
        verts[1] = SbVec3f(size, -size, 0);
        verts[2] = SbVec3f(-size, -size, 0);
        verts[3] = SbVec3f(-size, size, 0);
    }
    else {
        verts[0] = SbVec3f(size, size, 0);
        verts[1] = SbVec3f(size, startSize, 0);
        verts[2] = SbVec3f(startSize, startSize, 0);
        verts[3] = SbVec3f(startSize, size, 0);
    }

    auto pCoords = new SoCoordinate3();
    pCoords->point.setNum(4);
    pCoords->point.setValues(0, 4, verts);
    sep->addChild(pCoords);

    auto lineSeparator = new SoSeparator();
    auto pLines = new SoIndexedLineSet();
    static const int32_t lines[6] = { 0, 1, 2, 3, 0, -1 };
    pLines->coordIndex.setNum(6);
    pLines->coordIndex.setValues(0, 6, lines);

    auto ps = new SoPickStyle();
    ps->style.setValue(SoPickStyle::SHAPE_ON_TOP);
    lineSeparator->addChild(ps);
    lineSeparator->addChild(pLines);
    sep->addChild(lineSeparator);

    // add semi transparent face
    auto faceSeparator = new SoSeparator();
    sep->addChild(faceSeparator);

    faceSeparator->addChild(material);

    // disable backface culling and render with two-sided lighting
    auto shapeHints = new SoShapeHints();
    shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    shapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    faceSeparator->addChild(shapeHints);

    auto faceSet = new SoFaceSet();
    auto vertexProperty = new SoVertexProperty();
    vertexProperty->vertex.setValues(0, 4, verts);
    faceSet->vertexProperty.setValue(vertexProperty);
    faceSeparator->addChild(faceSet);

    auto textTranslation = new SoTranslation();
    SbVec3f centeringVec = size * SbVec3f(0.36F, 0.49F, 0.0F);  // NOLINT
    textTranslation->translation.setValue(centeringVec);
    sep->addChild(textTranslation);

    pLabel->string.setValue(getLabelText(role).c_str());

    labelSwitch = new SoSwitch();
    setLabelVisibility(false);
    labelSwitch->addChild(pLabel);
    sep->addChild(labelSwitch);
}

void ViewProviderPlane::setLabelVisibility(bool val)
{
    labelSwitch->whichChild = val ? SO_SWITCH_ALL : SO_SWITCH_NONE;
}

unsigned long ViewProviderPlane::getColor(const std::string& role) const
{
    auto planesRoles = App::LocalCoordinateSystem::PlaneRoles;
    if (role == planesRoles[0]) {
        return ViewParams::instance()->getAxisZColor(); // XY-plane
    }
    else if (role == planesRoles[1]) {
        return ViewParams::instance()->getAxisYColor(); // XZ-plane
    }
    else if (role == planesRoles[2]) {
        return ViewParams::instance()->getAxisXColor(); // YZ-plane
    }
    return 0;
}

std::string ViewProviderPlane::getLabelText(const std::string& role) const
{
    std::string text;
    auto planesRoles = App::LocalCoordinateSystem::PlaneRoles;
    if (role == planesRoles[0]) {
        text = "XY";
    }
    else if (role == planesRoles[1]) {
        text = "XZ";
    }
    else if (role == planesRoles[2]) {
        text = "YZ";
    }
    return text;
}

std::string ViewProviderPlane::getRole() const
{
    // Note: Role property of App::Plane is not set yet when attaching.
    const char* name = pcObject->getNameInDocument();
    auto planesRoles = App::LocalCoordinateSystem::PlaneRoles;
    if (strncmp(name, planesRoles[0], strlen(planesRoles[0])) == 0) {
        return planesRoles[0];
    }
    else if (strncmp(name, planesRoles[1], strlen(planesRoles[1])) == 0) {
        return planesRoles[1];
    }
    else if (strncmp(name, planesRoles[2], strlen(planesRoles[2])) == 0) {
        return planesRoles[2];
    }

    return "";
}
