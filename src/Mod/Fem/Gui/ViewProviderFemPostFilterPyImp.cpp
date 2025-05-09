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

// clang-format off
#include <Gui/Control.h>
#include <Gui/PythonWrapper.h>
#include "ViewProviderFemPostFilter.h"
#include "TaskPostBoxes.h"
#ifdef FC_USE_VTK_PYTHON
#include "TaskPostExtraction.h"
#endif
// inclusion of the generated files (generated out of ViewProviderFemPostFilterPy.xml)
#include "ViewProviderFemPostFilterPy.h"
#include "ViewProviderFemPostFilterPy.cpp"
#include <Base/PyWrapParseTupleAndKeywords.h>
// clang-format on


using namespace FemGui;

// returns a string which represents the object e.g. when printed in python
std::string ViewProviderFemPostFilterPy::representation() const
{
    return {"<ViewProviderFemPostFilter object>"};
}

PyObject* ViewProviderFemPostFilterPy::createDisplayTaskWidget(PyObject* args)
{
    // we take no arguments
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto panel = new TaskPostDisplay(getViewProviderFemPostObjectPtr());

    Gui::PythonWrapper wrap;
    if (wrap.loadCoreModule()) {
        return Py::new_reference_to(wrap.fromQWidget(panel));
    }

    PyErr_SetString(PyExc_TypeError, "creating the panel failed");
    return nullptr;
}

PyObject* ViewProviderFemPostFilterPy::createExtractionTaskWidget(PyObject* args)
{
#ifdef FC_USE_VTK_PYTHON
    // we take no arguments
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto panel = new TaskPostExtraction(getViewProviderFemPostObjectPtr());

    Gui::PythonWrapper wrap;
    if (wrap.loadCoreModule()) {
        return Py::new_reference_to(wrap.fromQWidget(panel));
    }

    PyErr_SetString(PyExc_TypeError, "creating the panel failed");
    return nullptr;
#else
    PyErr_SetString(PyExc_NotImplementedError, "VTK python wrapper not available");
    Py_Return;
#endif
}

PyObject* ViewProviderFemPostFilterPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ViewProviderFemPostFilterPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
