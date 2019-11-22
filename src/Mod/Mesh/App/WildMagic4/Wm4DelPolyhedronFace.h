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

#ifndef WM4DELPOLYHEDRONFACE_H
#define WM4DELPOLYHEDRONFACE_H

#include "Wm4FoundationLIB.h"
#include "Wm4ETManifoldMesh.h"
#include "Wm4DelTetrahedron.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM DelPolyhedronFace : public ETManifoldMesh::Triangle
{
public:
    DelPolyhedronFace (int iV0 = -1, int iV1 = -1, int iV2 = -1,
        int iNullIndex = -1, DelTetrahedron<Real>* pkTri = 0);

    static ETManifoldMesh::TPtr TCreator (int iV0, int iV1, int iV2);

    int NullIndex;
    DelTetrahedron<Real>* Tetra;
};

typedef DelPolyhedronFace<float> DelPolyhedronFacef;
typedef DelPolyhedronFace<double> DelPolyhedronFaced;

}

#endif
