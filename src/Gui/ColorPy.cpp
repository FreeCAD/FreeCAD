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

#include "ColorPy.h"

#include <cstring>

#include <fmt/format.h>

namespace Gui
{

ColorPy::ColorPy(const Base::Color& color)
    : _color(color)
{}

void ColorPy::init_type()
{
    behaviors().name("Gui.Color");
    behaviors().doc("RGBA color with components in [0.0, 1.0].");
    behaviors().supportRepr();
    behaviors().supportGetattr();
    add_varargs_method("asQColor", &ColorPy::asQColor, "asQColor() -> QColor\nReturns a PySide QColor.");
    add_varargs_method(
        "asSbColor",
        &ColorPy::asSbColor,
        "asSbColor() -> SbColor\nReturns a pivy SbColor (3-component, alpha dropped)."
    );
    behaviors().readyType();
}

Py::Object ColorPy::repr()
{
    return Py::String(
        fmt::format("Color(r={}, g={}, b={}, a={})", _color.r, _color.g, _color.b, _color.a)
    );
}

Py::Object ColorPy::getattr(const char* name)
{
    if (std::strcmp(name, "r") == 0) {
        return Py::Float(_color.r);
    }
    if (std::strcmp(name, "g") == 0) {
        return Py::Float(_color.g);
    }
    if (std::strcmp(name, "b") == 0) {
        return Py::Float(_color.b);
    }
    if (std::strcmp(name, "a") == 0) {
        return Py::Float(_color.a);
    }
    return getattr_default(name);
}

Py::Object ColorPy::asQColor(const Py::Tuple& /*args*/)
{
    // Try PySide6 first, fall back to PySide2
    PyObject* qtGuiModule = PyImport_ImportModule("PySide6.QtGui");
    if (!qtGuiModule) {
        PyErr_Clear();
        qtGuiModule = PyImport_ImportModule("PySide2.QtGui");
    }
    if (!qtGuiModule) {
        throw Py::ImportError("Neither PySide6 nor PySide2 is available");
    }
    Py::Module qtGui(qtGuiModule, true);
    Py::Callable fromRgbF(qtGui.getAttr("QColor").getAttr("fromRgbF"));
    return fromRgbF.apply(
        Py::TupleN(Py::Float(_color.r), Py::Float(_color.g), Py::Float(_color.b), Py::Float(_color.a))
    );
}

Py::Object ColorPy::asSbColor(const Py::Tuple& /*args*/)
{
    PyObject* coinModule = PyImport_ImportModule("pivy.coin");
    if (!coinModule) {
        PyErr_Clear();
        throw Py::ImportError("pivy is not available; install it with: pip install pivy");
    }
    Py::Module coin(coinModule, true);
    Py::Callable sbColorClass(coin.getAttr("SbColor"));
    return sbColorClass.apply(
        Py::TupleN(Py::Float(_color.r), Py::Float(_color.g), Py::Float(_color.b))
    );
}

}  // namespace Gui
