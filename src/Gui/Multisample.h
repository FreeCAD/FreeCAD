// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#pragma once

#include <map>
#include <vector>
#include <FCGlobal.h>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QSurfaceFormat>

namespace Gui
{

/** @name Anti-Aliasing modes of the rendered 3D scene
 * Specifies Anti-Aliasing (AA) method
 * - Smoothing enables OpenGL line and vertex smoothing (basically deprecated)
 * - MSAA is hardware multi sampling (with 2, 4, 6 or 8 passes), a quite common and efficient AA
 * technique
 */
//@{
enum class AntiAliasing
{
    None = 0,
    MSAA1x = 1,
    MSAA2x = 2,
    MSAA4x = 3,
    MSAA6x = 5,
    MSAA8x = 4
};
//@}

class GuiExport Multisample
{
public:
    Multisample();
    bool testSamples(int num) const;
    std::vector<std::pair<QString, AntiAliasing>> supported() const;
    static int toSamples(AntiAliasing msaa);
    static AntiAliasing toAntiAliasing(int samples);
    static AntiAliasing readMSAAFromSettings();
    static void writeMSAAToSettings(AntiAliasing msaa);

private:
    QSurfaceFormat format;
    QOpenGLContext context;
    QOffscreenSurface offscreen;
};

}  // namespace Gui
