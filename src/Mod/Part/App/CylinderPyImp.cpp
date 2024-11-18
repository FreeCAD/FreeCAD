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
# include <GC_MakeCylindricalSurface.hxx>
# include <Geom_Circle.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <gp_Cylinder.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>

#include "CylinderPy.h"
#include "CylinderPy.cpp"
#include "CirclePy.h"
#include "OCCError.h"


using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string CylinderPy::representation() const
{
    return "<Cylinder object>";
}

PyObject *CylinderPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of CylinderPy and the Twin object
    return new CylinderPy(new GeomCylinder);
}

// constructor method
int CylinderPy::PyInit(PyObject* args, PyObject* kwds)
{
    // cylinder and distance for offset
    PyObject *pCyl;
    double dist;
    static const std::array<const char *, 3> keywords_cd{"Cylinder", "Distance", nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!d", keywords_cd, &(CylinderPy::Type), &pCyl, &dist)) {
        CylinderPy* pcCylinder = static_cast<CylinderPy*>(pCyl);
        Handle(Geom_CylindricalSurface) cylinder = Handle(Geom_CylindricalSurface)::DownCast
            (pcCylinder->getGeomCylinderPtr()->handle());
        GC_MakeCylindricalSurface mc(cylinder->Cylinder(), dist);
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle(Geom_CylindricalSurface) cyl = Handle(Geom_CylindricalSurface)::DownCast
            (getGeomCylinderPtr()->handle());
        cyl->SetCylinder(mc.Value()->Cylinder());
        return 0;
    }

    static const std::array<const char *, 2> keywords_c {"Cylinder", nullptr};
    PyErr_Clear();
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!", keywords_c, &(CylinderPy::Type), &pCyl)) {
        CylinderPy* pcCylinder = static_cast<CylinderPy*>(pCyl);
        Handle(Geom_CylindricalSurface) cyl1 = Handle(Geom_CylindricalSurface)::DownCast
            (pcCylinder->getGeomCylinderPtr()->handle());
        Handle(Geom_CylindricalSurface) cyl2 = Handle(Geom_CylindricalSurface)::DownCast
            (this->getGeomCylinderPtr()->handle());
        cyl2->SetCylinder(cyl1->Cylinder());
        return 0;
    }

    PyObject *pV1, *pV2, *pV3;
    static const std::array<const char *, 4> keywords_ppp {"Point1", "Point2", "Point3", nullptr};
    PyErr_Clear();
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!O!O!", keywords_ppp,
                                            &(Base::VectorPy::Type), &pV1,
                                            &(Base::VectorPy::Type), &pV2,
                                            &(Base::VectorPy::Type), &pV3)) {
        Base::Vector3d v1 = static_cast<Base::VectorPy*>(pV1)->value();
        Base::Vector3d v2 = static_cast<Base::VectorPy*>(pV2)->value();
        Base::Vector3d v3 = static_cast<Base::VectorPy*>(pV3)->value();
        GC_MakeCylindricalSurface mc(gp_Pnt(v1.x,v1.y,v1.z),
                                     gp_Pnt(v2.x,v2.y,v2.z),
                                     gp_Pnt(v3.x,v3.y,v3.z));
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle(Geom_CylindricalSurface) cyl = Handle(Geom_CylindricalSurface)::DownCast
            (getGeomCylinderPtr()->handle());
        cyl->SetCylinder(mc.Value()->Cylinder());
        return 0;
    }

    static const std::array<const char *, 2> keywords_cc {"Circle", nullptr};
    PyErr_Clear();
    PyObject *pCirc;
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!", keywords_cc, &(CirclePy::Type), &pCirc)) {
        CirclePy* pcCircle = static_cast<CirclePy*>(pCirc);
        Handle(Geom_Circle) circ = Handle(Geom_Circle)::DownCast
            (pcCircle->getGeomCirclePtr()->handle());
        GC_MakeCylindricalSurface mc(circ->Circ());
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle(Geom_CylindricalSurface) cyl = Handle(Geom_CylindricalSurface)::DownCast
            (getGeomCylinderPtr()->handle());
        cyl->SetCylinder(mc.Value()->Cylinder());
        return 0;
    }

    static const std::array<const char *, 1> keywords_n {nullptr};
    PyErr_Clear();
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "", keywords_n)) {
        Handle(Geom_CylindricalSurface) cyl = Handle(Geom_CylindricalSurface)::DownCast
            (getGeomCylinderPtr()->handle());
        cyl->SetRadius(1.0);
        return 0;
    }

    // All checks failed
    PyErr_SetString(PyExc_TypeError, "Cylinder constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Cylinder\n"
        "-- Cylinder, Distance\n"
        "-- Point1, Point2, Point3\n"
        "-- Circle");
    return -1;
}

Py::Float CylinderPy::getRadius() const
{
    Handle(Geom_CylindricalSurface) cyl = Handle(Geom_CylindricalSurface)::DownCast
        (getGeomCylinderPtr()->handle());
    return Py::Float(cyl->Radius());
}

void CylinderPy::setRadius(Py::Float arg)
{
    Handle(Geom_CylindricalSurface) cyl = Handle(Geom_CylindricalSurface)::DownCast
        (getGeomCylinderPtr()->handle());
    cyl->SetRadius((double)arg);
}

Py::Object CylinderPy::getCenter() const
{
    Handle(Geom_CylindricalSurface) cyl = Handle(Geom_CylindricalSurface)::DownCast
        (getGeomCylinderPtr()->handle());
    gp_Pnt loc = cyl->Location();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

void CylinderPy::setCenter(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d loc = static_cast<Base::VectorPy*>(p)->value();
        Handle(Geom_CylindricalSurface) cyl = Handle(Geom_CylindricalSurface)::DownCast
            (getGeomCylinderPtr()->handle());
        cyl->SetLocation(gp_Pnt(loc.x, loc.y, loc.z));
    }
    else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        Base::Vector3d loc = Base::getVectorFromTuple<double>(p);
        Handle(Geom_CylindricalSurface) cyl = Handle(Geom_CylindricalSurface)::DownCast
            (getGeomCylinderPtr()->handle());
        cyl->SetLocation(gp_Pnt(loc.x, loc.y, loc.z));
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object CylinderPy::getAxis() const
{
    Handle(Geom_ElementarySurface) s = Handle(Geom_ElementarySurface)::DownCast
        (getGeometryPtr()->handle());
    gp_Dir dir = s->Axis().Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void CylinderPy::setAxis(Py::Object arg)
{
    Standard_Real dir_x, dir_y, dir_z;
    PyObject *p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d v = static_cast<Base::VectorPy*>(p)->value();
        dir_x = v.x;
        dir_y = v.y;
        dir_z = v.z;
    }
    else if (PyTuple_Check(p)) {
        Py::Tuple tuple(arg);
        dir_x = (double)Py::Float(tuple.getItem(0));
        dir_y = (double)Py::Float(tuple.getItem(1));
        dir_z = (double)Py::Float(tuple.getItem(2));
    }
    else {
        std::string error = std::string("type must be 'Vector' or tuple, not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    try {
        Handle(Geom_ElementarySurface) this_surf = Handle(Geom_ElementarySurface)::DownCast
            (this->getGeometryPtr()->handle());
        gp_Ax1 axis;
        axis.SetLocation(this_surf->Location());
        axis.SetDirection(gp_Dir(dir_x, dir_y, dir_z));
        this_surf->SetAxis(axis);
    }
    catch (Standard_Failure&) {
        throw Py::RuntimeError("cannot set axis");
    }
}

PyObject *CylinderPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int CylinderPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


