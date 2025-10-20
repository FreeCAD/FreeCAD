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

// inclusion of the generated files (generated out of .pyi file)
#include "ViewProviderShapeExtensionPy.h"
#include "ViewProviderShapeExtensionPy.cpp"

#include <Gui/PythonWrapper.h>


using namespace Gui;

// returns a string which represent the object e.g. when printed in python
std::string ViewProviderShapeExtensionPy::representation() const
{
    return {"<Fem shape extension object>"};
}

PyObject* ViewProviderShapeExtensionPy::createControlWidget(PyObject* args)
{
    // we take no arguments
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    auto ui = getViewProviderShapeExtensionPtr()->createShapeWidget();
    ui->setViewProvider(getViewProviderShapeExtensionPtr()->getExtendedViewProvider());

    Gui::PythonWrapper wrap;
    if (wrap.loadCoreModule()) {
        return Py::new_reference_to(wrap.fromQWidget(ui));
    }

    PyErr_SetString(PyExc_TypeError, "creating the ui element failed");
    return nullptr;
}

PyObject* ViewProviderShapeExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ViewProviderShapeExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
