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

#include "Wm4FoundationPCH.h"
#include "Wm4Vector3.h"

namespace Wm4 {

template<> const Vector3<float> Vector3<float>::ZERO(0.0f,0.0f,0.0f);
template<> const Vector3<float> Vector3<float>::UNIT_X(1.0f,0.0f,0.0f);
template<> const Vector3<float> Vector3<float>::UNIT_Y(0.0f,1.0f,0.0f);
template<> const Vector3<float> Vector3<float>::UNIT_Z(0.0f,0.0f,1.0f);
template<> const Vector3<float> Vector3<float>::ONE(1.0f,1.0f,1.0f);

/// @cond DOXERR
template<> const Vector3<double> Vector3<double>::ZERO(0.0,0.0,0.0);
template<> const Vector3<double> Vector3<double>::UNIT_X(1.0,0.0,0.0);
template<> const Vector3<double> Vector3<double>::UNIT_Y(0.0,1.0,0.0);
template<> const Vector3<double> Vector3<double>::UNIT_Z(0.0,0.0,1.0);
template<> const Vector3<double> Vector3<double>::ONE(1.0,1.0,1.0);
/// @endcond

}
