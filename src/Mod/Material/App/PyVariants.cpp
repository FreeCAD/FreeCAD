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

#include "PreCompiled.h"

#include <QVariant>

#include "PyVariants.h"
#include "Exceptions.h"

using namespace Materials;

PyObject* Materials::_pyObjectFromVariant(const QVariant& value)
{
    if (value.isNull()) {
        Py_RETURN_NONE;
    }

    if (value.userType() == qMetaTypeId<Base::Quantity>()) {
        return new Base::QuantityPy(new Base::Quantity(value.value<Base::Quantity>()));
    }
    if (value.userType() == QMetaType::Double) {
        return PyFloat_FromDouble(value.toDouble());
    }
    if (value.userType() == QMetaType::Float) {
        return PyFloat_FromDouble(value.toFloat());
    }
    if (value.userType() == QMetaType::Int) {
        return PyLong_FromLong(value.toInt());
    }
    if (value.userType() == QMetaType::Long) {
        return PyLong_FromLong(value.toInt());
    }
    if (value.userType() == QMetaType::Bool) {
        return Py::new_reference_to(Py::Boolean(value.toBool()));
    }
    if (value.userType() == QMetaType::QString) {
        return PyUnicode_FromString(value.toString().toStdString().c_str());
    }
    if (value.userType() == qMetaTypeId<QList<QVariant>>()) {
        return Py::new_reference_to(getList(value));
    }

    throw UnknownValueType();
}

Py::List Materials::getList(const QVariant& value)
{
    auto listValue = value.value<QList<QVariant>>();
    Py::List list;

    for (auto& it : listValue) {
        list.append(Py::Object(_pyObjectFromVariant(it)));
    }

    return list;
}
