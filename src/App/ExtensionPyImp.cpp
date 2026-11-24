// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2016 Stefan Tröger
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

#include "ExtensionContainer.h"

// inclusion of the generated files (generated out of ExtensionPy.pyi)
#include <App/ExtensionPy.h>
#include <App/ExtensionPy.cpp>

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string ExtensionPy::representation() const
{
    return {"<extension>"};
}

Py::Object ExtensionPy::getExtendedObject() const
{
    return Py::Object(getExtensionPtr()->getExtendedContainer()->getPyObject(), true);
}

PyObject* ExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
