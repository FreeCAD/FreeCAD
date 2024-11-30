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

#include "ShapeFix/ShapeFix_FacePy.h"
#include "ShapeFix/ShapeFix_FacePy.cpp"
#include "ShapeFix/ShapeFix_WirePy.h"
#include <Mod/Part/App/GeometrySurfacePy.h>
#include <Mod/Part/App/TopoShapeFacePy.h>
#include <Mod/Part/App/TopoShapeWirePy.h>


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string ShapeFix_FacePy::representation() const
{
    return "<ShapeFix_Face object>";
}

PyObject *ShapeFix_FacePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ShapeFix_FacePy
    return new ShapeFix_FacePy(nullptr);
}

// constructor method
int ShapeFix_FacePy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* face = nullptr;
    if (PyArg_ParseTuple(args, "|O!", &TopoShapeFacePy::Type, &face)) {
        setHandle(new ShapeFix_Face());
        if (face) {
            getShapeFix_FacePtr()->Init(TopoDS::Face(static_cast<TopoShapePy*>(face)->getTopoShapePtr()->getShape()));
        }

        return 0;
    }

    PyErr_Clear();
    double prec;
    PyObject* fwd = Py_True;
    if (PyArg_ParseTuple(args, "O!d|O!", &GeometrySurfacePy::Type, &face, &prec, &PyBool_Type, &fwd)) {
        setHandle(new ShapeFix_Face());
        if (face) {
            Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast(static_cast<GeometrySurfacePy*>(face)->getGeomSurfacePtr()->handle());
            getShapeFix_FacePtr()->Init(surf, prec, Base::asBoolean(fwd));
        }

        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Supported arguments are:\n"
        "-- Face\n"
        "-- Surface, Precision, [Forward=True}\n"
        "   Precision is a Float\n"
        "   If Forward is the orientation will be FORWARD or REVERSED otherwise"
    );
    return -1;
}

PyObject* ShapeFix_FacePy::init(PyObject *args)
{
    PyObject* face;
    if (PyArg_ParseTuple(args, "O!", &TopoShapeFacePy::Type, &face)) {
        getShapeFix_FacePtr()->Init(TopoDS::Face(static_cast<TopoShapePy*>(face)->getTopoShapePtr()->getShape()));
        Py_Return;
    }

    PyErr_Clear();
    double prec;
    PyObject* fwd = Py_True;
    if (PyArg_ParseTuple(args, "O!d|O!", &GeometrySurfacePy::Type, &face, &prec, &PyBool_Type, &fwd)) {
        if (face) {
            Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast(static_cast<GeometrySurfacePy*>(face)->getGeomSurfacePtr()->handle());
            getShapeFix_FacePtr()->Init(surf, prec, Base::asBoolean(fwd));
        }

        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "Supported arguments are:\n"
        "-- Face\n"
        "-- Surface, Precision, [Forward=True}\n"
        "   Precision is a Float\n"
        "   If Forward is the orientation will be FORWARD or REVERSED otherwise"
       );
    return nullptr;
}

PyObject* ShapeFix_FacePy::fixWireTool(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(ShapeFix_Wire) tool = getShapeFix_FacePtr()->FixWireTool();
    ShapeFix_WirePy* wire = new ShapeFix_WirePy(nullptr);
    wire->setHandle(tool);
    return wire;
}

PyObject* ShapeFix_FacePy::clearModes(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getShapeFix_FacePtr()->ClearModes();
    Py_Return;
}

PyObject* ShapeFix_FacePy::add(PyObject *args)
{
    PyObject* wire;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapeWirePy::Type, &wire))
        return nullptr;

    getShapeFix_FacePtr()->Add(TopoDS::Wire(static_cast<TopoShapePy*>(wire)->getTopoShapePtr()->getShape()));
    Py_Return;
}

PyObject* ShapeFix_FacePy::fixOrientation(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_FacePtr()->FixOrientation();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_FacePy::fixAddNaturalBound(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_FacePtr()->FixAddNaturalBound();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_FacePy::fixMissingSeam(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_FacePtr()->FixMissingSeam();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_FacePy::fixSmallAreaWire(PyObject *args)
{
    PyObject* removeSmall;
    if (!PyArg_ParseTuple(args, "O!", &PyBool_Type, &removeSmall))
        return nullptr;

    Standard_Boolean ok = getShapeFix_FacePtr()->FixSmallAreaWire(Base::asBoolean(removeSmall));
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_FacePy::fixLoopWire(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopTools_SequenceOfShape aResWires;
    Standard_Boolean ok = getShapeFix_FacePtr()->FixLoopWire(aResWires);
    Py::List list;
    for (int index = aResWires.Lower(); index <= aResWires.Upper(); index++) {
        TopoShape sh = aResWires(index);
        list.append(Py::asObject(sh.getPyObject()));
    }
    return Py::new_reference_to(Py::TupleN(Py::Boolean(ok), list));
}

PyObject* ShapeFix_FacePy::fixIntersectingWires(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_FacePtr()->FixIntersectingWires();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_FacePy::fixWiresTwoCoincidentEdges(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_FacePtr()->FixWiresTwoCoincEdges();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_FacePy::fixPeriodicDegenerated(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_FacePtr()->FixPeriodicDegenerated();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_FacePy::perform(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_FacePtr()->Perform();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_FacePy::face(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape shape = getShapeFix_FacePtr()->Face();
    return shape.getPyObject();
}

PyObject* ShapeFix_FacePy::result(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape shape = getShapeFix_FacePtr()->Result();
    return shape.getPyObject();
}

Py::Boolean ShapeFix_FacePy::getFixWireMode() const
{
    return Py::Boolean(getShapeFix_FacePtr()->FixWireMode());
}

void ShapeFix_FacePy::setFixWireMode(Py::Boolean arg)
{
    getShapeFix_FacePtr()->FixWireMode() = arg;
}

Py::Boolean ShapeFix_FacePy::getFixOrientationMode() const
{
    return Py::Boolean(getShapeFix_FacePtr()->FixOrientationMode());
}

void ShapeFix_FacePy::setFixOrientationMode(Py::Boolean arg)
{
    getShapeFix_FacePtr()->FixOrientationMode() = arg;
}

Py::Boolean ShapeFix_FacePy::getFixAddNaturalBoundMode() const
{
    return Py::Boolean(getShapeFix_FacePtr()->FixAddNaturalBoundMode());
}

void ShapeFix_FacePy::setFixAddNaturalBoundMode(Py::Boolean arg)
{
    getShapeFix_FacePtr()->FixAddNaturalBoundMode() = arg;
}

Py::Boolean ShapeFix_FacePy::getFixMissingSeamMode() const
{
    return Py::Boolean(getShapeFix_FacePtr()->FixMissingSeamMode());
}

void ShapeFix_FacePy::setFixMissingSeamMode(Py::Boolean arg)
{
    getShapeFix_FacePtr()->FixMissingSeamMode() = arg;
}

Py::Boolean ShapeFix_FacePy::getFixSmallAreaWireMode() const
{
    return Py::Boolean(getShapeFix_FacePtr()->FixSmallAreaWireMode());
}

void ShapeFix_FacePy::setFixSmallAreaWireMode(Py::Boolean arg)
{
    getShapeFix_FacePtr()->FixSmallAreaWireMode() = arg;
}

Py::Boolean ShapeFix_FacePy::getRemoveSmallAreaFaceMode() const
{
    return Py::Boolean(getShapeFix_FacePtr()->RemoveSmallAreaFaceMode());
}

void ShapeFix_FacePy::setRemoveSmallAreaFaceMode(Py::Boolean arg)
{
    getShapeFix_FacePtr()->RemoveSmallAreaFaceMode() = arg;
}

Py::Boolean ShapeFix_FacePy::getFixIntersectingWiresMode() const
{
    return Py::Boolean(getShapeFix_FacePtr()->FixIntersectingWiresMode());
}

void ShapeFix_FacePy::setFixIntersectingWiresMode(Py::Boolean arg)
{
    getShapeFix_FacePtr()->FixIntersectingWiresMode() = arg;
}

Py::Boolean ShapeFix_FacePy::getFixLoopWiresMode() const
{
    return Py::Boolean(getShapeFix_FacePtr()->FixLoopWiresMode());
}

void ShapeFix_FacePy::setFixLoopWiresMode(Py::Boolean arg)
{
    getShapeFix_FacePtr()->FixLoopWiresMode() = arg;
}

Py::Boolean ShapeFix_FacePy::getFixSplitFaceMode() const
{
    return Py::Boolean(getShapeFix_FacePtr()->FixSplitFaceMode());
}

void ShapeFix_FacePy::setFixSplitFaceMode(Py::Boolean arg)
{
    getShapeFix_FacePtr()->FixSplitFaceMode() = arg;
}

Py::Boolean ShapeFix_FacePy::getAutoCorrectPrecisionMode() const
{
    return Py::Boolean(getShapeFix_FacePtr()->AutoCorrectPrecisionMode());
}

void ShapeFix_FacePy::setAutoCorrectPrecisionMode(Py::Boolean arg)
{
    getShapeFix_FacePtr()->AutoCorrectPrecisionMode() = arg;
}

Py::Boolean ShapeFix_FacePy::getFixPeriodicDegeneratedMode() const
{
    return Py::Boolean(getShapeFix_FacePtr()->FixPeriodicDegeneratedMode());
}

void ShapeFix_FacePy::setFixPeriodicDegeneratedMode(Py::Boolean arg)
{
    getShapeFix_FacePtr()->FixPeriodicDegeneratedMode() = arg;
}

PyObject *ShapeFix_FacePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeFix_FacePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
