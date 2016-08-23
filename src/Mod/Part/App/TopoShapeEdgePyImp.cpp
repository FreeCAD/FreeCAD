/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2008     *
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
# include <algorithm>
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BRepLProp_CLProps.hxx>
# include <BRepLProp_CurveTool.hxx>
# include <GProp_GProps.hxx>
# include <GProp_PrincipalProps.hxx>
# include <Geom_Circle.hxx>
# include <Geom_Curve.hxx>
# include <Geom_Ellipse.hxx>
# include <Geom_Hyperbola.hxx>
# include <Geom_Parabola.hxx>
# include <Geom_Line.hxx>
# include <Geom_TrimmedCurve.hxx>
# include <Geom_BezierCurve.hxx>
# include <Geom_BSplineCurve.hxx>
# include <gp_Circ.hxx>
# include <gp_Elips.hxx>
# include <gp_Hypr.hxx>
# include <gp_Parab.hxx>
# include <gp_Lin.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Shape.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Vertex.hxx>
# include <ShapeAnalysis_Edge.hxx>
# include <Standard_Failure.hxx>
#endif

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <GCPnts_UniformDeflection.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <GCPnts_QuasiUniformAbscissa.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>

#include <Base/Vector3D.h>
#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include "Tools.h"
#include "OCCError.h"
#include "TopoShape.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeVertexPy.h"
#include "TopoShapeWirePy.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeEdgePy.cpp"

#include "Geometry.h"
#include "GeometryPy.h"
#include "LinePy.h"
#include "CirclePy.h"
#include "EllipsePy.h"
#include "HyperbolaPy.h"
#include "ParabolaPy.h"
#include "BezierCurvePy.h"
#include "BSplineCurvePy.h"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string TopoShapeEdgePy::representation(void) const
{
    std::stringstream str;
    str << "<Edge object at " << getTopoShapePtr() << ">";

    return str.str();
}

PyObject *TopoShapeEdgePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of TopoShapeEdgePy and the Twin object 
    return new TopoShapeEdgePy(new TopoShape);
}

// constructor method
int TopoShapeEdgePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject *pcObj, *pcObj2;
    double first=DBL_MAX, last=DBL_MAX;
    if (PyArg_ParseTuple(args, "O!|dd", &(Part::GeometryPy::Type), &pcObj, &first, &last)) {
        Geometry* geom = static_cast<GeometryPy*>(pcObj)->getGeometryPtr();
        Handle_Geom_Curve curve = Handle_Geom_Curve::DownCast(geom->handle());
        if (curve.IsNull()) {
            PyErr_SetString(PartExceptionOCCError, "geometry is not a curve type");
            return -1;
        }

        if (first==DBL_MAX)
            first = curve->FirstParameter();
        if (last==DBL_MAX)
            last = curve->LastParameter();

        try {
            BRepBuilderAPI_MakeEdge mkEdge(curve, first, last);
            getTopoShapePtr()->setShape(mkEdge.Edge());
            return 0;
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
            return -1;
        }
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &pcObj)) {
        TopoShape* shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr();
        if (shape && !shape->getShape().IsNull() && shape->getShape().ShapeType() == TopAbs_EDGE) {
            this->getTopoShapePtr()->setShape(shape->getShape());
            return 0;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Shape is not an edge");
            return -1;
        }
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!O!", &(Part::TopoShapeVertexPy::Type), &pcObj,
                                       &(Part::TopoShapeVertexPy::Type), &pcObj2)) {
        TopoShape* shape1 = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr();
        TopoShape* shape2 = static_cast<TopoShapePy*>(pcObj2)->getTopoShapePtr();
        const TopoDS_Vertex& v1 = TopoDS::Vertex(shape1->getShape());
        const TopoDS_Vertex& v2 = TopoDS::Vertex(shape2->getShape());

        try {
            BRepBuilderAPI_MakeEdge mkEdge(v1, v2);
            getTopoShapePtr()->setShape(mkEdge.Edge());
            return 0;
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
            return -1;
        }
    }

    PyErr_SetString(PartExceptionOCCError, "Curve or shape expected");
    return -1;
}

// ====== Methods  ======================================================================

PyObject* TopoShapeEdgePy::getParameterByLength(PyObject *args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d",&u))
        return 0;

    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    BRepAdaptor_Curve adapt(e);

    // transform value of [0,Length] to [First,Last]
    double first = BRepLProp_CurveTool::FirstParameter(adapt);
    double last = BRepLProp_CurveTool::LastParameter(adapt);
    if (!Precision::IsInfinite(first) && !Precision::IsInfinite(last)) {
        double length = GCPnts_AbscissaPoint::Length(adapt);

        if (u < 0 || u > length) {
            PyErr_SetString(PyExc_ValueError, "value out of range");
            return 0;
        }

        double stretch = (last - first) / length;
        u = first + u*stretch;
    }

    return PyFloat_FromDouble(u);
}

PyObject* TopoShapeEdgePy::valueAt(PyObject *args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d",&u))
        return 0;

    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    BRepAdaptor_Curve adapt(e);

    // Check now the orientation of the edge to make
    // sure that we get the right wanted point!
    BRepLProp_CLProps prop(adapt,u,0,Precision::Confusion());
    const gp_Pnt& V = prop.Value();
    return new Base::VectorPy(new Base::Vector3d(V.X(),V.Y(),V.Z()));
}

PyObject* TopoShapeEdgePy::parameterAt(PyObject *args)
{
    PyObject* pnt;
    PyObject* face=0;
    if (!PyArg_ParseTuple(args, "O!|O!",&TopoShapeVertexPy::Type,&pnt,
                                        &TopoShapeFacePy::Type,&face))
        return 0;

    try {
        const TopoDS_Shape& v = static_cast<TopoShapePy*>(pnt)->getTopoShapePtr()->getShape();
        const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());

        if (face) {
            const TopoDS_Shape& f = static_cast<TopoShapeFacePy*>(face)->getTopoShapePtr()->getShape();
            Standard_Real par = BRep_Tool::Parameter(TopoDS::Vertex(v), e, TopoDS::Face(f));
            return PyFloat_FromDouble(par);
        }
        else {
            Standard_Real par = BRep_Tool::Parameter(TopoDS::Vertex(v), e);
            return PyFloat_FromDouble(par);
        }
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeEdgePy::tangentAt(PyObject *args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d",&u))
        return 0;

    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    BRepAdaptor_Curve adapt(e);

    BRepLProp_CLProps prop(adapt,u,2,Precision::Confusion());
    if (prop.IsTangentDefined()) {
        gp_Dir dir;
        prop.Tangent(dir);
        return new Base::VectorPy(new Base::Vector3d(dir.X(),dir.Y(),dir.Z()));
    }
    else {
        PyErr_SetString(PyExc_NotImplementedError, "Tangent not defined at this position!");
        return 0;
    }
}

PyObject* TopoShapeEdgePy::normalAt(PyObject *args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d",&u))
        return 0;

    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    BRepAdaptor_Curve adapt(e);

    try {
        BRepLProp_CLProps prop(adapt,u,2,Precision::Confusion());
        gp_Dir V ;
        prop.Normal(V);
        return new Base::VectorPy(new Base::Vector3d(V.X(),V.Y(),V.Z()));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeEdgePy::curvatureAt(PyObject *args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d",&u))
        return 0;

    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    BRepAdaptor_Curve adapt(e);

    try {
        BRepLProp_CLProps prop(adapt,u,2,Precision::Confusion());
        double C = prop.Curvature();
        return Py::new_reference_to(Py::Float(C));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeEdgePy::centerOfCurvatureAt(PyObject *args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d",&u))
        return 0;

    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    BRepAdaptor_Curve adapt(e);

    try {
        BRepLProp_CLProps prop(adapt,u,2,Precision::Confusion());
        gp_Pnt V ;
        prop.CentreOfCurvature(V);
        return new Base::VectorPy(new Base::Vector3d(V.X(),V.Y(),V.Z()));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeEdgePy::derivative1At(PyObject *args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d",&u))
        return 0;

    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    BRepAdaptor_Curve adapt(e);

    try {
        BRepLProp_CLProps prop(adapt,u,1,Precision::Confusion());
        const gp_Vec& V = prop.D1();
        return new Base::VectorPy(new Base::Vector3d(V.X(),V.Y(),V.Z()));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeEdgePy::derivative2At(PyObject *args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d",&u))
        return 0;

    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    BRepAdaptor_Curve adapt(e);

    try {
        BRepLProp_CLProps prop(adapt,u,2,Precision::Confusion());
        const gp_Vec& V = prop.D2();
        return new Base::VectorPy(new Base::Vector3d(V.X(),V.Y(),V.Z()));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeEdgePy::derivative3At(PyObject *args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d",&u))
        return 0;

    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    BRepAdaptor_Curve adapt(e);

    try {
        BRepLProp_CLProps prop(adapt,u,3,Precision::Confusion());
        const gp_Vec& V = prop.D3();
        return new Base::VectorPy(new Base::Vector3d(V.X(),V.Y(),V.Z()));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeEdgePy::discretize(PyObject *args, PyObject *kwds)
{
    try {
        BRepAdaptor_Curve adapt(TopoDS::Edge(getTopoShapePtr()->getShape()));
        bool uniformAbscissaPoints = false;
        bool uniformAbscissaDistance = false;
        int numPoints = -1;
        double distance = -1;
        double first = adapt.FirstParameter();
        double last = adapt.LastParameter();

        // use no kwds
        PyObject* dist_or_num;
        if (PyArg_ParseTuple(args, "O", &dist_or_num)) {
            if (PyInt_Check(dist_or_num)) {
                numPoints = PyInt_AsLong(dist_or_num);
                uniformAbscissaPoints = true;
            }
            else if (PyFloat_Check(dist_or_num)) {
                distance = PyFloat_AsDouble(dist_or_num);
                uniformAbscissaDistance = true;
            }
            else {
                PyErr_SetString(PyExc_TypeError, "Either int or float expected");
                return 0;
            }
        }
        else {
            // use Number kwds
            static char* kwds_numPoints[] = {"Number","First","Last",NULL};
            PyErr_Clear();
            if (PyArg_ParseTupleAndKeywords(args, kwds, "i|dd", kwds_numPoints, &numPoints, &first, &last)) {
                uniformAbscissaPoints = true;
            }
            else {
                // use Abscissa kwds
                static char* kwds_Distance[] = {"Distance","First","Last",NULL};
                PyErr_Clear();
                if (PyArg_ParseTupleAndKeywords(args, kwds, "d|dd", kwds_Distance, &distance, &first, &last)) {
                    uniformAbscissaDistance = true;
                }
            }
        }

        if (uniformAbscissaPoints || uniformAbscissaDistance) {
            GCPnts_UniformAbscissa discretizer;
            if (uniformAbscissaPoints)
                discretizer.Initialize (adapt, numPoints, first, last);
            else
                discretizer.Initialize (adapt, distance, first, last);

            if (discretizer.IsDone () && discretizer.NbPoints () > 0) {
                Py::List points;
                int nbPoints = discretizer.NbPoints ();
                for (int i=1; i<=nbPoints; i++) {
                    gp_Pnt p = adapt.Value (discretizer.Parameter (i));
                    points.append(Py::Vector(Base::Vector3d(p.X(),p.Y(),p.Z())));
                }

                return Py::new_reference_to(points);
            }
            else {
                PyErr_SetString(PartExceptionOCCError, "Discretization of edge failed");
                return 0;
            }
        }

        // use Deflection kwds
        static char* kwds_Deflection[] = {"Deflection","First","Last",NULL};
        PyErr_Clear();
        double deflection;
        if (PyArg_ParseTupleAndKeywords(args, kwds, "d|dd", kwds_Deflection, &deflection, &first, &last)) {
            GCPnts_UniformDeflection discretizer(adapt, deflection, first, last);
            if (discretizer.IsDone () && discretizer.NbPoints () > 0) {
                Py::List points;
                int nbPoints = discretizer.NbPoints ();
                for (int i=1; i<=nbPoints; i++) {
                    gp_Pnt p = discretizer.Value (i);
                    points.append(Py::Vector(Base::Vector3d(p.X(),p.Y(),p.Z())));
                }

                return Py::new_reference_to(points);
            }
            else {
                PyErr_SetString(PartExceptionOCCError, "Discretization of edge failed");
                return 0;
            }
        }

        // use TangentialDeflection kwds
        static char* kwds_TangentialDeflection[] = {"Angular","Curvature","First","Last","Minimum",NULL};
        PyErr_Clear();
        double angular;
        double curvature;
        int minimumPoints = 2;
        if (PyArg_ParseTupleAndKeywords(args, kwds, "dd|ddi", kwds_TangentialDeflection, &angular, &curvature, &first, &last, &minimumPoints)) {
            GCPnts_TangentialDeflection discretizer(adapt, first, last, angular, curvature, minimumPoints);
            if (discretizer.NbPoints () > 0) {
                Py::List points;
                int nbPoints = discretizer.NbPoints ();
                for (int i=1; i<=nbPoints; i++) {
                    gp_Pnt p = discretizer.Value (i);
                    points.append(Py::Vector(Base::Vector3d(p.X(),p.Y(),p.Z())));
                }

                return Py::new_reference_to(points);
            }
            else {
                PyErr_SetString(PartExceptionOCCError, "Discretization of edge failed");
                return 0;
            }
        }

        // use QuasiNumber kwds
        static char* kwds_QuasiNumPoints[] = {"QuasiNumber","First","Last",NULL};
        PyErr_Clear();
        int quasiNumPoints;
        if (PyArg_ParseTupleAndKeywords(args, kwds, "i|dd", kwds_QuasiNumPoints, &quasiNumPoints, &first, &last)) {
            GCPnts_QuasiUniformAbscissa discretizer(adapt, quasiNumPoints, first, last);
            if (discretizer.NbPoints () > 0) {
                Py::List points;
                int nbPoints = discretizer.NbPoints ();
                for (int i=1; i<=nbPoints; i++) {
                    gp_Pnt p = adapt.Value (discretizer.Parameter (i));
                    points.append(Py::Vector(Base::Vector3d(p.X(),p.Y(),p.Z())));
                }

                return Py::new_reference_to(points);
            }
            else {
                PyErr_SetString(PartExceptionOCCError, "Discretization of edge failed");
                return 0;
            }
        }

        // use QuasiDeflection kwds
        static char* kwds_QuasiDeflection[] = {"QuasiDeflection","First","Last",NULL};
        PyErr_Clear();
        double quasiDeflection;
        if (PyArg_ParseTupleAndKeywords(args, kwds, "d|dd", kwds_QuasiDeflection, &quasiDeflection, &first, &last)) {
            GCPnts_QuasiUniformDeflection discretizer(adapt, quasiDeflection, first, last);
            if (discretizer.NbPoints () > 0) {
                Py::List points;
                int nbPoints = discretizer.NbPoints ();
                for (int i=1; i<=nbPoints; i++) {
                    gp_Pnt p = discretizer.Value (i);
                    points.append(Py::Vector(Base::Vector3d(p.X(),p.Y(),p.Z())));
                }

                return Py::new_reference_to(points);
            }
            else {
                PyErr_SetString(PartExceptionOCCError, "Discretization of edge failed");
                return 0;
            }
        }
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PartExceptionOCCError, e.what());
        return 0;
    }

    PyErr_SetString(PartExceptionOCCError,"Wrong arguments");
    return 0;
}

PyObject* TopoShapeEdgePy::split(PyObject *args)
{
    PyObject* float_or_list;
    if (!PyArg_ParseTuple(args, "O", &float_or_list))
        return 0;

    try {
        BRepAdaptor_Curve adapt(TopoDS::Edge(getTopoShapePtr()->getShape()));
        Standard_Real f = adapt.FirstParameter();
        Standard_Real l = adapt.LastParameter();

        std::vector<Standard_Real> par;
        par.push_back(f);
        if (PyFloat_Check(float_or_list)) {
            double val = PyFloat_AsDouble(float_or_list);
            if (val == f || val == l) {
                PyErr_SetString(PyExc_ValueError, "Cannot split edge at start or end point");
                return 0;
            }
            else if (val < f || val > l) {
                PyErr_SetString(PyExc_ValueError, "Value out of parameter range");
                return 0;
            }
            par.push_back(val);
        }
        else if (PySequence_Check(float_or_list)) {
            Py::Sequence list(float_or_list);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                double val = (double)Py::Float(*it);
                if (val == f || val == l) {
                    PyErr_SetString(PyExc_ValueError, "Cannot split edge at start or end point");
                    return 0;
                }
                else if (val < f || val > l) {
                    PyErr_SetString(PyExc_ValueError, "Value out of parameter range");
                    return 0;
                }
                par.push_back(val);
            }
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Either float or list of floats expected");
            return 0;
        }

        par.push_back(l);
        std::sort(par.begin(), par.end());

        BRepBuilderAPI_MakeWire mkWire;
        Handle_Geom_Curve c = adapt.Curve().Curve();
        std::vector<Standard_Real>::iterator end = par.end() - 1;
        for (std::vector<Standard_Real>::iterator it = par.begin(); it != end; ++it) {
            BRepBuilderAPI_MakeEdge mkBuilder(c, it[0], it[1]);
            mkWire.Add(mkBuilder.Edge());
        }

        return new TopoShapeWirePy(new TopoShape(mkWire.Shape()));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeEdgePy::isSeam(PyObject *args)
{
    PyObject* face;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapeFacePy::Type, &face))
        return 0;

    try {
        const TopoDS_Edge& e = TopoDS::Edge(this->getTopoShapePtr()->getShape());
        const TopoDS_Face& f = TopoDS::Face(static_cast<TopoShapeFacePy*>(face)->getTopoShapePtr()->getShape());

        ShapeAnalysis_Edge sa;
        Standard_Boolean ok = sa.IsSeam(e, f);
        return PyBool_FromLong(ok ? 1 : 0);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeEdgePy::setTolerance(PyObject *args)
{
    double tol;
    if (!PyArg_ParseTuple(args, "d", &tol))
        return 0;
    BRep_Builder aBuilder;
    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    aBuilder.UpdateEdge(e, tol);
    Py_Return;
}

// ====== Attributes ======================================================================

Py::Float TopoShapeEdgePy::getTolerance(void) const
{
    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    return Py::Float(BRep_Tool::Tolerance(e));
}

void TopoShapeEdgePy::setTolerance(Py::Float tol)
{
    BRep_Builder aBuilder;
    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    aBuilder.UpdateEdge(e, (double)tol);
}

Py::Float TopoShapeEdgePy::getLength(void) const
{
    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    BRepAdaptor_Curve adapt(e);
    return Py::Float(GCPnts_AbscissaPoint::Length(adapt));
}

Py::Object TopoShapeEdgePy::getCurve() const
{
    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    BRepAdaptor_Curve adapt(e);
    switch(adapt.GetType())
    {
    case GeomAbs_Line:
        {
            GeomLineSegment* line = new GeomLineSegment();
            Handle_Geom_TrimmedCurve this_curv = Handle_Geom_TrimmedCurve::DownCast
                (line->handle());
            Handle_Geom_Line this_line = Handle_Geom_Line::DownCast
                (this_curv->BasisCurve());
            this_line->SetLin(adapt.Line());
            this_curv->SetTrim(adapt.FirstParameter(), adapt.LastParameter());
            return Py::Object(new LinePy(line),true);
        }
    case GeomAbs_Circle:
        {
            GeomCircle* circle = new GeomCircle();
            Handle_Geom_Circle this_curv = Handle_Geom_Circle::DownCast
                (circle->handle());
            this_curv->SetCirc(adapt.Circle());
            //Standard_Real dd = adapt.FirstParameter();
            //Standard_Real ee = adapt.LastParameter();
            return Py::Object(new CirclePy(circle),true);
        }
    case GeomAbs_Ellipse:
        {
            GeomEllipse* elips = new GeomEllipse();
            Handle_Geom_Ellipse this_curv = Handle_Geom_Ellipse::DownCast
                (elips->handle());
            this_curv->SetElips(adapt.Ellipse());
            return Py::Object(new EllipsePy(elips),true);
        }
    case GeomAbs_Hyperbola:
        {
            GeomHyperbola* hypr = new GeomHyperbola();
            Handle_Geom_Hyperbola this_curv = Handle_Geom_Hyperbola::DownCast
                (hypr->handle());
            this_curv->SetHypr(adapt.Hyperbola());
            return Py::Object(new HyperbolaPy(hypr),true);
        }
    case GeomAbs_Parabola:
        {
            GeomParabola* parab = new GeomParabola();
            Handle_Geom_Parabola this_curv = Handle_Geom_Parabola::DownCast
                (parab->handle());
            this_curv->SetParab(adapt.Parabola());
            return Py::Object(new ParabolaPy(parab),true);
        }
    case GeomAbs_BezierCurve:
        {
            GeomBezierCurve* curve = new GeomBezierCurve(adapt.Bezier());
            return Py::Object(new BezierCurvePy(curve),true);
        }
    case GeomAbs_BSplineCurve:
        {
            GeomBSplineCurve* curve = new GeomBSplineCurve(adapt.BSpline());
            return Py::Object(new BSplineCurvePy(curve),true);
        }
    case GeomAbs_OtherCurve:
        break;
    }

    throw Py::TypeError("undefined curve type");
}

Py::Tuple TopoShapeEdgePy::getParameterRange(void) const
{
    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    BRepAdaptor_Curve adapt(e);
    double u = adapt.FirstParameter();
    double v = adapt.LastParameter();

    Py::Tuple t(2);
    t.setItem(0, Py::Float(u));
    t.setItem(1, Py::Float(v));
    return t;
}

Py::Float TopoShapeEdgePy::getFirstParameter(void) const
{
    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    BRepAdaptor_Curve adapt(e);
    double t = adapt.FirstParameter();
    return Py::Float(t);
}

Py::Float TopoShapeEdgePy::getLastParameter(void) const
{
    const TopoDS_Edge& e = TopoDS::Edge(getTopoShapePtr()->getShape());
    BRepAdaptor_Curve adapt(e);
    double t = adapt.LastParameter();
    return Py::Float(t);
}

Py::Object TopoShapeEdgePy::getMass(void) const
{
    GProp_GProps props;
    BRepGProp::LinearProperties(getTopoShapePtr()->getShape(), props);
    double c = props.Mass();
    return Py::Float(c);
}

Py::Object TopoShapeEdgePy::getCenterOfMass(void) const
{
    GProp_GProps props;
    BRepGProp::LinearProperties(getTopoShapePtr()->getShape(), props);
    gp_Pnt c = props.CentreOfMass();
    return Py::Vector(Base::Vector3d(c.X(),c.Y(),c.Z()));
}

Py::Object TopoShapeEdgePy::getMatrixOfInertia(void) const
{
    GProp_GProps props;
    BRepGProp::LinearProperties(getTopoShapePtr()->getShape(), props);
    gp_Mat m = props.MatrixOfInertia();
    Base::Matrix4D mat;
    for (int i=0; i<3; i++) {
        for (int j=0; j<3; j++) {
            mat[i][j] = m(i+1,j+1);
        }
    }
    return Py::Matrix(mat);
}

Py::Object TopoShapeEdgePy::getStaticMoments(void) const
{
    GProp_GProps props;
    BRepGProp::LinearProperties(getTopoShapePtr()->getShape(), props);
    Standard_Real lx,ly,lz;
    props.StaticMoments(lx,ly,lz);
    Py::Tuple tuple(3);
    tuple.setItem(0, Py::Float(lx));
    tuple.setItem(1, Py::Float(ly));
    tuple.setItem(2, Py::Float(lz));
    return tuple;
}

Py::Dict TopoShapeEdgePy::getPrincipalProperties(void) const
{
    GProp_GProps props;
    BRepGProp::LinearProperties(getTopoShapePtr()->getShape(), props);
    GProp_PrincipalProps pprops = props.PrincipalProperties();

    Py::Dict dict;
    dict.setItem("SymmetryAxis", Py::Boolean(pprops.HasSymmetryAxis() ? true : false));
    dict.setItem("SymmetryPoint", Py::Boolean(pprops.HasSymmetryPoint() ? true : false));
    Standard_Real lx,ly,lz;
    pprops.Moments(lx,ly,lz);
    Py::Tuple tuple(3);
    tuple.setItem(0, Py::Float(lx));
    tuple.setItem(1, Py::Float(ly));
    tuple.setItem(2, Py::Float(lz));
    dict.setItem("Moments",tuple);
    dict.setItem("FirstAxisOfInertia",Py::Vector(Base::convertTo
        <Base::Vector3d>(pprops.FirstAxisOfInertia())));
    dict.setItem("SecondAxisOfInertia",Py::Vector(Base::convertTo
        <Base::Vector3d>(pprops.SecondAxisOfInertia())));
    dict.setItem("ThirdAxisOfInertia",Py::Vector(Base::convertTo
        <Base::Vector3d>(pprops.ThirdAxisOfInertia())));

    Standard_Real Rxx,Ryy,Rzz;
    pprops.RadiusOfGyration(Rxx,Ryy,Rzz);
    Py::Tuple rog(3);
    rog.setItem(0, Py::Float(Rxx));
    rog.setItem(1, Py::Float(Ryy));
    rog.setItem(2, Py::Float(Rzz));
    dict.setItem("RadiusOfGyration",rog);
    return dict;
}

Py::Boolean TopoShapeEdgePy::getClosed(void) const
{
    if (getTopoShapePtr()->getShape().IsNull())
        throw Py::Exception("Cannot determine the 'Closed'' flag of an empty shape");
    Standard_Boolean ok = BRep_Tool::IsClosed(getTopoShapePtr()->getShape());
    return Py::Boolean(ok ? true : false);
}

Py::Boolean TopoShapeEdgePy::getDegenerated(void) const
{
    Standard_Boolean ok = BRep_Tool::Degenerated(TopoDS::Edge(getTopoShapePtr()->getShape()));
    return Py::Boolean(ok ? true : false);
}

PyObject *TopoShapeEdgePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int TopoShapeEdgePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
