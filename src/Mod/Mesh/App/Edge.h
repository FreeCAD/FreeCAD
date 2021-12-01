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


#ifndef MESH_EDGE_H
#define MESH_EDGE_H

#include <Base/Matrix.h>
#include <Base/Vector3D.h>
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
class MeshExport Edge : public MeshCore::MeshGeomEdge
{
public:
    Edge();
    Edge(const Edge& f);
    ~Edge();

    bool isBound() const {return Index != -1;}
    void unbound();
    void operator = (const Edge& f);

    int Index;
    MeshCore::PointIndex PIndex[2];
    MeshCore::FacetIndex NIndex[2];
    Base::Reference<MeshObject> Mesh;
};

} // namespace Mesh


#endif // MESH_EDGE_H
