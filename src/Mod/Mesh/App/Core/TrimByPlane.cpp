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
#ifndef _PreComp_
#include <algorithm>
#endif

#include "Grid.h"
#include "Iterator.h"
#include "TrimByPlane.h"


using namespace MeshCore;

MeshTrimByPlane::MeshTrimByPlane(MeshKernel& rclM)
    : myMesh(rclM)
{}

void MeshTrimByPlane::CheckFacets(const MeshFacetGrid& rclGrid,
                                  const Base::Vector3f& base,
                                  const Base::Vector3f& normal,
                                  std::vector<FacetIndex>& trimFacets,
                                  std::vector<FacetIndex>& removeFacets) const
{
    // Go through the grid and check for each cell if its bounding box intersects the plane.
    // If the box is completely below the plane all facets will be kept, if it's above the
    // plane all triangles will be removed.
    std::vector<FacetIndex> checkElements;
    MeshGridIterator clGridIter(rclGrid);
    for (clGridIter.Init(); clGridIter.More(); clGridIter.Next()) {
        Base::BoundBox3f clBBox3d = clGridIter.GetBoundBox();
        if (clBBox3d.IsCutPlane(base, normal)) {
            // save all elements in checkElements
            clGridIter.GetElements(checkElements);
        }
        else if (clBBox3d.CalcPoint(Base::BoundBox3f::TLB).DistanceToPlane(base, normal) > 0.0f) {
            // save all elements in removeFacets
            clGridIter.GetElements(removeFacets);
        }
    }

    // remove double elements
    std::sort(checkElements.begin(), checkElements.end());
    checkElements.erase(std::unique(checkElements.begin(), checkElements.end()),
                        checkElements.end());

    trimFacets.reserve(checkElements.size() / 2);  // reserve some memory
    for (FacetIndex element : checkElements) {
        MeshGeomFacet clFacet = myMesh.GetFacet(element);
        if (clFacet.IntersectWithPlane(base, normal)) {
            trimFacets.push_back(element);
            removeFacets.push_back(element);
        }
        else if (clFacet._aclPoints[0].DistanceToPlane(base, normal) > 0.0f) {
            removeFacets.push_back(element);
        }
    }

    // remove double elements
    std::sort(removeFacets.begin(), removeFacets.end());
    removeFacets.erase(std::unique(removeFacets.begin(), removeFacets.end()), removeFacets.end());
}

void MeshTrimByPlane::CreateOneFacet(const Base::Vector3f& base,
                                     const Base::Vector3f& normal,
                                     unsigned short shift,
                                     const MeshGeomFacet& facet,
                                     std::vector<MeshGeomFacet>& trimmedFacets) const
{
    unsigned short nul = shift % 3;
    unsigned short one = (shift + 1) % 3;
    unsigned short two = (shift + 2) % 3;

    Base::Vector3f p1, p2;
    MeshGeomEdge edge;

    edge._aclPoints[0] = facet._aclPoints[nul];
    edge._aclPoints[1] = facet._aclPoints[one];
    edge.IntersectWithPlane(base, normal, p1);

    edge._aclPoints[0] = facet._aclPoints[nul];
    edge._aclPoints[1] = facet._aclPoints[two];
    edge.IntersectWithPlane(base, normal, p2);

    MeshGeomFacet create;
    create._aclPoints[0] = facet._aclPoints[nul];
    create._aclPoints[1] = p1;
    create._aclPoints[2] = p2;
    trimmedFacets.push_back(create);
}

void MeshTrimByPlane::CreateTwoFacet(const Base::Vector3f& base,
                                     const Base::Vector3f& normal,
                                     unsigned short shift,
                                     const MeshGeomFacet& facet,
                                     std::vector<MeshGeomFacet>& trimmedFacets) const
{
    unsigned short nul = shift % 3;
    unsigned short one = (shift + 1) % 3;
    unsigned short two = (shift + 2) % 3;

    Base::Vector3f p1, p2;
    MeshGeomEdge edge;

    edge._aclPoints[0] = facet._aclPoints[nul];
    edge._aclPoints[1] = facet._aclPoints[two];
    edge.IntersectWithPlane(base, normal, p1);

    edge._aclPoints[0] = facet._aclPoints[one];
    edge._aclPoints[1] = facet._aclPoints[two];
    edge.IntersectWithPlane(base, normal, p2);

    MeshGeomFacet create;
    create._aclPoints[0] = facet._aclPoints[nul];
    create._aclPoints[1] = facet._aclPoints[one];
    create._aclPoints[2] = p1;
    trimmedFacets.push_back(create);

    create._aclPoints[0] = facet._aclPoints[one];
    create._aclPoints[1] = p2;
    create._aclPoints[2] = p1;
    trimmedFacets.push_back(create);
}

void MeshTrimByPlane::TrimFacets(const std::vector<FacetIndex>& trimFacets,
                                 const Base::Vector3f& base,
                                 const Base::Vector3f& normal,
                                 std::vector<MeshGeomFacet>& trimmedFacets)
{
    trimmedFacets.reserve(2 * trimFacets.size());
    for (FacetIndex index : trimFacets) {
        MeshGeomFacet facet = myMesh.GetFacet(index);
        float dist1 = facet._aclPoints[0].DistanceToPlane(base, normal);
        float dist2 = facet._aclPoints[1].DistanceToPlane(base, normal);
        float dist3 = facet._aclPoints[2].DistanceToPlane(base, normal);

        // only one point below
        if (dist1 < 0.0f && dist2 > 0.0f && dist3 > 0.0f) {
            CreateOneFacet(base, normal, 0, facet, trimmedFacets);
        }
        else if (dist1 > 0.0f && dist2 < 0.0f && dist3 > 0.0f) {
            CreateOneFacet(base, normal, 1, facet, trimmedFacets);
        }
        else if (dist1 > 0.0f && dist2 > 0.0f && dist3 < 0.0f) {
            CreateOneFacet(base, normal, 2, facet, trimmedFacets);
        }
        // two points below
        else if (dist1 < 0.0f && dist2 < 0.0f && dist3 > 0.0f) {
            CreateTwoFacet(base, normal, 0, facet, trimmedFacets);
        }
        else if (dist1 > 0.0f && dist2 < 0.0f && dist3 < 0.0f) {
            CreateTwoFacet(base, normal, 1, facet, trimmedFacets);
        }
        else if (dist1 < 0.0f && dist2 > 0.0f && dist3 < 0.0f) {
            CreateTwoFacet(base, normal, 2, facet, trimmedFacets);
        }
    }
}
