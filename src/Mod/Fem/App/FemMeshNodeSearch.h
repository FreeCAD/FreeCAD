// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Valentin Pohu                                      *
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

#include <set>

#include <Bnd_Box.hxx>
#include <TopoDS_Shape.hxx>

namespace Fem
{

class FemMesh;

// The mesh nodes on a face or an edge, read on the exact geometry
namespace NodeSearch
{

/* Add cuts a torus patch and a placed parabola short and leaves out the edges and vertices off a
 * curved face : the box of the patch, the parabola points, the edges and vertices hold them
 */
Bnd_Box boxOf(const TopoDS_Shape& shape);

// A vertex or an edge looser than limit on the way takes its own tolerance instead
std::set<int> nodesCloserThan(const FemMesh& mesh, const TopoDS_Shape& shape, double limit);

}  // namespace NodeSearch

}  // namespace Fem
