/***************************************************************************
 *   Copyright (c) 2017 Lorenz Lechner                                     *
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
#include <Mod/Part/App/TopoShapeFacePy.h>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/Sparse>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include <pybind11/numpy.h>
#include <pybind11/eigen.h>

#include <memory>
#include <vector>
#include <tuple>
#include <map>
#include <stdexcept>

#include "MeshFlattening.h"
#include "MeshFlatteningLscmRelax.h"
#include "MeshFlatteningNurbs.h"

#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>



namespace py = pybind11;

// void FaceUnwrapper_constructor(FaceUnwrapper& instance, Part::TopoShapeFacePy Face)
FaceUnwrapper* FaceUnwrapper_constructor(py::object face)
{
    if (PyObject_TypeCheck(face.ptr(), &(Part::TopoShapeFacePy::Type)))
    {
        const Part::TopoShapeFacePy* f = static_cast<Part::TopoShapeFacePy*>(face.ptr());
        const TopoDS_Face& myFace = TopoDS::Face(f->getTopoShapePtr()->getShape());
        return new FaceUnwrapper(myFace);
    }
    else
        throw std::invalid_argument("FaceUnwrapper should be initialized with Part.Face");
}

ColMat<double, 3> interpolateFlatFacePy(FaceUnwrapper& instance, py::object face)
{
    std::cout << face.ptr()->ob_type->tp_name << std::endl;
    std::cout << Part::TopoShapeFacePy::Type.tp_name << std::endl;
    if (PyObject_TypeCheck(face.ptr(), &(Part::TopoShapeFacePy::Type)))
    {
        const Part::TopoShapeFacePy* f = static_cast<Part::TopoShapeFacePy*>(face.ptr());
        const TopoDS_Face& myFace = TopoDS::Face(f->getTopoShapePtr()->getShape());
        return instance.interpolateFlatFace(myFace);
    }
    else
        throw std::invalid_argument("FaceUnwrapper.interpolateNurbs should be initialized with Part.Face");
}

PYBIND11_MODULE(flatmesh, m)
{
    m.doc() = "functions to unwrapp faces/ meshes";
    
    py::class_<LscmRelax>(m, "LscmRelax")
        .def(py::init<ColMat<double, 3>, ColMat<long, 3>, std::vector<long>>())
        .def("lscm", &LscmRelax::lscm)
        .def("relax", &LscmRelax::relax)
        .def("rotate_by_min_bound_area", &LscmRelax::rotate_by_min_bound_area)
        .def("transform", &LscmRelax::transform)
        .def_readonly("rhs", &LscmRelax::rhs)
        .def_readonly("MATRIX", &LscmRelax::MATRIX)
        .def_property_readonly("area", &LscmRelax::get_area)
        .def_property_readonly("flat_area", &LscmRelax::get_flat_area)
        .def_property_readonly("flat_vertices", [](LscmRelax& L){return L.flat_vertices.transpose();}, py::return_value_policy::copy)
        .def_property_readonly("flat_vertices_3D", &LscmRelax::get_flat_vertices_3D);

    py::class_<nurbs::NurbsBase2D>(m, "NurbsBase2D")
        .def(py::init<Eigen::VectorXd, Eigen::VectorXd, Eigen::VectorXd, int, int>())
        .def("computeFirstDerivatives", &nurbs::NurbsBase2D::computeFirstDerivatives)
        .def("getInfluenceVector", &nurbs::NurbsBase2D::getInfluenceVector)
        .def("getInfluenceMatrix", &nurbs::NurbsBase2D::getInfluenceMatrix)
        .def("getDuVector", &nurbs::NurbsBase2D::getDuVector)
        .def("getDuMatrix", &nurbs::NurbsBase2D::getDuMatrix)
        .def("getDvVector", &nurbs::NurbsBase2D::getDvVector)
        .def("getDvMatrix", &nurbs::NurbsBase2D::getDvMatrix);

    py::class_<nurbs::NurbsBase1D>(m, "NurbsBase1D")
        .def(py::init<Eigen::VectorXd, Eigen::VectorXd, int>())
        .def("computeFirstDerivatives", &nurbs::NurbsBase1D::computeFirstDerivatives)
        .def("getInfluenceVector", &nurbs::NurbsBase1D::getInfluenceVector)
        .def("getInfluenceMatrix", &nurbs::NurbsBase1D::getInfluenceMatrix)
        .def("getDuVector", &nurbs::NurbsBase1D::getDuVector)
        .def("getDuMatrix", &nurbs::NurbsBase1D::getDuMatrix);

    py::class_<FaceUnwrapper>(m, "FaceUnwrapper")
        .def(py::init(&FaceUnwrapper_constructor))
        .def(py::init<ColMat<double, 3>, ColMat<long, 3>>())
        .def("findFlatNodes", &FaceUnwrapper::findFlatNodes)
        .def("interpolateFlatFace", &interpolateFlatFacePy)
        .def("getFlatBoundaryNodes", &FaceUnwrapper::getFlatBoundaryNodes)
        .def_readonly("tris", &FaceUnwrapper::tris)
        .def_readonly("nodes", &FaceUnwrapper::xyz_nodes)
        .def_readonly("uv_nodes", &FaceUnwrapper::uv_nodes)
        .def_readonly("ze_nodes", &FaceUnwrapper::ze_nodes)
        .def_readonly("ze_poles", &FaceUnwrapper::ze_poles)
        .def_readonly("A", &FaceUnwrapper::A);
};
