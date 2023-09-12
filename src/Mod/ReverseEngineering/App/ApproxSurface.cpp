/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentMap>

#include <Geom_BSplineSurface.hxx>
#include <Precision.hxx>
#include <math_Gauss.hxx>
#include <math_Householder.hxx>
#endif

#include <Base/Sequencer.h>
#include <Base/Tools.h>
#include <Mod/Mesh/App/Core/Approximation.h>

#include "ApproxSurface.h"


using namespace Reen;
namespace sp = std::placeholders;

// SplineBasisfunction

SplineBasisfunction::SplineBasisfunction(int iSize)
    : _vKnotVector(0, iSize - 1)
    , _iOrder(1)
{}

SplineBasisfunction::SplineBasisfunction(TColStd_Array1OfReal& vKnots,
                                         TColStd_Array1OfInteger& vMults,
                                         int iSize,
                                         int iOrder)
    : _vKnotVector(0, iSize - 1)
{
    int sum = 0;
    for (int h = vMults.Lower(); h <= vMults.Upper(); h++) {
        sum += vMults(h);
    }

    if (vKnots.Length() != vMults.Length() || iSize != sum) {
        // Throw exception
        Standard_ConstructionError::Raise("BSplineBasis");
    }

    int k = 0;
    for (int i = vMults.Lower(); i <= vMults.Upper(); i++) {
        for (int j = 0; j < vMults(i); j++) {
            _vKnotVector(k) = vKnots(i);
            k++;
        }
    }

    _iOrder = iOrder;
}

SplineBasisfunction::SplineBasisfunction(TColStd_Array1OfReal& vKnots, int iOrder)
    : _vKnotVector(0, vKnots.Length() - 1)
{
    _vKnotVector = vKnots;
    _iOrder = iOrder;
}

SplineBasisfunction::~SplineBasisfunction() = default;

void SplineBasisfunction::SetKnots(TColStd_Array1OfReal& vKnots, int iOrder)
{
    if (_vKnotVector.Length() != vKnots.Length()) {
        Standard_RangeError::Raise("BSplineBasis");
    }

    _vKnotVector = vKnots;
    _iOrder = iOrder;
}

void SplineBasisfunction::SetKnots(TColStd_Array1OfReal& vKnots,
                                   TColStd_Array1OfInteger& vMults,
                                   int iOrder)
{
    int sum = 0;
    for (int h = vMults.Lower(); h <= vMults.Upper(); h++) {
        sum += vMults(h);
    }

    if (vKnots.Length() != vMults.Length() || _vKnotVector.Length() != sum) {
        // Throw exception
        Standard_RangeError::Raise("BSplineBasis");
    }
    int k = 0;
    for (int i = vMults.Lower(); i <= vMults.Upper(); i++) {
        for (int j = 0; j < vMults(i); j++) {
            _vKnotVector(k) = vKnots(i);
            k++;
        }
    }

    _iOrder = iOrder;
}

////////////////////////////////////////// BSplineBasis

BSplineBasis::BSplineBasis(int iSize)
    : SplineBasisfunction(iSize)
{}

BSplineBasis::BSplineBasis(TColStd_Array1OfReal& vKnots,
                           TColStd_Array1OfInteger& vMults,
                           int iSize,
                           int iOrder)
    : SplineBasisfunction(vKnots, vMults, iSize, iOrder)
{}

BSplineBasis::BSplineBasis(TColStd_Array1OfReal& vKnots, int iOrder)
    : SplineBasisfunction(vKnots, iOrder)
{}

BSplineBasis::~BSplineBasis() = default;

int BSplineBasis::FindSpan(double fParam)
{
    int n = _vKnotVector.Length() - _iOrder - 1;
    if (fParam == _vKnotVector(n + 1)) {
        return n;
    }

    int low = _iOrder - 1;
    int high = n + 1;
    int mid = (low + high) / 2;  // Binary search

    while (fParam < _vKnotVector(mid) || fParam >= _vKnotVector(mid + 1)) {
        if (fParam < _vKnotVector(mid)) {
            high = mid;
        }
        else {
            low = mid;
        }
        mid = (low + high) / 2;
    }

    return mid;
}

void BSplineBasis::AllBasisFunctions(double fParam, TColStd_Array1OfReal& vFuncVals)
{
    if (vFuncVals.Length() != _iOrder) {
        Standard_RangeError::Raise("BSplineBasis");
    }

    int iIndex = FindSpan(fParam);

    TColStd_Array1OfReal zaehler_left(1, _iOrder - 1);
    TColStd_Array1OfReal zaehler_right(1, _iOrder - 1);
    vFuncVals(0) = 1.0;

    for (int j = 1; j < _iOrder; j++) {
        zaehler_left(j) = fParam - _vKnotVector(iIndex + 1 - j);
        zaehler_right(j) = _vKnotVector(iIndex + j) - fParam;
        double saved = 0.0;
        for (int r = 0; r < j; r++) {
            double tmp = vFuncVals(r) / (zaehler_right(r + 1) + zaehler_left(j - r));
            vFuncVals(r) = saved + zaehler_right(r + 1) * tmp;
            saved = zaehler_left(j - r) * tmp;
        }

        vFuncVals(j) = saved;
    }
}

BSplineBasis::ValueT BSplineBasis::LocalSupport(int iIndex, double fParam)
{
    int m = _vKnotVector.Length() - 1;
    int p = _iOrder - 1;

    if ((iIndex == 0 && fParam == _vKnotVector(0))
        || (iIndex == m - p - 1 && fParam == _vKnotVector(m))) {
        return BSplineBasis::Full;
    }

    if (fParam < _vKnotVector(iIndex) || fParam >= _vKnotVector(iIndex + p + 1)) {
        return BSplineBasis::Zero;
    }

    return BSplineBasis::Other;
}

double BSplineBasis::BasisFunction(int iIndex, double fParam)
{
    int m = _vKnotVector.Length() - 1;
    int p = _iOrder - 1;
    double saved;
    TColStd_Array1OfReal N(0, p);

    if ((iIndex == 0 && fParam == _vKnotVector(0))
        || (iIndex == m - p - 1 && fParam == _vKnotVector(m))) {
        return 1.0;
    }

    if (fParam < _vKnotVector(iIndex) || fParam >= _vKnotVector(iIndex + p + 1)) {
        return 0.0;
    }

    for (int j = 0; j <= p; j++) {
        if (fParam >= _vKnotVector(iIndex + j) && fParam < _vKnotVector(iIndex + j + 1)) {
            N(j) = 1.0;
        }
        else {
            N(j) = 0.0;
        }
    }

    for (int k = 1; k <= p; k++) {
        if (N(0) == 0.0) {
            saved = 0.0;
        }
        else {
            saved = ((fParam - _vKnotVector(iIndex)) * N(0))
                / (_vKnotVector(iIndex + k) - _vKnotVector(iIndex));
        }

        for (int j = 0; j < p - k + 1; j++) {
            double Tleft = _vKnotVector(iIndex + j + 1);
            double Tright = _vKnotVector(iIndex + j + k + 1);

            if (N(j + 1) == 0.0) {
                N(j) = saved;
                saved = 0.0;
            }
            else {
                double tmp = N(j + 1) / (Tright - Tleft);
                N(j) = saved + (Tright - fParam) * tmp;
                saved = (fParam - Tleft) * tmp;
            }
        }
    }

    return N(0);
}

void BSplineBasis::DerivativesOfBasisFunction(int iIndex,
                                              int iMaxDer,
                                              double fParam,
                                              TColStd_Array1OfReal& Derivat)
{
    int iMax = iMaxDer;
    if (Derivat.Length() != iMax + 1) {
        Standard_RangeError::Raise("BSplineBasis");
    }
    // kth derivatives (k> degrees) are zero
    if (iMax >= _iOrder) {
        for (int i = _iOrder; i <= iMaxDer; i++) {
            Derivat(i) = 0.0;
        }
        iMax = _iOrder - 1;
    }

    TColStd_Array1OfReal ND(0, iMax);
    int p = _iOrder - 1;
    math_Matrix N(0, p, 0, p);
    double saved;

    // if value is outside the interval, then function value and all derivatives equal null
    if (fParam < _vKnotVector(iIndex) || fParam >= _vKnotVector(iIndex + p + 1)) {
        for (int k = 0; k <= iMax; k++) {
            Derivat(k) = 0.0;
        }
        return;
    }

    // Calculate the basis functions of Order 1
    for (int j = 0; j < _iOrder; j++) {
        if (fParam >= _vKnotVector(iIndex + j) && fParam < _vKnotVector(iIndex + j + 1)) {
            N(j, 0) = 1.0;
        }
        else {
            N(j, 0) = 0.0;
        }
    }

    // Calculate a triangular table of the function values
    for (int k = 1; k < _iOrder; k++) {
        if (N(0, k - 1) == 0.0) {
            saved = 0.0;
        }
        else {
            saved = ((fParam - _vKnotVector(iIndex)) * N(0, k - 1))
                / (_vKnotVector(iIndex + k) - _vKnotVector(iIndex));
        }
        for (int j = 0; j < p - k + 1; j++) {
            double Tleft = _vKnotVector(iIndex + j + 1);
            double Tright = _vKnotVector(iIndex + j + k + 1);

            if (N(j + 1, k - 1) == 0.0) {
                N(j, k) = saved;
                saved = 0.0;
            }
            else {
                double tmp = N(j + 1, k - 1) / (Tright - Tleft);
                N(j, k) = saved + (Tright - fParam) * tmp;
                saved = (fParam - Tleft) * tmp;
            }
        }
    }

    // Function value
    Derivat(0) = N(0, p);
    // Calculate the derivatives from the triangle table
    for (int k = 1; k <= iMax; k++) {
        for (int j = 0; j <= k; j++) {
            // Load the (p-k)th column
            ND(j) = N(j, p - k);
        }

        for (int jj = 1; jj <= k; jj++) {
            if (ND(0) == 0.0) {
                saved = 0.0;
            }
            else {
                saved = ND(0) / (_vKnotVector(iIndex + p - k + jj) - _vKnotVector(iIndex));
            }

            for (int j = 0; j < k - jj + 1; j++) {
                double Tleft = _vKnotVector(iIndex + j + 1);
                double Tright = _vKnotVector(iIndex + j + p - k + jj + 1);
                if (ND(j + 1) == 0.0) {
                    ND(j) = (p - k + jj) * saved;
                    saved = 0.0;
                }
                else {
                    double tmp = ND(j + 1) / (Tright - Tleft);
                    ND(j) = (p - k + jj) * (saved - tmp);
                    saved = tmp;
                }
            }
        }

        Derivat(k) = ND(0);  // kth derivative
    }
}

double BSplineBasis::DerivativeOfBasisFunction(int iIndex, int iMaxDer, double fParam)
{
    int iMax = iMaxDer;

    // Function value (0th derivative)
    if (iMax == 0) {
        return BasisFunction(iIndex, fParam);
    }

    // The kth derivatives (k>degrees) are null
    if (iMax >= _iOrder) {
        return 0.0;
    }

    TColStd_Array1OfReal ND(0, iMax);
    int p = _iOrder - 1;
    math_Matrix N(0, p, 0, p);
    double saved;

    // If value is outside the interval, then function value and derivatives equal null
    if (fParam < _vKnotVector(iIndex) || fParam >= _vKnotVector(iIndex + p + 1)) {
        return 0.0;
    }

    // Calculate the basis functions of Order 1
    for (int j = 0; j < _iOrder; j++) {
        if (fParam >= _vKnotVector(iIndex + j) && fParam < _vKnotVector(iIndex + j + 1)) {
            N(j, 0) = 1.0;
        }
        else {
            N(j, 0) = 0.0;
        }
    }

    // Calculate triangular table of function values
    for (int k = 1; k < _iOrder; k++) {
        if (N(0, k - 1) == 0.0) {
            saved = 0.0;
        }
        else {
            saved = ((fParam - _vKnotVector(iIndex)) * N(0, k - 1))
                / (_vKnotVector(iIndex + k) - _vKnotVector(iIndex));
        }

        for (int j = 0; j < p - k + 1; j++) {
            double Tleft = _vKnotVector(iIndex + j + 1);
            double Tright = _vKnotVector(iIndex + j + k + 1);

            if (N(j + 1, k - 1) == 0.0) {
                N(j, k) = saved;
                saved = 0.0;
            }
            else {
                double tmp = N(j + 1, k - 1) / (Tright - Tleft);
                N(j, k) = saved + (Tright - fParam) * tmp;
                saved = (fParam - Tleft) * tmp;
            }
        }
    }

    // Use the triangular table to calculate the derivatives
    for (int j = 0; j <= iMax; j++) {
        // Loading (p-iMax)th column
        ND(j) = N(j, p - iMax);
    }

    for (int jj = 1; jj <= iMax; jj++) {
        if (ND(0) == 0.0) {
            saved = 0.0;
        }
        else {
            saved = ND(0) / (_vKnotVector(iIndex + p - iMax + jj) - _vKnotVector(iIndex));
        }

        for (int j = 0; j < iMax - jj + 1; j++) {
            double Tleft = _vKnotVector(iIndex + j + 1);
            double Tright = _vKnotVector(iIndex + j + p - iMax + jj + 1);
            if (ND(j + 1) == 0.0) {
                ND(j) = (p - iMax + jj) * saved;
                saved = 0.0;
            }
            else {
                double tmp = ND(j + 1) / (Tright - Tleft);
                ND(j) = (p - iMax + jj) * (saved - tmp);
                saved = tmp;
            }
        }
    }

    return ND(0);  // iMax-th derivative
}

double BSplineBasis::GetIntegralOfProductOfBSplines(int iIdx1, int iIdx2, int iOrd1, int iOrd2)
{
    int iMax = CalcSize(iOrd1, iOrd2);
    double dIntegral = 0.0;
    double fMin, fMax;

    TColStd_Array1OfReal vRoots(0, iMax), vWeights(0, iMax);
    GenerateRootsAndWeights(vRoots, vWeights);

    /*Calculate the integral*/
    // Integration area
    int iBegin = 0;
    int iEnd = 0;
    FindIntegrationArea(iIdx1, iIdx2, iBegin, iEnd);

    for (int j = iBegin; j < iEnd; j++) {
        fMax = _vKnotVector(j + 1);
        fMin = _vKnotVector(j);

        if (fMax > fMin) {
            for (int i = 0; i <= iMax; i++) {
                double fParam = 0.5 * (vRoots(i) + 1) * (fMax - fMin) + fMin;
                dIntegral += 0.5 * (fMax - fMin) * vWeights(i)
                    * DerivativeOfBasisFunction(iIdx1, iOrd1, fParam)
                    * DerivativeOfBasisFunction(iIdx2, iOrd2, fParam);
            }
        }
    }

    return dIntegral;
}

void BSplineBasis::GenerateRootsAndWeights(TColStd_Array1OfReal& vRoots,
                                           TColStd_Array1OfReal& vWeights)
{
    int iSize = vRoots.Length();

    // Zeroing the Legendre-Polynomials and the corresponding weights
    if (iSize == 1) {
        vRoots(0) = 0.0;
        vWeights(0) = 2.0;
    }
    else if (iSize == 2) {
        vRoots(0) = 0.57735;
        vWeights(0) = 1.0;
        vRoots(1) = -vRoots(0);
        vWeights(1) = vWeights(0);
    }
    else if (iSize == 4) {
        vRoots(0) = 0.33998;
        vWeights(0) = 0.65214;
        vRoots(1) = 0.86113;
        vWeights(1) = 0.34785;
        vRoots(2) = -vRoots(0);
        vWeights(2) = vWeights(0);
        vRoots(3) = -vRoots(1);
        vWeights(3) = vWeights(1);
    }
    else if (iSize == 6) {
        vRoots(0) = 0.23861;
        vWeights(0) = 0.46791;
        vRoots(1) = 0.66120;
        vWeights(1) = 0.36076;
        vRoots(2) = 0.93246;
        vWeights(2) = 0.17132;
        vRoots(3) = -vRoots(0);
        vWeights(3) = vWeights(0);
        vRoots(4) = -vRoots(1);
        vWeights(4) = vWeights(1);
        vRoots(5) = -vRoots(2);
        vWeights(5) = vWeights(2);
    }
    else if (iSize == 8) {
        vRoots(0) = 0.18343;
        vWeights(0) = 0.36268;
        vRoots(1) = 0.52553;
        vWeights(1) = 0.31370;
        vRoots(2) = 0.79666;
        vWeights(2) = 0.22238;
        vRoots(3) = 0.96028;
        vWeights(3) = 0.10122;
        vRoots(4) = -vRoots(0);
        vWeights(4) = vWeights(0);
        vRoots(5) = -vRoots(1);
        vWeights(5) = vWeights(1);
        vRoots(6) = -vRoots(2);
        vWeights(6) = vWeights(2);
        vRoots(7) = -vRoots(3);
        vWeights(7) = vWeights(3);
    }
    else if (iSize == 10) {
        vRoots(0) = 0.14887;
        vWeights(0) = 0.29552;
        vRoots(1) = 0.43339;
        vWeights(1) = 0.26926;
        vRoots(2) = 0.67940;
        vWeights(2) = 0.21908;
        vRoots(3) = 0.86506;
        vWeights(3) = 0.14945;
        vRoots(4) = 0.97390;
        vWeights(4) = 0.06667;
        vRoots(5) = -vRoots(0);
        vWeights(5) = vWeights(0);
        vRoots(6) = -vRoots(1);
        vWeights(6) = vWeights(1);
        vRoots(7) = -vRoots(2);
        vWeights(7) = vWeights(2);
        vRoots(8) = -vRoots(3);
        vWeights(8) = vWeights(3);
        vRoots(9) = -vRoots(4);
        vWeights(9) = vWeights(4);
    }
    else {
        vRoots(0) = 0.12523;
        vWeights(0) = 0.24914;
        vRoots(1) = 0.36783;
        vWeights(1) = 0.23349;
        vRoots(2) = 0.58731;
        vWeights(2) = 0.20316;
        vRoots(3) = 0.76990;
        vWeights(3) = 0.16007;
        vRoots(4) = 0.90411;
        vWeights(4) = 0.10693;
        vRoots(5) = 0.98156;
        vWeights(5) = 0.04717;
        vRoots(6) = -vRoots(0);
        vWeights(6) = vWeights(0);
        vRoots(7) = -vRoots(1);
        vWeights(7) = vWeights(1);
        vRoots(8) = -vRoots(2);
        vWeights(8) = vWeights(2);
        vRoots(9) = -vRoots(3);
        vWeights(9) = vWeights(3);
        vRoots(10) = -vRoots(4);
        vWeights(10) = vWeights(4);
        vRoots(11) = -vRoots(5);
        vWeights(11) = vWeights(5);
    }
}

void BSplineBasis::FindIntegrationArea(int iIdx1, int iIdx2, int& iBegin, int& iEnd)
{
    // order by index
    if (iIdx2 < iIdx1) {
        int tmp = iIdx1;
        iIdx1 = iIdx2;
        iIdx2 = tmp;
    }

    iBegin = iIdx2;
    iEnd = iIdx1 + _iOrder;
    if (iEnd == _vKnotVector.Upper()) {
        iEnd -= 1;
    }
}

int BSplineBasis::CalcSize(int r, int s)
{
    int iMaxDegree = 2 * (_iOrder - 1) - r - s;

    if (iMaxDegree < 0) {
        return 0;
    }
    else if (iMaxDegree < 4) {
        return 1;
    }
    else if (iMaxDegree < 8) {
        return 3;
    }
    else if (iMaxDegree < 12) {
        return 5;
    }
    else if (iMaxDegree < 16) {
        return 7;
    }
    else if (iMaxDegree < 20) {
        return 9;
    }
    else {
        return 11;
    }
}

/////////////////// ParameterCorrection

ParameterCorrection::ParameterCorrection(unsigned usUOrder,
                                         unsigned usVOrder,
                                         unsigned usUCtrlpoints,
                                         unsigned usVCtrlpoints)
    : _usUOrder(usUOrder)
    , _usVOrder(usVOrder)
    , _usUCtrlpoints(usUCtrlpoints)
    , _usVCtrlpoints(usVCtrlpoints)
    , _vCtrlPntsOfSurf(0, usUCtrlpoints - 1, 0, usVCtrlpoints - 1)
    , _vUKnots(0, usUCtrlpoints - usUOrder + 1)
    , _vVKnots(0, usVCtrlpoints - usVOrder + 1)
    , _vUMults(0, usUCtrlpoints - usUOrder + 1)
    , _vVMults(0, usVCtrlpoints - usVOrder + 1)
{
    _bGetUVDir = false;
    _bSmoothing = false;
    _fSmoothInfluence = 0.0;
}

void ParameterCorrection::CalcEigenvectors()
{
    MeshCore::PlaneFit planeFit;
    for (int i = _pvcPoints->Lower(); i <= _pvcPoints->Upper(); i++) {
        const gp_Pnt& pnt = (*_pvcPoints)(i);
        planeFit.AddPoint(Base::Vector3f((float)pnt.X(), (float)pnt.Y(), (float)pnt.Z()));
    }

    planeFit.Fit();
    _clU = Base::toVector<double>(planeFit.GetDirU());
    _clV = Base::toVector<double>(planeFit.GetDirV());
    _clW = Base::toVector<double>(planeFit.GetNormal());
}

bool ParameterCorrection::DoInitialParameterCorrection(double fSizeFactor)
{
    // if directions are not given, calculate yourself
    if (!_bGetUVDir) {
        CalcEigenvectors();
    }
    if (!GetUVParameters(fSizeFactor)) {
        return false;
    }
    if (_bSmoothing) {
        if (!SolveWithSmoothing(_fSmoothInfluence)) {
            return false;
        }
    }
    else {
        if (!SolveWithoutSmoothing()) {
            return false;
        }
    }

    return true;
}

bool ParameterCorrection::GetUVParameters(double fSizeFactor)
{
    // Eigenvectors as a new base
    Base::Vector3d e[3];
    e[0] = _clU;
    e[1] = _clV;
    e[2] = _clW;

    // Canonical base of R^3
    Base::Vector3d b[3];
    b[0] = Base::Vector3d(1.0, 0.0, 0.0);
    b[1] = Base::Vector3d(0.0, 1.0, 0.0);
    b[2] = Base::Vector3d(0.0, 0.0, 1.0);
    // Create a right system from the orthogonal eigenvectors
    if ((e[0] % e[1]) * e[2] < 0) {
        Base::Vector3d tmp = e[0];
        e[0] = e[1];
        e[1] = tmp;
    }

    // Now generate the transpon. Rotation matrix
    Wm4::Matrix3d clRotMatTrans;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            clRotMatTrans[i][j] = b[j] * e[i];
        }
    }

    std::vector<Base::Vector2d> vcProjPts;
    Base::BoundBox2d clBBox;

    // Calculate the coordinates of the transf. Points and project
    // these on to the x,y-plane of the new coordinate system
    for (int ii = _pvcPoints->Lower(); ii <= _pvcPoints->Upper(); ii++) {
        const gp_Pnt& pnt = (*_pvcPoints)(ii);
        Wm4::Vector3d clProjPnt = clRotMatTrans * Wm4::Vector3d(pnt.X(), pnt.Y(), pnt.Z());
        vcProjPts.emplace_back(clProjPnt.X(), clProjPnt.Y());
        clBBox.Add(Base::Vector2d(clProjPnt.X(), clProjPnt.Y()));
    }

    if ((clBBox.MaxX == clBBox.MinX) || (clBBox.MaxY == clBBox.MinY)) {
        return false;
    }
    double tx = fSizeFactor * clBBox.MinX - (fSizeFactor - 1.0) * clBBox.MaxX;
    double ty = fSizeFactor * clBBox.MinY - (fSizeFactor - 1.0) * clBBox.MaxY;
    double fDeltaX = (2 * fSizeFactor - 1.0) * (clBBox.MaxX - clBBox.MinX);
    double fDeltaY = (2 * fSizeFactor - 1.0) * (clBBox.MaxY - clBBox.MinY);

    // Calculate the u,v parameters with u,v from [0,1]
    _pvcUVParam->Init(gp_Pnt2d(0.0, 0.0));
    int ii = 0;
    if (clBBox.MaxX - clBBox.MinX >= clBBox.MaxY - clBBox.MinY) {
        for (const auto& pt : vcProjPts) {
            (*_pvcUVParam)(ii) = gp_Pnt2d((pt.x - tx) / fDeltaX, (pt.y - ty) / fDeltaY);
            ii++;
        }
    }
    else {
        for (const auto& pt : vcProjPts) {
            (*_pvcUVParam)(ii) = gp_Pnt2d((pt.y - ty) / fDeltaY, (pt.x - tx) / fDeltaX);
            ii++;
        }
    }

    return true;
}

void ParameterCorrection::SetUV(const Base::Vector3d& clU, const Base::Vector3d& clV, bool bUseDir)
{
    _bGetUVDir = bUseDir;
    if (_bGetUVDir) {
        _clU = clU;
        _clW = clU % clV;
        _clV = _clW % _clU;
    }
}

void ParameterCorrection::GetUVW(Base::Vector3d& clU,
                                 Base::Vector3d& clV,
                                 Base::Vector3d& clW) const
{
    clU = _clU;
    clV = _clV;
    clW = _clW;
}

Base::Vector3d ParameterCorrection::GetGravityPoint() const
{
    Standard_Integer ulSize = _pvcPoints->Length();
    double x = 0.0, y = 0.0, z = 0.0;
    for (int i = _pvcPoints->Lower(); i <= _pvcPoints->Upper(); i++) {
        const gp_Pnt& pnt = (*_pvcPoints)(i);
        x += pnt.X();
        y += pnt.Y();
        z += pnt.Z();
    }

    return Base::Vector3d(x / ulSize, y / ulSize, z / ulSize);
}

void ParameterCorrection::ProjectControlPointsOnPlane()
{
    Base::Vector3d base = GetGravityPoint();
    for (unsigned j = 0; j < _usUCtrlpoints; j++) {
        for (unsigned k = 0; k < _usVCtrlpoints; k++) {
            gp_Pnt pole = _vCtrlPntsOfSurf(j, k);
            Base::Vector3d pnt(pole.X(), pole.Y(), pole.Z());
            pnt.ProjectToPlane(base, _clW);
            pole.SetX(pnt.x);
            pole.SetY(pnt.y);
            pole.SetZ(pnt.z);
            _vCtrlPntsOfSurf(j, k) = pole;
        }
    }
}

Handle(Geom_BSplineSurface) ParameterCorrection::CreateSurface(const TColgp_Array1OfPnt& points,
                                                               int iIter,
                                                               bool bParaCor,
                                                               double fSizeFactor)
{
    if (_pvcPoints) {
        delete _pvcPoints;
        _pvcPoints = nullptr;
        delete _pvcUVParam;
        _pvcUVParam = nullptr;
    }

    _pvcPoints = new TColgp_Array1OfPnt(points.Lower(), points.Upper());
    *_pvcPoints = points;
    _pvcUVParam = new TColgp_Array1OfPnt2d(points.Lower(), points.Upper());

    if (_usUCtrlpoints * _usVCtrlpoints > static_cast<unsigned>(_pvcPoints->Length())) {
        return nullptr;  // LGS under-determined
    }
    if (!DoInitialParameterCorrection(fSizeFactor)) {
        return nullptr;
    }

    // Generate the approximation plane as a B-spline area
    if (iIter < 0) {
        bParaCor = false;
        ProjectControlPointsOnPlane();
    }
    // No further parameter corrections
    else if (iIter == 0) {
        bParaCor = false;
    }

    if (bParaCor) {
        DoParameterCorrection(iIter);
    }

    return new Geom_BSplineSurface(_vCtrlPntsOfSurf,
                                   _vUKnots,
                                   _vVKnots,
                                   _vUMults,
                                   _vVMults,
                                   _usUOrder - 1,
                                   _usVOrder - 1);
}

void ParameterCorrection::EnableSmoothing(bool bSmooth, double fSmoothInfl)
{
    _bSmoothing = bSmooth;
    _fSmoothInfluence = fSmoothInfl;
}

/////////////////// BSplineParameterCorrection


BSplineParameterCorrection::BSplineParameterCorrection(unsigned usUOrder,
                                                       unsigned usVOrder,
                                                       unsigned usUCtrlpoints,
                                                       unsigned usVCtrlpoints)
    : ParameterCorrection(usUOrder, usVOrder, usUCtrlpoints, usVCtrlpoints)
    , _clUSpline(usUCtrlpoints + usUOrder)
    , _clVSpline(usVCtrlpoints + usVOrder)
    , _clSmoothMatrix(0, usUCtrlpoints * usVCtrlpoints - 1, 0, usUCtrlpoints * usVCtrlpoints - 1)
    , _clFirstMatrix(0, usUCtrlpoints * usVCtrlpoints - 1, 0, usUCtrlpoints * usVCtrlpoints - 1)
    , _clSecondMatrix(0, usUCtrlpoints * usVCtrlpoints - 1, 0, usUCtrlpoints * usVCtrlpoints - 1)
    , _clThirdMatrix(0, usUCtrlpoints * usVCtrlpoints - 1, 0, usUCtrlpoints * usVCtrlpoints - 1)
{
    Init();
}

void BSplineParameterCorrection::Init()
{
    // Initializations
    _pvcUVParam = nullptr;
    _pvcPoints = nullptr;
    _clFirstMatrix.Init(0.0);
    _clSecondMatrix.Init(0.0);
    _clThirdMatrix.Init(0.0);
    _clSmoothMatrix.Init(0.0);

    /* Calculate the knot vectors */
    unsigned usUMax = _usUCtrlpoints - _usUOrder + 1;
    unsigned usVMax = _usVCtrlpoints - _usVOrder + 1;

    // Knot vector for the CAS.CADE class
    // u-direction
    for (unsigned i = 0; i <= usUMax; i++) {
        _vUKnots(i) = static_cast<double>(i) / static_cast<double>(usUMax);
        _vUMults(i) = 1;
    }

    _vUMults(0) = _usUOrder;
    _vUMults(usUMax) = _usUOrder;

    // v-direction
    for (unsigned i = 0; i <= usVMax; i++) {
        _vVKnots(i) = static_cast<double>(i) / static_cast<double>(usVMax);
        _vVMults(i) = 1;
    }

    _vVMults(0) = _usVOrder;
    _vVMults(usVMax) = _usVOrder;

    // Set the B-spline basic functions
    _clUSpline.SetKnots(_vUKnots, _vUMults, _usUOrder);
    _clVSpline.SetKnots(_vVKnots, _vVMults, _usVOrder);
}

void BSplineParameterCorrection::SetUKnots(const std::vector<double>& afKnots)
{
    std::size_t numPoints = static_cast<std::size_t>(_usUCtrlpoints);
    std::size_t order = static_cast<std::size_t>(_usUOrder);
    if (afKnots.size() != (numPoints + order)) {
        return;
    }

    unsigned usUMax = _usUCtrlpoints - _usUOrder + 1;

    // Knot vector for the CAS.CADE class
    // u-direction
    for (unsigned i = 1; i < usUMax; i++) {
        _vUKnots(i) = afKnots[_usUOrder + i - 1];
        _vUMults(i) = 1;
    }

    // Set the B-spline basic functions
    _clUSpline.SetKnots(_vUKnots, _vUMults, _usUOrder);
}

void BSplineParameterCorrection::SetVKnots(const std::vector<double>& afKnots)
{
    std::size_t numPoints = static_cast<std::size_t>(_usVCtrlpoints);
    std::size_t order = static_cast<std::size_t>(_usVOrder);
    if (afKnots.size() != (numPoints + order)) {
        return;
    }

    unsigned usVMax = _usVCtrlpoints - _usVOrder + 1;

    // Knot vector for the CAS.CADE class
    // v-direction
    for (unsigned i = 1; i < usVMax; i++) {
        _vVKnots(i) = afKnots[_usVOrder + i - 1];
        _vVMults(i) = 1;
    }

    // Set the B-spline basic functions
    _clVSpline.SetKnots(_vVKnots, _vVMults, _usVOrder);
}

void BSplineParameterCorrection::DoParameterCorrection(int iIter)
{
    int i = 0;
    double fMaxDiff = 0.0, fMaxScalar = 1.0;
    double fWeight = _fSmoothInfluence;

    Base::SequencerLauncher seq("Calc surface...", iIter * _pvcPoints->Length());

    do {
        fMaxScalar = 1.0;
        fMaxDiff = 0.0;

        Handle(Geom_BSplineSurface) pclBSplineSurf = new Geom_BSplineSurface(_vCtrlPntsOfSurf,
                                                                             _vUKnots,
                                                                             _vVKnots,
                                                                             _vUMults,
                                                                             _vVMults,
                                                                             _usUOrder - 1,
                                                                             _usVOrder - 1);

        for (int ii = _pvcPoints->Lower(); ii <= _pvcPoints->Upper(); ii++) {
            double fDeltaU, fDeltaV, fU, fV;
            const gp_Pnt& pnt = (*_pvcPoints)(ii);
            gp_Vec P(pnt.X(), pnt.Y(), pnt.Z());
            gp_Pnt PntX;
            gp_Vec Xu, Xv, Xuv, Xuu, Xvv;
            // Calculate the first two derivatives and point at (u,v)
            gp_Pnt2d& uvValue = (*_pvcUVParam)(ii);
            pclBSplineSurf->D2(uvValue.X(), uvValue.Y(), PntX, Xu, Xv, Xuu, Xvv, Xuv);
            gp_Vec X(PntX.X(), PntX.Y(), PntX.Z());
            gp_Vec ErrorVec = X - P;

            // Calculate Xu x Xv the normal in X(u,v)
            gp_Dir clNormal = Xu ^ Xv;

            // Check, if X = P
            if (!(X.IsEqual(P, 0.001, 0.001))) {
                ErrorVec.Normalize();
                if (fabs(clNormal * ErrorVec) < fMaxScalar) {
                    fMaxScalar = fabs(clNormal * ErrorVec);
                }
            }

            fDeltaU = ((P - X) * Xu) / ((P - X) * Xuu - Xu * Xu);
            if (fabs(fDeltaU) < Precision::Confusion()) {
                fDeltaU = 0.0;
            }
            fDeltaV = ((P - X) * Xv) / ((P - X) * Xvv - Xv * Xv);
            if (fabs(fDeltaV) < Precision::Confusion()) {
                fDeltaV = 0.0;
            }

            // Replace old u/v values with new ones
            fU = uvValue.X() - fDeltaU;
            fV = uvValue.Y() - fDeltaV;
            if (fU <= 1.0 && fU >= 0.0 && fV <= 1.0 && fV >= 0.0) {
                uvValue.SetX(fU);
                uvValue.SetY(fV);
                fMaxDiff = std::max<double>(fabs(fDeltaU), fMaxDiff);
                fMaxDiff = std::max<double>(fabs(fDeltaV), fMaxDiff);
            }

            seq.next();
        }

        if (_bSmoothing) {
            fWeight *= 0.5f;
            SolveWithSmoothing(fWeight);
        }
        else {
            SolveWithoutSmoothing();
        }

        i++;
    } while (i < iIter && fMaxDiff > Precision::Confusion() && fMaxScalar < 0.99);
}

bool BSplineParameterCorrection::SolveWithoutSmoothing()
{
    unsigned ulSize = _pvcPoints->Length();
    unsigned ulDim = _usUCtrlpoints * _usVCtrlpoints;
    math_Matrix M(0, ulSize - 1, 0, ulDim - 1);
    math_Matrix Xx(0, ulDim - 1, 0, 0);
    math_Matrix Xy(0, ulDim - 1, 0, 0);
    math_Matrix Xz(0, ulDim - 1, 0, 0);
    math_Vector bx(0, ulSize - 1);
    math_Vector by(0, ulSize - 1);
    math_Vector bz(0, ulSize - 1);

    // Determining the coefficient matrix of the overdetermined LGS
    for (unsigned i = 0; i < ulSize; i++) {
        const gp_Pnt2d& uvValue = (*_pvcUVParam)(i);
        double fU = uvValue.X();
        double fV = uvValue.Y();
        unsigned ulIdx = 0;

        // Pre-calculation of the values of the base functions
        std::vector<double> basisU(_usUCtrlpoints);
        for (unsigned j = 0; j < _usUCtrlpoints; j++) {
            basisU[j] = _clUSpline.BasisFunction(j, fU);
        }
        std::vector<double> basisV(_usVCtrlpoints);
        for (unsigned k = 0; k < _usVCtrlpoints; k++) {
            basisV[k] = _clVSpline.BasisFunction(k, fV);
        }

        for (unsigned j = 0; j < _usUCtrlpoints; j++) {
            double valueU = basisU[j];
            if (valueU == 0.0) {
                for (unsigned k = 0; k < _usVCtrlpoints; k++) {
                    M(i, ulIdx) = 0.0;
                    ulIdx++;
                }
            }
            else {
                for (unsigned k = 0; k < _usVCtrlpoints; k++) {
                    M(i, ulIdx) = valueU * basisV[k];
                    ulIdx++;
                }
            }
        }
    }

    // Determine the right side
    for (int ii = _pvcPoints->Lower(); ii <= _pvcPoints->Upper(); ii++) {
        const gp_Pnt& pnt = (*_pvcPoints)(ii);
        bx(ii) = pnt.X();
        by(ii) = pnt.Y();
        bz(ii) = pnt.Z();
    }

    // Solve the over-determined LGS with Householder-Transformation
    math_Householder hhX(M, bx);
    math_Householder hhY(M, by);
    math_Householder hhZ(M, bz);

    if (!(hhX.IsDone() && hhY.IsDone() && hhZ.IsDone())) {
        // LGS could not be solved
        return false;
    }
    Xx = hhX.AllValues();
    Xy = hhY.AllValues();
    Xz = hhZ.AllValues();

    unsigned ulIdx = 0;
    for (unsigned j = 0; j < _usUCtrlpoints; j++) {
        for (unsigned k = 0; k < _usVCtrlpoints; k++) {
            _vCtrlPntsOfSurf(j, k) = gp_Pnt(Xx(ulIdx, 0), Xy(ulIdx, 0), Xz(ulIdx, 0));
            ulIdx++;
        }
    }

    return true;
}

namespace Reen
{
class ScalarProduct
{
public:
    explicit ScalarProduct(const math_Matrix& mat)
        : mat(mat)
    {}
    std::vector<double> multiply(int col) const
    {
        math_Vector vec = mat.Col(col);
        std::vector<double> out(mat.ColNumber());
        for (int n = mat.LowerCol(); n <= mat.UpperCol(); n++) {
            out[n] = vec * mat.Col(n);
        }
        return out;
    }

private:
    const math_Matrix& mat;
};
}  // namespace Reen

bool BSplineParameterCorrection::SolveWithSmoothing(double fWeight)
{
    unsigned ulSize = _pvcPoints->Length();
    unsigned ulDim = _usUCtrlpoints * _usVCtrlpoints;
    math_Matrix M(0, ulSize - 1, 0, ulDim - 1);
    math_Vector Xx(0, ulDim - 1);
    math_Vector Xy(0, ulDim - 1);
    math_Vector Xz(0, ulDim - 1);
    math_Vector bx(0, ulSize - 1);
    math_Vector by(0, ulSize - 1);
    math_Vector bz(0, ulSize - 1);
    math_Vector Mbx(0, ulDim - 1);
    math_Vector Mby(0, ulDim - 1);
    math_Vector Mbz(0, ulDim - 1);

    // Determining the coefficient matrix of the overdetermined LGS
    for (unsigned i = 0; i < ulSize; i++) {
        const gp_Pnt2d& uvValue = (*_pvcUVParam)(i);
        double fU = uvValue.X();
        double fV = uvValue.Y();
        int ulIdx = 0;

        // Pre-calculation of the values of the basis functions
        std::vector<double> basisU(_usUCtrlpoints);
        for (unsigned j = 0; j < _usUCtrlpoints; j++) {
            basisU[j] = _clUSpline.BasisFunction(j, fU);
        }
        std::vector<double> basisV(_usVCtrlpoints);
        for (unsigned k = 0; k < _usVCtrlpoints; k++) {
            basisV[k] = _clVSpline.BasisFunction(k, fV);
        }

        for (unsigned j = 0; j < _usUCtrlpoints; j++) {
            double valueU = basisU[j];
            if (valueU == 0.0) {
                for (unsigned k = 0; k < _usVCtrlpoints; k++) {
                    M(i, ulIdx) = 0.0;
                    ulIdx++;
                }
            }
            else {
                for (unsigned k = 0; k < _usVCtrlpoints; k++) {
                    M(i, ulIdx) = valueU * basisV[k];
                    ulIdx++;
                }
            }
        }
    }

    // The product of its transform and itself results in the quadratic
    // system matrix (slowly)
#if 0
    math_Matrix MTM = M.TMultiply(M);
#elif 0
    math_Matrix MTM(0, ulDim - 1, 0, ulDim - 1);
    for (unsigned m = 0; m < ulDim; m++) {
        math_Vector Mm = M.Col(m);
        for (unsigned n = m; n < ulDim; n++) {
            MTM(m, n) = MTM(n, m) = Mm * M.Col(n);
        }
    }
#else  // multi-threaded
    std::vector<int> columns(ulDim);
    std::generate(columns.begin(), columns.end(), Base::iotaGen<int>(0));
    ScalarProduct scalar(M);
    // NOLINTBEGIN
    QFuture<std::vector<double>> future =
        QtConcurrent::mapped(columns, std::bind(&ScalarProduct::multiply, &scalar, sp::_1));
    // NOLINTEND
    QFutureWatcher<std::vector<double>> watcher;
    watcher.setFuture(future);
    watcher.waitForFinished();

    math_Matrix MTM(0, ulDim - 1, 0, ulDim - 1);
    int rowIndex = 0;
    for (const auto& it : future) {
        int colIndex = 0;
        for (std::vector<double>::const_iterator jt = it.begin(); jt != it.end();
             ++jt, colIndex++) {
            MTM(rowIndex, colIndex) = *jt;
        }
        rowIndex++;
    }
#endif

    // Determine the right side
    for (int ii = _pvcPoints->Lower(); ii <= _pvcPoints->Upper(); ii++) {
        const gp_Pnt& pnt = (*_pvcPoints)(ii);
        bx(ii) = pnt.X();
        by(ii) = pnt.Y();
        bz(ii) = pnt.Z();
    }
    for (unsigned i = 0; i < ulDim; i++) {
        math_Vector Mi = M.Col(i);
        Mbx(i) = Mi * bx;
        Mby(i) = Mi * by;
        Mbz(i) = Mi * bz;
    }

    // Solve the LGS with the LU decomposition
    math_Gauss mgGaussX(MTM + fWeight * _clSmoothMatrix);
    math_Gauss mgGaussY(MTM + fWeight * _clSmoothMatrix);
    math_Gauss mgGaussZ(MTM + fWeight * _clSmoothMatrix);

    mgGaussX.Solve(Mbx, Xx);
    if (!mgGaussX.IsDone()) {
        return false;
    }

    mgGaussY.Solve(Mby, Xy);
    if (!mgGaussY.IsDone()) {
        return false;
    }

    mgGaussZ.Solve(Mbz, Xz);
    if (!mgGaussZ.IsDone()) {
        return false;
    }

    unsigned ulIdx = 0;
    for (unsigned j = 0; j < _usUCtrlpoints; j++) {
        for (unsigned k = 0; k < _usVCtrlpoints; k++) {
            _vCtrlPntsOfSurf(j, k) = gp_Pnt(Xx(ulIdx), Xy(ulIdx), Xz(ulIdx));
            ulIdx++;
        }
    }

    return true;
}

void BSplineParameterCorrection::CalcSmoothingTerms(bool bRecalc,
                                                    double fFirst,
                                                    double fSecond,
                                                    double fThird)
{
    if (bRecalc) {
        Base::SequencerLauncher seq("Initializing...",
                                    3 * _usUCtrlpoints * _usUCtrlpoints * _usVCtrlpoints
                                        * _usVCtrlpoints);
        CalcFirstSmoothMatrix(seq);
        CalcSecondSmoothMatrix(seq);
        CalcThirdSmoothMatrix(seq);
    }

    _clSmoothMatrix = fFirst * _clFirstMatrix + fSecond * _clSecondMatrix + fThird * _clThirdMatrix;
}

void BSplineParameterCorrection::CalcFirstSmoothMatrix(Base::SequencerLauncher& seq)
{
    unsigned m = 0;
    for (unsigned k = 0; k < _usUCtrlpoints; k++) {
        for (unsigned l = 0; l < _usVCtrlpoints; l++) {
            unsigned n = 0;

            for (unsigned i = 0; i < _usUCtrlpoints; i++) {
                for (unsigned j = 0; j < _usVCtrlpoints; j++) {
                    _clFirstMatrix(m, n) = _clUSpline.GetIntegralOfProductOfBSplines(i, k, 1, 1)
                            * _clVSpline.GetIntegralOfProductOfBSplines(j, l, 0, 0)
                        + _clUSpline.GetIntegralOfProductOfBSplines(i, k, 0, 0)
                            * _clVSpline.GetIntegralOfProductOfBSplines(j, l, 1, 1);
                    seq.next();
                    n++;
                }
            }
            m++;
        }
    }
}

void BSplineParameterCorrection::CalcSecondSmoothMatrix(Base::SequencerLauncher& seq)
{
    unsigned m = 0;
    for (unsigned k = 0; k < _usUCtrlpoints; k++) {
        for (unsigned l = 0; l < _usVCtrlpoints; l++) {
            unsigned n = 0;

            for (unsigned i = 0; i < _usUCtrlpoints; i++) {
                for (unsigned j = 0; j < _usVCtrlpoints; j++) {
                    _clSecondMatrix(m, n) = _clUSpline.GetIntegralOfProductOfBSplines(i, k, 2, 2)
                            * _clVSpline.GetIntegralOfProductOfBSplines(j, l, 0, 0)
                        + 2 * _clUSpline.GetIntegralOfProductOfBSplines(i, k, 1, 1)
                            * _clVSpline.GetIntegralOfProductOfBSplines(j, l, 1, 1)
                        + _clUSpline.GetIntegralOfProductOfBSplines(i, k, 0, 0)
                            * _clVSpline.GetIntegralOfProductOfBSplines(j, l, 2, 2);
                    seq.next();
                    n++;
                }
            }
            m++;
        }
    }
}

void BSplineParameterCorrection::CalcThirdSmoothMatrix(Base::SequencerLauncher& seq)
{
    unsigned m = 0;
    for (unsigned k = 0; k < _usUCtrlpoints; k++) {
        for (unsigned l = 0; l < _usVCtrlpoints; l++) {
            unsigned n = 0;

            for (unsigned i = 0; i < _usUCtrlpoints; i++) {
                for (unsigned j = 0; j < _usVCtrlpoints; j++) {
                    _clThirdMatrix(m, n) = _clUSpline.GetIntegralOfProductOfBSplines(i, k, 3, 3)
                            * _clVSpline.GetIntegralOfProductOfBSplines(j, l, 0, 0)
                        + _clUSpline.GetIntegralOfProductOfBSplines(i, k, 3, 1)
                            * _clVSpline.GetIntegralOfProductOfBSplines(j, l, 0, 2)
                        + _clUSpline.GetIntegralOfProductOfBSplines(i, k, 1, 3)
                            * _clVSpline.GetIntegralOfProductOfBSplines(j, l, 2, 0)
                        + _clUSpline.GetIntegralOfProductOfBSplines(i, k, 1, 1)
                            * _clVSpline.GetIntegralOfProductOfBSplines(j, l, 2, 2)
                        + _clUSpline.GetIntegralOfProductOfBSplines(i, k, 2, 2)
                            * _clVSpline.GetIntegralOfProductOfBSplines(j, l, 1, 1)
                        + _clUSpline.GetIntegralOfProductOfBSplines(i, k, 0, 2)
                            * _clVSpline.GetIntegralOfProductOfBSplines(j, l, 3, 1)
                        + _clUSpline.GetIntegralOfProductOfBSplines(i, k, 2, 0)
                            * _clVSpline.GetIntegralOfProductOfBSplines(j, l, 1, 3)
                        + _clUSpline.GetIntegralOfProductOfBSplines(i, k, 0, 0)
                            * _clVSpline.GetIntegralOfProductOfBSplines(j, l, 3, 3);
                    seq.next();
                    n++;
                }
            }
            m++;
        }
    }
}

void BSplineParameterCorrection::EnableSmoothing(bool bSmooth, double fSmoothInfl)
{
    EnableSmoothing(bSmooth, fSmoothInfl, 1.0, 0.0, 0.0);
}

void BSplineParameterCorrection::EnableSmoothing(bool bSmooth,
                                                 double fSmoothInfl,
                                                 double fFirst,
                                                 double fSec,
                                                 double fThird)
{
    if (_bSmoothing && bSmooth) {
        CalcSmoothingTerms(false, fFirst, fSec, fThird);
    }
    else if (bSmooth) {
        CalcSmoothingTerms(true, fFirst, fSec, fThird);
    }

    ParameterCorrection::EnableSmoothing(bSmooth, fSmoothInfl);
}

const math_Matrix& BSplineParameterCorrection::GetFirstSmoothMatrix() const
{
    return _clFirstMatrix;
}

const math_Matrix& BSplineParameterCorrection::GetSecondSmoothMatrix() const
{
    return _clSecondMatrix;
}

const math_Matrix& BSplineParameterCorrection::GetThirdSmoothMatrix() const
{
    return _clThirdMatrix;
}

void BSplineParameterCorrection::SetFirstSmoothMatrix(const math_Matrix& rclMat)
{
    _clFirstMatrix = rclMat;
}

void BSplineParameterCorrection::SetSecondSmoothMatrix(const math_Matrix& rclMat)
{
    _clSecondMatrix = rclMat;
}

void BSplineParameterCorrection::SetThirdSmoothMatrix(const math_Matrix& rclMat)
{
    _clThirdMatrix = rclMat;
}
