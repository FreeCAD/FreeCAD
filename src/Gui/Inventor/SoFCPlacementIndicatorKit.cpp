// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
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


#include "PreCompiled.h"

#ifndef _PreComp_
#include <sstream>

#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoFontStyle.h>
#endif

#include "SoFCPlacementIndicatorKit.h"

#include "So3DAnnotation.h"
#include "SoAxisCrossKit.h"

#include <SoTextLabel.h>
#include <Utilities.h>
#include <ViewParams.h>
#include <ViewProvider.h>
#include <Base/Color.h>
#include <Base/Placement.h>
#include <Base/Vector3D.h>

namespace Gui
{

SO_KIT_SOURCE(SoFCPlacementIndicatorKit);

SoFCPlacementIndicatorKit::SoFCPlacementIndicatorKit()
{
    SO_KIT_CONSTRUCTOR(SoFCPlacementIndicatorKit);

    SO_KIT_ADD_CATALOG_ENTRY(root, SoShapeScale, false, this, "", true);

    SO_KIT_INIT_INSTANCE();

    SO_NODE_ADD_FIELD(coloredAxis, (true));
    SO_NODE_ADD_FIELD(scaleFactor, (ViewParams::instance()->getPlacementIndicatorScale()));
    SO_NODE_ADD_FIELD(axisLength, (axisLengthDefault));
    SO_NODE_ADD_FIELD(parts, (AxisCross));
    SO_NODE_ADD_FIELD(axes, (AllAxes));

    SO_NODE_DEFINE_ENUM_VALUE(Part, Axes);
    SO_NODE_DEFINE_ENUM_VALUE(Part, ArrowHeads);
    SO_NODE_DEFINE_ENUM_VALUE(Part, Labels);
    SO_NODE_DEFINE_ENUM_VALUE(Part, AxisCross);
    SO_NODE_DEFINE_ENUM_VALUE(Part, PlaneIndicator);
    SO_NODE_DEFINE_ENUM_VALUE(Part, OriginIndicator);
    SO_NODE_DEFINE_ENUM_VALUE(Part, AllParts);
    SO_NODE_SET_SF_ENUM_TYPE(parts, Part);

    SO_NODE_DEFINE_ENUM_VALUE(Axes, X);
    SO_NODE_DEFINE_ENUM_VALUE(Axes, Y);
    SO_NODE_DEFINE_ENUM_VALUE(Axes, Z);
    SO_NODE_DEFINE_ENUM_VALUE(Axes, AllAxes);
    SO_NODE_SET_SF_ENUM_TYPE(axes, Axes);

    auto root = SO_GET_ANY_PART(this, "root", SoShapeScale);
    root->scaleFactor.connectFrom(&scaleFactor);

    recomputeGeometry();
}

void SoFCPlacementIndicatorKit::initClass()
{
    SO_KIT_INIT_CLASS(SoFCPlacementIndicatorKit, SoBaseKit, "BaseKit");
}

void SoFCPlacementIndicatorKit::notify(SoNotList* l)
{
    SoField* field = l->getLastField();

    if (field == &parts || field == &axes || field == &axisLength) {
        // certainly this is not the fastest way to recompute the geometry as it does recreate
        // everything from the scratch. It is however very easy to implement and this node should
        // not really change too often so the performance penalty is better than making code that
        // is harder to maintain.
        recomputeGeometry();
        return;
    }

    SoBaseKit::notify(l);
}

void SoFCPlacementIndicatorKit::recomputeGeometry()
{
    auto root = SO_GET_ANY_PART(this, "root", SoShapeScale);
    root->setPart("shape", createGeometry());
}

SoSeparator* SoFCPlacementIndicatorKit::createOriginIndicator()
{
    const uint32_t originColor = ViewParams::instance()->getOriginColor();

    auto sep = new SoSeparator();

    auto pcBaseColor = new SoBaseColor();
    pcBaseColor->rgb.setValue(Base::Color::fromPackedRGBA<SbColor>(originColor));

    auto pcSphere = new SoSphere();
    // the factor aligns radius of sphere and arrow head visually, without it, it looks too small
    constexpr float visualAdjustmentFactor = 1.2F;
    pcSphere->radius = arrowHeadRadius * visualAdjustmentFactor;

    sep->addChild(pcBaseColor);
    sep->addChild(pcSphere);

    return sep;
}

SoSeparator* SoFCPlacementIndicatorKit::createGeometry()
{
    const uint32_t neutralColor = ViewParams::instance()->getNeutralColor();

    auto sep = new SoSeparator();

    auto pcBaseColor = new SoBaseColor();
    pcBaseColor->rgb.setValue(Base::Color::fromPackedRGBA<SbColor>(neutralColor));

    auto pcLightModel = new SoLightModel();
    pcLightModel->model = SoLightModel::BASE_COLOR;

    sep->addChild(pcBaseColor);
    sep->addChild(pcLightModel);

    if (parts.getValue() & PlaneIndicator) {
        sep->addChild(createPlaneIndicator());
    }

    if (parts.getValue() & OriginIndicator) {
        sep->addChild(createOriginIndicator());
    }

    if (parts.getValue() & Axes) {
        sep->addChild(createAxes());
    }

    return sep;
}

SoSeparator* SoFCPlacementIndicatorKit::createAxes()
{
    const auto cylinderOffset = axisLength.getValue() / 2.F;

    const auto createAxis = [&](const char* label,
                                Base::Vector3d axis,
                                uint32_t packedColor,
                                const double offset) {
        Base::Color axisColor(packedColor);
        Base::Rotation rotation(Base::Vector3d::UnitY, axis);

        auto sep = new SoSeparator;

        auto pcTranslate = new SoTransform();
        pcTranslate->translation.setValue(
            Base::convertTo<SbVec3f>((cylinderOffset + offset) * axis));
        pcTranslate->rotation.setValue(Base::convertTo<SbRotation>(rotation));

        auto pcArrowShaft = new SoCylinder();
        pcArrowShaft->radius = axisThickness / 2.F;
        pcArrowShaft->height = axisLength;

        if (coloredAxis.getValue()) {
            auto pcBaseColor = new SoBaseColor();
            pcBaseColor->rgb.setValue(Base::convertTo<SbColor>(axisColor));

            sep->addChild(pcBaseColor);
        }

        sep->addChild(pcTranslate);
        sep->addChild(pcArrowShaft);

        if (parts.getValue() & ArrowHeads) {
            auto pcArrowHeadTranslation = new SoTranslation();
            pcArrowHeadTranslation->translation.setValue(0.0, cylinderOffset, 0.0);

            auto pcArrowHead = new SoCone();
            pcArrowHead->bottomRadius = arrowHeadRadius;
            pcArrowHead->height = arrowHeadHeight;

            auto pcArrowHeadSeparator = new SoSeparator();
            pcArrowHeadSeparator->addChild(pcArrowHeadTranslation);
            pcArrowHeadSeparator->addChild(pcArrowHead);

            sep->addChild(pcArrowHeadSeparator);
        }

        if (parts.getValue() & Labels) {
            auto pcLabelSeparator = new SoSeparator();

            auto pcLabelTranslation = new SoTranslation();
            pcLabelTranslation->translation.setValue(0.0, cylinderOffset + labelOffset, 0.0);
            pcLabelSeparator->addChild(pcLabelTranslation);

            auto pcAxisLabel = new SoFrameLabel();
            pcAxisLabel->string.setValue(label);
            pcAxisLabel->textColor.setValue(1.0, 1.0, 1.0);
            pcAxisLabel->horAlignment = SoImage::CENTER;
            pcAxisLabel->vertAlignment = SoImage::HALF;
            pcAxisLabel->border = false;
            pcAxisLabel->frame = false;
            pcAxisLabel->textUseBaseColor = true;
            pcAxisLabel->size = labelFontSize;

            pcLabelSeparator->addChild(pcAxisLabel);

            sep->addChild(pcLabelSeparator);
        }

        return sep;
    };

    double additionalAxisMargin = (parts.getValue() & OriginIndicator) ? axisThickness * 4 : 0;
    double xyOffset = (parts.getValue() & PlaneIndicator)
        ? planeIndicatorRadius + planeIndicatorMargin
        : axisMargin + additionalAxisMargin;

    auto sep = new SoSeparator;

    if (axes.getValue() & X) {
        sep->addChild(createAxis("X",
                                 Base::Vector3d::UnitX,
                                 ViewParams::instance()->getAxisXColor(),
                                 xyOffset));
    }

    if (axes.getValue() & Y) {
        sep->addChild(createAxis("Y",
                                 Base::Vector3d::UnitY,
                                 ViewParams::instance()->getAxisYColor(),
                                 xyOffset));
    }

    if (axes.getValue() & Z) {
        double zOffset = (parts.getValue() & PlaneIndicator)
            ? planeIndicatorMargin
            : axisMargin + additionalAxisMargin;

        sep->addChild(createAxis("Z",
                                 Base::Vector3d::UnitZ,
                                 ViewParams::instance()->getAxisZColor(),
                                 zOffset));
    }

    return sep;
}

SoSeparator* SoFCPlacementIndicatorKit::createPlaneIndicator()
{
    // cylinders are aligned with Y axis for some reason
    auto rotation = Base::Rotation(Base::Vector3d::UnitY, Base::Vector3d::UnitZ);

    auto pcRotation = new SoRotation();
    pcRotation->rotation = Base::convertTo<SbRotation>(rotation);

    auto pcComplexity = new SoComplexity();
    pcComplexity->value = 1.0f;

    auto pcCylinder = new SoCylinder();
    pcCylinder->height = 0.f;
    pcCylinder->radius = planeIndicatorRadius;
    pcCylinder->parts = SoCylinder::TOP;

    const auto gray = Base::Color(0.75f, 0.75f, 0.75f);

    auto pcMaterial = new SoMaterial();
    pcMaterial->diffuseColor.setValue(Base::convertTo<SbColor>(gray));
    pcMaterial->ambientColor.connectFrom(&pcMaterial->diffuseColor);
    pcMaterial->transparency = planeIndicatorTransparency;

    auto sep = new SoSeparator;

    sep->addChild(pcRotation);
    sep->addChild(pcMaterial);
    sep->addChild(pcComplexity);
    sep->addChild(pcCylinder);

    return sep;
}

SoFCPlacementIndicatorKit::~SoFCPlacementIndicatorKit() = default;

}  // namespace Gui
