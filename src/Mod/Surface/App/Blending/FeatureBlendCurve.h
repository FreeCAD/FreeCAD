// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Matteo Grellier
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

#pragma once

#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <Mod/Part/App/FeaturePartSpline.h>
#include <Mod/Surface/App/Blending/BlendPoint.h>
#include <Mod/Surface/SurfaceGlobal.h>

namespace Surface
{

class SurfaceExport FeatureBlendCurve: public Part::Spline
{
    PROPERTY_HEADER_WITH_OVERRIDE(Surface::FeatureBlendCurve);

public:
    FeatureBlendCurve();

    App::PropertyLinkSub StartEdge;
    App::PropertyFloatConstraint StartParameter;
    App::PropertyIntegerConstraint StartContinuity;
    App::PropertyFloatConstraint StartSize;

    App::PropertyLinkSub EndEdge;
    App::PropertyFloatConstraint EndParameter;
    App::PropertyIntegerConstraint EndContinuity;
    App::PropertyFloatConstraint EndSize;

    Standard_Integer maxDegree;

    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    const char* getViewProviderName() const override
    {
        return "SurfaceGui::ViewProviderBlendCurve";
    }

private:
    BlendPoint GetBlendPoint(
        App::PropertyLinkSub& link,
        App::PropertyFloatConstraint& param,
        App::PropertyIntegerConstraint& Continuity
    );
    double RelativeToRealParameters(double, double, double);

protected:
    void onChanged(const App::Property* prop) override;
};

}  // Namespace Surface
