// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <sstream>


#include "Edge.h"
#include "Mesh.h"


using namespace Mesh;

Edge::Edge()  // NOLINT
    : Mesh(nullptr)
{
    for (int i = 0; i < 2; i++) {
        PIndex[i] = MeshCore::POINT_INDEX_MAX;
        NIndex[i] = MeshCore::FACET_INDEX_MAX;
    }
}

Edge::Edge(const Edge& e)  // NOLINT
    : MeshCore::MeshGeomEdge(e)
    , Index(e.Index)
    , Mesh(e.Mesh)
{
    for (int i = 0; i < 2; i++) {
        PIndex[i] = e.PIndex[i];
        NIndex[i] = e.NIndex[i];
    }
}

Edge::Edge(Edge&& e)  // NOLINT
    : MeshCore::MeshGeomEdge(e)
    , Index(e.Index)
    , Mesh(e.Mesh)
{
    for (int i = 0; i < 2; i++) {
        PIndex[i] = e.PIndex[i];
        NIndex[i] = e.NIndex[i];
    }
}

Edge::~Edge() = default;

Edge& Edge::operator=(const Edge& e)
{
    Edge c = e;
    *this = std::move(c);
    return *this;
}

Edge& Edge::operator=(Edge&& e)
{
    MeshCore::MeshGeomEdge::operator=(e);
    Mesh = e.Mesh;
    Index = e.Index;
    for (int i = 0; i < 2; i++) {
        PIndex[i] = e.PIndex[i];
        NIndex[i] = e.NIndex[i];
    }

    return *this;
}

void Edge::unbound()
{
    Index = -1;
    Mesh = nullptr;
}
