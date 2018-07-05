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
#include <Base/Interpreter.h>
#include <Mod/Part/App/TopoShapeFacePy.h>
#include <Mod/Part/App/TopoShapeEdgePy.h>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/Sparse>
#include <boost/python.hpp>
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
#include <boost/python/wrapper.hpp>
#include <boost/python/call.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/return_value_policy.hpp>

#include <memory>
#include <vector>
#include <tuple>
#include <map>
#include <stdexcept>

#include "MeshFlattening.h"
#include "MeshFlatteningLscmRelax.h"
#include "MeshFlatteningNurbs.h"

#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS.hxx>
#include <ShapeFix_Edge.hxx>



namespace py = boost::python;

const TopoDS_Face& getTopoDSFace(const py::object& face)
{
    if (PyObject_TypeCheck(face.ptr(), &(Part::TopoShapeFacePy::Type)))
    {
        const Part::TopoShapeFacePy* f = static_cast<Part::TopoShapeFacePy*>(face.ptr());
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

boost::shared_ptr<FaceUnwrapper> FaceUnwrapper_face(const py::object& face)
{
    const TopoDS_Face& myFace = getTopoDSFace(face);
    return boost::shared_ptr<FaceUnwrapper>(new FaceUnwrapper(myFace));
}

boost::shared_ptr<FaceUnwrapper> FaceUnwrapper_mesh(const py::object& points,
                                                    const py::object& facets)
{
    try {
        Py::Sequence l1(points.ptr());
        ColMat<double, 3> coords;
        coords.resize(l1.size(), 3);
        int row = 0;
        for (Py::Sequence::iterator it = l1.begin(); it != l1.end(); ++it) {
            Py::Sequence c(*it);
            int col = 0;
            for (Py::Sequence::iterator jt = c.begin(); jt != c.end(); ++jt, ++col) {
                double v = static_cast<double>(Py::Float(*jt));
                coords(row, col) = v;
            }
        }

        Py::Sequence l2(facets.ptr());
        ColMat<long, 3> triangles;
        triangles.resize(l2.size(), 3);
        for (Py::Sequence::iterator it = l2.begin(); it != l2.end(); ++it) {
            Py::Sequence c(*it);
            int col = 0;
            for (Py::Sequence::iterator jt = c.begin(); jt != c.end(); ++jt, ++col) {
                long v = static_cast<long>(Py::Long(*jt));
                triangles(row, col) = v;
            }
        }

        return boost::shared_ptr<FaceUnwrapper>(new FaceUnwrapper(coords, triangles));
    }
    catch (const Py::Exception&) {
        Base::PyException e;
        throw std::invalid_argument(e.what());
    }
}

boost::python::list interpolateFlatFacePy(FaceUnwrapper& instance, const py::object& face)
{
    const TopoDS_Face& myFace = getTopoDSFace(face);
    ColMat<double, 3> mat = instance.interpolateFlatFace(myFace);
    boost::python::list plist;
    auto cols = mat.cols();
    auto rows = mat.rows();
    for (int i=0; i<rows; i++) {
        boost::python::list vec;
        for (int j=0; j<cols; j++) {
            double c = mat.coeff(i, j);
            vec.append(c);
        }
        plist.append(vec);
    }
    return plist;
}

boost::python::list getFlatBoundaryNodesPy(FaceUnwrapper& instance)
{
    std::vector<ColMat<double, 3>> mat_array = instance.getFlatBoundaryNodes();

    boost::python::list ary;
    for (auto mat : mat_array) {
        boost::python::list plist;
        auto cols = mat.cols();
        auto rows = mat.rows();
        for (int i=0; i<rows; i++) {
            boost::python::list vec;
            for (int j=0; j<cols; j++) {
                double c = mat.coeff(i, j);
                vec.append(c);
            }
            plist.append(vec);
        }

        ary.append(plist);
    }
    return ary;
}



BOOST_PYTHON_MODULE(flatmesh)
{
    //m.doc() = "functions to unwrapp faces/ meshes";
    
    py::class_<lscmrelax::LscmRelax>("LscmRelax")
        .def(py::init<ColMat<double, 3>, ColMat<long, 3>, std::vector<long>>())
        .def("lscm", &lscmrelax::LscmRelax::lscm)
        .def("relax", &lscmrelax::LscmRelax::relax)
        .def("rotate_by_min_bound_area", &lscmrelax::LscmRelax::rotate_by_min_bound_area)
        .def("transform", &lscmrelax::LscmRelax::transform)
        .def_readonly("rhs", &lscmrelax::LscmRelax::rhs)
        .def_readonly("MATRIX", &lscmrelax::LscmRelax::MATRIX)
        .def_readonly("area", &lscmrelax::LscmRelax::get_area)
        .def_readonly("flat_area", &lscmrelax::LscmRelax::get_flat_area)
//        .def_readonly("flat_vertices", [](lscmrelax::LscmRelax& L){return L.flat_vertices.transpose();}, py::return_value_policy<py::copy_const_reference>())
        .def_readonly("flat_vertices_3D", &lscmrelax::LscmRelax::get_flat_vertices_3D);

    py::class_<nurbs::NurbsBase2D>("NurbsBase2D")
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

    py::class_<nurbs::NurbsBase1D>("NurbsBase1D")
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
        .add_static_property("getKnotSequence", &nurbs::NurbsBase1D::getKnotSequence)
        .add_static_property("getWeightList", &nurbs::NurbsBase1D::getWeightList);

    py::class_<FaceUnwrapper>("FaceUnwrapper")
        .def("__init__", py::make_constructor(&FaceUnwrapper_face))
        .def("__init__", py::make_constructor(&FaceUnwrapper_mesh))
        .def(py::init<ColMat<double, 3>, ColMat<long, 3>>())
        .def("findFlatNodes", &FaceUnwrapper::findFlatNodes)
        .def("interpolateFlatFace", &interpolateFlatFacePy)
        .def("getFlatBoundaryNodes", &getFlatBoundaryNodesPy)
        .def_readonly("tris", &FaceUnwrapper::tris)
        .def_readonly("nodes", &FaceUnwrapper::xyz_nodes)
        .def_readonly("uv_nodes", &FaceUnwrapper::uv_nodes)
        .def_readonly("ze_nodes", &FaceUnwrapper::ze_nodes)
        .def_readonly("ze_poles", &FaceUnwrapper::ze_poles)
        .def_readonly("A", &FaceUnwrapper::A);
}
