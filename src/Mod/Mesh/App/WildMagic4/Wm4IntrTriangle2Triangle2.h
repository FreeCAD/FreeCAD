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
#include "Wm4Intersector.h"
#include "Wm4Intersector1.h"
#include "Wm4Triangle2.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM IntrTriangle2Triangle2
    : public Intersector<Real,Vector2<Real> >
{
public:
    IntrTriangle2Triangle2 (const Triangle2<Real>& rkTriangle0,
        const Triangle2<Real>& rkTriangle1);

    // object access
    const Triangle2<Real>& GetTriangle0 () const;
    const Triangle2<Real>& GetTriangle1 () const;

    // static queries
    virtual bool Test ();
    virtual bool Find ();

    // dynamic queries
    virtual bool Test (Real fTMax, const Vector2<Real>& rkVelocity0,
        const Vector2<Real>& rkVelocity1);
    virtual bool Find (Real fTMax, const Vector2<Real>& rkVelocity0,
        const Vector2<Real>& rkVelocity1);

    // information about the intersection set
    int GetQuantity () const;
    const Vector2<Real>& GetPoint (int i) const;

private:
    using Intersector<Real,Vector2<Real> >::m_fContactTime;

    static int WhichSide (const Vector2<Real> akV[3],
        const Vector2<Real>& rkP, const Vector2<Real>& rkD);

    static void ClipConvexPolygonAgainstLine (const Vector2<Real>& rkN,
        Real fC, int& riQuantity, Vector2<Real> akV[6]);

    enum ProjectionMap
    {
        M21,  // 2 vertices map to min, 1 vertex maps to max
        M12,  // 1 vertex maps to min, 2 vertices map to max
        M11   // 1 vertex maps to min, 1 vertex maps to max
    };

    class Configuration
    {
    public:
        ProjectionMap Map;  // how vertices map to the projection interval
        int Index[3];       // the sorted indices of the vertices
        Real Min, Max;      // the interval is [min,max]
    };

    void ComputeTwo (Configuration& rkCfg, const Vector2<Real> akV[3],
        const Vector2<Real>& rkD, int iI0, int iI1, int iI2);

    void ComputeThree (Configuration& rkCfg, const Vector2<Real> akV[3],
        const Vector2<Real>& rkD, const Vector2<Real>& rkP);

    static bool NoIntersect (const Configuration& rkCfg0,
        const Configuration& rkCfg1, Real fTMax, Real fSpeed, int& riSide,
        Configuration& rkTCfg0, Configuration& rkTCfg1, Real& rfTFirst,
        Real& rfTLast);

    static void GetIntersection (const Configuration& rkCfg0,
        const Configuration& rkCfg1, int iSide, const Vector2<Real> akV0[3],
        const Vector2<Real> akV1[3], int& riQuantity,
        Vector2<Real> akVertex[6]);

    // the objects to intersect
    const Triangle2<Real>& m_rkTriangle0;
    const Triangle2<Real>& m_rkTriangle1;

    // information about the intersection set
    int m_iQuantity;
    Vector2<Real> m_akPoint[6];
};

typedef IntrTriangle2Triangle2<float> IntrTriangle2Triangle2f;
typedef IntrTriangle2Triangle2<double> IntrTriangle2Triangle2d;

}