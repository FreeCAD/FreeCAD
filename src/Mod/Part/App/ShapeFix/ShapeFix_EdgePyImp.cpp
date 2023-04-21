/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <TopoDS.hxx>
#endif

#include <Base/PlacementPy.h>

#include "ShapeFix/ShapeFix_EdgePy.h"
#include "ShapeFix/ShapeFix_EdgePy.cpp"
#include "GeometrySurfacePy.h"
#include "Tools.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeFacePy.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string ShapeFix_EdgePy::representation() const
{
    return "<ShapeFix_Edge object>";
}

PyObject *ShapeFix_EdgePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ShapeFix_EdgePy
    return new ShapeFix_EdgePy(nullptr);
}

// constructor method
int ShapeFix_EdgePy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    if (!PyArg_ParseTuple(args, ""))
        return -1;

    setHandle(new ShapeFix_Edge);
    return 0;
}

PyObject* ShapeFix_EdgePy::fixRemovePCurve(PyObject *args)
{
    PyObject* edge;
    PyObject* face;
    if (PyArg_ParseTuple(args, "O!O!", &TopoShapeEdgePy::Type, &edge,
                                       &TopoShapeFacePy::Type, &face)) {
        TopoDS_Shape e = static_cast<TopoShapePy*>(edge)->getTopoShapePtr()->getShape();
        TopoDS_Shape f = static_cast<TopoShapePy*>(face)->getTopoShapePtr()->getShape();

        bool ok = getShapeFix_EdgePtr()->FixRemovePCurve(TopoDS::Edge(e), TopoDS::Face(f));
        return Py::new_reference_to(Py::Boolean(ok));
    }

    PyErr_Clear();
    PyObject* plm;
    if (PyArg_ParseTuple(args, "O!O!O!", &TopoShapeEdgePy::Type, &edge,
                                         &GeometrySurfacePy::Type, &face,
                                         &Base::PlacementPy::Type, &plm)) {
        TopoDS_Shape e = static_cast<TopoShapePy*>(edge)->getTopoShapePtr()->getShape();
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast(static_cast<GeometrySurfacePy*>(face)->getGeometryPtr()->handle());
        Base::Placement* pm = static_cast<Base::PlacementPy*>(plm)->getPlacementPtr();
        TopLoc_Location loc = Tools::fromPlacement(*pm);

        bool ok = getShapeFix_EdgePtr()->FixRemovePCurve(TopoDS::Edge(e), surf, loc);
        return Py::new_reference_to(Py::Boolean(ok));
    }

    PyErr_SetString(PyExc_TypeError, "Arguments must be:\n"
                    "-- fixRemovePCurve(edge, face) or\n"
                    "-- fixRemovePCurve(edge, surface, placement)");
    return nullptr;
}

PyObject* ShapeFix_EdgePy::fixRemoveCurve3d(PyObject *args)
{
    PyObject* edge;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapeEdgePy::Type, &edge))
        return nullptr;

    TopoDS_Shape e = static_cast<TopoShapePy*>(edge)->getTopoShapePtr()->getShape();
    bool ok = getShapeFix_EdgePtr()->FixRemoveCurve3d(TopoDS::Edge(e));
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_EdgePy::fixAddPCurve(PyObject *args)
{
    PyObject* edge;
    PyObject* face;
    PyObject* seam;
    double prec = 0.0;
    if (PyArg_ParseTuple(args, "O!O!O!|d", &TopoShapeEdgePy::Type, &edge,
                                           &TopoShapeFacePy::Type, &face,
                                           &PyBool_Type, &seam, &prec)) {
        TopoDS_Shape e = static_cast<TopoShapePy*>(edge)->getTopoShapePtr()->getShape();
        TopoDS_Shape f = static_cast<TopoShapePy*>(face)->getTopoShapePtr()->getShape();

        bool ok = getShapeFix_EdgePtr()->FixAddPCurve(TopoDS::Edge(e), TopoDS::Face(f),
                                                      Base::asBoolean(seam),
                                                      prec);
        return Py::new_reference_to(Py::Boolean(ok));
    }

    PyErr_Clear();
    PyObject* plm;
    if (PyArg_ParseTuple(args, "O!O!O!O!|d", &TopoShapeEdgePy::Type, &edge,
                                             &GeometrySurfacePy::Type, &face,
                                             &Base::PlacementPy::Type, &plm,
                                             &PyBool_Type, &seam, &prec)) {
        TopoDS_Shape e = static_cast<TopoShapePy*>(edge)->getTopoShapePtr()->getShape();
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast(static_cast<GeometrySurfacePy*>(face)->getGeometryPtr()->handle());
        Base::Placement* pm = static_cast<Base::PlacementPy*>(plm)->getPlacementPtr();
        TopLoc_Location loc = Tools::fromPlacement(*pm);

        bool ok = getShapeFix_EdgePtr()->FixAddPCurve(TopoDS::Edge(e), surf, loc,
                                                      Base::asBoolean(seam),
                                                      prec);
        return Py::new_reference_to(Py::Boolean(ok));
    }

    PyErr_SetString(PyExc_TypeError, "Arguments must be:\n"
                    "-- fixAddPCurve(edge, face, isSeam, prec) or\n"
                    "-- fixAddPCurve(edge, surface, placement, isSeam, prec)");
    return nullptr;
}

PyObject* ShapeFix_EdgePy::fixAddCurve3d(PyObject *args)
{
    PyObject* edge;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapeEdgePy::Type, &edge))
        return nullptr;

    TopoDS_Shape e = static_cast<TopoShapePy*>(edge)->getTopoShapePtr()->getShape();
    bool ok = getShapeFix_EdgePtr()->FixAddCurve3d(TopoDS::Edge(e));
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_EdgePy::fixVertexTolerance(PyObject *args)
{
    PyObject* edge;
    PyObject* face = nullptr;
    if (!PyArg_ParseTuple(args, "O!|O!", &TopoShapeEdgePy::Type, &edge,
                                         &TopoShapeFacePy::Type, &face))
        return nullptr;

    TopoDS_Shape e = static_cast<TopoShapePy*>(edge)->getTopoShapePtr()->getShape();

    if (face) {
        TopoDS_Shape f = static_cast<TopoShapePy*>(face)->getTopoShapePtr()->getShape();
        bool ok = getShapeFix_EdgePtr()->FixVertexTolerance(TopoDS::Edge(e), TopoDS::Face(f));
        return Py::new_reference_to(Py::Boolean(ok));
    }
    else {
        bool ok = getShapeFix_EdgePtr()->FixVertexTolerance(TopoDS::Edge(e));
        return Py::new_reference_to(Py::Boolean(ok));
    }
}

PyObject* ShapeFix_EdgePy::fixReversed2d(PyObject *args)
{
    PyObject* edge;
    PyObject* face;
    if (PyArg_ParseTuple(args, "O!O!", &TopoShapeEdgePy::Type, &edge,
                                       &TopoShapeFacePy::Type, &face)) {
        TopoDS_Shape e = static_cast<TopoShapePy*>(edge)->getTopoShapePtr()->getShape();
        TopoDS_Shape f = static_cast<TopoShapePy*>(face)->getTopoShapePtr()->getShape();

        bool ok = getShapeFix_EdgePtr()->FixReversed2d(TopoDS::Edge(e), TopoDS::Face(f));
        return Py::new_reference_to(Py::Boolean(ok));
    }

    PyErr_Clear();
    PyObject* plm;
    if (PyArg_ParseTuple(args, "O!O!O!", &TopoShapeEdgePy::Type, &edge,
                                         &GeometrySurfacePy::Type, &face,
                                         &Base::PlacementPy::Type, &plm)) {
        TopoDS_Shape e = static_cast<TopoShapePy*>(edge)->getTopoShapePtr()->getShape();
        Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast(static_cast<GeometrySurfacePy*>(face)->getGeometryPtr()->handle());
        Base::Placement* pm = static_cast<Base::PlacementPy*>(plm)->getPlacementPtr();
        TopLoc_Location loc = Tools::fromPlacement(*pm);

        bool ok = getShapeFix_EdgePtr()->FixReversed2d(TopoDS::Edge(e), surf, loc);
        return Py::new_reference_to(Py::Boolean(ok));
    }

    PyErr_SetString(PyExc_TypeError, "Arguments must be:\n"
                    "-- FixReversed2d(edge, face) or\n"
                    "-- FixReversed2d(edge, surface, placement)");
    return nullptr;
}

PyObject* ShapeFix_EdgePy::fixSameParameter(PyObject *args)
{
    PyObject* edge;
    double tolerance = 0.0;
    if (PyArg_ParseTuple(args, "O!|d", &TopoShapeEdgePy::Type, &edge, &tolerance)) {
        TopoDS_Shape e = static_cast<TopoShapePy*>(edge)->getTopoShapePtr()->getShape();

        bool ok = getShapeFix_EdgePtr()->FixSameParameter(TopoDS::Edge(e), tolerance);
        return Py::new_reference_to(Py::Boolean(ok));
    }

    PyErr_Clear();
    PyObject* face;
    if (PyArg_ParseTuple(args, "O!O!|d", &TopoShapeEdgePy::Type, &edge,
                                         &TopoShapeFacePy::Type, &face,
                                         &tolerance)) {
        TopoDS_Shape e = static_cast<TopoShapePy*>(edge)->getTopoShapePtr()->getShape();
        TopoDS_Shape f = static_cast<TopoShapePy*>(face)->getTopoShapePtr()->getShape();

        bool ok = getShapeFix_EdgePtr()->FixSameParameter(TopoDS::Edge(e), TopoDS::Face(f), tolerance);
        return Py::new_reference_to(Py::Boolean(ok));
    }

    PyErr_SetString(PyExc_TypeError, "Arguments must be:\n"
                    "-- fixSameParameter(edge, tolerance) or\n"
                    "-- fixSameParameter(edge, face, tolerance)");
    return nullptr;
}

PyObject *ShapeFix_EdgePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeFix_EdgePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
