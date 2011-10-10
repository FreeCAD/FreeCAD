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
#include "Wm4LinearSystem.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
LinearSystem<Real>::LinearSystem ()
{
    ZeroTolerance = Math<Real>::ZERO_TOLERANCE;
}
//----------------------------------------------------------------------------
template <class Real>
bool LinearSystem<Real>::Solve2 (const Real aafA[2][2], const Real afB[2],
    Real afX[2])
{
    Real fDet = aafA[0][0]*aafA[1][1]-aafA[0][1]*aafA[1][0];
    if (Math<Real>::FAbs(fDet) < ZeroTolerance)
    {
        return false;
    }

    Real fInvDet = ((Real)1.0)/fDet;
    afX[0] = (aafA[1][1]*afB[0]-aafA[0][1]*afB[1])*fInvDet;
    afX[1] = (aafA[0][0]*afB[1]-aafA[1][0]*afB[0])*fInvDet;
    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool LinearSystem<Real>::Solve3 (const Real aafA[3][3], const Real afB[3],
    Real afX[3])
{
    Real aafAInv[3][3];
    aafAInv[0][0] = aafA[1][1]*aafA[2][2]-aafA[1][2]*aafA[2][1];
    aafAInv[0][1] = aafA[0][2]*aafA[2][1]-aafA[0][1]*aafA[2][2];
    aafAInv[0][2] = aafA[0][1]*aafA[1][2]-aafA[0][2]*aafA[1][1];
    aafAInv[1][0] = aafA[1][2]*aafA[2][0]-aafA[1][0]*aafA[2][2];
    aafAInv[1][1] = aafA[0][0]*aafA[2][2]-aafA[0][2]*aafA[2][0];
    aafAInv[1][2] = aafA[0][2]*aafA[1][0]-aafA[0][0]*aafA[1][2];
    aafAInv[2][0] = aafA[1][0]*aafA[2][1]-aafA[1][1]*aafA[2][0];
    aafAInv[2][1] = aafA[0][1]*aafA[2][0]-aafA[0][0]*aafA[2][1];
    aafAInv[2][2] = aafA[0][0]*aafA[1][1]-aafA[0][1]*aafA[1][0];
    Real fDet = aafA[0][0]*aafAInv[0][0] + aafA[0][1]*aafAInv[1][0] +
        aafA[0][2]*aafAInv[2][0];

    if (Math<Real>::FAbs(fDet) < ZeroTolerance)
    {
        return false;
    }

    Real fInvDet = ((Real)1.0)/fDet;
    for (int iRow = 0; iRow < 3; iRow++)
    {
        for (int iCol = 0; iCol < 3; iCol++)
        {
            aafAInv[iRow][iCol] *= fInvDet;
        }
    }

    afX[0] = aafAInv[0][0]*afB[0]+aafAInv[0][1]*afB[1]+aafAInv[0][2]*afB[2];
    afX[1] = aafAInv[1][0]*afB[0]+aafAInv[1][1]*afB[1]+aafAInv[1][2]*afB[2];
    afX[2] = aafAInv[2][0]*afB[0]+aafAInv[2][1]*afB[1]+aafAInv[2][2]*afB[2];
    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool LinearSystem<Real>::Inverse (const GMatrix<Real>& rkA,
    GMatrix<Real>& rkInvA)
{
    // computations are performed in-place
    assert(rkA.GetRows() == rkA.GetColumns());
    int iSize = rkInvA.GetRows();
    rkInvA = rkA;

    int* aiColIndex = WM4_NEW int[iSize];
    int* aiRowIndex = WM4_NEW int[iSize];
    bool* abPivoted = WM4_NEW bool[iSize];
    memset(abPivoted,0,iSize*sizeof(bool));

    int i1, i2, iRow = 0, iCol = 0;
    Real fSave;

    // elimination by full pivoting
    for (int i0 = 0; i0 < iSize; i0++)
    {
        // search matrix (excluding pivoted rows) for maximum absolute entry
        Real fMax = 0.0f;
        for (i1 = 0; i1 < iSize; i1++)
        {
            if (!abPivoted[i1])
            {
                for (i2 = 0; i2 < iSize; i2++)
                {
                    if (!abPivoted[i2])
                    {
                        Real fAbs = Math<Real>::FAbs(rkInvA[i1][i2]);
                        if (fAbs > fMax)
                        {
                            fMax = fAbs;
                            iRow = i1;
                            iCol = i2;
                        }
                    }
                }
            }
        }

        if (fMax == (Real)0.0)
        {
            // matrix is not invertible
            WM4_DELETE[] aiColIndex;
            WM4_DELETE[] aiRowIndex;
            WM4_DELETE[] abPivoted;
            return false;
        }

        abPivoted[iCol] = true;

        // swap rows so that A[iCol][iCol] contains the pivot entry
        if (iRow != iCol)
        {
            rkInvA.SwapRows(iRow,iCol);
        }

        // keep track of the permutations of the rows
        aiRowIndex[i0] = iRow;
        aiColIndex[i0] = iCol;

        // scale the row so that the pivot entry is 1
        Real fInv = ((Real)1.0)/rkInvA[iCol][iCol];
        rkInvA[iCol][iCol] = (Real)1.0;
        for (i2 = 0; i2 < iSize; i2++)
        {
            rkInvA[iCol][i2] *= fInv;
        }

        // zero out the pivot column locations in the other rows
        for (i1 = 0; i1 < iSize; i1++)
        {
            if (i1 != iCol)
            {
                fSave = rkInvA[i1][iCol];
                rkInvA[i1][iCol] = (Real)0.0;
                for (i2 = 0; i2 < iSize; i2++)
                {
                    rkInvA[i1][i2] -= rkInvA[iCol][i2]*fSave;
                }
            }
        }
    }

    // reorder rows so that A[][] stores the inverse of the original matrix
    for (i1 = iSize-1; i1 >= 0; i1--)
    {
        if (aiRowIndex[i1] != aiColIndex[i1])
        {
            for (i2 = 0; i2 < iSize; i2++)
            {
                fSave = rkInvA[i2][aiRowIndex[i1]];
                rkInvA[i2][aiRowIndex[i1]] = rkInvA[i2][aiColIndex[i1]];
                rkInvA[i2][aiColIndex[i1]] = fSave;
            }
        }
    }

    WM4_DELETE[] aiColIndex;
    WM4_DELETE[] aiRowIndex;
    WM4_DELETE[] abPivoted;
    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool LinearSystem<Real>::Solve (const GMatrix<Real>& rkA, const Real* afB,
    Real* afX)
{
    // computations are performed in-place
    int iSize = rkA.GetColumns();
    GMatrix<Real> kInvA = rkA;
    size_t uiSize = iSize*sizeof(Real); 
    System::Memcpy(afX,uiSize,afB,uiSize);

    int* aiColIndex = WM4_NEW int[iSize];
    int* aiRowIndex = WM4_NEW int[iSize];
    bool* abPivoted = WM4_NEW bool[iSize];
    memset(abPivoted,0,iSize*sizeof(bool));

    int i1, i2, iRow = 0, iCol = 0;
    Real fSave;

    // elimination by full pivoting
    for (int i0 = 0; i0 < iSize; i0++)
    {
        // search matrix (excluding pivoted rows) for maximum absolute entry
        Real fMax = 0.0f;
        for (i1 = 0; i1 < iSize; i1++)
        {
            if (!abPivoted[i1])
            {
                for (i2 = 0; i2 < iSize; i2++)
                {
                    if (!abPivoted[i2])
                    {
                        Real fAbs = Math<Real>::FAbs(kInvA[i1][i2]);
                        if (fAbs > fMax)
                        {
                            fMax = fAbs;
                            iRow = i1;
                            iCol = i2;
                        }
                    }
                }
            }
        }

        if (fMax == (Real)0.0)
        {
            // matrix is not invertible
            WM4_DELETE[] aiColIndex;
            WM4_DELETE[] aiRowIndex;
            WM4_DELETE[] abPivoted;
            return false;
        }

        abPivoted[iCol] = true;

        // swap rows so that A[iCol][iCol] contains the pivot entry
        if (iRow != iCol)
        {
            kInvA.SwapRows(iRow,iCol);

            fSave = afX[iRow];
            afX[iRow] = afX[iCol];
            afX[iCol] = fSave;
        }

        // keep track of the permutations of the rows
        aiRowIndex[i0] = iRow;
        aiColIndex[i0] = iCol;

        // scale the row so that the pivot entry is 1
        Real fInv = ((Real)1.0)/kInvA[iCol][iCol];
        kInvA[iCol][iCol] = (Real)1.0;
        for (i2 = 0; i2 < iSize; i2++)
        {
            kInvA[iCol][i2] *= fInv;
        }
        afX[iCol] *= fInv;

        // zero out the pivot column locations in the other rows
        for (i1 = 0; i1 < iSize; i1++)
        {
            if (i1 != iCol)
            {
                fSave = kInvA[i1][iCol];
                kInvA[i1][iCol] = (Real)0.0;
                for (i2 = 0; i2 < iSize; i2++)
                    kInvA[i1][i2] -= kInvA[iCol][i2]*fSave;
                afX[i1] -= afX[iCol]*fSave;
            }
        }
    }

    // reorder rows so that A[][] stores the inverse of the original matrix
    for (i1 = iSize-1; i1 >= 0; i1--)
    {
        if (aiRowIndex[i1] != aiColIndex[i1])
        {
            for (i2 = 0; i2 < iSize; i2++)
            {
                fSave = kInvA[i2][aiRowIndex[i1]];
                kInvA[i2][aiRowIndex[i1]] = kInvA[i2][aiColIndex[i1]];
                kInvA[i2][aiColIndex[i1]] = fSave;
            }
        }
    }

    WM4_DELETE[] aiColIndex;
    WM4_DELETE[] aiRowIndex;
    WM4_DELETE[] abPivoted;
    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool LinearSystem<Real>::SolveTri (int iSize, Real* afA, Real* afB,
    Real* afC, Real* afR, Real* afU)
{
    if (afB[0] == (Real)0.0)
    {
        return false;
    }

    Real* afD = WM4_NEW Real[iSize-1];
    Real fE = afB[0];
    Real fInvE = ((Real)1.0)/fE;
    afU[0] = afR[0]*fInvE;

    int i0, i1;
    for (i0 = 0, i1 = 1; i1 < iSize; i0++, i1++)
    {
        afD[i0] = afC[i0]*fInvE;
        fE = afB[i1] - afA[i0]*afD[i0];
        if (fE == (Real)0.0)
        {
            WM4_DELETE[] afD;
            return false;
        }
        fInvE = ((Real)1.0)/fE;
        afU[i1] = (afR[i1] - afA[i0]*afU[i0])*fInvE;
    }

    for (i0 = iSize-1, i1 = iSize-2; i1 >= 0; i0--, i1--)
    {
        afU[i1] -= afD[i1]*afU[i0];
    }

    WM4_DELETE[] afD;
    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool LinearSystem<Real>::SolveConstTri (int iSize, Real fA, Real fB, Real fC,
    Real* afR, Real* afU)
{
    if (fB == (Real)0.0)
    {
        return false;
    }

    Real* afD = WM4_NEW Real[iSize-1];
    Real fE = fB;
    Real fInvE = ((Real)1.0)/fE;
    afU[0] = afR[0]*fInvE;

    int i0, i1;
    for (i0 = 0, i1 = 1; i1 < iSize; i0++, i1++)
    {
        afD[i0] = fC*fInvE;
        fE = fB - fA*afD[i0];
        if (fE == (Real)0.0)
        {
            WM4_DELETE[] afD;
            return false;
        }
        fInvE = ((Real)1.0)/fE;
        afU[i1] = (afR[i1] - fA*afU[i0])*fInvE;
    }
    for (i0 = iSize-1, i1 = iSize-2; i1 >= 0; i0--, i1--)
    {
        afU[i1] -= afD[i1]*afU[i0];
    }

    WM4_DELETE[] afD;
    return true;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// conjugate gradient methods
//----------------------------------------------------------------------------
template <class Real>
Real LinearSystem<Real>::Dot (int iSize, const Real* afU, const Real* afV)
{
    Real fDot = (Real)0.0;
    for (int i = 0; i < iSize; i++)
    {
        fDot += afU[i]*afV[i];
    }
    return fDot;
}
//----------------------------------------------------------------------------
template <class Real>
void LinearSystem<Real>::Multiply (const GMatrix<Real>& rkA, const Real* afX,
    Real* afProd)
{
    int iSize = rkA.GetRows();
    memset(afProd,0,iSize*sizeof(Real));
    for (int iRow = 0; iRow < iSize; iRow++)
    {
        for (int iCol = 0; iCol < iSize; iCol++)
        {
            afProd[iRow] += rkA[iRow][iCol]*afX[iCol];
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
void LinearSystem<Real>::Multiply (int iSize, const SparseMatrix& rkA,
    const Real* afX, Real* afProd)
{
    memset(afProd,0,iSize*sizeof(Real));
    typename SparseMatrix::const_iterator pkIter = rkA.begin();
    for (/**/; pkIter != rkA.end(); pkIter++)
    {
        int i = pkIter->first.first;
        int j = pkIter->first.second;
        Real fValue = pkIter->second;
        afProd[i] += fValue*afX[j];
        if (i != j)
        {
            afProd[j] += fValue*afX[i];
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
void LinearSystem<Real>::UpdateX (int iSize, Real* afX, Real fAlpha,
    const Real* afP)
{
    for (int i = 0; i < iSize; i++)
    {
        afX[i] += fAlpha*afP[i];
    }
}
//----------------------------------------------------------------------------
template <class Real>
void LinearSystem<Real>::UpdateR (int iSize, Real* afR, Real fAlpha,
    const Real* afW)
{
    for (int i = 0; i < iSize; i++)
    {
        afR[i] -= fAlpha*afW[i];
    }
}
//----------------------------------------------------------------------------
template <class Real>
void LinearSystem<Real>::UpdateP (int iSize, Real* afP, Real fBeta,
    const Real* afR)
{
    for (int i = 0; i < iSize; i++)
    {
        afP[i] = afR[i] + fBeta*afP[i];
    }
}
//----------------------------------------------------------------------------
template <class Real>
bool LinearSystem<Real>::SolveSymmetricCG (const GMatrix<Real>& rkA,
    const Real* afB, Real* afX)
{
    // based on the algorithm in "Matrix Computations" by Golum and Van Loan
    assert(rkA.GetRows() == rkA.GetColumns());
    int iSize = rkA.GetRows();
    Real* afR = WM4_NEW Real[iSize];
    Real* afP = WM4_NEW Real[iSize];
    Real* afW = WM4_NEW Real[iSize];

    // first iteration
    size_t uiSize = iSize*sizeof(Real);
    memset(afX,0,uiSize);
    System::Memcpy(afR,uiSize,afB,uiSize);
    Real fRho0 = Dot(iSize,afR,afR);
    System::Memcpy(afP,uiSize,afR,uiSize);
    Multiply(rkA,afP,afW);
    Real fAlpha = fRho0/Dot(iSize,afP,afW);
    UpdateX(iSize,afX,fAlpha,afP);
    UpdateR(iSize,afR,fAlpha,afW);
    Real fRho1 = Dot(iSize,afR,afR);

    // remaining iterations
    const int iMax = 1024;
    int i;
    for (i = 1; i < iMax; i++)
    {
        Real fRoot0 = Math<Real>::Sqrt(fRho1);
        Real fNorm = Dot(iSize,afB,afB);
        Real fRoot1 = Math<Real>::Sqrt(fNorm);
        if (fRoot0 <= ZeroTolerance*fRoot1)
        {
            break;
        }

        Real fBeta = fRho1/fRho0;
        UpdateP(iSize,afP,fBeta,afR);
        Multiply(rkA,afP,afW);
        fAlpha = fRho1/Dot(iSize,afP,afW);
        UpdateX(iSize,afX,fAlpha,afP);
        UpdateR(iSize,afR,fAlpha,afW);
        fRho0 = fRho1;
        fRho1 = Dot(iSize,afR,afR);
    }

    WM4_DELETE[] afW;
    WM4_DELETE[] afP;
    WM4_DELETE[] afR;

    return i < iMax;
}
//----------------------------------------------------------------------------
template <class Real>
bool LinearSystem<Real>::SolveSymmetricCG (int iSize,
    const SparseMatrix& rkA, const Real* afB, Real* afX)
{
    // based on the algorithm in "Matrix Computations" by Golum and Van Loan
    Real* afR = WM4_NEW Real[iSize];
    Real* afP = WM4_NEW Real[iSize];
    Real* afW = WM4_NEW Real[iSize];

    // first iteration
    size_t uiSize = iSize*sizeof(Real);
    memset(afX,0,uiSize);
    System::Memcpy(afR,uiSize,afB,uiSize);
    Real fRho0 = Dot(iSize,afR,afR);
    System::Memcpy(afP,uiSize,afR,uiSize);
    Multiply(iSize,rkA,afP,afW);
    Real fAlpha = fRho0/Dot(iSize,afP,afW);
    UpdateX(iSize,afX,fAlpha,afP);
    UpdateR(iSize,afR,fAlpha,afW);
    Real fRho1 = Dot(iSize,afR,afR);

    // remaining iterations
    const int iMax = 1024;
    int i;
    for (i = 1; i < iMax; i++)
    {
        Real fRoot0 = Math<Real>::Sqrt(fRho1);
        Real fNorm = Dot(iSize,afB,afB);
        Real fRoot1 = Math<Real>::Sqrt(fNorm);
        if (fRoot0 <= ZeroTolerance*fRoot1)
        {
            break;
        }

        Real fBeta = fRho1/fRho0;
        UpdateP(iSize,afP,fBeta,afR);
        Multiply(iSize,rkA,afP,afW);
        fAlpha = fRho1/Dot(iSize,afP,afW);
        UpdateX(iSize,afX,fAlpha,afP);
        UpdateR(iSize,afR,fAlpha,afW);
        fRho0 = fRho1;
        fRho1 = Dot(iSize,afR,afR);
    }

    WM4_DELETE[] afW;
    WM4_DELETE[] afP;
    WM4_DELETE[] afR;

    return i < iMax;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// banded matrices
//----------------------------------------------------------------------------
template <class Real>
bool LinearSystem<Real>::ForwardEliminate (int iReduceRow,
    BandedMatrix<Real>& rkA, Real* afB)
{
    // the pivot must be nonzero in order to proceed
    Real fDiag = rkA(iReduceRow,iReduceRow);
    if (fDiag == (Real)0.0)
    {
        return false;
    }

    Real fInvDiag = ((Real)1.0)/fDiag;
    rkA(iReduceRow,iReduceRow) = (Real)1.0;

    // multiply row to be consistent with diagonal term of 1
    int iColMin = iReduceRow + 1;
    int iColMax = iColMin + rkA.GetUBands();
    if (iColMax > rkA.GetSize())
    {
        iColMax = rkA.GetSize();
    }

    int iCol;
    for (iCol = iColMin; iCol < iColMax; iCol++)
    {
        rkA(iReduceRow,iCol) *= fInvDiag;
    }

    afB[iReduceRow] *= fInvDiag;

    // reduce remaining rows
    int iRowMin = iReduceRow + 1;
    int iRowMax = iRowMin + rkA.GetLBands();
    if (iRowMax > rkA.GetSize())
    {
        iRowMax = rkA.GetSize();
    }

    for (int iRow = iRowMin; iRow < iRowMax; iRow++)
    {
        Real fMult = rkA(iRow,iReduceRow);
        rkA(iRow,iReduceRow) = (Real)0.0;
        for (iCol = iColMin; iCol < iColMax; iCol++)
        {
            rkA(iRow,iCol) -= fMult*rkA(iReduceRow,iCol);
        }
        afB[iRow] -= fMult*afB[iReduceRow];
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool LinearSystem<Real>::SolveBanded (const BandedMatrix<Real>& rkA,
    const Real* afB, Real* afX)
{
    BandedMatrix<Real> kTmp = rkA;
    int iSize = rkA.GetSize();
    size_t uiSize = iSize*sizeof(Real); 
    System::Memcpy(afX,uiSize,afB,uiSize);

    // forward elimination
    int iRow;
    for (iRow = 0; iRow < iSize; iRow++)
    {
        if (!ForwardEliminate(iRow,kTmp,afX))
        {
            return false;
        }
    }

    // backward substitution
    for (iRow = iSize-2; iRow >= 0; iRow--)
    {
        int iColMin = iRow + 1;
        int iColMax = iColMin + kTmp.GetUBands();
        if (iColMax > iSize)
        {
            iColMax = iSize;
        }
        for (int iCol = iColMin; iCol < iColMax; iCol++)
        {
            afX[iRow] -= kTmp(iRow,iCol)*afX[iCol];
        }
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool LinearSystem<Real>::ForwardEliminate (int iReduceRow,
    BandedMatrix<Real>& rkA, GMatrix<Real>& rkB)
{
    // the pivot must be nonzero in order to proceed
    Real fDiag = rkA(iReduceRow,iReduceRow);
    if (fDiag == (Real)0.0)
    {
        return false;
    }

    Real fInvDiag = ((Real)1.0)/fDiag;
    rkA(iReduceRow,iReduceRow) = (Real)1.0;

    // multiply row to be consistent with diagonal term of 1
    int iColMin = iReduceRow + 1;
    int iColMax = iColMin + rkA.GetUBands();
    if (iColMax > rkA.GetSize())
    {
        iColMax = rkA.GetSize();
    }

    int iCol;
    for (iCol = iColMin; iCol < iColMax; iCol++)
    {
        rkA(iReduceRow,iCol) *= fInvDiag;
    }
    for (iCol = 0; iCol <= iReduceRow; iCol++)
    {
        rkB(iReduceRow,iCol) *= fInvDiag;
    }

    // reduce remaining rows
    int iRowMin = iReduceRow + 1;
    int iRowMax = iRowMin + rkA.GetLBands();
    if (iRowMax > rkA.GetSize())
    {
        iRowMax = rkA.GetSize();
    }

    for (int iRow = iRowMin; iRow < iRowMax; iRow++)
    {
        Real fMult = rkA(iRow,iReduceRow);
        rkA(iRow,iReduceRow) = (Real)0.0;
        for (iCol = iColMin; iCol < iColMax; iCol++)
        {
            rkA(iRow,iCol) -= fMult*rkA(iReduceRow,iCol);
        }
        for (iCol = 0; iCol <= iReduceRow; iCol++)
        {
            rkB(iRow,iCol) -= fMult*rkB(iReduceRow,iCol);
        }
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
void LinearSystem<Real>::BackwardEliminate (int iReduceRow,
    BandedMatrix<Real>& rkA, GMatrix<Real>& rkB)
{
    int iRowMax = iReduceRow - 1;
    int iRowMin = iReduceRow - rkA.GetUBands();
    if (iRowMin < 0)
    {
        iRowMin = 0;
    }

    for (int iRow = iRowMax; iRow >= iRowMin; iRow--)
    {
        Real fMult = rkA(iRow,iReduceRow);
        rkA(iRow,iReduceRow) = (Real)0.0;
        for (int iCol = 0; iCol < rkB.GetColumns(); iCol++)
        {
            rkB(iRow,iCol) -= fMult*rkB(iReduceRow,iCol);
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
bool LinearSystem<Real>::Invert (const BandedMatrix<Real>& rkA,
    GMatrix<Real>& rkInvA)
{
    int iSize = rkA.GetSize();
    BandedMatrix<Real> kTmp = rkA;
    int iRow;
    for (iRow = 0; iRow < iSize; iRow++)
    {
        for (int iCol = 0; iCol < iSize; iCol++)
        {
            if (iRow != iCol)
            {
                rkInvA(iRow,iCol) = (Real)0.0;
            }
            else
            {
                rkInvA(iRow,iRow) = (Real)1.0;
            }
        }
    }

    // forward elimination
    for (iRow = 0; iRow < iSize; iRow++)
    {
        if (!ForwardEliminate(iRow,kTmp,rkInvA))
        {
            return false;
        }
    }

    // backward elimination
    for (iRow = iSize-1; iRow >= 1; iRow--)
    {
        BackwardEliminate(iRow,kTmp,rkInvA);
    }

    return true;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class LinearSystem<float>;

template WM4_FOUNDATION_ITEM
class LinearSystem<double>;
//----------------------------------------------------------------------------
}
