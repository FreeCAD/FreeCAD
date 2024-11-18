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

// clang-format off
#ifdef _MSC_VER
# define strdup _strdup
#endif

#include "PreCompiled.h"
#ifndef _PreComp_
# include <stdexcept>
# include <vector>

# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
#endif

// necessary for the feature despite not all are necessary for compilation
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <Mod/Part/App/TopoShapeFacePy.h>
#include <Mod/Part/App/TopoShapeEdgePy.h>

#include "MeshFlattening.h"
#include "MeshFlatteningLscmRelax.h"
#include "MeshFlatteningNurbs.h"


namespace py = pybind11;

const TopoDS_Face& getTopoDSFace(py::object* face)
{
    if (PyObject_TypeCheck(face->ptr(), &(Part::TopoShapeFacePy::Type)))
    {
        const Part::TopoShapeFacePy* f = static_cast<Part::TopoShapeFacePy*>(face->ptr());
        const TopoDS_Face& myFace = TopoDS::Face(f->getTopoShapePtr()->getShape());
        return myFace;
    }
    else
        throw std::invalid_argument("must be a face");
}

const TopoDS_Edge& getTopoDSEdge(py::object* edge)
{
    if (PyObject_TypeCheck(edge->ptr(), &(Part::TopoShapeEdgePy::Type)))
    {
        const Part::TopoShapeEdgePy* e = static_cast<Part::TopoShapeEdgePy*>(edge->ptr());
        const TopoDS_Edge& myEdge = TopoDS::Edge(e->getTopoShapePtr()->getShape());
        return myEdge;
    }
    else
        throw std::invalid_argument("must be an edge");
}

Py::Object makeEdge(const TopoDS_Edge& edge)
{
    return Py::asObject(new Part::TopoShapeEdgePy(new Part::TopoShape(edge)));
}

py::object makeFace(const TopoDS_Face& face)
{
    return py::cast(new Part::TopoShapeFacePy(new Part::TopoShape(face)));
}


FaceUnwrapper* FaceUnwrapper_constructor(py::object* face)
{
    const TopoDS_Face& myFace = getTopoDSFace(face);
    return new FaceUnwrapper(myFace);
}

ColMat<double, 3> interpolateFlatFacePy(FaceUnwrapper& instance, py::object* face)
{
    const TopoDS_Face& myFace = getTopoDSFace(face);
    return instance.interpolateFlatFace(myFace);
}



PYBIND11_MODULE(flatmesh, m)
{
    m.doc() = "functions to unwrapp faces/ meshes";

    py::class_<lscmrelax::LscmRelax>(m, "LscmRelax")
        .def(py::init<ColMat<double, 3>, ColMat<long, 3>, std::vector<long>>())
        .def("lscm", &lscmrelax::LscmRelax::lscm)
        .def("relax", &lscmrelax::LscmRelax::relax)
        .def("rotate_by_min_bound_area", &lscmrelax::LscmRelax::rotate_by_min_bound_area)
        .def("transform", &lscmrelax::LscmRelax::transform)
        .def_readonly("rhs", &lscmrelax::LscmRelax::rhs)
        .def_readonly("MATRIX", &lscmrelax::LscmRelax::MATRIX)
        .def_property_readonly("area", &lscmrelax::LscmRelax::get_area)
        .def_property_readonly("flat_area", &lscmrelax::LscmRelax::get_flat_area)
        .def_property_readonly("flat_vertices", [](lscmrelax::LscmRelax& L){return L.flat_vertices.transpose();}, py::return_value_policy::copy)
        .def_property_readonly("flat_vertices_3D", &lscmrelax::LscmRelax::get_flat_vertices_3D);

    py::class_<nurbs::NurbsBase2D>(m, "NurbsBase2D")
        .def(py::init<Eigen::VectorXd, Eigen::VectorXd, Eigen::VectorXd, int, int>())
        .def_readonly("u_knots", &nurbs::NurbsBase2D::u_knots)
        .def_readonly("weights", &nurbs::NurbsBase2D::weights)
        .def_readonly("degree_u", &nurbs::NurbsBase2D::degree_u)
//         .def_readonly("v_knots", &nurbs::NurbsBase2D::u_knots)
        .def_readonly("degree_v", &nurbs::NurbsBase2D::degree_u)
        .def("getUVMesh", &nurbs::NurbsBase2D::getUVMesh)
        .def("computeFirstDerivatives", &nurbs::NurbsBase2D::computeFirstDerivatives)
        .def("getInfluenceVector", &nurbs::NurbsBase2D::getInfluenceVector)
        .def("getInfluenceMatrix", &nurbs::NurbsBase2D::getInfluenceMatrix)
        .def("getDuVector", &nurbs::NurbsBase2D::getDuVector)
        .def("getDuMatrix", &nurbs::NurbsBase2D::getDuMatrix)
        .def("getDvVector", &nurbs::NurbsBase2D::getDvVector)
        .def("getDvMatrix", &nurbs::NurbsBase2D::getDvMatrix)
        .def("interpolateUBS", &nurbs::NurbsBase2D::interpolateUBS);

    py::class_<nurbs::NurbsBase1D>(m, "NurbsBase1D")
        .def(py::init<Eigen::VectorXd, Eigen::VectorXd, int>())
        .def_readonly("u_knots", &nurbs::NurbsBase1D::u_knots)
        .def_readonly("weights", &nurbs::NurbsBase1D::weights)
        .def_readonly("degree_u", &nurbs::NurbsBase1D::degree_u)
        .def("getUMesh", &nurbs::NurbsBase1D::getUMesh)
        .def("computeFirstDerivatives", &nurbs::NurbsBase1D::computeFirstDerivatives)
        .def("getInfluenceVector", &nurbs::NurbsBase1D::getInfluenceVector)
        .def("getInfluenceMatrix", &nurbs::NurbsBase1D::getInfluenceMatrix)
        .def("getDuVector", &nurbs::NurbsBase1D::getDuVector)
        .def("getDuMatrix", &nurbs::NurbsBase1D::getDuMatrix)
        .def_static("getKnotSequence", &nurbs::NurbsBase1D::getKnotSequence)
        .def_static("getWeightList", &nurbs::NurbsBase1D::getWeightList);

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
// clang-format on
