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

#include "Geometry.h"
#include <Base/GeometryPyCXX.h>

#include <Mod/Part/App/BSplineCurvePy.h>
#include <Mod/Part/App/HermiteCurvePy.h>
#include <Mod/Part/App/HermiteCurvePy.cpp>

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string HermiteCurvePy::representation(void) const
{
    return std::string("<GeomHermiteCurve object>");
}

PyObject *HermiteCurvePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of HermiteCurvePy and the Twin object 
    return new HermiteCurvePy(new GeomHermiteCurve);
}

// constructor method
int HermiteCurvePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* HermiteCurvePy::interpolate(PyObject *args, PyObject *kwds)
{
    PyObject* pts;
    PyObject* tgs;

    static char* kwds_interp1[] = {"Points", "Tangents", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "OO",kwds_interp1, &pts, &tgs)) {
        Py::Sequence list(pts);
        std::vector<gp_Pnt> interpPoints;
        interpPoints.reserve(list.size());
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector v(*it);
            Base::Vector3d pnt = v.toVector();
            interpPoints.push_back(gp_Pnt(pnt.x,pnt.y,pnt.z));
        }
        Py::Sequence list2(tgs);
        std::vector<gp_Vec> interpTangents;
        interpTangents.reserve(list2.size());
        for (Py::Sequence::iterator it = list2.begin(); it != list2.end(); ++it) {
            Py::Vector v(*it);
            Base::Vector3d vec = v.toVector();
            interpTangents.push_back(gp_Vec(vec.x,vec.y,vec.z));
        }

        GeomHermiteCurve* hermite = this->getGeomHermiteCurvePtr();
        hermite->interpolate(interpPoints, interpTangents);
        Handle_Geom_BSplineCurve aBSpline = Handle_Geom_BSplineCurve::DownCast
            (hermite->handle());
        return new BSplineCurvePy(new GeomBSplineCurve(aBSpline));
    }

    return 0;
}

PyObject* HermiteCurvePy::getCardinalSplineTangents(PyObject *args, PyObject *kwds)
{
    PyObject* pts;
    PyObject* tgs;
    double parameter;

    static char* kwds_interp1[] = {"Points", "Parameter", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "Od",kwds_interp1, &pts, &parameter)) {
        Py::Sequence list(pts);
        std::vector<gp_Pnt> interpPoints;
        interpPoints.reserve(list.size());
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector v(*it);
            Base::Vector3d pnt = v.toVector();
            interpPoints.push_back(gp_Pnt(pnt.x,pnt.y,pnt.z));
        }

        GeomHermiteCurve* hermite = this->getGeomHermiteCurvePtr();
        std::vector<gp_Vec> tangents;
        hermite->getCardinalSplineTangents(interpPoints, parameter, tangents);

        Py::List vec;
        for (gp_Vec it : tangents)
            vec.append(Py::Vector(Base::Vector3d(it.X(), it.Y(), it.Z())));
        return Py::new_reference_to(vec);
    }

    PyErr_Clear();
    static char* kwds_interp2[] = {"Points", "Parameters", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "OO",kwds_interp2, &pts, &tgs)) {
        Py::Sequence list(pts);
        std::vector<gp_Pnt> interpPoints;
        interpPoints.reserve(list.size());
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector v(*it);
            Base::Vector3d pnt = v.toVector();
            interpPoints.push_back(gp_Pnt(pnt.x,pnt.y,pnt.z));
        }

        Py::Sequence list2(tgs);
        std::vector<double> parameters;
        parameters.reserve(list2.size());
        for (Py::Sequence::iterator it = list2.begin(); it != list2.end(); ++it) {
            Py::Float p(*it);
            parameters.push_back(static_cast<double>(p));
        }

        GeomHermiteCurve* hermite = this->getGeomHermiteCurvePtr();
        std::vector<gp_Vec> tangents;
        hermite->getCardinalSplineTangents(interpPoints, parameters, tangents);

        Py::List vec;
        for (gp_Vec it : tangents)
            vec.append(Py::Vector(Base::Vector3d(it.X(), it.Y(), it.Z())));
        return Py::new_reference_to(vec);
    }

    return 0;
}

PyObject *HermiteCurvePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int HermiteCurvePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
