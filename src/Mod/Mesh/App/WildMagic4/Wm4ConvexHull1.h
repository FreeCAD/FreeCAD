// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 4.10.0 (2009/11/18)

#ifndef WM4CONVEXHULL1_H
#define WM4CONVEXHULL1_H

// A fancy class to compute the minimum and maximum of a collection of
// real-valued numbers, but this provides some convenience for ConvexHull2 and
// ConvexHull3 when the input point set has intrinsic dimension smaller than
// the containing space.  The interface of ConvexHull1 is also the model for
// those of ConvexHull2 and ConvexHull3.

#include "Wm4FoundationLIB.h"
#include "Wm4ConvexHull.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM ConvexHull1 : public ConvexHull<Real>
{
public:
    // The input to the constructor is the array of vertices you want to sort.
    // If you want ConvexHull1 to delete the array during destruction, set
    // bOwner to 'true'.  Otherwise, you own the array and must delete it
    // yourself.  TO DO:  The computation type is currently ignored by this
    // class.  Add support for the various types later.
    ConvexHull1 (int iVertexQuantity, Real* afVertex, Real fEpsilon,
        bool bOwner, Query::Type eQueryType);
    virtual ~ConvexHull1 ();

    // The input vertex array.
    const Real* GetVertices () const;

    // Support for streaming to/from disk.
    ConvexHull1 (const char* acFilename);
    bool Load (const char* acFilename);
    bool Save (const char* acFilename) const;

private:
    using ConvexHull<Real>::m_iVertexQuantity;
    using ConvexHull<Real>::m_iDimension;
    using ConvexHull<Real>::m_iSimplexQuantity;
    using ConvexHull<Real>::m_aiIndex;
    using ConvexHull<Real>::m_fEpsilon;
    using ConvexHull<Real>::m_bOwner;

    Real* m_afVertex;

    class WM4_FOUNDATION_ITEM SortedVertex
    {
    public:
        Real Value;
        int Index;

        bool operator< (const SortedVertex& rkProj) const
        {
            return Value < rkProj.Value;
        }
    };
};

typedef ConvexHull1<float> ConvexHull1f;
typedef ConvexHull1<double> ConvexHull1d;

}

#endif
