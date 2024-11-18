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

#ifndef WM4SPHERE3_H
#define WM4SPHERE3_H

#include "Wm4FoundationLIB.h"
#include "Wm4Vector3.h"

namespace Wm4
{

template <class Real>
class Sphere3
{
public:
    // The sphere is represented as |X-C| = R where C is the center and R is
    // the radius.

    Sphere3 ();  // uninitialized
    Sphere3 (const Vector3<Real>& rkCenter, Real fRadius);
    Sphere3 (const Sphere3& rkSphere);

    // assignment
    Sphere3& operator= (const Sphere3& rkSphere);

    Vector3<Real> Center;
    Real Radius{};
};

}

#include "Wm4Sphere3.inl"

namespace Wm4
{
typedef Sphere3<float> Sphere3f;
typedef Sphere3<double> Sphere3d;

}

#endif
