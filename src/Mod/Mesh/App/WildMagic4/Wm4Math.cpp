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

#include "Wm4FoundationPCH.h"
#include "Wm4Math.h"

namespace Wm4 {

template<> WM4_FOUNDATION_ITEM const float Math<float>::EPSILON = FLT_EPSILON;
template<> WM4_FOUNDATION_ITEM const float Math<float>::ZERO_TOLERANCE = 1e-06f;
template<> WM4_FOUNDATION_ITEM const float Math<float>::MAX_REAL = FLT_MAX;
template<> WM4_FOUNDATION_ITEM const float Math<float>::PI = (float)(4.0*atan(1.0));
template<> WM4_FOUNDATION_ITEM const float Math<float>::TWO_PI = 2.0f*Math<float>::PI;
template<> WM4_FOUNDATION_ITEM const float Math<float>::HALF_PI = 0.5f*Math<float>::PI;
template<> WM4_FOUNDATION_ITEM const float Math<float>::INV_PI = 1.0f/Math<float>::PI;
template<> WM4_FOUNDATION_ITEM const float Math<float>::INV_TWO_PI = 1.0f/Math<float>::TWO_PI;
template<> WM4_FOUNDATION_ITEM const float Math<float>::DEG_TO_RAD = Math<float>::PI/180.0f;
template<> WM4_FOUNDATION_ITEM const float Math<float>::RAD_TO_DEG = 180.0f/Math<float>::PI;
template<> WM4_FOUNDATION_ITEM const float Math<float>::LN_2 = Math<float>::Log(2.0f);
template<> WM4_FOUNDATION_ITEM const float Math<float>::LN_10 = Math<float>::Log(10.0f);
template<> WM4_FOUNDATION_ITEM const float Math<float>::INV_LN_2 = 1.0f/Math<float>::LN_2;
template<> WM4_FOUNDATION_ITEM const float Math<float>::INV_LN_10 = 1.0f/Math<float>::LN_10;

template<> WM4_FOUNDATION_ITEM const double Math<double>::EPSILON = DBL_EPSILON;
template<> WM4_FOUNDATION_ITEM const double Math<double>::ZERO_TOLERANCE = 1e-08;
template<> WM4_FOUNDATION_ITEM const double Math<double>::MAX_REAL = DBL_MAX;
template<> WM4_FOUNDATION_ITEM const double Math<double>::PI = 4.0*atan(1.0);
template<> WM4_FOUNDATION_ITEM const double Math<double>::TWO_PI = 2.0*Math<double>::PI;
template<> WM4_FOUNDATION_ITEM const double Math<double>::HALF_PI = 0.5*Math<double>::PI;
template<> WM4_FOUNDATION_ITEM const double Math<double>::INV_PI = 1.0/Math<double>::PI;
template<> WM4_FOUNDATION_ITEM const double Math<double>::INV_TWO_PI = 1.0/Math<double>::TWO_PI;
template<> WM4_FOUNDATION_ITEM const double Math<double>::DEG_TO_RAD = Math<double>::PI/180.0;
template<> WM4_FOUNDATION_ITEM const double Math<double>::RAD_TO_DEG = 180.0/Math<double>::PI;
template<> WM4_FOUNDATION_ITEM const double Math<double>::LN_2 = Math<double>::Log(2.0);
template<> WM4_FOUNDATION_ITEM const double Math<double>::LN_10 = Math<double>::Log(10.0);
template<> WM4_FOUNDATION_ITEM const double Math<double>::INV_LN_2 = 1.0/Math<double>::LN_2;
template<> WM4_FOUNDATION_ITEM const double Math<double>::INV_LN_10 = 1.0/Math<double>::LN_10;

//----------------------------------------------------------------------------
//Does not compile with gcc 4.1.2
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
template <>
float Math<float>::FastInvSqrt (float fValue)
{
    float fHalf = 0.5f*fValue;
    int i  = *(int*)&fValue;
    i = 0x5f3759df - (i >> 1);
    fValue = *(float*)&i;
    fValue = fValue*(1.5f - fHalf*fValue*fValue);
    return fValue;
}
//----------------------------------------------------------------------------
template <>
double Math<double>::FastInvSqrt (double dValue)
{
    double dHalf = 0.5*dValue;
    Integer64 i  = *(Integer64*)&dValue;
#if defined(WM4_USING_VC70) || defined(WM4_USING_VC6)
    i = 0x5fe6ec85e7de30da - (i >> 1);
#else
    i = 0x5fe6ec85e7de30daLL - (i >> 1);
#endif
    dValue = *(double*)&i;
    dValue = dValue*(1.5 - dHalf*dValue*dValue);
    return dValue;
}
#endif
//----------------------------------------------------------------------------
}
