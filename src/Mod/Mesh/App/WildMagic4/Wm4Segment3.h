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
class Segment3
{
public:
    // The segment is represented as P+t*D, where P is the segment origin,
    // D is a unit-length direction vector and |t| <= e.  The value e is
    // referred to as the extent of the segment.  The end points of the
    // segment are P-e*D and P+e*D.  The user must ensure that the direction
    // vector is unit-length.  The representation for a segment is analogous
    // to that for an oriented bounding box.  P is the center, D is the
    // axis direction, and e is the extent.

    // construction
    Segment3 ();  // uninitialized
    Segment3 (const Vector3<Real>& rkOrigin, const Vector3<Real>& rkDirection,
        Real fExtent);

    // end points
    Vector3<Real> GetPosEnd () const;  // P+e*D
    Vector3<Real> GetNegEnd () const;  // P-e*D

    Vector3<Real> Origin, Direction;
    Real Extent;
};

}

#include "Wm4Segment3.inl"

namespace Wm4
{
typedef Segment3<float> Segment3f;
typedef Segment3<double> Segment3d;

}