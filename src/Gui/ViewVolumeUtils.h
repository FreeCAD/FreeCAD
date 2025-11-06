// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_VIEWVOLUMEUTILS

#include <utility>

#include <Inventor/SbMatrix.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/system/gl.h>

namespace Gui::GL {

template <typename ConfigureViewVolume>
inline void loadProjectionMatrix(ConfigureViewVolume&& configure)
{
    SbViewVolume viewVolume;
    std::forward<ConfigureViewVolume>(configure)(viewVolume);
    SbMatrix affine;
    SbMatrix projection;
    viewVolume.getMatrices(affine, projection);
    glLoadMatrixf(reinterpret_cast<const GLfloat*>(projection.getValue()));
}

template <typename T>
inline void loadOrthoMatrix(T left, T right, T bottom, T top, T nearVal, T farVal)
{
    loadProjectionMatrix([&](SbViewVolume& viewVolume) {
        viewVolume.ortho(left, right, bottom, top, nearVal, farVal);
    });
}

template <typename T>
inline void loadFrustumMatrix(T left, T right, T bottom, T top, T nearVal, T farVal)
{
    loadProjectionMatrix([&](SbViewVolume& viewVolume) {
        viewVolume.frustum(left, right, bottom, top, nearVal, farVal);
    });
}

}  // namespace Gui::GL

#endif