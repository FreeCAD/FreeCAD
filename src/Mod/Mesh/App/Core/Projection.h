/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESH_PROJECTION_H
#define MESH_PROJECTION_H

#include <vector>

#include <Base/Vector3D.h>
#ifdef FC_USE_OCC
using Base::Vector3f;

class TopoDS_Edge;
class TopoDS_Shape;

namespace MeshCore
{

class MeshFacetGrid;
class MeshKernel;
class MeshGeomFacet;

/// Helper class
struct SplitEdge
{
  unsigned long uE0, uE1; /**< start and endpoint of an edge */
  Base::Vector3f cPt; /**< Point on edge (\a uE0, \a uE1) */
};

/**
 * The MeshProjection class projects a shape onto a mesh.
 * @author Werner Mayer
 */
class MeshExport MeshProjection
{
public:
  /// Construction
  MeshProjection( const MeshKernel& rMesh);
  /// Destruction
  ~MeshProjection();

  /**
   * Searches all edges that intersect with the projected curve \a aShape. Therefore \a aShape must
   * contain shapes of type TopoDS_Edge, other shape types are ignored. A possible solution is
   * taken if the distance between the curve point and the projected point is <= \a fMaxDist.
   */
  void projectToMesh ( const TopoDS_Shape &aShape, float fMaxDist, std::vector<SplitEdge>& rSplitEdges ) const;
  /**
   * Cuts the mesh at the curve defined by \a aShape. This method call @ref projectToMesh() to get the
   * split the facet at the found points. @see projectToMesh() for more details.
   */
  void splitMeshByShape ( const TopoDS_Shape &aShape, float fMaxDist ) const;

protected:
  void projectEdgeToEdge( const TopoDS_Edge &aCurve, float fMaxDist, const MeshFacetGrid& rGrid, 
                          std::vector<SplitEdge>& rSplitEdges ) const;

private:
  const MeshKernel& _rcMesh;
};

} // namespace MeshCore
#endif

#endif  // MESH_PROJECTION_H 
