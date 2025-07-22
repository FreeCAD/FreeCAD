// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 4.10.0 (2009/11/18)

#include "Wm4FoundationPCH.h"
#include "Wm4ContBox3.h"
#include "Wm4ApprGaussPointsFit3.h"
#include "Wm4ContBox2.h"
#include "Wm4ConvexHull3.h"
#include "Wm4EdgeKey.h"
#include "Wm4Quaternion.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
Box3<Real> ContAlignedBox (int iQuantity, const Vector3<Real>* akPoint)
{
    Vector3<Real> kMin, kMax;
    Vector3<Real>::ComputeExtremes(iQuantity,akPoint,kMin,kMax);

    Box3<Real> kBox;
    kBox.Center = ((Real)0.5)*(kMin + kMax);
    kBox.Axis[0] = Vector3<Real>::UNIT_X;
    kBox.Axis[1] = Vector3<Real>::UNIT_Y;
    kBox.Axis[2] = Vector3<Real>::UNIT_Z;
    Vector3<Real> kHalfDiagonal = ((Real)0.5)*(kMax - kMin);
    for (int i = 0; i < 3; i++)
    {
        kBox.Extent[i] = kHalfDiagonal[i];
    }

    return kBox;
}
//----------------------------------------------------------------------------
template <class Real>
Box3<Real> ContOrientedBox (int iQuantity, const Vector3<Real>* akPoint)
{
    Box3<Real> kBox = GaussPointsFit3<Real>(iQuantity,akPoint);

    // Let C be the box center and let U0, U1, and U2 be the box axes.  Each
    // input point is of the form X = C + y0*U0 + y1*U1 + y2*U2.  The
    // following code computes min(y0), max(y0), min(y1), max(y1), min(y2),
    // and max(y2).  The box center is then adjusted to be
    //   C' = C + 0.5*(min(y0)+max(y0))*U0 + 0.5*(min(y1)+max(y1))*U1 +
    //        0.5*(min(y2)+max(y2))*U2

    Vector3<Real> kDiff = akPoint[0] - kBox.Center;
    Vector3<Real> kMin(kDiff.Dot(kBox.Axis[0]),kDiff.Dot(kBox.Axis[1]),
        kDiff.Dot(kBox.Axis[2]));
    Vector3<Real> kMax = kMin;
    for (int i = 1; i < iQuantity; i++)
    {
        kDiff = akPoint[i] - kBox.Center;
        for (int j = 0; j < 3; j++)
        {
            Real fDot = kDiff.Dot(kBox.Axis[j]);
            if (fDot < kMin[j])
            {
                kMin[j] = fDot;
            }
            else if (fDot > kMax[j])
            {
                kMax[j] = fDot;
            }
        }
    }

    kBox.Center +=
        (((Real)0.5)*(kMin[0]+kMax[0]))*kBox.Axis[0] +
        (((Real)0.5)*(kMin[1]+kMax[1]))*kBox.Axis[1] +
        (((Real)0.5)*(kMin[2]+kMax[2]))*kBox.Axis[2];

    kBox.Extent[0] = ((Real)0.5)*(kMax[0] - kMin[0]);
    kBox.Extent[1] = ((Real)0.5)*(kMax[1] - kMin[1]);
    kBox.Extent[2] = ((Real)0.5)*(kMax[2] - kMin[2]);

    return kBox;
}
//----------------------------------------------------------------------------
template <class Real>
Box3<Real> ContMinBox (int iQuantity, const Vector3<Real>* akPoint,
    Real fEpsilon, Query::Type eQueryType)
{
    Box3<Real> kBox;

    // Get the convex hull of the points.
    ConvexHull3<Real> kHull(iQuantity,(Vector3<Real>*)akPoint,fEpsilon,false,
        eQueryType);
    int iHDim = kHull.GetDimension();
    int iHQuantity;
    const int* aiHIndex;

    if (iHDim == 0)
    {
        kBox.Center = akPoint[0];
        kBox.Axis[0] = Vector3<Real>::UNIT_X;
        kBox.Axis[1] = Vector3<Real>::UNIT_Y;
        kBox.Axis[2] = Vector3<Real>::UNIT_Z;
        kBox.Extent[0] = (Real)0.0;
        kBox.Extent[1] = (Real)0.0;
        kBox.Extent[2] = (Real)0.0;
        return kBox;
    }

    if (iHDim == 1)
    {
        ConvexHull1<Real>* pkHull1 = kHull.GetConvexHull1();
        aiHIndex = pkHull1->GetIndices();

        kBox.Center = ((Real)0.5)*(akPoint[aiHIndex[0]]+akPoint[aiHIndex[1]]);
        Vector3<Real> kDiff = akPoint[aiHIndex[1]] - akPoint[aiHIndex[0]];
        kBox.Extent[0] = ((Real)0.5)*kDiff.Normalize();
        kBox.Extent[1] = (Real)0.0;
        kBox.Extent[2] = (Real)0.0;
        kBox.Axis[0] = kDiff;
        Vector3<Real>::GenerateComplementBasis(kBox.Axis[1],kBox.Axis[2],
            kBox.Axis[0]);

        WM4_DELETE pkHull1;
        return kBox;
    }

    int i, j;
    Vector3<Real> kOrigin, kDiff, kU, kV, kW;
    Vector2<Real>* akPoint2;
    Box2<Real> kBox2;

    if (iHDim == 2)
    {
        // When ConvexHull3 reports that the point set is 2-dimensional, the
        // caller is responsible for projecting the points onto a plane and
        // calling ConvexHull2.  ConvexHull3 does provide information about
        // the plane of the points.  In this application, we need only
        // project the input points onto that plane and call ContMinBox in
        // two dimensions.

        // Get a coordinate system relative to the plane of the points.
        kOrigin = kHull.GetPlaneOrigin();
        kW = kHull.GetPlaneDirection(0).Cross(kHull.GetPlaneDirection(1));
        Vector3<Real>::GenerateComplementBasis(kU,kV,kW);

        // Project the input points onto the plane.
        akPoint2 = WM4_NEW Vector2<Real>[iQuantity];
        for (i = 0; i < iQuantity; i++)
        {
            kDiff = akPoint[i] - kOrigin;
            akPoint2[i].X() = kU.Dot(kDiff);
            akPoint2[i].Y() = kV.Dot(kDiff);
        }

        // Compute the minimum area box in 2D.
        kBox2 = ContMinBox<Real>(iQuantity,akPoint2,fEpsilon,eQueryType,
            false);
        WM4_DELETE[] akPoint2;

        // Lift the values into 3D.
        kBox.Center = kOrigin + kBox2.Center.X()*kU + kBox2.Center.Y()*kV;
        kBox.Axis[0] = kBox2.Axis[0].X()*kU + kBox2.Axis[0].Y()*kV;
        kBox.Axis[1] = kBox2.Axis[1].X()*kU + kBox2.Axis[1].Y()*kV;
        kBox.Axis[2] = kW;
        kBox.Extent[0] = kBox2.Extent[0];
        kBox.Extent[1] = kBox2.Extent[1];
        kBox.Extent[2] = (Real)0.0;
        return kBox;
    }

    iHQuantity = kHull.GetSimplexQuantity();
    aiHIndex = kHull.GetIndices();
    Real fVolume, fMinVolume = Math<Real>::MAX_REAL;

    // Create the unique set of hull vertices to minimize the time spent
    // projecting vertices onto planes of the hull faces.
    std::set<int> kUniqueIndices;
    for (i = 0; i < 3*iHQuantity; i++)
    {
        kUniqueIndices.insert(aiHIndex[i]);
    }

    // Use the rotating calipers method on the projection of the hull onto
    // the plane of each face.  Also project the hull onto the normal line
    // of each face.  The minimum area box in the plane and the height on
    // the line produce a containing box.  If its volume is smaller than the
    // current volume, this box is the new candidate for the minimum volume
    // box.  The unique edges are accumulated into a set for use by a later
    // step in the algorithm.
    const int* piIndex = aiHIndex;
    Real fHeight, fMinHeight, fMaxHeight;
    std::set<EdgeKey> kEdges;
    akPoint2 = WM4_NEW Vector2<Real>[kUniqueIndices.size()];
    for (i = 0; i < iHQuantity; i++)
    {
        // get triangle
        int iV0 = *piIndex++;
        int iV1 = *piIndex++;
        int iV2 = *piIndex++;

        // save the edges for later use
        kEdges.insert(EdgeKey(iV0,iV1));
        kEdges.insert(EdgeKey(iV1,iV2));
        kEdges.insert(EdgeKey(iV2,iV0));

        // get 3D coordinate system relative to plane of triangle
        kOrigin = (akPoint[iV0] + akPoint[iV1] + akPoint[iV2])/(Real)3.0;
        Vector3<Real> kEdge1 = akPoint[iV1] - akPoint[iV0];
        Vector3<Real> kEdge2 = akPoint[iV2] - akPoint[iV0];
        kW = kEdge2.UnitCross(kEdge1);  // inner-pointing normal
        if (kW == Vector3<Real>::ZERO)
        {
            // The triangle is needle-like, so skip it.
            continue;
        }
        Vector3<Real>::GenerateComplementBasis(kU,kV,kW);

        // Project points onto plane of triangle, onto normal line of plane.
        // TO DO.  In theory, minHeight should be zero since W points to the
        // interior of the hull.  However, the snap rounding used in the 3D
        // convex hull finder involves loss of precision, which in turn can
        // cause a hull facet to have the wrong ordering (clockwise instead
        // of counterclockwise when viewed from outside the hull).  The
        // height calculations here trap that problem (the incorrectly ordered
        // face will not affect the minimum volume box calculations).
        fMinHeight = (Real)0.0;
        fMaxHeight = (Real)0.0;
        j = 0;
        std::set<int>::const_iterator pkUI = kUniqueIndices.begin();
        while (pkUI != kUniqueIndices.end())
        {
            int index = *pkUI++;
            kDiff = akPoint[index] - kOrigin;
            akPoint2[j].X() = kU.Dot(kDiff);
            akPoint2[j].Y() = kV.Dot(kDiff);
            fHeight = kW.Dot(kDiff);
            if (fHeight > fMaxHeight)
            {
                fMaxHeight = fHeight;
            }
            else if (fHeight < fMinHeight)
            {
                fMinHeight = fHeight;
            }

            j++;
        }
        if (-fMinHeight > fMaxHeight)
        {
            fMaxHeight = -fMinHeight;
        }

        // compute minimum area box in 2D
        kBox2 = ContMinBox<Real>((int)kUniqueIndices.size(),akPoint2,fEpsilon,
            eQueryType,false);

        // update current minimum-volume box (if necessary)
        fVolume = fMaxHeight*kBox2.Extent[0]*kBox2.Extent[1];
        if (fVolume < fMinVolume)
        {
            fMinVolume = fVolume;

            // lift the values into 3D
            kBox.Extent[0] = kBox2.Extent[0];
            kBox.Extent[1] = kBox2.Extent[1];
            kBox.Extent[2] = ((Real)0.5)*fMaxHeight;
            kBox.Axis[0] = kBox2.Axis[0].X()*kU + kBox2.Axis[0].Y()*kV;
            kBox.Axis[1] = kBox2.Axis[1].X()*kU + kBox2.Axis[1].Y()*kV;
            kBox.Axis[2] = kW;
            kBox.Center = kOrigin + kBox2.Center.X()*kU + kBox2.Center.Y()*kV
                + kBox.Extent[2]*kW;
        }
    }

    // The minimum-volume box can also be supported by three mutually
    // orthogonal edges of the convex hull.  For each triple of orthogonal
    // edges, compute the minimum-volume box for that coordinate frame by
    // projecting the points onto the axes of the frame.
    std::set<EdgeKey>::const_iterator pkE2;
    for (pkE2 = kEdges.begin(); pkE2 != kEdges.end(); pkE2++)
    {
        kW = akPoint[pkE2->V[1]] - akPoint[pkE2->V[0]];
        kW.Normalize();

        std::set<EdgeKey>::const_iterator pkE1 = pkE2;
        for (++pkE1; pkE1 != kEdges.end(); pkE1++)
        {
            kV = akPoint[pkE1->V[1]] - akPoint[pkE1->V[0]];
            kV.Normalize();
            Real fDot = kV.Dot(kW);
            if (Math<Real>::FAbs(fDot) > Math<Real>::ZERO_TOLERANCE)
            {
                continue;
            }

            std::set<EdgeKey>::const_iterator pkE0 = pkE1;
            for (++pkE0; pkE0 != kEdges.end(); pkE0++)
            {
                kU = akPoint[pkE0->V[1]] - akPoint[pkE0->V[0]];
                kU.Normalize();
                fDot = kU.Dot(kV);
                if (Math<Real>::FAbs(fDot) > Math<Real>::ZERO_TOLERANCE)
                {
                    continue;
                }
                fDot = kU.Dot(kW);
                if (Math<Real>::FAbs(fDot) > Math<Real>::ZERO_TOLERANCE)
                {
                    continue;
                }
    
                // The three edges are mutually orthogonal.  Project the
                // hull points onto the lines containing the edges.  Use
                // hull point zero as the origin.
                Real fUMin = (Real)0.0, fUMax = (Real)0.0;
                Real fVMin = (Real)0.0, fVMax = (Real)0.0;
                Real fWMin = (Real)0.0, fWMax = (Real)0.0;
                kOrigin = akPoint[aiHIndex[0]];
                std::set<int>::const_iterator pkUI = kUniqueIndices.begin();
                while (pkUI != kUniqueIndices.end())
                {
                    int index = *pkUI++;
                    kDiff = akPoint[index] - kOrigin;

                    Real fU = kU.Dot(kDiff);
                    if (fU < fUMin)
                    {
                        fUMin = fU;
                    }
                    else if (fU > fUMax)
                    {
                        fUMax = fU;
                    }

                    Real fV = kV.Dot(kDiff);
                    if (fV < fVMin)
                    {
                        fVMin = fV;
                    }
                    else if (fV > fVMax)
                    {
                        fVMax = fV;
                    }

                    Real fW = kW.Dot(kDiff);
                    if (fW < fWMin)
                    {
                        fWMin = fW;
                    }
                    else if (fW > fWMax)
                    {
                        fWMax = fW;
                    }
                }

                Real fUExtent = ((Real)0.5)*(fUMax - fUMin);
                Real fVExtent = ((Real)0.5)*(fVMax - fVMin);
                Real fWExtent = ((Real)0.5)*(fWMax - fWMin);

                // update current minimum-volume box (if necessary)
                fVolume = fUExtent*fVExtent*fWExtent;
                if (fVolume < fMinVolume)
                {
                    fMinVolume = fVolume;

                    kBox.Extent[0] = fUExtent;
                    kBox.Extent[1] = fVExtent;
                    kBox.Extent[2] = fWExtent;
                    kBox.Axis[0] = kU;
                    kBox.Axis[1] = kV;
                    kBox.Axis[2] = kW;
                    kBox.Center = kOrigin +
                        ((Real)0.5)*(fUMin+fUMax)*kU +
                        ((Real)0.5)*(fVMin+fVMax)*kV +
                        ((Real)0.5)*(fWMin+fWMax)*kW;
                }
            }
        }
    }

    WM4_DELETE[] akPoint2;
    return kBox;
}
//----------------------------------------------------------------------------
template <class Real>
bool InBox (const Vector3<Real>& rkPoint, const Box3<Real>& rkBox)
{
    Vector3<Real> kDiff = rkPoint - rkBox.Center;
    for (int i = 0; i < 3; i++)
    {
        Real fCoeff = kDiff.Dot(rkBox.Axis[i]);
        if (Math<Real>::FAbs(fCoeff) > rkBox.Extent[i])
        {
            return false;
        }
    }
    return true;
}
//----------------------------------------------------------------------------
template <class Real>
Box3<Real> MergeBoxes (const Box3<Real>& rkBox0, const Box3<Real>& rkBox1)
{
    // construct a box that contains the input boxes
    Box3<Real> kBox;

    // The first guess at the box center.  This value will be updated later
    // after the input box vertices are projected onto axes determined by an
    // average of box axes.
    kBox.Center = ((Real)0.5)*(rkBox0.Center + rkBox1.Center);

    // A box's axes, when viewed as the columns of a matrix, form a rotation
    // matrix.  The input box axes are converted to quaternions.  The average
    // quaternion is computed, then normalized to unit length.  The result is
    // the slerp of the two input quaternions with t-value of 1/2.  The result
    // is converted back to a rotation matrix and its columns are selected as
    // the merged box axes.
    Quaternion<Real> kQ0, kQ1;
    kQ0.FromRotationMatrix(rkBox0.Axis);
    kQ1.FromRotationMatrix(rkBox1.Axis);
    if (kQ0.Dot(kQ1) < (Real)0.0)
    {
        kQ1 = -kQ1;
    }

    Quaternion<Real> kQ = kQ0 + kQ1;
    Real fInvLength = Math<Real>::InvSqrt(kQ.Dot(kQ));
    kQ = fInvLength*kQ;
    kQ.ToRotationMatrix(kBox.Axis);

    // Project the input box vertices onto the merged-box axes.  Each axis
    // D[i] containing the current center C has a minimum projected value
    // pmin[i] and a maximum projected value pmax[i].  The corresponding end
    // points on the axes are C+pmin[i]*D[i] and C+pmax[i]*D[i].  The point C
    // is not necessarily the midpoint for any of the intervals.  The actual
    // box center will be adjusted from C to a point C' that is the midpoint
    // of each interval,
    //   C' = C + sum_{i=0}^2 0.5*(pmin[i]+pmax[i])*D[i]
    // The box extents are
    //   e[i] = 0.5*(pmax[i]-pmin[i])

    int i, j;
    Real fDot;
    Vector3<Real> akVertex[8], kDiff;
    Vector3<Real> kMin = Vector3<Real>::ZERO;
    Vector3<Real> kMax = Vector3<Real>::ZERO;

    rkBox0.ComputeVertices(akVertex);
    for (i = 0; i < 8; i++)
    {
        kDiff = akVertex[i] - kBox.Center;
        for (j = 0; j < 3; j++)
        {
            fDot = kDiff.Dot(kBox.Axis[j]);
            if (fDot > kMax[j])
            {
                kMax[j] = fDot;
            }
            else if (fDot < kMin[j])
            {
                kMin[j] = fDot;
            }
        }
    }

    rkBox1.ComputeVertices(akVertex);
    for (i = 0; i < 8; i++)
    {
        kDiff = akVertex[i] - kBox.Center;
        for (j = 0; j < 3; j++)
        {
            fDot = kDiff.Dot(kBox.Axis[j]);
            if (fDot > kMax[j])
            {
                kMax[j] = fDot;
            }
            else if (fDot < kMin[j])
            {
                kMin[j] = fDot;
            }
        }
    }

    // [kMin,kMax] is the axis-aligned box in the coordinate system of the
    // merged box axes.  Update the current box center to be the center of
    // the new box.  Compute the extents based on the new center.
    for (j = 0; j < 3; j++)
    {
        kBox.Center += (((Real)0.5)*(kMax[j]+kMin[j]))*kBox.Axis[j];
        kBox.Extent[j] = ((Real)0.5)*(kMax[j]-kMin[j]);
    }

    return kBox;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
Box3<float> ContAlignedBox<float> (int, const Vector3<float>*);

template WM4_FOUNDATION_ITEM
Box3<float> ContOrientedBox<float> (int, const Vector3<float>*);

template WM4_FOUNDATION_ITEM
Box3<float> ContMinBox<float> (int, const Vector3<float>*, float,
    Query::Type);

template WM4_FOUNDATION_ITEM
bool InBox<float> (const Vector3<float>&, const Box3<float>&);

template WM4_FOUNDATION_ITEM
Box3<float> MergeBoxes<float> (const Box3<float>&, const Box3<float>&);

template WM4_FOUNDATION_ITEM
Box3<double> ContAlignedBox<double> (int, const Vector3<double>*);

template WM4_FOUNDATION_ITEM
Box3<double> ContOrientedBox<double> (int, const Vector3<double>*);

template WM4_FOUNDATION_ITEM
Box3<double> ContMinBox<double> (int, const Vector3<double>*, double,
    Query::Type);

template WM4_FOUNDATION_ITEM
bool InBox<double> (const Vector3<double>&, const Box3<double>&);

template WM4_FOUNDATION_ITEM
Box3<double> MergeBoxes<double> (const Box3<double>&, const Box3<double>&);
//----------------------------------------------------------------------------
}
