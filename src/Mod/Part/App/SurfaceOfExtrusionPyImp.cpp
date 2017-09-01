/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Geom_SurfaceOfLinearExtrusion.hxx>
# include <Geom_Curve.hxx>
# include <Geom_Line.hxx>
# include <gp_Lin.hxx>
#endif

#include "Geometry.h"
#include "SurfaceOfExtrusionPy.h"
#include "SurfaceOfExtrusionPy.cpp"
#include "GeometryCurvePy.h"
#include "LinePy.h"
#include "BezierCurvePy.h"
#include "BSplineCurvePy.h"

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>
#include "OCCError.h"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string SurfaceOfExtrusionPy::representation(void) const
{
    return std::string("<SurfaceOfExtrusion object>");
}

PyObject *SurfaceOfExtrusionPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of SurfaceOfExtrusionPy and the Twin object 
    return new SurfaceOfExtrusionPy(new GeomSurfaceOfExtrusion);
}

// constructor method
int SurfaceOfExtrusionPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* pGeom;
    PyObject* pDir;
    if (!PyArg_ParseTuple(args, "O!O!", 
                            &(GeometryPy::Type), &pGeom, 
                            &(Base::VectorPy::Type),&pDir))
        return -1;

    GeometryPy* pcGeo = static_cast<GeometryPy*>(pGeom);
    Handle(Geom_Curve) curve = Handle(Geom_Curve)::DownCast
        (pcGeo->getGeometryPtr()->handle());
    if (curve.IsNull()) {
        PyErr_SetString(PyExc_TypeError, "geometry is not a curve");
        return -1;
    }

    try {
        Base::Vector3d dir = static_cast<Base::VectorPy*>(pDir)->value();
        Handle(Geom_SurfaceOfLinearExtrusion) curve2 = new Geom_SurfaceOfLinearExtrusion(curve,
            gp_Dir(dir.x,dir.y,dir.z));
        getGeomSurfaceOfExtrusionPtr()->setHandle(curve2);
        return 0;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return -1;
    }
}

Py::Object SurfaceOfExtrusionPy::getDirection(void) const
{
    Handle(Geom_SurfaceOfLinearExtrusion) curve = Handle(Geom_SurfaceOfLinearExtrusion)::DownCast
        (getGeometryPtr()->handle());
    const gp_Dir& dir = curve->Direction();
    return Py::Vector(Base::Vector3d(dir.X(),dir.Y(),dir.Z()));
}

void  SurfaceOfExtrusionPy::setDirection(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d dir = static_cast<Base::VectorPy*>(p)->value();
        Handle(Geom_SurfaceOfLinearExtrusion) curve = Handle(Geom_SurfaceOfLinearExtrusion)::DownCast
            (getGeometryPtr()->handle());
        curve->SetDirection(gp_Dir(dir.x,dir.y,dir.z));
    }
    else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        Base::Vector3d dir = Base::getVectorFromTuple<double>(p);
        Handle(Geom_SurfaceOfLinearExtrusion) curve = Handle(Geom_SurfaceOfLinearExtrusion)::DownCast
            (getGeometryPtr()->handle());
        curve->SetDirection(gp_Dir(dir.x,dir.y,dir.z));
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object SurfaceOfExtrusionPy::getBasisCurve(void) const
{
    throw Py::Exception(PyExc_NotImplementedError, "Not yet implemented");
}

void  SurfaceOfExtrusionPy::setBasisCurve(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(GeometryPy::Type))) {
        GeometryPy* pcGeo = static_cast<GeometryPy*>(p);
        Handle(Geom_Curve) curve = Handle(Geom_Curve)::DownCast
            (pcGeo->getGeometryPtr()->handle());
        if (curve.IsNull()) {
            throw Py::TypeError("geometry is not a curve");
        }

        try {
            Handle(Geom_SurfaceOfLinearExtrusion) curve2 = Handle(Geom_SurfaceOfLinearExtrusion)::DownCast
                (getGeometryPtr()->handle());
            curve2->SetBasisCurve(curve);
        }
        catch (Standard_Failure& e) {
    
            throw Py::Exception(e.GetMessageString());
        }
    }
}

PyObject* SurfaceOfExtrusionPy::uIso(PyObject * args)
{
    double v;
    if (!PyArg_ParseTuple(args, "d", &v))
        return 0;

    try {
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
            (getGeometryPtr()->handle());
        Handle(Geom_Curve) c = surf->UIso(v);
        if (c->IsKind(STANDARD_TYPE(Geom_TrimmedCurve))) {
            Handle(Geom_TrimmedCurve) aCurve = Handle(Geom_TrimmedCurve)::DownCast(c);
            return new GeometryCurvePy(new GeomTrimmedCurve(aCurve));
        }
        if (c->IsKind(STANDARD_TYPE(Geom_BezierCurve))) {
            Handle(Geom_BezierCurve) aCurve = Handle(Geom_BezierCurve)::DownCast(c);
            return new BezierCurvePy(new GeomBezierCurve(aCurve));
        }
        if (c->IsKind(STANDARD_TYPE(Geom_BSplineCurve))) {
            Handle(Geom_BSplineCurve) aCurve = Handle(Geom_BSplineCurve)::DownCast(c);
            return new BSplineCurvePy(new GeomBSplineCurve(aCurve));
        }
        if (c->IsKind(STANDARD_TYPE(Geom_Line))) {
            Handle(Geom_Line) aLine = Handle(Geom_Line)::DownCast(c);
            GeomLine* line = new GeomLine();
            Handle(Geom_Line) this_line = Handle(Geom_Line)::DownCast
                (line->handle());
            this_line->SetLin(aLine->Lin());
            return new LinePy(line);
        }
        PyErr_Format(PyExc_NotImplementedError, "Iso curve is of type '%s'",
            c->DynamicType()->Name());
        return 0;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* SurfaceOfExtrusionPy::vIso(PyObject * args)
{
    double v;
    if (!PyArg_ParseTuple(args, "d", &v))
        return 0;

    try {
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast
            (getGeometryPtr()->handle());
        Handle(Geom_Curve) c = surf->VIso(v);
        if (c->IsKind(STANDARD_TYPE(Geom_TrimmedCurve))) {
            Handle(Geom_TrimmedCurve) aCurve = Handle(Geom_TrimmedCurve)::DownCast(c);
            return new GeometryCurvePy(new GeomTrimmedCurve(aCurve));
        }
        if (c->IsKind(STANDARD_TYPE(Geom_BezierCurve))) {
            Handle(Geom_BezierCurve) aCurve = Handle(Geom_BezierCurve)::DownCast(c);
            return new BezierCurvePy(new GeomBezierCurve(aCurve));
        }
        if (c->IsKind(STANDARD_TYPE(Geom_BSplineCurve))) {
            Handle(Geom_BSplineCurve) aCurve = Handle(Geom_BSplineCurve)::DownCast(c);
            return new BSplineCurvePy(new GeomBSplineCurve(aCurve));
        }
        if (c->IsKind(STANDARD_TYPE(Geom_Line))) {
            Handle(Geom_Line) aLine = Handle(Geom_Line)::DownCast(c);
            GeomLine* line = new GeomLine();
            Handle(Geom_Line) this_curv = Handle(Geom_Line)::DownCast
                (line->handle());
            this_curv->SetLin(aLine->Lin());
            return new LinePy(line);
        }
        PyErr_Format(PyExc_NotImplementedError, "Iso curve is of type '%s'",
            c->DynamicType()->Name());
        return 0;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject *SurfaceOfExtrusionPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int SurfaceOfExtrusionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
