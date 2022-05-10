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
#include "OCCError.h"

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
            Handle(Geom_Surface)::DownCast(static_cast<GeometrySurfacePy*>(surf)->
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
            Handle(Geom_Surface)::DownCast(static_cast<GeometrySurfacePy*>(surf)->
                getGeomSurfacePtr()->handle()),
            param1, param2, UTrim, Sense
        ));
        return 0;
    }

    PyErr_SetString(PartExceptionOCCError, "A surface and the trim parameters must be given");
    return -1;
}

PyObject* RectangularTrimmedSurfacePy::setTrim(PyObject *args)
{
    double u1, u2, v1, v2;
    if (!PyArg_ParseTuple(args, "dddd", &u1, &u2, &v1, &v2))
        return nullptr;

    try {
        Handle(Geom_RectangularTrimmedSurface) surf = Handle(Geom_RectangularTrimmedSurface)::DownCast
            (getGeometryPtr()->handle());
        if (surf.IsNull()) {
            PyErr_SetString(PyExc_TypeError, "geometry is not a surface");
            return nullptr;
        }

        surf->SetTrim(u1, u2, v1, v2);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

Py::Object RectangularTrimmedSurfacePy::getBasisSurface() const
{
    Handle(Geom_RectangularTrimmedSurface) surf = Handle(Geom_RectangularTrimmedSurface)::DownCast
        (getGeometryPtr()->handle());
    if (surf.IsNull()) {
        throw Py::TypeError("geometry is not a surface");
    }

    std::unique_ptr<GeomSurface> geo(makeFromSurface(surf->BasisSurface()));
    return Py::asObject(geo->getPyObject());
}

PyObject *RectangularTrimmedSurfacePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int RectangularTrimmedSurfacePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
