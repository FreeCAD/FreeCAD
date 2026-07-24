// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Andrew Shkolik <shkolik@gmail.com>                  *
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

#ifndef FEATURE_GORDONSURFACE_H
#define FEATURE_GORDONSURFACE_H

#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <Mod/Part/App/FeaturePartSpline.h>
#include <Mod/Surface/SurfaceGlobal.h>

#include <Geom_BSplineCurve.hxx>

namespace Surface
{

class SurfaceExport GordonSurface: public Part::Spline
{
    PROPERTY_HEADER_WITH_OVERRIDE(Surface::GordonSurface);

public:
    GordonSurface();

    App::PropertyLinkSubList ProfileEdges;    // Profiles
    App::PropertyLinkSubList GuideEdges;      // Guides
    App::PropertyBoolList ProfileDirections;  // Profile Directions
    App::PropertyBoolList GuideDirections;    // Guide Directions
    App::PropertyFloat Tolerance;             // Tolerance

    App::PropertyBool UseNativeAlgorithm;
    App::PropertyBool ParallelMode;
    App::PropertyEnumeration ApproximationMode;

    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    void onChanged(const App::Property* prop) override;
    const char* getViewProviderName() const override
    {
        return "SurfaceGui::ViewProviderGordonSurface";
    }

protected:
    void onDocumentRestored() override;

private:
    std::vector<Handle(Geom_BSplineCurve)> getCurves(
        const App::PropertyLinkSubList& edges,
        const App::PropertyBoolList& directions
    );

    Handle(
        Geom_BSplineCurve
    ) moveCurveSeam(const Handle(Geom_BSplineCurve) & curve, double targetParameter) const;

    void prepareCurvesNetwork(
        std::vector<Handle(Geom_BSplineCurve)>& profiles,
        std::vector<Handle(Geom_BSplineCurve)>& guides,
        bool addSeamCopy
    ) const;
};

}  // Namespace Surface

#endif
