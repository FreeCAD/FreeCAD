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
#include "Wm4DelTetrahedron.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
DelTetrahedron<Real>::DelTetrahedron (int iV0, int iV1, int iV2, int iV3)
{
    V[0] = iV0;
    V[1] = iV1;
    V[2] = iV2;
    V[3] = iV3;
    A[0] = nullptr;
    A[1] = nullptr;
    A[2] = nullptr;
    A[3] = nullptr;
    Time = -1;
    IsComponent = false;
    OnStack = false;
}
//----------------------------------------------------------------------------
template <class Real>
bool DelTetrahedron<Real>::IsInsertionComponent (int i, DelTetrahedron* pkAdj,
    const Query3<Real>* pkQuery, const int* aiSupervertex)
{
    // Indexing for the vertices of the triangle opposite a vertex.  The
    // triangle opposite vertex j is
    //   <aaiIndex[j][0], aaiIndex[j][1], aaiIndex[j][2]>
    // and is listed in counterclockwise order when viewed from outside the
    // tetrahedron.
    const int aaiIndex[4][3] = { {1,2,3}, {0,3,2}, {0,1,3}, {0,2,1} };

    if (i != Time)
    {
        Time = i;

        // Determine if the circumsphere of the tetrahedron contains the
        // input point.
        int iRelation = pkQuery->ToCircumsphere(i,V[0],V[1],V[2],V[3]);
        IsComponent = (iRelation <= 0 ? true : false);
        if (IsComponent)
        {
            return true;
        }

        // It is possible that a tetrahedron that shares a supervertex does
        // not have the circumsphere-containing property, but all faces of
        // it (other than the shared one with the calling tetrahedron) are
        // visible.  These are also included in the insertion polyhedron.
        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                if (V[j] == aiSupervertex[k])
                {
                    // Tetrahedron shares a supervertex.  It is safe to reuse
                    // k as a loop index because we are returning from the
                    // function.
                    int iNumInvisible = 0;
                    for (k = 0; k < 4; k++)
                    {
                        if (A[k] != pkAdj)
                        {
                            int iV0 = V[aaiIndex[k][0]];
                            int iV1 = V[aaiIndex[k][1]];
                            int iV2 = V[aaiIndex[k][2]];
                            iRelation = pkQuery->ToPlane(i,iV0,iV1,iV2);
                            if (iRelation > 0)
                            {
                                iNumInvisible++;
                            }
                        }
                    }
                    IsComponent = (iNumInvisible == 0 ? true : false);
                    return IsComponent;
                }
            }
        }
    }

    return IsComponent;
}
//----------------------------------------------------------------------------
template <class Real>
int DelTetrahedron<Real>::DetachFrom (int iAdj, DelTetrahedron* pkAdj)
{
    assert(0 <= iAdj && iAdj < 4 && A[iAdj] == pkAdj);
    A[iAdj] = nullptr;
    for (int i = 0; i < 4; i++)
    {
        if (pkAdj->A[i] == this)
        {
            pkAdj->A[i] = nullptr;
            return i;
        }
    }
    return -1;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class DelTetrahedron<float>;

template WM4_FOUNDATION_ITEM
class DelTetrahedron<double>;
//----------------------------------------------------------------------------
}
