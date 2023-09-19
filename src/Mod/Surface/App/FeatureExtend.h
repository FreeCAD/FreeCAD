/***************************************************************************
 *   Copyright (c) 2017 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef SURFACE_FEATUREEXTEND_H
#define SURFACE_FEATUREEXTEND_H

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
    void handleChangedPropertyName(Base::XMLReader& reader,
                                   const char* TypeName,
                                   const char* PropName) override;

private:
    bool lockOnChangeMutex {false};
};

}  // Namespace Surface

#endif
