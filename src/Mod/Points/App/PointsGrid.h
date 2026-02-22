// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <limits>
#include <set>

#include <Base/BoundBox.h>
#include <Base/Vector3D.h>

#include "Points.h"

static constexpr int POINTS_CT_GRID = 256;       // Default value for number of elements per grid
static constexpr int POINTS_MAX_GRIDS = 100000;  // Default value for maximum number of grids
static constexpr int POINTS_CT_GRID_PER_AXIS = 20;
static constexpr float PONTSGRID_BBOX_EXTENSION = 10.0F;


namespace Points
{
class PointsGrid;

/**
 * The PointsGrid allows one to divide a global point cloud into smaller regions of elements
 * depending on the resolution of the grid. All grid elements in the grid structure have the same
 * size.
 *
 * Grids can be used within algorithms to avoid to iterate through all elements, so grids can speed
 * up algorithms dramatically.
 * @author Werner Mayer
 */
class PointsExport PointsGrid
{
public:
    /** @name Construction */
    //@{
    /// Construction
    explicit PointsGrid(const PointKernel& rclM);
    /// Construction
    PointsGrid();
    /// Construction
    PointsGrid(const PointKernel& rclM, int iCtGridPerAxis);
    /// Construction
    PointsGrid(const PointKernel& rclM, double fGridLen);
    /// Construction
    PointsGrid(const PointKernel& rclM, unsigned long ulX, unsigned long ulY, unsigned long ulZ);
    PointsGrid(const PointsGrid&) = default;
    PointsGrid(PointsGrid&&) = default;
    /// Destruction
    virtual ~PointsGrid() = default;
    PointsGrid& operator=(const PointsGrid&) = default;
    PointsGrid& operator=(PointsGrid&&) = default;
    //@}

public:
    /** Attaches the point kernel to this grid, an already attached point cloud gets detached. The
     * grid gets rebuilt automatically. */
    virtual void Attach(const PointKernel& rclM);
    /** Rebuilds the grid structure. */
    virtual void Rebuild(
        unsigned long ulPerGrid = POINTS_CT_GRID,
        unsigned long ulMaxGrid = POINTS_MAX_GRIDS
    );
    /** Rebuilds the grid structure. */
    virtual void Rebuild(int iCtGridPerAxis = POINTS_CT_GRID_PER_AXIS);
    /** Rebuilds the grid structure. */
    virtual void Rebuild(unsigned long ulX, unsigned long ulY, unsigned long ulZ);

    /** @name Search */
    //@{
    /** Searches for elements lying in the intersection area of the grid and the bounding box. */
    virtual unsigned long InSide(
        const Base::BoundBox3d& rclBB,
        std::vector<unsigned long>& raulElements,
        bool bDelDoubles = true
    ) const;
    /** Searches for elements lying in the intersection area of the grid and the bounding box. */
    virtual unsigned long InSide(
        const Base::BoundBox3d& rclBB,
        std::set<unsigned long>& raulElementss
    ) const;
    /** Searches for elements lying in the intersection area of the grid and the bounding box. */
    virtual unsigned long InSide(
        const Base::BoundBox3d& rclBB,
        std::vector<unsigned long>& raulElements,
        const Base::Vector3d& rclOrg,
        double fMaxDist,
        bool bDelDoubles = true
    ) const;
    /** Searches for the nearest grids that contain elements from a point, the result are grid
     * indices. */
    void SearchNearestFromPoint(const Base::Vector3d& rclPt, std::set<unsigned long>& rclInd) const;
    //@}

    /** Returns the lengths of the grid elements in x,y and z direction. */
    virtual void GetGridLengths(double& rfLenX, double& rfLenY, double& rfLenZ) const
    {
        rfLenX = _fGridLenX;
        rfLenY = _fGridLenY;
        rfLenZ = _fGridLenZ;
    }
    /** Returns the number of grid elements in x,y and z direction. */
    virtual void GetCtGrids(unsigned long& rulX, unsigned long& rulY, unsigned long& rulZ) const
    {
        rulX = _ulCtGridsX;
        rulY = _ulCtGridsY;
        rulZ = _ulCtGridsZ;
    }

    /** @name Boundings */
    //@{
    /** Returns the bounding box of a given grid element. */
    inline Base::BoundBox3d GetBoundBox(unsigned long ulX, unsigned long ulY, unsigned long ulZ) const;
    /** Returns the bounding box of the whole. */
    inline Base::BoundBox3d GetBoundBox() const;
    //@}
    /** Returns the number of elements in a given grid. */
    unsigned long GetCtElements(unsigned long ulX, unsigned long ulY, unsigned long ulZ) const
    {
        return _aulGrid[ulX][ulY][ulZ].size();
    }
    /** Finds all points that lie in the same grid as the point \a rclPoint. */
    unsigned long FindElements(const Base::Vector3d& rclPoint, std::set<unsigned long>& aulElements) const;
    /** Validates the grid structure and rebuilds it if needed. */
    virtual void Validate(const PointKernel& rclM);
    /** Validates the grid structure and rebuilds it if needed. */
    virtual void Validate();
    /** Verifies the grid structure and returns false if inconsistencies are found. */
    virtual bool Verify() const;
    /** Returns the indices of the grid this point lies in. If the point is outside the grid then
     * the indices of the nearest grid element are taken.*/
    virtual void Position(
        const Base::Vector3d& rclPoint,
        unsigned long& rulX,
        unsigned long& rulY,
        unsigned long& rulZ
    ) const;
    /** Returns the indices of the elements in the given grid. */
    unsigned long GetElements(
        unsigned long ulX,
        unsigned long ulY,
        unsigned long ulZ,
        std::set<unsigned long>& raclInd
    ) const;

protected:
    /** Checks if this is a valid grid position. */
    inline bool CheckPos(unsigned long ulX, unsigned long ulY, unsigned long ulZ) const;
    /** Initializes the size of the internal structure. */
    virtual void InitGrid();
    /** Deletes the grid structure. */
    virtual void Clear();
    /** Calculates the grid length dependent on maximum number of grids. */
    virtual void CalculateGridLength(unsigned long ulCtGrid, unsigned long ulMaxGrids);
    /** Calculates the grid length dependent on the number of grids per axis. */
    virtual void CalculateGridLength(int iCtGridPerAxis);
    /** Rebuilds the grid structure. */
    virtual void RebuildGrid();
    /** Returns the number of stored elements. */
    unsigned long HasElements() const
    {
        return _pclPoints->size();
    }
    /** Get the indices of all elements lying in the grids around a given grid with distance \a
     * ulDistance. */
    void GetHull(
        unsigned long ulX,
        unsigned long ulY,
        unsigned long ulZ,
        unsigned long ulDistance,
        std::set<unsigned long>& raclInd
    ) const;

private:
    std::vector<std::vector<std::vector<std::set<unsigned long>>>> _aulGrid; /**< Grid data
                                                                                structure. */
    const PointKernel* _pclPoints; /**< The point kernel. */
    unsigned long _ulCtElements;   /**< Number of grid elements for validation issues. */
    unsigned long _ulCtGridsX;     /**< Number of grid elements in z. */
    unsigned long _ulCtGridsY;     /**< Number of grid elements in z. */
    unsigned long _ulCtGridsZ;     /**< Number of grid elements in z. */
    double _fGridLenX;             /**< Length of grid elements in x. */
    double _fGridLenY;             /**< Length of grid elements in y. */
    double _fGridLenZ;             /**< Length of grid elements in z. */
    double _fMinX;                 /**< Grid null position in x. */
    double _fMinY;                 /**< Grid null position in y. */
    double _fMinZ;                 /**< Grid null position in z. */

    // friends
    friend class PointsGridIterator;
    friend class PointsGridIteratorStatistic;

public:
protected:
    /** Adds a new point element to the grid structure. \a rclPt is the geometric point and \a
     * ulPtIndex the corresponding index in the point kernel. */
    void AddPoint(const Base::Vector3d& rclPt, unsigned long ulPtIndex, float fEpsilon = 0.0F);
    /** Returns the grid numbers to the given point \a rclPoint. */
    void Pos(
        const Base::Vector3d& rclPoint,
        unsigned long& rulX,
        unsigned long& rulY,
        unsigned long& rulZ
    ) const;
};

/**
 * The PointsGridIterator class provides an interface to walk through
 * all grid elements of a point grid.
 */
class PointsExport PointsGridIterator
{
public:
    /// Construction
    explicit PointsGridIterator(const PointsGrid& rclG);
    /** Returns the bounding box of the current grid element. */
    Base::BoundBox3d GetBoundBox() const
    {
        return _rclGrid.GetBoundBox(_ulX, _ulY, _ulZ);
    }
    /** Returns indices of the elements in the current grid. */
    void GetElements(std::vector<unsigned long>& raulElements) const
    {
        raulElements.insert(
            raulElements.end(),
            _rclGrid._aulGrid[_ulX][_ulY][_ulZ].begin(),
            _rclGrid._aulGrid[_ulX][_ulY][_ulZ].end()
        );
    }
    /** @name Iteration */
    //@{
    /** Sets the iterator to the first element*/
    void Init()
    {
        _ulX = _ulY = _ulZ = 0;
    }
    /** Checks if the iterator has not yet reached the end position. */
    bool More() const
    {
        return (_ulZ < _rclGrid._ulCtGridsZ);
    }
    /** Go to the next grid. */
    void Next()
    {
        if (++_ulX >= (_rclGrid._ulCtGridsX)) {
            _ulX = 0;
        }
        else {
            return;
        }
        if (++_ulY >= (_rclGrid._ulCtGridsY)) {
            _ulY = 0;
            _ulZ++;
        }
        else {
            return;
        }
    }
    //@}

    /** @name Tests with rays */
    //@{
    /** Searches for facets around the ray. */
    bool InitOnRay(
        const Base::Vector3d& rclPt,
        const Base::Vector3d& rclDir,
        std::vector<unsigned long>& raulElements
    );
    /** Searches for facets around the ray. */
    bool InitOnRay(
        const Base::Vector3d& rclPt,
        const Base::Vector3d& rclDir,
        float fMaxSearchArea,
        std::vector<unsigned long>& raulElements
    );
    /** Searches for facets around the ray. */
    bool NextOnRay(std::vector<unsigned long>& raulElements);
    //@}

    /** Returns the grid number of the current position. */
    void GetGridPos(unsigned long& rulX, unsigned long& rulY, unsigned long& rulZ) const
    {
        rulX = _ulX;
        rulY = _ulY;
        rulZ = _ulZ;
    }

private:
    const PointsGrid& _rclGrid; /**< The point grid. */
    unsigned long _ulX {0};     /**< Number of grids in x. */
    unsigned long _ulY {0};     /**< Number of grids in y. */
    unsigned long _ulZ {0};     /**< Number of grids in z. */
    Base::Vector3d _clPt;       /**< Base point of search ray. */
    Base::Vector3d _clDir;      /**< Direction of search ray. */
    bool _bValidRay {false};    /**< Search ray ok? */
    float _fMaxSearchArea {std::numeric_limits<float>::max()};
    /** Checks if a grid position is already visited by NextOnRay(). */
    struct GridElement
    {
        GridElement(unsigned long x, unsigned long y, unsigned long z)
            : x {x}
            , y {y}
            , z {z}
        {}
        bool operator<(const GridElement& pos) const
        {
            if (x == pos.x) {
                if (y == pos.y) {
                    return z < pos.z;
                }
                else {
                    return y < pos.y;
                }
            }
            else {
                return x < pos.x;
            }
        }

    private:
        unsigned long x, y, z;
    };
    std::set<GridElement> _cSearchPositions;
};

// --------------------------------------------------------------

inline Base::BoundBox3d PointsGrid::GetBoundBox(unsigned long ulX, unsigned long ulY, unsigned long ulZ) const
{
    double fX = _fMinX + (double(ulX) * _fGridLenX);
    double fY = _fMinY + (double(ulY) * _fGridLenY);
    double fZ = _fMinZ + (double(ulZ) * _fGridLenZ);

    return Base::BoundBox3d(fX, fY, fZ, fX + _fGridLenX, fY + _fGridLenY, fZ + _fGridLenZ);
}

inline Base::BoundBox3d PointsGrid::GetBoundBox() const
{
    return Base::BoundBox3d(
        _fMinX,
        _fMinY,
        _fMinZ,
        _fMinX + (_fGridLenX * double(_ulCtGridsX)),
        _fMinY + (_fGridLenY * double(_ulCtGridsY)),
        _fMinZ + (_fGridLenZ * double(_ulCtGridsZ))
    );
}

inline bool PointsGrid::CheckPos(unsigned long ulX, unsigned long ulY, unsigned long ulZ) const
{
    return ((ulX < _ulCtGridsX) && (ulY < _ulCtGridsY) && (ulZ < _ulCtGridsZ));
}

// --------------------------------------------------------------

}  // namespace Points
