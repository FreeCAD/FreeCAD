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

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string StringHasherPy::representation(void) const
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
int StringHasherPy::PyInit(PyObject* , PyObject* )
{
    return 0;
}


PyObject* StringHasherPy::isSame(PyObject *args)
{
    PyObject *other;
    if (!PyArg_ParseTuple(args, "O!", &StringHasherPy::Type, &other)){     // convert args: Python->C 
        return Py::new_reference_to(Py::False());
    }
    auto otherHasher = static_cast<StringHasherPy*>(other)->getStringHasherPtr();
    return Py::new_reference_to(Py::Boolean(getStringHasherPtr() == otherHasher));
}

PyObject* StringHasherPy::getID(PyObject *args)
{
    long id = -1;
    int index = 0;
    PyObject *value = 0;
    PyObject *base64 = Py_False;
    if (!PyArg_ParseTuple(args, "l|i",&id,&index)) {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args, "O|O",&value,&base64))
            return NULL;    // NULL triggers exception
    }
    if(id>0) {
        PY_TRY {
            auto sid = getStringHasherPtr()->getID(id, index);
            if(!sid) Py_Return;
            return sid.getPyObject();
        }PY_CATCH;
    }
    std::string txt;
#if PY_MAJOR_VERSION >= 3
    if (PyUnicode_Check(value)) {
        txt = PyUnicode_AsUTF8(value);
    }
#else
    if (PyUnicode_Check(value)) {
        PyObject* unicode = PyUnicode_AsLatin1String(value);
        txt = PyString_AsString(unicode);
        Py_DECREF(unicode);
    }
    else if (PyString_Check(value)) {
        txt = PyString_AsString(value);
    }
#endif
    else 
        throw Py::TypeError("expect argument of type string");
    PY_TRY {
        QByteArray data;
        StringIDRef sid;
        if(PyObject_IsTrue(base64)) {
            data = QByteArray::fromBase64(QByteArray::fromRawData(txt.c_str(),txt.size()));
            sid = getStringHasherPtr()->getID(data,true);
        }else
            sid = getStringHasherPtr()->getID(txt.c_str(),txt.size());
        return sid.getPyObject();
    }PY_CATCH;
}

Py::Int StringHasherPy::getCount(void) const {
    return Py::Int((long)getStringHasherPtr()->count());
}

Py::Int StringHasherPy::getSize(void) const {
    return Py::Int((long)getStringHasherPtr()->size());
}

Py::Boolean StringHasherPy::getSaveAll(void) const {
    return Py::Boolean(getStringHasherPtr()->getSaveAll());
}

void StringHasherPy::setSaveAll(Py::Boolean value) {
    getStringHasherPtr()->setSaveAll(value);
}

Py::Int StringHasherPy::getThreshold(void) const {
    return Py::Int((long)getStringHasherPtr()->getThreshold());
}

void StringHasherPy::setThreshold(Py::Int value) {
    getStringHasherPtr()->setThreshold(value);
}

Py::Dict StringHasherPy::getTable() const {
    Py::Dict dict;
    for(auto &v : getStringHasherPtr()->getIDMap())
        dict.setItem(Py::Int(v.first),Py::String(v.second.dataToText()));
    return dict;
}

PyObject *StringHasherPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int StringHasherPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


