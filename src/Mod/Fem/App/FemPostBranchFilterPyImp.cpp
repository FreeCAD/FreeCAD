/***************************************************************************
 *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
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
#ifndef _PreComp_
#include <Python.h>
#endif

// clang-format off
#include "FemPostBranchFilter.h"
#include "FemPostBranchFilterPy.h"
#include "FemPostBranchFilterPy.cpp"
// clang-format on


using namespace Fem;

// returns a string which represents the object e.g. when printed in python
std::string FemPostBranchFilterPy::representation() const
{
    return {"<FemPostBranchFilter object>"};
}

PyObject* FemPostBranchFilterPy::getFilter(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto filters = getFemPostBranchFilterPtr()->getFilter();
    Py::List sequence;
    for (auto filter : filters) {
        sequence.append(Py::asObject(filter->getPyObject()));
    }

    return Py::new_reference_to(sequence);
}

PyObject* FemPostBranchFilterPy::recomputeChildren(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    getFemPostBranchFilterPtr()->recomputeChildren();
    Py_Return;
}

PyObject* FemPostBranchFilterPy::getLastPostObject(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    App::DocumentObject* obj = getFemPostBranchFilterPtr()->getLastPostObject();
    if (obj) {
        return obj->getPyObject();
    }
    Py_Return;
}

PyObject* FemPostBranchFilterPy::holdsPostObject(PyObject* args)
{
    PyObject* py;
    if (!PyArg_ParseTuple(args, "O!", &(App::DocumentObjectPy::Type), &py)) {
        return nullptr;
    }

    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(py)->getDocumentObjectPtr();
    if (!obj->isDerivedFrom<FemPostObject>()) {
        PyErr_SetString(PyExc_TypeError, "object is not a post-processing object");
        return nullptr;
    }

    bool ok = getFemPostBranchFilterPtr()->holdsPostObject(static_cast<FemPostObject*>(obj));
    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

PyObject* FemPostBranchFilterPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int FemPostBranchFilterPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
