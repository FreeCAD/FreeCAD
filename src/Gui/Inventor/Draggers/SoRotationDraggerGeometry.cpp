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

#include <algorithm>
#include <cmath>
#include <numbers>

#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTranslation.h>

#include "SoRotationDraggerGeometry.h"

#include <Gui/Inventor/SoToggleSwitch.h>

using namespace Gui;

SO_KIT_SOURCE(SoRotatorGeometryKit)

void SoRotatorGeometryKit::initClass()
{
    SO_KIT_INIT_CLASS(SoRotatorGeometryKit, SoBaseKit, "BaseKit");
}

SoRotatorGeometryKit::SoRotatorGeometryKit()
{
    SO_KIT_CONSTRUCTOR(SoRotatorGeometryKit);

    SO_KIT_ADD_FIELD(pivotPosition, (0.0, 0.0, 0.0));
    SO_KIT_ADD_FIELD(geometryScale, (1.0, 1.0, 1.0));

    SO_KIT_INIT_INSTANCE();
}

SO_KIT_SOURCE(SoRotatorGeometry)

void SoRotatorGeometry::initClass()
{
    SO_KIT_INIT_CLASS(SoRotatorGeometry, SoRotatorGeometryKit, "RotatorGeometryKit");
}

SoRotatorGeometry::SoRotatorGeometry()
{
    SO_KIT_CONSTRUCTOR(SoRotatorGeometry);
    SO_KIT_ADD_CATALOG_ENTRY(separator, SoSeparator, false, this, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(lightModel, SoLightModel, false, separator, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(pickStyle, SoPickStyle, false, separator, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(drawStyle, SoDrawStyle, false, separator, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(arcCoords, SoCoordinate3, false, separator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(arc, SoLineSet, false, separator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(pivotSeparator, SoSeparator, false, separator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(rotorPivot, SoSphere, false, pivotSeparator, "", true);

    SO_KIT_ADD_CATALOG_ENTRY(_rotorPivotTranslation, SoTranslation, false, pivotSeparator, rotorPivot, false);

    SO_KIT_ADD_FIELD(arcRadius, (8.0f));
    SO_KIT_ADD_FIELD(arcAngle, (std::numbers::pi_v<float> / 2.0f));
    SO_KIT_ADD_FIELD(sphereRadius, (0.8f));
    SO_KIT_ADD_FIELD(arcThickness, (4.0f));

    SO_KIT_INIT_INSTANCE();

    auto lightModel = SO_GET_ANY_PART(this, "lightModel", SoLightModel);
    lightModel->model = SoLightModel::BASE_COLOR;

    auto pickStyle = SO_GET_ANY_PART(this, "pickStyle", SoPickStyle);
    pickStyle->style = SoPickStyle::SHAPE_ON_TOP;

    auto rotorPivot = SO_GET_ANY_PART(this, "rotorPivot", SoSphere);
    rotorPivot->radius.connectFrom(&sphereRadius);

    auto drawStyle = SO_GET_ANY_PART(this, "drawStyle", SoDrawStyle);
    drawStyle->lineWidth.connectFrom(&arcThickness);

    auto translation = SO_GET_ANY_PART(this, "_rotorPivotTranslation", SoTranslation);
    pivotPosition.connectFrom(&translation->translation);

    auto arc = SO_GET_ANY_PART(this, "arc", SoLineSet);
    arc->numVertices = segments + 1;

    // forces the notify method to get called so that the initial translations and other values are set
    arcRadius.touch();
}

void SoRotatorGeometry::notify(SoNotList* notList)
{
    SoField* lastField = notList->getLastField();

    if (lastField == &arcRadius || lastField == &arcAngle) {
        float angle = arcAngle.getValue();
        float radius = arcRadius.getValue();

        auto coordinates = SO_GET_ANY_PART(this, "arcCoords", SoCoordinate3);
        float angleIncrement = angle / static_cast<float>(segments);
        SbRotation rotation(SbVec3f(0.0, 0.0, 1.0), angleIncrement);
        SbVec3f point(radius, 0.0, 0.0);
        for (int index = 0; index <= segments; ++index) {
            coordinates->point.set1Value(index, point);
            rotation.multVec(point, point);
        }

        auto translation = SO_GET_ANY_PART(this, "_rotorPivotTranslation", SoTranslation);
        translation->translation = {radius * cos(angle / 2.0f), radius * sin(angle / 2.0f), 0};
    }
}

SO_KIT_SOURCE(SoRotatorGeometry2)

void SoRotatorGeometry2::initClass()
{
    SO_KIT_INIT_CLASS(SoRotatorGeometry2, SoRotatorGeometry, "SoRotatorGeometry");
}

SoRotatorGeometry2::SoRotatorGeometry2()
{
    SO_KIT_CONSTRUCTOR(SoRotatorGeometry2);

    SO_KIT_ADD_CATALOG_ENTRY(leftArrowSwitch, SoToggleSwitch, true, separator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(leftArrowSeparator, SoSeparator, true, leftArrowSwitch, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(leftArrowTransform, SoTransform, true, leftArrowSeparator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(leftArrow, SoCone, true, leftArrowSeparator, "", true);

    SO_KIT_ADD_CATALOG_ENTRY(rightArrowSwitch, SoToggleSwitch, true, separator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(rightArrowSeparator, SoSeparator, true, rightArrowSwitch, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(rightArrowTransform, SoTransform, true, rightArrowSeparator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(rightArrow, SoCone, true, rightArrowSeparator, "", true);

    SO_KIT_ADD_FIELD(coneBottomRadius, (0.8));
    SO_KIT_ADD_FIELD(coneHeight, (2.5));
    SO_KIT_ADD_FIELD(leftArrowVisible, (true));
    SO_KIT_ADD_FIELD(rightArrowVisible, (true));

    SO_KIT_INIT_INSTANCE();

    auto arrowHead = SO_GET_ANY_PART(this, "leftArrow", SoCone);
    arrowHead->bottomRadius.connectFrom(&coneBottomRadius);
    arrowHead->height.connectFrom(&coneHeight);

    arrowHead = SO_GET_ANY_PART(this, "rightArrow", SoCone);
    arrowHead->bottomRadius.connectFrom(&coneBottomRadius);
    arrowHead->height.connectFrom(&coneHeight);

    auto sw = SO_GET_ANY_PART(this, "leftArrowSwitch", SoToggleSwitch);
    sw->on.connectFrom(&leftArrowVisible);

    sw = SO_GET_ANY_PART(this, "rightArrowSwitch", SoToggleSwitch);
    sw->on.connectFrom(&rightArrowVisible);

    // forces the notify method to get called so that the initial transforms are set
    arcAngle.touch();
}

void SoRotatorGeometry2::notify(SoNotList* notList)
{
    SoField* lastField = notList->getLastField();

    inherited::notify(notList);

    if (lastField == &arcAngle || lastField == &arcRadius) {
        auto angle = arcAngle.getValue();
        auto radius = arcRadius.getValue();

        auto transform = SO_GET_ANY_PART(this, "leftArrowTransform", SoTransform);
        transform->translation = SbVec3f {radius, 0, 0};
        transform->rotation = SbRotation({0, 0, 1}, std::numbers::pi_v<float>);

        transform = SO_GET_ANY_PART(this, "rightArrowTransform", SoTransform);
        transform->translation = SbVec3f {radius * std::cos(angle), radius * std::sin(angle), 0};
        transform->rotation = SbRotation({0, 0, 1}, angle);
    }
}

void SoRotatorGeometry2::toggleArrowVisibility()
{
    leftArrowVisible = !leftArrowVisible.getValue();
    rightArrowVisible = !rightArrowVisible.getValue();
}

SO_KIT_SOURCE(SoRotatorArrow)

void SoRotatorArrow::initClass()
{
    SO_KIT_INIT_CLASS(SoRotatorArrow, SoRotatorGeometryKit, "RotatorGeometryKit");
}

SoRotatorArrow::SoRotatorArrow()
{
    SO_KIT_CONSTRUCTOR(SoRotatorArrow);
    SO_KIT_ADD_CATALOG_ENTRY(separator, SoSeparator, false, this, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(lightModel, SoLightModel, false, separator, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(pickStyle, SoPickStyle, false, separator, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(arrowBody, SoCylinder, false, separator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(arrowTip, SoCone, false, separator, "", true);

    SO_KIT_ADD_CATALOG_ENTRY(_arrowTransform, SoTransform, false, separator, arrowBody, false);
    SO_KIT_ADD_CATALOG_ENTRY(_arrowBodyTranslation, SoTranslation, false, separator, arrowBody, false);
    SO_KIT_ADD_CATALOG_ENTRY(_arrowTipTranslation, SoTranslation, false, separator, arrowTip, false);

    SO_KIT_ADD_FIELD(coneBottomRadius, (0.8));
    SO_KIT_ADD_FIELD(coneHeight, (2.5));
    SO_KIT_ADD_FIELD(cylinderHeight, (3.0));
    SO_KIT_ADD_FIELD(cylinderRadius, (0.2));
    SO_KIT_ADD_FIELD(radius, (8.0));
    SO_KIT_ADD_FIELD(minRadius, (8.0));

    SO_KIT_INIT_INSTANCE();

    auto lightModel = SO_GET_ANY_PART(this, "lightModel", SoLightModel);
    lightModel->model = SoLightModel::BASE_COLOR;

    auto pickStyle = SO_GET_ANY_PART(this, "pickStyle", SoPickStyle);
    pickStyle->style = SoPickStyle::SHAPE_ON_TOP;

    auto arrowBody = SO_GET_ANY_PART(this, "arrowBody", SoCylinder);
    arrowBody->height.connectFrom(&cylinderHeight);
    arrowBody->radius.connectFrom(&cylinderRadius);

    auto arrowTip = SO_GET_ANY_PART(this, "arrowTip", SoCone);
    arrowTip->height.connectFrom(&coneHeight);
    arrowTip->bottomRadius.connectFrom(&coneBottomRadius);

    auto transform = SO_GET_ANY_PART(this, "_arrowTransform", SoTransform);
    transform->rotation = SbRotation({1, 0, 0}, std::numbers::pi)
        * SbRotation({0, 0, 1}, std::numbers::pi / 2);

    // forces the notify method to get called so that the initial dimensions are set
    cylinderHeight.touch();
    radius.touch();
}

void SoRotatorArrow::flipArrow()
{
    auto transform = SO_GET_ANY_PART(this, "_arrowTransform", SoTransform);
    transform->rotation = transform->rotation.getValue() * SbRotation({0, 0, 1}, std::numbers::pi);
}

void SoRotatorArrow::notify(SoNotList* notList)
{
    SoField* lastField = notList->getLastField();

    if (lastField == &cylinderHeight) {
        auto translation = SO_GET_ANY_PART(this, "_arrowBodyTranslation", SoTranslation);
        translation->translation = SbVec3f(0, cylinderHeight.getValue() / 2.0f, 0);
    }
    else if (lastField == &radius || lastField == &minRadius || lastField == &geometryScale) {
        auto transform = SO_GET_ANY_PART(this, "_arrowTransform", SoTransform);
        // The geometry nodes are scaled from the parent dragger but in this case the translation
        // itself shouldn't be scaled
        pivotPosition = transform->translation = SbVec3f(
            0,
            std::max(minRadius.getValue(), radius.getValue() / geometryScale.getValue()[1]),
            0
        );
    }

    if (lastField == &coneHeight || lastField == &cylinderHeight) {
        auto translation = SO_GET_ANY_PART(this, "_arrowTipTranslation", SoTranslation);
        translation->translation
            = SbVec3f(0, (cylinderHeight.getValue() + coneHeight.getValue()) / 2.0f, 0);
    }
}

SO_KIT_SOURCE(SoRotatorGeometryBaseKit)

void SoRotatorGeometryBaseKit::initClass()
{
    SO_KIT_INIT_CLASS(SoRotatorGeometryBaseKit, SoBaseKit, "BaseKit");
}

SoRotatorGeometryBaseKit::SoRotatorGeometryBaseKit()
{
    SO_KIT_CONSTRUCTOR(SoRotatorGeometryBaseKit);

    SO_KIT_ADD_FIELD(rotation, ({0.0, 0.0, 0.0}, 0.0));
    SO_KIT_ADD_FIELD(geometryScale, (1.0, 1.0, 1.0));
    SO_KIT_ADD_FIELD(active, (false));

    SO_KIT_INIT_INSTANCE();
}

SO_KIT_SOURCE(SoRotatorBase)

void SoRotatorBase::initClass()
{
    SO_KIT_INIT_CLASS(SoRotatorBase, SoRotatorGeometryBaseKit, "RotatorGeometryBaseKit");
}

SoRotatorBase::SoRotatorBase()
{
    SO_KIT_CONSTRUCTOR(SoRotatorBase);
    SO_KIT_ADD_CATALOG_ENTRY(separator, SoSeparator, false, this, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(lightModel, SoLightModel, false, separator, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(pickStyle, SoPickStyle, false, separator, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(baseColor, SoBaseColor, false, separator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(drawStyle, SoDrawStyle, false, separator, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(arcCoords, SoCoordinate3, false, separator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(arc, SoLineSet, false, separator, "", true);

    SO_KIT_ADD_FIELD(minArcRadius, (8.0));
    SO_KIT_ADD_FIELD(arcRadius, (8.0));
    SO_KIT_ADD_FIELD(arcThickness, (2.0));
    SO_KIT_ADD_FIELD(color, (0.214, 0.560, 0.930));

    SO_KIT_INIT_INSTANCE();

    auto lightModel = SO_GET_ANY_PART(this, "lightModel", SoLightModel);
    lightModel->model = SoLightModel::BASE_COLOR;

    auto pickStyle = SO_GET_ANY_PART(this, "pickStyle", SoPickStyle);
    pickStyle->style = SoPickStyle::UNPICKABLE;

    // We don't want to change the color of the base geometry when the dragger is
    // dragged so the active field from SoRotatorGeometryBaseKit is unused
    // If that is desired then just add a toggle switch with a colour after the
    // baseColor node in the graph and it will work as expected
    auto baseColor = SO_GET_ANY_PART(this, "baseColor", SoBaseColor);
    baseColor->rgb.connectFrom(&color);

    auto drawStyle = SO_GET_ANY_PART(this, "drawStyle", SoDrawStyle);
    drawStyle->lineWidth.connectFrom(&arcThickness);

    auto arc = SO_GET_ANY_PART(this, "arc", SoLineSet);
    arc->numVertices = segments + 3;

    // forces the notify method to get called so that the initial translations and other values are set
    arcRadius.touch();
}

void SoRotatorBase::notify(SoNotList* notList)
{
    SoField* lastField = notList->getLastField();

    if (lastField == &arcRadius || lastField == &minArcRadius || lastField == &rotation
        || lastField == &geometryScale) {
        SbVec3f axis;
        float angle;
        rotation.getValue(axis, angle);
        float radius
            = std::max(minArcRadius.getValue() * geometryScale.getValue()[0], arcRadius.getValue());

        auto coordinates = SO_GET_ANY_PART(this, "arcCoords", SoCoordinate3);
        float angleIncrement = angle / static_cast<float>(segments);
        SbRotation rotation(axis, angleIncrement);
        SbVec3f point(0.0, radius, 0.0);
        coordinates->point.set1Value(0, {0.0, 0.0, 0.0});
        for (int index = 0; index <= segments; ++index) {
            coordinates->point.set1Value(index + 1, point);
            rotation.multVec(point, point);
        }
        coordinates->point.set1Value(segments + 2, {0.0, 0.0, 0.0});
    }
}
