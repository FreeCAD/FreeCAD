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

#ifndef STYLEPARAMETERS_PY_H
#define STYLEPARAMETERS_PY_H

#include <CXX/Extensions.hxx>

#include "Value.h"

namespace Gui::StyleParameters
{

/**
 * @brief Converts a StyleParameters Value to a Python object.
 */
Py::Object valueToPy(const Value& value);

/**
 * @brief Python wrapper for Numeric (value + unit).
 *
 * Exposes:
 *  - .value  → float
 *  - .unit   → str
 *  - float() → .value (number protocol)
 *  - repr()  → "10.0px" or "10.0"
 */
class NumericPy: public Py::PythonExtension<NumericPy>
{
public:
    static void init_type();

    explicit NumericPy(const Numeric& numeric);
    ~NumericPy() override = default;

    Py::Object repr() override;
    Py::Object getattr(const char* name) override;
    Py::Object number_float() override;

private:
    Numeric _numeric;
};

/**
 * @brief Python wrapper for Tuple (kind + named/unnamed elements).
 *
 * Generic access:
 *  - .kind          → str
 *  - len(t)         → element count
 *  - t[int]         → element by index (IndexError on OOB)
 *  - t[str]         → element by name (KeyError if not found)
 *  - t.find(name)   → element by name or None
 *  - t.attr         → named element lookup via __getattr__ (AttributeError if not found)
 *
 * Concrete shape conversions (via fit() module function):
 *  - fit(tuple, "Padding") etc. returns a new Tuple with expanded named elements,
 *    after which .top, .right, .bottom, .left work via __getattr__
 */
class TuplePy: public Py::PythonExtension<TuplePy>
{
public:
    static void init_type();

    explicit TuplePy(const Tuple& tuple);
    ~TuplePy() override = default;

    Py::Object repr() override;
    Py::Object getattr(const char* name) override;

    // Mapping protocol
    PyCxx_ssize_t mapping_length() override;
    Py::Object mapping_subscript(const Py::Object& key) override;

    Py::Object find(const Py::Tuple& args);

private:
    Tuple _tuple;
};

/**
 * @brief PyCXX extension module for FreeCADGui.StyleParameters.
 *
 * Exposes:
 *  - resolve(name) → Numeric | Color | str | Tuple | None
 *  - evaluate(expr) → Numeric | Color | str | Tuple
 */
class StyleParametersModule: public Py::ExtensionModule<StyleParametersModule>
{
public:
    StyleParametersModule();
    ~StyleParametersModule() override = default;

    Py::Object resolve(const Py::Tuple& args);
    Py::Object evaluate(const Py::Tuple& args);
};

}  // namespace Gui::StyleParameters

#endif  // STYLEPARAMETERS_PY_H
