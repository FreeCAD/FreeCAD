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

#include <Base/Interpreter.h>
#include <Mod/Part/App/TopoShapePy.h>

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

Py::Object ViewProviderPreviewExtensionPy::getPreviewRootNode() const
{
    try {
        SoSeparator* node = getViewProviderPreviewExtensionPtr()->getPreviewRootNode();

        // Null until extensionAttach() runs; addExtension() alone does not
        // trigger that, so a preview node can legitimately not exist yet.
        if (!node) {
            throw Py::RuntimeError("Preview extension is not attached");
        }

        PyObject* pointer
            = Base::Interpreter().createSWIGPointerObj("pivy.coin", "_p_SoSeparator", node, 1);
        node->ref();

        return Py::Object(pointer, true);
    }
    catch (const Base::Exception& exception) {
        throw Py::RuntimeError(exception.what());
    }
}

Py::Object ViewProviderPreviewExtensionPy::getPreviewShapeNode() const
{
    try {
        SoPreviewShape* node = getViewProviderPreviewExtensionPtr()->getPreviewShapeNode();

        if (!node) {
            throw Py::RuntimeError("Preview extension is not attached");
        }

        // pivy has no wrapper for the concrete SoPreviewShape type, so it is
        // handed out as its SoSeparator base.
        PyObject* pointer
            = Base::Interpreter().createSWIGPointerObj("pivy.coin", "_p_SoSeparator", node, 1);
        node->ref();

        return Py::Object(pointer, true);
    }
    catch (const Base::Exception& exception) {
        throw Py::RuntimeError(exception.what());
    }
}

PyObject* ViewProviderPreviewExtensionPy::updatePreview(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    try {
        getViewProviderPreviewExtensionPtr()->updatePreview();
    }
    catch (Base::Exception& exception) {
        exception.setPyException();
        return nullptr;
    }

    Py_RETURN_NONE;
}

PyObject* ViewProviderPreviewExtensionPy::updatePreviewShape(PyObject* args)
{
    PyObject* shapeObject {nullptr};
    PyObject* nodeObject {nullptr};

    if (!PyArg_ParseTuple(args, "O!O", &Part::TopoShapePy::Type, &shapeObject, &nodeObject)) {
        return nullptr;
    }

    void* pointer {nullptr};
    try {
        Base::Interpreter().convertSWIGPointerObj("pivy.coin", "_p_SoNode", nodeObject, &pointer, 0);
    }
    catch (const Base::Exception&) {
        PyErr_SetString(PyExc_TypeError, "second argument must be an SoPreviewShape");
        return nullptr;
    }

    auto* node = static_cast<SoNode*>(pointer);

    if (!node || !node->isOfType(SoPreviewShape::getClassTypeId())) {
        PyErr_SetString(PyExc_TypeError, "second argument must be an SoPreviewShape");
        return nullptr;
    }

    const Part::TopoShape shape = *static_cast<Part::TopoShapePy*>(shapeObject)->getTopoShapePtr();

    try {
        getViewProviderPreviewExtensionPtr()->updatePreviewShape(
            shape,
            static_cast<SoPreviewShape*>(node)
        );
    }
    catch (Base::Exception& exception) {
        exception.setPyException();
        return nullptr;
    }

    Py_RETURN_NONE;
}

PyObject* ViewProviderPreviewExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ViewProviderPreviewExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
