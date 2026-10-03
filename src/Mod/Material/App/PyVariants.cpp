// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include <string>
#include <variant>

#include <Base/Quantity.h>
#include <Base/QuantityPy.h>
#include <CXX/Objects.hxx>

#include "Exceptions.h"
#include "PyVariants.h"

namespace
{

template<class... Ts>
struct Overloaded: Ts...
{
    using Ts::operator()...;
};

}  // namespace

PyObject* Materials::pyObjectFromValue(const Value& value)
{
    return std::visit(
        Overloaded {[](std::monostate) -> PyObject* { Py_RETURN_NONE; },
                    [](const std::string& text) -> PyObject* {
                        return PyUnicode_FromStringAndSize(text.data(),
                                                           static_cast<Py_ssize_t>(text.size()));
                    },
                    [](bool flag) -> PyObject* { return Py::new_reference_to(Py::Boolean(flag)); },
                    [](int number) -> PyObject* { return PyLong_FromLong(number); },
                    [](double number) -> PyObject* { return PyFloat_FromDouble(number); },
                    [](const Base::Quantity& quantity) -> PyObject* {
                        return new Base::QuantityPy(new Base::Quantity(quantity));
                    },
                    [](const ValueList& list) -> PyObject* {
                        return Py::new_reference_to(getList(list));
                    }},
        value.variant());
}

Py::List Materials::getList(const ValueList& value)
{
    Py::List list;
    for (const auto& item : value) {
        list.append(Py::asObject(pyObjectFromValue(item)));
    }

    return list;
}
