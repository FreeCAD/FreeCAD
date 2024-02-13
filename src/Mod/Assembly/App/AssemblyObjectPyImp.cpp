/***************************************************************************
 *   Copyright (c) 2014 Jürgen Riegel <juergen.riegel@web.de>              *
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
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

// inclusion of the generated files (generated out of AssemblyObject.xml)
#include "AssemblyObjectPy.h"
#include "AssemblyObjectPy.cpp"

using namespace Assembly;

// returns a string which represents the object e.g. when printed in python
std::string AssemblyObjectPy::representation() const
{
    return {"<Assembly object>"};
}

PyObject* AssemblyObjectPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int AssemblyObjectPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

PyObject* AssemblyObjectPy::solve(PyObject* args)
{
    PyObject* enableUndoPy;
    bool enableUndo;

    if (!PyArg_ParseTuple(args, "O!", &PyBool_Type, &enableUndoPy)) {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args, "")) {
            return nullptr;
        }
        else {
            enableUndo = false;
        }
    }
    else {
        enableUndo = Base::asBoolean(enableUndoPy);
    }

    int ret = this->getAssemblyObjectPtr()->solve(enableUndo);
    return Py_BuildValue("i", ret);
}

PyObject* AssemblyObjectPy::undoSolve(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    this->getAssemblyObjectPtr()->undoSolve();
    Py_Return;
}

PyObject* AssemblyObjectPy::clearUndo(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    this->getAssemblyObjectPtr()->clearUndo();
    Py_Return;
}

PyObject* AssemblyObjectPy::isPartConnected(PyObject* args)
{
    PyObject* pyobj;

    if (!PyArg_ParseTuple(args, "O", &pyobj)) {
        return nullptr;
    }
    auto* obj = static_cast<App::DocumentObjectPy*>(pyobj)->getDocumentObjectPtr();
    bool ok = this->getAssemblyObjectPtr()->isPartConnected(obj);
    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

PyObject* AssemblyObjectPy::exportAsASMT(PyObject* args)
{
    char* utf8Name;
    if (!PyArg_ParseTuple(args, "et", "utf-8", &utf8Name)) {
        return nullptr;
    }

    std::string fileName = utf8Name;
    PyMem_Free(utf8Name);

    if (fileName.empty()) {
        PyErr_SetString(PyExc_ValueError, "Passed string is empty");
        return nullptr;
    }

    this->getAssemblyObjectPtr()->exportAsASMT(fileName);

    Py_Return;
}
