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

#ifndef GUI_CAMERA_H
#define GUI_CAMERA_H

#include <Inventor/SbRotation.h>
#include <Base/Rotation.h>
#include <FCGlobal.h>

namespace Gui {

class GuiExport Camera
{
public:
    enum Orientation {
        Top,
        Bottom,
        Front,
        Rear,
        Right,
        Left,
        Isometric,
        Dimetric,
        Trimetric,
    };

    static SbRotation top();
    static SbRotation bottom();
    static SbRotation front();
    static SbRotation rear();
    static SbRotation right();
    static SbRotation left();
    static SbRotation isometric();
    static SbRotation dimetric();
    static SbRotation trimetric();

    static SbRotation rotation(Orientation view);
    static Base::Rotation convert(Orientation view);
    static Base::Rotation convert(const SbRotation&);
    static SbRotation convert(const Base::Rotation&);
};

}

#endif // GUI_CAMERA_H
