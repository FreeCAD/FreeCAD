/***************************************************************************
 *   Copyright (c) 2017 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Base/FileInfo.h>

// clang-format off
#include "FemPostPipeline.h"
#include "FemPostPipelinePy.h"
#include "FemPostPipelinePy.cpp"
// clang-format on


using namespace Fem;

// returns a string which represents the object e.g. when printed in python
std::string FemPostPipelinePy::representation() const
{
    return {"<FemPostPipeline object>"};
}

PyObject* FemPostPipelinePy::read(PyObject* args)
{
    char* Name;
    if (PyArg_ParseTuple(args, "et", "utf-8", &Name)) {
        getFemPostPipelinePtr()->read(Base::FileInfo(Name));
        PyMem_Free(Name);
        Py_Return;
    }
    return nullptr;
}

PyObject* FemPostPipelinePy::scale(PyObject* args)
{
    double scale;
    if (PyArg_ParseTuple(args, "d", &scale)) {
        getFemPostPipelinePtr()->scale(scale);
        Py_Return;
    }
    return nullptr;
}

PyObject* FemPostPipelinePy::load(PyObject* args)
{
    PyObject* py;
    if (!PyArg_ParseTuple(args, "O!", &(App::DocumentObjectPy::Type), &py)) {
        return nullptr;
    }

    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(py)->getDocumentObjectPtr();
    if (!obj->getTypeId().isDerivedFrom(FemResultObject::getClassTypeId())) {
        PyErr_SetString(PyExc_TypeError, "object is not a result object");
        return nullptr;
    }

    getFemPostPipelinePtr()->load(static_cast<FemResultObject*>(obj));
    Py_Return;
}

PyObject* FemPostPipelinePy::recomputeChildren(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    getFemPostPipelinePtr()->recomputeChildren();
    Py_Return;
}

PyObject* FemPostPipelinePy::getLastPostObject(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    App::DocumentObject* obj = getFemPostPipelinePtr()->getLastPostObject();
    if (obj) {
        return obj->getPyObject();
    }
    Py_Return;
}

PyObject* FemPostPipelinePy::holdsPostObject(PyObject* args)
{
    PyObject* py;
    if (!PyArg_ParseTuple(args, "O!", &(App::DocumentObjectPy::Type), &py)) {
        return nullptr;
    }

    App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(py)->getDocumentObjectPtr();
    if (!obj->getTypeId().isDerivedFrom(FemPostObject::getClassTypeId())) {
        PyErr_SetString(PyExc_TypeError, "object is not a post-processing object");
        return nullptr;
    }

    bool ok = getFemPostPipelinePtr()->holdsPostObject(static_cast<FemPostObject*>(obj));
    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

PyObject* FemPostPipelinePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int FemPostPipelinePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
