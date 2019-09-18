/***************************************************************************
 *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESH_MESHTEXTURE_H
#define MESH_MESHTEXTURE_H

#include <memory>
#include "Core/Algorithm.h"
#include "Core/MeshKernel.h"
#include "Core/KDTree.h"
#include "Mesh.h"


namespace Mesh
{

/*! The MeshTexture class.
  This algorithm is useful to update the material after a mesh has been modified
  by removing points or facets. It can't be used if the coordinates of points have
  changed or if new points have been added.
  @author Werner Mayer
 */
class MeshExport MeshTexture
{
public:
    /*!
      A mesh with material. The number of points or facets must match with the number of colors.
     */
    MeshTexture(const Mesh::MeshObject& mesh, const MeshCore::Material &material);
    /*!
      The \a mesh must be a sub-set of the mesh passed to the constructor. This means
      that points or facets can be removed but neither changed nor new points added.
     */
    void apply(const Mesh::MeshObject& mesh, MeshCore::Material &material);

private:
    const MeshCore::Material &materialRefMesh;
    unsigned long countPointsRefMesh;
    std::unique_ptr<MeshCore::MeshKDTree> kdTree;
    std::unique_ptr<MeshCore::MeshRefPointToFacets> refPnt2Fac;
    MeshCore::MeshIO::Binding binding = MeshCore::MeshIO::OVERALL;
};

} // namespace Mesh

#endif // MESH_MESHTEXTURE_H
