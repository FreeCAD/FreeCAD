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

#include "PreCompiled.h"

#include "MeshTexture.h"


using namespace Mesh;

MeshTexture::MeshTexture(const Mesh::MeshObject& mesh, const MeshCore::Material& material)
    : materialRefMesh(material)
    , countPointsRefMesh {mesh.countPoints()}
{
    unsigned long countFacets = mesh.countFacets();

    if (material.binding == MeshCore::MeshIO::PER_VERTEX
        && material.diffuseColor.size() == countPointsRefMesh) {
        binding = MeshCore::MeshIO::PER_VERTEX;
        kdTree = std::make_unique<MeshCore::MeshKDTree>(mesh.getKernel().GetPoints());
    }
    else if (material.binding == MeshCore::MeshIO::PER_FACE
             && material.diffuseColor.size() == countFacets) {
        binding = MeshCore::MeshIO::PER_FACE;
        kdTree = std::make_unique<MeshCore::MeshKDTree>(mesh.getKernel().GetPoints());
        refPnt2Fac = std::make_unique<MeshCore::MeshRefPointToFacets>(mesh.getKernel());
    }
}

void MeshTexture::apply(const Mesh::MeshObject& mesh,
                        const App::Color& defaultColor,
                        MeshCore::Material& material)
{
    apply(mesh, true, defaultColor, -1.0f, material);
}

void MeshTexture::apply(const Mesh::MeshObject& mesh,
                        const App::Color& defaultColor,
                        float max_dist,
                        MeshCore::Material& material)
{
    apply(mesh, true, defaultColor, max_dist, material);
}

void MeshTexture::apply(const Mesh::MeshObject& mesh, MeshCore::Material& material)
{
    App::Color defaultColor;
    apply(mesh, false, defaultColor, -1.0f, material);
}

void MeshTexture::apply(const Mesh::MeshObject& mesh, float max_dist, MeshCore::Material& material)
{
    App::Color defaultColor;
    apply(mesh, false, defaultColor, max_dist, material);
}

void MeshTexture::apply(const Mesh::MeshObject& mesh,
                        bool addDefaultColor,
                        const App::Color& defaultColor,
                        float max_dist,
                        MeshCore::Material& material)
{
    // copy the color values because the passed material could be the same instance as
    // 'materialRefMesh'
    std::vector<App::Color> textureColor = materialRefMesh.diffuseColor;
    material.diffuseColor.clear();
    material.binding = MeshCore::MeshIO::OVERALL;

    if (kdTree.get()) {
        // the points of the current mesh
        std::vector<App::Color> diffuseColor;
        const MeshCore::MeshPointArray& points = mesh.getKernel().GetPoints();
        const MeshCore::MeshFacetArray& facets = mesh.getKernel().GetFacets();

        if (binding == MeshCore::MeshIO::PER_VERTEX) {
            diffuseColor.reserve(points.size());
            for (size_t index = 0; index < points.size(); index++) {
                PointIndex pos = findIndex(points[index], max_dist);
                if (pos < countPointsRefMesh) {
                    diffuseColor.push_back(textureColor[pos]);
                }
                else if (addDefaultColor) {
                    diffuseColor.push_back(defaultColor);
                }
            }

            if (diffuseColor.size() == points.size()) {
                material.diffuseColor.swap(diffuseColor);
                material.binding = MeshCore::MeshIO::PER_VERTEX;
            }
        }
        else if (binding == MeshCore::MeshIO::PER_FACE) {
            // the values of the map give the point indices of the original mesh
            std::vector<PointIndex> pointMap;
            pointMap.reserve(points.size());
            for (size_t index = 0; index < points.size(); index++) {
                PointIndex pos = findIndex(points[index], max_dist);
                if (pos < countPointsRefMesh) {
                    pointMap.push_back(pos);
                }
                else if (addDefaultColor) {
                    pointMap.push_back(MeshCore::POINT_INDEX_MAX);
                }
            }

            // now determine the facet indices of the original mesh
            if (pointMap.size() == points.size()) {
                diffuseColor.reserve(facets.size());
                for (const auto& it : facets) {
                    PointIndex index1 = pointMap[it._aulPoints[0]];
                    PointIndex index2 = pointMap[it._aulPoints[1]];
                    PointIndex index3 = pointMap[it._aulPoints[2]];
                    if (index1 != MeshCore::POINT_INDEX_MAX && index2 != MeshCore::POINT_INDEX_MAX
                        && index3 != MeshCore::POINT_INDEX_MAX) {
                        std::vector<FacetIndex> found =
                            refPnt2Fac->GetIndices(index1, index2, index3);
                        if (found.size() == 1) {
                            diffuseColor.push_back(textureColor[found.front()]);
                        }
                    }
                    else if (addDefaultColor) {
                        diffuseColor.push_back(defaultColor);
                    }
                }
            }

            if (diffuseColor.size() == facets.size()) {
                material.diffuseColor.swap(diffuseColor);
                material.binding = MeshCore::MeshIO::PER_FACE;
            }
        }
    }
}
