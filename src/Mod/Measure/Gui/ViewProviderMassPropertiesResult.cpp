// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Morten Vajhøj
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "ViewProviderMassPropertiesResult.h"

#include <QSize>

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoAlphaTest.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoImage.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTranslation.h>

#include <Base/Precision.h>
#include <Base/Tools.h>

#include <App/Application.h>

#include <Gui/BitmapFactory.h>
#include <Gui/Inventor/SoAxisCrossKit.h>
#include <Gui/ViewParams.h>
#include <Gui/ViewProviderCoordinateSystem.h>

#include "ViewProviderMeasureBase.h"

using namespace MassPropertiesGui;

PROPERTY_SOURCE(MassPropertiesGui::ViewProviderMassPropertiesResult, Gui::ViewProviderDocumentObject)

ViewProviderMassPropertiesResult::ViewProviderMassPropertiesResult()
    : displayRoot(new SoSeparator())
{
    sPixmap = "MassPropertiesIcon";
    displayRoot->ref();
    getModeSwitch()->addChild(displayRoot);
    setDefaultMode(0);
    setShowable(true);
}

ViewProviderMassPropertiesResult::~ViewProviderMassPropertiesResult()
{
    displayRoot->unref();
}

void ViewProviderMassPropertiesResult::attach(App::DocumentObject* obj)
{
    ViewProviderDocumentObject::attach(obj);

    auto* annotation = new SoAnnotation();
    displayRoot->addChild(annotation);

    auto* depth = new SoDepthBuffer();
    depth->function = SoDepthBufferElement::LESS;
    depth->range = SbVec2f(0.0f, 0.001f);
    annotation->addChild(depth);

    lcsSwitch = new SoSwitch(SO_SWITCH_NONE);
    annotation->addChild(lcsSwitch);

    auto* seperator = new SoSeparator();
    lcsSwitch->addChild(seperator);

    lcsOriginTranslation = new SoTranslation();
    seperator->addChild(lcsOriginTranslation);

    lcsScale = new Gui::SoShapeScale();
    seperator->addChild(lcsScale);

    auto* lcsSep = new SoSeparator();
    lcsScale->setPart("shape", lcsSep);

    auto* style = new SoDrawStyle();

    // There does not seem to be a property in preferences for this
    // ViewProviderDatum.cpp also defaults to 2.0f
    style->lineWidth = 2.0f;
    lcsSep->addChild(style);

    lcsMaterial = new SoMaterial();
    lcsMaterial->diffuseColor.setNum(3);
    lcsSep->addChild(lcsMaterial);

    auto* lcsMaterialBinding = new SoMaterialBinding();
    lcsMaterialBinding->value = SoMaterialBinding::PER_PART;
    lcsSep->addChild(lcsMaterialBinding);

    lcsCoords = new SoCoordinate3();
    lcsSep->addChild(lcsCoords);

    auto* lines = new SoIndexedLineSet();

    static const int32_t lineIndices[9] = {0, 1, -1, 2, 3, -1, 4, 5, -1};
    static const int32_t materialIndices[3] = {0, 1, 2};

    lines->coordIndex.setValues(0, 9, lineIndices);
    lines->materialIndex.setValues(0, 3, materialIndices);
    lcsSep->addChild(lines);

    auto addAxisLabel =
        [lcsSep](const char* text, SoBaseColor*& colorOut, SoTranslation*& translationOut) {
            auto* labelSep = new SoSeparator();

            colorOut = new SoBaseColor();
            labelSep->addChild(colorOut);

            translationOut = new SoTranslation();
            labelSep->addChild(translationOut);

            auto* labelAlpha = new SoAlphaTest();
            labelAlpha->function = SoAlphaTest::GREATER;
            labelAlpha->value = 0.0f;
            labelSep->addChild(labelAlpha);

            auto* labelText = new Gui::SoFrameLabel();
            labelText->string.setValue(text);
            labelText->horAlignment = SoImage::CENTER;
            labelText->vertAlignment = SoImage::HALF;
            labelText->border = false;
            labelText->frame = false;
            labelText->textUseBaseColor = true;
            labelText->size = 8;
            labelSep->addChild(labelText);

            lcsSep->addChild(labelSep);
        };

    addAxisLabel("1", lcsLabel1Color, lcsLabel1Translation);
    addAxisLabel("2", lcsLabel2Color, lcsLabel2Translation);
    addAxisLabel("3", lcsLabel3Color, lcsLabel3Translation);

    auto addMarker = [annotation](const char* iconName, SoTranslation*& translation) {
        auto* markerSep = new SoSeparator();
        translation = new SoTranslation();
        auto* image = new SoImage();

        image->horAlignment = SoImage::CENTER;
        image->vertAlignment = SoImage::HALF;

        QPixmap pixmap = Gui::BitmapFactory().pixmapFromSvg(iconName, QSize(16, 16));
        SoSFImage iconData;
        Gui::BitmapFactory().convert(pixmap.toImage(), iconData);
        image->image = iconData;

        markerSep->addChild(translation);
        markerSep->addChild(image);
        annotation->addChild(markerSep);
    };

    addMarker("COG-Icon", cogTranslation);
    addMarker("COV-Icon", covTranslation);

    updateCenterMarkers();
    updatePrincipalAxesMarker();
}

void ViewProviderMassPropertiesResult::setCenters(const Base::Vector3d& cog, const Base::Vector3d& cov)
{
    centerOfGravity = cog;
    centerOfVolume = cov;
    updateCenterMarkers();
}

void ViewProviderMassPropertiesResult::setPrincipalAxes(
    const Base::Vector3d& origin,
    const Base::Vector3d& axis1,
    const Base::Vector3d& axis2,
    const Base::Vector3d& axis3,
    bool visible
)
{
    principalOrigin = origin;
    principalAxis1 = axis1;
    principalAxis2 = axis2;
    principalAxis3 = axis3;
    showPrincipalAxes = visible;
    updatePrincipalAxesMarker();
}

void ViewProviderMassPropertiesResult::updateCenterMarkers()
{
    if (cogTranslation) {
        cogTranslation->translation.setValue(
            MeasureGui::ViewProviderMeasureBase::toSbVec3f(centerOfGravity)
        );
    }
    if (covTranslation) {
        covTranslation->translation.setValue(
            MeasureGui::ViewProviderMeasureBase::toSbVec3f(centerOfVolume)
        );
    }
}

void ViewProviderMassPropertiesResult::updatePrincipalAxesMarker()
{
    if (!lcsSwitch || !lcsCoords) {
        return;
    }

    // If the user has selected an axis. The principal axes does not need to be added
    if (!showPrincipalAxes) {
        lcsSwitch->whichChild = SO_SWITCH_NONE;
        return;
    }

    const auto params = Gui::ViewParams::instance();

    SbColor axisColors[3];
    float transparency = 0.0f;
    axisColors[0].setPackedValue(static_cast<uint32_t>(params->getAxisXColor()), transparency);
    axisColors[1].setPackedValue(static_cast<uint32_t>(params->getAxisYColor()), transparency);
    axisColors[2].setPackedValue(static_cast<uint32_t>(params->getAxisZColor()), transparency);

    if (lcsMaterial) {
        lcsMaterial->diffuseColor.set1Value(0, axisColors[0]);
        lcsMaterial->diffuseColor.set1Value(1, axisColors[1]);
        lcsMaterial->diffuseColor.set1Value(2, axisColors[2]);
    }

    SoBaseColor* labelColors[3] = {
        lcsLabel1Color,
        lcsLabel2Color,
        lcsLabel3Color,
    };
    for (int i = 0; i < 3; ++i) {
        if (labelColors[i]) {
            labelColors[i]->rgb.setValue(axisColors[i]);
        }
    }

    const Base::Vector3d axes[3] = {principalAxis1, principalAxis2, principalAxis3};

    double size = params->getDatumLineSize() * Base::fromPercent(params->getDatumScale());

    // Again using preferences elsewhere
    const float lcsScaleFactor = App::GetApplication()
                                     .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                                     ->GetFloat("LocalCoordinateSystemSize", 1.0f);

    if (lcsScale) {
        lcsScale->scaleFactor = lcsScaleFactor;
    }

    if (size < Base::Precision::Confusion()) {
        size = Gui::ViewProviderCoordinateSystem::defaultSize();
    }

    if (lcsOriginTranslation) {
        lcsOriginTranslation->translation.setValue(
            MeasureGui::ViewProviderMeasureBase::toSbVec3f(principalOrigin)
        );
    }

    // the 0.2 is so that there is a small gap in the beginning so it does not overlap with the COG
    // marker
    SbVec3f coords[6];
    coords[0] = MeasureGui::ViewProviderMeasureBase::toSbVec3f(axes[0] * (0.2 * size));
    coords[1] = MeasureGui::ViewProviderMeasureBase::toSbVec3f(axes[0] * size);
    coords[2] = MeasureGui::ViewProviderMeasureBase::toSbVec3f(axes[1] * (0.2 * size));
    coords[3] = MeasureGui::ViewProviderMeasureBase::toSbVec3f(axes[1] * size);
    coords[4] = MeasureGui::ViewProviderMeasureBase::toSbVec3f(axes[2] * (0.2 * size));
    coords[5] = MeasureGui::ViewProviderMeasureBase::toSbVec3f(axes[2] * size);

    lcsCoords->point.setNum(6);
    lcsCoords->point.setValues(0, 6, coords);

    // The 1.2 factor places labels slightly beyond each axis
    if (lcsLabel1Translation) {
        lcsLabel1Translation->translation.setValue(
            MeasureGui::ViewProviderMeasureBase::toSbVec3f(axes[0] * (1.2 * size))
        );
    }
    if (lcsLabel2Translation) {
        lcsLabel2Translation->translation.setValue(
            MeasureGui::ViewProviderMeasureBase::toSbVec3f(axes[1] * (1.2 * size))
        );
    }
    if (lcsLabel3Translation) {
        lcsLabel3Translation->translation.setValue(
            MeasureGui::ViewProviderMeasureBase::toSbVec3f(axes[2] * (1.2 * size))
        );
    }

    lcsSwitch->whichChild = SO_SWITCH_ALL;
}
