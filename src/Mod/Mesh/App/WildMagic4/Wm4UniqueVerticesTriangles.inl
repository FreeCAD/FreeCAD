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

namespace Wm4
{
//----------------------------------------------------------------------------
template <int N, class Real>
UniqueVerticesTriangles<N,Real>::UniqueVerticesTriangles (int iTQuantity,
    const TTuple<N,Real>* akInVertex, int& riOutVQuantity,
    TTuple<N,Real>*& rakOutVertex, int*& raiOutIndex)
{
    assert(iTQuantity > 0 && akInVertex);

    ConstructUniqueVertices(3*iTQuantity,akInVertex,riOutVQuantity,
        rakOutVertex);

    // The input index array is implicitly {<0,1,2>,<3,4,5>,...,<n-3,n-2,n-1>}
    // where n is the number of vertices.  The output index array is the same
    // as the mapping array.
    int iIQuantity = 3*iTQuantity;
    raiOutIndex = WM4_NEW int[iIQuantity];
    size_t uiSize = iIQuantity*sizeof(int);
    System::Memcpy(raiOutIndex,uiSize,m_aiInToOutMapping,uiSize);
}
//----------------------------------------------------------------------------
template <int N, class Real>
UniqueVerticesTriangles<N,Real>::UniqueVerticesTriangles (int iInVQuantity,
    const TTuple<N,Real>* akInVertex, int iTQuantity, const int* aiInIndex,
    int& riOutVQuantity, TTuple<N,Real>*& rakOutVertex, int*& raiOutIndex)
{
    assert(iInVQuantity > 0 && akInVertex && iTQuantity > 0 && aiInIndex);

    ConstructUniqueVertices(iInVQuantity,akInVertex,riOutVQuantity,
        rakOutVertex);

    // The input index array needs it indices mapped to the unique vertex
    // indices.
    int iIQuantity = 3*iTQuantity;
    raiOutIndex = WM4_NEW int[iIQuantity];
    for (int i = 0; i < iIQuantity; i++)
    {
        assert(0 <= aiInIndex[i] && aiInIndex[i] < iInVQuantity);
        raiOutIndex[i] = m_aiInToOutMapping[aiInIndex[i]];
        assert(0 <= raiOutIndex[i] && raiOutIndex[i] < riOutVQuantity);
    }
}
//----------------------------------------------------------------------------
template <int N, class Real>
UniqueVerticesTriangles<N,Real>::~UniqueVerticesTriangles ()
{
    WM4_DELETE[] m_aiInToOutMapping;
}
//----------------------------------------------------------------------------
template <int N, class Real>
int UniqueVerticesTriangles<N,Real>::GetOutputIndexFor (int iInputIndex) const
{
    assert(0 <= iInputIndex && iInputIndex < m_iInVQuantity);
    return m_aiInToOutMapping[iInputIndex];
}
//----------------------------------------------------------------------------
template <int N, class Real>
void UniqueVerticesTriangles<N,Real>::ConstructUniqueVertices (
    int iInVQuantity, const TTuple<N,Real>* akInVertex, int& riOutVQuantity,
    TTuple<N,Real>*& rakOutVertex)
{
    // Construct the unique vertices.
    m_iInVQuantity = iInVQuantity;
    m_aiInToOutMapping = WM4_NEW int[m_iInVQuantity];
    std::map<TTuple<N,Real>,int> kTable;
    m_iOutVQuantity = 0;
    typename std::map<TTuple<N,Real>,int>::iterator pkIter;
    for (int i = 0; i < m_iInVQuantity; i++)
    {
        pkIter = kTable.find(akInVertex[i]);
        if (pkIter != kTable.end())
        {
            // Vertex i is a duplicate of one inserted earlier into the
            // table.  Map vertex i to the first-found copy.
            m_aiInToOutMapping[i] = pkIter->second;
        }
        else
        {
            // Vertex i is the first occurrence of such a point.
            kTable.insert(std::make_pair(akInVertex[i],m_iOutVQuantity));
            m_aiInToOutMapping[i] = m_iOutVQuantity;
            m_iOutVQuantity++;
        }
    }

    // Pack the unique vertices into an array in the correct order.
    riOutVQuantity = m_iOutVQuantity;
    rakOutVertex = WM4_NEW TTuple<N,Real>[m_iOutVQuantity];
    for (pkIter = kTable.begin(); pkIter != kTable.end(); pkIter++)
    {
        assert(0 <= pkIter->second && pkIter->second < m_iOutVQuantity);
        rakOutVertex[pkIter->second] = pkIter->first;
    }
}
//----------------------------------------------------------------------------
} //namespace Wm4
