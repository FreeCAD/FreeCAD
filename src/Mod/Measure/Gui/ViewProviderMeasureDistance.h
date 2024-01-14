/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
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
    void onGuiInit(const Measure::MeasureBase* measureObject) override;

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
