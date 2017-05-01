/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepBuilderAPI_MakeFace.hxx>
# include <gp_Dir.hxx>
# include <gp_Vec.hxx>
# include <gp_Lin.hxx>
# include <Geom_Geometry.hxx>
# include <Geom_Surface.hxx>
# include <GeomConvert_ApproxSurface.hxx>
# include <GeomLProp_SLProps.hxx>
# include <Precision.hxx>
# include <Standard_Failure.hxx>
# include <Standard_Version.hxx>
# include <ShapeAnalysis_Surface.hxx>
# include <GeomAPI_IntSS.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "OCCError.h"
#include "Geometry.h"
#include <Mod/Part/App/GeometrySurfacePy.h>
#include <Mod/Part/App/GeometrySurfacePy.cpp>
#include <Mod/Part/App/GeometryCurvePy.h>
#include <Mod/Part/App/BSplineSurfacePy.h>

#include <Mod/Part/App/LinePy.h>
#include <Mod/Part/App/BezierCurvePy.h>
#include <Mod/Part/App/BSplineCurvePy.h>
#include <Mod/Part/App/CirclePy.h>
#include <Mod/Part/App/EllipsePy.h>
#include <Mod/Part/App/HyperbolaPy.h>
#include <Mod/Part/App/ParabolaPy.h>
#include <Mod/Part/App/OffsetCurvePy.h>

#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapePy.h>
#include <Mod/Part/App/TopoShapeFacePy.h>

namespace Part {
const Py::Object makeGeometryCurvePy(const Handle(Geom_Curve)& c)
{
    if (c->IsKind(STANDARD_TYPE(Geom_Circle))) {
        Handle(Geom_Circle) circ = Handle(Geom_Circle)::DownCast(c);
        return Py::asObject(new CirclePy(new GeomCircle(circ)));
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_Ellipse))) {
        Handle(Geom_Ellipse) ell = Handle(Geom_Ellipse)::DownCast(c);
        return Py::asObject(new EllipsePy(new GeomEllipse(ell)));
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_Hyperbola))) {
        Handle(Geom_Hyperbola) hyp = Handle(Geom_Hyperbola)::DownCast(c);
        return Py::asObject(new HyperbolaPy(new GeomHyperbola(hyp)));
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_Line))) {
        Handle(Geom_Line) lin = Handle(Geom_Line)::DownCast(c);
        return Py::asObject(new GeometryCurvePy(new GeomLine(lin)));
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_OffsetCurve))) {
        Handle(Geom_OffsetCurve) oc = Handle(Geom_OffsetCurve)::DownCast(c);
        return Py::asObject(new OffsetCurvePy(new GeomOffsetCurve(oc)));
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_Parabola))) {
        Handle(Geom_Parabola) par = Handle(Geom_Parabola)::DownCast(c);
        return Py::asObject(new ParabolaPy(new GeomParabola(par)));
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_TrimmedCurve))) {
        Handle(Geom_TrimmedCurve) trc = Handle(Geom_TrimmedCurve)::DownCast(c);
        return Py::asObject(new GeometryCurvePy(new GeomTrimmedCurve(trc)));
    }
    /*else if (c->IsKind(STANDARD_TYPE(Geom_BoundedCurve))) {
        Handle(Geom_BoundedCurve) bc = Handle(Geom_BoundedCurve)::DownCast(c);
        return Py::asObject(new GeometryCurvePy(new GeomBoundedCurve(bc)));
    }*/
    else if (c->IsKind(STANDARD_TYPE(Geom_BezierCurve))) {
        Handle(Geom_BezierCurve) bezier = Handle(Geom_BezierCurve)::DownCast(c);
        return Py::asObject(new BezierCurvePy(new GeomBezierCurve(bezier)));
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_BSplineCurve))) {
        Handle(Geom_BSplineCurve) bspline = Handle(Geom_BSplineCurve)::DownCast(c);
        return Py::asObject(new BSplineCurvePy(new GeomBSplineCurve(bspline)));
    }

    std::string err = "Unhandled curve type ";
    err += c->DynamicType()->Name();
    throw Py::TypeError(err);
}

} // Part

// ---------------------------------------

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string GeometrySurfacePy::representation(void) const
{
    return "<Surface object>";
}

PyObject *GeometrySurfacePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'GeometrySurface'.");
    return 0;
}

// constructor method
int GeometrySurfacePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* GeometrySurfacePy::toShape(PyObject *args)
{
    Handle(Geom_Geometry) g = getGeometryPtr()->handle();
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(g);
    try {
        if (!s.IsNull()) {
            double u1,u2,v1,v2;
            s->Bounds(u1,u2,v1,v2);
            if (!PyArg_ParseTuple(args, "|dddd", &u1,&u2,&v1,&v2))
                return 0;
            BRepBuilderAPI_MakeFace mkBuilder(s, u1, u2, v1, v2
#if OCC_VERSION_HEX >= 0x060502
              , Precision::Confusion()
#endif
            );
            TopoDS_Shape sh = mkBuilder.Shape();
            return new TopoShapeFacePy(new TopoShape(sh));
        }
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return 0;
}

PyObject* GeometrySurfacePy::value(PyObject *args)
{
    Handle(Geom_Geometry) g = getGeometryPtr()->handle();
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(g);
    try {
        if (!s.IsNull()) {
            double u,v;
            if (!PyArg_ParseTuple(args, "dd", &u,&v))
                return 0;
            gp_Pnt p = s->Value(u,v);
            return new Base::VectorPy(Base::Vector3d(p.X(),p.Y(),p.Z()));
        }
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return 0;
}

PyObject* GeometrySurfacePy::tangent(PyObject *args)
{
    Handle(Geom_Geometry) g = getGeometryPtr()->handle();
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(g);
    try {
        if (!s.IsNull()) {
            double u,v;
            if (!PyArg_ParseTuple(args, "dd", &u,&v))
                return 0;
            gp_Dir dir;
            Py::Tuple tuple(2);
            GeomLProp_SLProps prop(s,u,v,1,Precision::Confusion());
            if (prop.IsTangentUDefined()) {
                prop.TangentU(dir);
                tuple.setItem(0, Py::Vector(Base::Vector3d(dir.X(),dir.Y(),dir.Z())));
            }
            if (prop.IsTangentVDefined()) {
                prop.TangentV(dir);
                tuple.setItem(1, Py::Vector(Base::Vector3d(dir.X(),dir.Y(),dir.Z())));
            }

            return Py::new_reference_to(tuple);
        }
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return 0;
}

PyObject* GeometrySurfacePy::parameter(PyObject *args)
{
    Handle(Geom_Surface) surf = Handle(Geom_Surface)
        ::DownCast(getGeometryPtr()->handle());
    try {
        if (!surf.IsNull()) {
            PyObject *p;
            double prec = Precision::Confusion();
            if (!PyArg_ParseTuple(args, "O!|d", &(Base::VectorPy::Type), &p, &prec))
                return 0;
            Base::Vector3d v = Py::Vector(p, false).toVector();
            gp_Pnt pnt(v.x,v.y,v.z);
            ShapeAnalysis_Surface as(surf);
            gp_Pnt2d uv = as.ValueOfUV(pnt, prec);
            Py::Tuple tuple(2);
            tuple.setItem(0, Py::Float(uv.X()));
            tuple.setItem(1, Py::Float(uv.Y()));
            return Py::new_reference_to(tuple);
        }
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return 0;
}

PyObject* GeometrySurfacePy::bounds(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle(Geom_Surface) surf = Handle(Geom_Surface)
        ::DownCast(getGeometryPtr()->handle());
    Py::Tuple bound(4);
    Standard_Real u1,u2,v1,v2;
    surf->Bounds(u1,u2,v1,v2);
    bound.setItem(0,Py::Float(u1));
    bound.setItem(1,Py::Float(u2));
    bound.setItem(2,Py::Float(v1));
    bound.setItem(3,Py::Float(v2));
    return Py::new_reference_to(bound);
}

PyObject* GeometrySurfacePy::uIso(PyObject * args)
{
    double v;
    if (!PyArg_ParseTuple(args, "d", &v))
        return 0;

    try {
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
            (getGeometryPtr()->handle());
        Handle(Geom_Curve) c = surf->UIso(v);
        if (c.IsNull()) {
            PyErr_SetString(PyExc_RuntimeError, "failed to create u iso curve");
            return 0;
        }

        if (c->IsKind(STANDARD_TYPE(Geom_Line))) {
            Handle(Geom_Line) aLine = Handle(Geom_Line)::DownCast(c);
            GeomLine* line = new GeomLine();
            Handle(Geom_Line) this_curv = Handle(Geom_Line)::DownCast
                (line->handle());
            this_curv->SetLin(aLine->Lin());
            return new LinePy(line);
        }
        else {
            return Py::new_reference_to(makeGeometryCurvePy(c));
        }
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }
}

PyObject* GeometrySurfacePy::vIso(PyObject * args)
{
    double v;
    if (!PyArg_ParseTuple(args, "d", &v))
        return 0;

    try {
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
            (getGeometryPtr()->handle());
        Handle(Geom_Curve) c = surf->VIso(v);
        if (c.IsNull()) {
            PyErr_SetString(PyExc_RuntimeError, "failed to create v iso curve");
            return 0;
        }

        if (c->IsKind(STANDARD_TYPE(Geom_Line))) {
            Handle(Geom_Line) aLine = Handle(Geom_Line)::DownCast(c);
            GeomLine* line = new GeomLine();
            Handle(Geom_Line) this_curv = Handle(Geom_Line)::DownCast
                (line->handle());
            this_curv->SetLin(aLine->Lin());
            return new LinePy(line);
        }
        else {
            return Py::new_reference_to(makeGeometryCurvePy(c));
        }
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }
}

PyObject* GeometrySurfacePy::isUPeriodic(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsUPeriodic();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* GeometrySurfacePy::isVPeriodic(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsVPeriodic();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* GeometrySurfacePy::isUClosed(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsUClosed();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* GeometrySurfacePy::isVClosed(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsVClosed();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* GeometrySurfacePy::UPeriod(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
            (getGeometryPtr()->handle());
        Standard_Real val = surf->UPeriod();
        return PyFloat_FromDouble(val);
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }
}

PyObject* GeometrySurfacePy::VPeriod(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
            (getGeometryPtr()->handle());
        Standard_Real val = surf->VPeriod();
        return PyFloat_FromDouble(val);
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return 0;
    }
}

Py::String GeometrySurfacePy::getContinuity(void) const
{
    GeomAbs_Shape c = Handle(Geom_Surface)::DownCast
        (getGeometryPtr()->handle())->Continuity();
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

PyObject* GeometrySurfacePy::toBSpline(PyObject * args)
{
    double tol3d;
    char *ucont, *vcont;
    int maxDegU,maxDegV,maxSegm,prec=0;
    if (!PyArg_ParseTuple(args, "dssiii|i",&tol3d,&ucont,&vcont,
                                           &maxDegU,&maxDegV,&maxSegm,&prec))
        return 0;

    std::string uc = ucont;
    GeomAbs_Shape absU, absV;
    if (uc == "C0")
        absU = GeomAbs_C0;
    else if (uc == "C1")
        absU = GeomAbs_C1;
    else if (uc == "C2")
        absU = GeomAbs_C2;
    else if (uc == "C3")
        absU = GeomAbs_C3;
    else if (uc == "CN")
        absU = GeomAbs_CN;
    else if (uc == "G1")
        absU = GeomAbs_G1;
    else
        absU = GeomAbs_G2;

    std::string vc = vcont;
    if (vc == "C0")
        absV = GeomAbs_C0;
    else if (vc == "C1")
        absV = GeomAbs_C1;
    else if (vc == "C2")
        absV = GeomAbs_C2;
    else if (vc == "C3")
        absV = GeomAbs_C3;
    else if (vc == "CN")
        absV = GeomAbs_CN;
    else if (vc == "G1")
        absV = GeomAbs_G1;
    else
        absV = GeomAbs_G2;

    try {
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
            (getGeometryPtr()->handle());
        GeomConvert_ApproxSurface cvt(surf, tol3d, absU, absV, maxDegU, maxDegV, maxSegm, prec);
        if (cvt.IsDone() && cvt.HasResult()) {
            return new BSplineSurfacePy(new GeomBSplineSurface(cvt.Surface()));
        }
        else {
            Standard_Failure::Raise("Cannot convert to B-Spline surface");
        }
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
    }

    return 0;
}

PyObject *GeometrySurfacePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int GeometrySurfacePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}

// Specialized intersection functions

PyObject* GeometrySurfacePy::intersectSS(PyObject *args)
{
    Handle(Geom_Surface) surf1 = Handle(Geom_Surface)::DownCast(getGeometryPtr()->handle());
    try {
        if (!surf1.IsNull()) {
            PyObject *p;
            double prec = Precision::Confusion();
            if (!PyArg_ParseTuple(args, "O!|d", &(Part::GeometrySurfacePy::Type), &p, &prec))
                return 0;
            Handle(Geom_Surface) surf2 = Handle(Geom_Surface)::DownCast(static_cast<GeometryPy*>(p)->getGeometryPtr()->handle());
            GeomAPI_IntSS intersector(surf1, surf2, prec);
            if (!intersector.IsDone()) {
                PyErr_SetString(PyExc_RuntimeError, "Intersection of surfaces failed");
                return 0;
            }

            Py::List result;
            for (int i = 1; i <= intersector.NbLines(); i++) {
                Handle(Geom_Curve) line = intersector.Line(i);
                result.append(makeGeometryCurvePy(line));
            }

            return Py::new_reference_to(result);
        }
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_RuntimeError, e->GetMessageString());
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "intersectSS(): Geometry is not a surface");
    return 0;
}

// General intersection function

PyObject* GeometrySurfacePy::intersect(PyObject *args)
{
    Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast(getGeometryPtr()->handle());
    try {
        if (!surf.IsNull()) {
            PyObject *p;
            double prec = Precision::Confusion();

            try {
                if (PyArg_ParseTuple(args, "O!|d", &(Part::GeometrySurfacePy::Type), &p, &prec))
                    return intersectSS(args);
            } catch(...) {};
            PyErr_Clear();

            if (PyArg_ParseTuple(args, "O!|d", &(Part::GeometryCurvePy::Type), &p, &prec)) {
                GeometryCurvePy* curve = static_cast<GeometryCurvePy*>(p);
                PyObject* t = PyTuple_New(2);
                PyTuple_SetItem(t, 0, this);
                PyTuple_SetItem(t, 1, PyFloat_FromDouble(prec));
                return curve->intersectCS(t);
            } else {
                return 0;
            }
        }
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_RuntimeError, e->GetMessageString());
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "intersect(): Geometry is not a surface");
    return 0;
}
