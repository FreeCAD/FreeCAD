// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 4.10.0 (2009/11/18)

#ifndef WM4CONVEXHULL3_H
#define WM4CONVEXHULL3_H

#include "Wm4FoundationLIB.h"
#include "Wm4ConvexHull1.h"
#include "Wm4ConvexHull2.h"
#include "Wm4Query3.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM ConvexHull3 : public ConvexHull<Real>
{
public:
    // The input to the constructor is the array of vertices whose convex hull
    // is required.  If you want ConvexHull3 to delete the vertices during
    // destruction, set bOwner to 'true'.  Otherwise, you own the vertices and
    // must delete them yourself.
    //
    // You have a choice of speed versus accuracy.  The fastest choice is
    // Query::QT_INT64, but it gives up a lot of precision, scaling the points
    // to [0,2^{20}]^3.  The choice Query::QT_INTEGER gives up less precision,
    // scaling the points to [0,2^{24}]^3.  The choice Query::QT_RATIONAL uses
    // exact arithmetic, but is the slowest choice.  The choice Query::QT_REAL
    // uses floating-point arithmetic, but is not robust in all cases.

    ConvexHull3 (int iVertexQuantity, Vector3<Real>* akVertex, Real fEpsilon,
        bool bOwner, Query::Type eQueryType);
    virtual ~ConvexHull3 ();

    // If GetDimension() returns 1, then the points lie on a line.  You must
    // create a ConvexHull1 object using the function provided.
    const Vector3<Real>& GetLineOrigin () const;
    const Vector3<Real>& GetLineDirection () const;
    ConvexHull1<Real>* GetConvexHull1 () const;

    // If GetDimension() returns 2, then the points lie on a plane.  The plane
    // has two direction vectors (inputs 0 or 1).  You must create a
    // ConvexHull2 object using the function provided.
    const Vector3<Real>& GetPlaneOrigin () const;
    const Vector3<Real>& GetPlaneDirection (int i) const;
    ConvexHull2<Real>* GetConvexHull2 () const;

    // Support for streaming to/from disk.
    ConvexHull3 (const char* acFilename);
    bool Load (const char* acFilename);
    bool Save (const char* acFilename) const;

private:
    using ConvexHull<Real>::m_eQueryType;
    using ConvexHull<Real>::m_iVertexQuantity;
    using ConvexHull<Real>::m_iDimension;
    using ConvexHull<Real>::m_iSimplexQuantity;
    using ConvexHull<Real>::m_aiIndex;
    using ConvexHull<Real>::m_fEpsilon;
    using ConvexHull<Real>::m_bOwner;

    class WM4_FOUNDATION_ITEM Triangle
    {
    public:
        Triangle (int iV0, int iV1, int iV2);

        int GetSign (int i, const Query3<Real>* pkQuery);
        void AttachTo (Triangle* pkAdj0, Triangle* pkAdj1, Triangle* pkAdj2);
        int DetachFrom (int iAdj, Triangle* pkAdj);

        int V[3];
        Triangle* A[3];
        int Sign;
        int Time;
        bool OnStack;
    };

    bool Update (int i);
    void ExtractIndices ();
    void DeleteHull ();

    // The input points.
    Vector3<Real>* m_akVertex;

    // Support for robust queries.
    Vector3<Real>* m_akSVertex;
    Query3<Real>* m_pkQuery;

    // The line of containment if the dimension is 1.
    Vector3<Real> m_kLineOrigin, m_kLineDirection;

    // The plane of containment if the dimension is 2.
    Vector3<Real> m_kPlaneOrigin, m_akPlaneDirection[2];

    // The current hull.
    std::set<Triangle*> m_kHull;

    class WM4_FOUNDATION_ITEM TerminatorData
    {
    public:
        TerminatorData (int iV0 = -1, int iV1 = -1, int iNullIndex = -1,
            Triangle* pkTri = nullptr)
        {
            V[0] = iV0;
            V[1] = iV1;
            NullIndex = iNullIndex;
            Tri = pkTri;
        }

        int V[2];
        int NullIndex;
        Triangle* Tri;
    };
};

typedef ConvexHull3<float> ConvexHull3f;
typedef ConvexHull3<double> ConvexHull3d;

}

#endif
