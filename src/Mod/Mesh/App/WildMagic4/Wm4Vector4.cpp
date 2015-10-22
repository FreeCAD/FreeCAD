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
#include "Wm4Vector4.h"

namespace Wm4 {

template<> const Vector4<float> Vector4<float>::ZERO(0.0f,0.0f,0.0f,0.0f);
template<> const Vector4<float> Vector4<float>::UNIT_X(1.0f,0.0f,0.0f,0.0f);
template<> const Vector4<float> Vector4<float>::UNIT_Y(0.0f,1.0f,0.0f,0.0f);
template<> const Vector4<float> Vector4<float>::UNIT_Z(0.0f,0.0f,1.0f,0.0f);
template<> const Vector4<float> Vector4<float>::UNIT_W(0.0f,0.0f,0.0f,1.0f);
template<> const Vector4<float> Vector4<float>::ONE(1.0f,1.0f,1.0f,1.0f);

template<> const Vector4<double> Vector4<double>::ZERO(0.0,0.0,0.0,0.0);
template<> const Vector4<double> Vector4<double>::UNIT_X(1.0,0.0,0.0,0.0);
template<> const Vector4<double> Vector4<double>::UNIT_Y(0.0,1.0,0.0,0.0);
template<> const Vector4<double> Vector4<double>::UNIT_Z(0.0,0.0,1.0,0.0);
template<> const Vector4<double> Vector4<double>::UNIT_W(0.0,0.0,0.0,1.0);
template<> const Vector4<double> Vector4<double>::ONE(1.0,1.0,1.0,1.0);

}
