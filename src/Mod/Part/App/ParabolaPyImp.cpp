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
# include <Geom_Parabola.hxx>
# include <gce_MakeParab.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>

#include "ParabolaPy.h"
#include "ParabolaPy.cpp"
#include "OCCError.h"


using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string ParabolaPy::representation() const
{
    return "<Parabola object>";
}

PyObject *ParabolaPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ParabolaPy and the Twin object
    return new ParabolaPy(new GeomParabola);
}

// constructor method
int ParabolaPy::PyInit(PyObject* args, PyObject* kwds)
{
    static const std::array<const char *, 1> keywords_n {nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "", keywords_n)) {
        Handle(Geom_Parabola) parabola = Handle(Geom_Parabola)::DownCast(getGeomParabolaPtr()->handle());
        parabola->SetFocal(1.0);
        return 0;
    }

    static const std::array<const char *, 2> keywords_e {"Parabola", nullptr};
    PyErr_Clear();
    PyObject *pParab;
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!",keywords_e, &(ParabolaPy::Type), &pParab)) {
        ParabolaPy* pParabola = static_cast<ParabolaPy*>(pParab);
        Handle(Geom_Parabola) Parab1 = Handle(Geom_Parabola)::DownCast
            (pParabola->getGeomParabolaPtr()->handle());
        Handle(Geom_Parabola) Parab2 = Handle(Geom_Parabola)::DownCast
            (this->getGeomParabolaPtr()->handle());
        Parab2->SetParab(Parab1->Parab());
        return 0;
    }

    static const std::array<const char *, 4> keywords_ssc {"Focus","Center","Normal",nullptr};
    PyErr_Clear();
    PyObject *pV1, *pV2, *pV3;
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!O!O!", keywords_ssc,
                                            &(Base::VectorPy::Type), &pV1,
                                            &(Base::VectorPy::Type), &pV2,
                                            &(Base::VectorPy::Type), &pV3)) {
        Base::Vector3d focus = static_cast<Base::VectorPy*>(pV1)->value();
        Base::Vector3d center = static_cast<Base::VectorPy*>(pV2)->value();
        Base::Vector3d normal = static_cast<Base::VectorPy*>(pV3)->value();

        Base::Vector3d xvect = focus-center;

        // set the geometry
        gp_Pnt p1(center.x,center.y,center.z);
        gp_Dir norm(normal.x,normal.y,normal.z);
        gp_Dir xdiroce(xvect.x,xvect.y,xvect.z);

        gp_Ax2 xdir(p1, norm, xdiroce);

        gce_MakeParab mc(xdir, (Standard_Real) xvect.Length());
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle(Geom_Parabola) parabola = Handle(Geom_Parabola)::DownCast(getGeomParabolaPtr()->handle());
        parabola->SetParab(mc.Value());
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Parabola constructor accepts:\n"
    "-- empty parameter list\n"
    "-- Parabola\n"
    "-- Point, Point, Point");

    return -1;
}

PyObject* ParabolaPy::compute(PyObject *args)
{
    PyObject *p1, *p2, *p3;
    if (!PyArg_ParseTuple(args, "O!O!O!",
        &Base::VectorPy::Type,&p1,
        &Base::VectorPy::Type,&p2,
        &Base::VectorPy::Type,&p3))
        return nullptr;
    Base::Vector3d v1 = Py::Vector(p1,false).toVector();
    Base::Vector3d v2 = Py::Vector(p2,false).toVector();
    Base::Vector3d v3 = Py::Vector(p3,false).toVector();
    Base::Vector3d c = (v1-v2) % (v3-v2);
    double zValue = v1.z;
    if (fabs(c.Length()) < 0.0001) {
        PyErr_SetString(PartExceptionOCCError, "Points are collinear");
        return nullptr;
    }

    Base::Matrix4D m;
    Base::Vector3d v;
    m[0][0] = v1.y * v1.y;
    m[0][1] = v1.y;
    m[0][2] = 1;
    m[1][0] = v2.y * v2.y;
    m[1][1] = v2.y;
    m[1][2] = 1;
    m[2][0] = v3.y * v3.y;
    m[2][1] = v3.y;
    m[2][2] = 1.0;
    v.x = v1.x;
    v.y = v2.x;
    v.z = v3.x;
    m.inverseGauss();
    v = m * v;
    double a22 = v.x;
    double a10 = -0.5;
    double a20 = v.y/2.0;
    double a00 = v.z;
    Handle(Geom_Parabola) curve = Handle(Geom_Parabola)::DownCast(getGeometryPtr()->handle());
    curve->SetFocal(0.5*fabs(a10/a22));
    curve->SetLocation(gp_Pnt((a20*a20-a22*a00)/(2*a22*a10), -a20/a22, zValue));

    Py_Return;
}

Py::Float ParabolaPy::getFocal() const
{
    Handle(Geom_Parabola) curve = Handle(Geom_Parabola)::DownCast(getGeometryPtr()->handle());
    return Py::Float(curve->Focal());
}

void ParabolaPy::setFocal(Py::Float arg)
{
    Handle(Geom_Parabola) curve = Handle(Geom_Parabola)::DownCast(getGeometryPtr()->handle());
    curve->SetFocal((double)arg);
}

Py::Object ParabolaPy::getFocus() const
{
    Handle(Geom_Parabola) c = Handle(Geom_Parabola)::DownCast
        (getGeometryPtr()->handle());
    gp_Pnt loc = c->Focus();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

Py::Float ParabolaPy::getParameter() const
{
    Handle(Geom_Parabola) curve = Handle(Geom_Parabola)::DownCast(getGeometryPtr()->handle());
    return Py::Float(curve->Parameter());
}

PyObject *ParabolaPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ParabolaPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


