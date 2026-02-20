// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
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


#ifndef GUI_VIEWPROVIDERMEASUREANGLE_H
#define GUI_VIEWPROVIDERMEASUREANGLE_H

#include <Inventor/fields/SoMFFloat.h>
#include <Mod/Measure/MeasureGlobal.h>

#include <QObject>

#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFMatrix.h>
#include <Inventor/fields/SoSFVec3f.h>

#include <Mod/Measure/App/MeasureAngle.h>

#include "ViewProviderMeasureBase.h"

// NOLINTBEGIN
class SoText2;
class SoTranslation;
class SoCoordinate3;
class SoIndexedLineSet;
class SoTransform;
class SoSeparator;
class SoCone;
class SoLineSet;
class SoBaseColor;
class SoDrawStyle;
// NOLINTEND

namespace MeasureGui
{

class MeasureGuiExport ViewProviderMeasureAngle: public MeasureGui::ViewProviderMeasureBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeasureGui::ViewProviderMeasureAngle);

public:
    App::PropertyBool IsFlipped;
    /// Constructor
    ViewProviderMeasureAngle();

    Measure::MeasureAngle* getMeasureAngle();
    void redrawAnnotation() override;
    void positionAnno(const Measure::MeasureBase* measureObject) override;

private:
    // Fields
    SoSFVec3f element1Location;
    SoSFVec3f element2Location;
    SoSFFloat fieldAngle;  // radians.
    SoSFFloat sectorArcRotation;
    SoSFBool isArcFlipped;
    SoSFVec3f normalStartPoint1;  // start point for normal line 1
    SoSFVec3f normalStartPoint2;  // start point for normal line 2
    SoSFInt32 visualMode;         // 0 for normal, 1 for skew

    SbMatrix getMatrix();

    void onLabelMoved() override;
    void onLabelMoveEnd() override;
    void onChanged(const App::Property* prop) override;
};


}  // namespace MeasureGui


#endif  // GUI_VIEWPROVIDERMEASUREANGLE_H
