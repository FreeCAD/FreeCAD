// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
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

#include <array>

#include <Base/PyWrapParseTupleAndKeywords.h>

#include "MaterialObserverPython.h"
#include "MaterialsModulePy.h"

#include "MaterialsModulePy.cpp"

using namespace Materials;

namespace
{

PyObject* parseObserverArgument(PyObject* args, PyObject* kwd)
{
    static constexpr std::array<const char*, 2> keywords {"observer", nullptr};

    PyObject* obj {};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwd, "O", keywords, &obj)) {
        return nullptr;
    }

    if (obj == Py_None) {
        PyErr_SetString(PyExc_TypeError, "observer cannot be None");
        return nullptr;
    }

    return obj;
}

}  // namespace

PyObject* MaterialsModulePy::addMaterialObserver(PyObject* args, PyObject* kwd)
{
    PyObject* obj = parseObserverArgument(args, kwd);
    if (!obj) {
        return nullptr;
    }

    MaterialObserverPython::addObserver(Py::Object(obj));
    return Py::new_reference_to(Py::None());
}

PyObject* MaterialsModulePy::removeMaterialObserver(PyObject* args, PyObject* kwd)
{
    PyObject* obj = parseObserverArgument(args, kwd);
    if (!obj) {
        return nullptr;
    }

    MaterialObserverPython::removeObserver(Py::Object(obj));
    return Py::new_reference_to(Py::None());
}
