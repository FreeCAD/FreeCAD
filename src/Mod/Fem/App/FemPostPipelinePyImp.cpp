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

#include <Python.h>

#include <Base/FileInfo.h>
#include <Base/UnitPy.h>

#include "FemPostPipeline.h"
#include "FemPostPipelinePy.h"
#include "FemPostPipelinePy.cpp"

#ifdef FC_USE_VTK_PYTHON
# include <vtkPythonUtil.h>
#endif  // BUILD_FEM_VTK


using namespace Fem;

// returns a string which represents the object e.g. when printed in python
std::string FemPostPipelinePy::representation() const
{
    return {"<FemPostPipeline object>"};
}

PyObject* FemPostPipelinePy::read(PyObject* args)
{
    PyObject* files;
    PyObject* values = nullptr;
    PyObject* unitobj = nullptr;
    const char* value_type;

    if (PyArg_ParseTuple(args, "O|OO!s", &files, &values, &(Base::UnitPy::Type), &unitobj, &value_type)) {
        if (!values) {
            // single argument version was called!
            if (!PyUnicode_Check(files)) {
                PyErr_SetString(PyExc_TypeError, "argument must be file path");
                return nullptr;
            }
            const char* path = PyUnicode_AsUTF8(files);
            getFemPostPipelinePtr()->read(Base::FileInfo(path));
            Py_Return;
        }
        else if (values && unitobj) {
            // multistep version!
            if (!(PyTuple_Check(files) || PyList_Check(files))
                || !(PyTuple_Check(values) || PyList_Check(values))) {
                PyErr_SetString(
                    PyExc_TypeError,
                    "Files and values must be list of strings and number respectively"
                );
                return nullptr;
            }

            // extract the result objects
            Py::Sequence file_list(files);
            Py::Sequence::size_type size = file_list.size();
            std::vector<Base::FileInfo> file_result;
            file_result.resize(size);

            for (Py::Sequence::size_type i = 0; i < size; i++) {
                auto path = Py::Object(file_list[i]);
                if (!path.isString()) {
                    PyErr_SetString(PyExc_TypeError, "File path must be string");
                    return nullptr;
                }
                file_result[i] = Base::FileInfo(path.as_string());
            }

            // extract the values
            Py::Sequence values_list(values);
            size = values_list.size();
            std::vector<double> value_result;
            value_result.resize(size);

            for (Py::Sequence::size_type i = 0; i < size; i++) {
                auto value = Py::Object(values_list[i]);
                if (!value.isNumeric()) {
                    PyErr_SetString(PyExc_TypeError, "Values must be numbers");
                    return nullptr;
                }
                value_result[i] = Py::Float(value).as_double();
            }

            // extract the unit
            Base::Unit unit = *(static_cast<Base::UnitPy*>(unitobj)->getUnitPtr());

            // extract the value type
            std::string step_type = std::string(value_type);

            // Finally call the c++ function!
            getFemPostPipelinePtr()->read(file_result, value_result, unit, step_type);
            Py_Return;
        }
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
    PyObject* list = nullptr;
    PyObject* unitobj = nullptr;
    const char* value_type;

    if (PyArg_ParseTuple(args, "O|OO!s", &py, &list, &(Base::UnitPy::Type), &unitobj, &value_type)) {

        if (!list) {

            // single argument version!

            if (!PyObject_TypeCheck(py, &(App::DocumentObjectPy::Type))) {
                PyErr_SetString(PyExc_TypeError, "object is not a result object");
                return nullptr;
            }
            App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(py)->getDocumentObjectPtr();
            if (!obj->isDerivedFrom<FemResultObject>()) {
                PyErr_SetString(PyExc_TypeError, "object is not a result object");
                return nullptr;
            }

            getFemPostPipelinePtr()->load(static_cast<FemResultObject*>(obj));
            Py_Return;
        }
        else if (list && unitobj) {

            // multistep version!

            if (!(PyTuple_Check(py) || PyList_Check(py))
                || !(PyTuple_Check(list) || PyList_Check(list))) {

                std::string error = std::string(
                    "Result and value must be list of ResultObject and number respectively."
                );
                PyErr_SetString(PyExc_TypeError, error.c_str());
                return nullptr;
            }

            // extract the result objects
            Py::Sequence result_list(py);
            Py::Sequence::size_type size = result_list.size();
            std::vector<FemResultObject*> results;
            results.resize(size);

            for (Py::Sequence::size_type i = 0; i < size; i++) {
                Py::Object item = result_list[i];
                if (!PyObject_TypeCheck(*item, &(DocumentObjectPy::Type))) {
                    std::string error = std::string("type in result list must be 'ResultObject', not ");
                    PyErr_SetString(PyExc_TypeError, error.c_str());
                    return nullptr;
                }
                auto obj = static_cast<DocumentObjectPy*>(*item)->getDocumentObjectPtr();
                if (!obj->isDerivedFrom<FemResultObject>()) {
                    std::string error = std::string("type in result list must be 'ResultObject', not ");
                    PyErr_SetString(PyExc_TypeError, error.c_str());
                    return nullptr;
                }
                results[i] = static_cast<FemResultObject*>(obj);
            }

            // extract the values
            Py::Sequence values_list(list);
            size = values_list.size();
            std::vector<double> values;
            values.resize(size);

            for (Py::Sequence::size_type i = 0; i < size; i++) {
                Py::Object value = values_list[i];
                if (!value.isNumeric()) {
                    PyErr_SetString(PyExc_TypeError, "Values must be numbers");
                    return nullptr;
                }
                values[i] = Py::Float(value).as_double();
            }

            // extract the unit
            Base::Unit unit = *(static_cast<Base::UnitPy*>(unitobj)->getUnitPtr());

            // extract the value type
            std::string step_type = std::string(value_type);

            // Finally call the c++ function!
            getFemPostPipelinePtr()->load(results, values, unit, step_type);
            Py_Return;
        }
        else {
            std::string error = std::string(
                "Multistep load requires 4 arguments: ResultList, ValueList, unit, type"
            );
            PyErr_SetString(PyExc_ValueError, error.c_str());
            return nullptr;
        }
    }
    return nullptr;
}


PyObject* FemPostPipelinePy::getFilter(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto filters = getFemPostPipelinePtr()->getFilter();
    Py::List sequence;
    for (auto filter : filters) {
        sequence.append(Py::asObject(filter->getPyObject()));
    }

    return Py::new_reference_to(sequence);
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
    if (!obj->isDerivedFrom<FemPostObject>()) {
        PyErr_SetString(PyExc_TypeError, "object is not a post-processing object");
        return nullptr;
    }

    bool ok = getFemPostPipelinePtr()->holdsPostObject(static_cast<FemPostObject*>(obj));
    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

PyObject* FemPostPipelinePy::renameArrays(PyObject* args)
{
    PyObject* pyObj;
    if (!PyArg_ParseTuple(args, "O!", &(PyDict_Type), &pyObj)) {
        return nullptr;
    }

    Py::Dict pyNames {pyObj};
    std::map<std::string, std::string> names {};
    for (auto&& [key, value] : pyNames) {
        if (!key.isString() || !value.isString()) {
            PyErr_SetString(PyExc_TypeError, "Names must be string objects");
            return nullptr;
        }
        names.emplace(key.as_string(), static_cast<Py::Object>(value).as_string());
    }

    getFemPostPipelinePtr()->renameArrays(names);

    Py_Return;
}

PyObject* FemPostPipelinePy::getOutputAlgorithm([[maybe_unused]] PyObject* args)
{
#ifdef FC_USE_VTK_PYTHON
    // we take no arguments
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    // return python object for the algorithm
    auto algorithm = getFemPostPipelinePtr()->getOutputAlgorithm();
    PyObject* py_algorithm = vtkPythonUtil::GetObjectFromPointer(algorithm);

    return py_algorithm;
#else
    PyErr_SetString(PyExc_NotImplementedError, "VTK python wrapper not available");
    return nullptr;
#endif
}

PyObject* FemPostPipelinePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int FemPostPipelinePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
