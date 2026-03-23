// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <Base/Handle.h>

#include <Mod/Mesh/App/Core/Elements.h>


namespace Mesh
{
// forward declaration
class MeshObject;

/** The Edge helper class
 * The Edge class provides an interface for the EdgePy class for
 * convenient access to the Mesh data structure. This class should not be used
 * for programming algorithms in C++. Use Mesh Core classes instead!
 */
class MeshExport Edge: public MeshCore::MeshGeomEdge
{
public:
    Edge();
    Edge(const Edge& f);
    Edge(Edge&& f);
    ~Edge();

    bool isBound() const
    {
        return Index != -1;
    }
    void unbound();
    Edge& operator=(const Edge& e);
    Edge& operator=(Edge&& e);

    int Index {-1};
    MeshCore::PointIndex PIndex[2];
    MeshCore::FacetIndex NIndex[2];
    Base::Reference<const MeshObject> Mesh;
};

}  // namespace Mesh
