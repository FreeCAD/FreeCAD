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
#include "Wm4Math.h"

namespace Wm4
{

template <class Real>
class LinComp
{
public:
    // abstract base class
    virtual ~LinComp ();

    // The linear component is represented as P+t*D where P is the component
    // origin and D is a unit-length direction vector.  The user must ensure
    // that the direction vector satisfies this condition.  The t-intervals
    // for lines, rays, segments, points, or the empty set are described
    // later.

    // component type
    enum
    {
        CT_EMPTY,
        CT_POINT,
        CT_SEGMENT,
        CT_RAY,
        CT_LINE
    };

    int GetType () const;

    // The interval of restriction for t, as defined above.  The function
    // SetInterval(min,max) sets the t-interval; it handles all possible
    // inputs according to the following scheme:
    //   CT_LINE:
    //     [-MAX_REAL,MAX_REAL]
    //   CT_RAY:
    //     [min,MAX_REAL], where min is finite
    //     [-MAX_REAL,max], where max is finite
    //   CT_SEGMENT:
    //     [min,max], where min and max are finite with min < max
    //   CT_POINT:
    //     [min,max], where min and max are finite with min = max
    //   CT_EMPTY:
    //     [min,max], where min > max or min = max = MAX_REAL or
    //                min = max = -MAX_REAL
    void SetInterval (Real fMin, Real fMax);

    // Determine the type of an interval without having to create an instance
    // of a LinComp object.
    static int GetTypeFromInterval (Real fMin, Real fMax);

    // The canonical intervals are [-MAX_REAL,MAX_REAL] for a line;
    // [0,MAX_REAL] for a ray; [-e,e] for a segment, where e > 0; [0,0] for
    // a point, and [MAX_REAL,-MAX_REAL] for the empty set.  If the interval
    // is [min,max], the adjustments are as follows.
    // 
    // CT_RAY:  If max is MAX_REAL and if min is not zero, then P is modified
    // to P' = P+min*D so that the ray is represented by P'+t*D for t >= 0.
    // If min is -MAX_REAL and max is finite, then the origin and direction
    // are modified to P' = P+max*D and D' = -D.
    //
    // CT_SEGMENT:  If min is not -max, then P is modified to
    // P' = P + ((min+max)/2)*D and the extent is e' = (max-min)/2.
    //
    // CT_POINT:  If min is not zero, the P is modified to P' = P+min*D.
    //
    // CT_EMPTY:  Set max to -MAX_REAL and min to MAX_REAL.
    //
    // The first function is virtual since the updates are dependent on the
    // dimension of the vector space.
    virtual void MakeCanonical () = 0;
    bool IsCanonical () const;

    // access the interval [min,max]
    Real GetMin () const;
    Real GetMax () const;

    // Determine if the specified parameter is in the interval.
    bool Contains (Real fParam) const;

protected:
    LinComp ();  // default is CT_NONE

    // assignment
    LinComp& operator= (const LinComp& rkComponent);

    // component type
    int m_iType;

    // the interval of restriction for t
    Real m_fMin, m_fMax;
};

} //namespace Wm4

#include "Wm4LinComp.inl"

namespace Wm4
{
typedef LinComp<float> LinCompf;
typedef LinComp<double> LinCompd;
}