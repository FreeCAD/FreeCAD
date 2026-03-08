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

#include <limits>

#include <Standard_Version.hxx>
#include <TopoDS_Shape.hxx>
#if OCC_VERSION_HEX >= 0x070800
# include <functional>
#endif

namespace Part
{

class ShapeMapHasher
{
public:
    size_t operator()(const TopoDS_Shape& theShape) const
    {
#if OCC_VERSION_HEX < 0x070800
        return theShape.HashCode(std::numeric_limits<int>::max());
#else
        return std::hash<TopoDS_Shape> {}(theShape);
#endif
    }
};

}  // namespace Part
