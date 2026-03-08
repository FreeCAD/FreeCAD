// SPDX-License-Identifier: LGPL-2.1-or-later

// Wild Magic Source Code
// David Eberly
// http://www.geometrictools.com
// Copyright (c) 1998-2007
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or (at
// your option) any later version.  The license is available for reading at
// either of the locations:
//     http://www.gnu.org/copyleft/lgpl.html
//     http://www.geometrictools.com/License/WildMagicLicense.pdf
// The license applies to versions 0 through 4 of Wild Magic.
//
// Version: 4.0.0 (2006/06/28)

#pragma once

#include "Wm4FoundationLIB.h"
#include "Wm4Vector3.h"

namespace Wm4
{

template <class Real>
class Mapper3
{
public:
    // Construction and destruction.  The value of epsilon is used as a
    // relative error when computing the dimension of the point set.
    Mapper3 (int iVQuantity, const Vector3<Real>* akVertex, Real fEpsilon);
    ~Mapper3 ();

    // Axis-aligned bounding box of the input points.
    const Vector3<Real>& GetMin () const;
    const Vector3<Real>& GetMax () const;
    Real GetMaxRange () const;

    // Dimension d of the set (0, 1, 2, or 3).
    int GetDimension () const;

    // Coordinate system.  The origin is valid for any dimension d.  The
    // unit-length direction vector is valid only for 0 <= i < d.  The extreme
    // index is relative to the array of input points, and is also valid only
    // for 0 <= i < d.  If d = 0, all points are effectively the same, but the
    // use of an epsilon may lead to an extreme index that is not zero.  If
    // d = 1, all points effectively lie on a line segment.  The extreme
    // indices correspond to input points that are the end points of the
    // segment.  If d = 2, the first two extreme indices correspond to a
    // line segment.  The next extreme index corresponds to the input point
    // that is farthest from this line segment in a direction perpendicular
    // to the segment.  If d = 3, the first three extreme indices correspond
    // to a triangle.  The next extreme index corresponds to the input point
    // that is farthest from this triangle in a direction perpendicular to
    // the triangle.
    const Vector3<Real>& GetOrigin () const;
    const Vector3<Real>& GetDirection (int i) const;
    int GetExtremeIndex (int i) const;

    // If d = 3, the direction vectors {U0,U1,U2} form a right-handed set.
    // The four extreme points form a tetrahedron.  This function indicates
    // if that tetrahedron is counterclockwise ordered.
    bool GetExtremeCCW () const;

private:
    // Axis-aligned bounding box of input points.  The maximum range is the
    // larger of max[0]-min[0], max[1]-min[1], and max[2]-min[2].
    Vector3<Real> m_kMin, m_kMax;
    Real m_fMaxRange;

    // The intrinsic dimension of the input set.  The parameter fEpsilon to
    // the constructor is used to provide a tolerance when determining the
    // dimension.
    int m_iDimension;

    // The indices that define the maximum dimensional extents.  The values
    // m_aiExtreme[0] and m_aiExtreme[1] are the indices for the vertices
    // which define the largest extent in one of the coordinate axis
    // directions.  If the intrinsic dimensionality is 2, then m_aiExtreme[2]
    // is the index for the vertex which causes the largest extent in the
    // direction perpendicular to the line through the vertices corresponding
    // to m_aiExtreme[0] and m_aiExtreme[1].  Furthermore, if the intrinsic
    // dimensionality is 3, then m_aiExtreme[3] is the index for the vertex
    // which causes the largest extent in the direction perpendicular to the
    // triangle defined by the other extreme points.  The tetrahedron
    // <V[extreme0],V[extreme1],V[extreme2],V[extreme3]> can be clockwise or
    // counterclockwise, the condition stored in m_bExtremeCCW.
    int m_aiExtreme[4];
    bool m_bExtremeCCW;

    // See the comments describing the member functions which return these
    // values.
    Vector3<Real> m_kOrigin;
    Vector3<Real> m_akDirection[3];
};

} //namespace Wm4

#include "Wm4Mapper3.inl"

namespace Wm4
{
typedef Mapper3<float> Mapper3f;
typedef Mapper3<double> Mapper3d;
}