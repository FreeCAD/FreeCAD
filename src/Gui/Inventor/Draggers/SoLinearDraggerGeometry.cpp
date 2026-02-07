// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Sayantan Deb <sayantandebin[at]gmail.com>           *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoTranslation.h>

#include "SoLinearDraggerGeometry.h"

using namespace Gui;

SO_KIT_SOURCE(SoLinearGeometryKit)

void SoLinearGeometryKit::initClass()
{
    SO_KIT_INIT_CLASS(SoLinearGeometryKit, SoBaseKit, "BaseKit");
}

SoLinearGeometryKit::SoLinearGeometryKit()
{
    SO_KIT_CONSTRUCTOR(SoLinearGeometryKit);

    SO_KIT_ADD_FIELD(tipPosition, (0.0, 0.0, 0.0));

    SO_KIT_INIT_INSTANCE();
}

SO_KIT_SOURCE(SoArrowGeometry)

void SoArrowGeometry::initClass()
{
    SO_KIT_INIT_CLASS(SoArrowGeometry, SoLinearGeometryKit, "LinearGeometryKit");
}

SoArrowGeometry::SoArrowGeometry()
{
    SO_KIT_CONSTRUCTOR(SoArrowGeometry);
    SO_KIT_ADD_CATALOG_ENTRY(separator, SoSeparator, false, this, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(lightModel, SoLightModel, false, separator, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(pickStyle, SoPickStyle, false, separator, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(arrowBody, SoCylinder, false, separator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(arrowTip, SoCone, false, separator, "", true);

    SO_KIT_ADD_CATALOG_ENTRY(_arrowBodyTranslation, SoTranslation, false, separator, arrowBody, false);
    SO_KIT_ADD_CATALOG_ENTRY(_arrowTipTranslation, SoTranslation, false, separator, arrowTip, false);

    SO_KIT_ADD_FIELD(coneBottomRadius, (0.8f));
    SO_KIT_ADD_FIELD(coneHeight, (2.5f));
    SO_KIT_ADD_FIELD(cylinderHeight, (10.0f));
    SO_KIT_ADD_FIELD(cylinderRadius, (0.1f));

    SO_KIT_INIT_INSTANCE();

    auto arrowBody = SO_GET_ANY_PART(this, "arrowBody", SoCylinder);
    arrowBody->height.connectFrom(&cylinderHeight);
    arrowBody->radius.connectFrom(&cylinderRadius);

    auto arrowTip = SO_GET_ANY_PART(this, "arrowTip", SoCone);
    arrowTip->height.connectFrom(&coneHeight);
    arrowTip->bottomRadius.connectFrom(&coneBottomRadius);

    auto lightModel = SO_GET_ANY_PART(this, "lightModel", SoLightModel);
    lightModel->model = SoLightModel::BASE_COLOR;

    auto pickStyle = SO_GET_ANY_PART(this, "pickStyle", SoPickStyle);
    pickStyle->style = SoPickStyle::SHAPE_ON_TOP;

    // forces the notify method to get called so that the initial translations and tipPostion are set
    cylinderHeight.touch();
}

void SoArrowGeometry::notify(SoNotList* notList)
{
    SoField* lastField = notList->getLastField();

    if (lastField == &cylinderHeight) {
        auto translation = SO_GET_ANY_PART(this, "_arrowBodyTranslation", SoTranslation);
        translation->translation = SbVec3f(0, cylinderHeight.getValue() / 2.0f, 0);
    }

    if (lastField == &coneHeight || lastField == &cylinderHeight) {
        auto translation = SO_GET_ANY_PART(this, "_arrowTipTranslation", SoTranslation);
        translation->translation
            = SbVec3f(0, (cylinderHeight.getValue() + coneHeight.getValue()) / 2.0f, 0);

        tipPosition = {0, cylinderHeight.getValue() + 1.5f * coneHeight.getValue(), 0};
    }
}

SO_KIT_SOURCE(SoLinearGeometryBaseKit)

void SoLinearGeometryBaseKit::initClass()
{
    SO_KIT_INIT_CLASS(SoLinearGeometryBaseKit, SoBaseKit, "BaseKit");
}

SoLinearGeometryBaseKit::SoLinearGeometryBaseKit()
{
    SO_KIT_CONSTRUCTOR(SoLinearGeometryBaseKit);

    SO_KIT_ADD_FIELD(translation, (0.0, 0.0, 0.0));
    SO_KIT_ADD_FIELD(geometryScale, (1.0, 1.0, 1.0));
    SO_KIT_ADD_FIELD(active, (false));

    SO_KIT_INIT_INSTANCE();
}

SO_KIT_SOURCE(SoArrowBase)

void SoArrowBase::initClass()
{
    SO_KIT_INIT_CLASS(SoArrowBase, SoLinearGeometryBaseKit, "LinearGeometryBaseKit");
}

SoArrowBase::SoArrowBase()
{
    SO_KIT_CONSTRUCTOR(SoArrowBase);
    SO_KIT_ADD_CATALOG_ENTRY(separator, SoSeparator, false, this, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(lightModel, SoLightModel, false, separator, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(pickStyle, SoPickStyle, false, separator, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(baseColor, SoBaseColor, false, separator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(cylinder, SoCylinder, false, separator, "", true);

    SO_KIT_ADD_CATALOG_ENTRY(_cylinderTranslation, SoTranslation, false, separator, cylinder, false);

    SO_KIT_ADD_FIELD(cylinderHeight, (1.0));
    SO_KIT_ADD_FIELD(cylinderRadius, (0.15));
    SO_KIT_ADD_FIELD(color, (0, 0, 1));

    SO_KIT_INIT_INSTANCE();

    auto lightModel = SO_GET_ANY_PART(this, "lightModel", SoLightModel);
    lightModel->model = SoLightModel::BASE_COLOR;

    auto pickStyle = SO_GET_ANY_PART(this, "pickStyle", SoPickStyle);
    pickStyle->style = SoPickStyle::UNPICKABLE;

    // We don't want to change the color of the base geometry when the dragger is
    // dragged so the active field from SoLinearGeometryBaseKit is unused
    // If that is desired then just add a toggle switch with a colour after the
    // baseColor node in the graph and it will work as expected
    auto baseColor = SO_GET_ANY_PART(this, "baseColor", SoBaseColor);
    baseColor->rgb.connectFrom(&color);

    // Forces the cylinder dimensions to be computed
    cylinderHeight.touch();
    cylinderRadius.touch();
}

void SoArrowBase::notify(SoNotList* notList)
{
    SoField* lastField = notList->getLastField();

    if (lastField == &cylinderHeight || lastField == &translation) {
        auto cylinder = SO_GET_ANY_PART(this, "cylinder", SoCylinder);
        cylinder->height = cylinderHeight.getValue() * translation.getValue()[1];

        auto cylinderTranslation = SO_GET_ANY_PART(this, "_cylinderTranslation", SoTranslation);
        cylinderTranslation->translation = {0, cylinder->height.getValue() / 2, 0};
    }
    else if (lastField == &cylinderRadius || lastField == &geometryScale) {
        auto cylinder = SO_GET_ANY_PART(this, "cylinder", SoCylinder);
        assert(
            geometryScale.getValue()[0] == geometryScale.getValue()[1]
            && geometryScale.getValue()[1] == geometryScale.getValue()[2]
            && "Camera scale should be equal along all three axes"
        );
        cylinder->radius = cylinderRadius.getValue() * geometryScale.getValue()[0];
    }
}
