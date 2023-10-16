/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <vector>
#endif

#include "PointsFeature.h"


using namespace Points;

//===========================================================================
// Feature
//===========================================================================

PROPERTY_SOURCE(Points::Feature, App::GeoFeature)

Feature::Feature()
{
    ADD_PROPERTY(Points, (PointKernel()));
}

short Feature::mustExecute() const
{
    return 0;
}

App::DocumentObjectExecReturn* Feature::execute()
{
    this->Points.touch();
    return App::DocumentObject::StdReturn;
}

void Feature::Restore(Base::XMLReader& reader)
{
    GeoFeature::Restore(reader);
}

void Feature::RestoreDocFile(Base::Reader& reader)
{
    // This gets only invoked if a points file has been added from Restore()
    Points.RestoreDocFile(reader);
}

void Feature::onChanged(const App::Property* prop)
{
    // if the placement has changed apply the change to the point data as well
    if (prop == &this->Placement) {
        this->Points.setTransform(this->Placement.getValue().toMatrix());
    }
    // if the point data has changed check and adjust the transformation as well
    else if (prop == &this->Points) {
        try {
            Base::Placement p;
            p.fromMatrix(this->Points.getTransform());
            if (p != this->Placement.getValue()) {
                this->Placement.setValue(p);
            }
        }
        catch (const Base::ValueError&) {
        }
    }

    GeoFeature::onChanged(prop);
}

// ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Points::FeatureCustom, Points::Feature)
/// @endcond

// explicit template instantiation
template class PointsExport FeatureCustomT<Points::Feature>;
}  // namespace App

// ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Points::FeaturePython, Points::Feature)
template<>
const char* Points::FeaturePython::getViewProviderName() const
{
    return "PointsGui::ViewProviderPython";
}
/// @endcond

// explicit template instantiation
template class PointsExport FeaturePythonT<Points::Feature>;
}  // namespace App
