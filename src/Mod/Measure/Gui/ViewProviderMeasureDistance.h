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

#include <QObject>

#include <Mod/Measure/MeasureGlobal.h>
#include "ViewProviderMeasureBase.h"

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


class SoCoordinate3;
class SoIndexedLineSet;

namespace MeasureGui
{

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
    void onChanged(const App::Property* prop) override;

private:
    SoCoordinate3* pCoords;
    SoIndexedLineSet* pLines;
    SoSwitch* pDeltaDimensionSwitch;

    SoSFVec3f fieldPosition1;
    SoSFVec3f fieldPosition2;

    SoSFFloat fieldDistance;


    SbMatrix getMatrix();
};

}  // namespace MeasureGui
