/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

#include <App/DocumentObjectPy.h>

#include "FeaturePath.h"


using namespace Path;

PROPERTY_SOURCE(Path::Feature, App::GeoFeature)


Feature::Feature()
{
    ADD_PROPERTY_TYPE(Path,(Path::Toolpath()),"Base",App::Prop_None,"The path data of this feature");
}

Feature::~Feature()
{
}

short Feature::mustExecute() const
{
    return App::GeoFeature::mustExecute();
}

PyObject *Feature::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new App::DocumentObjectPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

void Feature::onChanged(const App::Property* prop)
{
    App::GeoFeature::onChanged(prop);
}

// Python Path feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Path::FeaturePython, Path::Feature)
template<> const char* Path::FeaturePython::getViewProviderName() const {
    return "PathGui::ViewProviderPathPython";
}
/// @endcond

// explicit template instantiation
template class PathExport FeaturePythonT<Path::Feature>;
}
