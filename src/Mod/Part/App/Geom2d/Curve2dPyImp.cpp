/***************************************************************************
 *   Copyright (c) 2016 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <sstream>

# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeEdge2d.hxx>
# include <BRepLib.hxx>
# include <GCPnts_UniformAbscissa.hxx>
# include <GCPnts_UniformDeflection.hxx>
# include <GCPnts_TangentialDeflection.hxx>
# include <GCPnts_QuasiUniformAbscissa.hxx>
# include <GCPnts_QuasiUniformDeflection.hxx>
# include <GCPnts_AbscissaPoint.hxx>
# include <Geom2d_Curve.hxx>
# include <Geom2d_Geometry.hxx>
# include <Geom2dAdaptor_Curve.hxx>
# include <Geom2dAPI_ExtremaCurveCurve.hxx>
# include <Geom2dAPI_InterCurveCurve.hxx>
# include <Geom2dAPI_ProjectPointOnCurve.hxx>
# include <Geom2dConvert_ApproxCurve.hxx>
# include <Geom2dLProp_CLProps2d.hxx>
# include <gp_Dir2d.hxx>
# include <Precision.hxx>
# include <ShapeConstruct_Curve.hxx>
# include <Standard_Failure.hxx>
# include <Standard_NullValue.hxx>
# include <TopoDS.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/PyWrapParseTupleAndKeywords.h>

#include "Geom2d/Curve2dPy.h"
#include "Geom2d/Curve2dPy.cpp"
#include "Geom2d/BSplineCurve2dPy.h"
#include "GeometrySurfacePy.h"
#include "OCCError.h"
#include "TopoShapeFacePy.h"


namespace Part {
extern const Py::Object makeGeometryCurvePy(const Handle(Geom_Curve)& c);
}

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string Curve2dPy::representation() const
{
    return "<Curve2d object>";
}

PyObject *Curve2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'Curve2d'.");
    return nullptr;
}

// constructor method
int Curve2dPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* Curve2dPy::reverse(PyObject * args)
{
    if (PyArg_ParseTuple(args, "")) {
        try {
            Handle(Geom2d_Curve) curve = Handle(Geom2d_Curve)::DownCast(getGeom2dCurvePtr()->handle());
            curve->Reverse();
            Py_Return;
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    return nullptr;
}

namespace Part {
extern Py::Object shape2pyshape(const TopoDS_Shape &shape);

TopoDS_Edge create3dCurve(const TopoDS_Edge& edge)
{
    TopoDS_Edge edge3d;
    BRepAdaptor_Curve adapt_curve(edge);
    switch(adapt_curve.GetType()) {
    case GeomAbs_Line:
        {
            BRepBuilderAPI_MakeEdge mkBuilder3d(adapt_curve.Line(),
                                                adapt_curve.FirstParameter(),
                                                adapt_curve.LastParameter());
            edge3d =  mkBuilder3d.Edge();
        } break;
    case GeomAbs_Circle:
        {
            BRepBuilderAPI_MakeEdge mkBuilder3d(adapt_curve.Circle(),
                                                adapt_curve.FirstParameter(),
                                                adapt_curve.LastParameter());
            edge3d =  mkBuilder3d.Edge();
        } break;
    case GeomAbs_Ellipse:
        {
            BRepBuilderAPI_MakeEdge mkBuilder3d(adapt_curve.Ellipse(),
                                                adapt_curve.FirstParameter(),
                                                adapt_curve.LastParameter());
            edge3d =  mkBuilder3d.Edge();
        } break;
    case GeomAbs_Hyperbola:
        {
            BRepBuilderAPI_MakeEdge mkBuilder3d(adapt_curve.Hyperbola(),
                                                adapt_curve.FirstParameter(),
                                                adapt_curve.LastParameter());
            edge3d =  mkBuilder3d.Edge();
        } break;
    case GeomAbs_Parabola:
        {
            BRepBuilderAPI_MakeEdge mkBuilder3d(adapt_curve.Parabola(),
                                                adapt_curve.FirstParameter(),
                                                adapt_curve.LastParameter());
            edge3d =  mkBuilder3d.Edge();
        } break;
    case GeomAbs_BezierCurve:
        {
            BRepBuilderAPI_MakeEdge mkBuilder3d(adapt_curve.Bezier(),
                                                adapt_curve.FirstParameter(),
                                                adapt_curve.LastParameter());
            edge3d =  mkBuilder3d.Edge();
        } break;
    default:
        edge3d = edge;
        BRepLib::BuildCurves3d(edge3d, Precision::Confusion(), GeomAbs_Shape::GeomAbs_C1, 14, 10000);
        break;
    }

    return edge3d;
}
}

PyObject* Curve2dPy::toShape(PyObject *args)
{
    if (PyArg_ParseTuple(args, "")) {
        try {
            Handle(Geom2d_Curve) curv = Handle(Geom2d_Curve)::DownCast(getGeometry2dPtr()->handle());

            BRepBuilderAPI_MakeEdge2d mkBuilder(curv);
            TopoDS_Shape edge =  mkBuilder.Shape();
            return Py::new_reference_to(shape2pyshape(edge));
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_Clear();
    double u1, u2;
    if (PyArg_ParseTuple(args, "dd", &u1, &u2)) {
        try {
            Handle(Geom2d_Curve) curv = Handle(Geom2d_Curve)::DownCast(getGeometry2dPtr()->handle());

            BRepBuilderAPI_MakeEdge2d mkBuilder(curv, u1, u2);
            TopoDS_Shape edge =  mkBuilder.Shape();
            return Py::new_reference_to(shape2pyshape(edge));
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_Clear();
    PyObject* p;
    if (PyArg_ParseTuple(args, "O!", &(Part::GeometrySurfacePy::Type), &p)) {
        try {
            Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast(
                        static_cast<GeometrySurfacePy*>(p)->getGeomSurfacePtr()->handle());
            Handle(Geom2d_Curve) curv = Handle(Geom2d_Curve)::DownCast(getGeometry2dPtr()->handle());

            BRepBuilderAPI_MakeEdge mkBuilder(curv, surf);
            TopoDS_Edge edge =  mkBuilder.Edge();
            edge = create3dCurve(edge);

            return Py::new_reference_to(shape2pyshape(edge));
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!dd", &(Part::GeometrySurfacePy::Type), &p, &u1, &u2)) {
        try {
            Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast(
                        static_cast<GeometrySurfacePy*>(p)->getGeomSurfacePtr()->handle());
            Handle(Geom2d_Curve) curv = Handle(Geom2d_Curve)::DownCast(getGeometry2dPtr()->handle());

            BRepBuilderAPI_MakeEdge mkBuilder(curv, surf, u1, u2);
            TopoDS_Edge edge =  mkBuilder.Edge();
            edge = create3dCurve(edge);

            return Py::new_reference_to(shape2pyshape(edge));
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(Part::TopoShapeFacePy::Type), &p)) {
        try {
            const TopoDS_Face& face = TopoDS::Face(static_cast<TopoShapeFacePy*>(p)->getTopoShapePtr()->getShape());
            Handle(Geom2d_Curve) curv = Handle(Geom2d_Curve)::DownCast(getGeometry2dPtr()->handle());

            BRepAdaptor_Surface adapt(face);
            BRepBuilderAPI_MakeEdge mkBuilder(curv, adapt.Surface().Surface());
            TopoDS_Edge edge =  mkBuilder.Edge();
            edge = create3dCurve(edge);

            return Py::new_reference_to(shape2pyshape(edge));
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!dd", &(Part::TopoShapeFacePy::Type), &p, &u1, &u2)) {
        try {
            const TopoDS_Face& face = TopoDS::Face(static_cast<TopoShapeFacePy*>(p)->getTopoShapePtr()->getShape());
            Handle(Geom2d_Curve) curv = Handle(Geom2d_Curve)::DownCast(getGeometry2dPtr()->handle());

            BRepAdaptor_Surface adapt(face);
            BRepBuilderAPI_MakeEdge mkBuilder(curv, adapt.Surface().Surface(), u1, u2);
            TopoDS_Edge edge =  mkBuilder.Edge();
            edge = create3dCurve(edge);

            return Py::new_reference_to(shape2pyshape(edge));
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_SetString(PyExc_TypeError, "empty parameter list, parameter range or surface expected");
    return nullptr;
}

PyObject* Curve2dPy::discretize(PyObject *args, PyObject *kwds)
{
    try {
        Handle(Geom2d_Geometry) g = getGeometry2dPtr()->handle();
        Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(g);
        if (c.IsNull()) {
            PyErr_SetString(PartExceptionOCCError, "Geometry is not a curve");
            return nullptr;
        }

        Geom2dAdaptor_Curve adapt(c);
        double first = adapt.FirstParameter();
        double last = adapt.LastParameter();

        // use Number kwds
        static const std::array<const char *, 4> kwds_numPoints {"Number", "First", "Last", nullptr};
        PyErr_Clear();
        int numPoints = -1;
        if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "i|dd", kwds_numPoints, &numPoints, &first, &last)) {
            GCPnts_UniformAbscissa discretizer;
            discretizer.Initialize (adapt, numPoints, first, last);

            if (discretizer.IsDone () && discretizer.NbPoints () > 0) {
                Py::List points;
                int nbPoints = discretizer.NbPoints ();

                for (int i=1; i<=nbPoints; i++) {
                    gp_Pnt2d p = adapt.Value (discretizer.Parameter (i));
                    points.append(Base::Vector2dPy::create(p.X(), p.Y()));
                }

                return Py::new_reference_to(points);
            }
            else {
                PyErr_SetString(PartExceptionOCCError, "Discretization of curve failed");
                return nullptr;
            }
        }

        // use Distance kwds
        static const std::array<const char *, 4> kwds_Distance{"Distance", "First", "Last", nullptr};
        PyErr_Clear();
        double distance = -1;
        if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "d|dd", kwds_Distance, &distance, &first, &last)) {
            GCPnts_UniformAbscissa discretizer;
            discretizer.Initialize (adapt, distance, first, last);

            if (discretizer.IsDone () && discretizer.NbPoints () > 0) {
                Py::List points;
                int nbPoints = discretizer.NbPoints ();

                for (int i=1; i<=nbPoints; i++) {
                    gp_Pnt2d p = adapt.Value (discretizer.Parameter (i));
                    points.append(Base::Vector2dPy::create(p.X(), p.Y()));
                }

                return Py::new_reference_to(points);
            }
            else {
                PyErr_SetString(PartExceptionOCCError, "Discretization of curve failed");
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
                    points.append(Base::Vector2dPy::create(p.X(), p.Y()));
                }

                return Py::new_reference_to(points);
            }
            else {
                PyErr_SetString(PartExceptionOCCError, "Discretization of curve failed");
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
        if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "dd|ddi", kwds_TangentialDeflection, &angular, &curvature,
                                                &first, &last, &minimumPoints)) {
            GCPnts_TangentialDeflection discretizer(adapt, first, last, angular, curvature, minimumPoints);
            if (discretizer.NbPoints () > 0) {
                Py::List points;
                int nbPoints = discretizer.NbPoints ();

                for (int i=1; i<=nbPoints; i++) {
                    gp_Pnt p = discretizer.Value (i);
                    points.append(Base::Vector2dPy::create(p.X(), p.Y()));
                }

                return Py::new_reference_to(points);
            }
            else {
                PyErr_SetString(PartExceptionOCCError, "Discretization of curve failed");
                return nullptr;
            }
        }

        // use QuasiNumber kwds
        static const std::array<const char *, 4> kwds_QuasiNumPoints{"QuasiNumber", "First", "Last", nullptr};
        PyErr_Clear();
        int quasiNumPoints;
        if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "i|dd", kwds_QuasiNumPoints, &quasiNumPoints, &first,
                                                &last)) {
            GCPnts_QuasiUniformAbscissa discretizer(adapt, quasiNumPoints, first, last);
            if (discretizer.NbPoints () > 0) {
                Py::List points;
                int nbPoints = discretizer.NbPoints ();

                for (int i=1; i<=nbPoints; i++) {
                    gp_Pnt2d p = adapt.Value (discretizer.Parameter (i));
                    points.append(Base::Vector2dPy::create(p.X(), p.Y()));
                }

                return Py::new_reference_to(points);
            }
            else {
                PyErr_SetString(PartExceptionOCCError, "Discretization of curve failed");
                return nullptr;
            }
        }

        // use QuasiDeflection kwds
        static const std::array<const char *, 4> kwds_QuasiDeflection {"QuasiDeflection","First","Last",nullptr};
        PyErr_Clear();
        double quasiDeflection;
        if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "d|dd", kwds_QuasiDeflection, &quasiDeflection, &first,
                                                &last)) {
            GCPnts_QuasiUniformDeflection discretizer(adapt, quasiDeflection, first, last);
            if (discretizer.NbPoints () > 0) {
                Py::List points;
                int nbPoints = discretizer.NbPoints ();
                for (int i=1; i<=nbPoints; i++) {
                    gp_Pnt p = discretizer.Value (i);
                    points.append(Base::Vector2dPy::create(p.X(), p.Y()));
                }

                return Py::new_reference_to(points);
            }
            else {
                PyErr_SetString(PartExceptionOCCError, "Discretization of curve failed");
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

PyObject* Curve2dPy::length(PyObject *args)
{
    Handle(Geom2d_Geometry) g = getGeometry2dPtr()->handle();
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(g);
    try {
        if (!c.IsNull()) {
            double u=c->FirstParameter();
            double v=c->LastParameter();
            double t=Precision::Confusion();
            if (!PyArg_ParseTuple(args, "|ddd", &u,&v,&t))
                return nullptr;
            Geom2dAdaptor_Curve adapt(c);
            double len = GCPnts_AbscissaPoint::Length(adapt,u,v,t);
            return PyFloat_FromDouble(len);
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a curve");
    return nullptr;
}

PyObject* Curve2dPy::parameterAtDistance(PyObject *args)
{
    Handle(Geom2d_Geometry) g = getGeometry2dPtr()->handle();
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(g);
    try {
        if (!c.IsNull()) {
            double abscissa;
            double u = 0;
            if (!PyArg_ParseTuple(args, "d|d", &abscissa,&u))
                return nullptr;
            Geom2dAdaptor_Curve adapt(c);
            GCPnts_AbscissaPoint abscissaPoint(adapt,abscissa,u);
            double parm = abscissaPoint.Parameter();
            return PyFloat_FromDouble(parm);
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a curve");
    return nullptr;
}

PyObject* Curve2dPy::value(PyObject *args)
{
    Handle(Geom2d_Geometry) g = getGeometry2dPtr()->handle();
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(g);
    try {
        if (!c.IsNull()) {
            double u;
            if (!PyArg_ParseTuple(args, "d", &u))
                return nullptr;
            gp_Pnt2d p = c->Value(u);
            return Py::new_reference_to(Base::Vector2dPy::create(p.X(), p.Y()));
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a curve");
    return nullptr;
}

PyObject* Curve2dPy::tangent(PyObject *args)
{
    Handle(Geom2d_Geometry) g = getGeometry2dPtr()->handle();
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(g);
    try {
        if (!c.IsNull()) {
            double u;
            if (!PyArg_ParseTuple(args, "d", &u))
                return nullptr;
            gp_Dir2d dir;
            Geom2dLProp_CLProps2d prop(c,u,2,Precision::Confusion());
            if (prop.IsTangentDefined()) {
                prop.Tangent(dir);
            }

            return Py::new_reference_to(Base::Vector2dPy::create(dir.X(), dir.Y()));
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a curve");
    return nullptr;
}

PyObject* Curve2dPy::normal(PyObject *args)
{
    Handle(Geom2d_Geometry) g = getGeometry2dPtr()->handle();
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(g);
    try {
        if (!c.IsNull()) {
            double u;
            if (!PyArg_ParseTuple(args, "d", &u))
                return nullptr;
            gp_Dir2d dir;
            Geom2dLProp_CLProps2d prop(c,u,2,Precision::Confusion());
            prop.Normal(dir);

            return Py::new_reference_to(Base::Vector2dPy::create(dir.X(), dir.Y()));
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a curve");
    return nullptr;
}

PyObject* Curve2dPy::curvature(PyObject *args)
{
    Handle(Geom2d_Geometry) g = getGeometry2dPtr()->handle();
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(g);
    try {
        if (!c.IsNull()) {
            double u;
            if (!PyArg_ParseTuple(args, "d", &u))
                return nullptr;
            Geom2dLProp_CLProps2d prop(c,u,2,Precision::Confusion());
            double C = prop.Curvature();
            return Py::new_reference_to(Py::Float(C));
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a curve");
    return nullptr;
}

PyObject* Curve2dPy::centerOfCurvature(PyObject *args)
{
    Handle(Geom2d_Geometry) g = getGeometry2dPtr()->handle();
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(g);
    try {
        if (!c.IsNull()) {
            double u;
            if (!PyArg_ParseTuple(args, "d", &u))
                return nullptr;
            Geom2dLProp_CLProps2d prop(c,u,2,Precision::Confusion());
            gp_Pnt2d pnt ;
            prop.CentreOfCurvature(pnt);

            return Py::new_reference_to(Base::Vector2dPy::create(pnt.X(), pnt.Y()));
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a curve");
    return nullptr;
}

PyObject* Curve2dPy::parameter(PyObject *args)
{
    Handle(Geom2d_Geometry) g = getGeometry2dPtr()->handle();
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(g);
    try {
        if (!c.IsNull()) {
            PyObject *p;
            if (!PyArg_ParseTuple(args, "O!", Base::Vector2dPy::type_object(), &p))
                return nullptr;
            Base::Vector2d v = Py::toVector2d(p);
            gp_Pnt2d pnt(v.x,v.y);
            Geom2dAPI_ProjectPointOnCurve ppc(pnt, c);
            double val = ppc.LowerDistanceParameter();
            return Py::new_reference_to(Py::Float(val));
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a curve");
    return nullptr;
}

PyObject* Curve2dPy::toBSpline(PyObject * args)
{
    Handle(Geom2d_Geometry) g = getGeometry2dPtr()->handle();
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(g);
    try {
        if (!c.IsNull()) {
            double u,v;
            u=c->FirstParameter();
            v=c->LastParameter();
            if (!PyArg_ParseTuple(args, "|dd", &u,&v))
                return nullptr;
            ShapeConstruct_Curve scc;
            Handle(Geom2d_BSplineCurve) spline = scc.ConvertToBSpline(c, u, v, Precision::Confusion());
            if (spline.IsNull())
                Standard_NullValue::Raise("Conversion to B-spline failed");
            return new BSplineCurve2dPy(new Geom2dBSplineCurve(spline));
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a curve");
    return nullptr;
}

PyObject* Curve2dPy::approximateBSpline(PyObject *args)
{
    double tolerance;
    int maxSegment, maxDegree;
    char* order = "C2";
    if (!PyArg_ParseTuple(args, "dii|s", &tolerance, &maxSegment, &maxDegree, &order))
        return nullptr;

    GeomAbs_Shape absShape;
    std::string str = order;
    if (str == "C0")
        absShape = GeomAbs_C0;
    else if (str == "G1")
        absShape = GeomAbs_G1;
    else if (str == "C1")
        absShape = GeomAbs_C1;
    else if (str == "G2")
        absShape = GeomAbs_G2;
    else if (str == "C2")
        absShape = GeomAbs_C2;
    else if (str == "C3")
        absShape = GeomAbs_C3;
    else if (str == "CN")
        absShape = GeomAbs_CN;
    else
        absShape = GeomAbs_C2;

    try {
        Handle(Geom2d_Curve) self = Handle(Geom2d_Curve)::DownCast(getGeometry2dPtr()->handle());
        Geom2dConvert_ApproxCurve approx(self, tolerance, absShape, maxSegment, maxDegree);
        if (approx.IsDone()) {
            return new BSplineCurve2dPy(new Geom2dBSplineCurve(approx.Curve()));
        }
        else if (approx.HasResult()) {
            std::stringstream str;
            str << "Maximum error (" << approx.MaxError() << ") is outside tolerance";
            PyErr_SetString(PyExc_RuntimeError, str.str().c_str());
            return nullptr;
        }
        else {
            PyErr_SetString(PyExc_RuntimeError, "Approximation of curve failed");
            return nullptr;
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

Py::String Curve2dPy::getContinuity() const
{
    GeomAbs_Shape c = Handle(Geom2d_Curve)::DownCast
        (getGeometry2dPtr()->handle())->Continuity();
    std::string str;
    switch (c) {
    case GeomAbs_C0:
        str = "C0";
        break;
    case GeomAbs_G1:
        str = "G1";
        break;
    case GeomAbs_C1:
        str = "C1";
        break;
    case GeomAbs_G2:
        str = "G2";
        break;
    case GeomAbs_C2:
        str = "C2";
        break;
    case GeomAbs_C3:
        str = "C3";
        break;
    case GeomAbs_CN:
        str = "CN";
        break;
    default:
        str = "Unknown";
        break;
    }
    return Py::String(str);
}

Py::Boolean Curve2dPy::getClosed() const
{
    return Py::Boolean(Handle(Geom2d_Curve)::DownCast
        (getGeometry2dPtr()->handle())->IsClosed() ? true : false);
}

Py::Boolean Curve2dPy::getPeriodic() const
{
    return Py::Boolean(Handle(Geom2d_Curve)::DownCast
        (getGeometry2dPtr()->handle())->IsPeriodic() ? true : false);
}

Py::Float Curve2dPy::getFirstParameter() const
{
    return Py::Float(Handle(Geom2d_Curve)::DownCast
        (getGeometry2dPtr()->handle())->FirstParameter());
}

Py::Float Curve2dPy::getLastParameter() const
{
    return Py::Float(Handle(Geom2d_Curve)::DownCast
        (getGeometry2dPtr()->handle())->LastParameter());
}

PyObject *Curve2dPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int Curve2dPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

PyObject* Curve2dPy::intersectCC(PyObject *args)
{
    Handle(Geom2d_Curve) curve1 = Handle(Geom2d_Curve)::DownCast(getGeometry2dPtr()->handle());
    try {
        if (!curve1.IsNull()) {
            PyObject *p;
            double prec = Precision::Confusion();
            if (!PyArg_ParseTuple(args, "O!|d", &(Part::Curve2dPy::Type), &p, &prec))
                return nullptr;

            Handle(Geom2d_Curve) curve2 = Handle(Geom2d_Curve)::DownCast(static_cast<Geometry2dPy*>(p)->getGeometry2dPtr()->handle());
            Py::List points;
            Geom2dAPI_InterCurveCurve intersector(curve1, curve2, prec);
            if ((intersector.NbPoints() == 0) && (intersector.NbSegments() == 0)) {
                // No intersection
                return Py::new_reference_to(Py::List());
            }
            if (intersector.NbPoints() > 0) {
                // Cross intersections
                for (int i = 1; i <= intersector.NbPoints(); i++) {
                    gp_Pnt2d p1 = intersector.Point(i);
                    points.append(Base::Vector2dPy::create(p1.X(), p1.Y()));
                }
            }
            if (intersector.NbSegments() > 0) {
                // Tangential intersections
                Geom2dAPI_ExtremaCurveCurve intersector2(curve1, curve2,
                                                        curve1->FirstParameter(),
                                                        curve1->LastParameter(),
                                                        curve2->FirstParameter(),
                                                        curve2->LastParameter());
                for (int i = 1; i <= intersector2.NbExtrema(); i++) {
                    if (intersector2.Distance(i) > prec)
                        continue;
                    gp_Pnt2d p1, p2;
                    intersector2.Points(i, p1, p2);
                    points.append(Base::Vector2dPy::create(p1.X(), p1.Y()));
                }
            }
            return Py::new_reference_to(points);
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PyExc_TypeError, "Geometry is not a curve");
    return nullptr;
}
