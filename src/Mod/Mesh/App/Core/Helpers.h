// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2005 Imetric 3D GmbH                                    *
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

#include "Elements.h"


namespace MeshCore
{

/**
 * Helper class for points.
 */
struct MeshExport MeshHelpPoint
{
    inline void Set(FacetIndex ulCorner, FacetIndex ulFacet, const Base::Vector3f& rclPt);

    inline bool operator<(const MeshHelpPoint& rclObj) const;
    inline bool operator==(const MeshHelpPoint& rclObj) const;

    FacetIndex Index() const
    {
        return _ulInd >> 2;
    }

    FacetIndex Corner() const
    {
        return _ulInd & 3;
    }

    MeshPoint _clPt;
    FacetIndex _ulInd {FACET_INDEX_MAX};
};

/**
 * Helper class for list of points.
 */
struct MeshPointBuilder: public std::vector<MeshHelpPoint>
{
    inline void Add(FacetIndex ulCorner, FacetIndex ulFacet, const Base::Vector3f& rclPt);
};

/**
 * Helper class for edges.
 */
struct MeshExport MeshHelpBuilderEdge
{
    FacetIndex Side() const
    {
        return _ulFIndex & 3;
    }

    FacetIndex Index() const
    {
        return _ulFIndex >> 2;
    }

    inline void Set(PointIndex ulInd1, PointIndex ulInd2, FacetIndex ulSide, FacetIndex ulFInd);

    inline bool operator<(const MeshHelpBuilderEdge& rclObj) const;

    inline bool operator==(const MeshHelpBuilderEdge& rclObj) const;


    inline bool operator!=(const MeshHelpBuilderEdge& rclObj) const;

    FacetIndex _ulFIndex;   // facet index
    PointIndex _aulInd[2];  // point index
};

/**
 * Helper class to build up list of edges.
 */
struct MeshEdgeBuilder: public std::vector<MeshHelpBuilderEdge>
{
    using _TIterator = std::vector<MeshHelpBuilderEdge>::iterator;
    inline void Add(PointIndex ulInd1, PointIndex ulInd2, FacetIndex ulSide, FacetIndex ulFInd);
};

inline void MeshHelpPoint::Set(FacetIndex ulCorner, FacetIndex ulFacet, const Base::Vector3f& rclPt)
{
    _ulInd = (ulFacet << 2) | ulCorner;
    _clPt = rclPt;
}

inline bool MeshHelpPoint::operator<(const MeshHelpPoint& rclObj) const
{
    if (_clPt.x == rclObj._clPt.x) {
        if (_clPt.y == rclObj._clPt.y) {
            return _clPt.z < rclObj._clPt.z;
        }
        else {
            return _clPt.y < rclObj._clPt.y;
        }
    }
    else {
        return _clPt.x < rclObj._clPt.x;
    }
}

inline bool MeshHelpPoint::operator==(const MeshHelpPoint& rclObj) const
{
    return Base::DistanceP2(_clPt, rclObj._clPt) < MeshDefinitions::_fMinPointDistanceP2;
}

inline void MeshPointBuilder::Add(FacetIndex ulCorner, FacetIndex ulFacet, const Base::Vector3f& rclPt)
{
    MeshHelpPoint clObj;
    clObj.Set(ulCorner, ulFacet, rclPt);
    push_back(clObj);
}

inline void MeshHelpBuilderEdge::Set(
    PointIndex ulInd1,
    PointIndex ulInd2,
    FacetIndex ulSide,
    FacetIndex ulFInd
)
{
    if (ulInd1 < ulInd2) {
        _aulInd[0] = ulInd1;
        _aulInd[1] = ulInd2;
    }
    else {
        _aulInd[0] = ulInd2;
        _aulInd[1] = ulInd1;
    }
    _ulFIndex = (ulFInd << 2) | ulSide;
}

inline bool MeshHelpBuilderEdge::operator<(const MeshHelpBuilderEdge& rclObj) const
{
    if (_aulInd[0] == rclObj._aulInd[0]) {
        return _aulInd[1] < rclObj._aulInd[1];
    }
    else {
        return _aulInd[0] < rclObj._aulInd[0];
    }
}

inline bool MeshHelpBuilderEdge::operator==(const MeshHelpBuilderEdge& rclObj) const
{
    return (_aulInd[0] == rclObj._aulInd[0]) && (_aulInd[1] == rclObj._aulInd[1]);
}

inline bool MeshHelpBuilderEdge::operator!=(const MeshHelpBuilderEdge& rclObj) const
{
    return (_aulInd[0] != rclObj._aulInd[0]) || (_aulInd[1] != rclObj._aulInd[1]);
}


inline void MeshEdgeBuilder::Add(PointIndex ulInd1, PointIndex ulInd2, FacetIndex ulSide, FacetIndex ulFInd)
{
    MeshHelpBuilderEdge clObj {};
    clObj.Set(ulInd1, ulInd2, ulSide, ulFInd);
    push_back(clObj);
}

}  // namespace MeshCore
