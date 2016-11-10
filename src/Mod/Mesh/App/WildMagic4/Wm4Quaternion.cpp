// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 4.10.0 (2009/11/18)

#include "Wm4FoundationPCH.h"
#include "Wm4Quaternion.h"

namespace Wm4
{
template<> const Quaternion<float>
    Quaternion<float>::IDENTITY(1.0f,0.0f,0.0f,0.0f);
template<> const Quaternion<float>
    Quaternion<float>::ZERO(0.0f,0.0f,0.0f,0.0f);
template<> int Quaternion<float>::ms_iNext[3] = { 1, 2, 0 };
template<> float Quaternion<float>::ms_fTolerance = 1e-06f;
template<> float Quaternion<float>::ms_fRootTwo = (float)sqrt(2.0);
template<> float Quaternion<float>::ms_fRootHalf = (float)sqrt(0.5);

template<> const Quaternion<double>
    Quaternion<double>::IDENTITY(1.0,0.0,0.0,0.0);
template<> const Quaternion<double>
    Quaternion<double>::ZERO(0.0,0.0,0.0,0.0);
template<> int Quaternion<double>::ms_iNext[3] = { 1, 2, 0 };
template<> double Quaternion<double>::ms_fTolerance = 1e-08;
template<> double Quaternion<double>::ms_fRootTwo = sqrt(2.0);
template<> double Quaternion<double>::ms_fRootHalf = sqrt(0.5);
}
