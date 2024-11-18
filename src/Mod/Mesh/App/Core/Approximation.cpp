/***************************************************************************
 *   Copyright (c) 2005 Imetric 3D GmbH                                    *
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
#include <algorithm>
#include <cstdlib>
#include <iterator>
#endif

#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Mod/Mesh/App/WildMagic4/Wm4ApprPolyFit3.h>
#include <Mod/Mesh/App/WildMagic4/Wm4ApprQuadraticFit3.h>
#include <Mod/Mesh/App/WildMagic4/Wm4ApprSphereFit3.h>
#include <boost/math/special_functions/fpclassify.hpp>

// #define FC_USE_EIGEN
#include <Eigen/Eigen>
#include <Eigen/QR>
#ifdef FC_USE_EIGEN
#include <Eigen/Eigenvalues>
#endif

#include "Approximation.h"
#include "CylinderFit.h"
#include "Elements.h"
#include "SphereFit.h"
#include "Utilities.h"


using namespace MeshCore;

Approximation::Approximation() = default;

Approximation::~Approximation()
{
    Clear();
}

void Approximation::GetMgcVectorArray(std::vector<Wm4::Vector3<double>>& rcPts) const
{
    std::list<Base::Vector3f>::const_iterator It;
    rcPts.reserve(_vPoints.size());
    for (It = _vPoints.begin(); It != _vPoints.end(); ++It) {
        rcPts.push_back(Base::convertTo<Wm4::Vector3d>(*It));
    }
}

void Approximation::AddPoint(const Base::Vector3f& rcVector)
{
    _vPoints.push_back(rcVector);
    _bIsFitted = false;
}

void Approximation::AddPoints(const std::vector<Base::Vector3f>& points)
{
    std::copy(points.begin(), points.end(), std::back_inserter(_vPoints));
    _bIsFitted = false;
}

void Approximation::AddPoints(const std::set<Base::Vector3f>& points)
{
    std::copy(points.begin(), points.end(), std::back_inserter(_vPoints));
    _bIsFitted = false;
}

void Approximation::AddPoints(const std::list<Base::Vector3f>& points)
{
    std::copy(points.begin(), points.end(), std::back_inserter(_vPoints));
    _bIsFitted = false;
}

void Approximation::AddPoints(const MeshPointArray& points)
{
    std::copy(points.begin(), points.end(), std::back_inserter(_vPoints));
    _bIsFitted = false;
}

Base::Vector3f Approximation::GetGravity() const
{
    Base::Vector3f clGravity;
    if (!_vPoints.empty()) {
        for (const auto& vPoint : _vPoints) {
            clGravity += vPoint;
        }
        clGravity *= 1.0f / float(_vPoints.size());
    }
    return clGravity;
}

std::size_t Approximation::CountPoints() const
{
    return _vPoints.size();
}

void Approximation::Clear()
{
    _vPoints.clear();
    _bIsFitted = false;
}

float Approximation::GetLastResult() const
{
    return _fLastResult;
}

bool Approximation::Done() const
{
    return _bIsFitted;
}

// -------------------------------------------------------------------------------

PlaneFit::PlaneFit()
    : _vBase(0, 0, 0)
    , _vDirU(1, 0, 0)
    , _vDirV(0, 1, 0)
    , _vDirW(0, 0, 1)
{}

float PlaneFit::Fit()
{
    _bIsFitted = true;
    if (CountPoints() < 3) {
        return FLOAT_MAX;
    }

    double sxx {0.0};
    double sxy {0.0};
    double sxz {0.0};
    double syy {0.0};
    double syz {0.0};
    double szz {0.0};
    double mx {0.0};
    double my {0.0};
    double mz {0.0};

    for (const auto& vPoint : _vPoints) {
        sxx += double(vPoint.x * vPoint.x);
        sxy += double(vPoint.x * vPoint.y);
        sxz += double(vPoint.x * vPoint.z);
        syy += double(vPoint.y * vPoint.y);
        syz += double(vPoint.y * vPoint.z);
        szz += double(vPoint.z * vPoint.z);
        mx += double(vPoint.x);
        my += double(vPoint.y);
        mz += double(vPoint.z);
    }

    size_t nSize = _vPoints.size();
    sxx = sxx - mx * mx / (double(nSize));
    sxy = sxy - mx * my / (double(nSize));
    sxz = sxz - mx * mz / (double(nSize));
    syy = syy - my * my / (double(nSize));
    syz = syz - my * mz / (double(nSize));
    szz = szz - mz * mz / (double(nSize));

#if defined(FC_USE_EIGEN)
    Eigen::Matrix3d covMat = Eigen::Matrix3d::Zero();
    covMat(0, 0) = sxx;
    covMat(1, 1) = syy;
    covMat(2, 2) = szz;
    covMat(0, 1) = sxy;
    covMat(1, 0) = sxy;
    covMat(0, 2) = sxz;
    covMat(2, 0) = sxz;
    covMat(1, 2) = syz;
    covMat(2, 1) = syz;
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eig(covMat);

    Eigen::Vector3d u = eig.eigenvectors().col(1);
    Eigen::Vector3d v = eig.eigenvectors().col(2);
    Eigen::Vector3d w = eig.eigenvectors().col(0);

    _vDirU.Set(u.x(), u.y(), u.z());
    _vDirV.Set(v.x(), v.y(), v.z());
    _vDirW.Set(w.x(), w.y(), w.z());
    _vBase.Set(mx / (float)nSize, my / (float)nSize, mz / (float)nSize);

    float sigma = w.dot(covMat * w);
#else
    // Covariance matrix
    Wm4::Matrix3<double> akMat(sxx, sxy, sxz, sxy, syy, syz, sxz, syz, szz);
    Wm4::Matrix3<double> rkRot, rkDiag;
    try {
        akMat.EigenDecomposition(rkRot, rkDiag);
    }
    catch (const std::exception&) {
        return FLOAT_MAX;
    }

    // We know the Eigenvalues are ordered
    // rkDiag(0,0) <= rkDiag(1,1) <= rkDiag(2,2)
    //
    // points describe a line or even are identical
    if (rkDiag(1, 1) <= 0) {
        return FLOAT_MAX;
    }

    Wm4::Vector3<double> U = rkRot.GetColumn(1);
    Wm4::Vector3<double> V = rkRot.GetColumn(2);
    Wm4::Vector3<double> W = rkRot.GetColumn(0);

    // It may happen that the result have nan values
    for (int i = 0; i < 3; i++) {
        if (boost::math::isnan(W[i])) {
            return FLOAT_MAX;
        }
    }

    // In some cases when the points exactly lie on a plane it can happen that
    // U or V have nan values but W is valid.
    // In this case create an orthonormal basis
    bool validUV = true;
    for (int i = 0; i < 3; i++) {
        if (boost::math::isnan(U[i]) || boost::math::isnan(V[i])) {
            validUV = false;
            break;
        }
    }

    if (!validUV) {
        Wm4::Vector3<double>::GenerateOrthonormalBasis(U, V, W);
    }

    _vDirU.Set(float(U.X()), float(U.Y()), float(U.Z()));
    _vDirV.Set(float(V.X()), float(V.Y()), float(V.Z()));
    _vDirW.Set(float(W.X()), float(W.Y()), float(W.Z()));
    _vBase.Set(float(mx / nSize), float(my / nSize), float(mz / nSize));
    float sigma = float(W.Dot(akMat * W));
#endif

    // In case sigma is nan
    if (boost::math::isnan(sigma)) {
        return FLOAT_MAX;
    }

    // This must be caused by some round-off errors. Theoretically it's impossible
    // that 'sigma' becomes negative because the covariance matrix is positive semi-definite.
    if (sigma < 0) {
        sigma = 0;
    }

    // make a right-handed system
    if ((_vDirU % _vDirV) * _vDirW < 0.0f) {
        Base::Vector3f tmp = _vDirU;
        _vDirU = _vDirV;
        _vDirV = tmp;
    }

    if (nSize > 3) {
        sigma = sqrt(sigma / (nSize - 3));
    }
    else {
        sigma = 0;
    }

    _fLastResult = sigma;
    return _fLastResult;
}

Base::Vector3f PlaneFit::GetBase() const
{
    if (_bIsFitted) {
        return _vBase;
    }
    else {
        return Base::Vector3f();
    }
}

Base::Vector3f PlaneFit::GetDirU() const
{
    if (_bIsFitted) {
        return _vDirU;
    }
    else {
        return Base::Vector3f();
    }
}

Base::Vector3f PlaneFit::GetDirV() const
{
    if (_bIsFitted) {
        return _vDirV;
    }
    else {
        return Base::Vector3f();
    }
}

Base::Vector3f PlaneFit::GetNormal() const
{
    if (_bIsFitted) {
        return _vDirW;
    }
    else {
        return Base::Vector3f();
    }
}

float PlaneFit::GetDistanceToPlane(const Base::Vector3f& rcPoint) const
{
    float fResult = FLOAT_MAX;
    if (_bIsFitted) {
        fResult = (rcPoint - _vBase) * _vDirW;
    }
    return fResult;
}

float PlaneFit::GetStdDeviation() const
{
    // Mean: M=(1/N)*SUM Xi
    // Variance: VAR=(N/N-1)*[(1/N)*SUM(Xi^2)-M^2]
    // Standard deviation: SD=SQRT(VAR)
    // Standard error of the mean: SE=SD/SQRT(N)
    if (!_bIsFitted) {
        return FLOAT_MAX;
    }

    float fSumXi = 0.0f, fSumXi2 = 0.0f, fMean = 0.0f, fDist = 0.0f;

    float ulPtCt = float(CountPoints());
    std::list<Base::Vector3f>::const_iterator cIt;

    for (cIt = _vPoints.begin(); cIt != _vPoints.end(); ++cIt) {
        fDist = GetDistanceToPlane(*cIt);
        fSumXi += fDist;
        fSumXi2 += (fDist * fDist);
    }

    fMean = (1.0f / ulPtCt) * fSumXi;
    return sqrt((ulPtCt / (ulPtCt - 1.0f)) * ((1.0f / ulPtCt) * fSumXi2 - fMean * fMean));
}

float PlaneFit::GetSignedStdDeviation() const
{
    // if the nearest point to the gravity is at the side
    // of normal direction the value will be
    // positive otherwise negative
    if (!_bIsFitted) {
        return FLOAT_MAX;
    }

    float fSumXi = 0.0F, fSumXi2 = 0.0F, fMean = 0.0F, fDist = 0.0F;
    float fMinDist = FLOAT_MAX;
    float fFactor = 0.0F;

    float ulPtCt = float(CountPoints());
    Base::Vector3f clGravity, clPt;
    std::list<Base::Vector3f>::const_iterator cIt;
    for (cIt = _vPoints.begin(); cIt != _vPoints.end(); ++cIt) {
        clGravity += *cIt;
    }
    clGravity *= (1.0f / ulPtCt);

    for (cIt = _vPoints.begin(); cIt != _vPoints.end(); ++cIt) {
        if ((clGravity - *cIt).Length() < fMinDist) {
            fMinDist = (clGravity - *cIt).Length();
            clPt = *cIt;
        }
        fDist = GetDistanceToPlane(*cIt);
        fSumXi += fDist;
        fSumXi2 += (fDist * fDist);
    }

    // which side
    if ((clPt - clGravity) * GetNormal() > 0) {
        fFactor = 1.0f;
    }
    else {
        fFactor = -1.0f;
    }

    fMean = 1.0f / ulPtCt * fSumXi;

    return fFactor * sqrt((ulPtCt / (ulPtCt - 3.0f)) * ((1.0f / ulPtCt) * fSumXi2 - fMean * fMean));
}

void PlaneFit::ProjectToPlane()
{
    Base::Vector3f cGravity(GetGravity());
    Base::Vector3f cNormal(GetNormal());

    for (auto& cPnt : _vPoints) {
        float fD = (cPnt - cGravity) * cNormal;
        cPnt = cPnt - fD * cNormal;
    }
}

void PlaneFit::Dimension(float& length, float& width) const
{
    const Base::Vector3f& bs = _vBase;
    const Base::Vector3f& ex = _vDirU;
    const Base::Vector3f& ey = _vDirV;

    Base::BoundBox3f bbox;
    std::list<Base::Vector3f>::const_iterator cIt;
    for (cIt = _vPoints.begin(); cIt != _vPoints.end(); ++cIt) {
        Base::Vector3f pnt = *cIt;
        pnt.TransformToCoordinateSystem(bs, ex, ey);
        bbox.Add(pnt);
    }

    length = bbox.MaxX - bbox.MinX;
    width = bbox.MaxY - bbox.MinY;
}

std::vector<Base::Vector3f> PlaneFit::GetLocalPoints() const
{
    std::vector<Base::Vector3f> localPoints;
    if (_bIsFitted && _fLastResult < FLOAT_MAX) {
        Base::Vector3d bs = Base::convertTo<Base::Vector3d>(this->_vBase);
        Base::Vector3d ex = Base::convertTo<Base::Vector3d>(this->_vDirU);
        Base::Vector3d ey = Base::convertTo<Base::Vector3d>(this->_vDirV);
        // Base::Vector3d ez = Base::convertTo<Base::Vector3d>(this->_vDirW);

        localPoints.insert(localPoints.begin(), _vPoints.begin(), _vPoints.end());
        for (auto& localPoint : localPoints) {
            Base::Vector3d clPoint = Base::convertTo<Base::Vector3d>(localPoint);
            clPoint.TransformToCoordinateSystem(bs, ex, ey);
            localPoint.Set(static_cast<float>(clPoint.x),
                           static_cast<float>(clPoint.y),
                           static_cast<float>(clPoint.z));
        }
    }

    return localPoints;
}

Base::BoundBox3f PlaneFit::GetBoundings() const
{
    Base::BoundBox3f bbox;
    std::vector<Base::Vector3f> pts = GetLocalPoints();
    for (const auto& it : pts) {
        bbox.Add(it);
    }
    return bbox;
}

// -------------------------------------------------------------------------------

bool QuadraticFit::GetCurvatureInfo(double x,
                                    double y,
                                    double z,
                                    double& rfCurv0,
                                    double& rfCurv1,
                                    Base::Vector3f& rkDir0,
                                    Base::Vector3f& rkDir1,
                                    double& dDistance)
{
    assert(_bIsFitted);
    bool bResult = false;

    if (_bIsFitted) {
        Wm4::Vector3<double> Dir0, Dir1;
        FunctionContainer clFuncCont(_fCoeff);
        bResult = clFuncCont.CurvatureInfo(x, y, z, rfCurv0, rfCurv1, Dir0, Dir1, dDistance);

        dDistance = double(clFuncCont.GetGradient(x, y, z).Length());
        rkDir0 = Base::convertTo<Base::Vector3f>(Dir0);
        rkDir1 = Base::convertTo<Base::Vector3f>(Dir1);
    }

    return bResult;
}

bool QuadraticFit::GetCurvatureInfo(double x, double y, double z, double& rfCurv0, double& rfCurv1)
{
    bool bResult = false;

    if (_bIsFitted) {
        FunctionContainer clFuncCont(_fCoeff);
        bResult = clFuncCont.CurvatureInfo(x, y, z, rfCurv0, rfCurv1);
    }

    return bResult;
}

const double& QuadraticFit::GetCoeffArray() const
{
    return _fCoeff[0];
}

double QuadraticFit::GetCoeff(std::size_t ulIndex) const
{
    assert(ulIndex < 10);

    if (_bIsFitted) {
        return _fCoeff[ulIndex];
    }
    else {
        return double(FLOAT_MAX);
    }
}

float QuadraticFit::Fit()
{
    float fResult = FLOAT_MAX;

    if (CountPoints() > 0) {
        std::vector<Wm4::Vector3<double>> cPts;
        GetMgcVectorArray(cPts);
        fResult = (float)Wm4::QuadraticFit3<double>(CountPoints(), &(cPts[0]), _fCoeff);
        _fLastResult = fResult;

        _bIsFitted = true;
    }

    return fResult;
}

void QuadraticFit::CalcEigenValues(double& dLambda1,
                                   double& dLambda2,
                                   double& dLambda3,
                                   Base::Vector3f& clEV1,
                                   Base::Vector3f& clEV2,
                                   Base::Vector3f& clEV3) const
{
    assert(_bIsFitted);

    /*
     * F(x,y,z) = a11*x*x + a22*y*y + a33*z*z +2*a12*x*y + 2*a13*x*z + 2*a23*y*z + 2*a10*x + 2*a20*y
     * + 2*a30*z * a00 = 0
     *
     * Form matrix:
     *
     *      ( a11   a12   a13 )
     * A =  ( a21   a22   a23 )       where a[i,j] = a[j,i]
     *      ( a31   a32   a33 )
     *
     * Coefficients of the Quadrik-Fit based on the notation used here:
     *
     *   0 = C[0] + C[1]*X + C[2]*Y + C[3]*Z + C[4]*X^2 + C[5]*Y^2
     *     + C[6]*Z^2 + C[7]*X*Y + C[8]*X*Z + C[9]*Y*Z
     *
     * Square:        a11 := c[4],    a22 := c[5],    a33 := c[6]
     * Mixed:         a12 := c[7]/2,  a13 := c[8]/2,  a23 := c[9]/2
     * Linear:        a10 := c[1]/2,  a20 := c[2]/2,  a30 := c[3]/2
     * Constant:      a00 := c[0]
     *
     */

    Wm4::Matrix3<double> akMat(_fCoeff[4],
                               _fCoeff[7] / 2.0,
                               _fCoeff[8] / 2.0,
                               _fCoeff[7] / 2.0,
                               _fCoeff[5],
                               _fCoeff[9] / 2.0,
                               _fCoeff[8] / 2.0,
                               _fCoeff[9] / 2.0,
                               _fCoeff[6]);

    Wm4::Matrix3<double> rkRot, rkDiag;
    akMat.EigenDecomposition(rkRot, rkDiag);

    Wm4::Vector3<double> vEigenU = rkRot.GetColumn(0);
    Wm4::Vector3<double> vEigenV = rkRot.GetColumn(1);
    Wm4::Vector3<double> vEigenW = rkRot.GetColumn(2);

    clEV1 = Base::convertTo<Base::Vector3f>(vEigenU);
    clEV2 = Base::convertTo<Base::Vector3f>(vEigenV);
    clEV3 = Base::convertTo<Base::Vector3f>(vEigenW);

    dLambda1 = rkDiag[0][0];
    dLambda2 = rkDiag[1][1];
    dLambda3 = rkDiag[2][2];
}

void QuadraticFit::CalcZValues(double x, double y, double& dZ1, double& dZ2) const
{
    assert(_bIsFitted);

    double dDisk = _fCoeff[3] * _fCoeff[3] + 2 * _fCoeff[3] * _fCoeff[8] * x
        + 2 * _fCoeff[3] * _fCoeff[9] * y + _fCoeff[8] * _fCoeff[8] * x * x
        + 2 * _fCoeff[8] * x * _fCoeff[9] * y + _fCoeff[9] * _fCoeff[9] * y * y
        - 4 * _fCoeff[6] * _fCoeff[0] - 4 * _fCoeff[6] * _fCoeff[1] * x
        - 4 * _fCoeff[6] * _fCoeff[2] * y - 4 * _fCoeff[6] * _fCoeff[7] * x * y
        - 4 * _fCoeff[6] * _fCoeff[4] * x * x - 4 * _fCoeff[6] * _fCoeff[5] * y * y;

    if (fabs(_fCoeff[6]) < 0.000005) {
        dZ1 = double(FLOAT_MAX);
        dZ2 = double(FLOAT_MAX);
        return;
    }

    if (dDisk < 0.0) {
        dZ1 = double(FLOAT_MAX);
        dZ2 = double(FLOAT_MAX);
        return;
    }
    else {
        dDisk = sqrt(dDisk);
    }

    dZ1 = 0.5 * ((-_fCoeff[3] - _fCoeff[8] * x - _fCoeff[9] * y + dDisk) / _fCoeff[6]);
    dZ2 = 0.5 * ((-_fCoeff[3] - _fCoeff[8] * x - _fCoeff[9] * y - dDisk) / _fCoeff[6]);
}

// -------------------------------------------------------------------------------

SurfaceFit::SurfaceFit()
    : _fCoeff {}
{}

float SurfaceFit::Fit()
{
    float fResult = FLOAT_MAX;

    if (CountPoints() > 0) {
        fResult = float(PolynomFit());
        _fLastResult = fResult;

        _bIsFitted = true;
    }

    return fResult;
}

bool SurfaceFit::GetCurvatureInfo(double x,
                                  double y,
                                  double z,
                                  double& rfCurv0,
                                  double& rfCurv1,
                                  Base::Vector3f& rkDir0,
                                  Base::Vector3f& rkDir1,
                                  double& dDistance)
{
    bool bResult = false;

    if (_bIsFitted) {
        Wm4::Vector3<double> Dir0, Dir1;
        FunctionContainer clFuncCont(_fCoeff);
        bResult = clFuncCont.CurvatureInfo(x, y, z, rfCurv0, rfCurv1, Dir0, Dir1, dDistance);

        dDistance = double(clFuncCont.GetGradient(x, y, z).Length());
        rkDir0 = Base::convertTo<Base::Vector3f>(Dir0);
        rkDir1 = Base::convertTo<Base::Vector3f>(Dir1);
    }

    return bResult;
}

bool SurfaceFit::GetCurvatureInfo(double x, double y, double z, double& rfCurv0, double& rfCurv1)
{
    assert(_bIsFitted);
    bool bResult = false;

    if (_bIsFitted) {
        FunctionContainer clFuncCont(_fCoeff);
        bResult = clFuncCont.CurvatureInfo(x, y, z, rfCurv0, rfCurv1);
    }

    return bResult;
}

double SurfaceFit::PolynomFit()
{
    if (PlaneFit::Fit() >= FLOAT_MAX) {
        return double(FLOAT_MAX);
    }

    Base::Vector3d bs = Base::convertTo<Base::Vector3d>(this->_vBase);
    Base::Vector3d ex = Base::convertTo<Base::Vector3d>(this->_vDirU);
    Base::Vector3d ey = Base::convertTo<Base::Vector3d>(this->_vDirV);
    // Base::Vector3d ez = Base::convertTo<Base::Vector3d>(this->_vDirW);

    // A*x = b
    // See also www.cs.jhu.edu/~misha/Fall05/10.23.05.pdf
    // z = f(x,y) = a*x^2 + b*y^2 + c*x*y + d*x + e*y + f
    // z = P * Vi with Vi=(xi^2,yi^2,xiyi,xi,yi,1) and P=(a,b,c,d,e,f)
    // To get the best-fit values the sum needs to be minimized:
    // S = sum[(z-zi)^2} -> min with zi=z coordinates of the given points
    // <=> S = sum[z^2 - 2*z*zi + zi^2] -> min
    // <=> S(P) = sum[(P*Vi)^2 - 2*(P*Vi)*zi + zi^2] -> min
    // To get the optimum the gradient of the expression must be the null vector
    // Note: grad F(P) = (P*Vi)^2 = 2*(P*Vi)*Vi
    //       grad G(P) = -2*(P*Vi)*zi = -2*Vi*zi
    //       grad H(P) = zi^2 = 0
    //  => grad S(P) = sum[2*(P*Vi)*Vi - 2*Vi*zi] = 0
    // <=> sum[(P*Vi)*Vi] = sum[Vi*zi]
    // <=> sum[(Vi*Vi^t)*P] = sum[Vi*zi] where (Vi*Vi^t) is a symmetric matrix
    // <=> sum[(Vi*Vi^t)]*P = sum[Vi*zi]
    Eigen::Matrix<double, 6, 6> A = Eigen::Matrix<double, 6, 6>::Zero();
    Eigen::Matrix<double, 6, 1> b = Eigen::Matrix<double, 6, 1>::Zero();
    Eigen::Matrix<double, 6, 1> x = Eigen::Matrix<double, 6, 1>::Zero();

    std::vector<Base::Vector3d> transform;
    transform.reserve(_vPoints.size());

    double dW2 = 0;
    for (std::list<Base::Vector3f>::const_iterator it = _vPoints.begin(); it != _vPoints.end();
         ++it) {
        Base::Vector3d clPoint = Base::convertTo<Base::Vector3d>(*it);
        clPoint.TransformToCoordinateSystem(bs, ex, ey);
        transform.push_back(clPoint);
        double dU = clPoint.x;
        double dV = clPoint.y;
        double dW = clPoint.z;

        double dU2 = dU * dU;
        double dV2 = dV * dV;
        double dUV = dU * dV;

        dW2 += dW * dW;

        A(0, 0) = A(0, 0) + dU2 * dU2;
        A(0, 1) = A(0, 1) + dU2 * dV2;
        A(0, 2) = A(0, 2) + dU2 * dUV;
        A(0, 3) = A(0, 3) + dU2 * dU;
        A(0, 4) = A(0, 4) + dU2 * dV;
        A(0, 5) = A(0, 5) + dU2;
        b(0) = b(0) + dU2 * dW;

        A(1, 1) = A(1, 1) + dV2 * dV2;
        A(1, 2) = A(1, 2) + dV2 * dUV;
        A(1, 3) = A(1, 3) + dV2 * dU;
        A(1, 4) = A(1, 4) + dV2 * dV;
        A(1, 5) = A(1, 5) + dV2;
        b(1) = b(1) + dV2 * dW;

        A(2, 2) = A(2, 2) + dUV * dUV;
        A(2, 3) = A(2, 3) + dUV * dU;
        A(2, 4) = A(2, 4) + dUV * dV;
        A(2, 5) = A(2, 5) + dUV;
        b(3) = b(3) + dUV * dW;

        A(3, 3) = A(3, 3) + dU * dU;
        A(3, 4) = A(3, 4) + dU * dV;
        A(3, 5) = A(3, 5) + dU;
        b(3) = b(3) + dU * dW;

        A(4, 4) = A(4, 4) + dV * dV;
        A(4, 5) = A(4, 5) + dV;
        b(5) = b(5) + dV * dW;

        A(5, 5) = A(5, 5) + 1;
        b(5) = b(5) + 1 * dW;
    }

    // Mat is symmetric
    //
    A(1, 0) = A(0, 1);
    A(2, 0) = A(0, 2);
    A(3, 0) = A(0, 3);
    A(4, 0) = A(0, 4);
    A(5, 0) = A(0, 5);

    A(2, 1) = A(1, 2);
    A(3, 1) = A(1, 3);
    A(4, 1) = A(1, 4);
    A(5, 1) = A(1, 5);

    A(3, 2) = A(2, 3);
    A(4, 2) = A(2, 4);
    A(5, 2) = A(2, 5);

    A(4, 3) = A(3, 4);
    A(5, 3) = A(3, 5);

    A(5, 4) = A(4, 5);

    Eigen::HouseholderQR<Eigen::Matrix<double, 6, 6>> qr(A);
    x = qr.solve(b);

    // FunctionContainer gets an implicit function F(x,y,z) = 0 of this form
    // _fCoeff[0] +
    // _fCoeff[1]*x   + _fCoeff[2]*y   + _fCoeff[3]*z   +
    // _fCoeff[4]*x^2 + _fCoeff[5]*y^2 + _fCoeff[6]*z^2 +
    // _fCoeff[7]*x*y + _fCoeff[8]*x*z + _fCoeff[9]*y*z
    //
    // The bivariate polynomial surface we get here is of the form
    // z = f(x,y) = a*x^2 + b*y^2 + c*x*y + d*x + e*y + f
    // Writing it as implicit surface F(x,y,z) = 0 gives this form
    // F(x,y,z) = f(x,y) - z = a*x^2 + b*y^2 + c*x*y + d*x + e*y - z + f
    // Thus:
    // _fCoeff[0] = f
    // _fCoeff[1] = d
    // _fCoeff[2] = e
    // _fCoeff[3] = -1
    // _fCoeff[4] = a
    // _fCoeff[5] = b
    // _fCoeff[6] = 0
    // _fCoeff[7] = c
    // _fCoeff[8] = 0
    // _fCoeff[9] = 0

    _fCoeff[0] = x(5);
    _fCoeff[1] = x(3);
    _fCoeff[2] = x(4);
    _fCoeff[3] = -1.0;
    _fCoeff[4] = x(0);
    _fCoeff[5] = x(1);
    _fCoeff[6] = 0.0;
    _fCoeff[7] = x(2);
    _fCoeff[8] = 0.0;
    _fCoeff[9] = 0.0;

    // Get S(P) = sum[(P*Vi)^2 - 2*(P*Vi)*zi + zi^2]
    double sigma = 0;
    FunctionContainer clFuncCont(_fCoeff);
    for (const auto& it : transform) {
        double u = it.x;
        double v = it.y;
        double z = clFuncCont.F(u, v, 0.0);
        sigma += z * z;
    }

    sigma += dW2 - 2 * x.dot(b);
    // This must be caused by some round-off errors. Theoretically it's impossible
    // that 'sigma' becomes negative.
    if (sigma < 0) {
        sigma = 0;
    }
    if (!_vPoints.empty()) {
        sigma = sqrt(sigma / _vPoints.size());
    }

    _fLastResult = static_cast<float>(sigma);
    return double(_fLastResult);
}

double SurfaceFit::Value(double x, double y) const
{
    double z = 0.0;
    if (_bIsFitted) {
        FunctionContainer clFuncCont(_fCoeff);
        z = clFuncCont.F(x, y, 0.0);
    }

    return z;
}

void SurfaceFit::GetCoefficients(double& a, double& b, double& c, double& d, double& e, double& f)
    const
{
    a = _fCoeff[4];
    b = _fCoeff[5];
    c = _fCoeff[7];
    d = _fCoeff[1];
    e = _fCoeff[2];
    f = _fCoeff[0];
}

void SurfaceFit::Transform(std::vector<Base::Vector3f>& pts) const
{
    Base::Vector3d bs = Base::convertTo<Base::Vector3d>(this->_vBase);
    Base::Vector3d ex = Base::convertTo<Base::Vector3d>(this->_vDirU);
    Base::Vector3d ey = Base::convertTo<Base::Vector3d>(this->_vDirV);
    Base::Vector3d ez = Base::convertTo<Base::Vector3d>(this->_vDirW);

    Base::Matrix4D mat;
    mat[0][0] = ex.x;
    mat[0][1] = ey.x;
    mat[0][2] = ez.x;
    mat[0][3] = bs.x;

    mat[1][0] = ex.y;
    mat[1][1] = ey.y;
    mat[1][2] = ez.y;
    mat[1][3] = bs.y;

    mat[2][0] = ex.z;
    mat[2][1] = ey.z;
    mat[2][2] = ez.z;
    mat[2][3] = bs.z;

    std::transform(pts.begin(), pts.end(), pts.begin(), [&mat](const Base::Vector3f& pt) {
        Base::Vector3f v(pt);
        mat.multVec(v, v);
        return v;
    });
}

void SurfaceFit::Transform(std::vector<Base::Vector3d>& pts) const
{
    Base::Vector3d bs = Base::convertTo<Base::Vector3d>(this->_vBase);
    Base::Vector3d ex = Base::convertTo<Base::Vector3d>(this->_vDirU);
    Base::Vector3d ey = Base::convertTo<Base::Vector3d>(this->_vDirV);
    Base::Vector3d ez = Base::convertTo<Base::Vector3d>(this->_vDirW);

    Base::Matrix4D mat;
    mat[0][0] = ex.x;
    mat[0][1] = ey.x;
    mat[0][2] = ez.x;
    mat[0][3] = bs.x;

    mat[1][0] = ex.y;
    mat[1][1] = ey.y;
    mat[1][2] = ez.y;
    mat[1][3] = bs.y;

    mat[2][0] = ex.z;
    mat[2][1] = ey.z;
    mat[2][2] = ez.z;
    mat[2][3] = bs.z;

    std::transform(pts.begin(), pts.end(), pts.begin(), [&mat](const Base::Vector3d& pt) {
        Base::Vector3d v(pt);
        mat.multVec(v, v);
        return v;
    });
}

/*!
 * \brief SurfaceFit::toBezier
 * This function computes the Bezier representation of the polynomial surface of the form
 * f(x,y) = a*x*x + b*y*y + c*x*y + d*y + e*f + f
 * by getting the 3x3 control points.
 */
std::vector<Base::Vector3d>
SurfaceFit::toBezier(double umin, double umax, double vmin, double vmax) const
{
    std::vector<Base::Vector3d> pts;
    pts.reserve(9);

    // the Bezier surface is defined by the 3x3 control points
    // P11 P21 P31
    // P12 P22 P32
    // P13 P23 P33
    //
    // The surface goes through the points P11, P31, P31 and P33
    // To get the four control points P21, P12, P32 and P23 we inverse
    // the de-Casteljau algorithm used for Bezier curves of degree 2
    // as we already know the points for the parameters
    // (0, 0.5), (0.5, 0), (0.5, 1.0) and (1.0, 0.5)
    // To get the control point P22 we inverse the de-Casteljau algorithm
    // for the surface point on (0.5, 0.5)
    double umid = 0.5 * (umin + umax);
    double vmid = 0.5 * (vmin + vmax);

    // first row
    double z11 = Value(umin, vmin);
    double v21 = Value(umid, vmin);
    double z31 = Value(umax, vmin);
    double z21 = 2.0 * v21 - 0.5 * (z11 + z31);

    // third row
    double z13 = Value(umin, vmax);
    double v23 = Value(umid, vmax);
    double z33 = Value(umax, vmax);
    double z23 = 2.0 * v23 - 0.5 * (z13 + z33);

    // second row
    double v12 = Value(umin, vmid);
    double z12 = 2.0 * v12 - 0.5 * (z11 + z13);
    double v32 = Value(umax, vmid);
    double z32 = 2.0 * v32 - 0.5 * (z31 + z33);
    double v22 = Value(umid, vmid);
    double z22 = 4.0 * v22 - 0.25 * (z11 + z31 + z13 + z33 + 2.0 * (z12 + z21 + z32 + z23));

    // first row
    pts.emplace_back(umin, vmin, z11);
    pts.emplace_back(umid, vmin, z21);
    pts.emplace_back(umax, vmin, z31);

    // second row
    pts.emplace_back(umin, vmid, z12);
    pts.emplace_back(umid, vmid, z22);
    pts.emplace_back(umax, vmid, z32);

    // third row
    pts.emplace_back(umin, vmax, z13);
    pts.emplace_back(umid, vmax, z23);
    pts.emplace_back(umax, vmax, z33);
    return pts;
}

// -------------------------------------------------------------------------------

namespace MeshCore
{

struct LMCylinderFunctor
{
    Eigen::MatrixXd measuredValues;

    // Compute 'm' errors, one for each data point, for the given parameter values in 'x'
    int operator()(const Eigen::VectorXd& xvec, Eigen::VectorXd& fvec) const
    {
        // 'xvec' has dimensions n x 1
        // It contains the current estimates for the parameters.

        // 'fvec' has dimensions m x 1
        // It will contain the error for each data point.
        double aParam = xvec(0);  // dir_x
        double bParam = xvec(1);  // dir_y
        double cParam = xvec(2);  // dir_z
        double dParam = xvec(3);  // cnt_x
        double eParam = xvec(4);  // cnt_y
        double fParam = xvec(5);  // cnt_z
        double gParam = xvec(6);  // radius

        // use distance functions (fvec(i)) for cylinders as defined in the paper:
        // Least-Squares Fitting Algorithms of the NIST Algorithm Testing System
        for (int i = 0; i < values(); i++) {
            double xValue = measuredValues(i, 0);
            double yValue = measuredValues(i, 1);
            double zValue = measuredValues(i, 2);

            double a = aParam / (sqrt(aParam * aParam + bParam * bParam + cParam * cParam));
            double b = bParam / (sqrt(aParam * aParam + bParam * bParam + cParam * cParam));
            double c = cParam / (sqrt(aParam * aParam + bParam * bParam + cParam * cParam));
            double x = dParam;
            double y = eParam;
            double z = fParam;
            double u = c * (yValue - y) - b * (zValue - z);
            double v = a * (zValue - z) - c * (xValue - x);
            double w = b * (xValue - x) - a * (yValue - y);

            fvec(i) = sqrt(u * u + v * v + w * w) - gParam;
        }
        return 0;
    }

    // Compute the jacobian of the errors
    int df(const Eigen::VectorXd& x, Eigen::MatrixXd& fjac) const
    {
        // 'x' has dimensions n x 1
        // It contains the current estimates for the parameters.

        // 'fjac' has dimensions m x n
        // It will contain the jacobian of the errors, calculated numerically in this case.

        const double epsilon = 1e-5;

        for (int i = 0; i < x.size(); i++) {
            Eigen::VectorXd xPlus(x);
            xPlus(i) += epsilon;
            Eigen::VectorXd xMinus(x);
            xMinus(i) -= epsilon;

            Eigen::VectorXd fvecPlus(values());
            operator()(xPlus, fvecPlus);

            Eigen::VectorXd fvecMinus(values());
            operator()(xMinus, fvecMinus);

            Eigen::VectorXd fvecDiff(values());
            fvecDiff = (fvecPlus - fvecMinus) / (2.0f * epsilon);

            fjac.block(0, i, values(), 1) = fvecDiff;
        }

        return 0;
    }

    // Number of data points, i.e. values.
    int m;

    // Returns 'm', the number of values.
    int values() const
    {
        return m;
    }

    // The number of parameters, i.e. inputs.
    int n;

    // Returns 'n', the number of inputs.
    int inputs() const
    {
        return n;
    }
};

}  // namespace MeshCore

CylinderFit::CylinderFit()
    : _vBase(0, 0, 0)
    , _vAxis(0, 0, 1)
{}

Base::Vector3f CylinderFit::GetInitialAxisFromNormals(const std::vector<Base::Vector3f>& n) const
{
#if 0
    int nc = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    for (int i = 0; i < (int)n.size()-1; ++i) {
        for (int j = i+1; j < (int)n.size(); ++j) {
            Base::Vector3f cross = n[i] % n[j];
            if (cross.Sqr() > 1.0e-6) {
                cross.Normalize();
                x += cross.x;
                y += cross.y;
                z += cross.z;
                ++nc;
            }
        }
    }

    if (nc > 0) {
        x /= (double)nc;
        y /= (double)nc;
        z /= (double)nc;
        Base::Vector3f axis(x,y,z);
        axis.Normalize();
        return axis;
    }

    PlaneFit planeFit;
    planeFit.AddPoints(n);
    planeFit.Fit();
    return planeFit.GetNormal();
#endif

    // Like a plane fit where the base is at (0,0,0)
    double sxx {0.0};
    double sxy {0.0};
    double sxz {0.0};
    double syy {0.0};
    double syz {0.0};
    double szz {0.0};

    for (auto it : n) {
        sxx += double(it.x * it.x);
        sxy += double(it.x * it.y);
        sxz += double(it.x * it.z);
        syy += double(it.y * it.y);
        syz += double(it.y * it.z);
        szz += double(it.z * it.z);
    }

    Eigen::Matrix3d covMat = Eigen::Matrix3d::Zero();
    covMat(0, 0) = sxx;
    covMat(1, 1) = syy;
    covMat(2, 2) = szz;
    covMat(0, 1) = sxy;
    covMat(1, 0) = sxy;
    covMat(0, 2) = sxz;
    covMat(2, 0) = sxz;
    covMat(1, 2) = syz;
    covMat(2, 1) = syz;
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eig(covMat);
    Eigen::Vector3d w = eig.eigenvectors().col(0);

    Base::Vector3f normal;
    normal.Set(w.x(), w.y(), w.z());
    return normal;
}

void CylinderFit::SetInitialValues(const Base::Vector3f& b, const Base::Vector3f& n)
{
    _vBase = b;
    _vAxis = n;
    _initialGuess = true;
}

float CylinderFit::Fit()
{
    if (CountPoints() < 7) {
        return FLOAT_MAX;
    }
    _bIsFitted = true;

#if 1
    // Do the cylinder fit
    MeshCoreFit::CylinderFit cylFit;
    cylFit.AddPoints(_vPoints);
    if (_initialGuess) {
        cylFit.SetApproximations(_fRadius,
                                 Base::Vector3d(_vBase.x, _vBase.y, _vBase.z),
                                 Base::Vector3d(_vAxis.x, _vAxis.y, _vAxis.z));
    }

    float result = cylFit.Fit();
    if (result < FLOAT_MAX) {
        Base::Vector3d base = cylFit.GetBase();
        Base::Vector3d dir = cylFit.GetAxis();

#if defined(FC_DEBUG)
        Base::Console().Log(
            "MeshCoreFit::Cylinder Fit:  Base: (%0.4f, %0.4f, %0.4f),  Axis: (%0.6f, %0.6f, "
            "%0.6f),  Radius: %0.4f,  Std Dev: %0.4f,  Iterations: %d\n",
            base.x,
            base.y,
            base.z,
            dir.x,
            dir.y,
            dir.z,
            cylFit.GetRadius(),
            cylFit.GetStdDeviation(),
            cylFit.GetNumIterations());
#endif
        _vBase = Base::convertTo<Base::Vector3f>(base);
        _vAxis = Base::convertTo<Base::Vector3f>(dir);
        _fRadius = (float)cylFit.GetRadius();
        _fLastResult = result;
    }
#else
    int m = static_cast<int>(_vPoints.size());
    int n = 7;

    Eigen::MatrixXd measuredValues(m, 3);
    int index = 0;
    for (const auto& it : _vPoints) {
        measuredValues(index, 0) = it.x;
        measuredValues(index, 1) = it.y;
        measuredValues(index, 2) = it.z;
        index++;
    }

    Eigen::VectorXd x(n);
    x(0) = 1.0;  // initial value for dir_x
    x(1) = 1.0;  // initial value for dir_y
    x(2) = 1.0;  // initial value for dir_z
    x(3) = 0.0;  // initial value for cnt_x
    x(4) = 0.0;  // initial value for cnt_y
    x(5) = 0.0;  // initial value for cnt_z
    x(6) = 0.0;  // initial value for radius

    //
    // Run the LM optimization
    // Create a LevenbergMarquardt object and pass it the functor.
    //

    LMCylinderFunctor functor;
    functor.measuredValues = measuredValues;
    functor.m = m;
    functor.n = n;

    Eigen::LevenbergMarquardt<LMCylinderFunctor, double> lm(functor);
    int status = lm.minimize(x);
    Base::Console().Log("Cylinder fit: %d, iterations: %d, gradient norm: %f\n",
                        status,
                        lm.iter,
                        lm.gnorm);

    _vAxis.x = x(0);
    _vAxis.y = x(1);
    _vAxis.z = x(2);
    _vAxis.Normalize();

    _vBase.x = x(3);
    _vBase.y = x(4);
    _vBase.z = x(5);

    _fRadius = x(6);

    _fLastResult = lm.gnorm;
#endif

    return _fLastResult;
}

float CylinderFit::GetRadius() const
{
    return _fRadius;
}

Base::Vector3f CylinderFit::GetBase() const
{
    if (_bIsFitted) {
        return _vBase;
    }
    else {
        return Base::Vector3f();
    }
}

Base::Vector3f CylinderFit::GetAxis() const
{
    if (_bIsFitted) {
        return _vAxis;
    }
    else {
        return Base::Vector3f();
    }
}

float CylinderFit::GetDistanceToCylinder(const Base::Vector3f& rcPoint) const
{
    float fResult = FLOAT_MAX;
    if (_bIsFitted) {
        fResult = rcPoint.DistanceToLine(_vBase, _vAxis) - _fRadius;
    }
    return fResult;
}

float CylinderFit::GetStdDeviation() const
{
    // Mean: M=(1/N)*SUM Xi
    // Variance: VAR=(N/N-1)*[(1/N)*SUM(Xi^2)-M^2]
    // Standard deviation: SD=SQRT(VAR)
    // Standard error of the mean: SE=SD/SQRT(N)
    if (!_bIsFitted) {
        return FLOAT_MAX;
    }

    float fSumXi = 0.0f, fSumXi2 = 0.0f, fMean = 0.0f, fDist = 0.0f;

    float ulPtCt = float(CountPoints());
    std::list<Base::Vector3f>::const_iterator cIt;

    for (cIt = _vPoints.begin(); cIt != _vPoints.end(); ++cIt) {
        fDist = GetDistanceToCylinder(*cIt);
        fSumXi += fDist;
        fSumXi2 += (fDist * fDist);
    }

    fMean = (1.0f / ulPtCt) * fSumXi;
    return sqrt((ulPtCt / (ulPtCt - 1.0f)) * ((1.0f / ulPtCt) * fSumXi2 - fMean * fMean));
}

void CylinderFit::GetBounding(Base::Vector3f& bottom, Base::Vector3f& top) const
{
    float distMin = FLT_MAX;
    float distMax = FLT_MIN;

    std::list<Base::Vector3f>::const_iterator cIt;
    for (cIt = _vPoints.begin(); cIt != _vPoints.end(); ++cIt) {
        float dist = cIt->DistanceToPlane(_vBase, _vAxis);
        if (dist < distMin) {
            distMin = dist;
            bottom = *cIt;
        }
        if (dist > distMax) {
            distMax = dist;
            top = *cIt;
        }
    }

    // Project the points onto the cylinder axis
    bottom = bottom.Perpendicular(_vBase, _vAxis);
    top = top.Perpendicular(_vBase, _vAxis);
}

void CylinderFit::ProjectToCylinder()
{
    Base::Vector3f cBase(GetBase());
    Base::Vector3f cAxis(GetAxis());

    for (auto& cPnt : _vPoints) {
        if (cPnt.DistanceToLine(cBase, cAxis) > 0) {
            Base::Vector3f proj;
            cBase.ProjectToPlane(cPnt, cAxis, proj);
            Base::Vector3f diff = cPnt - proj;
            diff.Normalize();
            cPnt = proj + diff * _fRadius;
        }
        else {
            // Point is on the cylinder axis, so it can be moved in
            // any direction perpendicular to the cylinder axis
            Base::Vector3f cMov(cPnt);
            do {
                float x = (float(rand()) / float(RAND_MAX));
                float y = (float(rand()) / float(RAND_MAX));
                float z = (float(rand()) / float(RAND_MAX));
                cMov.Move(x, y, z);
            } while (cMov.DistanceToLine(cBase, cAxis) == 0);

            Base::Vector3f proj;
            cMov.ProjectToPlane(cPnt, cAxis, proj);
            Base::Vector3f diff = cPnt - proj;
            diff.Normalize();
            cPnt = proj + diff * _fRadius;
        }
    }
}

// -----------------------------------------------------------------------------

SphereFit::SphereFit()
    : _vCenter(0, 0, 0)
{}

float SphereFit::GetRadius() const
{
    if (_bIsFitted) {
        return _fRadius;
    }
    else {
        return FLOAT_MAX;
    }
}

Base::Vector3f SphereFit::GetCenter() const
{
    if (_bIsFitted) {
        return _vCenter;
    }
    else {
        return Base::Vector3f();
    }
}

float SphereFit::Fit()
{
    _bIsFitted = true;
    if (CountPoints() < 4) {
        return FLOAT_MAX;
    }

    std::vector<Wm4::Vector3d> input;
    std::transform(_vPoints.begin(),
                   _vPoints.end(),
                   std::back_inserter(input),
                   [](const Base::Vector3f& v) {
                       return Wm4::Vector3d(v.x, v.y, v.z);
                   });

    Wm4::Sphere3d sphere;
    Wm4::SphereFit3<double>(input.size(), input.data(), 10, sphere, false);
    _vCenter = Base::convertTo<Base::Vector3f>(sphere.Center);
    _fRadius = float(sphere.Radius);

    // TODO
    _fLastResult = 0;

#if defined(_DEBUG)
    Base::Console().Message("   WildMagic Sphere Fit:  Center: (%0.4f, %0.4f, %0.4f),  Radius: "
                            "%0.4f,  Std Dev: %0.4f\n",
                            _vCenter.x,
                            _vCenter.y,
                            _vCenter.z,
                            _fRadius,
                            GetStdDeviation());
#endif

    MeshCoreFit::SphereFit sphereFit;
    sphereFit.AddPoints(_vPoints);
    sphereFit.ComputeApproximations();
    float result = sphereFit.Fit();
    if (result < FLOAT_MAX) {
        Base::Vector3d center = sphereFit.GetCenter();
#if defined(_DEBUG)
        Base::Console().Message("MeshCoreFit::Sphere Fit:  Center: (%0.4f, %0.4f, %0.4f),  Radius: "
                                "%0.4f,  Std Dev: %0.4f,  Iterations: %d\n",
                                center.x,
                                center.y,
                                center.z,
                                sphereFit.GetRadius(),
                                sphereFit.GetStdDeviation(),
                                sphereFit.GetNumIterations());
#endif
        _vCenter = Base::convertTo<Base::Vector3f>(center);
        _fRadius = (float)sphereFit.GetRadius();
        _fLastResult = result;
    }

    return _fLastResult;
}

float SphereFit::GetDistanceToSphere(const Base::Vector3f& rcPoint) const
{
    float fResult = FLOAT_MAX;
    if (_bIsFitted) {
        fResult = Base::Vector3f(rcPoint - _vCenter).Length() - _fRadius;
    }
    return fResult;
}

float SphereFit::GetStdDeviation() const
{
    // Mean: M=(1/N)*SUM Xi
    // Variance: VAR=(N/N-1)*[(1/N)*SUM(Xi^2)-M^2]
    // Standard deviation: SD=SQRT(VAR)
    // Standard error of the mean: SE=SD/SQRT(N)
    if (!_bIsFitted) {
        return FLOAT_MAX;
    }

    float fSumXi = 0.0f, fSumXi2 = 0.0f, fMean = 0.0f, fDist = 0.0f;

    float ulPtCt = float(CountPoints());
    std::list<Base::Vector3f>::const_iterator cIt;

    for (cIt = _vPoints.begin(); cIt != _vPoints.end(); ++cIt) {
        fDist = GetDistanceToSphere(*cIt);
        fSumXi += fDist;
        fSumXi2 += (fDist * fDist);
    }

    fMean = (1.0f / ulPtCt) * fSumXi;
    return sqrt((ulPtCt / (ulPtCt - 1.0f)) * ((1.0f / ulPtCt) * fSumXi2 - fMean * fMean));
}

void SphereFit::ProjectToSphere()
{
    for (auto& cPnt : _vPoints) {
        // Compute unit vector from sphere centre to point.
        // Because this vector is orthogonal to the sphere's surface at the
        // intersection point we can easily compute the projection point on the
        // closest surface point using the radius of the sphere
        Base::Vector3f diff = cPnt - _vCenter;
        double length = diff.Length();
        if (length == 0.0) {
            // Point is exactly at the sphere center, so it can be projected in any direction onto
            // the sphere! So here just project in +Z direction
            cPnt.z += _fRadius;
        }
        else {
            diff /= length;  // normalizing the vector
            cPnt = _vCenter + diff * _fRadius;
        }
    }
}

// -------------------------------------------------------------------------------

PolynomialFit::PolynomialFit()
    : _fCoeff {}
{}

float PolynomialFit::Fit()
{
    std::vector<float> x, y, z;
    x.reserve(_vPoints.size());
    y.reserve(_vPoints.size());
    z.reserve(_vPoints.size());
    for (std::list<Base::Vector3f>::const_iterator it = _vPoints.begin(); it != _vPoints.end();
         ++it) {
        x.push_back(it->x);
        y.push_back(it->y);
        z.push_back(it->z);
    }

    try {
        float* coeff = Wm4::PolyFit3<float>(_vPoints.size(), &(x[0]), &(y[0]), &(z[0]), 2, 2);
        for (int i = 0; i < 9; i++) {
            _fCoeff[i] = coeff[i];
        }
    }
    catch (const std::exception&) {
        return FLOAT_MAX;
    }

    return 0.0f;
}

float PolynomialFit::Value(float x, float y) const
{
    float fValue = _fCoeff[0] + _fCoeff[1] * x + _fCoeff[2] * x * x + _fCoeff[3] * y
        + _fCoeff[4] * x * y + _fCoeff[5] * x * x * y + _fCoeff[6] * y * y + _fCoeff[7] * x * y * y
        + _fCoeff[8] * x * x * y * y;
    return fValue;
}
