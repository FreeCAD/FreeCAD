// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Kevin Martin <kpmartin[at]papertrail.ca>           *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <functional>
#include <TopoDS_Shape.hxx>
#include <Standard_TypeDef.hxx>

// This include file is only needed for using OCCT versions before 7.8, and it provides a
// template specialization of std::hash<TopoDS_Shape> to allow use of std container classes.
// OCCT version 7.8 and later do not have TopoDS_Shape::HashCode() so this code must be protected by #if
#if (OCC_VERSION_MAJOR < 7 || (OCC_VERSION_MAJOR == 7 && OCC_VERSION_MINOR < 8))
namespace std
{
template<>
struct hash<TopoDS_Shape>
{
    size_t operator()(const TopoDS_Shape& theShape) const noexcept
    {
        return theShape.HashCode(std::numeric_limits<Standard_Integer>::max());
    }
};
}  // namespace std
#endif
