// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 4.10.0 (2009/11/18)

//----------------------------------------------------------------------------
template <class Real>
Box2<Real>::Box2 ()
{
    // uninitialized
}
//----------------------------------------------------------------------------
template <class Real>
Box2<Real>::Box2 (const Vector2<Real>& rkCenter, const Vector2<Real>* akAxis,
    const Real* afExtent)
    :
    Center(rkCenter)
{
    for (int i = 0; i < 2; i++)
    {
        Axis[i] = akAxis[i];
        Extent[i] = afExtent[i];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Box2<Real>::Box2 (const Vector2<Real>& rkCenter, const Vector2<Real>& rkAxis0,
    const Vector2<Real>& rkAxis1, Real fExtent0, Real fExtent1)
    :
    Center(rkCenter)
{
    Axis[0] = rkAxis0;
    Axis[1] = rkAxis1;
    Extent[0] = fExtent0;
    Extent[1] = fExtent1;
}
//----------------------------------------------------------------------------
template <class Real>
void Box2<Real>::ComputeVertices (Vector2<Real> akVertex[4]) const
{
    Vector2<Real> akEAxis[2] =
    {
        Axis[0]*Extent[0],
        Axis[1]*Extent[1]
    };

    akVertex[0] = Center - akEAxis[0] - akEAxis[1];
    akVertex[1] = Center + akEAxis[0] - akEAxis[1];
    akVertex[2] = Center + akEAxis[0] + akEAxis[1];
    akVertex[3] = Center - akEAxis[0] + akEAxis[1];
}
//----------------------------------------------------------------------------
