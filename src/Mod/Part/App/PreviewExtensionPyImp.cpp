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

#include "PreviewExtensionPy.h"
#include "PreviewExtensionPy.cpp"

using namespace Part;

std::string PreviewExtensionPy::representation() const
{
    return {"<PreviewExtension>"};
}

PyObject* PreviewExtensionPy::updatePreview(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    try {
        getPreviewExtensionPtr()->updatePreview();
    }
    catch (Base::Exception& exception) {
        exception.setPyException();
        return nullptr;
    }

    Py_RETURN_NONE;
}

PyObject* PreviewExtensionPy::invalidatePreview(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    getPreviewExtensionPtr()->invalidatePreview();

    Py_RETURN_NONE;
}

PyObject* PreviewExtensionPy::isPreviewFresh(PyObject* args) const
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    return Py::new_reference_to(Py::Boolean(getPreviewExtensionPtr()->isPreviewFresh()));
}

PyObject* PreviewExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int PreviewExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
