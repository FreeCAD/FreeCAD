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
// Version: 4.0.1 (2006/07/25)

#pragma once

#include "Wm4FoundationLIB.h"
#include "Wm4Intersector.h"
#include "Wm4Triangle3.h"
#include "Wm4Line3.h"
#include "Wm4Plane3.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM IntrTriangle3Triangle3
    : public Intersector<Real,Vector3<Real> >
{
public:
    IntrTriangle3Triangle3 (const Triangle3<Real>& rkTriangle0,
        const Triangle3<Real>& rkTriangle1);

    // object access
    const Triangle3<Real>& GetTriangle0 () const;
    const Triangle3<Real>& GetTriangle1 () const;

    bool ReportCoplanarIntersections;  // default 'true'

    // static queries
    virtual bool Test ();
    virtual bool Find ();

    // dynamic queries
    virtual bool Test (Real fTMax, const Vector3<Real>& rkVelocity0,
        const Vector3<Real>& rkVelocity1);
    virtual bool Find (Real fTMax, const Vector3<Real>& rkVelocity0,
        const Vector3<Real>& rkVelocity1);

    // information about the intersection set
    int GetQuantity () const;
    const Vector3<Real>& GetPoint (int i) const;

private:
    using Intersector<Real,Vector3<Real> >::m_fContactTime;

    static void ProjectOntoAxis (const Triangle3<Real>& rkTri,
        const Vector3<Real>& rkAxis, Real& rfMin, Real& rfMax);

    static void TrianglePlaneRelations (const Triangle3<Real>& rkTriangle,
        const Plane3<Real>& rkPlane, Real afDistance[3], int aiSign[3],
        int& riPositive, int& riNegative, int& riZero);

    static void GetInterval (const Triangle3<Real>& rkTriangle,
        const Line3<Real>& rkLine, const Real afDistance[3],
        const int aiSign[3], Real afParam[2]);

    bool ContainsPoint (const Triangle3<Real>& rkTriangle,
        const Plane3<Real>& rkPlane, const Vector3<Real>& rkPoint);

    bool IntersectsSegment (const Triangle3<Real>& rkTriangle,
        const Vector3<Real>& rkEnd0, const Vector3<Real>& rkEnd1);

    bool GetCoplanarIntersection (const Plane3<Real>& rkPlane,
        const Triangle3<Real>& rkTri0, const Triangle3<Real>& rkTri1);

    static bool TestOverlap (const Vector3<Real>& rkAxis, Real fTMax,
        Real fSpeed, Real fUMin, Real fUMax, Real fVMin, Real fVMax,
        Real& rfTFirst, Real& rfTLast);

    bool TestOverlap (const Vector3<Real>& rkAxis, Real fTMax,
        const Vector3<Real>& rkVelocity, Real& rfTFirst, Real& rfTLast);

    enum ProjectionMap
    {
        M2, M11,                // lines
        M3, M21, M12, M111,     // triangles
        M44, M2_2, M1_1         // boxes
    };

    enum ContactSide
    {
        CS_LEFT,
        CS_RIGHT,
        CS_NONE
    };

    class WM4_FOUNDATION_ITEM Configuration
    {
    public:
        ProjectionMap Map;  // how vertices map to the projection interval
        int Index[8];       // the sorted indices of the vertices
        Real Min, Max;      // the interval is [min,max]
    };

    static void ProjectOntoAxis (const Triangle3<Real>& rkTri,
        const Vector3<Real>& rkAxis, Configuration& rkCfg);

    bool FindOverlap (const Vector3<Real>& rkAxis, Real fTMax, Real fSpeed,
        const Configuration& rkUC, const Configuration& rkVC,
        ContactSide& reSide, Configuration& rkTUC, Configuration& rkTVC,
        Real& rfTFirst, Real& rfTLast);

    bool FindOverlap (const Vector3<Real>& rkAxis, Real fTMax,
        const Vector3<Real>& rkVelocity, ContactSide& reSide,
        Configuration& rkTCfg0, Configuration& rkTCfg1, Real& rfTFirst,
        Real& rfTLast);

    void FindContactSet (const Triangle3<Real>& rkTri0,
        const Triangle3<Real>& rkTri1, ContactSide& reSide,
        Configuration& rkCfg0, Configuration& rkCfg1);

    void GetEdgeEdgeIntersection (const Vector3<Real>& rkU0,
        const Vector3<Real>& rkU1, const Vector3<Real>& rkV0,
        const Vector3<Real>& rkV1);

    // the objects to intersect
    const Triangle3<Real>& m_rkTriangle0;
    const Triangle3<Real>& m_rkTriangle1;

    // information about the intersection set
    int m_iQuantity;
    Vector3<Real> m_akPoint[6];
};

typedef IntrTriangle3Triangle3<float> IntrTriangle3Triangle3f;
typedef IntrTriangle3Triangle3<double> IntrTriangle3Triangle3d;

}