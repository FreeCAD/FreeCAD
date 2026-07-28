// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDItemIJPy.h"
#include "MbDItemIJPy.cpp"

#include <App/DocumentObjectPy.h>

#include "MbDMarker.h"

namespace
{

MbDFEM::MbDMarker* markerFromPyObject(PyObject* object, const char* methodName)
{
    auto* documentObject = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
    if (!documentObject->isDerivedFrom<MbDFEM::MbDMarker>()) {
        PyErr_Format(PyExc_TypeError, "%s expects an MbDFEM::MbDMarker", methodName);
        return nullptr;
    }

    return static_cast<MbDFEM::MbDMarker*>(documentObject);
}

}  // namespace

std::string MbDFEM::MbDItemIJPy::representation() const
{
    return "<MbDFEM::MbDItemIJ>";
}

PyObject* MbDFEM::MbDItemIJPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MbDFEM::MbDItemIJPy::setCustomAttributes(const char* /*attr*/, PyObject* /*value*/)
{
    return 0;
}

PyObject* MbDFEM::MbDItemIJPy::setMarkerI(PyObject* args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args, "O!", &App::DocumentObjectPy::Type, &object)) {
        return nullptr;
    }

    auto* marker = markerFromPyObject(object, "setMarkerI");
    if (!marker) {
        return nullptr;
    }

    getMbDItemIJPtr()->setMarkerI(marker);
    Py_Return;
}

PyObject* MbDFEM::MbDItemIJPy::setMarkerJ(PyObject* args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args, "O!", &App::DocumentObjectPy::Type, &object)) {
        return nullptr;
    }

    auto* marker = markerFromPyObject(object, "setMarkerJ");
    if (!marker) {
        return nullptr;
    }

    getMbDItemIJPtr()->setMarkerJ(marker);
    Py_Return;
}

PyObject* MbDFEM::MbDItemIJPy::setMarkers(PyObject* args)
{
    PyObject* firstObject;
    PyObject* secondObject;
    if (!PyArg_ParseTuple(args,
                          "O!O!",
                          &App::DocumentObjectPy::Type,
                          &firstObject,
                          &App::DocumentObjectPy::Type,
                          &secondObject)) {
        return nullptr;
    }

    auto* firstMarker = markerFromPyObject(firstObject, "setMarkers");
    if (!firstMarker) {
        return nullptr;
    }

    auto* secondMarker = markerFromPyObject(secondObject, "setMarkers");
    if (!secondMarker) {
        return nullptr;
    }

    getMbDItemIJPtr()->setMarkers(firstMarker, secondMarker);
    Py_Return;
}
