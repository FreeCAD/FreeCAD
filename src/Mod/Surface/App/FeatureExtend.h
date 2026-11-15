// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017 Werner Mayer
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
#include <Mod/Part/App/FeaturePartSpline.h>
#include <Mod/Surface/SurfaceGlobal.h>


namespace Surface
{

class SurfaceExport Extend: public Part::Spline
{
    PROPERTY_HEADER_WITH_OVERRIDE(Surface::Extend);

public:
    Extend();

    App::PropertyLinkSub Face;
    App::PropertyFloatConstraint Tolerance;
    App::PropertyFloatConstraint ExtendUNeg;
    App::PropertyFloatConstraint ExtendUPos;
    App::PropertyBool ExtendUSymetric;
    App::PropertyFloatConstraint ExtendVNeg;
    App::PropertyFloatConstraint ExtendVPos;
    App::PropertyBool ExtendVSymetric;
    App::PropertyIntegerConstraint SampleU;
    App::PropertyIntegerConstraint SampleV;

    // recalculate the feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "SurfaceGui::ViewProviderExtend";
    }

protected:
    void onChanged(const App::Property* prop) override;
    void handleChangedPropertyName(
        Base::XMLReader& reader,
        const char* TypeName,
        const char* PropName
    ) override;

private:
    bool lockOnChangeMutex {false};
};

}  // Namespace Surface
