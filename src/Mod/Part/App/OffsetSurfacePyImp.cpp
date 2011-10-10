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
# include <Geom_OffsetSurface.hxx>
#endif

#include <Base/VectorPy.h>
#include <Base/Vector3D.h>

#include "Geometry.h"
#include "OffsetSurfacePy.h"
#include "OffsetSurfacePy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string OffsetSurfacePy::representation(void) const
{
    return "<OffsetSurface object>";
}

PyObject *OffsetSurfacePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of OffsetSurfacePy and the Twin object 
    return new OffsetSurfacePy(new GeomOffsetSurface);
}

// constructor method
int OffsetSurfacePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* pGeom;
    double offset;
    if (!PyArg_ParseTuple(args, "O!d", 
                            &(GeometryPy::Type), &pGeom, 
                            &offset))
        return -1;

    GeometryPy* pcGeo = static_cast<GeometryPy*>(pGeom);
    Handle_Geom_Surface surf = Handle_Geom_Surface::DownCast
        (pcGeo->getGeometryPtr()->handle());
    if (surf.IsNull()) {
        PyErr_SetString(PyExc_TypeError, "geometry is not a surface");
        return -1;
    }

    try {
        Handle_Geom_OffsetSurface surf2 = new Geom_OffsetSurface(surf, offset);
        getGeomOffsetSurfacePtr()->setHandle(surf2);
        return 0;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return -1;
    }
}

PyObject* OffsetSurfacePy::uIso(PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

PyObject* OffsetSurfacePy::vIso(PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

Py::Float OffsetSurfacePy::getOffsetValue(void) const
{
    Handle_Geom_OffsetSurface surf = Handle_Geom_OffsetSurface::DownCast(getGeometryPtr()->handle());
    return Py::Float(surf->Offset());
}

void  OffsetSurfacePy::setOffsetValue(Py::Float arg)
{
    Handle_Geom_OffsetSurface surf = Handle_Geom_OffsetSurface::DownCast(getGeometryPtr()->handle());
    surf->SetOffsetValue((double)arg);
}

Py::Object OffsetSurfacePy::getBasisSurface(void) const
{
    throw Py::Exception(PyExc_NotImplementedError, "Not yet implemented");
}

void  OffsetSurfacePy::setBasisSurface(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(GeometryPy::Type))) {
        GeometryPy* pcGeo = static_cast<GeometryPy*>(p);
        Handle_Geom_Surface surf = Handle_Geom_Surface::DownCast
            (pcGeo->getGeometryPtr()->handle());
        if (surf.IsNull()) {
            throw Py::TypeError("geometry is not a surface");
        }

        try {
            Handle_Geom_OffsetSurface surf2 = Handle_Geom_OffsetSurface::DownCast
                (getGeometryPtr()->handle());
            surf2->SetBasisSurface(surf);
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            throw Py::Exception(e->GetMessageString());
        }
    }
}

PyObject *OffsetSurfacePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int OffsetSurfacePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
