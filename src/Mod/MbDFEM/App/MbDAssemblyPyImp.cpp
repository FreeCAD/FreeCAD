// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDAssemblyPy.h"
#include "MbDAssemblyPy.cpp"

#include <App/DocumentObjectPy.h>

#include "MbDAction.h"
#include "MbDJoint.h"
#include "MbDMarker.h"
#include "MbDMotion.h"
#include "MbDPart.h"

std::string MbDFEM::MbDAssemblyPy::representation() const
{
    return "<MbDFEM::MbDAssembly>";
}

PyObject* MbDFEM::MbDAssemblyPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MbDFEM::MbDAssemblyPy::setCustomAttributes(const char* /*attr*/, PyObject* /*value*/)
{
    return 0;
}

PyObject* MbDFEM::MbDAssemblyPy::addPart(PyObject* args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args, "O!", &App::DocumentObjectPy::Type, &object)) {
        return nullptr;
    }

    auto* documentObject = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    if (!documentObject->isDerivedFrom<MbDFEM::MbDPart>()) {
        PyErr_SetString(PyExc_TypeError, "addPart expects an MbDFEM::MbDPart");
        return nullptr;
    }

    getMbDAssemblyPtr()->addPart(static_cast<MbDFEM::MbDPart*>(documentObject));
    Py_Return;
}

PyObject* MbDFEM::MbDAssemblyPy::addMarker(PyObject* args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args, "O!", &App::DocumentObjectPy::Type, &object)) {
        return nullptr;
    }

    auto* documentObject = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    if (!documentObject->isDerivedFrom<MbDFEM::MbDMarker>()) {
        PyErr_SetString(PyExc_TypeError, "addMarker expects an MbDFEM::MbDMarker");
        return nullptr;
    }

    getMbDAssemblyPtr()->addMarker(static_cast<MbDFEM::MbDMarker*>(documentObject));
    Py_Return;
}

PyObject* MbDFEM::MbDAssemblyPy::addJoint(PyObject* args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args, "O!", &App::DocumentObjectPy::Type, &object)) {
        return nullptr;
    }

    auto* documentObject = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    if (!documentObject->isDerivedFrom<MbDFEM::MbDJoint>()) {
        PyErr_SetString(PyExc_TypeError, "addJoint expects an MbDFEM::MbDJoint");
        return nullptr;
    }

    getMbDAssemblyPtr()->addJoint(static_cast<MbDFEM::MbDJoint*>(documentObject));
    Py_Return;
}

PyObject* MbDFEM::MbDAssemblyPy::addMotion(PyObject* args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args, "O!", &App::DocumentObjectPy::Type, &object)) {
        return nullptr;
    }

    auto* documentObject = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    if (!documentObject->isDerivedFrom<MbDFEM::MbDMotion>()) {
        PyErr_SetString(PyExc_TypeError, "addMotion expects an MbDFEM::MbDMotion");
        return nullptr;
    }

    getMbDAssemblyPtr()->addMotion(static_cast<MbDFEM::MbDMotion*>(documentObject));
    Py_Return;
}

PyObject* MbDFEM::MbDAssemblyPy::addAction(PyObject* args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args, "O!", &App::DocumentObjectPy::Type, &object)) {
        return nullptr;
    }

    auto* documentObject = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    if (!documentObject->isDerivedFrom<MbDFEM::MbDAction>()) {
        PyErr_SetString(PyExc_TypeError, "addAction expects an MbDFEM::MbDAction");
        return nullptr;
    }

    getMbDAssemblyPtr()->addAction(static_cast<MbDFEM::MbDAction*>(documentObject));
    Py_Return;
}

PyObject* MbDFEM::MbDAssemblyPy::getPartsFolder(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto* folder = getMbDAssemblyPtr()->getPartsFolder();
    if (!folder) {
        Py_Return;
    }

    return Py::new_reference_to(Py::asObject(folder->getPyObject()));
}

PyObject* MbDFEM::MbDAssemblyPy::getJointsFolder(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto* folder = getMbDAssemblyPtr()->getJointsFolder();
    if (!folder) {
        Py_Return;
    }

    return Py::new_reference_to(Py::asObject(folder->getPyObject()));
}

PyObject* MbDFEM::MbDAssemblyPy::getMotionsFolder(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto* folder = getMbDAssemblyPtr()->getMotionsFolder();
    if (!folder) {
        Py_Return;
    }

    return Py::new_reference_to(Py::asObject(folder->getPyObject()));
}

PyObject* MbDFEM::MbDAssemblyPy::getActionsFolder(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto* folder = getMbDAssemblyPtr()->getActionsFolder();
    if (!folder) {
        Py_Return;
    }

    return Py::new_reference_to(Py::asObject(folder->getPyObject()));
}

PyObject* MbDFEM::MbDAssemblyPy::getMarkersFolder(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto* folder = getMbDAssemblyPtr()->getMarkersFolder();
    if (!folder) {
        Py_Return;
    }

    return Py::new_reference_to(Py::asObject(folder->getPyObject()));
}
