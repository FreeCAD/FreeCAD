/***************************************************************************
 *   Copyright (c) 2014 Matteo Grellier <matteogrellier@gmail.com>         *
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

#ifndef FEATURE_BLEND_CURVE_H
#define FEATURE_BLEND_CURVE_H

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
    BlendPoint GetBlendPoint(App::PropertyLinkSub& link,
                             App::PropertyFloatConstraint& param,
                             App::PropertyIntegerConstraint& Continuity);
    double RelativeToRealParameters(double, double, double);

protected:
    void onChanged(const App::Property* prop) override;
};

}  // Namespace Surface

#endif
