// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2018 Zheng Lei
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


#include "StringHasher.h"

#include "StringIDPy.h"
#include "StringIDPy.cpp"

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string StringIDPy::representation() const
{
    return getStringIDPtr()->toString(this->_index);
}

PyObject* StringIDPy::isSame(PyObject* args) const
{
    PyObject* other = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &StringIDPy::Type, &other)) {
        return nullptr;
    }

    auto* otherPy = static_cast<StringIDPy*>(other);
    bool same =
        (otherPy->getStringIDPtr() == this->getStringIDPtr()) && (otherPy->_index == this->_index);

    return PyBool_FromLong(same ? 1 : 0);
}

Py::Long StringIDPy::getValue() const
{
    return Py::Long(getStringIDPtr()->value());
}

Py::List StringIDPy::getRelated() const
{
    Py::List list;
    for (const auto& id : getStringIDPtr()->relatedIDs()) {
        list.append(Py::Long(id.value()));
    }

    return list;
}

Py::String StringIDPy::getData() const
{
    return {getStringIDPtr()->dataToText(this->_index)};
}

Py::Boolean StringIDPy::getIsBinary() const
{
    return {getStringIDPtr()->isBinary()};
}

Py::Boolean StringIDPy::getIsHashed() const
{
    return {getStringIDPtr()->isHashed()};
}

Py::Long StringIDPy::getIndex() const
{
    return Py::Long(this->_index);
}

void StringIDPy::setIndex(Py::Long index)
{
    this->_index = index;
}

PyObject* StringIDPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int StringIDPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
