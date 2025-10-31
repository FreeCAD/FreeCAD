// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/

#include "Type.h"
#include "BindingManager.h"

// generated out of Type.pyi
#include "TypePy.h"
#include "TypePy.cpp"

using namespace Base;

// returns a string which represent the object e.g. when printed in python
std::string TypePy::representation() const
{
    std::stringstream str;
    str << "<class '" << getBaseTypePtr()->getName() << "'>";
    return str.str();
}

PyObject* TypePy::fromName(PyObject* args)
{
    const char* name {};
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    return new TypePy(new Base::Type(Type::fromName(name)));
}

PyObject* TypePy::fromKey(PyObject* args)
{
    unsigned int index {};
    if (!PyArg_ParseTuple(args, "I", &index)) {
        return nullptr;
    }

    return new TypePy(new Base::Type(Type::fromKey(index)));
}

PyObject* TypePy::getNumTypes(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    int num = Type::getNumTypes();
    return PyLong_FromLong(num);
}

PyObject* TypePy::getBadType(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    return new TypePy(new Base::Type(Type::BadType));
}

PyObject* TypePy::getParent(PyObject* args) const
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    return new TypePy(new Base::Type(getBaseTypePtr()->getParent()));
}

PyObject* TypePy::isBad(PyObject* args) const
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    bool val = getBaseTypePtr()->isBad();
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* TypePy::isDerivedFrom(PyObject* args) const
{
    Base::Type type;

    do {
        const char* name {};
        if (PyArg_ParseTuple(args, "s", &name)) {
            type = Type::fromName(name);
            break;
        }

        PyErr_Clear();
        PyObject* py {};
        if (PyArg_ParseTuple(args, "O!", &TypePy::Type, &py)) {
            type = *static_cast<TypePy*>(py)->getBaseTypePtr();
            break;
        }

        PyErr_SetString(PyExc_TypeError, "TypeId or str expected");
        return nullptr;
    } while (false);

    bool val = (!type.isBad() && getBaseTypePtr()->isDerivedFrom(type));
    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* TypePy::getAllDerivedFrom(PyObject* args)
{
    Base::Type type;

    do {
        const char* name {};
        if (PyArg_ParseTuple(args, "s", &name)) {
            type = Type::fromName(name);
            break;
        }

        PyErr_Clear();
        PyObject* py {};
        if (PyArg_ParseTuple(args, "O!", &TypePy::Type, &py)) {
            type = *static_cast<TypePy*>(py)->getBaseTypePtr();
            break;
        }

        PyErr_SetString(PyExc_TypeError, "TypeId or str expected");
        return nullptr;
    } while (false);

    std::vector<Base::Type> ary;
    Type::getAllDerivedFrom(type, ary);
    Py::List res;
    for (const auto& it : ary) {
        res.append(Py::asObject(new TypePy(new Base::Type(it))));
    }
    return Py::new_reference_to(res);
}

PyObject* TypePy::getAllDerived(PyObject* args) const
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Base::Type type = Type::fromName(getBaseTypePtr()->getName());
    std::vector<Base::Type> ary;
    Type::getAllDerivedFrom(type, ary);
    Py::List res;
    for (const auto& it : ary) {
        res.append(Py::asObject(new TypePy(new Base::Type(it))));
    }
    return Py::new_reference_to(res);
}

namespace
{
void deallocPyObject(PyObject* py)
{
    PyObjectBase* pybase = static_cast<PyObjectBase*>(py);
    BaseClass* base = static_cast<BaseClass*>(pybase->getTwinPointer());
    if (BindingManager::instance().retrieveWrapper(base) == py) {
        BindingManager::instance().releaseWrapper(base, py);
        delete base;
    }

    PyObjectBase::PyDestructor(py);
}

PyObject* createPyObject(BaseClass* base)
{
    PyObject* py = base->getPyObject();

    if (PyObject_TypeCheck(py, &PyObjectBase::Type)) {
        // if the Python wrapper is a sub-class of PyObjectBase then
        // check if the C++ object must be added to the list of tracked objects
        PyObjectBase* pybase = static_cast<PyObjectBase*>(py);
        if (base == pybase->getTwinPointer()) {
            // steal a reference because at this point the counter is at 2
            Py_DECREF(py);
            Py_TYPE(py)->tp_dealloc = deallocPyObject;
            BindingManager::instance().registerWrapper(base, py);
        }
        else {
            // The Python wrapper creates its own copy of the C++ object
            delete base;
        }
    }
    else {
        // if the Python wrapper is not a sub-class of PyObjectBase then
        // immediately destroy the C++ object
        delete base;
    }
    return py;
}

}  // namespace

PyObject* TypePy::createInstance(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::String name(getBaseTypePtr()->getName());
    Py::TupleN tuple(name);

    return createInstanceByName(tuple.ptr());
}

PyObject* TypePy::createInstanceByName(PyObject* args)
{
    const char* name {};
    PyObject* load = Py_False;  // NOLINT
    if (!PyArg_ParseTuple(args, "s|O!", &name, &PyBool_Type, &load)) {
        return nullptr;
    }

    bool loadModule = asBoolean(load);
    Base::Type type = Type::getTypeIfDerivedFrom(name, BaseClass::getClassTypeId(), loadModule);
    if (type.isBad()) {
        Py_Return;
    }

    void* typeInstance = type.createInstance();
    if (!typeInstance) {
        Py_Return;
    }

    BaseClass* base = static_cast<BaseClass*>(typeInstance);

    return createPyObject(base);
}

Py::String TypePy::getName() const
{
    return {std::string(getBaseTypePtr()->getName())};
}

Py::Long TypePy::getKey() const
{
    return Py::Long(static_cast<long>(getBaseTypePtr()->getKey()));
}

Py::String TypePy::getModule() const
{
    std::string module(getBaseTypePtr()->getName());
    std::string::size_type pos = module.find_first_of("::");

    if (pos != std::string::npos) {
        module = std::string(module, 0, pos);
    }
    else {
        module.clear();
    }

    return {module};
}

PyObject* TypePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int TypePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
