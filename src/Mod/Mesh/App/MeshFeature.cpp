/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/FeaturePythonPyImp.h>

#include "MeshFeature.h"
#include "MeshFeaturePy.h"


using namespace Mesh;


//===========================================================================
// Feature
//===========================================================================

PROPERTY_SOURCE(Mesh::Feature, App::GeoFeature)

Feature::Feature()
{
    ADD_PROPERTY_TYPE(Mesh, (MeshObject()), 0, App::Prop_Output, "The mesh kernel");
}

App::DocumentObjectExecReturn* Feature::execute()
{
    this->Mesh.touch();
    return App::DocumentObject::StdReturn;
}

PyObject* Feature::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new MeshFeaturePy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

void Feature::onChanged(const App::Property* prop)
{
    // if the placement has changed apply the change to the mesh data as well
    if (prop == &this->Placement) {
        this->Mesh.setTransform(this->Placement.getValue().toMatrix());
    }
    // if the mesh data has changed check and adjust the transformation as well
    else if (prop == &this->Mesh) {
        try {
            Base::Placement p;
            p.fromMatrix(this->Mesh.getTransform());
            if (p != this->Placement.getValue()) {
                this->Placement.setValue(p);
            }
        }
        catch (const Base::ValueError&) {
        }
    }

    // Note: If the Mesh property has changed the property and this object are marked as 'touched'
    // but no recomputation of this objects needs to be done because the Mesh property is regarded
    // as output of a previous recomputation The mustExecute() method returns 0 in that case.
    // However, objects that reference this object the Mesh property can be an input parameter.
    // As this object and the property are touched such objects can check this and return a value 1
    // (or -1) in their mustExecute() to be recomputed the next time the document recomputes itself.
    DocumentObject::onChanged(prop);
}

// ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Mesh::FeatureCustom, Mesh::Feature)
/// @endcond

// explicit template instantiation
template class MeshExport FeatureCustomT<Mesh::Feature>;
}  // namespace App

// ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Mesh::FeaturePython, Mesh::Feature)
template<>
const char* Mesh::FeaturePython::getViewProviderName() const
{
    return "MeshGui::ViewProviderPython";
}
template<>
PyObject* Mesh::FeaturePython::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<Mesh::MeshFeaturePy>(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class MeshExport FeaturePythonT<Mesh::Feature>;
}  // namespace App
