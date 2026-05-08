// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "ViewProviderPreviewExtensionPy.h"
#include "ViewProviderPreviewExtensionPy.cpp"

using namespace PartGui;

std::string ViewProviderPreviewExtensionPy::representation() const
{
    return {"<ViewProviderPreviewExtension>"};
}

PyObject* ViewProviderPreviewExtensionPy::showPreview(PyObject* args)
{
    int enable {};
    if (!PyArg_ParseTuple(args, "p", &enable)) {
        return nullptr;
    }

    try {
        getViewProviderPreviewExtensionPtr()->showPreview(enable != 0);
    }
    catch (Base::Exception& exception) {
        exception.setPyException();
        return nullptr;
    }

    Py_RETURN_NONE;
}

PyObject* ViewProviderPreviewExtensionPy::isPreviewEnabled(PyObject* args) const
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    return Py::new_reference_to(Py::Boolean(getViewProviderPreviewExtensionPtr()->isPreviewEnabled()));
}

PyObject* ViewProviderPreviewExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ViewProviderPreviewExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
