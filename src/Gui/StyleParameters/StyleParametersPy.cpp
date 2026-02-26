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

#include "StyleParametersPy.h"

#include <cstring>

#include <Base/Exception.h>
#include <fmt/format.h>

#include "Application.h"
#include "ColorPy.h"
#include "ParameterManager.h"

namespace Gui::StyleParameters
{

Py::Object valueToPy(const Value& value)
{
    return std::visit(
        [](const auto& alternative) -> Py::Object {
            using T = std::decay_t<decltype(alternative)>;
            if constexpr (std::is_same_v<T, Numeric>) {
                return Py::asObject(new NumericPy(alternative));
            }
            else if constexpr (std::is_same_v<T, Base::Color>) {
                return Py::asObject(new Gui::ColorPy(alternative));
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                return Py::String(alternative);
            }
            else if constexpr (std::is_same_v<T, Tuple>) {
                return Py::asObject(new TuplePy(alternative));
            }
        },
        value
    );
}

// --- NumericPy ---

NumericPy::NumericPy(const Numeric& numeric)
    : _numeric(numeric)
{}

void NumericPy::init_type()
{
    behaviors().name("StyleParameters.Numeric");
    behaviors().doc("Numeric value with optional unit (e.g. 10.0px).");
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportNumberType(Py::PythonType::support_number_float);
    behaviors().readyType();
}

Py::Object NumericPy::repr()
{
    if (_numeric.unit.empty()) {
        return Py::String(fmt::format("{}", _numeric.value));
    }
    return Py::String(fmt::format("{}{}", _numeric.value, _numeric.unit));
}

Py::Object NumericPy::getattr(const char* name)
{
    if (std::strcmp(name, "value") == 0) {
        return Py::Float(_numeric.value);
    }
    if (std::strcmp(name, "unit") == 0) {
        return Py::String(_numeric.unit);
    }
    return getattr_default(name);
}

Py::Object NumericPy::number_float()
{
    return Py::Float(_numeric.value);
}

// --- TuplePy ---

TuplePy::TuplePy(const Tuple& tuple)
    : _tuple(tuple)
{}

void TuplePy::init_type()
{
    behaviors().name("StyleParameters.Tuple");
    behaviors().doc("Tuple of named or unnamed style parameter values.");
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportMappingType(
        Py::PythonType::support_mapping_length | Py::PythonType::support_mapping_subscript
    );
    add_varargs_method(
        "find",
        &TuplePy::find,
        "find(name) -> value or None\nFind a named element, returning None if not found."
    );
    behaviors().readyType();
}

Py::Object TuplePy::repr()
{
    return Py::String(fmt::format("Tuple(kind='{}')", tupleKindName(_tuple.kind)));
}

Py::Object TuplePy::getattr(const char* name)
{
    if (std::strcmp(name, "kind") == 0) {
        return Py::String(tupleKindName(_tuple.kind));
    }
    // Registered methods and Python builtins take priority
    try {
        return getattr_default(name);
    }
    catch (Py::AttributeError&) {
        // Fall through to named element lookup
    }
    if (const Value* found = _tuple.find(name)) {
        return valueToPy(*found);
    }
    throw Py::AttributeError(fmt::format("'Tuple' object has no attribute '{}'", name));
}

PyCxx_ssize_t TuplePy::mapping_length()
{
    return static_cast<PyCxx_ssize_t>(_tuple.size());
}

Py::Object TuplePy::mapping_subscript(const Py::Object& key)
{
    if (PyLong_Check(key.ptr())) {
        const auto index = static_cast<size_t>(Py::Long(key).as_long());
        try {
            return valueToPy(_tuple.at(index));
        }
        catch (const Base::RuntimeError& error) {
            throw Py::IndexError(error.what());
        }
    }
    if (key.isString()) {
        const std::string name = Py::String(key).as_std_string("utf-8");
        if (const Value* found = _tuple.find(name)) {
            return valueToPy(*found);
        }
        throw Py::KeyError(fmt::format("'{}'", name));
    }
    throw Py::TypeError("Tuple indices must be int or str");
}

Py::Object TuplePy::find(const Py::Tuple& args)
{
    if (args.size() != 1 || !args[0].isString()) {
        throw Py::TypeError("find() takes exactly one string argument");
    }
    const std::string name = Py::String(args[0]).as_std_string("utf-8");
    if (const Value* found = _tuple.find(name)) {
        return valueToPy(*found);
    }
    return Py::None();
}

// --- StyleParametersModule ---

StyleParametersModule::StyleParametersModule()
    : Py::ExtensionModule<StyleParametersModule>("StyleParameters")
{
    add_varargs_method(
        "resolve",
        &StyleParametersModule::resolve,
        "resolve(name) -> Numeric | Color | str | Tuple | None\n"
        "Resolve a named style parameter. Returns None if not found."
    );
    add_varargs_method(
        "evaluate",
        &StyleParametersModule::evaluate,
        "evaluate(expression) -> Numeric | Color | str | Tuple\n"
        "Evaluate a style parameter expression."
    );
    initialize("Style parameter resolution module");
}

Py::Object StyleParametersModule::resolve(const Py::Tuple& args)
{
    if (args.size() != 1 || !args[0].isString()) {
        throw Py::TypeError("resolve() takes exactly one string argument");
    }
    const std::string name = Py::String(args[0]).as_std_string("utf-8");
    try {
        const auto result = Application::Instance->styleParameterManager()->resolve(name);
        if (!result) {
            return Py::None();
        }
        return valueToPy(*result);
    }
    catch (const Base::Exception& error) {
        throw Py::RuntimeError(error.what());
    }
}

Py::Object StyleParametersModule::evaluate(const Py::Tuple& args)
{
    if (args.size() != 1 || !args[0].isString()) {
        throw Py::TypeError("evaluate() takes exactly one string argument");
    }
    const std::string expr = Py::String(args[0]).as_std_string("utf-8");
    try {
        return valueToPy(Application::Instance->styleParameterManager()->evaluate(expr));
    }
    catch (const Base::Exception& error) {
        throw Py::RuntimeError(error.what());
    }
}

}  // namespace Gui::StyleParameters
