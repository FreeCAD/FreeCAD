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
#include "Wm4Query3TRational.h"
#include "Wm4Vector3.h"

namespace Wm4
{

template <class Real>
class Query3Filtered : public Query3<Real>
{
public:
    // The base class handles floating-point queries.  Each query involves
    // comparing a determinant to zero.  If the determinant is sufficiently
    // close to zero, numerical round-off errors may cause the determinant
    // sign to be misclassified.  To avoid this, the query is repeated with
    // exact rational arithmetic.  You specify the closeness to zero for the
    // switch to rational arithmetic via fUncertainty, a value in the
    // interval [0,1].  The uncertainty of 0 causes the class to behave
    // as if it were Query3.  The uncertainty of 1 causes the class to
    // behave as if it were Query3TRational.
    Query3Filtered (int iVQuantity, const Vector3<Real>* akVertex,
        Real fUncertainty);
    virtual ~Query3Filtered ();

    // run-time type information
    virtual Query::Type GetType () const;

    // Queries about the relation of a point to various geometric objects.

    virtual int ToPlane (const Vector3<Real>& rkP, int iV0, int iV1, int iV2)
        const;

    virtual int ToCircumsphere (const Vector3<Real>& rkP, int iV0, int iV1,
        int iV2, int iV3) const;

private:
    using Query3<Real>::m_akVertex;

    Query3TRational<Real> m_kRQuery;
    Real m_fUncertainty;
};

}

#include "Wm4Query3Filtered.inl"

namespace Wm4
{
typedef Query3Filtered<float> Query3Filteredf;
typedef Query3Filtered<double> Query3Filteredd;

}