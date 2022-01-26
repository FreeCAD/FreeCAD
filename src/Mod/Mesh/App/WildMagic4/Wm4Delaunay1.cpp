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
#include "Wm4Delaunay1.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
Delaunay1<Real>::Delaunay1 (int iVertexQuantity, Real* afVertex,
    Real fEpsilon, bool bOwner, Query::Type eQueryType)
    :
    Delaunay<Real>(iVertexQuantity,fEpsilon,bOwner,eQueryType)
{
    assert(afVertex);
    m_afVertex = afVertex;

    std::vector<SortedVertex> kArray(m_iVertexQuantity);
    int i;
    for (i = 0; i < m_iVertexQuantity; i++)
    {
        kArray[i].Value = m_afVertex[i];
        kArray[i].Index = i;
    }
    std::sort(kArray.begin(),kArray.end());

    Real fRange = kArray[m_iVertexQuantity-1].Value - kArray[0].Value;
    if (fRange >= m_fEpsilon)
    {
        m_iDimension = 1;
        m_iSimplexQuantity = m_iVertexQuantity - 1;
        m_aiIndex = WM4_NEW int[2*m_iSimplexQuantity];
        for (i = 0; i < m_iSimplexQuantity; i++)
        {
            m_aiIndex[2*i] = kArray[i].Index;
            m_aiIndex[2*i+1] = kArray[i+1].Index;
        }

        m_aiAdjacent = WM4_NEW int[2*m_iSimplexQuantity];
        for (i = 0; i < m_iSimplexQuantity; i++)
        {
            m_aiAdjacent[2*i] = i-1;
            m_aiAdjacent[2*i+1] = i+1;
        }
        m_aiAdjacent[2*m_iSimplexQuantity-1] = -1;
    }
}
//----------------------------------------------------------------------------
template <class Real>
Delaunay1<Real>::~Delaunay1 ()
{
    if (m_bOwner)
    {
        WM4_DELETE[] m_afVertex;
    }
}
//----------------------------------------------------------------------------
template <class Real>
const Real* Delaunay1<Real>::GetVertices () const
{
    return m_afVertex;
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay1<Real>::GetHull (int aiIndex[2])
{
    assert(m_iDimension == 1);
    if (m_iDimension != 1)
    {
        return false;
    }

    aiIndex[0] = m_aiIndex[0];
    aiIndex[1] = m_aiIndex[2*m_iSimplexQuantity-1];
    return true;
}
//----------------------------------------------------------------------------
template <class Real>
int Delaunay1<Real>::GetContainingSegment (const Real fP) const
{
    assert(m_iDimension == 1);
    if (m_iDimension != 1)
    {
        return -1;
    }

    if (fP < m_afVertex[m_aiIndex[0]])
    {
        return -1;
    }

    if (fP > m_afVertex[m_aiIndex[2*m_iSimplexQuantity-1]])
    {
        return -1;
    }

    int i;
    for (i = 0; i < m_iSimplexQuantity; i++)
    {
        if (fP < m_afVertex[m_aiIndex[2*i+1]])
        {
            break;
        }
    }

    assert(i < m_iSimplexQuantity);
    return i;
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay1<Real>::GetVertexSet (int i, Real afV[2]) const
{
    assert(m_iDimension == 1);
    if (m_iDimension != 1)
    {
        return false;
    }

    if (0 <= i && i < m_iSimplexQuantity)
    {
        afV[0] = m_afVertex[m_aiIndex[2*i]];
        afV[1] = m_afVertex[m_aiIndex[2*i+1]];
        return true;
    }
    return false;
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay1<Real>::GetIndexSet (int i, int aiIndex[2]) const
{
    assert(m_iDimension == 1);
    if (m_iDimension != 1)
    {
        return false;
    }

    if (0 <= i && i < m_iSimplexQuantity)
    {
        aiIndex[0] = m_aiIndex[2*i];
        aiIndex[1] = m_aiIndex[2*i+1];
        return true;
    }
    return false;
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay1<Real>::GetAdjacentSet (int i, int aiAdjacent[2]) const
{
    assert(m_iDimension == 1);
    if (m_iDimension != 1)
    {
        return false;
    }

    if (0 <= i && i < m_iSimplexQuantity)
    {
        aiAdjacent[0] = m_aiAdjacent[2*i];
        aiAdjacent[1] = m_aiAdjacent[2*i+1];
        return true;
    }
    return false;
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay1<Real>::GetBarycentricSet (int i, const Real fP, Real afBary[2])
    const
{
    assert(m_iDimension == 1);
    if (m_iDimension != 1)
    {
        return false;
    }

    if (0 <= i && i < m_iSimplexQuantity)
    {
        Real fV0 = m_afVertex[m_aiIndex[2*i]];
        Real fV1 = m_afVertex[m_aiIndex[2*i+1]];
        Real fDenom = fV1 - fV0;
        if (fDenom > m_fEpsilon)
        {
            afBary[0] = (fV1 - fP)/fDenom;
        }
        else
        {
            afBary[0] = (Real)1.0;
        }

        afBary[1] = (Real)1.0 - afBary[0];
        return true;
    }
    return false;
}
//----------------------------------------------------------------------------
template <class Real>
Delaunay1<Real>::Delaunay1 (const char* acFilename)
    :
    Delaunay<Real>(0,(Real)0.0,false,Query::QT_REAL)
{
    m_afVertex = nullptr;
    bool bLoaded = Load(acFilename);
    assert(bLoaded);
    (void)bLoaded;  // avoid warning in Release build
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay1<Real>::Load (const char* acFilename)
{
    FILE* pkIFile = System::Fopen(acFilename,"rb");
    if (!pkIFile)
    {
        return false;
    }

    Delaunay<Real>::Load(pkIFile);

    if (m_bOwner)
    {
        WM4_DELETE[] m_afVertex;
    }

    m_bOwner = true;
    m_afVertex = WM4_NEW Real[m_iVertexQuantity];

    size_t uiSize = sizeof(Real);
    if (uiSize == 4)
    {
        System::Read4le(pkIFile,m_iVertexQuantity,m_afVertex);
    }
    else // uiSize == 8
    {
        System::Read8le(pkIFile,m_iVertexQuantity,m_afVertex);
    }

    System::Fclose(pkIFile);
    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay1<Real>::Save (const char* acFilename) const
{
    FILE* pkOFile = System::Fopen(acFilename,"wb");
    if (!pkOFile)
    {
        return false;
    }

    Delaunay<Real>::Save(pkOFile);

    size_t uiSize = sizeof(Real);
    if (uiSize == 4)
    {
        System::Write4le(pkOFile,m_iVertexQuantity,m_afVertex);
    }
    else // uiSize == 8
    {
        System::Write8le(pkOFile,m_iVertexQuantity,m_afVertex);
    }

    System::Fclose(pkOFile);
    return true;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class Delaunay1<float>;

template WM4_FOUNDATION_ITEM
class Delaunay1<double>;
//----------------------------------------------------------------------------
}
