// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#ifndef _PreComp_
#include <Python.h>
#endif

#include "Exception.h"
#include "PythonTools.h"


using namespace Base;

PyObject* PyTools::asUnicodeObject(const char *str)
{
    // Returns a new reference, don't increment it!
    Py_ssize_t len = Py_SAFE_DOWNCAST(strlen(str), size_t, Py_ssize_t);
    PyObject* uni = PyUnicode_DecodeUTF8(str, len, nullptr);
    if (!uni) {
        throw Base::UnicodeError("UTF8 conversion failure at PyTools::AsUnicodeObject()");
    }
    return uni;
}

PyObject* PyTools::asUnicodeObject(const std::string &str)
{
    return asUnicodeObject(str.c_str());
}

const char* PyTools::asUTF8FromUnicode(PyObject* obj)
{
    return PyUnicode_AsUTF8(obj);
}

PyObject* PyTools::runString(const char* str)
{
    PyObject *module, *dict;
    module = PyImport_AddModule("__main__");
    if (!module) {
        return nullptr;
    }

    dict = PyModule_GetDict(module);
    if (!dict) {
        return nullptr;
    }

    return PyRun_String(str, Py_file_input, dict, dict);
}

PyObject* PyTools::runString(const char* str, PyObject* dict)
{
    return PyRun_String(str, Py_file_input, dict, dict);
}
