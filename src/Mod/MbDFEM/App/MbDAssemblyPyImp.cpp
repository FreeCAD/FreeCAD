// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDAssemblyPy.h"
#include "MbDAssemblyPy.cpp"

#include <App/DocumentObjectPy.h>

#include "MbDMarker.h"
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
