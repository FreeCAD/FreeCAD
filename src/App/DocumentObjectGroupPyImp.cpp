// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2007 Werner Mayer
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



#include "DocumentObjectGroup.h"

// inclusion of the generated files (generated out of DocumentObjectGroupPy.xml)
#include "DocumentObjectGroupPy.h"
#include "DocumentObjectGroupPy.cpp"

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string DocumentObjectGroupPy::representation() const
{
    return {"<group object>"};
}

PyObject* DocumentObjectGroupPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int DocumentObjectGroupPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
