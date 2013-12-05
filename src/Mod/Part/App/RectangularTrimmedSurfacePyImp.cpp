/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Geom_RectangularTrimmedSurface.hxx>
# include <Geom_Curve.hxx>
#endif

#include "Mod/Part/App/Geometry.h"

// inclusion of the generated files (generated out of RectangularTrimmedSurfacePy.xml)
#include "RectangularTrimmedSurfacePy.h"
#include "RectangularTrimmedSurfacePy.cpp"
#include "GeometryCurvePy.h"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string RectangularTrimmedSurfacePy::representation(void) const
{
    return std::string("<RectangularTrimmedSurface object>");
}

PyObject *RectangularTrimmedSurfacePy::PyMake(struct _typeobject *, PyObject *, PyObject *)
{
    // create a new instance of RectangularTrimmedSurfacePy and the Twin object 
    return new RectangularTrimmedSurfacePy(new GeomTrimmedSurface);
}

// constructor method
int RectangularTrimmedSurfacePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* surf;
    double u1,u2,v1,v2;
    PyObject *usense=Py_True, *vsense=Py_True;
    if (PyArg_ParseTuple(args, "O!dddd|O!O!",&(Part::GeometrySurfacePy::Type),&surf,
                         &u1,&u2,&v1,&v2,&PyBool_Type,&usense,&PyBool_Type,&vsense)) {
        getGeomTrimmedSurfacePtr()->setHandle(new Geom_RectangularTrimmedSurface(
            Handle_Geom_Surface::DownCast(static_cast<GeometrySurfacePy*>(surf)->
                getGeomSurfacePtr()->handle()),
            u1, u2, v1, v2,
            PyObject_IsTrue(usense) ? Standard_True : Standard_False,
            PyObject_IsTrue(vsense) ? Standard_True : Standard_False
        ));
        return 0;
    }

    PyErr_Clear();
    double param1,param2;
    PyObject *utrim=Py_False, *sense=Py_True;
    if (PyArg_ParseTuple(args, "O!ddO!|O!",&(Part::GeometrySurfacePy::Type),&surf,
                         &param1,&param2,&PyBool_Type,&utrim,&PyBool_Type,&sense)) {
        Standard_Boolean UTrim = PyObject_IsTrue(utrim) ? Standard_True : Standard_False;
        Standard_Boolean Sense = PyObject_IsTrue(sense) ? Standard_True : Standard_False;
        getGeomTrimmedSurfacePtr()->setHandle(new Geom_RectangularTrimmedSurface(
            Handle_Geom_Surface::DownCast(static_cast<GeometrySurfacePy*>(surf)->
                getGeomSurfacePtr()->handle()),
            param1, param2, UTrim, Sense
        ));
        return 0;
    }

    PyErr_SetString(PyExc_Exception, "A surface and the trim parameters must be given");
    return -1;
}

PyObject* RectangularTrimmedSurfacePy::uIso(PyObject * args)
{
    double v;
    if (!PyArg_ParseTuple(args, "d", &v))
        return 0;

    try {
        Handle_Geom_Surface surf = Handle_Geom_Surface::DownCast
            (getGeometryPtr()->handle());
        Handle_Geom_Curve c = surf->UIso(v);
        if (c->IsKind(STANDARD_TYPE(Geom_TrimmedCurve))) {
            Handle_Geom_TrimmedCurve aCurve = Handle_Geom_TrimmedCurve::DownCast(c);
            return new GeometryCurvePy(new GeomTrimmedCurve(aCurve));
        }

        PyErr_Format(PyExc_NotImplementedError, "Iso curve is of type '%s'",
            c->DynamicType()->Name());
        return 0;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* RectangularTrimmedSurfacePy::vIso(PyObject * args)
{
    double v;
    if (!PyArg_ParseTuple(args, "d", &v))
        return 0;

    try {
        Handle_Geom_Surface surf = Handle_Geom_Surface::DownCast
            (getGeometryPtr()->handle());
        Handle_Geom_Curve c = surf->VIso(v);
        if (c->IsKind(STANDARD_TYPE(Geom_TrimmedCurve))) {
            Handle_Geom_TrimmedCurve aCurve = Handle_Geom_TrimmedCurve::DownCast(c);
            return new GeometryCurvePy(new GeomTrimmedCurve(aCurve));
        }

        PyErr_Format(PyExc_NotImplementedError, "Iso curve is of type '%s'",
            c->DynamicType()->Name());
        return 0;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject *RectangularTrimmedSurfacePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int RectangularTrimmedSurfacePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
