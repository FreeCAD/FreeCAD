/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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
#include <SMESH_Mesh.hxx>
#endif

#include <App/FeaturePythonPyImp.h>
#include <App/GeoFeaturePy.h>
#include <Mod/Part/App/PartFeature.h>

#include "FemMesh.h"
#include "FemMeshShapeObject.h"


using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemMeshShapeBaseObject, Fem::FemMeshObject)

FemMeshShapeBaseObject::FemMeshShapeBaseObject()
{
    ADD_PROPERTY_TYPE(
        Shape,
        (nullptr),
        "FEM Mesh",
        Prop_None,
        "Geometry object, the mesh is made from. The geometry object has to have a Shape.");

    Shape.setScope(LinkScope::Global);
}

FemMeshShapeBaseObject::~FemMeshShapeBaseObject() = default;

// ------------------------------------------------------------------------

PROPERTY_SOURCE(Fem::FemMeshShapeObject, Fem::FemMeshShapeBaseObject)

FemMeshShapeObject::FemMeshShapeObject() = default;

FemMeshShapeObject::~FemMeshShapeObject() = default;

App::DocumentObjectExecReturn* FemMeshShapeObject::execute()
{
    Fem::FemMesh newMesh;

    Part::Feature* feat = Shape.getValue<Part::Feature*>();
    TopoDS_Shape shape = feat->Shape.getValue();

    newMesh.getSMesh()->ShapeToMesh(shape);
    newMesh.setStandardHypotheses();

    newMesh.compute();

    // set the value to the object
    FemMesh.setValue(newMesh);

    return App::DocumentObject::StdReturn;
}

// Python feature ---------------------------------------------------------

namespace App
{

PROPERTY_SOURCE_TEMPLATE(Fem::FemMeshShapeBaseObjectPython, Fem::FemMeshShapeBaseObject)

template<>
const char* Fem::FemMeshShapeBaseObjectPython::getViewProviderName() const
{
    return "FemGui::ViewProviderFemMeshShapeBasePython";
}

template<>
PyObject* Fem::FemMeshShapeBaseObjectPython::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::asObject(new App::FeaturePythonPyT<App::GeoFeaturePy>(this));
    }
    return Py::new_reference_to(PythonObject);
}

// explicit template instantiation
template class FemExport FeaturePythonT<Fem::FemMeshShapeBaseObject>;

}  // namespace App
