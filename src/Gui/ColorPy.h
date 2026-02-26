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

#ifndef GUI_COLORPY_H
#define GUI_COLORPY_H

#include <CXX/Extensions.hxx>

#include <Base/Color.h>

namespace Gui
{

/**
 * @brief Python wrapper for Base::Color (r, g, b, a in [0.0, 1.0]).
 *
 * Exposes:
 *  - .r, .g, .b, .a → float
 *  - asQColor()      → PySide2/PySide6 QColor
 *  - asSbColor()     → pivy SbColor (3-component, alpha dropped)
 *  - repr()          → "Color(r=…, g=…, b=…, a=…)"
 */
class GuiExport ColorPy: public Py::PythonExtension<ColorPy>
{
public:
    static void init_type();

    explicit ColorPy(const Base::Color& color);
    ~ColorPy() override = default;

    Py::Object repr() override;
    Py::Object getattr(const char* name) override;

    Py::Object asQColor(const Py::Tuple& args);
    Py::Object asSbColor(const Py::Tuple& args);

private:
    Base::Color _color;
};

}  // namespace Gui

#endif  // GUI_COLORPY_H
