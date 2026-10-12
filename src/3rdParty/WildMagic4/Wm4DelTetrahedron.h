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
#include "Wm4Query3.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM DelTetrahedron
{
public:
    DelTetrahedron (int iV0, int iV1, int iV2, int iV3);

    bool IsInsertionComponent (int i, DelTetrahedron* pkAdj,
        const Query3<Real>* pkQuery, const int* aiSupervertex);
    int DetachFrom (int iAdj, DelTetrahedron* pkAdj);

    int V[4];
    DelTetrahedron* A[4];
    int Time;
    bool IsComponent;
    bool OnStack;
};

typedef DelTetrahedron<float> DelTetrahedronf;
typedef DelTetrahedron<double> DelTetrahedrond;

}