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
#include "Wm4DelPolyhedronFace.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
DelPolyhedronFace<Real>::DelPolyhedronFace (int iV0, int iV1, int iV2,
    int iNullIndex, DelTetrahedron<Real>* pkTetra)
    :
    ETManifoldMesh::Triangle(iV0,iV1,iV2)
{
    NullIndex = iNullIndex;
    Tetra = pkTetra;
}
//----------------------------------------------------------------------------
template <class Real>
ETManifoldMesh::TPtr DelPolyhedronFace<Real>::TCreator (int iV0, int iV1,
    int iV2)
{
    return WM4_NEW DelPolyhedronFace<Real>(iV0,iV1,iV2,0,nullptr);
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class DelPolyhedronFace<float>;

template WM4_FOUNDATION_ITEM
class DelPolyhedronFace<double>;
//----------------------------------------------------------------------------
}
