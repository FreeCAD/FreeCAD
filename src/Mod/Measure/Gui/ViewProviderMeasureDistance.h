// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *   Copyright (c) 2013 Thomas Anderson <blobfish[at]gmx.com>              *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#pragma once

#include <functional>
#include <vector>

#include <QObject>

#include <Gui/SoLabelNodes.h>
#include <Mod/Measure/MeasureGlobal.h>
#include "ViewProviderMeasureBase.h"

#include <Inventor/SbBox2f.h>
#include <Inventor/engines/SoSubEngine.h>
#include <Inventor/engines/SoEngine.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFMatrix.h>
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/nodekits/SoSeparatorKit.h>


class SoCallback;
class SoCoordinate3;
class SoIndexedLineSet;

namespace MeasureGui
{

//! A frame label that moves down on the screen to keep clear of other labels
class DimensionLabel: public Gui::SoFrameLabel
{
    using inherited = Gui::SoFrameLabel;

    SO_NODE_HEADER(DimensionLabel);

public:
    using AnchorFunction = std::function<SbVec3f()>;

    DimensionLabel();
    static void initClass();
    void avoidOverlapWith(DimensionLabel* label);
    void avoidOverlapWith(Gui::SoFrameLabel* label, const AnchorFunction& anchorInWorld);
    void clearObstacles();

protected:
    ~DimensionLabel() override;
    void GLRender(SoGLRenderAction* action) override;

private:
    struct LaterLabel
    {
        Gui::SoFrameLabel* label;
        AnchorFunction anchorInWorld;
    };

    std::vector<DimensionLabel*> earlierLabels {};
    std::vector<LaterLabel> laterLabels {};
    SbBox2f screenRect;  // in viewport pixels
};

class DimensionLinear: public SoSeparatorKit
{
    SO_KIT_HEADER(DimensionLinear);

    SO_KIT_CATALOG_ENTRY_HEADER(transformation);
    SO_KIT_CATALOG_ENTRY_HEADER(annotate);
    SO_KIT_CATALOG_ENTRY_HEADER(leftArrow);
    SO_KIT_CATALOG_ENTRY_HEADER(rightArrow);
    SO_KIT_CATALOG_ENTRY_HEADER(line);
    SO_KIT_CATALOG_ENTRY_HEADER(textSep);

public:
    DimensionLinear();
    static void initClass();
    SbBool affectsState() const override;
    void setupDimension();
    void avoidLabelOverlapWith(DimensionLinear* dimension);
    void avoidLabelOverlapWith(
        Gui::SoFrameLabel* label,
        const DimensionLabel::AnchorFunction& anchorInWorld
    );
    void clearLabelObstacles();

    SoSFVec3f point1;
    SoSFVec3f point2;
    SoSFString text;
    SoSFColor dColor;
    SoSFColor backgroundColor;
    SoSFBool showArrows;
    SoSFFloat fontSize;

protected:
    SoSFRotation rotate;
    SoSFFloat length;
    SoSFVec3f origin;

private:
    ~DimensionLinear() override;

    DimensionLabel* label {nullptr};
};


class MeasureGuiExport ViewProviderMeasureDistance: public MeasureGui::ViewProviderMeasureBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeasureGui::ViewProviderMeasureDistance);

public:
    /// Constructor
    ViewProviderMeasureDistance();
    ~ViewProviderMeasureDistance() override;

    App::PropertyBool ShowDelta;

    void redrawAnnotation() override;
    void positionAnno(const Measure::MeasureBase* measureObject) override;

protected:
    Base::Vector3d getTextDirection(
        Base::Vector3d elementDirection,
        double tolerance = defaultTolerance
    ) override;
    // Keep the label draggable in the distance annotation's local measurement frame instead of
    // rotating the dragger into the current view plane.
    void onLabelMoveStart() override
    {}
    void onChanged(const App::Property* prop) override;

private:
    SoCoordinate3* pCoords;
    SoIndexedLineSet* pLines;
    SoSwitch* pDeltaDimensionSwitch;
    SoCallback* pGlobalMatrixCallback;

    SoSFVec3f fieldPosition1;
    SoSFVec3f fieldPosition2;

    SbMatrix globalMatrix {SbMatrix::identity()};

    SoSFFloat fieldDistance;


    SbMatrix getMatrix();
};

}  // namespace MeasureGui
