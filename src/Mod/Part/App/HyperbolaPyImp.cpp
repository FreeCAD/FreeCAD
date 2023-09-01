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
# include <GC_MakeHyperbola.hxx>
# include <Geom_Hyperbola.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>

#include "HyperbolaPy.h"
#include "HyperbolaPy.cpp"
#include "OCCError.h"


using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string HyperbolaPy::representation() const
{
    return "<Hyperbola object>";
}

PyObject *HyperbolaPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of HyperbolaPy and the Twin object
    return new HyperbolaPy(new GeomHyperbola);
}

// constructor method
int HyperbolaPy::PyInit(PyObject* args, PyObject* kwds)
{
    static const std::array<const char *, 1> keywords_n {nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "", keywords_n)) {
        Handle(Geom_Hyperbola) hyperbola = Handle(Geom_Hyperbola)::DownCast(getGeomHyperbolaPtr()->handle());
        hyperbola->SetMajorRadius(2.0);
        hyperbola->SetMinorRadius(1.0);
        return 0;
    }

    static const std::array<const char *, 2> keywords_e {"Hyperbola", nullptr};
    PyErr_Clear();
    PyObject *pHypr;
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!",keywords_e, &(HyperbolaPy::Type), &pHypr)) {
        HyperbolaPy* pHyperbola = static_cast<HyperbolaPy*>(pHypr);
        Handle(Geom_Hyperbola) Hypr1 = Handle(Geom_Hyperbola)::DownCast
            (pHyperbola->getGeomHyperbolaPtr()->handle());
        Handle(Geom_Hyperbola) Hypr2 = Handle(Geom_Hyperbola)::DownCast
            (this->getGeomHyperbolaPtr()->handle());
        Hypr2->SetHypr(Hypr1->Hypr());
        return 0;
    }

    static const std::array<const char *, 4> keywords_ssc{"S1", "S2", "Center", nullptr};
    PyErr_Clear();
    PyObject *pV1, *pV2, *pV3;
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!O!O!", keywords_ssc,
                                            &(Base::VectorPy::Type), &pV1,
                                             &(Base::VectorPy::Type), &pV2,
                                            &(Base::VectorPy::Type), &pV3)) {
        Base::Vector3d v1 = static_cast<Base::VectorPy*>(pV1)->value();
        Base::Vector3d v2 = static_cast<Base::VectorPy*>(pV2)->value();
        Base::Vector3d v3 = static_cast<Base::VectorPy*>(pV3)->value();
        GC_MakeHyperbola me(gp_Pnt(v1.x,v1.y,v1.z),
                          gp_Pnt(v2.x,v2.y,v2.z),
                          gp_Pnt(v3.x,v3.y,v3.z));
        if (!me.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(me.Status()));
            return -1;
        }

        Handle(Geom_Hyperbola) hyperbola = Handle(Geom_Hyperbola)::DownCast(getGeomHyperbolaPtr()->handle());
        hyperbola->SetHypr(me.Value()->Hypr());
        return 0;
    }

    static const std::array<const char *, 4> keywords_cmm {"Center","MajorRadius","MinorRadius",nullptr};
    PyErr_Clear();
    PyObject *pV;
    double major, minor;
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!dd", keywords_cmm,
                                        &(Base::VectorPy::Type), &pV,
                                        &major, &minor)) {
        Base::Vector3d c = static_cast<Base::VectorPy*>(pV)->value();
        GC_MakeHyperbola me(gp_Ax2(gp_Pnt(c.x,c.y,c.z), gp_Dir(0.0,0.0,1.0)),
                          major, minor);
        if (!me.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(me.Status()));
            return -1;
        }

        Handle(Geom_Hyperbola) hyperbola = Handle(Geom_Hyperbola)::DownCast(getGeomHyperbolaPtr()->handle());
        hyperbola->SetHypr(me.Value()->Hypr());
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Hyperbola constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Hyperbola\n"
        "-- Point, double, double\n"
        "-- Point, Point, Point");
    return -1;
}

Py::Float HyperbolaPy::getMajorRadius() const
{
    Handle(Geom_Hyperbola) hyperbola = Handle(Geom_Hyperbola)::DownCast(getGeomHyperbolaPtr()->handle());
    return Py::Float(hyperbola->MajorRadius());
}

void HyperbolaPy::setMajorRadius(Py::Float arg)
{
    Handle(Geom_Hyperbola) hyperbola = Handle(Geom_Hyperbola)::DownCast(getGeomHyperbolaPtr()->handle());
    hyperbola->SetMajorRadius((double)arg);
}

Py::Float HyperbolaPy::getMinorRadius() const
{
    Handle(Geom_Hyperbola) hyperbola = Handle(Geom_Hyperbola)::DownCast(getGeomHyperbolaPtr()->handle());
    return Py::Float(hyperbola->MinorRadius());
}

void HyperbolaPy::setMinorRadius(Py::Float arg)
{
    Handle(Geom_Hyperbola) hyperbola = Handle(Geom_Hyperbola)::DownCast(getGeomHyperbolaPtr()->handle());
    hyperbola->SetMinorRadius((double)arg);
}

Py::Float HyperbolaPy::getFocal() const
{
    Handle(Geom_Hyperbola) hyperbola = Handle(Geom_Hyperbola)::DownCast(getGeomHyperbolaPtr()->handle());
    return Py::Float(hyperbola->Focal());
}

Py::Object HyperbolaPy::getFocus1() const
{
    Handle(Geom_Hyperbola) hyperbola = Handle(Geom_Hyperbola)::DownCast(getGeomHyperbolaPtr()->handle());
    gp_Pnt loc = hyperbola->Focus1();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

Py::Object HyperbolaPy::getFocus2() const
{
    Handle(Geom_Hyperbola) hyperbola = Handle(Geom_Hyperbola)::DownCast(getGeomHyperbolaPtr()->handle());
    gp_Pnt loc = hyperbola->Focus2();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

PyObject *HyperbolaPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int HyperbolaPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
