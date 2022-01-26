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
// Version: 4.0.5 (2006/10/23)

#include "Wm4FoundationPCH.h"
#include "Wm4TriangulateEC.h"
#include "Wm4Query2Filtered.h"
#include "Wm4Query2Int64.h"
#include "Wm4Query2TInteger.h"
#include "Wm4Query2TRational.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
TriangulateEC<Real>::TriangulateEC (const Positions& rkPositions,
    Query::Type eQueryType, Real fEpsilon, Indices& rkTriangles)
{
    // No extra elements are needed for triangulating a simple polygon.
    InitializePositions(rkPositions,eQueryType,fEpsilon,0);

    // Triangulate the unindexed polygon.
    int iVQuantity = (int)rkPositions.size();
    const int* aiIndex = nullptr;
    InitializeVertices(iVQuantity,aiIndex,rkTriangles);
    DoEarClipping(iVQuantity,aiIndex,rkTriangles);
}
//----------------------------------------------------------------------------
template <class Real>
TriangulateEC<Real>::TriangulateEC (const Positions& rkPositions,
    Query::Type eQueryType, Real fEpsilon, const Indices& rkPolygon,
    Indices& rkTriangles)
{
    // No extra elements are needed for triangulating a simple polygon.
    InitializePositions(rkPositions,eQueryType,fEpsilon,0);

    // Triangulate the indexed polygon.
    int iVQuantity = (int)rkPolygon.size();
    const int* aiIndex = &rkPolygon[0];
    InitializeVertices(iVQuantity,aiIndex,rkTriangles);
    DoEarClipping(iVQuantity,aiIndex,rkTriangles);
}
//----------------------------------------------------------------------------
template <class Real>
TriangulateEC<Real>::TriangulateEC (const Positions& rkPositions,
    Query::Type eQueryType, Real fEpsilon, const Indices& rkOuter,
    const Indices& rkInner, Indices& rkTriangles)
{
    // Two extra elements are needed to duplicate the endpoints of the edge
    // introduced to combine outer and inner polygons.
    InitializePositions(rkPositions,eQueryType,fEpsilon,2);

    // Combine the outer polygon and the inner polygon into a simple polygon
    // by inserting two edges connecting mutually visible vertices, one from
    // the outer polygon and one from the inner polygon.
    int iNextElement = (int)rkPositions.size();  // next available element
    IndexMap kMap;
    Indices kCombined;
    CombinePolygons(eQueryType,fEpsilon,iNextElement,rkOuter,rkInner,kMap,
        kCombined);

    // The combined polygon is now in the format of a simple polygon, albeit
    // one with coincident edges.
    int iVQuantity = (int)kCombined.size();
    const int* aiIndex = &kCombined[0];
    InitializeVertices(iVQuantity,aiIndex,rkTriangles);
    DoEarClipping(iVQuantity,aiIndex,rkTriangles);

    // Map the duplicate indices back to the original indices.
    RemapIndices(kMap,rkTriangles);
}
//----------------------------------------------------------------------------
template <class Real>
TriangulateEC<Real>::TriangulateEC (const Positions& rkPositions,
    Query::Type eQueryType, Real fEpsilon, const Indices& rkOuter,
    const IndicesArray& rkInners, Indices& rkTriangles)
{
    // Two extra elements per inner polygon are needed to duplicate the
    // endpoints of the edges introduced to combine outer and inner polygons.
    int iNumInners = (int)rkInners.size();
    int iExtraElements = 2*iNumInners;
    InitializePositions(rkPositions,eQueryType,fEpsilon,iExtraElements);

    // Combine the outer polygon and the inner polygons into a simple polygon
    // by inserting two edges per inner polygon connecting mutually visible
    // vertices.
    int iNextElement = (int)rkPositions.size();
    Indices kCombined;
    IndexMap kMap;
    ProcessOuterAndInners(eQueryType,fEpsilon,rkOuter,rkInners,iNextElement,
        kMap,kCombined);

    // The combined polygon is now in the format of a simple polygon, albeit
    // with coincident edges.
    int iVQuantity = (int)kCombined.size();
    const int* aiIndex = &kCombined[0];
    InitializeVertices(iVQuantity,aiIndex,rkTriangles);
    DoEarClipping(iVQuantity,aiIndex,rkTriangles);

    // Map the duplicate indices back to the original indices.
    RemapIndices(kMap,rkTriangles);
}
//----------------------------------------------------------------------------
template <class Real>
TriangulateEC<Real>::TriangulateEC (const Positions& rkPositions,
    Query::Type eQueryType, Real fEpsilon, const Tree* pkTree,
    Indices& rkTriangles)
{
    // Two extra elements per inner polygon are needed to duplicate the
    // endpoints of the edges introduced to combine outer and inner polygons.
    int iExtraElements = GetExtraElements(pkTree);
    InitializePositions(rkPositions,eQueryType,fEpsilon,iExtraElements);

    int iNextElement = (int)rkPositions.size();
    IndexMap kMap;

    std::queue<const Tree*> kQueue;
    kQueue.push(pkTree);
    while (kQueue.size() > 0)
    {
        const Tree* pkOuterNode = kQueue.front();
        kQueue.pop();

        int iNumChildren = (int)pkOuterNode->Child.size();
        int iVQuantity;
        const int* aiIndex;

        if (iNumChildren == 0)
        {
            // The outer polygon is a simple polygon (no nested inner
            // polygons).  Triangulate the simple polygon.
            iVQuantity = (int)pkOuterNode->Polygon.size();
            aiIndex = &pkOuterNode->Polygon[0];
            InitializeVertices(iVQuantity,aiIndex,rkTriangles);
            DoEarClipping(iVQuantity,aiIndex,rkTriangles);
        }
        else
        {
            // Place the next level of outer polygon nodes on the queue for
            // triangulation.
            std::vector<std::vector<int>*> kInners(iNumChildren);
            for (int i = 0; i < iNumChildren; i++)
            {
                const Tree* pkInnerNode = pkOuterNode->Child[i];
                kInners[i] = (std::vector<int>*)&pkInnerNode->Polygon;
                int iNumGrandChildren = (int)pkInnerNode->Child.size();
                for (int j = 0; j < iNumGrandChildren; j++)
                {
                    kQueue.push(pkInnerNode->Child[j]);
                }
            }

            // Combine the outer polygon and the inner polygons into a
            // simple polygon by inserting two edges per inner polygon
            // connecting mutually visible vertices.
            std::vector<int> kCombined;
            ProcessOuterAndInners(eQueryType,fEpsilon,pkOuterNode->Polygon,
                kInners,iNextElement,kMap,kCombined);

            // The combined polygon is now in the format of a simple polygon,
            // albeit with coincident edges.
            iVQuantity = (int)kCombined.size();
            aiIndex = &kCombined[0];
            InitializeVertices(iVQuantity,aiIndex,rkTriangles);
            DoEarClipping(iVQuantity,aiIndex,rkTriangles);
        }
    }

    // Map the duplicate indices back to the original indices.
    RemapIndices(kMap,rkTriangles);
}
//----------------------------------------------------------------------------
template <class Real>
TriangulateEC<Real>::~TriangulateEC ()
{
    WM4_DELETE m_pkQuery;
}
//----------------------------------------------------------------------------
template <class Real>
void TriangulateEC<Real>::InitializePositions (const Positions& rkPositions,
    Query::Type eQueryType, Real fEpsilon, int iExtraElements)
{
    int iPQuantity = (int)rkPositions.size();
    assert(iPQuantity >= 3);
    int iPEQuantity = iPQuantity + iExtraElements;
    m_kSPositions.resize(iPEQuantity);

    if (eQueryType == Query::QT_FILTERED)
    {
        assert((Real)0.0 <= fEpsilon && fEpsilon <= (Real)1.0);
    }

    Vector2<Real> kMin, kMax, kRange;
    Real fScale, fRMax;
    int i;

    switch (eQueryType)
    {
    case Query::QT_INT64:
        // Transform the vertices to the square [0,2^{20}]^2.
        Vector2<Real>::ComputeExtremes(iPQuantity,&rkPositions[0],kMin,kMax);
        kRange = kMax - kMin;
        fRMax = (kRange[0] >= kRange[1] ? kRange[0] : kRange[1]);
        fScale = ((Real)(1 << 20))/fRMax;
        for (i = 0; i < iPQuantity; i++)
        {
            m_kSPositions[i] = (rkPositions[i] - kMin)*fScale;
        }
        m_pkQuery = WM4_NEW Query2Int64<Real>(iPEQuantity,&m_kSPositions[0]);
        return;

    case Query::QT_INTEGER:
        // Transform the vertices to the square [0,2^{24}]^2.
        Vector2<Real>::ComputeExtremes(iPQuantity,&rkPositions[0],kMin,kMax);
        kRange = kMax - kMin;
        fRMax = (kRange[0] >= kRange[1] ? kRange[0] : kRange[1]);
        fScale = ((Real)(1 << 24))/fRMax;
        for (i = 0; i < iPQuantity; i++)
        {
            m_kSPositions[i] = (rkPositions[i] - kMin)*fScale;
        }
        m_pkQuery = WM4_NEW Query2TInteger<Real>(iPEQuantity,
            &m_kSPositions[0]);
        return;

    case Query::QT_REAL:
        // Transform the vertices to the square [0,1]^2.
        Vector2<Real>::ComputeExtremes(iPQuantity,&rkPositions[0],kMin,kMax);
        kRange = kMax - kMin;
        fRMax = (kRange[0] >= kRange[1] ? kRange[0] : kRange[1]);
        fScale = ((Real)1.0)/fRMax;
        for (i = 0; i < iPQuantity; i++)
        {
            m_kSPositions[i] = (rkPositions[i] - kMin)*fScale;
        }
        m_pkQuery = WM4_NEW Query2<Real>(iPEQuantity,&m_kSPositions[0]);
        return;

    case Query::QT_RATIONAL:
        // No transformation of the input data.  Make a copy that can be
        // expanded when triangulating polygons with holes.
        for (i = 0; i < iPQuantity; i++)
        {
            m_kSPositions[i] = rkPositions[i];
        }
        m_pkQuery = WM4_NEW Query2TRational<Real>(iPEQuantity,
            &m_kSPositions[0]);
        return;

    case Query::QT_FILTERED:
        // No transformation of the input data.  Make a copy that can be
        // expanded when triangulating polygons with holes.
        for (i = 0; i < iPQuantity; i++)
        {
            m_kSPositions[i] = rkPositions[i];
        }
        m_pkQuery = WM4_NEW Query2Filtered<Real>(iPEQuantity,
            &m_kSPositions[0],fEpsilon);
        return;
    }

    assert(false);
}
//----------------------------------------------------------------------------
template <class Real>
void TriangulateEC<Real>::InitializeVertices (int iVQuantity,
    const int* aiIndex, std::vector<int>& rkTriangle)
{
    (void)rkTriangle;
    m_kVertex.clear();
    m_kVertex.resize(iVQuantity);
    m_iCFirst = -1;
    m_iCLast = -1;
    m_iRFirst = -1;
    m_iRLast = -1;
    m_iEFirst = -1;
    m_iELast = -1;

    // Create a circular list of the polygon vertices for dynamic removal of
    // vertices.
    int iVQm1 = iVQuantity - 1;
    int i;
    for (i = 0; i <= iVQm1; i++)
    {
        Vertex& rkV = V(i);
        rkV.Index = (aiIndex ? aiIndex[i] : i);
        rkV.VPrev = (i > 0 ? i-1 : iVQm1);
        rkV.VNext = (i < iVQm1 ? i+1 : 0);
    }

    // Create a circular list of the polygon vertices for dynamic removal of
    // vertices.  Keep track of two linear sublists, one for the convex
    // vertices and one for the reflex vertices.  This is an O(N) process
    // where N is the number of polygon vertices.
    for (i = 0; i <= iVQm1; i++)
    {
        if (IsConvex(i))
        {
            InsertAfterC(i);
        }
        else
        {
            InsertAfterR(i);
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
void TriangulateEC<Real>::DoEarClipping (int iVQuantity, const int* aiIndex,
    std::vector<int>& rkTriangle)
{
    // If the polygon is convex, just create a triangle fan.
    int i;
    if (m_iRFirst == -1)
    {
        int iVQm1 = iVQuantity - 1;
        if (aiIndex)
        {
            for (i = 1; i < iVQm1; i++)
            {
                rkTriangle.push_back(aiIndex[0]);
                rkTriangle.push_back(aiIndex[i]);
                rkTriangle.push_back(aiIndex[i+1]);
            }
        }
        else
        {
            for (i = 1; i < iVQm1; i++)
            {
                rkTriangle.push_back(0);
                rkTriangle.push_back(i);
                rkTriangle.push_back(i+1);
            }
        }
        return;
    }

    // Identify the ears and build a circular list of them.  Let V0, V1, and
    // V2 be consecutive vertices forming a triangle T.  The vertex V1 is an
    // ear if no other vertices of the polygon lie inside T.  Although it is
    // enough to show that V1 is not an ear by finding at least one other
    // vertex inside T, it is sufficient to search only the reflex vertices.
    // This is an O(C*R) process, where C is the number of convex vertices and
    // R is the number of reflex vertices with N = C+R.  The order is O(N^2),
    // for example when C = R = N/2.
    for (i = m_iCFirst; i != -1; i = V(i).SNext)
    {
        if (IsEar(i))
        {
            InsertEndE(i);
        }
    }
    V(m_iEFirst).EPrev = m_iELast;
    V(m_iELast).ENext = m_iEFirst;

    // Remove the ears, one at a time.
    while (true)
    {
        // Add the triangle with the ear to the output list of triangles.
        int iVPrev = V(m_iEFirst).VPrev;
        int iVNext = V(m_iEFirst).VNext;
        rkTriangle.push_back(V(iVPrev).Index);
        rkTriangle.push_back(V(m_iEFirst).Index);
        rkTriangle.push_back(V(iVNext).Index);

        // Remove the vertex corresponding to the ear.
        RemoveV(m_iEFirst);
        if (--iVQuantity == 3)
        {
            // Only one triangle remains, just remove the ear and copy it.
            m_iEFirst = RemoveE(m_iEFirst);
            iVPrev = V(m_iEFirst).VPrev;
            iVNext = V(m_iEFirst).VNext;
            rkTriangle.push_back(V(iVPrev).Index);
            rkTriangle.push_back(V(m_iEFirst).Index);
            rkTriangle.push_back(V(iVNext).Index);
            break;
        }

        // Removal of the ear can cause an adjacent vertex to become an ear
        // or to stop being an ear.
        Vertex& rkVPrev = V(iVPrev);
        if (rkVPrev.IsEar)
        {
            if (!IsEar(iVPrev))
            {
                RemoveE(iVPrev);
            }
        }
        else
        {
            bool bWasReflex = !rkVPrev.IsConvex;
            if (IsConvex(iVPrev))
            {
                if (bWasReflex)
                {
                    RemoveR(iVPrev);
                }

                if (IsEar(iVPrev))
                {
                    InsertBeforeE(iVPrev);
                }
            }
        }

        Vertex& rkVNext = V(iVNext);
        if (rkVNext.IsEar)
        {
            if (!IsEar(iVNext))
            {
                RemoveE(iVNext);
            }
        }
        else
        {
            bool bWasReflex = !rkVNext.IsConvex;
            if (IsConvex(iVNext))
            {
                if (bWasReflex)
                {
                    RemoveR(iVNext);
                }

                if (IsEar(iVNext))
                {
                    InsertAfterE(iVNext);
                }
            }
        }

        // Remove the ear.
        m_iEFirst = RemoveE(m_iEFirst);
    }
}
//----------------------------------------------------------------------------
template <class Real>
int TriangulateEC<Real>::TriangleQuery (const Vector2<Real>& rkPoint,
    Query::Type eQueryType, Real fEpsilon, const Vector2<Real> akSTriangle[3])
    const
{
    switch (eQueryType)
    {
    case Query::QT_INT64:
        return Query2Int64<Real>(3,akSTriangle).ToTriangle(rkPoint,0,1,2);

    case Query::QT_INTEGER:
        return Query2TInteger<Real>(3,akSTriangle).ToTriangle(rkPoint,0,1,2);

    case Query::QT_REAL:
        return Query2<Real>(3,akSTriangle).ToTriangle(rkPoint,0,1,2);

    case Query::QT_RATIONAL:
        return Query2TRational<Real>(3,akSTriangle).ToTriangle(rkPoint,0,1,2);

    case Query::QT_FILTERED:
        return Query2Filtered<Real>(3,akSTriangle,fEpsilon).ToTriangle(
            rkPoint,0,1,2);
    }

    assert(false);
    return 1;
}
//----------------------------------------------------------------------------
template <class Real>
void TriangulateEC<Real>::CombinePolygons (Query::Type eQueryType,
    Real fEpsilon, int iNextElement, const Indices& rkOuter,
    const Indices& rkInner, IndexMap& rkMap, Indices& rkCombined)
{
    int iOQuantity = (int)rkOuter.size();
    int iIQuantity = (int)rkInner.size();

    // Locate the inner-polygon vertex of maximum x-value, call this vertex M.
    Real fXMax = m_kSPositions[rkInner[0]][0];
    int iXMaxIndex = 0;
    int i;
    for (i = 1; i < iIQuantity; i++)
    {
        Real fX = m_kSPositions[rkInner[i]][0];
        if (fX > fXMax)
        {
            fXMax = fX;
            iXMaxIndex = i;
        }
    }
    Vector2<Real> kM = m_kSPositions[rkInner[iXMaxIndex]];

    // Find the edge whose intersection Intr with the ray M+t*(1,0) minimizes
    // the ray parameter t >= 0.
    Vector2<Real> kIntr(Math<Real>::MAX_REAL,kM[1]);
    int iV0Min = -1, iV1Min = -1, iEndMin = -1;
    int i0, i1;
    for (i0 = iOQuantity-1, i1 = 0; i1 < iOQuantity; i0 = i1++)
    {
        // Only consider edges for which the first vertex is below (or on)
        // the ray and the second vertex is above (or on) the ray.
        Vector2<Real> kDiff0 = m_kSPositions[rkOuter[i0]] - kM;
        if (kDiff0[1] > (Real)0.0)
        {
            continue;
        }
        Vector2<Real> kDiff1 = m_kSPositions[rkOuter[i1]] - kM;
        if (kDiff1[1] < (Real)0.0)
        {
            continue;
        }

        // At this time, diff0.y <= 0 and diff1.y >= 0.
        Real fS, fT;
        int iCurrentEndMin = -1;
        if (kDiff0[1] < (Real)0.0)
        {
            if (kDiff1[1] > (Real)0.0)
            {
                // The intersection of the edge and ray occurs at an interior
                // edge point.
                fS = kDiff0[1]/(kDiff0[1] - kDiff1[1]);
                fT = kDiff0[0] + fS*(kDiff1[0] - kDiff0[0]);
            }
            else  // diff1.y == 0
            {
                // The vertex Outer[i1] is the intersection of the edge and
                // the ray.
                fT = kDiff1[0];
                iCurrentEndMin = i1;
            }
        }
        else  // diff0.y == 0
        {
            if (kDiff1[1] > (Real)0.0)
            {
                // The vertex Outer[i0] is the intersection of the edge and
                // the ray;
                fT = kDiff0[0];
                iCurrentEndMin = i0;
            }
            else  // diff1.y == 0
            {
                if (kDiff0[0] < kDiff1[0])
                {
                    fT = kDiff0[0];
                    iCurrentEndMin = i0;
                }
                else
                {
                    fT = kDiff1[0];
                    iCurrentEndMin = i1;
                }
            }
        }

        if ((Real)0.0 <= fT && fT < kIntr[0])
        {
            kIntr[0] = fT;
            iV0Min = i0;
            iV1Min = i1;
            if (iCurrentEndMin == -1)
            {
                // The current closest point is an edge-interior point.
                iEndMin = -1;
            }
            else
            {
                // The current closest point is a vertex.
                iEndMin = iCurrentEndMin;
            }
        }
    }

    int iMaxCosIndex;
    if (iEndMin == -1)
    {
        // Select one of Outer[v0min] and Outer[v1min] that has an x-value
        // larger than M.x, call this vertex P.  The triangle <M,I,P> must
        // contain an outer-polygon vertex that is visible to M, which is
        // possibly P itself.
        Vector2<Real> akSTriangle[3];  // <P,M,I> or <P,I,M>
        int iPIndex;
        if (m_kSPositions[rkOuter[iV0Min]][0] >
            m_kSPositions[rkOuter[iV1Min]][0])
        {
            akSTriangle[0] = m_kSPositions[rkOuter[iV0Min]];
            akSTriangle[1] = kIntr;
            akSTriangle[2] = kM;
            iPIndex = iV0Min;
        }
        else
        {
            akSTriangle[0] = m_kSPositions[rkOuter[iV1Min]];
            akSTriangle[1] = kM;
            akSTriangle[2] = kIntr;
            iPIndex = iV1Min;
        }

        // If any outer-polygon vertices other than P are inside the triangle
        // <M,I,P>, then at least one of these vertices must be a reflex
        // vertex.  It is sufficient to locate the reflex vertex R (if any)
        // in <M,I,P> that minimizes the angle between R-M and (1,0).  The
        // data member m_pkQuery is used for the reflex query.
        Vector2<Real> kDiff = akSTriangle[0] - kM;
        Real fMaxSqrLen = kDiff.SquaredLength();
        Real fMaxCos = kDiff[0]*kDiff[0]/fMaxSqrLen;
        iMaxCosIndex = iPIndex;
        for (i = 0; i < iOQuantity; i++)
        {
            if (i == iPIndex)
            {
                continue;
            }

            int iCurr = rkOuter[i];
            int iPrev = rkOuter[(i+iOQuantity-1) % iOQuantity];
            int iNext = rkOuter[(i+1) % iOQuantity];
            if (m_pkQuery->ToLine(iCurr,iPrev,iNext) <= 0
            &&  TriangleQuery(m_kSPositions[iCurr],eQueryType,fEpsilon,
                    akSTriangle) <= 0)
            {
                // The vertex is reflex and inside the <M,I,P> triangle.
                kDiff = m_kSPositions[iCurr] - kM;
                Real fSqrLen = kDiff.SquaredLength();
                Real fCos = kDiff[0]*kDiff[0]/fSqrLen;
                if (fCos > fMaxCos)
                {
                    // The reflex vertex forms a smaller angle with the
                    // positive x-axis, so it becomes the new visible
                    // candidate.
                    fMaxSqrLen = fSqrLen;
                    fMaxCos = fCos;
                    iMaxCosIndex = i;
                }
                else if (fCos == fMaxCos && fSqrLen < fMaxSqrLen)
                {
                    // The reflex vertex has angle equal to the current
                    // minimum but the length is smaller, so it becomes the
                    // new visible candidate.
                    fMaxSqrLen = fSqrLen;
                    iMaxCosIndex = i;
                }
            }
        }
    }
    else
    {
        iMaxCosIndex = iEndMin;
    }

    // The visible vertices are Position[Inner[iXMaxIndex]] and
    // Position[Outer[iMaxCosIndex]].  Two coincident edges with these
    // endpoints are inserted to connect the outer and inner polygons into a
    // simple polygon.  Each of the two Position[] values must be duplicated,
    // because the original might be convex (or reflex) and the duplicate is
    // reflex (or convex).  The ear-clipping algorithm needs to distinguish
    // between them.
    rkCombined.resize(iOQuantity+iIQuantity+2);
    int iCIndex = 0;
    for (i = 0; i <= iMaxCosIndex; i++, iCIndex++)
    {
        rkCombined[iCIndex] = rkOuter[i];
    }

    for (i = 0; i < iIQuantity; i++, iCIndex++)
    {
        int j = (iXMaxIndex + i) % iIQuantity;
        rkCombined[iCIndex] = rkInner[j];
    }

    int iInnerIndex = rkInner[iXMaxIndex];
    m_kSPositions[iNextElement] = m_kSPositions[iInnerIndex];
    rkCombined[iCIndex] = iNextElement;
    IndexMap::iterator pkIter = rkMap.find(iInnerIndex);
    if (pkIter != rkMap.end())
    {
        iInnerIndex = pkIter->second;
    }
    rkMap[iNextElement] = iInnerIndex;
    iCIndex++;
    iNextElement++;

    int iOuterIndex = rkOuter[iMaxCosIndex];
    m_kSPositions[iNextElement] = m_kSPositions[iOuterIndex];
    rkCombined[iCIndex] = iNextElement;
    pkIter = rkMap.find(iOuterIndex);
    if (pkIter != rkMap.end())
    {
        iOuterIndex = pkIter->second;
    }
    rkMap[iNextElement] = iOuterIndex;
    iCIndex++;
    iNextElement++;

    for (i = iMaxCosIndex+1; i < iOQuantity; i++, iCIndex++)
    {
        rkCombined[iCIndex] = rkOuter[i];
    }
}
//----------------------------------------------------------------------------
template <class Real>
void TriangulateEC<Real>::ProcessOuterAndInners (Query::Type eQueryType,
    Real fEpsilon, const Indices& rkOuter, const IndicesArray& rkInners,
    int& riNextElement, IndexMap& rkMap, Indices& rkCombined)
{
    // Sort the inner polygons based on maximum x-values.
    int iNumInners = (int)rkInners.size();
    std::vector<std::pair<Real,int> > kPairs(iNumInners);
    int i;
    for (i = 0; i < iNumInners; i++)
    {
        const Indices& rkInner = *rkInners[i];
        int iVQuantity = (int)rkInner.size();
        Real fXMax = m_kSPositions[rkInner[0]][0];
        for (int j = 1; j < iVQuantity; j++)
        {
            Real fX = m_kSPositions[rkInner[j]][0];
            if (fX > fXMax)
            {
                fXMax = fX;
            }
        }
        kPairs[i].first = fXMax;
        kPairs[i].second = i;
    }
    std::sort(kPairs.begin(),kPairs.end());

    // Merge the inner polygons with the outer polygon.
    Indices kCurrentOuter = rkOuter;
    for (i = iNumInners-1; i >= 0; i--)
    {
        const Indices& rkInner = *rkInners[kPairs[i].second];
        Indices kCurrentCombined;
        CombinePolygons(eQueryType,fEpsilon,riNextElement,kCurrentOuter,
            rkInner,rkMap,kCurrentCombined);
        kCurrentOuter = kCurrentCombined;
        riNextElement += 2;
    }

    for (i = 0; i < (int)kCurrentOuter.size(); i++)
    {
        rkCombined.push_back(kCurrentOuter[i]);
    }
}
//----------------------------------------------------------------------------
template <class Real>
void TriangulateEC<Real>::RemapIndices (const IndexMap& rkMap,
    Indices& rkTriangles) const
{
    // The triangulation includes indices to the duplicated outer and inner
    // vertices.  These indices must be mapped back to the original ones.
    for (int i = 0; i < (int)rkTriangles.size(); i++)
    {
        IndexMap::const_iterator pkIter = rkMap.find(rkTriangles[i]);
        if (pkIter != rkMap.end())
        {
            rkTriangles[i] = pkIter->second;
        }
    }
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Vertex list handling
//----------------------------------------------------------------------------
template <class Real>
typename TriangulateEC<Real>::Vertex& TriangulateEC<Real>::V (int i)
{
    return m_kVertex[i];
}
//----------------------------------------------------------------------------
template <class Real>
bool TriangulateEC<Real>::IsConvex (int i)
{
    Vertex& rkV = V(i);
    int iCurr = rkV.Index;
    int iPrev = V(rkV.VPrev).Index;
    int iNext = V(rkV.VNext).Index;
    rkV.IsConvex = (m_pkQuery->ToLine(iCurr,iPrev,iNext) > 0);
    return rkV.IsConvex;
}
//----------------------------------------------------------------------------
template <class Real>
bool TriangulateEC<Real>::IsEar (int i)
{
    Vertex& rkV = V(i);

    if (m_iRFirst == -1)
    {
        // The remaining polygon is convex.
        rkV.IsEar = true;
        return true;
    }

    // Search the reflex vertices and test if any are in the triangle
    // <V[prev],V[curr],V[next]>.
    int iPrev = V(rkV.VPrev).Index;
    int iCurr = rkV.Index;
    int iNext = V(rkV.VNext).Index;
    rkV.IsEar = true;
    for (int j = m_iRFirst; j != -1; j = V(j).SNext)
    {
        // Check if the test vertex is already one of the triangle vertices.
        if (j == rkV.VPrev || j == i || j == rkV.VNext)
        {
            continue;
        }

        // V[j] has been ruled out as one of the original vertices of the
        // triangle <V[prev],V[curr],V[next]>.  When triangulating polygons
        // with holes, V[j] might be a duplicated vertex, in which case it
        // does not affect the earness of V[curr].
        int iTest = V(j).Index;
        if (m_kSPositions[iTest] == m_kSPositions[iPrev]
        ||  m_kSPositions[iTest] == m_kSPositions[iCurr]
        ||  m_kSPositions[iTest] == m_kSPositions[iNext])
        {
            continue;
        }

        // Test if the vertex is inside or on the triangle.  When it is, it
        // causes V[curr] not to be an ear.
        if (m_pkQuery->ToTriangle(iTest,iPrev,iCurr,iNext) <= 0)
        {
            rkV.IsEar = false;
            break;
        }
    }

    return rkV.IsEar;
}
//----------------------------------------------------------------------------
template <class Real>
void TriangulateEC<Real>::InsertAfterC (int i)
{
    if (m_iCFirst == -1)
    {
        // add first convex vertex
        m_iCFirst = i;
    }
    else
    {
        V(m_iCLast).SNext = i;
        V(i).SPrev = m_iCLast;
    }
    m_iCLast = i;
}
//----------------------------------------------------------------------------
template <class Real>
void TriangulateEC<Real>::InsertAfterR (int i)
{
    if (m_iRFirst == -1)
    {
        // add first reflex vertex
        m_iRFirst = i;
    }
    else
    {
        V(m_iRLast).SNext = i;
        V(i).SPrev = m_iRLast;
    }
    m_iRLast = i;
}
//----------------------------------------------------------------------------
template <class Real>
void TriangulateEC<Real>::InsertEndE (int i)
{
    if (m_iEFirst == -1)
    {
        // add first ear
        m_iEFirst = i;
        m_iELast = i;
    }
    V(m_iELast).ENext = i;
    V(i).EPrev = m_iELast;
    m_iELast = i;
}
//----------------------------------------------------------------------------
template <class Real>
void TriangulateEC<Real>::InsertAfterE (int i)
{
    Vertex& rkVFirst = V(m_iEFirst);
    int iCurrENext = rkVFirst.ENext;
    Vertex& rkV = V(i);
    rkV.EPrev = m_iEFirst;
    rkV.ENext = iCurrENext;
    rkVFirst.ENext = i;
    V(iCurrENext).EPrev = i;
}
//----------------------------------------------------------------------------
template <class Real>
void TriangulateEC<Real>::InsertBeforeE (int i)
{
    Vertex& rkVFirst = V(m_iEFirst);
    int iCurrEPrev = rkVFirst.EPrev;
    Vertex& rkV = V(i);
    rkV.EPrev = iCurrEPrev;
    rkV.ENext = m_iEFirst;
    rkVFirst.EPrev = i;
    V(iCurrEPrev).ENext = i;
}
//----------------------------------------------------------------------------
template <class Real>
void TriangulateEC<Real>::RemoveV (int i)
{
    int iCurrVPrev = V(i).VPrev;
    int iCurrVNext = V(i).VNext;
    V(iCurrVPrev).VNext = iCurrVNext;
    V(iCurrVNext).VPrev = iCurrVPrev;
}
//----------------------------------------------------------------------------
template <class Real>
int TriangulateEC<Real>::RemoveE (int i)
{
    int iCurrEPrev = V(i).EPrev;
    int iCurrENext = V(i).ENext;
    V(iCurrEPrev).ENext = iCurrENext;
    V(iCurrENext).EPrev = iCurrEPrev;
    return iCurrENext;
}
//----------------------------------------------------------------------------
template <class Real>
void TriangulateEC<Real>::RemoveR (int i)
{
    assert(m_iRFirst != -1 && m_iRLast != -1);

    if (i == m_iRFirst)
    {
        m_iRFirst = V(i).SNext;
        if (m_iRFirst != -1)
        {
            V(m_iRFirst).SPrev = -1;
        }
        V(i).SNext = -1;
    }
    else if (i == m_iRLast)
    {
        m_iRLast = V(i).SPrev;
        if (m_iRLast != -1)
        {
            V(m_iRLast).SNext = -1;
        }
        V(i).SPrev = -1;
    }
    else
    {
        int iCurrSPrev = V(i).SPrev;
        int iCurrSNext = V(i).SNext;
        V(iCurrSPrev).SNext = iCurrSNext;
        V(iCurrSNext).SPrev = iCurrSPrev;
        V(i).SNext = -1;
        V(i).SPrev = -1;
    }
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Tree support.
//----------------------------------------------------------------------------
template <class Real>
void TriangulateEC<Real>::Delete (Tree*& rpkRoot)
{
    if (rpkRoot)
    {
        std::queue<Tree*> kQueue;
        kQueue.push(rpkRoot);

        while (kQueue.size() > 0)
        {
            Tree* pkTree = kQueue.front();
            kQueue.pop();
            for (int i = 0; i < (int)pkTree->Child.size(); i++)
            {
               kQueue.push(pkTree->Child[i]);
            }
            WM4_DELETE pkTree;
        }

        rpkRoot = nullptr;
    }
}
//----------------------------------------------------------------------------
template <class Real>
int TriangulateEC<Real>::GetExtraElements (const Tree* pkTree)
{
    int iExtraElements = 0;

    std::queue<const Tree*> kQueue;
    kQueue.push(pkTree);
    while (kQueue.size() > 0)
    {
        const Tree* pkRoot = kQueue.front();
        kQueue.pop();
        int iNumChildren = (int)pkRoot->Child.size();
        iExtraElements += 2*iNumChildren;

        for (int i = 0; i < iNumChildren; i++)
        {
            const Tree* pkChild = pkRoot->Child[i];
            int iNumGrandChildren = (int)pkChild->Child.size();
            for (int j = 0; j < iNumGrandChildren; j++)
            {
                kQueue.push(pkChild->Child[j]);
            }
        }
    }

    return iExtraElements;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class TriangulateEC<float>;

template WM4_FOUNDATION_ITEM
class TriangulateEC<double>;
//----------------------------------------------------------------------------
}
