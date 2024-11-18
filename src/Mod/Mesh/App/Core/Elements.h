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

#ifndef MESH_ELEMENTS_H
#define MESH_ELEMENTS_H

#include <climits>
#include <cstring>
#include <functional>
#include <vector>

#include <Base/BoundBox.h>
#include <Base/Matrix.h>

#include "Definitions.h"


// Cannot use namespace Base in constructors of MeshPoint
#ifdef _MSC_VER
using Base::Vector3f;
#endif

namespace MeshCore
{

class MeshHelpEdge;
class MeshPoint;

/**
 * Helper class providing an operator for comparison
 * of two edges. The class holds the point indices of the
 * underlying edge.
 */
class MeshExport MeshHelpEdge
{
public:
    inline bool operator==(const MeshHelpEdge& rclEdge) const;
    PointIndex _ulIndex[2];  // point indices
};

/**
 * Structure that holds the facet index with the two corner point
 * indices of the facet's orientation this edge is attached to.
 */
class MeshExport MeshIndexEdge
{
public:
    FacetIndex _ulFacetIndex;      // Facet index
    unsigned short _ausCorner[2];  // corner point indices of the facet
};

/** MeshEdge just a pair of two point indices */
using MeshEdge = std::pair<PointIndex, PointIndex>;

struct MeshExport EdgeCollapse
{
    PointIndex _fromPoint;
    PointIndex _toPoint;
    std::vector<PointIndex> _adjacentFrom;  // adjacent points to _fromPoint
    std::vector<PointIndex> _adjacentTo;    // adjacent points to _toPoint
    std::vector<FacetIndex> _removeFacets;
    std::vector<FacetIndex> _changeFacets;
};

struct MeshExport VertexCollapse
{
    PointIndex _point;
    std::vector<PointIndex> _circumPoints;
    std::vector<FacetIndex> _circumFacets;
};

/**
 * The MeshPoint class represents a point in the mesh data structure. The class inherits from
 * Vector3f and provides some additional information such as flag state and property value.
 * The flags can be modified by the Set() and Reset() and queried by IsFlag().
 * A point can temporary be in an invalid state (e.g during deletion of several points), but
 * must not be set in general, i.e. always usable within a mesh-internal algorithm.
 *
 * Note: The status flag SEGMENT mark a point to be part of certain subset, a segment.
 * This flag must not be set by any algorithm unless it adds or removes points to a segment.
 *
 * Note: The status flag SELECTED mark a point to be selected which is e.g. used in the GUI.
 * This flag must not be set by any algorithm unless it adds or removes points to the selection.
 */
class MeshExport MeshPoint: public Base::Vector3f
{
public:
    enum TFlagType
    {
        INVALID = 1,
        VISIT = 2,
        SEGMENT = 4,
        MARKED = 8,
        SELECTED = 16,
        REV = 32,
        TMP0 = 64,
        TMP1 = 128
    };

    /** @name Construction */
    //@{
    MeshPoint()
        : _ucFlag(0)
        , _ulProp(0)
    {}
    inline MeshPoint(float x, float y, float z);
    inline MeshPoint(const Base::Vector3f& rclPt);  // explicit bombs
    inline MeshPoint(const MeshPoint& rclPt) = default;
    inline MeshPoint(MeshPoint&& rclPt) = default;
    ~MeshPoint() = default;
    //@}

public:
    /** @name Flag state
     * @note All flag methods are const as they do NOT change the actual behaviour of the object
     */
    //@{
    void SetFlag(TFlagType tF) const
    {
        _ucFlag |= static_cast<unsigned char>(tF);
    }
    void ResetFlag(TFlagType tF) const
    {
        _ucFlag &= ~static_cast<unsigned char>(tF);
    }
    bool IsFlag(TFlagType tF) const
    {
        return (_ucFlag & static_cast<unsigned char>(tF)) == static_cast<unsigned char>(tF);
    }
    void ResetInvalid() const
    {
        ResetFlag(INVALID);
    }
    void SetInvalid() const
    {
        SetFlag(INVALID);
    }
    bool IsValid() const
    {
        return !IsFlag(INVALID);
    }
    void SetProperty(unsigned long uP) const
    {
        _ulProp = uP;
    }
    //@}

    // Assignment
    inline MeshPoint& operator=(const MeshPoint& rclPt) = default;
    inline MeshPoint& operator=(MeshPoint&& rclPt) = default;

    // compare operator
    inline bool operator==(const MeshPoint& rclPt) const;
    inline bool operator==(const Base::Vector3f& rclV) const;
    inline bool operator<(const MeshPoint& rclPt) const;

public:
    mutable unsigned char _ucFlag; /**< Flag member */
    mutable unsigned long _ulProp; /**< Free usable property */
};

/**
 * The MeshGeomEdge class is geometric counterpart to MeshEdge that holds the
 * geometric data points of an edge.
 */
class MeshExport MeshGeomEdge
{
public:
    MeshGeomEdge() = default;

    /** Checks if the edge is inside the bounding box or intersects with it. */
    bool ContainedByOrIntersectBoundingBox(const Base::BoundBox3f& rclBB) const;
    /** Returns the bounding box of the edge. */
    Base::BoundBox3f GetBoundBox() const;
    /** Checks if the edge intersects with the given bounding box. */
    bool IntersectBoundingBox(const Base::BoundBox3f& rclBB) const;
    /** Calculates the intersection point of the line defined by the base \a rclPt and the direction
     * \a rclDir with the edge. The intersection must be inside the edge. If there is no
     * intersection false is returned.
     */
    bool IntersectWithLine(const Base::Vector3f& rclPt,
                           const Base::Vector3f& rclDir,
                           Base::Vector3f& rclRes) const;
    /** Calculates the intersection point of an edge with this edge.
     * The intersection must be inside both edges. If there is no intersection false is returned.
     */
    bool IntersectWithEdge(const MeshGeomEdge& edge, Base::Vector3f& res) const;
    /** Calculates the intersection point of the plane defined by the base \a rclPt and the
     * direction \a rclDir with the edge. The intersection must be inside the edge. If there is no
     * intersection false is returned.
     */
    bool IntersectWithPlane(const Base::Vector3f& rclPt,
                            const Base::Vector3f& rclDir,
                            Base::Vector3f& rclRes) const;
    /**
     * Calculates the projection of a point onto the line defined by the edge. The caller must check
     * if the projection point is inside the edge.
     */
    void ProjectPointToLine(const Base::Vector3f& rclPoint, Base::Vector3f& rclProj) const;
    /**
     * Get the closest points \a rclPnt1 and \a rclPnt2 of the line defined by this edge and the
     * line defined by \a rclPt and \a rclDir. If the two points are identical then both lines
     * intersect each other.
     */
    void ClosestPointsToLine(const Base::Vector3f& linePt,
                             const Base::Vector3f& lineDir,
                             Base::Vector3f& rclPnt1,
                             Base::Vector3f& rclPnt2) const;
    /**
     * Checks if the point is part of the edge. A point is regarded as part
     * of an edge if the distance is lower than \a fDistance to the projected point
     * of \a rclPoint on the edge.
     */
    bool IsPointOf(const Base::Vector3f& rclPoint, float fDistance) const;
    /**
     * Checks if the projection point of \a point lies on the edge.
     */
    bool IsProjectionPointOf(const Base::Vector3f& point) const;
    /**
     * Checks if the two edges are parallel.
     * \note Parallel edges could be collinear.
     */
    bool IsParallel(const MeshGeomEdge& edge) const;
    /**
     * Checks if the two edges are collinear.
     * \note Collinear edges always are parallel.
     */
    bool IsCollinear(const MeshGeomEdge& edge) const;

public:
    Base::Vector3f _aclPoints[2]; /**< Corner points */
    bool _bBorder {false};        /**< Set to true if border edge */
};

/**
 * The MeshFacet class represent a triangle facet in the mesh data.structure. A facet indexes
 * three neighbour facets and also three corner points.
 * This class only keeps topologic information but no geometric information at all.
 *
 * Here are the most important conventions concerning the facet's orientation:
 * \li neighbour or edge number of 0 is defined by corner 0 and 1
 * \li neighbour or edge number of 1 is defined by corner 1 and 2
 * \li neighbour or edge number of 2 is defined by corner 2 and 0
 * \li neighbour index is set to FACET_INDEX_MAX if there is no neighbour facet
 *
 * Note: The status flag SEGMENT mark a facet to be part of certain subset, a segment.
 * This flag must not be set by any algorithm unless it adds or removes facets to a segment.
 *
 * Note: The status flag SELECTED mark a facet to be selected which is e.g. used in the GUI.
 * This flag must not be set by any algorithm unless it adds or removes facets to the selection.
 */
class MeshFacet
{
public:
    enum TFlagType
    {
        INVALID = 1,
        VISIT = 2,
        SEGMENT = 4,
        MARKED = 8,
        SELECTED = 16,
        REV = 32,
        TMP0 = 64,
        TMP1 = 128
    };

public:
    /** @name Construction */
    //@{
    inline MeshFacet();
    inline MeshFacet(const MeshFacet& rclF) = default;
    inline MeshFacet(MeshFacet&& rclF) = default;
    inline MeshFacet(PointIndex p1,
                     PointIndex p2,
                     PointIndex p3,
                     FacetIndex n1 = FACET_INDEX_MAX,
                     FacetIndex n2 = FACET_INDEX_MAX,
                     FacetIndex n3 = FACET_INDEX_MAX);
    ~MeshFacet() = default;
    //@}

    /** @name Flag state
     * @note All flag methods are const as they do NOT change the actual behaviour of the object
     */
    //@{
    void SetFlag(TFlagType tF) const
    {
        _ucFlag |= static_cast<unsigned char>(tF);
    }
    void ResetFlag(TFlagType tF) const
    {
        _ucFlag &= ~static_cast<unsigned char>(tF);
    }
    bool IsFlag(TFlagType tF) const
    {
        return (_ucFlag & static_cast<unsigned char>(tF)) == static_cast<unsigned char>(tF);
    }
    void ResetInvalid() const
    {
        ResetFlag(INVALID);
    }
    void SetProperty(unsigned long uP) const
    {
        _ulProp = uP;
    }
    /**
     * Marks a facet as invalid. Should be used only temporary from within an algorithm
     * (e.g. deletion of several facets) but must not be set permanently.
     * From outside the data-structure must not have invalid facets.
     */
    void SetInvalid() const
    {
        SetFlag(INVALID);
    }
    bool IsValid() const
    {
        return !IsFlag(INVALID);
    }
    //@}

    // Assignment
    inline MeshFacet& operator=(const MeshFacet& rclF) = default;
    inline MeshFacet& operator=(MeshFacet&& rclF) = default;
    inline void SetVertices(PointIndex, PointIndex, PointIndex);
    inline void SetNeighbours(FacetIndex, FacetIndex, FacetIndex);

    /**
     * Returns the indices of the corner points of the given edge number.
     */
    inline void GetEdge(unsigned short usSide, MeshHelpEdge& rclEdge) const;
    /**
     * Returns the indices of the corner points of the given edge number.
     */
    inline std::pair<PointIndex, PointIndex> GetEdge(unsigned short usSide) const;
    /**
     * Returns the edge-number to the given index of neighbour facet.
     * If \a ulNIndex is not a neighbour USHRT_MAX is returned.
     */
    inline unsigned short Side(FacetIndex ulNIndex) const;
    /**
     * Returns the edge-number defined by two points. If one point is
     * not a corner point USHRT_MAX is returned.
     */
    inline unsigned short Side(PointIndex ulP0, PointIndex P1) const;
    /**
     * Returns the edge-number defined by the shared edge of both facets. If the facets don't
     * share a common edge USHRT_MAX is returned.
     */
    inline unsigned short Side(const MeshFacet& rcFace) const;
    /**
     * Returns true if this facet shares the same three points as \a rcFace.
     * The orientation is not of interest in this case.
     */
    inline bool IsEqual(const MeshFacet& rcFace) const;
    /**
     * Replaces the index of the corner point that is equal to \a ulOrig
     * by \a ulNew. If the facet does not have a corner point with this index
     * nothing happens.
     */
    inline void Transpose(PointIndex ulOrig, PointIndex ulNew);
    /**
     * Decrement the index for each corner point that is higher than \a ulIndex.
     */
    inline void Decrement(PointIndex ulIndex);
    /**
     * Checks if the facets references the given point index.
     */
    inline bool HasPoint(PointIndex) const;
    /**
     * Replaces the index of the neighbour facet that is equal to \a ulOrig
     * by \a ulNew. If the facet does not have a neighbourt with this index
     * nothing happens.
     */
    inline void ReplaceNeighbour(FacetIndex ulOrig, FacetIndex ulNew);
    /**
     * Checks if the neighbour exists at the given edge-number.
     */
    bool HasNeighbour(unsigned short usSide) const
    {
        return (_aulNeighbours[usSide] != FACET_INDEX_MAX);
    }
    /**
     * Checks if the given index is a neighbour facet.
     */
    bool IsNeighbour(FacetIndex index) const
    {
        return Side(index) < 3;
    }
    /** Counts the number of edges without neighbour. */
    inline unsigned short CountOpenEdges() const;
    /** Returns true if there is an edge without neighbour, otherwise false. */
    inline bool HasOpenEdge() const;
    /** Returns true if the two facets have the same orientation, false otherwise
     * Therefore the two facets must be adjacent.
     */
    inline bool HasSameOrientation(const MeshFacet&) const;
    /** Checks whether the facet is degenerated to a line of point. */
    inline bool IsDegenerated() const;
    /** Flips the orientation of the facet. */
    void FlipNormal()
    {
        std::swap(_aulPoints[1], _aulPoints[2]);
        std::swap(_aulNeighbours[0], _aulNeighbours[2]);
    }

public:
    mutable unsigned char _ucFlag; /**< Flag member. */
    mutable unsigned long _ulProp; /**< Free usable property. */
    PointIndex _aulPoints[3];      /**< Indices of corner points. */
    FacetIndex _aulNeighbours[3];  /**< Indices of neighbour facets. */
};

/**
 * The MeshGeomFacet class is geometric counterpart to MeshFacet that holds the
 * geometric data points of a triangle.
 */
class MeshExport MeshGeomFacet
{
public:
    /** @name Construction */
    //@{
    /// default constructor
    MeshGeomFacet();
    /// Constructor with the corner points
    MeshGeomFacet(const Base::Vector3f& v1, const Base::Vector3f& v2, const Base::Vector3f& v3);
    MeshGeomFacet(const MeshGeomFacet&) = default;
    MeshGeomFacet(MeshGeomFacet&&) = default;
    /// Destruction
    ~MeshGeomFacet() = default;
    //@}

    MeshGeomFacet& operator=(const MeshGeomFacet&) = default;
    MeshGeomFacet& operator=(MeshGeomFacet&&) = default;

public:
    /**
     * Checks if the point is part of the facet. A point is regarded as part
     * of a facet if the distance is lower \a fDistance and the projected point
     * in the facet normal direction is inside the triangle.
     */
    bool IsPointOf(const Base::Vector3f& rclPoint, float fDistance) const;
    /**
     * Checks if the point is inside or at the border of the facet. The point
     * must already exactly lie on the plane defined by the facet, which is not
     * checked. This method is very efficient.
     */
    bool IsPointOf(const Base::Vector3f& rclPoint) const;
    /** Checks whether the given point is inside the facet with tolerance \a fDistance.
     * This method does actually the same as IsPointOf() but this implementation
     * is done more effective through comparison of normals.
     */
    bool IsPointOfFace(const Base::Vector3f& rclP, float fDistance) const;
    /** Calculates the weights \a w1, ...,  \a w3 of the corners to get the point \a rclP, i.e.
     * rclP = w0*v0 + w1*v1 + w2*v2 (v0-v2 are the corners corners).
     * If w0+w1+w2==1.0 then the point rclP lies on the plane that is spanned by the facet,
     * otherwise the point doesn't lie on the plane. If the sum of wi is 1 and if each wi is between
     * [0,1] than the point lies inside the facet or on the border, respectively.
     *
     * If the point doesn't lie on the plane false is returned, true otherwise.
     */
    bool Weights(const Base::Vector3f& rclP, float& w0, float& w1, float& w2) const;
    /**
     * Calculates the distance of a point to the plane defined by the triangle.
     */
    inline float DistancePlaneToPoint(const Base::Vector3f& rclPoint) const;
    /**
     * Calculates the projection of a point onto the plane defined by the triangle.
     */
    void ProjectPointToPlane(const Base::Vector3f& rclPoint, Base::Vector3f& rclProj) const;
    /**
     * Calculates the projection of a facet onto the plane defined by the triangle.
     */
    void ProjectFacetToPlane(MeshGeomFacet& rclFacet) const;
    /**
     * Checks whether the triangle is degenerated. A triangle is degenerated if its area
     * is less than an epsilon.
     */
    bool IsDegenerated(float epsilon) const;
    /**
     * Checks whether the triangle is deformed. A triangle is deformed if an angle
     * exceeds a given maximum angle or falls below a given minimum angle.
     * For performance reasons the cosine of minimum and maximum angle is expected.
     */
    bool IsDeformed(float fCosOfMinAngle, float fCosOfMaxAngle) const;
    /**
     * Enlarges the triangle.
     */
    void Enlarge(float fDist);
    /**
     * Calculates the facet normal for storing internally.
     */
    inline void CalcNormal() const;
    /**
     * Arrange the facet normal so the both vectors have the same orientation.
     */
    inline void ArrangeNormal(const Base::Vector3f& rclN);
    /**
     * Adjusts the facet's orientation to its normal.
     */
    inline void AdjustCirculationDirection();
    /** Invalidate the normal. It will be recomputed when querying it. */
    void NormalInvalid()
    {
        _bNormalCalculated = false;
    }
    /** Query the flag state of the facet. */
    bool IsFlag(MeshFacet::TFlagType tF) const
    {
        return (_ucFlag & static_cast<unsigned char>(tF)) == static_cast<unsigned char>(tF);
    }
    /** Set flag state */
    void SetFlag(MeshFacet::TFlagType tF)
    {
        _ucFlag |= static_cast<unsigned char>(tF);
    }
    /** Reset flag state */
    void ResetFlag(MeshFacet::TFlagType tF)
    {
        _ucFlag &= ~static_cast<unsigned char>(tF);
    }
    /** Calculates the facet's gravity point. */
    inline Base::Vector3f GetGravityPoint() const;
    /** Returns the normal of the facet. */
    inline Base::Vector3f GetNormal() const;
    /** Sets the facet's normal. */
    inline void SetNormal(const Base::Vector3f& rclNormal);
    /** Returns the wrapping bounding box. */
    inline Base::BoundBox3f GetBoundBox() const;
    /** Calculates the perimeter of the facet. */
    inline float Perimeter() const;
    /** Calculates the area of a facet. */
    inline float Area() const;
    /** Calculates the maximum angle of a facet. */
    float MaximumAngle() const;
    /** Calculates the minimum angle of a facet. */
    float MinimumAngle() const;
    /** Checks if the facet is inside the bounding box or intersects with it. */
    inline bool ContainedByOrIntersectBoundingBox(const Base::BoundBox3f& rcBB) const;
    /** Checks if the facet intersects with the given bounding box. */
    bool IntersectBoundingBox(const Base::BoundBox3f& rclBB) const;
    /** This method checks if both facets intersect.
     */
    bool IntersectWithFacet(const MeshGeomFacet& rclFacet) const;
    /**
     * Intersect the facet with the other facet
     * The result is line given by two points (if intersected).
     * Return is the number of intersections points: 0: no intersection, 1: one intersection point
     * (rclPt0), 2: two intersections points (rclPt0, rclPt1)
     */
    int IntersectWithFacet(const MeshGeomFacet& facet,
                           Base::Vector3f& rclPt0,
                           Base::Vector3f& rclPt1) const;
    /** Calculates the shortest distance from the line segment defined by \a rcP1 and \a rcP2 to
     * this facet.
     */
    float DistanceToLineSegment(const Base::Vector3f& rcP1, const Base::Vector3f& rcP2) const;
    /** Calculates the shortest distance from the point \a rcPt to the facet. */
    float DistanceToPoint(const Base::Vector3f& rcPt) const
    {
        Base::Vector3f res;
        return DistanceToPoint(rcPt, res);
    }
    /** Calculates the shortest distance from the point \a rcPt to the facet. \a rclNt is the point
     * of the facet with shortest distance.
     */
    float DistanceToPoint(const Base::Vector3f& rclPt, Base::Vector3f& rclNt) const;
    /** Calculates the intersection point of the line defined by the base \a rclPt and the direction
     * \a rclDir with the facet. The intersection must be inside the facet. If there is no
     * intersection false is returned.
     */
    bool IntersectWithLine(const Base::Vector3f& rclPt,
                           const Base::Vector3f& rclDir,
                           Base::Vector3f& rclRes) const;
    /** Calculates the intersection point of the line defined by the base \a rclPt and the direction
     * \a rclDir with the facet. The intersection must be inside the facet. If there is no
     * intersection false is returned. This does actually the same as IntersectWithLine() with one
     * additionally constraint that the angle between the direction of the line and the normal of
     * the plane must not exceed \a fMaxAngle.
     */
    bool Foraminate(const Base::Vector3f& rclPt,
                    const Base::Vector3f& rclDir,
                    Base::Vector3f& rclRes,
                    float fMaxAngle = Mathf::PI) const;
    /** Checks if the facet intersects with the plane defined by the base \a rclBase and the normal
     * \a rclNormal and returns true if two points are found, false otherwise.
     */
    bool IntersectWithPlane(const Base::Vector3f& rclBase,
                            const Base::Vector3f& rclNormal,
                            Base::Vector3f& rclP1,
                            Base::Vector3f& rclP2) const;
    /**
     * Checks if the facet intersects with the plane defined by the base \a rclBase and the normal
     * \a rclNormal.
     */
    inline bool IntersectWithPlane(const Base::Vector3f& rclBase,
                                   const Base::Vector3f& rclNormal) const;
    /** Checks if the plane defined by the facet \a rclFacet intersects with the line defined by the
     * base \a rclBase and the direction \a rclNormal and returns the intersection point \a rclRes
     * if possible.
     */
    bool IntersectPlaneWithLine(const Base::Vector3f& rclBase,
                                const Base::Vector3f& rclNormal,
                                Base::Vector3f& rclRes) const;
    /** Calculates the volume of the prism defined by two facets.
     * \note The two facets must not intersect.
     */
    float VolumeOfPrism(const MeshGeomFacet& rclF) const;
    /** Subsamples the facet into points with resolution \a fStep. */
    void SubSample(float fStep, std::vector<Base::Vector3f>& rclPoints) const;
    /** Calculates the center and radius of the inscribed circle of the facet. */
    float CenterOfInscribedCircle(Base::Vector3f& rclCenter) const;
    /** Calculates the center and radius of the circum circle of the facet. */
    float CenterOfCircumCircle(Base::Vector3f& rclCenter) const;
    /** Returns the edge number of the facet that is nearest to the point \a rclPt. */
    unsigned short NearestEdgeToPoint(const Base::Vector3f& rclPt) const;
    /** Returns the edge number \a side of the facet and the distance to the edge that is nearest to
     * the point \a rclPt. */
    void
    NearestEdgeToPoint(const Base::Vector3f& rclPt, float& fDistance, unsigned short& side) const;
    /** Returns the edge for \a side. */
    MeshGeomEdge GetEdge(short side) const;
    /** The center and radius of the circum circle define a sphere in 3D. If the point \a rP is part
     * of this sphere true is returned, otherwise false.
     */
    bool IsPointOfSphere(const Base::Vector3f& rP) const;
    /** This is an overloaded member function, provided for convenience. It behaves essentially like
     * the above function. If one of the facet's points is inside the sphere true is returned,
     * otherwise false.
     */
    bool IsPointOfSphere(const MeshGeomFacet& rFacet) const;
    /** The aspect ratio is the longest edge length divided by its height.
     */
    float AspectRatio() const;
    /** The alternative aspect ration is the ratio of the radius of the circum-circle and twice the
     * radius of the in-circle.
     */
    float AspectRatio2() const;
    /** The roundness is in the range between 0.0 (colinear) and 1.0 (equilateral).
     */
    float Roundness() const;
    /** Apply a transformation on the triangle.
     */
    void Transform(const Base::Matrix4D&);
    /**
     * Checks if the two triangles are coplanar.
     */
    bool IsCoplanar(const MeshGeomFacet& facet) const;

private:
    mutable Base::Vector3f _clNormal; /**< Normal of the facet. */
    mutable bool _bNormalCalculated;  /**< True if the normal is already calculated. */

public:
    // NOLINTBEGIN
    Base::Vector3f _aclPoints[3]; /**< Geometric corner points. */
    unsigned char _ucFlag;        /**< Flag property */
    unsigned long _ulProp;        /**< Free usable property. */
                                  // NOLINTEND
};

using TMeshPointArray = std::vector<MeshPoint>;
/**
 * Stores all data points of the mesh structure.
 */
class MeshExport MeshPointArray: public TMeshPointArray
{
public:
    // Iterator interface
    using _TIterator = std::vector<MeshPoint>::iterator;
    using _TConstIterator = std::vector<MeshPoint>::const_iterator;

    /** @name Construction */
    //@{
    // constructor
    MeshPointArray() = default;
    // constructor
    explicit MeshPointArray(PointIndex ulSize)
        : TMeshPointArray(ulSize)
    {}
    /// copy-constructor
    MeshPointArray(const MeshPointArray&);
    MeshPointArray(MeshPointArray&&);
    // Destructor
    ~MeshPointArray() = default;
    //@}

    /** @name Flag state
     * @note All flag methods are const as they do NOT change the actual properties of the object
     */
    //@{
    /// Sets the flag for all points
    void SetFlag(MeshPoint::TFlagType tF) const;
    /// Resets the flag for all points
    void ResetFlag(MeshPoint::TFlagType tF) const;
    /// Sets all points invalid
    void ResetInvalid() const;
    /// Sets the property for all points
    void SetProperty(unsigned long ulVal) const;
    //@}

    // Assignment
    MeshPointArray& operator=(const MeshPointArray& rclPAry);
    MeshPointArray& operator=(MeshPointArray&& rclPAry);
    void Transform(const Base::Matrix4D&);
    /**
     * Searches for the first point index  Two points are equal if the distance is less
     * than EPSILON. If no such points is found POINT_INDEX_MAX is returned.
     */
    PointIndex Get(const MeshPoint& rclPoint);
    /**
     * Searches for the first point index  Two points are equal if the distance is less
     * than EPSILON. If no such points is found the point is added to the array at end
     * and its index is returned.
     */
    PointIndex GetOrAddIndex(const MeshPoint& rclPoint);
};

using TMeshFacetArray = std::vector<MeshFacet>;

/**
 * Stores all facets of the mesh data-structure.
 */
class MeshExport MeshFacetArray: public TMeshFacetArray
{
public:
    // Iterator interface
    using _TIterator = std::vector<MeshFacet>::iterator;
    using _TConstIterator = std::vector<MeshFacet>::const_iterator;

    /** @name Construction */
    //@{
    /// constructor
    MeshFacetArray() = default;
    /// constructor
    explicit MeshFacetArray(FacetIndex ulSize)
        : TMeshFacetArray(ulSize)
    {}
    /// copy-constructor
    MeshFacetArray(const MeshFacetArray&);
    MeshFacetArray(MeshFacetArray&&);
    /// destructor
    ~MeshFacetArray() = default;
    //@}

    /** @name Flag state
     * @note All flag methods are const as they do NOT change the actual properties
     * of the object
     */
    //@{
    /// Sets the flag for all facets.
    void SetFlag(MeshFacet::TFlagType tF) const;
    /// Resets the flag for all facets.
    void ResetFlag(MeshFacet::TFlagType tF) const;
    /// Sets all facets invalid
    void ResetInvalid() const;
    /// Sets the property for all facets
    void SetProperty(unsigned long ulVal) const;
    //@}

    // Assignment
    MeshFacetArray& operator=(const MeshFacetArray& rclFAry);
    MeshFacetArray& operator=(MeshFacetArray&& rclFAry);

    /**
     * Removes the facet from the array the iterator points to. All neighbour
     * indices of the other facets get adjusted.
     */
    void Erase(_TIterator pIter);
    /**
     * Checks and flips the point indices if needed. @see MeshFacet::Transpose().
     */
    void TransposeIndices(PointIndex ulOrig, PointIndex ulNew);
    /**
     * Decrements all point indices that are higher than \a ulIndex.
     */
    void DecrementIndices(PointIndex ulIndex);
};

/**
 * MeshPointModifier is a helper class that allows to modify the
 * point array of a mesh kernel but with limited access.
 */
class MeshExport MeshPointModifier
{
public:
    explicit MeshPointModifier(MeshPointArray& points)
        : rPoints(points)
    {}
    ~MeshPointModifier() = default;

    MeshPointArray& GetPoints() const
    {
        return rPoints;
    }

    MeshPointModifier(const MeshPointModifier& c) = default;
    MeshPointModifier(MeshPointModifier&& c) = default;

    MeshPointModifier& operator=(const MeshPointModifier& c) = delete;
    MeshPointModifier& operator=(MeshPointModifier&& c) = delete;

private:
    MeshPointArray& rPoints;
};

/**
 * MeshFacetModifier is a helper class that allows to modify the
 * facet array of a mesh kernel but with limited access.
 */
class MeshExport MeshFacetModifier
{
public:
    explicit MeshFacetModifier(MeshFacetArray& facets)
        : rFacets(facets)
    {}
    ~MeshFacetModifier() = default;

    MeshFacetModifier(const MeshFacetModifier& c) = default;
    MeshFacetModifier(MeshFacetModifier&& c) = default;
    MeshFacetModifier& operator=(const MeshFacetModifier& c) = delete;
    MeshFacetModifier& operator=(MeshFacetModifier&& c) = delete;

    /**
     * Replaces the index of the corner point of the facet at position \a pos
     * that is equal to \a old by \a now. If the facet does not have a corner
     * point with this index nothing happens.
     */
    void Transpose(PointIndex pos, PointIndex old, PointIndex now)
    {
        rFacets[pos].Transpose(old, now);
    }

private:
    MeshFacetArray& rFacets;
};

inline MeshPoint::MeshPoint(float x, float y, float z)
    : Base::Vector3f(x, y, z)
    , _ucFlag(0)
    , _ulProp(0)
{}

inline MeshPoint::MeshPoint(const Base::Vector3f& rclPt)
    : Base::Vector3f(rclPt)
    , _ucFlag(0)
    , _ulProp(0)
{}

inline bool MeshPoint::operator==(const MeshPoint& rclPt) const
{
    return Base::DistanceP2(*this, rclPt) < MeshDefinitions::_fMinPointDistanceP2;
}

inline bool MeshPoint::operator==(const Base::Vector3f& rclV) const
{
    return Base::DistanceP2(*this, rclV) < MeshDefinitions::_fMinPointDistanceP2;
}

inline bool MeshPoint::operator<(const MeshPoint& rclPt) const
{
    if (fabs(this->x - rclPt.x) >= MeshDefinitions::_fMinPointDistanceD1) {
        return this->x < rclPt.x;
    }
    if (fabs(this->y - rclPt.y) >= MeshDefinitions::_fMinPointDistanceD1) {
        return this->y < rclPt.y;
    }
    if (fabs(this->z - rclPt.z) >= MeshDefinitions::_fMinPointDistanceD1) {
        return this->z < rclPt.z;
    }
    return false;  // points are considered to be equal
}

inline float MeshGeomFacet::DistancePlaneToPoint(const Base::Vector3f& rclPoint) const
{
    // internal normal is forced to have length equal to 1
    return float(fabs(rclPoint.DistanceToPlane(_aclPoints[0], GetNormal())));
}

inline void MeshGeomFacet::CalcNormal() const
{
    _clNormal = (_aclPoints[1] - _aclPoints[0]) % (_aclPoints[2] - _aclPoints[0]);
    _clNormal.Normalize();
    _bNormalCalculated = true;
}

inline Base::Vector3f MeshGeomFacet::GetNormal() const
{
    if (!_bNormalCalculated) {
        CalcNormal();
    }
    return _clNormal;
}

inline void MeshGeomFacet::SetNormal(const Base::Vector3f& rclNormal)
{
    if (rclNormal.Sqr() == 0.0f) {
        return;
    }
    _clNormal = rclNormal;
    _clNormal.Normalize();
    _bNormalCalculated = true;
}

inline void MeshGeomFacet::ArrangeNormal(const Base::Vector3f& rclN)
{
    // force internal normal to be computed if not done yet
    if ((rclN * GetNormal()) < 0.0f) {
        _clNormal = -_clNormal;
    }
}

inline Base::Vector3f MeshGeomFacet::GetGravityPoint() const
{
    return (1.0f / 3.0f) * (_aclPoints[0] + _aclPoints[1] + _aclPoints[2]);
}

inline void MeshGeomFacet::AdjustCirculationDirection()
{
    Base::Vector3f clN = (_aclPoints[1] - _aclPoints[0]) % (_aclPoints[2] - _aclPoints[0]);
    if ((clN * _clNormal) < 0.0f) {
        std::swap(_aclPoints[1], _aclPoints[2]);
    }
}

inline Base::BoundBox3f MeshGeomFacet::GetBoundBox() const
{
    return {_aclPoints, 3};
}

inline float MeshGeomFacet::Perimeter() const
{
    float perimeter = 0.0f;
    perimeter += Base::Distance(_aclPoints[0], _aclPoints[1]);
    perimeter += Base::Distance(_aclPoints[1], _aclPoints[2]);
    perimeter += Base::Distance(_aclPoints[2], _aclPoints[0]);
    return perimeter;
}

inline float MeshGeomFacet::Area() const
{
    return ((_aclPoints[1] - _aclPoints[0]) % (_aclPoints[2] - _aclPoints[0])).Length() / 2.0f;
}

inline bool MeshGeomFacet::ContainedByOrIntersectBoundingBox(const Base::BoundBox3f& rclBB) const
{
    // Test, if all corner points of the facet are on one of the 6 sides of the BB
    if (!(GetBoundBox() && rclBB)) {
        return false;
    }

    // Test, whether Facet-BB is completely within BB
    if (rclBB.IsInBox(GetBoundBox())) {
        return true;
    }

    // Test, whether one of the corner points is in BB
    for (auto pnt : _aclPoints) {
        if (rclBB.IsInBox(pnt)) {
            return true;
        }
    }

    // "real" test for cutting
    if (IntersectBoundingBox(rclBB)) {
        return true;
    }

    return false;
}

inline bool MeshGeomFacet::IntersectWithPlane(const Base::Vector3f& rclBase,
                                              const Base::Vector3f& rclNormal) const
{
    bool bD0 = (_aclPoints[0].DistanceToPlane(rclBase, rclNormal) > 0.0f);
    return !((bD0 == (_aclPoints[1].DistanceToPlane(rclBase, rclNormal) > 0.0f))
             && (bD0 == (_aclPoints[2].DistanceToPlane(rclBase, rclNormal) > 0.0f)));
}

inline MeshFacet::MeshFacet()  // NOLINT
    : _ucFlag(0)
    , _ulProp(0)
{
    memset(_aulNeighbours, 0xff, sizeof(FacetIndex) * 3);
    memset(_aulPoints, 0xff, sizeof(PointIndex) * 3);
}

inline MeshFacet::MeshFacet(PointIndex p1,
                            PointIndex p2,
                            PointIndex p3,
                            FacetIndex n1,
                            FacetIndex n2,
                            FacetIndex n3)
    : _ucFlag(0)
    , _ulProp(0)
    , _aulPoints {p1, p2, p3}
    , _aulNeighbours {n1, n2, n3}
{}

void MeshFacet::SetVertices(PointIndex p1, PointIndex p2, PointIndex p3)
{
    _aulPoints[0] = p1;
    _aulPoints[1] = p2;
    _aulPoints[2] = p3;
}

void MeshFacet::SetNeighbours(FacetIndex n1, FacetIndex n2, FacetIndex n3)
{
    _aulNeighbours[0] = n1;
    _aulNeighbours[1] = n2;
    _aulNeighbours[2] = n3;
}

inline void MeshFacet::GetEdge(unsigned short usSide, MeshHelpEdge& rclEdge) const
{
    rclEdge._ulIndex[0] = _aulPoints[usSide];
    rclEdge._ulIndex[1] = _aulPoints[(usSide + 1) % 3];
}

inline std::pair<PointIndex, PointIndex> MeshFacet::GetEdge(unsigned short usSide) const
{
    return {_aulPoints[usSide], _aulPoints[(usSide + 1) % 3]};
}

inline void MeshFacet::Transpose(PointIndex ulOrig, PointIndex ulNew)
{
    if (_aulPoints[0] == ulOrig) {
        _aulPoints[0] = ulNew;
    }
    else if (_aulPoints[1] == ulOrig) {
        _aulPoints[1] = ulNew;
    }
    else if (_aulPoints[2] == ulOrig) {
        _aulPoints[2] = ulNew;
    }
}

inline void MeshFacet::Decrement(PointIndex ulIndex)
{
    if (_aulPoints[0] > ulIndex) {
        _aulPoints[0]--;
    }
    if (_aulPoints[1] > ulIndex) {
        _aulPoints[1]--;
    }
    if (_aulPoints[2] > ulIndex) {
        _aulPoints[2]--;
    }
}

inline bool MeshFacet::HasPoint(PointIndex ulIndex) const
{
    if (_aulPoints[0] == ulIndex) {
        return true;
    }
    if (_aulPoints[1] == ulIndex) {
        return true;
    }
    if (_aulPoints[2] == ulIndex) {
        return true;
    }
    return false;
}

inline void MeshFacet::ReplaceNeighbour(FacetIndex ulOrig, FacetIndex ulNew)
{
    if (_aulNeighbours[0] == ulOrig) {
        _aulNeighbours[0] = ulNew;
    }
    else if (_aulNeighbours[1] == ulOrig) {
        _aulNeighbours[1] = ulNew;
    }
    else if (_aulNeighbours[2] == ulOrig) {
        _aulNeighbours[2] = ulNew;
    }
}

inline unsigned short MeshFacet::CountOpenEdges() const
{
    unsigned short ct = 0;
    for (unsigned short i = 0; i < 3; i++) {
        if (!HasNeighbour(i)) {
            ct++;
        }
    }
    return ct;
}

inline bool MeshFacet::HasOpenEdge() const
{
    return (CountOpenEdges() != 0);
}

inline bool MeshFacet::HasSameOrientation(const MeshFacet& f) const
{
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (_aulPoints[i] == f._aulPoints[j]) {
                if ((_aulPoints[(i + 1) % 3] == f._aulPoints[(j + 1) % 3])
                    || (_aulPoints[(i + 2) % 3] == f._aulPoints[(j + 2) % 3])) {
                    return false;
                }
            }
        }
    }

    return true;
}

inline bool MeshFacet::IsDegenerated() const
{
    if (_aulPoints[0] == _aulPoints[1]) {
        return true;
    }
    if (_aulPoints[1] == _aulPoints[2]) {
        return true;
    }
    if (_aulPoints[2] == _aulPoints[0]) {
        return true;
    }
    return false;
}

inline unsigned short MeshFacet::Side(FacetIndex ulNIndex) const
{
    if (_aulNeighbours[0] == ulNIndex) {
        return 0;
    }
    else if (_aulNeighbours[1] == ulNIndex) {
        return 1;
    }
    else if (_aulNeighbours[2] == ulNIndex) {
        return 2;
    }
    else {
        return USHRT_MAX;
    }
}

inline unsigned short MeshFacet::Side(PointIndex ulP0, PointIndex ulP1) const
{
    if (_aulPoints[0] == ulP0) {
        if (_aulPoints[1] == ulP1) {
            return 0;  // Edge 0-1 ==> 0
        }
        else if (_aulPoints[2] == ulP1) {
            return 2;  // Edge 0-2 ==> 2
        }
    }
    else if (_aulPoints[1] == ulP0) {
        if (_aulPoints[0] == ulP1) {
            return 0;  // Edge 1-0 ==> 0
        }
        else if (_aulPoints[2] == ulP1) {
            return 1;  // Edge 1-2 ==> 1
        }
    }
    else if (_aulPoints[2] == ulP0) {
        if (_aulPoints[0] == ulP1) {
            return 2;  // Edge 2-0 ==> 2
        }
        else if (_aulPoints[1] == ulP1) {
            return 1;  // Edge 2-1 ==> 1
        }
    }

    return USHRT_MAX;
}

inline unsigned short MeshFacet::Side(const MeshFacet& rFace) const
{
    unsigned short side {};
    for (int i = 0; i < 3; i++) {
        side = Side(rFace._aulPoints[i], rFace._aulPoints[(i + 1) % 3]);
        if (side != USHRT_MAX) {
            return side;
        }
    }

    return USHRT_MAX;
}

inline bool MeshFacet::IsEqual(const MeshFacet& rcFace) const
{
    for (int i = 0; i < 3; i++) {
        if (this->_aulPoints[0] == rcFace._aulPoints[i]) {
            if (this->_aulPoints[1] == rcFace._aulPoints[(i + 1) % 3]
                && this->_aulPoints[2] == rcFace._aulPoints[(i + 2) % 3]) {
                return true;
            }
            else if (this->_aulPoints[1] == rcFace._aulPoints[(i + 2) % 3]
                     && this->_aulPoints[2] == rcFace._aulPoints[(i + 1) % 3]) {
                return true;
            }
        }
    }

    return false;
}

/**
 * Binary function to query the flags for use with generic STL functions.
 */
template<class TCLASS>
class MeshIsFlag
{
public:
    using first_argument_type = TCLASS;
    using second_argument_type = typename TCLASS::TFlagType;
    using result_type = bool;
    bool operator()(const TCLASS& rclElem, typename TCLASS::TFlagType tFlag) const
    {
        return rclElem.IsFlag(tFlag);
    }
};

/**
 * Binary function to query the flags for use with generic STL functions.
 */
template<class TCLASS>
class MeshIsNotFlag
{
public:
    using first_argument_type = TCLASS;
    using second_argument_type = typename TCLASS::TFlagType;
    using result_type = bool;
    bool operator()(const TCLASS& rclElem, typename TCLASS::TFlagType tFlag) const
    {
        return !rclElem.IsFlag(tFlag);
    }
};

/**
 * Binary function to set the flags for use with generic STL functions.
 */
template<class TCLASS>
class MeshSetFlag
{
public:
    using first_argument_type = TCLASS;
    using second_argument_type = typename TCLASS::TFlagType;
    using result_type = bool;
    bool operator()(const TCLASS& rclElem, typename TCLASS::TFlagType tFlag) const
    {
        rclElem.SetFlag(tFlag);
        return true;
    }
};

/**
 * Binary function to reset the flags for use with generic STL functions.
 */
template<class TCLASS>
class MeshResetFlag
{
public:
    using first_argument_type = TCLASS;
    using second_argument_type = typename TCLASS::TFlagType;
    using result_type = bool;
    bool operator()(const TCLASS& rclElem, typename TCLASS::TFlagType tFlag) const
    {
        rclElem.ResetFlag(tFlag);
        return true;
    }
};

}  // namespace MeshCore

#endif  // MESH_ELEMENTS_H
