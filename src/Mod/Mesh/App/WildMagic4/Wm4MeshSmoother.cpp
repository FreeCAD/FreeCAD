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
#include "Wm4MeshSmoother.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
MeshSmoother<Real>::MeshSmoother ()
{
    m_iVQuantity = 0;
    m_akVertex = nullptr;
    m_iTQuantity = 0;
    m_aiIndex = nullptr;
    m_akNormal = nullptr;
    m_akMean = nullptr;
    m_aiNeighborCount = nullptr;
}
//----------------------------------------------------------------------------
template <class Real>
MeshSmoother<Real>::MeshSmoother (int iVQuantity, Vector3<Real>* akVertex,
    int iTQuantity, const int* aiIndex)
{
    m_akVertex = nullptr;
    m_akNormal = nullptr;
    m_aiIndex = nullptr;
    m_akMean = nullptr;
    m_aiNeighborCount = nullptr;

    Create(iVQuantity,akVertex,iTQuantity,aiIndex);
}
//----------------------------------------------------------------------------
template <class Real>
MeshSmoother<Real>::~MeshSmoother ()
{
    Destroy();
}
//----------------------------------------------------------------------------
template <class Real>
void MeshSmoother<Real>::Create (int iVQuantity, Vector3<Real>* akVertex,
    int iTQuantity, const int* aiIndex)
{
    // remove previously allocated smoother data
    Destroy();

    m_iVQuantity = iVQuantity;
    m_akVertex = akVertex;
    m_iTQuantity = iTQuantity;
    m_aiIndex = aiIndex;

    m_akNormal = WM4_NEW Vector3<Real>[m_iVQuantity];
    m_akMean = WM4_NEW Vector3<Real>[m_iVQuantity];
    m_aiNeighborCount = WM4_NEW int[m_iVQuantity];

    // count the number of vertex neighbors
    memset(m_aiNeighborCount,0,m_iVQuantity*sizeof(int));
    const int* piIndex = m_aiIndex;
    for (int i = 0; i < m_iTQuantity; i++)
    {
        m_aiNeighborCount[*piIndex++] += 2;
        m_aiNeighborCount[*piIndex++] += 2;
        m_aiNeighborCount[*piIndex++] += 2;
    }
}
//----------------------------------------------------------------------------
template <class Real>
void MeshSmoother<Real>::Destroy ()
{
    WM4_DELETE[] m_akNormal;
    WM4_DELETE[] m_akMean;
    WM4_DELETE[] m_aiNeighborCount;
}
//----------------------------------------------------------------------------
template <class Real>
void MeshSmoother<Real>::Update (Real fTime)
{
    std::fill_n(m_akNormal,m_iVQuantity,Vector3<Real>{0,0,0});
    std::fill_n(m_akMean,m_iVQuantity,Vector3<Real>{0,0,0});

    const int* piIndex = m_aiIndex;
    int i;
    for (i = 0; i < m_iTQuantity; i++)
    {
        int iV0 = *piIndex++;
        int iV1 = *piIndex++;
        int iV2 = *piIndex++;

        Vector3<Real>& rkV0 = m_akVertex[iV0];
        Vector3<Real>& rkV1 = m_akVertex[iV1];
        Vector3<Real>& rkV2 = m_akVertex[iV2];

        Vector3<Real> kEdge1 = rkV1 - rkV0;
        Vector3<Real> kEdge2 = rkV2 - rkV0;
        Vector3<Real> kNormal = kEdge1.Cross(kEdge2);

        m_akNormal[iV0] += kNormal;
        m_akNormal[iV1] += kNormal;
        m_akNormal[iV2] += kNormal;

        m_akMean[iV0] += rkV1 + rkV2;
        m_akMean[iV1] += rkV2 + rkV0;
        m_akMean[iV2] += rkV0 + rkV1;
    }

    for (i = 0; i < m_iVQuantity; i++)
    {
        m_akNormal[i].Normalize();
        m_akMean[i] /= (Real)m_aiNeighborCount[i];
    }

    for (i = 0; i < m_iVQuantity; i++)
    {
        if (VertexInfluenced(i,fTime))
        {
            Vector3<Real> kLocalDiff = m_akMean[i] - m_akVertex[i];
            Vector3<Real> kSurfaceNormal = kLocalDiff.Dot(m_akNormal[i]) *
                m_akNormal[i];
            Vector3<Real> kTangent = kLocalDiff - kSurfaceNormal;

            Real fTWeight = GetTangentWeight(i,fTime);
            Real fNWeight = GetNormalWeight(i,fTime);
            m_akVertex[i] += fTWeight*kTangent + fNWeight*m_akNormal[i];
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
bool MeshSmoother<Real>::VertexInfluenced (int, Real)
{
    return true;
}
//----------------------------------------------------------------------------
template <class Real>
Real MeshSmoother<Real>::GetTangentWeight (int, Real)
{
    return (Real)0.5;
}
//----------------------------------------------------------------------------
template <class Real>
Real MeshSmoother<Real>::GetNormalWeight (int, Real)
{
    return (Real)0.0;
}
//----------------------------------------------------------------------------
template <class Real>
int MeshSmoother<Real>::GetVQuantity () const
{
    return m_iVQuantity;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>* MeshSmoother<Real>::GetVertices () const
{
    return m_akVertex;
}
//----------------------------------------------------------------------------
template <class Real>
int MeshSmoother<Real>::GetTQuantity () const
{
    return m_iTQuantity;
}
//----------------------------------------------------------------------------
template <class Real>
const int* MeshSmoother<Real>::GetIndices () const
{
    return m_aiIndex;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>* MeshSmoother<Real>::GetNormals () const
{
    return m_akNormal;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>* MeshSmoother<Real>::GetMeans () const
{
    return m_akMean;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class MeshSmoother<float>;

template WM4_FOUNDATION_ITEM
class MeshSmoother<double>;
//----------------------------------------------------------------------------
}
