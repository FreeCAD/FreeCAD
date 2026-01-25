// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) Jürgen Riegel <juergen.riegel@web.de>                   *
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

#include <limits>

#include <Base/Vector3D.h>
#include <Base/Handle.h>

#ifndef MESH_GLOBAL_H
# include <Mod/Mesh/MeshGlobal.h>
#endif

using Base::Vector3d;

namespace Mesh
{
// forward declaration
class MeshObject;

/** The MeshPoint helper class
 * The MeshPoint class provides an interface for the MeshPointPy classes for
 * convenient access to the Mesh data structure. This class should not be used for
 * programming algorithms in C++. Use Mesh Core classes instead!
 */
class MeshExport MeshPoint: public Vector3d
{

public:
    /// simple constructor
    explicit MeshPoint(
        const Vector3d& vec = Vector3d(),
        const MeshObject* obj = nullptr,
        unsigned int index = std::numeric_limits<unsigned>::max()
    )
        : Vector3d(vec)
        , Index(index)
        , Mesh(obj)
    {}

    bool isBound() const
    {
        return Index != std::numeric_limits<unsigned>::max();
    }

    unsigned int Index;
    Base::Reference<const MeshObject> Mesh;
};

}  // namespace Mesh
