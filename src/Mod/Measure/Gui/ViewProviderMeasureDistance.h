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


#ifndef MEASUREGUI_VIEWPROVIDERMEASUREDISTANCE_H
#define MEASUREGUI_VIEWPROVIDERMEASUREDISTANCE_H

#include <Mod/Measure/MeasureGlobal.h>

#include <QObject>

#include <Mod/Measure/App/MeasureDistance.h>

#include "ViewProviderMeasureBase.h"

class SoCoordinate3;
class SoIndexedLineSet;

namespace MeasureGui
{


class MeasureGuiExport ViewProviderMeasureDistance : public MeasureGui::ViewProviderMeasureBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeasureGui::ViewProviderMeasureDistance);

public:
    /// Constructor
    ViewProviderMeasureDistance();
    ~ViewProviderMeasureDistance() override;

    Measure::MeasureDistance* getMeasureDistance();
    void redrawAnnotation() override;
    void positionAnno(const Measure::MeasureBase* measureObject) override;

protected:
    Base::Vector3d getTextDirection(Base::Vector3d elementDirection, double tolerance = defaultTolerance) override;

private:
    SoCoordinate3    * pCoords;
    SoIndexedLineSet * pLines;

    SoSFFloat         fieldDistance;

    SbMatrix getMatrix();
};

} //namespace MeasureGui


#endif // MEASUREGUI_VIEWPROVIDERMEASUREDISTANCE_H
