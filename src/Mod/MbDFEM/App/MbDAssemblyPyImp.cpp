// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDAssemblyPy.h"
#include "MbDAssemblyPy.cpp"

#include <App/DocumentObjectPy.h>

#include "MbDAction.h"
#include "MbDJoint.h"
#include "MbDMotion.h"
#include "MbDParameters.h"
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

PyObject* MbDFEM::MbDAssemblyPy::removePart(PyObject* args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args, "O!", &App::DocumentObjectPy::Type, &object)) {
        return nullptr;
    }

    auto* documentObject = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    if (!documentObject->isDerivedFrom<MbDFEM::MbDPart>()) {
        PyErr_SetString(PyExc_TypeError, "removePart expects an MbDFEM::MbDPart");
        return nullptr;
    }

    getMbDAssemblyPtr()->removePart(static_cast<MbDFEM::MbDPart*>(documentObject));
    Py_Return;
}

PyObject* MbDFEM::MbDAssemblyPy::addFixedPart(PyObject* args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args, "O!", &App::DocumentObjectPy::Type, &object)) {
        return nullptr;
    }

    auto* documentObject = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    if (!documentObject->isDerivedFrom<MbDFEM::MbDPart>()) {
        PyErr_SetString(PyExc_TypeError, "addFixedPart expects an MbDFEM::MbDPart");
        return nullptr;
    }

    getMbDAssemblyPtr()->addFixedPart(static_cast<MbDFEM::MbDPart*>(documentObject));
    Py_Return;
}

PyObject* MbDFEM::MbDAssemblyPy::removeFixedPart(PyObject* args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args, "O!", &App::DocumentObjectPy::Type, &object)) {
        return nullptr;
    }

    auto* documentObject = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    if (!documentObject->isDerivedFrom<MbDFEM::MbDPart>()) {
        PyErr_SetString(PyExc_TypeError, "removeFixedPart expects an MbDFEM::MbDPart");
        return nullptr;
    }

    getMbDAssemblyPtr()->removeFixedPart(static_cast<MbDFEM::MbDPart*>(documentObject));
    Py_Return;
}

PyObject* MbDFEM::MbDAssemblyPy::groundPart(PyObject* args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args, "O!", &App::DocumentObjectPy::Type, &object)) {
        return nullptr;
    }

    auto* documentObject = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    if (!documentObject->isDerivedFrom<MbDFEM::MbDPart>()) {
        PyErr_SetString(PyExc_TypeError, "groundPart expects an MbDFEM::MbDPart");
        return nullptr;
    }

    getMbDAssemblyPtr()->groundPart(static_cast<MbDFEM::MbDPart*>(documentObject));
    Py_Return;
}

PyObject* MbDFEM::MbDAssemblyPy::addAssembly(PyObject* args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args, "O!", &App::DocumentObjectPy::Type, &object)) {
        return nullptr;
    }

    auto* documentObject = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    if (!documentObject->isDerivedFrom<MbDFEM::MbDAssembly>()) {
        PyErr_SetString(PyExc_TypeError, "addAssembly expects an MbDFEM::MbDAssembly");
        return nullptr;
    }

    getMbDAssemblyPtr()->addAssembly(static_cast<MbDFEM::MbDAssembly*>(documentObject));
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

PyObject* MbDFEM::MbDAssemblyPy::getFixedPartsFolder(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto* folder = getMbDAssemblyPtr()->getFixedPartsFolder();
    if (!folder) {
        Py_Return;
    }

    return Py::new_reference_to(Py::asObject(folder->getPyObject()));
}

PyObject* MbDFEM::MbDAssemblyPy::getAssembliesFolder(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto* folder = getMbDAssemblyPtr()->getAssembliesFolder();
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

PyObject* MbDFEM::MbDAssemblyPy::getSimulationParameters(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto* parameters = getMbDAssemblyPtr()->getSimulationParameters();
    if (!parameters) {
        Py_Return;
    }

    return Py::new_reference_to(Py::asObject(parameters->getPyObject()));
}

PyObject* MbDFEM::MbDAssemblyPy::getAnimationParameters(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto* parameters = getMbDAssemblyPtr()->getAnimationParameters();
    if (!parameters) {
        Py_Return;
    }

    return Py::new_reference_to(Py::asObject(parameters->getPyObject()));
}

PyObject* MbDFEM::MbDAssemblyPy::getGravity(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto* gravity = getMbDAssemblyPtr()->getGravity();
    if (!gravity) {
        Py_Return;
    }

    return Py::new_reference_to(Py::asObject(gravity->getPyObject()));
}

PyObject* MbDFEM::MbDAssemblyPy::ensureSimulationParameters(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto* parameters = getMbDAssemblyPtr()->ensureSimulationParameters();
    if (!parameters) {
        Py_Return;
    }

    return Py::new_reference_to(Py::asObject(parameters->getPyObject()));
}

PyObject* MbDFEM::MbDAssemblyPy::ensureGravity(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto* gravity = getMbDAssemblyPtr()->ensureGravity();
    if (!gravity) {
        Py_Return;
    }

    return Py::new_reference_to(Py::asObject(gravity->getPyObject()));
}

PyObject* MbDFEM::MbDAssemblyPy::ensureAnimationParameters(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto* parameters = getMbDAssemblyPtr()->ensureAnimationParameters();
    if (!parameters) {
        Py_Return;
    }

    return Py::new_reference_to(Py::asObject(parameters->getPyObject()));
}
