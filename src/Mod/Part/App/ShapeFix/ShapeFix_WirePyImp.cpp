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

#include "ShapeFix/ShapeFix_WirePy.h"
#include "ShapeFix/ShapeFix_WirePy.cpp"
#include "ShapeFix/ShapeFix_EdgePy.h"
#include "GeometrySurfacePy.h"
#include "OCCError.h"
#include "Tools.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeWirePy.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string ShapeFix_WirePy::representation() const
{
    return "<ShapeFix_Wire object>";
}

PyObject *ShapeFix_WirePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    return new ShapeFix_WirePy(nullptr);
}

// constructor method
int ShapeFix_WirePy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    if (PyArg_ParseTuple(args, "")) {
        // Nothing needs to be done
        setHandle(new ShapeFix_Wire);
        return 0;
    }

    PyErr_Clear();
    PyObject* wire;
    PyObject* face;
    double prec;
    if (PyArg_ParseTuple(args, "O!O!d", &TopoShapeWirePy::Type, &wire,
                                        &TopoShapeFacePy::Type, &face, &prec)) {
        setHandle(new ShapeFix_Wire);
        TopoDS_Shape w = static_cast<TopoShapePy*>(wire)->getTopoShapePtr()->getShape();
        TopoDS_Shape f = static_cast<TopoShapePy*>(face)->getTopoShapePtr()->getShape();
        getShapeFix_WirePtr()->Init(TopoDS::Wire(w), TopoDS::Face(f), prec);

        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Supported arguments are:\n"
        "-- Empty\n"
        "-- Wire, Face, Precision"
    );
    return -1;
}

PyObject* ShapeFix_WirePy::init(PyObject *args)
{
    PyObject* wire;
    PyObject* face;
    double prec;
    if (!PyArg_ParseTuple(args, "O!O!d", &TopoShapeWirePy::Type, &wire,
                                         &TopoShapeFacePy::Type, &face, &prec))
        return nullptr;

    TopoDS_Shape w = static_cast<TopoShapePy*>(wire)->getTopoShapePtr()->getShape();
    TopoDS_Shape f = static_cast<TopoShapePy*>(face)->getTopoShapePtr()->getShape();
    getShapeFix_WirePtr()->Init(TopoDS::Wire(w), TopoDS::Face(f), prec);

    Py_Return;
}

PyObject* ShapeFix_WirePy::fixEdgeTool(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(ShapeFix_Edge) tool = getShapeFix_WirePtr()->FixEdgeTool();
    ShapeFix_EdgePy* edge = new ShapeFix_EdgePy(nullptr);
    edge->setHandle(tool);
    return edge;
}

PyObject* ShapeFix_WirePy::clearModes(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getShapeFix_WirePtr()->ClearModes();
    Py_Return;
}

PyObject* ShapeFix_WirePy::clearStatuses(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getShapeFix_WirePtr()->ClearStatuses();
    Py_Return;
}

PyObject* ShapeFix_WirePy::load(PyObject *args)
{
    PyObject* wire;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapeWirePy::Type, &wire))
        return nullptr;

    TopoDS_Shape w = static_cast<TopoShapePy*>(wire)->getTopoShapePtr()->getShape();
    getShapeFix_WirePtr()->Load(TopoDS::Wire(w));
    Py_Return;
}

PyObject* ShapeFix_WirePy::setFace(PyObject *args)
{
    PyObject* face;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapeFacePy::Type, &face))
        return nullptr;

    TopoDS_Shape f = static_cast<TopoShapePy*>(face)->getTopoShapePtr()->getShape();
    getShapeFix_WirePtr()->SetFace(TopoDS::Face(f));
    Py_Return;
}

PyObject* ShapeFix_WirePy::setSurface(PyObject *args)
{
    PyObject* surface;
    PyObject* plm = nullptr;
    if (!PyArg_ParseTuple(args, "O!|O!", &GeometrySurfacePy::Type, &surface,
                                         &Base::PlacementPy::Type, &plm))
        return nullptr;

    Handle(Geom_Surface) surf = Handle(Geom_Surface)::DownCast(static_cast<GeometrySurfacePy*>(surface)->getGeomSurfacePtr()->handle());
    if (plm) {
        Base::Placement* pm = static_cast<Base::PlacementPy*>(plm)->getPlacementPtr();
        TopLoc_Location loc = Tools::fromPlacement(*pm);
        getShapeFix_WirePtr()->SetSurface(surf, loc);
    }
    else {
        getShapeFix_WirePtr()->SetSurface(surf);
    }
    Py_Return;
}

PyObject* ShapeFix_WirePy::setMaxTailAngle(PyObject *args)
{
    double angle;
    if (!PyArg_ParseTuple(args, "d", &angle))
        return nullptr;

    getShapeFix_WirePtr()->SetMaxTailAngle(angle);
    Py_Return;
}

PyObject* ShapeFix_WirePy::setMaxTailWidth(PyObject *args)
{
    double width;
    if (!PyArg_ParseTuple(args, "d", &width))
        return nullptr;

    getShapeFix_WirePtr()->SetMaxTailWidth(width);
    Py_Return;
}

PyObject* ShapeFix_WirePy::isLoaded(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_WirePtr()->IsLoaded();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_WirePy::isReady(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_WirePtr()->IsReady();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_WirePy::numberOfEdges(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    int num = getShapeFix_WirePtr()->NbEdges();
    return Py::new_reference_to(Py::Long(num));
}

PyObject* ShapeFix_WirePy::wire(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape wire = getShapeFix_WirePtr()->Wire();
    return wire.getPyObject();
}

PyObject* ShapeFix_WirePy::wireAPIMake(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape wire = getShapeFix_WirePtr()->WireAPIMake();
    return wire.getPyObject();
}

PyObject* ShapeFix_WirePy::face(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape face = getShapeFix_WirePtr()->Face();
    return face.getPyObject();
}

PyObject* ShapeFix_WirePy::perform(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_WirePtr()->Perform();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_WirePy::fixReorder(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_WirePtr()->FixReorder();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_WirePy::fixSmall(PyObject *args)
{
    PyObject* lock;
    double prec = 0.0;
    if (PyArg_ParseTuple(args, "O!|d", &PyBool_Type, &lock, &prec)) {
        try {
            int num = getShapeFix_WirePtr()->FixSmall(Base::asBoolean(lock), prec);
            return Py::new_reference_to(Py::Long(num));
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_Clear();
    int num;
    if (PyArg_ParseTuple(args, "iO!d", &num, &PyBool_Type, &lock, &prec)) {
        try {
            bool ok = getShapeFix_WirePtr()->FixSmall(num, Base::asBoolean(lock), prec);
            return Py::new_reference_to(Py::Boolean(ok));
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_SetString(PyExc_TypeError, "Arguments must be:\n"
                    "-- fixSmall(bool, [float]) or\n"
                    "-- fixSmall(int, bool, float)");
    return nullptr;
}

PyObject* ShapeFix_WirePy::fixConnected(PyObject *args)
{
    double prec = -1.0;
    if (PyArg_ParseTuple(args, "|d", &prec)) {
        try {
            Standard_Boolean ok = getShapeFix_WirePtr()->FixConnected(prec);
            return Py::new_reference_to(Py::Boolean(ok));
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_Clear();
    int num;
    if (PyArg_ParseTuple(args, "id", &num, &prec)) {
        try {
            Standard_Boolean ok = getShapeFix_WirePtr()->FixConnected(num, prec);
            return Py::new_reference_to(Py::Boolean(ok));
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_SetString(PyExc_TypeError, "Arguments must be:\n"
                    "-- fixConnected([float]) or\n"
                    "-- fixConnected(int, float)");
    return nullptr;
}

PyObject* ShapeFix_WirePy::fixEdgeCurves(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_WirePtr()->FixEdgeCurves();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_WirePy::fixDegenerated(PyObject *args)
{
    int num = -1;
    if (!PyArg_ParseTuple(args, "|i", &num))
        return nullptr;

    try {
        Standard_Boolean ok = num > -1 ? getShapeFix_WirePtr()->FixDegenerated(num)
                                       : getShapeFix_WirePtr()->FixDegenerated();
        return Py::new_reference_to(Py::Boolean(ok));
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* ShapeFix_WirePy::fixSelfIntersection(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_WirePtr()->FixSelfIntersection();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_WirePy::fixLacking(PyObject *args)
{
    PyObject* force = Py_False;
    if (PyArg_ParseTuple(args, "|O!", &PyBool_Type, &force)) {
        try {
            Standard_Boolean ok = getShapeFix_WirePtr()->FixLacking(Base::asBoolean(force));
            return Py::new_reference_to(Py::Boolean(ok));
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_Clear();
    int num;
    force = Py_False;
    if (PyArg_ParseTuple(args, "i|O!", &num, &PyBool_Type, &force)) {
        try {
            Standard_Boolean ok = getShapeFix_WirePtr()->FixLacking(num, Base::asBoolean(force));
            return Py::new_reference_to(Py::Boolean(ok));
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_SetString(PyExc_TypeError, "Arguments must be:\n"
                    "-- fixLacking([bool=False]) or\n"
                    "-- fixLacking(int, bool)");
    return nullptr;
}

PyObject* ShapeFix_WirePy::fixClosed(PyObject *args)
{
    double prec = -1.0;
    if (!PyArg_ParseTuple(args, "|d", &prec))
        return nullptr;

    Standard_Boolean ok = getShapeFix_WirePtr()->FixClosed(prec);
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_WirePy::fixGaps3d(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_WirePtr()->FixGaps3d();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_WirePy::fixGaps2d(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_WirePtr()->FixGaps2d();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_WirePy::fixGap3d(PyObject *args)
{
    int num;
    PyObject* convert;
    if (!PyArg_ParseTuple(args, "iO!", &num, &PyBool_Type, &convert))
        return nullptr;

    try {
        Standard_Boolean ok = getShapeFix_WirePtr()->FixGap3d(num, Base::asBoolean(convert));
        return Py::new_reference_to(Py::Boolean(ok));
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* ShapeFix_WirePy::fixGap2d(PyObject *args)
{
    int num;
    PyObject* convert;
    if (!PyArg_ParseTuple(args, "iO!", &num, &PyBool_Type, &convert))
        return nullptr;

    try {
        Standard_Boolean ok = getShapeFix_WirePtr()->FixGap2d(num, Base::asBoolean(convert));
        return Py::new_reference_to(Py::Boolean(ok));
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* ShapeFix_WirePy::fixSeam(PyObject *args)
{
    int num;
    if (!PyArg_ParseTuple(args, "i", &num))
        return nullptr;

    try {
        Standard_Boolean ok = getShapeFix_WirePtr()->FixSeam(num);
        return Py::new_reference_to(Py::Boolean(ok));
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* ShapeFix_WirePy::fixShifted(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_WirePtr()->FixShifted();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_WirePy::fixNotchedEdges(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_WirePtr()->FixNotchedEdges();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_WirePy::fixTails(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_WirePtr()->FixTails();
    return Py::new_reference_to(Py::Boolean(ok));
}

Py::Boolean ShapeFix_WirePy::getModifyTopologyMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->ModifyTopologyMode());
}

void ShapeFix_WirePy::setModifyTopologyMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->ModifyTopologyMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getModifyGeometryMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->ModifyGeometryMode());
}

void ShapeFix_WirePy::setModifyGeometryMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->ModifyGeometryMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getModifyRemoveLoopMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->ModifyRemoveLoopMode());
}

void ShapeFix_WirePy::setModifyRemoveLoopMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->ModifyRemoveLoopMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getClosedWireMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->ClosedWireMode());
}

void ShapeFix_WirePy::setClosedWireMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->ClosedWireMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getPreferencePCurveMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->PreferencePCurveMode());
}

void ShapeFix_WirePy::setPreferencePCurveMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->PreferencePCurveMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixGapsByRangesMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixGapsByRangesMode());
}

void ShapeFix_WirePy::setFixGapsByRangesMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixGapsByRangesMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixReorderMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixReorderMode());
}

void ShapeFix_WirePy::setFixReorderMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixReorderMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixSmallMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixSmallMode());
}

void ShapeFix_WirePy::setFixSmallMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixSmallMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixConnectedMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixConnectedMode());
}

void ShapeFix_WirePy::setFixConnectedMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixConnectedMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixEdgeCurvesMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixEdgeCurvesMode());
}

void ShapeFix_WirePy::setFixEdgeCurvesMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixEdgeCurvesMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixDegeneratedMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixDegeneratedMode());
}

void ShapeFix_WirePy::setFixDegeneratedMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixDegeneratedMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixSelfIntersectionMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixSelfIntersectionMode());
}

void ShapeFix_WirePy::setFixSelfIntersectionMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixSelfIntersectionMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixLackingMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixLackingMode());
}

void ShapeFix_WirePy::setFixLackingMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixLackingMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixGaps3dMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixGaps3dMode());
}

void ShapeFix_WirePy::setFixGaps3dMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixGaps3dMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixGaps2dMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixGaps2dMode());
}

void ShapeFix_WirePy::setFixGaps2dMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixGaps2dMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixReversed2dMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixReversed2dMode());
}

void ShapeFix_WirePy::setFixReversed2dMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixReversed2dMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixRemovePCurveMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixRemovePCurveMode());
}

void ShapeFix_WirePy::setFixRemovePCurveMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixRemovePCurveMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixAddPCurveMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixAddPCurveMode());
}

void ShapeFix_WirePy::setFixAddPCurveMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixAddPCurveMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixRemoveCurve3dMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixRemoveCurve3dMode());
}

void ShapeFix_WirePy::setFixRemoveCurve3dMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixRemoveCurve3dMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixAddCurve3dMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixAddCurve3dMode());
}

void ShapeFix_WirePy::setFixAddCurve3dMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixAddCurve3dMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixSeamMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixSeamMode());
}

void ShapeFix_WirePy::setFixSeamMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixSeamMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixShiftedMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixShiftedMode());
}

void ShapeFix_WirePy::setFixShiftedMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixShiftedMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixSameParameterMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixSameParameterMode());
}

void ShapeFix_WirePy::setFixSameParameterMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixSameParameterMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixVertexToleranceMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixVertexToleranceMode());
}

void ShapeFix_WirePy::setFixVertexToleranceMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixVertexToleranceMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixNotchedEdgesMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixNotchedEdgesMode());
}

void ShapeFix_WirePy::setFixNotchedEdgesMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixNotchedEdgesMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixSelfIntersectingEdgeMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixSelfIntersectingEdgeMode());
}

void ShapeFix_WirePy::setFixSelfIntersectingEdgeMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixSelfIntersectingEdgeMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixIntersectingEdgesMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixIntersectingEdgesMode());
}

void ShapeFix_WirePy::setFixIntersectingEdgesMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixIntersectingEdgesMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixNonAdjacentIntersectingEdgesMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixNonAdjacentIntersectingEdgesMode());
}

void ShapeFix_WirePy::setFixNonAdjacentIntersectingEdgesMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixNonAdjacentIntersectingEdgesMode() = arg;
}

Py::Boolean ShapeFix_WirePy::getFixTailMode() const
{
    return Py::Boolean(getShapeFix_WirePtr()->FixTailMode());
}

void ShapeFix_WirePy::setFixTailMode(Py::Boolean arg)
{
    getShapeFix_WirePtr()->FixTailMode() = arg;
}

PyObject *ShapeFix_WirePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeFix_WirePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
