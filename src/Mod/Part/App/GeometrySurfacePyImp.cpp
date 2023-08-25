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
# include <BRepBuilderAPI_MakeShell.hxx>
# include <Geom_BSplineSurface.hxx>
# include <Geom_Geometry.hxx>
# include <Geom_Surface.hxx>
# include <GeomAPI_IntSS.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <GeomConvert_ApproxSurface.hxx>
# include <GeomLib_IsPlanarSurface.hxx>
# include <GeomLProp_SLProps.hxx>
# include <gp_Dir.hxx>
# include <gp_Quaternion.hxx>
# include <gp_Vec.hxx>
# include <Precision.hxx>
# include <ShapeAnalysis_Surface.hxx>
# include <Standard_Failure.hxx>
# include <Standard_Version.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>

#include "GeometrySurfacePy.h"
#include "GeometrySurfacePy.cpp"
#include "BSplineSurfacePy.h"
#include "GeometryCurvePy.h"
#include "LinePy.h"
#include "OCCError.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeShellPy.h"


namespace Part {
const Py::Object makeTrimmedCurvePy(const Handle(Geom_Curve)& c, double f, double l)
{
    try {
        std::unique_ptr<GeomCurve> gc(makeFromTrimmedCurve(c, f, l));
        return Py::asObject(gc->getPyObject());
    }
    catch (const Base::Exception& e) {
        throw Py::TypeError(e.what());
    }
}

const Py::Object makeGeometryCurvePy(const Handle(Geom_Curve)& c)
{
    try {
        std::unique_ptr<GeomCurve> gc(makeFromCurve(c));
        return Py::asObject(gc->getPyObject());
    }
    catch (const Base::Exception& e) {
        throw Py::TypeError(e.what());
    }
}

} // Part

// ---------------------------------------

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string GeometrySurfacePy::representation() const
{
    return "<Surface object>";
}

PyObject *GeometrySurfacePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'GeometrySurface'.");
    return nullptr;
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
                return nullptr;
            BRepBuilderAPI_MakeFace mkBuilder(s, u1, u2, v1, v2, Precision::Confusion() );
            TopoDS_Shape sh = mkBuilder.Shape();
            return new TopoShapeFacePy(new TopoShape(sh));
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return nullptr;
}

PyObject* GeometrySurfacePy::toShell(PyObject *args, PyObject* kwds)
{
    PyObject* bound = nullptr;
    PyObject* segm = nullptr;
    static const std::array<const char *, 3> kwlist {"Bounds", "Segment", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist,
                                             &PyTuple_Type, &bound, &PyBool_Type, &segm)) {
        return nullptr;
    }

    Handle(Geom_Geometry) g = getGeometryPtr()->handle();
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(g);
    try {
        if (!s.IsNull()) {
            if (segm) {
                Standard_Boolean segment = Base::asBoolean(segm);
                BRepBuilderAPI_MakeShell mkBuilder(s, segment);
                TopoDS_Shape sh = mkBuilder.Shape();
                return new TopoShapeShellPy(new TopoShape(sh));
            }
            else {
                double u1,u2,v1,v2;
                s->Bounds(u1,u2,v1,v2);

                if (bound) {
                    Py::Tuple tuple(bound);
                    u1 = double(Py::Float(tuple[0]));
                    u2 = double(Py::Float(tuple[1]));
                    v1 = double(Py::Float(tuple[2]));
                    v2 = double(Py::Float(tuple[3]));
                }

                BRepBuilderAPI_MakeShell mkBuilder(s, u1, u2, v1, v2);
                TopoDS_Shape sh = mkBuilder.Shape();
                return new TopoShapeShellPy(new TopoShape(sh));
            }
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return nullptr;
}

PyObject* GeometrySurfacePy::getD0(PyObject *args)
{
    Handle(Geom_Geometry) g = getGeometryPtr()->handle();
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(g);
    try {
        if (!s.IsNull()) {
            double u,v;
            if (!PyArg_ParseTuple(args, "dd", &u, &v))
                return nullptr;
            gp_Pnt p;
            s->D0(u, v, p);
            return new Base::VectorPy(Base::Vector3d(p.X(),p.Y(),p.Z()));
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return nullptr;
}

PyObject* GeometrySurfacePy::getDN(PyObject *args)
{
    try {
        int nu, nv;
        double u,v;
        if (!PyArg_ParseTuple(args, "ddii", &u, &v, &nu, &nv))
            return nullptr;
        gp_Vec v1 = getGeomSurfacePtr()->getDN(u, v, nu, nv);
        return new Base::VectorPy(Base::Vector3d(v1.X(),v1.Y(),v1.Z()));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* GeometrySurfacePy::value(PyObject *args)
{
    Handle(Geom_Geometry) g = getGeometryPtr()->handle();
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(g);
    try {
        if (!s.IsNull()) {
            double u,v;
            if (!PyArg_ParseTuple(args, "dd", &u,&v))
                return nullptr;
            gp_Pnt p = s->Value(u,v);
            return new Base::VectorPy(Base::Vector3d(p.X(),p.Y(),p.Z()));
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return nullptr;
}

PyObject* GeometrySurfacePy::tangent(PyObject *args)
{
    Handle(Geom_Geometry) g = getGeometryPtr()->handle();
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(g);
    try {
        if (!s.IsNull()) {
            double u,v;
            if (!PyArg_ParseTuple(args, "dd", &u,&v))
                return nullptr;
            gp_Dir dir;
            Py::Tuple tuple(2);
            GeomLProp_SLProps prop(s,u,v,2,Precision::Confusion());
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
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return nullptr;
}

PyObject* GeometrySurfacePy::normal(PyObject *args)
{
    try {
        GeomSurface* s = getGeomSurfacePtr();
        if (s) {
            double u,v;
            if (!PyArg_ParseTuple(args, "dd", &u,&v))
                return nullptr;
            gp_Dir d;
            if (s->normal(u,v,d)) {
                return new Base::VectorPy(Base::Vector3d(d.X(),d.Y(),d.Z()));
            }
            else {
                PyErr_SetString(PyExc_RuntimeError, "normal at this point is not defined");
                return nullptr;
            }
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return nullptr;
}

PyObject* GeometrySurfacePy::projectPoint(PyObject *args, PyObject* kwds)
{
    PyObject* v;
    const char* meth = "NearestPoint";
    static const std::array<const char *, 3> kwlist {"Point", "Method", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!|s", kwlist, &Base::VectorPy::Type, &v, &meth)) {
        return nullptr;
    }

    try {
        Base::Vector3d vec = Py::Vector(v, false).toVector();
        gp_Pnt pnt(vec.x, vec.y, vec.z);
        std::string method = meth;

        Handle(Geom_Geometry) geom = getGeometryPtr()->handle();
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast(geom);

        GeomAPI_ProjectPointOnSurf proj(pnt, surf);
        if (method == "NearestPoint") {
            pnt = proj.NearestPoint();
            vec.Set(pnt.X(), pnt.Y(), pnt.Z());
            return new Base::VectorPy(vec);
        }
        else if (method == "LowerDistance") {
            Py::Float dist(proj.LowerDistance());
            return Py::new_reference_to(dist);
        }
        else if (method == "LowerDistanceParameters") {
            Standard_Real u, v;
            proj.LowerDistanceParameters(u, v);
            Py::Tuple par(2);
            par.setItem(0, Py::Float(u));
            par.setItem(1, Py::Float(v));
            return Py::new_reference_to(par);
        }
        else if (method == "Distance") {
            Standard_Integer num = proj.NbPoints();
            Py::List list;
            for (Standard_Integer i=1; i <= num; i++) {
                list.append(Py::Float(proj.Distance(i)));
            }
            return Py::new_reference_to(list);
        }
        else if (method == "Parameters") {
            Standard_Integer num = proj.NbPoints();
            Py::List list;
            for (Standard_Integer i=1; i <= num; i++) {
                Standard_Real u, v;
                proj.Parameters(i, u, v);
                Py::Tuple par(2);
                par.setItem(0, Py::Float(u));
                par.setItem(1, Py::Float(v));
                list.append(par);
            }
            return Py::new_reference_to(list);
        }
        else if (method == "Point") {
            Standard_Integer num = proj.NbPoints();
            Py::List list;
            for (Standard_Integer i=1; i <= num; i++) {
                gp_Pnt pnt = proj.Point(i);
                Base::Vector3d vec(pnt.X(), pnt.Y(), pnt.Z());
                list.append(Py::Vector(vec));
            }
            return Py::new_reference_to(list);
        }
        else {
            PyErr_SetString(PartExceptionOCCError, "Unsupported method");
            return nullptr;
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* GeometrySurfacePy::isUmbillic(PyObject *args)
{
    try {
        GeomSurface* s = getGeomSurfacePtr();
        if (s) {
            double u,v;
            if (!PyArg_ParseTuple(args, "dd", &u,&v))
                return nullptr;

            bool val = s->isUmbillic(u,v);
            return PyBool_FromLong(val ? 1 : 0);
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return nullptr;
}

PyObject* GeometrySurfacePy::curvatureDirections(PyObject *args)
{
    try {
        GeomSurface* s = getGeomSurfacePtr();
        if (s) {
            double u,v;
            if (!PyArg_ParseTuple(args, "dd", &u,&v))
                return nullptr;

            gp_Dir maxd, mind;
            s->curvatureDirections(u,v,maxd,mind);

            Py::Tuple tuple(2);
            tuple.setItem(0, Py::Vector(Base::Vector3d(maxd.X(),maxd.Y(),maxd.Z())));
            tuple.setItem(1, Py::Vector(Base::Vector3d(mind.X(),mind.Y(),mind.Z())));
            return Py::new_reference_to(tuple);
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return nullptr;
}

PyObject* GeometrySurfacePy::curvature(PyObject *args)
{
    try {
        GeomSurface* s = getGeomSurfacePtr();
        if (s) {
            double u,v;
            char* type;
            if (!PyArg_ParseTuple(args, "dds", &u,&v,&type))
                return nullptr;

            GeomSurface::Curvature t;
            if (strcmp(type,"Max") == 0) {
                t = GeomSurface::Maximum;
            }
            else if (strcmp(type,"Min") == 0) {
                t = GeomSurface::Minimum;
            }
            else if (strcmp(type,"Mean") == 0) {
                t = GeomSurface::Mean;
            }
            else if (strcmp(type,"Gauss") == 0) {
                t = GeomSurface::Gaussian;
            }
            else {
                PyErr_SetString(PyExc_ValueError, "unknown curvature type");
                return nullptr;
            }

            double c = s->curvature(u,v,t);
            return PyFloat_FromDouble(c);
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return nullptr;
}

PyObject* GeometrySurfacePy::isPlanar(PyObject *args)
{
    try {
        Handle(Geom_Surface) surf = Handle(Geom_Surface)
            ::DownCast(getGeometryPtr()->handle());
        if (!surf.IsNull()) {
            double tol = Precision::Confusion();
            if (!PyArg_ParseTuple(args, "|d", &tol))
                return nullptr;

            GeomLib_IsPlanarSurface check(surf, tol);
            Standard_Boolean val = check.IsPlanar();
            return PyBool_FromLong(val ? 1 : 0);
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return nullptr;
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
                return nullptr;
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
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a surface");
    return nullptr;
}

PyObject* GeometrySurfacePy::bounds(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

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
        return nullptr;

    try {
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
            (getGeometryPtr()->handle());
        Handle(Geom_Curve) c = surf->UIso(v);
        if (c.IsNull()) {
            PyErr_SetString(PyExc_RuntimeError, "failed to create u iso curve");
            return nullptr;
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
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* GeometrySurfacePy::vIso(PyObject * args)
{
    double v;
    if (!PyArg_ParseTuple(args, "d", &v))
        return nullptr;

    try {
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
            (getGeometryPtr()->handle());
        Handle(Geom_Curve) c = surf->VIso(v);
        if (c.IsNull()) {
            PyErr_SetString(PyExc_RuntimeError, "failed to create v iso curve");
            return nullptr;
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
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* GeometrySurfacePy::isUPeriodic(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsUPeriodic();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* GeometrySurfacePy::isVPeriodic(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsVPeriodic();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* GeometrySurfacePy::isUClosed(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsUClosed();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* GeometrySurfacePy::isVClosed(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = surf->IsVClosed();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* GeometrySurfacePy::UPeriod(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
            (getGeometryPtr()->handle());
        Standard_Real val = surf->UPeriod();
        return PyFloat_FromDouble(val);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* GeometrySurfacePy::VPeriod(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
            (getGeometryPtr()->handle());
        Standard_Real val = surf->VPeriod();
        return PyFloat_FromDouble(val);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

Py::String GeometrySurfacePy::getContinuity() const
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

PyObject* GeometrySurfacePy::toBSpline(PyObject * args, PyObject * kwds)
{
    double tol3d=Precision::Confusion();
    char *ucont="C1", *vcont="C1";
    int maxDegU=Geom_BSplineSurface::MaxDegree();
    int maxDegV=Geom_BSplineSurface::MaxDegree();
    int maxSegm=1000, prec=0;

    static const std::array<const char *, 8> kwlist{"Tol3d", "UContinuity", "VContinuity", "MaxDegreeU", "MaxDegreeV",
                                                    "MaxSegments", "PrecisCode", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "|dssiiii", kwlist,
                                             &tol3d, &ucont, &vcont,
                                             &maxDegU, &maxDegV, &maxSegm, &prec)) {
        return nullptr;
    }

    GeomAbs_Shape absU, absV;
    std::string uc = ucont;
    if (maxDegU <= 1)
        absU = GeomAbs_C0;
    else if (uc == "C0")
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
    if (maxDegV <= 1)
        absV = GeomAbs_C0;
    else if (vc == "C0")
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
            Standard_Failure::Raise("Cannot convert to B-spline surface");
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
    }

    return nullptr;
}

PyObject *GeometrySurfacePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
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
                return nullptr;
            Handle(Geom_Surface) surf2 = Handle(Geom_Surface)::DownCast(static_cast<GeometryPy*>(p)->getGeometryPtr()->handle());
            GeomAPI_IntSS intersector(surf1, surf2, prec);
            if (!intersector.IsDone()) {
                PyErr_SetString(PyExc_RuntimeError, "Intersection of surfaces failed");
                return nullptr;
            }

            Py::List result;
            for (int i = 1; i <= intersector.NbLines(); i++) {
                Handle(Geom_Curve) line = intersector.Line(i);
                result.append(makeGeometryCurvePy(line));
            }

            return Py::new_reference_to(result);
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PyExc_TypeError, "intersectSS(): Geometry is not a surface");
    return nullptr;
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
                Py_INCREF(this);
                PyTuple_SetItem(t, 0, this);
                PyTuple_SetItem(t, 1, PyFloat_FromDouble(prec));
                return curve->intersectCS(t);
            } else {
                return nullptr;
            }
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PyExc_TypeError, "intersect(): Geometry is not a surface");
    return nullptr;
}

Py::Object GeometrySurfacePy::getRotation() const
{
    Handle(Geom_ElementarySurface) s = Handle(Geom_ElementarySurface)::DownCast
        (getGeometryPtr()->handle());
    if(!s)
        return Py::Object();
    gp_Trsf trsf;
    trsf.SetTransformation(s->Position().Ax2(),gp_Ax3());
    auto q = trsf.GetRotation();
    return Py::Rotation(Base::Rotation(q.X(),q.Y(),q.Z(),q.W()));
}

