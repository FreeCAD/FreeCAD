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

#include <array>
#include <limits>
#include "Matrix.h"
#include "Tools2D.h"
#include "ViewProj.h"


namespace Base
{

class ViewProjMethod;

/** The 3D bounding box class. */
template<class Precision>
class BoundBox3
{
    // helper function
    static bool isOnRayW(Precision min, Precision max, Precision val);
    static bool isOnRayS(Precision min, Precision max, Precision val);

public:
    using num_type = Precision;
    using traits_type = float_traits<num_type>;

    /**  Public attributes */
    //@{
    Precision MinX;
    Precision MinY;
    Precision MinZ;
    Precision MaxX;
    Precision MaxY;
    Precision MaxZ;
    //@}

    /** Builds box from pairs of x,y,z values. */
    inline explicit BoundBox3(
        Precision fMinX = std::numeric_limits<Precision>::max(),
        Precision fMinY = std::numeric_limits<Precision>::max(),
        Precision fMinZ = std::numeric_limits<Precision>::max(),
        Precision fMaxX = -std::numeric_limits<Precision>::max(),
        Precision fMaxY = -std::numeric_limits<Precision>::max(),
        Precision fMaxZ = -std::numeric_limits<Precision>::max()
    );
    BoundBox3(const BoundBox3<Precision>& rcBB) = default;
    BoundBox3(BoundBox3<Precision>&& rcBB) noexcept = default;
    /** Builds box from an array of points. */
    inline BoundBox3(const Vector3<Precision>* pclVect, std::size_t ulCt);

    /** Defines a bounding box around the center \a rcCnt with the
     * distances \a fDistance in each coordinate.
     */
    BoundBox3(const Vector3<Precision>& point, Precision distance);
    ~BoundBox3();

    /// Assignment operator
    inline BoundBox3<Precision>& operator=(const BoundBox3<Precision>& rcBound) = default;
    inline BoundBox3<Precision>& operator=(BoundBox3<Precision>&& rcBound) noexcept = default;

    /** Methods for intersection, cuttíng and union of bounding boxes */
    //@{
    /** Checks for intersection. */
    inline bool Intersect(const BoundBox3<Precision>& rcBB) const;
    /** Checks for intersection. */
    inline bool operator&&(const BoundBox3<Precision>& rcBB) const;
    /** Checks for intersection. */
    inline bool Intersect(const BoundBox2d& rcBB) const;
    /** Checks for intersection. */
    inline bool operator&&(const BoundBox2d& rcBB) const;
    /** Computes the intersection between two bounding boxes.
     * The result is also a bounding box.
     */
    BoundBox3<Precision> Intersected(const BoundBox3<Precision>& rcBB) const;
    /** The union of two bounding boxes. */
    BoundBox3<Precision> United(const BoundBox3<Precision>& rcBB) const;
    /** Appends the point to the box. The box can grow but not shrink. */
    inline void Add(const Vector3<Precision>& rclVect);
    /** Appends the bounding box to this box. The box can grow but not shrink. */
    inline void Add(const BoundBox3<Precision>& rcBB);
    //@}

    /** Test methods */
    //@{
    /** Checks if this point lies inside the box.
     * @note It's up to the client programmer to make sure that this bounding box is valid.
     */
    inline bool IsInBox(const Vector3<Precision>& rcVct) const;
    /** Checks if this 3D box lies inside the box.
     * @note It's up to the client programmer to make sure that both bounding boxes are valid.
     */
    inline bool IsInBox(const BoundBox3<Precision>& rcBB) const;
    /** Checks if this 2D box lies inside the box.
     * @note It's up to the client programmer to make sure that both bounding boxes are valid.
     */
    inline bool IsInBox(const BoundBox2d& rcbb) const;
    /** Checks whether the bounding box is valid. */
    bool IsValid() const;
    //@}

    enum OCTANT
    {
        OCT_LDB = 0,
        OCT_RDB,
        OCT_LUB,
        OCT_RUB,
        OCT_LDF,
        OCT_RDF,
        OCT_LUF,
        OCT_RUF
    };
    bool GetOctantFromVector(const Vector3<Precision>& rclVct, OCTANT& rclOctant) const;
    BoundBox3<Precision> CalcOctant(typename BoundBox3<Precision>::OCTANT Octant) const;

    enum SIDE
    {
        LEFT = 0,
        RIGHT = 1,
        TOP = 2,
        BOTTOM = 3,
        FRONT = 4,
        BACK = 5,
        INVALID = 255
    };
    enum CORNER
    {
        TLB = 0,  // top-left-back
        TLF = 1,  // top-left-front
        TRF = 2,  // top-right-front
        TRB = 3,  // top-right-back
        BLB = 4,  // bottom-left-back
        BLF = 5,  // bottom-left-front
        BRF = 6,  // bottom-right-front
        BRB = 7,  // bottom-right-back
    };
    enum EDGE
    {
        TLB_TLF = 0,
        TLF_TRF = 1,
        TRF_TRB = 2,
        TRB_TLB = 3,
        BLB_BLF = 4,
        BLF_BRF = 5,
        BRF_BRB = 6,
        BRB_BLB = 7,
        TLB_BLB = 8,
        TLF_BLF = 9,
        TRF_BRF = 10,
        TRB_BRB = 11
    };

    /**
     * Returns the corner point \a usPoint.
     */
    inline Vector3<Precision> CalcPoint(unsigned short usPoint) const;
    /** Returns the plane of the given side. */
    void CalcPlane(unsigned short usPlane, Vector3<Precision>& rBase, Vector3<Precision>& rNormal) const;
    /** Calculates the two points of an edge.
     */
    bool CalcEdge(unsigned short usEdge, Vector3<Precision>& rcP0, Vector3<Precision>& rcP1) const;
    /** Intersection point of an inner search ray with the bounding box, built of
     * the base \a rcVct and the direction \a rcVctDir. \a rcVct must lie inside the
     * bounding box.
     */
    bool IntersectionPoint(
        const Vector3<Precision>& rcVct,
        const Vector3<Precision>& rcVctDir,
        Vector3<Precision>& cVctRes,
        Precision epsilon
    ) const;
    /** Checks for intersection with line incl. search tolerance. */
    bool IsCutLine(
        const Vector3<Precision>& rcBase,
        const Vector3<Precision>& rcDir,
        Precision fTolerance = 0.0F
    ) const;
    /** Checks if this plane specified by (point,normal) cuts this box.
     * @note It's up to the client programmer to make sure that this bounding box is valid.
     */
    inline bool IsCutPlane(const Vector3<Precision>& rclBase, const Vector3<Precision>& rclNormal) const;
    /** Computes the intersection points of line and bounding box. */
    bool IntersectWithLine(
        const Vector3<Precision>& rcBase,
        const Vector3<Precision>& rcDir,
        Vector3<Precision>& rcP0,
        Vector3<Precision>& rcP1
    ) const;
    /** Computes the intersection point of line and a plane of the bounding box. */
    bool IntersectPlaneWithLine(
        unsigned short usSide,
        const Vector3<Precision>& rcBase,
        const Vector3<Precision>& rcDir,
        Vector3<Precision>& rcP0
    ) const;
    /** Returns the side of the bounding box the ray exits. */
    typename BoundBox3<Precision>::SIDE GetSideFromRay(
        const Vector3<Precision>& rclPt,
        const Vector3<Precision>& rclDir
    ) const;
    /** Returns the side of the bounding box the ray exits. */
    typename BoundBox3<Precision>::SIDE GetSideFromRay(
        const Vector3<Precision>& rclPt,
        const Vector3<Precision>& rclDir,
        Vector3<Precision>& rcInt
    ) const;

    /**
     * Searches for the closest point of the bounding box.
     */
    Vector3<Precision> ClosestPoint(const Vector3<Precision>& rclPt) const;
    /** Projects the box onto a plane and returns a 2D box. */
    BoundBox2d ProjectBox(const ViewProjMethod* proj) const;
    /** Transform the corners of this box with the given matrix and create a new bounding box.
     * @note It's up to the client programmer to make sure that this bounding box is valid.
     */
    BoundBox3<Precision> Transformed(const Matrix4D& mat) const;

    /** Returns the center of the box. */
    inline Vector3<Precision> GetCenter() const;
    /** Returns the minimum point of the box. */
    inline Vector3<Precision> GetMinimum() const;
    /** Returns the maximum point of the box. */
    inline Vector3<Precision> GetMaximum() const;
    /** Compute the diagonal length of this bounding box.
     * @note It's up to the client programmer to make sure that this bounding box is valid.
     */
    inline Precision CalcDiagonalLength() const;
    void SetVoid();

    /** Enlarges the box with \a fLen. */
    inline void Enlarge(Precision fLen);
    /** Shrinks the box with \a fLen. */
    inline void Shrink(Precision fLen);

    /** Calculates expansion in x-direction. */
    inline Precision LengthX() const;
    /** Calculates expansion in y-direction. */
    inline Precision LengthY() const;
    /** Calculates expansion in z-direction. */
    inline Precision LengthZ() const;
    /** Calculates the volume. If the box is invalid a negative value is returned */
    inline Precision Volume() const;
    /** Moves in x-direction. */
    inline void MoveX(Precision value);
    /** Moves in y-direction. */
    inline void MoveY(Precision value);
    /** Moves in z-direction. */
    inline void MoveZ(Precision value);
    /** Scales in x-direction. */
    inline void ScaleX(Precision value);
    /** Scales in y-direction. */
    inline void ScaleY(Precision value);
    /** Scales in z-direction. */
    inline void ScaleZ(Precision value);

    /** Prints the values to stream. */
    void Print(std::ostream&) const;
};


template<class Precision>
bool BoundBox3<Precision>::isOnRayW(Precision min, Precision max, Precision val)
{
    // Checks if point val lies on the ray [min,max]
    return ((min <= val) && (val <= max));
}

template<class Precision>
bool BoundBox3<Precision>::isOnRayS(Precision min, Precision max, Precision val)
{
    // Checks if point val lies on the ray [min,max[
    return ((min <= val) && (val < max));
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
template<class Precision>
inline BoundBox3<Precision>::BoundBox3(
    Precision fMinX,
    Precision fMinY,
    Precision fMinZ,
    Precision fMaxX,
    Precision fMaxY,
    Precision fMaxZ
)
    : MinX(fMinX)
    , MinY(fMinY)
    , MinZ(fMinZ)
    , MaxX(fMaxX)
    , MaxY(fMaxY)
    , MaxZ(fMaxZ)
{}
// NOLINTEND(bugprone-easily-swappable-parameters)

template<class Precision>
inline BoundBox3<Precision>::BoundBox3(const Vector3<Precision>* pclVect, std::size_t ulCt)
    : MinX(std::numeric_limits<Precision>::max())
    , MinY(std::numeric_limits<Precision>::max())
    , MinZ(std::numeric_limits<Precision>::max())
    , MaxX(-std::numeric_limits<Precision>::max())
    , MaxY(-std::numeric_limits<Precision>::max())
    , MaxZ(-std::numeric_limits<Precision>::max())
{
    const Vector3<Precision>* pI = nullptr;
    const Vector3<Precision>* pEnd = pclVect + ulCt;
    for (pI = pclVect; pI < pEnd; ++pI) {
        MinX = std::min<Precision>(MinX, pI->x);
        MinY = std::min<Precision>(MinY, pI->y);
        MinZ = std::min<Precision>(MinZ, pI->z);
        MaxX = std::max<Precision>(MaxX, pI->x);
        MaxY = std::max<Precision>(MaxY, pI->y);
        MaxZ = std::max<Precision>(MaxZ, pI->z);
    }
}

template<class Precision>
inline BoundBox3<Precision>::BoundBox3(const Vector3<Precision>& point, Precision distance)
    : MinX(point.x - distance)
    , MinY(point.y - distance)
    , MinZ(point.z - distance)
    , MaxX(point.x + distance)
    , MaxY(point.y + distance)
    , MaxZ(point.z + distance)
{}

template<class Precision>
inline BoundBox3<Precision>::~BoundBox3() = default;

template<class Precision>
inline bool BoundBox3<Precision>::Intersect(const BoundBox3<Precision>& rcBB) const
{
    if (rcBB.MaxX < this->MinX || rcBB.MinX > this->MaxX) {
        return false;
    }
    if (rcBB.MaxY < this->MinY || rcBB.MinY > this->MaxY) {
        return false;
    }
    if (rcBB.MaxZ < this->MinZ || rcBB.MinZ > this->MaxZ) {
        return false;
    }
    return true;
}

template<class Precision>
bool BoundBox3<Precision>::operator&&(const BoundBox3<Precision>& rcBB) const
{
    return Intersect(rcBB);
}

template<class Precision>
inline bool BoundBox3<Precision>::Intersect(const BoundBox2d& rcBB) const
{
    if (rcBB.MaxX < this->MinX || rcBB.MinX > this->MaxX) {
        return false;
    }
    if (rcBB.MaxY < this->MinY || rcBB.MinY > this->MaxY) {
        return false;
    }
    return true;
}

template<class Precision>
inline bool BoundBox3<Precision>::operator&&(const BoundBox2d& rcBB) const
{
    return Intersect(rcBB);
}

template<class Precision>
inline BoundBox3<Precision> BoundBox3<Precision>::Intersected(const BoundBox3<Precision>& rcBB) const
{
    BoundBox3<Precision> cBBRes;

    cBBRes.MinX = std::max<Precision>(MinX, rcBB.MinX);
    cBBRes.MaxX = std::min<Precision>(MaxX, rcBB.MaxX);
    cBBRes.MinY = std::max<Precision>(MinY, rcBB.MinY);
    cBBRes.MaxY = std::min<Precision>(MaxY, rcBB.MaxY);
    cBBRes.MinZ = std::max<Precision>(MinZ, rcBB.MinZ);
    cBBRes.MaxZ = std::min<Precision>(MaxZ, rcBB.MaxZ);

    return cBBRes;
}

template<class Precision>
inline BoundBox3<Precision> BoundBox3<Precision>::United(const BoundBox3<Precision>& rcBB) const
{
    BoundBox3<Precision> cBBRes;

    cBBRes.MinX = std::min<Precision>(MinX, rcBB.MinX);
    cBBRes.MaxX = std::max<Precision>(MaxX, rcBB.MaxX);
    cBBRes.MinY = std::min<Precision>(MinY, rcBB.MinY);
    cBBRes.MaxY = std::max<Precision>(MaxY, rcBB.MaxY);
    cBBRes.MinZ = std::min<Precision>(MinZ, rcBB.MinZ);
    cBBRes.MaxZ = std::max<Precision>(MaxZ, rcBB.MaxZ);

    return cBBRes;
}

template<class Precision>
inline void BoundBox3<Precision>::Add(const Vector3<Precision>& rclVect)
{
    this->MinX = std::min<Precision>(this->MinX, rclVect.x);
    this->MinY = std::min<Precision>(this->MinY, rclVect.y);
    this->MinZ = std::min<Precision>(this->MinZ, rclVect.z);
    this->MaxX = std::max<Precision>(this->MaxX, rclVect.x);
    this->MaxY = std::max<Precision>(this->MaxY, rclVect.y);
    this->MaxZ = std::max<Precision>(this->MaxZ, rclVect.z);
}

template<class Precision>
inline void BoundBox3<Precision>::Add(const BoundBox3<Precision>& rcBB)
{
    this->MinX = std::min<Precision>(this->MinX, rcBB.MinX);
    this->MaxX = std::max<Precision>(this->MaxX, rcBB.MaxX);
    this->MinY = std::min<Precision>(this->MinY, rcBB.MinY);
    this->MaxY = std::max<Precision>(this->MaxY, rcBB.MaxY);
    this->MinZ = std::min<Precision>(this->MinZ, rcBB.MinZ);
    this->MaxZ = std::max<Precision>(this->MaxZ, rcBB.MaxZ);
}

template<class Precision>
inline bool BoundBox3<Precision>::IsInBox(const Vector3<Precision>& rcVct) const
{
    if (rcVct.x < this->MinX || rcVct.x > this->MaxX) {
        return false;
    }
    if (rcVct.y < this->MinY || rcVct.y > this->MaxY) {
        return false;
    }
    if (rcVct.z < this->MinZ || rcVct.z > this->MaxZ) {
        return false;
    }
    return true;
}

template<class Precision>
inline bool BoundBox3<Precision>::IsInBox(const BoundBox3<Precision>& rcBB) const
{
    if (rcBB.MinX < this->MinX || rcBB.MaxX > this->MaxX) {
        return false;
    }
    if (rcBB.MinY < this->MinY || rcBB.MaxY > this->MaxY) {
        return false;
    }
    if (rcBB.MinZ < this->MinZ || rcBB.MaxZ > this->MaxZ) {
        return false;
    }
    return true;
}

template<class Precision>
inline bool BoundBox3<Precision>::IsInBox(const BoundBox2d& rcBB) const
{
    if (rcBB.MinX < this->MinX || rcBB.MaxX > this->MaxX) {
        return false;
    }
    if (rcBB.MinY < this->MinY || rcBB.MaxY > this->MaxY) {
        return false;
    }
    return true;
}

template<class Precision>
inline bool BoundBox3<Precision>::IsValid() const
{
    return ((MinX <= MaxX) && (MinY <= MaxY) && (MinZ <= MaxZ));
}

template<class Precision>
inline bool BoundBox3<Precision>::GetOctantFromVector(
    const Vector3<Precision>& rclVct,
    OCTANT& rclOctant
) const
{
    if (!IsInBox(rclVct)) {
        return false;
    }

    unsigned short usNdx = 0;
    if (isOnRayS((MinX + MaxX) / 2, MaxX, rclVct.x)) {  // left/RIGHT
        usNdx |= 1;
    }
    if (isOnRayS((MinY + MaxY) / 2, MaxY, rclVct.y)) {  // down/UP
        usNdx |= 2;
    }
    if (isOnRayS((MinZ + MaxZ) / 2, MaxZ, rclVct.z)) {  // back/FRONT
        usNdx |= 4;
    }
    rclOctant = static_cast<OCTANT>(usNdx);
    return true;
}

template<class Precision>
inline BoundBox3<Precision> BoundBox3<Precision>::CalcOctant(
    typename BoundBox3<Precision>::OCTANT Octant
) const
{
    BoundBox3<Precision> cOct(*this);

    switch (Octant) {
        case OCT_LDB:
            cOct.MaxX = (cOct.MinX + cOct.MaxX) / 2;
            cOct.MaxY = (cOct.MinY + cOct.MaxY) / 2;
            cOct.MaxZ = (cOct.MinZ + cOct.MaxZ) / 2;
            break;

        case OCT_RDB:
            cOct.MinX = (cOct.MinX + cOct.MaxX) / 2;
            cOct.MaxY = (cOct.MinY + cOct.MaxY) / 2;
            cOct.MaxZ = (cOct.MinZ + cOct.MaxZ) / 2;
            break;

        case OCT_LUB:
            cOct.MaxX = (cOct.MinX + cOct.MaxX) / 2;
            cOct.MinY = (cOct.MinY + cOct.MaxY) / 2;
            cOct.MaxZ = (cOct.MinZ + cOct.MaxZ) / 2;
            break;

        case OCT_RUB:
            cOct.MinX = (cOct.MinX + cOct.MaxX) / 2;
            cOct.MinY = (cOct.MinY + cOct.MaxY) / 2;
            cOct.MaxZ = (cOct.MinZ + cOct.MaxZ) / 2;
            break;

        case OCT_LDF:
            cOct.MaxX = (cOct.MinX + cOct.MaxX) / 2;
            cOct.MaxY = (cOct.MinY + cOct.MaxY) / 2;
            cOct.MinZ = (cOct.MinZ + cOct.MaxZ) / 2;
            break;

        case OCT_RDF:
            cOct.MinX = (cOct.MinX + cOct.MaxX) / 2;
            cOct.MaxY = (cOct.MinY + cOct.MaxY) / 2;
            cOct.MinZ = (cOct.MinZ + cOct.MaxZ) / 2;
            break;

        case OCT_LUF:
            cOct.MaxX = (cOct.MinX + cOct.MaxX) / 2;
            cOct.MinY = (cOct.MinY + cOct.MaxY) / 2;
            cOct.MinZ = (cOct.MinZ + cOct.MaxZ) / 2;
            break;

        case OCT_RUF:
            cOct.MinX = (cOct.MinX + cOct.MaxX) / 2;
            cOct.MinY = (cOct.MinY + cOct.MaxY) / 2;
            cOct.MinZ = (cOct.MinZ + cOct.MaxZ) / 2;
            break;
    }
    return cOct;
}

template<class Precision>
inline Vector3<Precision> BoundBox3<Precision>::CalcPoint(unsigned short usPoint) const
{
    switch (usPoint) {
        case TLB:
            return Vector3<Precision>(MinX, MinY, MaxZ);
        case TLF:
            return Vector3<Precision>(MaxX, MinY, MaxZ);
        case TRF:
            return Vector3<Precision>(MaxX, MaxY, MaxZ);
        case TRB:
            return Vector3<Precision>(MinX, MaxY, MaxZ);
        case BLB:
            return Vector3<Precision>(MinX, MinY, MinZ);
        case BLF:
            return Vector3<Precision>(MaxX, MinY, MinZ);
        case BRF:
            return Vector3<Precision>(MaxX, MaxY, MinZ);
        case BRB:
            return Vector3<Precision>(MinX, MaxY, MinZ);
    }

    return Vector3<Precision>();
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
template<class Precision>
inline void BoundBox3<Precision>::CalcPlane(
    unsigned short usPlane,
    Vector3<Precision>& rBase,
    Vector3<Precision>& rNormal
) const
// NOLINTEND(bugprone-easily-swappable-parameters)
{
    switch (usPlane) {
        case LEFT:
            rBase.Set(MinX, MinY, MaxZ);
            rNormal.Set(1.0F, 0.0F, 0.0F);
            break;

        case RIGHT:
            rBase.Set(MaxX, MinY, MaxZ);
            rNormal.Set(1.0F, 0.0F, 0.0F);
            break;

        case TOP:
            rBase.Set(MinX, MaxY, MaxZ);
            rNormal.Set(0.0F, 1.0F, 0.0F);
            break;

        case BOTTOM:
            rBase.Set(MinX, MinY, MaxZ);
            rNormal.Set(0.0F, 1.0F, 0.0F);
            break;

        case FRONT:
            rBase.Set(MinX, MinY, MaxZ);
            rNormal.Set(0.0F, 0.0F, 1.0F);
            break;

        case BACK:
            rBase.Set(MinX, MinY, MinZ);
            rNormal.Set(0.0F, 0.0F, 1.0F);
            break;
        default:
            break;
    }
}

template<class Precision>
inline bool BoundBox3<Precision>::CalcEdge(
    unsigned short usEdge,
    Vector3<Precision>& rcP0,
    Vector3<Precision>& rcP1
) const
{
    switch (usEdge) {
        case TLB_TLF:
            rcP0 = CalcPoint(TLB);
            rcP1 = CalcPoint(TLF);
            break;
        case TLF_TRF:
            rcP0 = CalcPoint(TLF);
            rcP1 = CalcPoint(TRF);
            break;
        case TRF_TRB:
            rcP0 = CalcPoint(TRF);
            rcP1 = CalcPoint(TRB);
            break;
        case TRB_TLB:
            rcP0 = CalcPoint(TRB);
            rcP1 = CalcPoint(TLB);
            break;
        case BLB_BLF:
            rcP0 = CalcPoint(BLB);
            rcP1 = CalcPoint(BLF);
            break;
        case BLF_BRF:
            rcP0 = CalcPoint(BLF);
            rcP1 = CalcPoint(BRF);
            break;
        case BRF_BRB:
            rcP0 = CalcPoint(BRF);
            rcP1 = CalcPoint(BRB);
            break;
        case BRB_BLB:
            rcP0 = CalcPoint(BRB);
            rcP1 = CalcPoint(BLB);
            break;
        case TLB_BLB:
            rcP0 = CalcPoint(TLB);
            rcP1 = CalcPoint(BLB);
            break;
        case TLF_BLF:
            rcP0 = CalcPoint(TLF);
            rcP1 = CalcPoint(BLF);
            break;
        case TRF_BRF:
            rcP0 = CalcPoint(TRF);
            rcP1 = CalcPoint(BRF);
            break;
        case TRB_BRB:
            rcP0 = CalcPoint(TRB);
            rcP1 = CalcPoint(BRB);
            break;
        default:
            return false;  // undefined
    }

    return true;
}

template<class Precision>
inline bool BoundBox3<Precision>::IntersectionPoint(
    const Vector3<Precision>& rcVct,
    const Vector3<Precision>& rcVctDir,
    Vector3<Precision>& cVctRes,
    Precision epsilon
) const
{
    const unsigned short num = 6;
    bool rc = false;
    BoundBox3<Precision> cCmpBound(*this);

    // enlarge bounding box by epsilon
    cCmpBound.Enlarge(epsilon);

    // Is point inside?
    if (cCmpBound.IsInBox(rcVct)) {
        // test sides
        for (unsigned short i = 0; (i < num) && (!rc); i++) {
            rc = IntersectPlaneWithLine(i, rcVct, rcVctDir, cVctRes);

            if (!cCmpBound.IsInBox(cVctRes)) {
                rc = false;
            }

            if (rc) {
                // does intersection point lie in desired direction
                // or was found the opposing side?
                // -> scalar product of both direction vectors > 0 (angle < 90)
                rc = ((cVctRes - rcVct) * rcVctDir) >= 0.0;
            }
        }
    }

    return rc;
}

template<class Precision>
inline bool BoundBox3<Precision>::IsCutLine(
    const Vector3<Precision>& rcBase,
    const Vector3<Precision>& rcDir,
    Precision fTolerance
) const
{
    const unsigned short num = 6;
    Precision fDist;

    // at first only a rough and quick test by the
    // Distance of the line to the center of the BB is calculated
    // and with the maximum diagonal length + fTolerance
    // will be compared.

    // Distance between center point and line
    fDist = (rcDir % (GetCenter() - rcBase)).Length() / rcDir.Length();

    if (fDist > (CalcDiagonalLength() + fTolerance)) {
        return false;
    }

    // more detailed test here
    Vector3<Precision> clVectRes;

    // intersect each face with the line
    for (unsigned short i = 0; i < num; i++) {
        if (IntersectPlaneWithLine(i, rcBase, rcDir, clVectRes)) {
            // Check whether the intersection point is within BB limits + Tolerance
            switch (i) {
                case LEFT:  // left and right plane
                case RIGHT:
                    if ((isOnRayW(MinY - fTolerance, MaxY + fTolerance, clVectRes.y)
                         && isOnRayW(MinZ - fTolerance, MaxZ + fTolerance, clVectRes.z))) {
                        return true;
                    }
                    break;
                case TOP:  // top and bottom plane
                case BOTTOM:
                    if ((isOnRayW(MinX - fTolerance, MaxX + fTolerance, clVectRes.x)
                         && isOnRayW(MinZ - fTolerance, MaxZ + fTolerance, clVectRes.z))) {
                        return true;
                    }
                    break;
                case FRONT:  // front and back plane
                case BACK:
                    if ((isOnRayW(MinX - fTolerance, MaxX + fTolerance, clVectRes.x)
                         && isOnRayW(MinY - fTolerance, MaxY + fTolerance, clVectRes.y))) {
                        return true;
                    }
                    break;
            }
        }
    }

    return false;
}

template<class Precision>
inline bool BoundBox3<Precision>::IsCutPlane(
    const Vector3<Precision>& rclBase,
    const Vector3<Precision>& rclNormal
) const
{
    const unsigned short num = 8;
    if (fabs(GetCenter().DistanceToPlane(rclBase, rclNormal)) < CalcDiagonalLength()) {
        Precision fD = CalcPoint(CORNER::TLB).DistanceToPlane(rclBase, rclNormal);
        for (unsigned short i = 1; i < num; i++) {
            if ((CalcPoint(i).DistanceToPlane(rclBase, rclNormal) * fD) <= 0.0F) {
                return true;
            }
        }
    }
    return false;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
template<class Precision>
inline bool BoundBox3<Precision>::IntersectWithLine(
    const Vector3<Precision>& rcBase,
    const Vector3<Precision>& rcDir,
    Vector3<Precision>& rcP0,
    Vector3<Precision>& rcP1
) const
// NOLINTEND(bugprone-easily-swappable-parameters)
{
    const unsigned short num = 6;
    Vector3<Precision> clVectRes;
    std::array<Vector3<Precision>, num> clVect;
    unsigned short numIntersect = 0;

    auto checkIntersect = [&](Base::Vector3<Precision> p1, Base::Vector3<Precision> p2) {
        if (isOnRayS(p1.x, p1.y, p1.z) && isOnRayS(p2.x, p2.y, p2.z)) {
            clVect[numIntersect] = clVectRes;
            numIntersect++;
        }
    };

    // cut each face with the line
    for (unsigned short i = 0; i < num; i++) {
        if (IntersectPlaneWithLine(i, rcBase, rcDir, clVectRes)) {
            // check if intersection point is inside
            switch (i) {
                case LEFT:  // left and right plane
                case RIGHT:
                    checkIntersect(
                        Vector3<Precision> {MinY, MaxY, clVectRes.y},
                        Vector3<Precision> {MinZ, MaxZ, clVectRes.z}
                    );
                    break;
                case TOP:  // top and bottom plane
                case BOTTOM:
                    checkIntersect(
                        Vector3<Precision> {MinX, MaxX, clVectRes.x},
                        Vector3<Precision> {MinZ, MaxZ, clVectRes.z}
                    );
                    break;
                case FRONT:  // front and back plane
                case BACK:
                    checkIntersect(
                        Vector3<Precision> {MinX, MaxX, clVectRes.x},
                        Vector3<Precision> {MinY, MaxY, clVectRes.y}
                    );
                    break;
            }
        }
    }

    if (numIntersect == 2) {
        rcP0 = clVect[0];
        rcP1 = clVect[1];
        return true;
    }

    if (numIntersect > 2) {  // search two different intersection points
        for (unsigned short i = 1; i < numIntersect; i++) {
            if (clVect[i] != clVect[0]) {
                rcP0 = clVect[0];
                rcP1 = clVect[i];
                return true;
            }
        }
    }

    return false;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
template<class Precision>
inline bool BoundBox3<Precision>::IntersectPlaneWithLine(
    unsigned short usSide,
    const Vector3<Precision>& rcBase,
    const Vector3<Precision>& rcDir,
    Vector3<Precision>& rcP0
) const
// NOLINTEND(bugprone-easily-swappable-parameters)
{
    Precision value;
    Vector3<Precision> cBase;
    Vector3<Precision> cNormal;
    Vector3<Precision> cDir(rcDir);
    CalcPlane(usSide, cBase, cNormal);

    if ((cNormal * cDir) == 0.0F) {
        return false;  // no point of intersection
    }

    value = (cNormal * (cBase - rcBase)) / (cNormal * cDir);
    cDir.Scale(value, value, value);
    rcP0 = rcBase + cDir;
    return true;
}

template<class Precision>
inline typename BoundBox3<Precision>::SIDE BoundBox3<Precision>::GetSideFromRay(
    const Vector3<Precision>& rclPt,
    const Vector3<Precision>& rclDir
) const
{
    Vector3<Precision> cIntersection;
    return GetSideFromRay(rclPt, rclDir, cIntersection);
}

template<class Precision>
inline typename BoundBox3<Precision>::SIDE BoundBox3<Precision>::GetSideFromRay(
    const Vector3<Precision>& rclPt,
    const Vector3<Precision>& rclDir,
    Vector3<Precision>& rcInt
) const
{
    Vector3<Precision> cP0;
    Vector3<Precision> cP1;
    if (!IntersectWithLine(rclPt, rclDir, cP0, cP1)) {
        return INVALID;
    }

    Vector3<Precision> cOut;
    // same orientation
    if ((cP1 - cP0) * rclDir > 0) {
        cOut = cP1;
    }
    else {
        cOut = cP0;
    }

    rcInt = cOut;

    Precision fMax = 1.0e-3F;  // NOLINT
    SIDE tSide = INVALID;

    if (fabs(cOut.x - MinX) < fMax) {  // left plane
        fMax = Precision(fabs(cOut.x - MinX));
        tSide = LEFT;
    }

    if (fabs(cOut.x - MaxX) < fMax) {  // right plane
        fMax = Precision(fabs(cOut.x - MaxX));
        tSide = RIGHT;
    }

    if (fabs(cOut.y - MinY) < fMax) {  // bottom plane
        fMax = Precision(fabs(cOut.y - MinY));
        tSide = BOTTOM;
    }

    if (fabs(cOut.y - MaxY) < fMax) {  // top plane
        fMax = Precision(fabs(cOut.y - MaxY));
        tSide = TOP;
    }

    if (fabs(cOut.z - MinZ) < fMax) {  // front plane
        fMax = Precision(fabs(cOut.z - MinZ));
        tSide = FRONT;
    }

    if (fabs(cOut.z - MaxZ) < fMax) {  // back plane
        fMax = Precision(fabs(cOut.z - MaxZ));
        tSide = BACK;
    }

    return tSide;
}

template<class Precision>
inline Vector3<Precision> BoundBox3<Precision>::ClosestPoint(const Vector3<Precision>& rclPt) const
{
    Vector3<Precision> closest = rclPt;

    Vector3<Precision> center = GetCenter();
    Precision devx = closest.x - center.x;
    Precision devy = closest.y - center.y;
    Precision devz = closest.z - center.z;

    Precision halfwidth = (MaxX - MinX) / 2;
    Precision halfheight = (MaxY - MinY) / 2;
    Precision halfdepth = (MaxZ - MinZ) / 2;

    // Move point to be on the nearest plane of the box.
    if ((fabs(devx) > fabs(devy)) && (fabs(devx) > fabs(devz))) {
        closest.x = center.x + halfwidth * ((devx < 0.0) ? -1.0 : 1.0);
    }
    else if (fabs(devy) > fabs(devz)) {
        closest.y = center.y + halfheight * ((devy < 0.0) ? -1.0 : 1.0);
    }
    else {
        closest.z = center.z + halfdepth * ((devz < 0.0) ? -1.0 : 1.0);
    }

    // Clamp to be inside box.
    closest.x = std::min<Precision>(std::max<Precision>(closest.x, MinX), MaxX);
    closest.y = std::min<Precision>(std::max<Precision>(closest.y, MinY), MaxY);
    closest.z = std::min<Precision>(std::max<Precision>(closest.z, MinZ), MaxZ);

    return closest;
}

template<class Precision>
inline BoundBox2d BoundBox3<Precision>::ProjectBox(const ViewProjMethod* proj) const
{
    const int num = 8;
    BoundBox2d clBB2D;
    clBB2D.SetVoid();

    for (int i = 0; i < num; i++) {
        Vector3<Precision> clTrsPt = (*proj)(CalcPoint(i));
        clBB2D.Add(Vector2d(clTrsPt.x, clTrsPt.y));
    }

    return clBB2D;
}

template<class Precision>
inline BoundBox3<Precision> BoundBox3<Precision>::Transformed(const Matrix4D& mat) const
{
    const int num = 8;
    BoundBox3<Precision> bbox;
    for (int i = 0; i < num; i++) {
        bbox.Add(mat * CalcPoint(i));
    }
    return bbox;
}

template<class Precision>
inline Vector3<Precision> BoundBox3<Precision>::GetCenter() const
{
    return Vector3<Precision>((MaxX + MinX) / 2, (MaxY + MinY) / 2, (MaxZ + MinZ) / 2);
}

template<class Precision>
inline Vector3<Precision> BoundBox3<Precision>::GetMinimum() const
{
    return Vector3<Precision>(MinX, MinY, MinZ);
}

template<class Precision>
inline Vector3<Precision> BoundBox3<Precision>::GetMaximum() const
{
    return Vector3<Precision>(MaxX, MaxY, MaxZ);
}

template<class Precision>
inline Precision BoundBox3<Precision>::CalcDiagonalLength() const
{
    return static_cast<Precision>(sqrt(
        ((MaxX - MinX) * (MaxX - MinX)) + ((MaxY - MinY) * (MaxY - MinY))
        + ((MaxZ - MinZ) * (MaxZ - MinZ))
    ));
}

template<class Precision>
inline void BoundBox3<Precision>::SetVoid()
{
    MinX = MinY = MinZ = std::numeric_limits<Precision>::max();
    MaxX = MaxY = MaxZ = -std::numeric_limits<Precision>::max();
}

template<class Precision>
inline void BoundBox3<Precision>::Enlarge(Precision fLen)
{
    MinX -= fLen;
    MinY -= fLen;
    MinZ -= fLen;
    MaxX += fLen;
    MaxY += fLen;
    MaxZ += fLen;
}

template<class Precision>
inline void BoundBox3<Precision>::Shrink(Precision fLen)
{
    MinX += fLen;
    MinY += fLen;
    MinZ += fLen;
    MaxX -= fLen;
    MaxY -= fLen;
    MaxZ -= fLen;
}

template<class Precision>
inline Precision BoundBox3<Precision>::LengthX() const
{
    return MaxX - MinX;
}

template<class Precision>
inline Precision BoundBox3<Precision>::LengthY() const
{
    return MaxY - MinY;
}

template<class Precision>
inline Precision BoundBox3<Precision>::LengthZ() const
{
    return MaxZ - MinZ;
}

template<class Precision>
inline Precision BoundBox3<Precision>::Volume() const
{
    if (!IsValid()) {
        return -1.0;
    }
    return LengthX() * LengthY() * LengthZ();
}

template<class Precision>
inline void BoundBox3<Precision>::MoveX(Precision value)
{
    MinX += value;
    MaxX += value;
}

template<class Precision>
inline void BoundBox3<Precision>::MoveY(Precision value)
{
    MinY += value;
    MaxY += value;
}

template<class Precision>
inline void BoundBox3<Precision>::MoveZ(Precision value)
{
    MinZ += value;
    MaxZ += value;
}

template<class Precision>
inline void BoundBox3<Precision>::ScaleX(Precision value)
{
    MinX *= value;
    MaxX *= value;
}

template<class Precision>
inline void BoundBox3<Precision>::ScaleY(Precision value)
{
    MinY *= value;
    MaxY *= value;
}

template<class Precision>
inline void BoundBox3<Precision>::ScaleZ(Precision value)
{
    MinZ *= value;
    MaxZ *= value;
}

using BoundBox3f = BoundBox3<float>;
using BoundBox3d = BoundBox3<double>;

}  // namespace Base
