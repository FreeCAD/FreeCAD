// SPDX-License-Identifier: LGPL-2.1-or-later

/**************************************************************************
 *   Copyright (c) 2017 Shai Seger <shaise at gmail>                       *
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


#include <Base/PlacementPy.h>
#include <Base/RotationPy.h>
#include <Base/VectorPy.h>
#include <Base/PyWrapParseTupleAndKeywords.h>

#include <Gui/Document.h>
#include <Mod/Mesh/App/MeshPy.h>
#include <Mod/CAM/App/CommandPy.h>
#include <Mod/Part/App/TopoShapePy.h>

#include "DocumentPy.h"
// inclusion of the generated files (generated out of CAMSimPy.xml)
#include "CAMSimPy.h"
#include "CAMSimPy.cpp"


namespace CAMSimulator
{

// returns a string which represents the object e.g. when printed in python
std::string CAMSimPy::representation() const
{
    return std::string("<CAMSim object>");
}

PyObject* CAMSimPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of CAMSimPy and the Twin object
    return new CAMSimPy(new CAMSim);
}

// constructor method
int CAMSimPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


PyObject* CAMSimPy::ResetSimulation(PyObject* args)
{
    PyObject* pObjDoc;
    if (!PyArg_ParseTuple(args, "O!", &(Gui::DocumentPy::Type), &pObjDoc)) {
        return nullptr;
    }
    CAMSim* sim = getCAMSimPtr();
    Gui::Document* doc = static_cast<Gui::DocumentPy*>(pObjDoc)->getDocumentPtr();
    sim->resetSimulation(doc);
    Py_IncRef(Py_None);
    return Py_None;
}

PyObject* CAMSimPy::BeginSimulation(PyObject* args, PyObject* kwds)
{
    static const std::array<const char*, 3> kwlist {"stock", "resolution", nullptr};
    PyObject* pObjStock;
    float resolution;
    if (!Base::Wrapped_ParseTupleAndKeywords(
            args,
            kwds,
            "O!f",
            kwlist,
            &(Part::TopoShapePy::Type),
            &pObjStock,
            &resolution
        )) {
        return nullptr;
    }
    CAMSim* sim = getCAMSimPtr();
    const Part::TopoShape& stock = *static_cast<Part::TopoShapePy*>(pObjStock)->getTopoShapePtr();
    sim->BeginSimulation(stock, resolution);
    Py_IncRef(Py_None);
    return Py_None;
}

namespace
{

// appends a sequence of numbers to out (None adds nothing); false with the Python error set
bool ParseFloatList(PyObject* obj, const char* what, std::vector<float>& out)
{
    if (obj == Py_None) {
        return true;
    }
    PyObject* seq = PySequence_Fast(obj, what);
    if (!seq) {
        return false;
    }
    Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
    for (Py_ssize_t i = 0; i < n; ++i) {
        out.push_back((float)PyFloat_AsDouble(PySequence_Fast_GET_ITEM(seq, i)));
    }
    Py_DECREF(seq);
    return !PyErr_Occurred();
}

}  // namespace

PyObject* CAMSimPy::AddTool(PyObject* args, PyObject* kwds)
{
    static const std::array<const char*, 5>
        kwlist {"shape", "toolnumber", "diameter", "resolution", nullptr};
    PyObject* pObjToolShape;
    int toolNumber;
    float resolution;
    float diameter;
    if (!Base::Wrapped_ParseTupleAndKeywords(
            args,
            kwds,
            "Oiff",
            kwlist,
            &pObjToolShape,
            &toolNumber,
            &diameter,
            &resolution
        )) {
        return nullptr;
    }
    // The tool shape is defined by a list of 2d points that represents the tool revolving profile
    Py_ssize_t num_floats = PyList_Size(pObjToolShape);
    std::vector<float> toolProfile;
    for (Py_ssize_t i = 0; i < num_floats; ++i) {
        PyObject* item = PyList_GetItem(pObjToolShape, i);
        toolProfile.push_back(static_cast<float>(PyFloat_AsDouble(item)));
    }

    CAMSim* sim = getCAMSimPtr();
    sim->addTool(toolProfile, toolNumber, diameter, resolution);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* CAMSimPy::SetBaseShape(PyObject* args, PyObject* kwds)
{
    static const std::array<const char*, 3> kwlist {"shape", "resolution", nullptr};
    PyObject* pObjBaseShape;
    float resolution;
    if (!Base::Wrapped_ParseTupleAndKeywords(
            args,
            kwds,
            "O!f",
            kwlist,
            &(Part::TopoShapePy::Type),
            &pObjBaseShape,
            &resolution
        )) {
        return nullptr;
    }
    if (!PyArg_ParseTuple(args, "O!f", &(Part::TopoShapePy::Type), &pObjBaseShape, &resolution)) {
        return nullptr;
    }
    CAMSim* sim = getCAMSimPtr();
    const Part::TopoShape& baseShape
        = static_cast<Part::TopoShapePy*>(pObjBaseShape)->getTopoShapePtr()->getShape();
    sim->SetBaseShape(baseShape, resolution);

    Py_IncRef(Py_None);
    return Py_None;
}

PyObject* CAMSimPy::AddCommand(PyObject* args)
{
    PyObject* pObjCmd;
    if (!PyArg_ParseTuple(args, "O!", &(Path::CommandPy::Type), &pObjCmd)) {
        return nullptr;
    }
    CAMSim* sim = getCAMSimPtr();
    Path::Command* cmd = static_cast<Path::CommandPy*>(pObjCmd)->getCommandPtr();
    sim->AddCommand(cmd);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* CAMSimPy::BeginOperation(PyObject* args)
{
    const char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }
    getCAMSimPtr()->BeginOperation(name);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* CAMSimPy::SetFrame(PyObject* args)
{
    PyObject* pObjPlacement;
    PyObject* pObjPose = nullptr;
    float indexRate = 0;
    PyObject* pObjAngles = nullptr;
    if (!PyArg_ParseTuple(
            args,
            "O!|O!fO",
            &(Base::PlacementPy::Type),
            &pObjPlacement,
            &(Base::RotationPy::Type),
            &pObjPose,
            &indexRate,
            &pObjAngles
        )) {
        return nullptr;
    }
    Base::Rotation pose;
    if (pObjPose) {
        pose = *static_cast<Base::RotationPy*>(pObjPose)->getRotationPtr();
    }
    std::vector<float> angles;
    if (pObjAngles && !ParseFloatList(pObjAngles, "angles must be a sequence of numbers", angles)) {
        return nullptr;
    }
    DlgCAMSimulator::instance()->setFrame(
        *static_cast<Base::PlacementPy*>(pObjPlacement)->getPlacementPtr(),
        pose,
        indexRate,
        angles
    );

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* CAMSimPy::SetRotaryAxes(PyObject* args)
{
    PyObject* pObjAxes;
    if (!PyArg_ParseTuple(args, "O", &pObjAxes)) {
        return nullptr;
    }
    PyObject* seq = PySequence_Fast(
        pObjAxes,
        "axes must be a sequence of (name, direction, rate, sequence, head)"
    );
    if (!seq) {
        return nullptr;
    }
    std::vector<SimRotaryAxis> axes;
    for (Py_ssize_t i = 0; i < PySequence_Fast_GET_SIZE(seq); i++) {
        const char* name;
        PyObject* pObjDir;
        int head = 0;
        SimRotaryAxis axis;
        if (!PyArg_ParseTuple(
                PySequence_Fast_GET_ITEM(seq, i),
                "sO!fi|p",
                &name,
                &(Base::VectorPy::Type),
                &pObjDir,
                &axis.rate,
                &axis.sequence,
                &head
            )) {
            Py_DECREF(seq);
            return nullptr;
        }
        const Base::Vector3d dir = *static_cast<Base::VectorPy*>(pObjDir)->getVectorPtr();
        vec3_set(axis.axis, (float)dir.x, (float)dir.y, (float)dir.z);
        axis.name = name;
        axis.head = head != 0;
        axes.push_back(axis);
    }
    Py_DECREF(seq);
    DlgCAMSimulator::instance()->setRotaryAxes(axes);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* CAMSimPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int CAMSimPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

}  // namespace CAMSimulator
