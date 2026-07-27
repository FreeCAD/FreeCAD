// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDPartPy.h"
#include "MbDPartPy.cpp"

#include <App/DocumentObjectPy.h>

#include "MbDMarker.h"

std::string MbDFEM::MbDPartPy::representation() const
{
    return "<MbDFEM::MbDPart>";
}

PyObject* MbDFEM::MbDPartPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MbDFEM::MbDPartPy::setCustomAttributes(const char* /*attr*/, PyObject* /*value*/)
{
    return 0;
}

PyObject* MbDFEM::MbDPartPy::addMarker(PyObject* args)
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

    getMbDPartPtr()->addMarker(static_cast<MbDFEM::MbDMarker*>(documentObject));
    Py_Return;
}

PyObject* MbDFEM::MbDPartPy::getMarkersFolder(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto* folder = getMbDPartPtr()->getMarkersFolder();
    if (!folder) {
        Py_Return;
    }

    return Py::new_reference_to(Py::asObject(folder->getPyObject()));
}
