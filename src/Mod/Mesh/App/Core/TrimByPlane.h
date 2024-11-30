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

#ifndef MESHTRIM_BY_PLANE_H
#define MESHTRIM_BY_PLANE_H

#include "MeshKernel.h"


namespace MeshCore
{

/**
 * Trim the facets in 3D with a plane
 * \author Werner Mayer
 */
class MeshExport MeshTrimByPlane
{
public:
    explicit MeshTrimByPlane(MeshKernel& mesh);

public:
    /**
     * Checks all facets for intersection with the plane and writes all touched facets into the
     * vector
     */
    void CheckFacets(const MeshFacetGrid& rclGrid,
                     const Base::Vector3f& base,
                     const Base::Vector3f& normal,
                     std::vector<FacetIndex>& trimFacets,
                     std::vector<FacetIndex>& removeFacets) const;

    /**
     * The facets from \a trimFacets will be trimmed or deleted and \a trimmedFacets holds the newly
     * generated facets
     */
    void TrimFacets(const std::vector<FacetIndex>& trimFacets,
                    const Base::Vector3f& base,
                    const Base::Vector3f& normal,
                    std::vector<MeshGeomFacet>& trimmedFacets);

private:
    void CreateOneFacet(const Base::Vector3f& base,
                        const Base::Vector3f& normal,
                        unsigned short shift,
                        const MeshGeomFacet& facet,
                        std::vector<MeshGeomFacet>& trimmedFacets) const;
    void CreateTwoFacet(const Base::Vector3f& base,
                        const Base::Vector3f& normal,
                        unsigned short shift,
                        const MeshGeomFacet& facet,
                        std::vector<MeshGeomFacet>& trimmedFacets) const;

private:
    MeshKernel& myMesh;
};

}  // namespace MeshCore

#endif  // MESHTRIM_BY_PLANE_H
