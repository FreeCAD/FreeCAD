// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include "Wm4FoundationLIB.h"
#include "Wm4Vector3.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM MeshSmoother
{
public:
    // The caller is responsible for deleting the input arrays.
    MeshSmoother (int iVQuantity, Vector3<Real>* akVertex, int iTQuantity,
        const int* aiIndex);

    virtual ~MeshSmoother ();

    // For deferred construction and destruction.  The caller is responsible
    // for deleting the input arrays.
    MeshSmoother ();
    void Create (int iVQuantity, Vector3<Real>* akVertex, int iTQuantity,
        const int* aiIndex);
    void Destroy ();

    // input values from the constructor
    int GetVQuantity () const;
    const Vector3<Real>* GetVertices () const;
    int GetTQuantity () const;
    const int* GetIndices () const;

    // derived quantites from the input mesh
    const Vector3<Real>* GetNormals () const;
    const Vector3<Real>* GetMeans () const;

    // Apply one iteration of the smoother.  The input time is supported for
    // applications where the surface evolution is time-dependent.
    void Update (Real fTime = (Real)0.0);

protected:
    virtual bool VertexInfluenced (int i, Real fTime);
    virtual Real GetTangentWeight (int i, Real fTime);
    virtual Real GetNormalWeight (int i, Real fTime);

    int m_iVQuantity;
    Vector3<Real>* m_akVertex;
    int m_iTQuantity;
    const int* m_aiIndex;

    Vector3<Real>* m_akNormal;
    Vector3<Real>* m_akMean;
    int* m_aiNeighborCount;
};

typedef MeshSmoother<float> MeshSmootherf;
typedef MeshSmoother<double> MeshSmootherd;

}