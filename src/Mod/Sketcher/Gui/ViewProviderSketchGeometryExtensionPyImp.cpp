// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023 Abdullah Tahiri
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#include "ViewProviderSketchGeometryExtensionPy.h"

#include "ViewProviderSketchGeometryExtensionPy.cpp"


using namespace SketcherGui;

// returns a string which represents the object e.g. when printed in python
std::string ViewProviderSketchGeometryExtensionPy::representation() const
{
    std::stringstream str;
    str << "<ViewProviderSketchGeometryExtension ( VisualLayerId=";

    str << getViewProviderSketchGeometryExtensionPtr()->getVisualLayerId() << "\") >";
    return str.str();
}

PyObject* ViewProviderSketchGeometryExtensionPy::PyMake(
    struct _typeobject*,
    PyObject*,
    PyObject*
)  // Python wrapper
{
    // create a new instance of PointPy and the Twin object
    return new ViewProviderSketchGeometryExtensionPy(new ViewProviderSketchGeometryExtension);
}

// constructor method
int ViewProviderSketchGeometryExtensionPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{

    if (PyArg_ParseTuple(args, "")) {
        // default extension
        return 0;
    }

    PyErr_Clear();
    int VisualLayerId;
    if (PyArg_ParseTuple(args, "i", &VisualLayerId)) {
        this->getViewProviderSketchGeometryExtensionPtr()->setVisualLayerId(VisualLayerId);
        return 0;
    }

    PyErr_SetString(
        PyExc_TypeError,
        "ViewProviderSketchGeometryExtension constructor accepts:\n"
        "-- empty parameter list\n"
        "-- int\n"
    );
    return -1;
}

Py::Long ViewProviderSketchGeometryExtensionPy::getVisualLayerId() const
{
    return Py::Long(this->getViewProviderSketchGeometryExtensionPtr()->getVisualLayerId());
}

void ViewProviderSketchGeometryExtensionPy::setVisualLayerId(Py::Long Id)
{
    this->getViewProviderSketchGeometryExtensionPtr()->setVisualLayerId(Id);
}

PyObject* ViewProviderSketchGeometryExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ViewProviderSketchGeometryExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
