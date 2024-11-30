/***************************************************************************
 *   Copyright (c) 2017 Victor Titov (DeepSOIC)   <vv.titov@gmail.com>     *
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

#include "ViewProvider.h"

// inclusion of the generated files (generated out of ViewProviderPy.xml)
#include "ViewProviderPy.h"
#include "ViewProviderPy.cpp"

using namespace PartDesignGui;

// returns a string which represent the object e.g. when printed in python
std::string ViewProviderPy::representation() const
{
    return {"<PartDesign::ViewProvider>"};
}

PyObject *ViewProviderPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int ViewProviderPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}

PyObject* ViewProviderPy::setBodyMode(PyObject* args)
{
    PyObject* b_mode;
    if (!PyArg_ParseTuple(args, "O!", &PyBool_Type, &b_mode)) {
        return nullptr;
    }

    PartDesignGui::ViewProvider* base = getViewProviderPtr();
    base->setBodyMode(Base::asBoolean(b_mode));

    return Py::new_reference_to(Py::None());
}

PyObject* ViewProviderPy::makeTemporaryVisible(PyObject* args)
{
    PyObject* b_vis;
    if (!PyArg_ParseTuple(args, "O!", &PyBool_Type, &b_vis)) {
        return nullptr;
    }

    PartDesignGui::ViewProvider* base = getViewProviderPtr();
    base->makeTemporaryVisible(Base::asBoolean(b_vis));

    return Py::new_reference_to(Py::None());
}
