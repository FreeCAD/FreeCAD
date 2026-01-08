// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2015 Eivind Kvedalen <eivind@kvedalen.name>                            *
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


#include "PropertyRowHeights.h"
// inclusion of the generated files (generated out of PropertyRowHeightsPy.xml)
// clang-format off
#include "PropertyRowHeightsPy.h"
#include "PropertyRowHeightsPy.cpp"
// clang-format on


using namespace Spreadsheet;

// returns a string which represents the object e.g. when printed in python
std::string PropertyRowHeightsPy::representation() const
{
    return {"<PropertyRowHeights object>"};
}

PyObject* PropertyRowHeightsPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of PropertyRowHeightsPy and the Twin object
    return new PropertyRowHeightsPy(new PropertyRowHeights);
}

// constructor method
int PropertyRowHeightsPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* PropertyRowHeightsPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int PropertyRowHeightsPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
