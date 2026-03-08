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
#include "Wm4System.h"

namespace Wm4
{

template <class Real>
class BandedMatrix
{
public:
    BandedMatrix (int iSize, int iLBands, int iUBands);
    BandedMatrix (const BandedMatrix& rkM);
    ~BandedMatrix ();

    BandedMatrix& operator= (const BandedMatrix& rkM);

    int GetSize () const;
    int GetLBands () const;
    int GetUBands () const;

    Real* GetDBand ();
    const Real* GetDBand () const;

    int GetLBandMax (int i) const;  // LBand(i):  0 <= index < LBandMax
    Real* GetLBand (int i);
    const Real* GetLBand (int i) const;

    int GetUBandMax (int i) const;  // UBand(i):  0 <= index < UBandMax
    Real* GetUBand (int i);
    const Real* GetUBand (int i) const;

    Real& operator() (int iRow, int iCol);
    Real operator() (int iRow, int iCol) const;

    void SetZero ();
    void SetIdentity ();

private:
    void Allocate ();
    void Deallocate ();

    int m_iSize, m_iLBands, m_iUBands;
    Real* m_afDBand;
    Real** m_aafLBand;
    Real** m_aafUBand;
};

}

#include "Wm4BandedMatrix.inl"

namespace Wm4
{
typedef BandedMatrix<float> BandedMatrixf;
typedef BandedMatrix<double> BandedMatrixd;
}