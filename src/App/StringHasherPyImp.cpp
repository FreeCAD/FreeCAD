/****************************************************************************
*   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
*                                                                          *
*   This file is part of the FreeCAD CAx development system.               *
*                                                                          *
*   This library is free software; you can redistribute it and/or          *
*   modify it under the terms of the GNU Library General Public            *
*   License as published by the Free Software Foundation; either           *
*   version 2 of the License, or (at your option) any later version.       *
*                                                                          *
*   This library  is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
*   GNU Library General Public License for more details.                   *
*                                                                          *
*   You should have received a copy of the GNU Library General Public      *
*   License along with this library; see the file COPYING.LIB. If not,     *
*   write to the Free Software Foundation, Inc., 59 Temple Place,          *
*   Suite 330, Boston, MA  02111-1307, USA                                 *
*                                                                          *
****************************************************************************/

#include "PreCompiled.h"

#include "StringHasher.h"

#include "StringHasherPy.h"
#include "StringHasherPy.cpp"
#include <Base/PyWrapParseTupleAndKeywords.h>

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string StringHasherPy::representation() const
{
    std::ostringstream str;
    str << "<StringHasher at " << getStringHasherPtr() << ">";
    return str.str();
}

PyObject *StringHasherPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    return new StringHasherPy(new StringHasher);
}

// constructor method
int StringHasherPy::PyInit(PyObject* args, PyObject* kwds)
{
    static const std::array<const char *, 1> kwlist {nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "", kwlist)) {
        return -1;
    }

    return 0;
}


PyObject* StringHasherPy::isSame(PyObject *args)
{
    PyObject *other;
    if (!PyArg_ParseTuple(args, "O!", &StringHasherPy::Type, &other)) {
        return nullptr;
    }

    auto otherHasher = static_cast<StringHasherPy*>(other)->getStringHasherPtr();
    bool same = getStringHasherPtr() == otherHasher;

    return  PyBool_FromLong(same ? 1 : 0);
}

PyObject* StringHasherPy::getID(PyObject *args)
{
    long id;
    int index = 0;
    if (PyArg_ParseTuple(args, "l|i", &id, &index)) {
        if (id > 0) {
            PY_TRY {
                auto sid = getStringHasherPtr()->getID(id, index);
                if (!sid) {
                    Py_Return;
                }

                return sid.getPyObject();
            }
            PY_CATCH;
        }
        else {
            PyErr_SetString(PyExc_ValueError, "Id must be positive integer");
            return nullptr;
        }
    }

    PyErr_Clear();
    PyObject *value = nullptr;
    PyObject *base64 = Py_False;
    if (PyArg_ParseTuple(args, "O!|O!", &PyUnicode_Type, &value, &PyBool_Type, &base64)) {
        PY_TRY {
            std::string txt = PyUnicode_AsUTF8(value);
            QByteArray data;
            StringIDRef sid;
            if (PyObject_IsTrue(base64)) {
                data = QByteArray::fromBase64(QByteArray::fromRawData(txt.c_str(),txt.size()));
                sid = getStringHasherPtr()->getID(data,true);
            }
            else {
                sid = getStringHasherPtr()->getID(txt.c_str(),txt.size());
            }

            return sid.getPyObject();
        }
        PY_CATCH;
    }

    PyErr_SetString(PyExc_TypeError, "Positive integer and optional integer or "
        "string and optional boolean is required");
    return nullptr;
}

Py::Long StringHasherPy::getCount() const
{
    return Py::Long(PyLong_FromSize_t(getStringHasherPtr()->count()), true);
}

Py::Long StringHasherPy::getSize() const
{
    return Py::Long(PyLong_FromSize_t(getStringHasherPtr()->size()), true);
}

Py::Boolean StringHasherPy::getSaveAll() const
{
    return {getStringHasherPtr()->getSaveAll()};
}

void StringHasherPy::setSaveAll(Py::Boolean value)
{
    getStringHasherPtr()->setSaveAll(value);
}

Py::Long StringHasherPy::getThreshold() const
{
    return Py::Long(getStringHasherPtr()->getThreshold());
}

void StringHasherPy::setThreshold(Py::Long value)
{
    getStringHasherPtr()->setThreshold(value);
}

Py::Dict StringHasherPy::getTable() const
{
    Py::Dict dict;
    for (const auto &v : getStringHasherPtr()->getIDMap()) {
        dict.setItem(Py::Long(v.first), Py::String(v.second.dataToText()));
    }

    return dict;
}

PyObject *StringHasherPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int StringHasherPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
