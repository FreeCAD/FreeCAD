// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 4.10.0 (2009/11/18)

#ifndef WM4CONTBOX3_H
#define WM4CONTBOX3_H

#include "Wm4FoundationLIB.h"
#include "Wm4Box3.h"
#include "Wm4Query.h"

namespace Wm4
{

// Compute the smallest axis-aligned bounding box of the points.
template <class Real> WM4_FOUNDATION_ITEM
Box3<Real> ContAlignedBox (int iQuantity, const Vector3<Real>* akPoint);

// Compute an oriented bounding box of the points.  The box center is the
// average of the points.  The box axes are the eigenvectors of the covariance
// matrix.
template <class Real> WM4_FOUNDATION_ITEM
Box3<Real> ContOrientedBox (int iQuantity, const Vector3<Real>* akPoint);

// Compute a minimum volume oriented box containing the specified points.
template <class Real> WM4_FOUNDATION_ITEM
Box3<Real> ContMinBox (int iQuantity, const Vector3<Real>* akPoint,
    Real fEpsilon, Query::Type eQueryType);

// Test for containment.  Let X = C + y0*U0 + y1*U1 + y2*U2 where C is the
// box center and U0, U1, U2 are the orthonormal axes of the box.  X is in
// the box if |y_i| <= E_i for all i where E_i are the extents of the box.
template <class Real> WM4_FOUNDATION_ITEM
bool InBox (const Vector3<Real>& rkPoint, const Box3<Real>& rkBox);

// Construct an oriented box that contains two other oriented boxes.  The
// result is not guaranteed to be the minimum volume box containing the
// input boxes.
template <class Real> WM4_FOUNDATION_ITEM
Box3<Real> MergeBoxes (const Box3<Real>& rkBox0, const Box3<Real>& rkBox1);

}

#endif
