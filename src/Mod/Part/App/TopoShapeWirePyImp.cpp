/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <Approx_Curve3d.hxx>
# include <BRepAdaptor_CompCurve.hxx>
# include <BRepBuilderAPI_FindPlane.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepGProp.hxx>
# include <BRepOffsetAPI_MakeOffset.hxx>
# include <BRepTools_WireExplorer.hxx>
# include <GProp_GProps.hxx>
# include <GProp_PrincipalProps.hxx>
# include <GCPnts_QuasiUniformAbscissa.hxx>
# include <GCPnts_QuasiUniformDeflection.hxx>
# include <GCPnts_TangentialDeflection.hxx>
# include <GCPnts_UniformAbscissa.hxx>
# include <GCPnts_UniformDeflection.hxx>
# include <Precision.hxx>
# include <ShapeAlgo_AlgoContainer.hxx>
# include <ShapeFix_Wire.hxx>
# include <TopExp.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Wire.hxx>
#endif
#include <BRepOffsetAPI_MakeEvolved.hxx>

#include <Base/GeometryPyCXX.h>
#include <Base/PyWrapParseTupleAndKeywords.h>

#include <Mod/Part/App/BSplineCurvePy.h>
#include <Mod/Part/App/TopoShapeFacePy.h>
#include <Mod/Part/App/TopoShapeWirePy.h>
#include <Mod/Part/App/TopoShapeWirePy.cpp>
#include "OCCError.h"
#include "Tools.h"


using namespace Part;

namespace Part {
    extern Py::Object shape2pyshape(const TopoDS_Shape &shape);
}


// returns a string which represents the object e.g. when printed in python
std::string TopoShapeWirePy::representation() const
{
    std::stringstream str;
    str << "<Wire object at " << getTopoShapePtr() << ">";

    return str.str();
}

PyObject *TopoShapeWirePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of TopoShapeWirePy and the Twin object
    return new TopoShapeWirePy(new TopoShape);
}

// constructor method
int TopoShapeWirePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        // Undefined Wire
        getTopoShapePtr()->setShape(TopoDS_Wire());
        return 0;
    }

    PyErr_Clear();
    PyObject *pcObj;
    if (PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &pcObj)) {
        BRepBuilderAPI_MakeWire mkWire;
        const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
        if (sh.IsNull()) {
            PyErr_SetString(PyExc_TypeError, "given shape is invalid");
            return -1;
        }
        if (sh.ShapeType() == TopAbs_EDGE)
            mkWire.Add(TopoDS::Edge(sh));
        else if (sh.ShapeType() == TopAbs_WIRE)
            mkWire.Add(TopoDS::Wire(sh));
        else {
            PyErr_SetString(PyExc_TypeError, "shape is neither edge nor wire");
            return -1;
        }

        try {
            getTopoShapePtr()->setShape(mkWire.Wire());
            return 0;
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return -1;
        }
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O", &pcObj)) {
        if(!Py::Object(pcObj).isList() && !Py::Object(pcObj).isTuple()) {
            PyErr_SetString(PyExc_TypeError, "object is neither a list nor a tuple");
            return -1;
        }

        BRepBuilderAPI_MakeWire mkWire;
        Py::Sequence list(pcObj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
                const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape();
                if (sh.IsNull()) {
                    PyErr_SetString(PyExc_TypeError, "given shape is invalid");
                    return -1;
                }
                if (sh.ShapeType() == TopAbs_EDGE)
                    mkWire.Add(TopoDS::Edge(sh));
                else if (sh.ShapeType() == TopAbs_WIRE)
                    mkWire.Add(TopoDS::Wire(sh));
                else {
                    PyErr_SetString(PyExc_TypeError, "shape is neither edge nor wire");
                    return -1;
                }
            }
            else {
                PyErr_SetString(PyExc_TypeError, "item is not a shape");
                return -1;
            }
        }

        try {
            getTopoShapePtr()->setShape(mkWire.Wire());
            return 0;
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return -1;
        }
    }

    PyErr_SetString(PartExceptionOCCError, "edge or wire or list of edges and wires expected");
    return -1;
}

PyObject* TopoShapeWirePy::add(PyObject *args)
{
    PyObject* edge;
    if (!PyArg_ParseTuple(args, "O!",&(TopoShapePy::Type), &edge))
        return nullptr;
    const TopoDS_Wire& w = TopoDS::Wire(getTopoShapePtr()->getShape());
    BRepBuilderAPI_MakeWire mkWire(w);

    const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(edge)->getTopoShapePtr()->getShape();
    if (sh.IsNull()) {
        PyErr_SetString(PyExc_TypeError, "given shape is invalid");
        return nullptr;
    }
    if (sh.ShapeType() == TopAbs_EDGE)
        mkWire.Add(TopoDS::Edge(sh));
    else if (sh.ShapeType() == TopAbs_WIRE)
        mkWire.Add(TopoDS::Wire(sh));
    else {
        PyErr_SetString(PyExc_TypeError, "shape is neither edge nor wire");
        return nullptr;
    }

    try {
        getTopoShapePtr()->setShape(mkWire.Wire());
        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* TopoShapeWirePy::fixWire(PyObject *args)
{
    PyObject* face=nullptr;
    double tol = Precision::Confusion();
    if (!PyArg_ParseTuple(args, "|O!d",&(TopoShapeFacePy::Type), &face, &tol))
        return nullptr;

    try {
        ShapeFix_Wire aFix;
        const TopoDS_Wire& w = TopoDS::Wire(getTopoShapePtr()->getShape());

        if (face) {
            const TopoDS_Face& f = TopoDS::Face(static_cast<TopoShapePy*>(face)->getTopoShapePtr()->getShape());
            aFix.Init(w, f, tol);
        }
        else {
            aFix.SetPrecision(tol);
            aFix.Load(w);
        }

        aFix.FixReorder();
        aFix.FixConnected();
        aFix.FixClosed();
        getTopoShapePtr()->setShape(aFix.Wire());

        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* TopoShapeWirePy::makeOffset(PyObject *args)
{
    double dist;
    if (!PyArg_ParseTuple(args, "d",&dist))
        return nullptr;
    const TopoDS_Wire& w = TopoDS::Wire(getTopoShapePtr()->getShape());
    BRepBuilderAPI_FindPlane findPlane(w);
    if (!findPlane.Found()) {
        PyErr_SetString(PartExceptionOCCError, "No planar wire");
        return nullptr;
    }

    BRepOffsetAPI_MakeOffset mkOffset(w);
    mkOffset.Perform(dist);

    return new TopoShapePy(new TopoShape(mkOffset.Shape()));
}

PyObject* TopoShapeWirePy::makePipe(PyObject *args)
{
    PyObject *pShape;
    if (PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &pShape)) {
        try {
            TopoDS_Shape profile = static_cast<TopoShapePy*>(pShape)->getTopoShapePtr()->getShape();
            TopoDS_Shape shape = this->getTopoShapePtr()->makePipe(profile);
            return new TopoShapePy(new TopoShape(shape));
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    return nullptr;
}

PyObject* TopoShapeWirePy::makePipeShell(PyObject *args)
{
    PyObject *obj;
    PyObject *make_solid = Py_False;
    PyObject *is_Frenet = Py_False;
    int transition = 0;

    if (PyArg_ParseTuple(args, "O|O!O!i", &obj,
                             &PyBool_Type, &make_solid,
                             &PyBool_Type, &is_Frenet,
                             &transition)) {
        try {
            TopTools_ListOfShape sections;
            Py::Sequence list(obj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& shape = static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr()->getShape();
                    sections.Append(shape);
                }
            }
            TopoDS_Shape shape = this->getTopoShapePtr()->makePipeShell(sections,
                Base::asBoolean(make_solid), Base::asBoolean(is_Frenet), transition);
            return new TopoShapePy(new TopoShape(shape));
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    return nullptr;
}

/*
import PartEnums
v = App.Vector
profile = Part.makePolygon([v(0.,0.,0.), v(-60.,-60.,-100.), v(-60.,-60.,-140.)])
spine = Part.makePolygon([v(0.,0.,0.), v(100.,0.,0.), v(100.,100.,0.), v(0.,100.,0.), v(0.,0.,0.)])
evolve = spine.makeEvolved(Profile=profile, Join=PartEnums.JoinType.Arc)
*/
PyObject* TopoShapeWirePy::makeEvolved(PyObject *args, PyObject *kwds)
{
    PyObject* Profile;
    PyObject* AxeProf = Py_True;
    PyObject* Solid = Py_False;
    PyObject* ProfOnSpine = Py_False;
    int JoinType = int(GeomAbs_Arc);
    double Tolerance = 0.0000001;

    static const std::array<const char *, 7> kwds_evolve{"Profile", "Join", "AxeProf", "Solid", "ProfOnSpine",
                                                         "Tolerance", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!|iO!O!O!d", kwds_evolve,
                                             &TopoShapeWirePy::Type, &Profile, &JoinType,
                                             &PyBool_Type, &AxeProf, &PyBool_Type, &Solid,
                                             &PyBool_Type, &ProfOnSpine, &Tolerance)) {
        return nullptr;
    }

    const TopoDS_Wire& spine = TopoDS::Wire(getTopoShapePtr()->getShape());
    BRepBuilderAPI_FindPlane findPlane(spine);
    if (!findPlane.Found()) {
        PyErr_SetString(PartExceptionOCCError, "No planar wire");
        return nullptr;
    }

    const TopoDS_Wire& profile = TopoDS::Wire(static_cast<TopoShapeWirePy*>(Profile)->getTopoShapePtr()->getShape());

    GeomAbs_JoinType joinType;
    switch (JoinType) {
    case GeomAbs_Tangent:
        joinType = GeomAbs_Tangent;
        break;
    case GeomAbs_Intersection:
        joinType = GeomAbs_Intersection;
        break;
    default:
        joinType = GeomAbs_Arc;
        break;
    }

    try {
        BRepOffsetAPI_MakeEvolved evolved(spine, profile, joinType,
                                          Base::asBoolean(AxeProf),
                                          Base::asBoolean(Solid),
                                          Base::asBoolean(ProfOnSpine),
                                          Tolerance);
        TopoDS_Shape shape = evolved.Shape();
        return Py::new_reference_to(shape2pyshape(shape));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* TopoShapeWirePy::makeHomogenousWires(PyObject *args)
{
    PyObject* wire;
    if (!PyArg_ParseTuple(args, "O!",&(Part::TopoShapeWirePy::Type),&wire))
        return nullptr;
    try {
        TopoDS_Wire o1, o2;
        const TopoDS_Wire& w1 = TopoDS::Wire(getTopoShapePtr()->getShape());
        const TopoDS_Wire& w2 = TopoDS::Wire(static_cast<TopoShapePy*>(wire)->getTopoShapePtr()->getShape());
        ShapeAlgo_AlgoContainer shapeAlgo;
        if (shapeAlgo.HomoWires(w1,w2,o1,o2,Standard_True)) {
            getTopoShapePtr()->setShape(o1);
            return new TopoShapeWirePy(new TopoShape(o2));
        }
        else {
            Py_INCREF(wire);
            return wire;
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* TopoShapeWirePy::approximate(PyObject *args, PyObject *kwds)
{
    double tol2d = gp::Resolution();
    double tol3d = 0.0001;
    int maxseg=10, maxdeg=3;

    static const std::array<const char *, 5> kwds_approx{"Tol2d", "Tol3d", "MaxSegments", "MaxDegree", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "|ddii", kwds_approx, &tol2d, &tol3d, &maxseg, &maxdeg)) {
        return nullptr;
    }
    try {
        BRepAdaptor_CompCurve adapt(TopoDS::Wire(getTopoShapePtr()->getShape()));
        auto hcurve = adapt.Trim(adapt.FirstParameter(),
                                 adapt.LastParameter(),
                                 tol2d);
        Approx_Curve3d approx(hcurve, tol3d, GeomAbs_C0, maxseg, maxdeg);
        if (approx.IsDone()) {
            return new BSplineCurvePy(new GeomBSplineCurve(approx.Curve()));
        }
        else {
            PyErr_SetString(PartExceptionOCCError, "failed to approximate wire");
            return nullptr;
        }
    }
    catch (Standard_Failure&) {
        PyErr_SetString(PartExceptionOCCError, "failed to approximate wire");
        return nullptr;
    }
}

PyObject* TopoShapeWirePy::discretize(PyObject *args, PyObject *kwds)
{
    try {
        BRepAdaptor_CompCurve adapt(TopoDS::Wire(getTopoShapePtr()->getShape()));
        bool uniformAbscissaPoints = false;
        bool uniformAbscissaDistance = false;
        int numPoints = -1;
        double distance = -1;
        double first = adapt.FirstParameter();
        double last = adapt.LastParameter();

        // use no kwds
        PyObject* dist_or_num;
        if (PyArg_ParseTuple(args, "O", &dist_or_num)) {
            if (PyLong_Check(dist_or_num)) {
                numPoints = PyLong_AsLong(dist_or_num);
                uniformAbscissaPoints = true;
            }
            else if (PyFloat_Check(dist_or_num)) {
                distance = PyFloat_AsDouble(dist_or_num);
                uniformAbscissaDistance = true;
            }
            else {
                PyErr_SetString(PyExc_TypeError, "Either int or float expected");
                return nullptr;
            }
        }
        else {
            // use Number kwds
            static const std::array<const char *, 4> kwds_numPoints{"Number", "First", "Last", nullptr};
            PyErr_Clear();
            if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "i|dd", kwds_numPoints, &numPoints, &first, &last)) {
                uniformAbscissaPoints = true;
            }
            else {
                // use Abscissa kwds
                static const std::array<const char *, 4> kwds_Distance{"Distance", "First", "Last", nullptr};
                PyErr_Clear();
                if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "d|dd", kwds_Distance, &distance, &first, &last)) {
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
                PyErr_SetString(PartExceptionOCCError, "Discretization of wire failed");
                return nullptr;
            }
        }

        // use Deflection kwds
        static const std::array<const char *, 4> kwds_Deflection{"Deflection", "First", "Last", nullptr};
        PyErr_Clear();
        double deflection;
        if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "d|dd", kwds_Deflection, &deflection, &first, &last)) {
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
                PyErr_SetString(PartExceptionOCCError, "Discretization of wire failed");
                return nullptr;
            }
        }

        // use TangentialDeflection kwds
        static const std::array<const char *, 6> kwds_TangentialDeflection{"Angular", "Curvature", "First", "Last",
                                                                           "Minimum", nullptr};
        PyErr_Clear();
        double angular;
        double curvature;
        int minimumPoints = 2;
        if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "dd|ddi", kwds_TangentialDeflection,
                                                &angular, &curvature, &first, &last, &minimumPoints)) {
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
                PyErr_SetString(PartExceptionOCCError, "Discretization of wire failed");
                return nullptr;
            }
        }

        // use QuasiNumber kwds
        static const std::array<const char *, 4> kwds_QuasiNumPoints{"QuasiNumber", "First", "Last", nullptr};
        PyErr_Clear();
        int quasiNumPoints;
        if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "i|dd", kwds_QuasiNumPoints,
                                                &quasiNumPoints, &first, &last)) {
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
                PyErr_SetString(PartExceptionOCCError, "Discretization of wire failed");
                return nullptr;
            }
        }

        // use QuasiDeflection kwds
        static const std::array<const char *, 4> kwds_QuasiDeflection{"QuasiDeflection", "First", "Last", nullptr};
        PyErr_Clear();
        double quasiDeflection;
        if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "d|dd", kwds_QuasiDeflection,
                                                &quasiDeflection, &first, &last)) {
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
                PyErr_SetString(PartExceptionOCCError, "Discretization of wire failed");
                return nullptr;
            }
        }
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PartExceptionOCCError, e.what());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError,"Wrong arguments");
    return nullptr;
}

Py::String TopoShapeWirePy::getContinuity() const
{
    BRepAdaptor_CompCurve adapt(TopoDS::Wire(getTopoShapePtr()->getShape()));
    std::string cont;
    switch (adapt.Continuity()) {
    case GeomAbs_C0:
        cont = "C0";
        break;
    case GeomAbs_G1:
        cont = "G1";
        break;
    case GeomAbs_C1:
        cont = "C1";
        break;
    case GeomAbs_G2:
        cont = "G2";
        break;
    case GeomAbs_C2:
        cont = "C2";
        break;
    case GeomAbs_C3:
        cont = "C3";
        break;
    case GeomAbs_CN:
        cont = "CN";
        break;
    }

    return Py::String(cont);
}

Py::Object TopoShapeWirePy::getMass() const
{
    GProp_GProps props;
    BRepGProp::LinearProperties(getTopoShapePtr()->getShape(), props);
    double c = props.Mass();
    return Py::Float(c);
}

Py::Object TopoShapeWirePy::getCenterOfMass() const
{
    GProp_GProps props;
    BRepGProp::LinearProperties(getTopoShapePtr()->getShape(), props);
    gp_Pnt c = props.CentreOfMass();
    return Py::Vector(Base::Vector3d(c.X(),c.Y(),c.Z()));
}

Py::Object TopoShapeWirePy::getMatrixOfInertia() const
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

Py::Object TopoShapeWirePy::getStaticMoments() const
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

Py::Dict TopoShapeWirePy::getPrincipalProperties() const
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

Py::List TopoShapeWirePy::getOrderedEdges() const
{
    Py::List ret;

    BRepTools_WireExplorer xp(TopoDS::Wire(getTopoShapePtr()->getShape()));
    while (xp.More()) {
        ret.append(shape2pyshape(xp.Current()));
        xp.Next();
    }

    return ret;
}

Py::List TopoShapeWirePy::getOrderedVertexes() const
{
    Py::List ret;

    TopoDS_Wire wire = TopoDS::Wire(getTopoShapePtr()->getShape());
    BRepTools_WireExplorer xp(wire);
    while (xp.More()) {
        ret.append(shape2pyshape(xp.CurrentVertex()));
        xp.Next();
    }

    // special treatment for open wires
    TopoDS_Vertex Vfirst, Vlast;
    TopExp::Vertices(wire, Vfirst, Vlast);
    if (!Vfirst.IsNull() && !Vlast.IsNull()) {
        if (!Vfirst.IsSame(Vlast)) {
            ret.append(shape2pyshape(Vlast));
        }
    }

    return ret;
}

PyObject *TopoShapeWirePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int TopoShapeWirePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


