// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD Project Association AISBL
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <algorithm>
#include <array>
#include <cctype>
#include <memory>
#include <sstream>
#include <string>

#include <Base/Interpreter.h>
#include <Base/PlacementPy.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>

#include "EditableDatumLabel.h"

// generated out of EditableDatumLabel.pyi
#include "EditableDatumLabelPy.h"
#include "EditableDatumLabelPy.cpp"

#include "PythonWrapper.h"
#include "View3DInventor.h"
#include "View3DPy.h"
#include "View3DViewerPy.h"

using namespace Gui;

View3DInventorViewer* asViewer(PyObject* pyViewer);

namespace
{

const SbColor defaultEditableDatumLabelColor(1.0F, 0.149F, 0.0F);

std::string normalizeToken(std::string value)
{
    value.erase(
        std::remove_if(
            value.begin(),
            value.end(),
            [](unsigned char ch) { return ch == '_' || ch == '-' || std::isspace(ch); }
        ),
        value.end()
    );
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

EditableDatumLabel* asLabel(EditableDatumLabelPy* self)
{
    if (!self || !self->getEditableDatumLabelPtr()) {
        throw Py::RuntimeError("EditableDatumLabel is deleted");
    }
    return self->getEditableDatumLabelPtr();
}

Base::Placement asPlacement(PyObject* pyPlacement)
{
    if (!PyObject_TypeCheck(pyPlacement, &Base::PlacementPy::Type)) {
        throw Py::TypeError("placement must be a Base.Placement");
    }

    return *static_cast<Base::PlacementPy*>(pyPlacement)->getPlacementPtr();
}

Base::Vector3d asVector(PyObject* pyVector)
{
    if (!PyObject_TypeCheck(pyVector, &Base::VectorPy::Type)) {
        throw Py::TypeError("point must be a Base.Vector");
    }

    return *static_cast<Base::VectorPy*>(pyVector)->getVectorPtr();
}

bool asBool(PyObject* pyObject)
{
    int value = PyObject_IsTrue(pyObject);
    if (value < 0) {
        throw Py::Exception();
    }
    return value == 1;
}

SbColor asColor(PyObject* pyColor)
{
    if (pyColor == Py_None) {
        return defaultEditableDatumLabelColor;
    }

    Py::Sequence sequence(Py::Object(pyColor, false));
    if (sequence.length() != 3) {
        throw Py::TypeError("color must be a sequence of three floats");
    }

    const float r = static_cast<float>(Py::Float(sequence[0]).as_double());
    const float g = static_cast<float>(Py::Float(sequence[1]).as_double());
    const float b = static_cast<float>(Py::Float(sequence[2]).as_double());
    return {r, g, b};
}

SoDatumLabel::Type asLabelType(PyObject* pyType)
{
    std::string labelType = normalizeToken(Py::String(pyType).as_std_string());
    if (labelType == "angle") {
        return SoDatumLabel::ANGLE;
    }
    if (labelType == "distance") {
        return SoDatumLabel::DISTANCE;
    }
    if (labelType == "distancex") {
        return SoDatumLabel::DISTANCEX;
    }
    if (labelType == "distancey") {
        return SoDatumLabel::DISTANCEY;
    }
    if (labelType == "radius") {
        return SoDatumLabel::RADIUS;
    }
    if (labelType == "diameter") {
        return SoDatumLabel::DIAMETER;
    }
    if (labelType == "symmetric") {
        return SoDatumLabel::SYMMETRIC;
    }
    if (labelType == "arclength") {
        return SoDatumLabel::ARCLENGTH;
    }

    throw Py::ValueError("unknown label type");
}

EditableDatumLabel::Function asFunction(PyObject* pyFunction)
{
    std::string function = normalizeToken(Py::String(pyFunction).as_std_string());
    if (function == "positioning") {
        return EditableDatumLabel::Function::Positioning;
    }
    if (function == "dimensioning") {
        return EditableDatumLabel::Function::Dimensioning;
    }
    if (function == "forced") {
        return EditableDatumLabel::Function::Forced;
    }

    throw Py::ValueError("unknown EditableDatumLabel function");
}

PyObject* functionToPyObject(EditableDatumLabel::Function function)
{
    switch (function) {
        case EditableDatumLabel::Function::Positioning:
            return Py::new_reference_to(Py::String("positioning"));
        case EditableDatumLabel::Function::Dimensioning:
            return Py::new_reference_to(Py::String("dimensioning"));
        case EditableDatumLabel::Function::Forced:
            return Py::new_reference_to(Py::String("forced"));
    }

    return Py::new_reference_to(Py::String("positioning"));
}

void invokeCallback(PyObject* callback)
{
    if (!callback) {
        return;
    }

    Base::PyGILStateLocker lock;
    try {
        Py::Callable method(callback);
        method.apply(Py::Tuple());
    }
    catch (const Py::Exception&) {
        PyErr_Print();
    }
}

void invokeCallback(PyObject* callback, double value)
{
    if (!callback) {
        return;
    }

    Base::PyGILStateLocker lock;
    try {
        Py::Callable method(callback);
        Py::Tuple args(1);
        args.setItem(0, Py::Float(value));
        method.apply(args);
    }
    catch (const Py::Exception&) {
        PyErr_Print();
    }
}

void replaceCallback(PyObject*& slot, PyObject* callback)
{
    if (callback == Py_None) {
        callback = nullptr;
    }
    else if (!PyCallable_Check(callback)) {
        throw Py::TypeError("callback must be callable or None");
    }

    Base::PyGILStateLocker lock;
    Py_XINCREF(callback);
    Py_XDECREF(slot);
    slot = callback;
}

}  // namespace

struct EditableDatumLabelPy::CallbackState
{
    PyObject* valueChangedCallback {nullptr};
    PyObject* editingFinishedCallback {nullptr};
    PyObject* editingCanceledCallback {nullptr};
    PyObject* parameterUnsetCallback {nullptr};
    PyObject* finishEditingCallback {nullptr};
};

std::string EditableDatumLabelPy::representation() const
{
    std::stringstream s;
    s << "<EditableDatumLabel at " << getEditableDatumLabelPtr() << ">";
    return s.str();
}

PyObject* EditableDatumLabelPy::PyMake(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
    static const std::array<const char*, 6>
        keywords {"viewer", "placement", "color", "autoDistance", "avoidMouseCursor", nullptr};

    PyObject* pyViewer = nullptr;
    PyObject* pyPlacement = nullptr;
    PyObject* pyColor = Py_None;
    PyObject* pyAutoDistance = Py_False;
    PyObject* pyAvoidMouseCursor = Py_False;

    if (!Base::Wrapped_ParseTupleAndKeywords(
            args,
            kwds,
            "OO|OOO",
            keywords,
            &pyViewer,
            &pyPlacement,
            &pyColor,
            &pyAutoDistance,
            &pyAvoidMouseCursor
        )) {
        return nullptr;
    }

    auto label = std::make_unique<EditableDatumLabel>(
        asViewer(pyViewer),
        asPlacement(pyPlacement),
        asColor(pyColor),
        asBool(pyAutoDistance),
        asBool(pyAvoidMouseCursor)
    );
    auto* wrapper = new EditableDatumLabelPy(label.get(), type);
    label.release();
    return wrapper;
}

int EditableDatumLabelPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

int EditableDatumLabelPy::initialization()
{
    callbackState = new CallbackState;
    auto* label = getEditableDatumLabelPtr();
    QObject::connect(label, &EditableDatumLabel::valueChanged, label, [this](double value) {
        if (this->callbackState) {
            invokeCallback(this->callbackState->valueChangedCallback, value);
        }
    });
    QObject::connect(label, &EditableDatumLabel::editingFinished, label, [this](double value) {
        if (this->callbackState) {
            invokeCallback(this->callbackState->editingFinishedCallback, value);
        }
    });
    QObject::connect(label, &EditableDatumLabel::editingCanceled, label, [this](double value) {
        if (this->callbackState) {
            invokeCallback(this->callbackState->editingCanceledCallback, value);
        }
    });
    QObject::connect(label, &EditableDatumLabel::parameterUnset, label, [this]() {
        if (this->callbackState) {
            invokeCallback(this->callbackState->parameterUnsetCallback);
        }
    });
    QObject::connect(label, &EditableDatumLabel::finishEditingOnAllOVPs, label, [this]() {
        if (this->callbackState) {
            invokeCallback(this->callbackState->finishEditingCallback);
        }
    });
    return 1;
}

int EditableDatumLabelPy::finalization()
{
    if (!callbackState) {
        return 1;
    }

    {
        Base::PyGILStateLocker lock;
        Py_XDECREF(callbackState->valueChangedCallback);
        Py_XDECREF(callbackState->editingFinishedCallback);
        Py_XDECREF(callbackState->editingCanceledCallback);
        Py_XDECREF(callbackState->parameterUnsetCallback);
        Py_XDECREF(callbackState->finishEditingCallback);
    }

    delete callbackState;
    callbackState = nullptr;
    return 1;
}

View3DInventorViewer* asViewer(PyObject* pyViewer)
{
    if (PyObject_TypeCheck(pyViewer, View3DInventorPy::type_object())) {
        auto* viewPy = dynamic_cast<View3DInventorPy*>(Py::getPythonExtensionBase(pyViewer));
        if (!viewPy) {
            throw Py::RuntimeError("Cannot resolve View3DInventor Python wrapper");
        }
        auto* view = viewPy->getView3DInventorPtr();
        if (!view || !view->getViewer()) {
            throw Py::RuntimeError("Cannot use a deleted View3DInventor");
        }
        return view->getViewer();
    }

    if (!PyObject_TypeCheck(pyViewer, View3DInventorViewerPy::type_object())) {
        throw Py::TypeError("viewer must be a View3DInventor or View3DInventorViewer");
    }

    auto* viewerPy = dynamic_cast<View3DInventorViewerPy*>(Py::getPythonExtensionBase(pyViewer));
    if (!viewerPy) {
        throw Py::RuntimeError("Cannot resolve View3DInventorViewer Python wrapper");
    }
    if (!viewerPy->getView3DInventorViewerPtr()) {
        throw Py::RuntimeError("Cannot use a deleted View3DInventorViewer");
    }

    return viewerPy->getView3DInventorViewerPtr();
}

PyObject* EditableDatumLabelPy::activate(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    asLabel(this)->activate();
    Py_Return;
}

PyObject* EditableDatumLabelPy::deactivate(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    asLabel(this)->deactivate();
    Py_Return;
}

PyObject* EditableDatumLabelPy::startEdit(PyObject* args, PyObject* kwds)
{
    static const std::array<const char*, 4> keywords {"value", "eventFilter", "visibleToMouse", nullptr};

    double value = 0.0;
    PyObject* pyFilter = Py_None;
    PyObject* pyVisible = Py_False;
    if (
        !Base::Wrapped_ParseTupleAndKeywords(args, kwds, "d|OO", keywords, &value, &pyFilter, &pyVisible)
    ) {
        return nullptr;
    }

    QObject* filter = nullptr;
    if (pyFilter != Py_None) {
        PythonWrapper wrap;
        wrap.loadWidgetsModule();
        filter = wrap.toQObject(Py::Object(pyFilter, false));
        if (!filter) {
            PyErr_SetString(PyExc_TypeError, "eventFilter must be a QObject or None");
            return nullptr;
        }
    }

    asLabel(this)->startEdit(value, filter, asBool(pyVisible));
    Py_Return;
}

PyObject* EditableDatumLabelPy::stopEdit(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    asLabel(this)->stopEdit();
    Py_Return;
}

PyObject* EditableDatumLabelPy::isActive(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    return Py::new_reference_to(Py::Boolean(asLabel(this)->isActive()));
}

PyObject* EditableDatumLabelPy::isInEdit(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    return Py::new_reference_to(Py::Boolean(asLabel(this)->isInEdit()));
}

PyObject* EditableDatumLabelPy::getValue(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    return Py::new_reference_to(Py::Float(asLabel(this)->getValue()));
}

PyObject* EditableDatumLabelPy::setSpinboxValue(PyObject* args)
{
    double value = 0.0;
    if (!PyArg_ParseTuple(args, "d", &value)) {
        return nullptr;
    }
    asLabel(this)->setSpinboxValue(value);
    Py_Return;
}

PyObject* EditableDatumLabelPy::setPlacement(PyObject* args)
{
    PyObject* pyPlacement = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &Base::PlacementPy::Type, &pyPlacement)) {
        return nullptr;
    }
    asLabel(this)->setPlacement(asPlacement(pyPlacement));
    Py_Return;
}

PyObject* EditableDatumLabelPy::setColor(PyObject* args)
{
    PyObject* pyColor = nullptr;
    if (!PyArg_ParseTuple(args, "O", &pyColor)) {
        return nullptr;
    }
    asLabel(this)->setColor(asColor(pyColor));
    Py_Return;
}

PyObject* EditableDatumLabelPy::setPoints(PyObject* args)
{
    PyObject* pyP1 = nullptr;
    PyObject* pyP2 = nullptr;
    if (!PyArg_ParseTuple(args, "O!O!", &Base::VectorPy::Type, &pyP1, &Base::VectorPy::Type, &pyP2)) {
        return nullptr;
    }
    asLabel(this)->setPoints(asVector(pyP1), asVector(pyP2));
    Py_Return;
}

PyObject* EditableDatumLabelPy::setFocus(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    asLabel(this)->setFocus();
    Py_Return;
}

PyObject* EditableDatumLabelPy::setFocusToSpinbox(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    asLabel(this)->setFocusToSpinbox();
    Py_Return;
}

PyObject* EditableDatumLabelPy::clearSelection(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    asLabel(this)->clearSelection();
    Py_Return;
}

PyObject* EditableDatumLabelPy::setLabelType(PyObject* args)
{
    PyObject* pyType = nullptr;
    PyObject* pyFunction = nullptr;
    if (!PyArg_ParseTuple(args, "O|O", &pyType, &pyFunction)) {
        return nullptr;
    }

    EditableDatumLabel::Function function = EditableDatumLabel::Function::Positioning;
    if (pyFunction) {
        function = asFunction(pyFunction);
    }

    asLabel(this)->setLabelType(asLabelType(pyType), function);
    Py_Return;
}

PyObject* EditableDatumLabelPy::setLabelDistance(PyObject* args)
{
    double value = 0.0;
    if (!PyArg_ParseTuple(args, "d", &value)) {
        return nullptr;
    }
    asLabel(this)->setLabelDistance(value);
    Py_Return;
}

PyObject* EditableDatumLabelPy::setLabelStartAngle(PyObject* args)
{
    double value = 0.0;
    if (!PyArg_ParseTuple(args, "d", &value)) {
        return nullptr;
    }
    asLabel(this)->setLabelStartAngle(value);
    Py_Return;
}

PyObject* EditableDatumLabelPy::setLabelRange(PyObject* args)
{
    double value = 0.0;
    if (!PyArg_ParseTuple(args, "d", &value)) {
        return nullptr;
    }
    asLabel(this)->setLabelRange(value);
    Py_Return;
}

PyObject* EditableDatumLabelPy::setLabelRecommendedDistance(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    asLabel(this)->setLabelRecommendedDistance();
    Py_Return;
}

PyObject* EditableDatumLabelPy::setLabelAutoDistanceReverse(PyObject* args)
{
    PyObject* value = Py_False;
    if (!PyArg_ParseTuple(args, "O", &value)) {
        return nullptr;
    }
    asLabel(this)->setLabelAutoDistanceReverse(asBool(value));
    Py_Return;
}

PyObject* EditableDatumLabelPy::setSpinboxVisibleToMouse(PyObject* args)
{
    PyObject* value = Py_False;
    if (!PyArg_ParseTuple(args, "O", &value)) {
        return nullptr;
    }
    asLabel(this)->setSpinboxVisibleToMouse(asBool(value));
    Py_Return;
}

PyObject* EditableDatumLabelPy::setLockedAppearance(PyObject* args)
{
    PyObject* value = Py_False;
    if (!PyArg_ParseTuple(args, "O", &value)) {
        return nullptr;
    }
    asLabel(this)->setLockedAppearance(asBool(value));
    Py_Return;
}

PyObject* EditableDatumLabelPy::resetLockedState(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    asLabel(this)->resetLockedState();
    Py_Return;
}

PyObject* EditableDatumLabelPy::updateGeometry(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    asLabel(this)->updateGeometry();
    Py_Return;
}

PyObject* EditableDatumLabelPy::getFunction(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    return functionToPyObject(asLabel(this)->getFunction());
}

PyObject* EditableDatumLabelPy::setValueChangedCallback(PyObject* args)
{
    PyObject* callback = Py_None;
    if (!PyArg_ParseTuple(args, "O", &callback)) {
        return nullptr;
    }
    if (!callbackState) {
        callbackState = new CallbackState;
    }
    replaceCallback(callbackState->valueChangedCallback, callback);
    Py_Return;
}

PyObject* EditableDatumLabelPy::setEditingFinishedCallback(PyObject* args)
{
    PyObject* callback = Py_None;
    if (!PyArg_ParseTuple(args, "O", &callback)) {
        return nullptr;
    }
    if (!callbackState) {
        callbackState = new CallbackState;
    }
    replaceCallback(callbackState->editingFinishedCallback, callback);
    Py_Return;
}

PyObject* EditableDatumLabelPy::setEditingCanceledCallback(PyObject* args)
{
    PyObject* callback = Py_None;
    if (!PyArg_ParseTuple(args, "O", &callback)) {
        return nullptr;
    }
    if (!callbackState) {
        callbackState = new CallbackState;
    }
    replaceCallback(callbackState->editingCanceledCallback, callback);
    Py_Return;
}

PyObject* EditableDatumLabelPy::setParameterUnsetCallback(PyObject* args)
{
    PyObject* callback = Py_None;
    if (!PyArg_ParseTuple(args, "O", &callback)) {
        return nullptr;
    }
    if (!callbackState) {
        callbackState = new CallbackState;
    }
    replaceCallback(callbackState->parameterUnsetCallback, callback);
    Py_Return;
}

PyObject* EditableDatumLabelPy::setFinishEditingCallback(PyObject* args)
{
    PyObject* callback = Py_None;
    if (!PyArg_ParseTuple(args, "O", &callback)) {
        return nullptr;
    }
    if (!callbackState) {
        callbackState = new CallbackState;
    }
    replaceCallback(callbackState->finishEditingCallback, callback);
    Py_Return;
}

PyObject* EditableDatumLabelPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int EditableDatumLabelPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
