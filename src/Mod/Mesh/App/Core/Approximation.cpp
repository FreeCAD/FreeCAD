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
# include <algorithm>
#endif

#include "Approximation.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <Mod/Mesh/App/WildMagic4/Wm4ApprQuadraticFit3.h>
#include <Mod/Mesh/App/WildMagic4/Wm4ApprPlaneFit3.h>
#include <Mod/Mesh/App/WildMagic4/Wm4DistVector3Plane3.h>
#include <Mod/Mesh/App/WildMagic4/Wm4Matrix3.h>
#include <Mod/Mesh/App/WildMagic4/Wm4ApprPolyFit3.h>

//#define FC_USE_EIGEN
//#define FC_USE_BOOST
#if defined(FC_USE_BOOST)
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/bindings/traits/ublas_matrix.hpp>
#include <boost/numeric/bindings/traits/ublas_vector.hpp>

#define BOOST_NUMERIC_BINDINGS_USE_CLAPACK
#include <boost/numeric/bindings/lapack/syev.hpp>
#include <boost/numeric/bindings/lapack/heev.hpp>
#include <boost/numeric/bindings/lapack/gesv.hpp> 

namespace ublas = boost::numeric::ublas; 
extern "C" void LAPACK_DGESV (int const* n, int const* nrhs, 
                     double* a, int const* lda, int* ipiv, 
                     double* b, int const* ldb, int* info);
#elif defined(FC_USE_EIGEN)
# include <Eigen/LeastSquares>
#endif
# include <Eigen/LeastSquares>

using namespace MeshCore;


void Approximation::Convert( const Wm4::Vector3<double>& Wm4, Base::Vector3f& pt)
{
    pt.Set( (float)Wm4.X(), (float)Wm4.Y(), (float)Wm4.Z() );
}

void Approximation::Convert( const Base::Vector3f& pt, Wm4::Vector3<double>& Wm4)
{
    Wm4.X() = pt.x; Wm4.Y() = pt.y; Wm4.Z() = pt.z;
}

void Approximation::GetMgcVectorArray(std::vector< Wm4::Vector3<double> >& rcPts) const
{
    std::list< Base::Vector3f >::const_iterator It;
    for (It = _vPoints.begin(); It != _vPoints.end(); ++It) {
        Wm4::Vector3<double> pt( (*It).x, (*It).y, (*It).z );
        rcPts.push_back( pt );
    }
}

void Approximation::AddPoint(const Base::Vector3f &rcVector)
{
    _vPoints.push_back(rcVector);
    _bIsFitted = false;
}

void Approximation::AddPoints(const std::vector<Base::Vector3f> &rvPointVect)
{
    std::vector<Base::Vector3f>::const_iterator cIt;
    for (cIt = rvPointVect.begin(); cIt != rvPointVect.end(); ++cIt)
        _vPoints.push_back(*cIt);
    _bIsFitted = false;
}

void Approximation::AddPoints(const std::set<Base::Vector3f> &rsPointSet)
{
    std::set<Base::Vector3f>::const_iterator cIt;
    for (cIt = rsPointSet.begin(); cIt != rsPointSet.end(); ++cIt)
        _vPoints.push_back(*cIt);
    _bIsFitted = false;
}

void Approximation::AddPoints(const std::list<Base::Vector3f> &rsPointList)
{
    std::list<Base::Vector3f>::const_iterator cIt;
    for (cIt = rsPointList.begin(); cIt != rsPointList.end(); ++cIt)
        _vPoints.push_back(*cIt);
    _bIsFitted = false;
}

Base::Vector3f Approximation::GetGravity() const
{
    Base::Vector3f clGravity;
    for (std::list<Base::Vector3f>::const_iterator it = _vPoints.begin(); it!=_vPoints.end(); ++it)
        clGravity += *it;
    clGravity *= 1.0f / float(_vPoints.size());
    return clGravity;
}

unsigned long Approximation::CountPoints() const
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

float PlaneFit::Fit()
{
    _bIsFitted = true;
    if (CountPoints() < 3)
        return FLOAT_MAX;

    double sxx,sxy,sxz,syy,syz,szz,mx,my,mz;
    sxx=sxy=sxz=syy=syz=szz=mx=my=mz=0.0f;

    for (std::list<Base::Vector3f>::iterator it = _vPoints.begin(); it!=_vPoints.end(); ++it) {
        sxx += it->x * it->x; sxy += it->x * it->y;
        sxz += it->x * it->z; syy += it->y * it->y;
        syz += it->y * it->z; szz += it->z * it->z;
        mx  += it->x;   my += it->y;   mz += it->z;
    }

    unsigned int nSize = _vPoints.size();
    sxx = sxx - mx*mx/((double)nSize);
    sxy = sxy - mx*my/((double)nSize);
    sxz = sxz - mx*mz/((double)nSize);
    syy = syy - my*my/((double)nSize);
    syz = syz - my*mz/((double)nSize);
    szz = szz - mz*mz/((double)nSize);

#if defined(FC_USE_BOOST)
    ublas::matrix<double> A(3,3);
    A(0,0) = sxx;
    A(1,1) = syy;
    A(2,2) = szz;
    A(0,1) = sxy; A(1,0) = sxy;
    A(0,2) = sxz; A(2,0) = sxz;
    A(1,2) = syz; A(2,1) = syz;
    namespace lapack= boost::numeric::bindings::lapack;
    ublas::vector<double> eigenval(3);
    int r = lapack::syev('V','U',A,eigenval,lapack::optimal_workspace());
    if (r) {
    }
    float sigma = 0;
#elif defined(FC_USE_EIGEN)
    Eigen::Matrix3d covMat = Eigen::Matrix3d::Zero();
    covMat(0,0) = sxx;
    covMat(1,1) = syy;
    covMat(2,2) = szz;
    covMat(0,1) = sxy; covMat(1,0) = sxy;
    covMat(0,2) = sxz; covMat(2,0) = sxz;
    covMat(1,2) = syz; covMat(2,1) = syz;
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eig(covMat);

    Eigen::Vector3d u = eig.eigenvectors().col(1);
    Eigen::Vector3d v = eig.eigenvectors().col(2);
    Eigen::Vector3d w = eig.eigenvectors().col(0);

    _vDirU.Set(u.x(), u.y(), u.z());
    _vDirV.Set(v.x(), v.y(), v.z());
    _vDirW.Set(w.x(), w.y(), w.z());
    _vBase.Set(mx/(float)nSize, my/(float)nSize, mz/(float)nSize);

    float sigma = w.dot(covMat * w);
#else
    // Covariance matrix
    Wm4::Matrix3<double> akMat(sxx,sxy,sxz,sxy,syy,syz,sxz,syz,szz);
    Wm4::Matrix3<double> rkRot, rkDiag;
    try {
        akMat.EigenDecomposition(rkRot, rkDiag);
    }
    catch (const std::exception&) {
        return FLOAT_MAX;
    }

    Wm4::Vector3<double> U = rkRot.GetColumn(1);
    Wm4::Vector3<double> V = rkRot.GetColumn(2);
    Wm4::Vector3<double> W = rkRot.GetColumn(0);

    // It may happen that the result have nan values
    for (int i=0; i<3; i++) {
        if (boost::math::isnan(U[i]) || 
            boost::math::isnan(V[i]) ||
            boost::math::isnan(W[i]))
            return FLOAT_MAX;
    }

    _vDirU.Set((float)U.X(), (float)U.Y(), (float)U.Z());
    _vDirV.Set((float)V.X(), (float)V.Y(), (float)V.Z());
    _vDirW.Set((float)W.X(), (float)W.Y(), (float)W.Z());
    _vBase.Set((float)(mx/nSize), (float)(my/nSize), (float)(mz/nSize));
    float sigma = (float)W.Dot(akMat * W);
#endif

    // make a right-handed system
    if ((_vDirU % _vDirV) * _vDirW < 0.0f) {
        Base::Vector3f tmp = _vDirU;
        _vDirU = _vDirV;
        _vDirV = tmp;
    }

    if (nSize > 3)
        sigma = sqrt(sigma/(nSize-3));
    _fLastResult = sigma;
    return _fLastResult;
}

Base::Vector3f PlaneFit::GetBase() const
{
    if (_bIsFitted)
        return _vBase;
    else
        return Base::Vector3f();
}

Base::Vector3f PlaneFit::GetDirU() const
{
    if (_bIsFitted)
        return _vDirU;
    else
        return Base::Vector3f();
}

Base::Vector3f PlaneFit::GetDirV() const
{
    if (_bIsFitted)
        return _vDirV;
    else
        return Base::Vector3f();
}

Base::Vector3f PlaneFit::GetNormal() const
{
    if (_bIsFitted)
        return _vDirW;
    else
        return Base::Vector3f();
}

float PlaneFit::GetDistanceToPlane(const Base::Vector3f &rcPoint) const
{
    float fResult = FLOAT_MAX;
    if (_bIsFitted)
        fResult = (rcPoint - _vBase) * _vDirW;
    return fResult;
}

float PlaneFit::GetStdDeviation() const
{
    // Mean: M=(1/N)*SUM Xi
    // Variance: VAR=(N/N-3)*[(1/N)*SUM(Xi^2)-M^2]
    // Standard deviation: SD=SQRT(VAR)
    // Standard error of the mean: SE=SD/SQRT(N)
    if (!_bIsFitted)
        return FLOAT_MAX;

    float fSumXi = 0.0f, fSumXi2 = 0.0f,
          fMean  = 0.0f, fDist   = 0.0f;

    float ulPtCt = (float)CountPoints();
    std::list< Base::Vector3f >::const_iterator cIt;

    for (cIt = _vPoints.begin(); cIt != _vPoints.end(); ++cIt) {
        fDist = GetDistanceToPlane( *cIt );
        fSumXi  += fDist;
        fSumXi2 += ( fDist * fDist );
    }

    fMean = (1.0f / ulPtCt) * fSumXi;
    return (float)sqrt((ulPtCt / (ulPtCt - 3.0)) * ((1.0 / ulPtCt) * fSumXi2 - fMean * fMean));
}

float PlaneFit::GetSignedStdDeviation() const
{
    // if the nearest point to the gravity is at the side 
    // of normal direction the value will be 
    // positive otherwise negative
    if (!_bIsFitted)
        return FLOAT_MAX;

    float fSumXi = 0.0f, fSumXi2 = 0.0f,
          fMean  = 0.0f, fDist   = 0.0f;
    float fMinDist = FLOAT_MAX;
    float fFactor;

    float ulPtCt = (float)CountPoints();
    Base::Vector3f clGravity, clPt;
    std::list<Base::Vector3f>::const_iterator cIt;
    for (cIt = _vPoints.begin(); cIt != _vPoints.end(); cIt++)
        clGravity += *cIt;
    clGravity *= (1.0f / ulPtCt);

    for (cIt = _vPoints.begin(); cIt != _vPoints.end(); ++cIt) {
        if ((clGravity - *cIt).Length() < fMinDist) {
            fMinDist = (clGravity - *cIt).Length();
            clPt = *cIt;
        }
        fDist = GetDistanceToPlane(*cIt);
        fSumXi  += fDist;
        fSumXi2 += (fDist * fDist);
    }

    // which side
    if ((clPt-clGravity)*GetNormal() > 0)
        fFactor = 1.0f;
    else
        fFactor = -1.0f;

    fMean = 1.0f / ulPtCt * fSumXi;

    return fFactor * (float)sqrt((ulPtCt / (ulPtCt - 3.0)) * ((1.0 / ulPtCt) * fSumXi2 - fMean * fMean));
}

void PlaneFit::ProjectToPlane ()
{
    Base::Vector3f cGravity(GetGravity());
    Base::Vector3f cNormal (GetNormal ());

    for (std::list< Base::Vector3f >::iterator it = _vPoints.begin(); it != _vPoints.end(); ++it) {
        Base::Vector3f& cPnt = *it;
        float fD = (cPnt - cGravity) * cNormal;
        cPnt = cPnt - fD * cNormal;
    }
}

// -------------------------------------------------------------------------------

bool QuadraticFit::GetCurvatureInfo(double x, double y, double z,
                                    double &rfCurv0, double &rfCurv1,
                                    Base::Vector3f &rkDir0, Base::Vector3f &rkDir1, double &dDistance)
{
    assert( _bIsFitted );
    bool bResult = false;

    if (_bIsFitted) {
        Wm4::Vector3<double> Dir0, Dir1;
        FunctionContainer  clFuncCont( _fCoeff );
        bResult = clFuncCont.CurvatureInfo( x, y, z, rfCurv0, rfCurv1, Dir0, Dir1, dDistance );

        dDistance = clFuncCont.GetGradient( x, y, z ).Length();
        Convert( Dir0, rkDir0 );
        Convert( Dir1, rkDir1 );
    }

    return bResult;
}

bool QuadraticFit::GetCurvatureInfo(double x, double y, double z, double &rfCurv0, double &rfCurv1)
{
    bool bResult = false;

    if (_bIsFitted) {
        FunctionContainer clFuncCont( _fCoeff );
        bResult = clFuncCont.CurvatureInfo( x, y, z, rfCurv0, rfCurv1 );
    }

    return bResult;
}

const double& QuadraticFit::GetCoeffArray() const
{
    return _fCoeff[0];
}

double QuadraticFit::GetCoeff(unsigned long ulIndex) const
{
    assert( ulIndex >= 0 && ulIndex < 10 );

    if( _bIsFitted )
        return _fCoeff[ ulIndex ];
    else
        return FLOAT_MAX;
}

float QuadraticFit::Fit()
{
    float fResult = FLOAT_MAX;

    if (CountPoints() > 0) {
        std::vector< Wm4::Vector3<double> > cPts;
        GetMgcVectorArray( cPts );
        fResult = (float) Wm4::QuadraticFit3<double>( CountPoints(), &(cPts[0]), _fCoeff );
        _fLastResult = fResult;

        _bIsFitted = true;
    }

    return fResult;
}

void QuadraticFit::CalcEigenValues(double &dLambda1, double &dLambda2, double &dLambda3,
                                   Base::Vector3f &clEV1, Base::Vector3f &clEV2, Base::Vector3f &clEV3) const
{
    assert( _bIsFitted );

    /*
     * F(x,y,z) = a11*x*x + a22*y*y + a33*z*z +2*a12*x*y + 2*a13*x*z + 2*a23*y*z + 2*a10*x + 2*a20*y + 2*a30*z * a00 = 0
     *
     * Formenmatrix:
     *
     *      ( a11   a12   a13 )
     * A =  ( a21   a22   a23 )       wobei gilt a[i,j] = a[j,i]
     *      ( a31   a32   a33 )
     *
     * Koeffizienten des Quadrik-Fits bezogen auf die hier verwendete Schreibweise:
     * 
     *   0 = C[0] + C[1]*X + C[2]*Y + C[3]*Z + C[4]*X^2 + C[5]*Y^2
     *     + C[6]*Z^2 + C[7]*X*Y + C[8]*X*Z + C[9]*Y*Z
     *
     * Quadratisch:   a11 := c[4],    a22 := c[5],    a33 := c[6]
     * Gemischt:      a12 := c[7]/2,  a13 := c[8]/2,  a23 := c[9]/2
     * Linear:        a10 := c[1]/2,  a20 := c[2]/2,  a30 := c[3]/2
     * Konstant:      a00 := c[0]
     *
     */

    Wm4::Matrix3<double>  akMat(_fCoeff[4],       _fCoeff[7]/2.0f, _fCoeff[8]/2.0f,
                                _fCoeff[7]/2.0f,  _fCoeff[5],      _fCoeff[9]/2.0f,
                                _fCoeff[8]/2.0f,  _fCoeff[9]/2.0f, _fCoeff[6]       );

    Wm4::Matrix3<double> rkRot, rkDiag;
    akMat.EigenDecomposition( rkRot, rkDiag );

    Wm4::Vector3<double> vEigenU = rkRot.GetColumn(0);
    Wm4::Vector3<double> vEigenV = rkRot.GetColumn(1);
    Wm4::Vector3<double> vEigenW = rkRot.GetColumn(2);

    Convert( vEigenU, clEV1 );
    Convert( vEigenV, clEV2 );
    Convert( vEigenW, clEV3 );

    dLambda1 = rkDiag[0][0];
    dLambda2 = rkDiag[1][1];
    dLambda3 = rkDiag[2][2];
}

void QuadraticFit::CalcZValues( double x, double y, double &dZ1, double &dZ2 ) const
{
    assert( _bIsFitted );

    double dDisk = _fCoeff[3]*_fCoeff[3]+2*_fCoeff[3]*_fCoeff[8]*x+2*_fCoeff[3]*_fCoeff[9]*y+
                   _fCoeff[8]*_fCoeff[8]*x*x+2*_fCoeff[8]*x*_fCoeff[9]*y+_fCoeff[9]*_fCoeff[9]*y*y-
                   4*_fCoeff[6]*_fCoeff[0]-4*_fCoeff[6]*_fCoeff[1]*x-4*_fCoeff[6]*_fCoeff[2]*y-
                   4*_fCoeff[6]*_fCoeff[7]*x*y-4*_fCoeff[6]*_fCoeff[4]*x*x-4*_fCoeff[6]*_fCoeff[5]*y*y;

    if (fabs( _fCoeff[6] ) < 0.000005) {
        dZ1 = FLOAT_MAX; 
        dZ2 = FLOAT_MAX; 
        return;
    }

    if (dDisk < 0.0f) {
        dZ1 = FLOAT_MAX; 
        dZ2 = FLOAT_MAX; 
        return;
    }
    else
        dDisk = sqrt( dDisk );

    dZ1 = 0.5 * ( ( -_fCoeff[3] - _fCoeff[8]*x - _fCoeff[9]*y + dDisk ) / _fCoeff[6] );
    dZ2 = 0.5 * ( ( -_fCoeff[3] - _fCoeff[8]*x - _fCoeff[9]*y - dDisk ) / _fCoeff[6] );
}

// -------------------------------------------------------------------------------

SurfaceFit::SurfaceFit()
   : PlaneFit()
{
    _fCoeff[0] = 0.0f;
    _fCoeff[1] = 0.0f;
    _fCoeff[2] = 0.0f;
    _fCoeff[3] = 0.0f;
    _fCoeff[4] = 0.0f;
    _fCoeff[5] = 0.0f;
    _fCoeff[6] = 0.0f;
    _fCoeff[7] = 0.0f;
    _fCoeff[8] = 0.0f;
    _fCoeff[9] = 0.0f;
}

float SurfaceFit::Fit()
{
    float fResult = FLOAT_MAX;

    if (CountPoints() > 0) {
        fResult = (float) PolynomFit();
        _fLastResult = fResult;

        _bIsFitted = true;
    }

    return fResult;
}

bool SurfaceFit::GetCurvatureInfo(double x, double y, double z, double &rfCurv0, double &rfCurv1,
                                  Base::Vector3f &rkDir0, Base::Vector3f &rkDir1, double &dDistance )
{
    bool bResult = false;

    if (_bIsFitted) {
        Wm4::Vector3<double> Dir0, Dir1;
        FunctionContainer  clFuncCont( _fCoeff );
        bResult = clFuncCont.CurvatureInfo( x, y, z, rfCurv0, rfCurv1, Dir0, Dir1, dDistance );

        dDistance = clFuncCont.GetGradient( x, y, z ).Length();
        Convert( Dir0, rkDir0 );
        Convert( Dir1, rkDir1 );
    }

    return bResult;
}

bool SurfaceFit::GetCurvatureInfo(double x, double y, double z, double &rfCurv0, double &rfCurv1)
{
    assert( _bIsFitted );
    bool bResult = false;

    if (_bIsFitted) {
        FunctionContainer clFuncCont( _fCoeff );
        bResult = clFuncCont.CurvatureInfo( x, y, z, rfCurv0, rfCurv1 );
    }

    return bResult;
}

double SurfaceFit::PolynomFit()
{
    if (PlaneFit::Fit() == FLOAT_MAX)
        return FLOAT_MAX;

    Base::Vector3d bs(this->_vBase.x,this->_vBase.y,this->_vBase.z);
    Base::Vector3d ex(this->_vDirU.x,this->_vDirU.y,this->_vDirU.z);
    Base::Vector3d ey(this->_vDirV.x,this->_vDirV.y,this->_vDirV.z);
    Base::Vector3d ez(this->_vDirW.x,this->_vDirW.y,this->_vDirW.z);

#if defined(FC_USE_BOOST)
    ublas::matrix<double> A(6,6);
    ublas::vector<double> b(6);
    for (int i=0; i<6; i++) {
        for (int j=0; j<6; j++) {
            A(i,j) = 0.0;
        }
        b(i) = 0.0;
    }
#else
    Eigen::Matrix<double,6,6> A = Eigen::Matrix<double,6,6>::Zero();
    Eigen::Matrix<double,6,1> b = Eigen::Matrix<double,6,1>::Zero();
    Eigen::Matrix<double,6,1> x = Eigen::Matrix<double,6,1>::Zero();
#endif

    for (std::list<Base::Vector3f>::const_iterator it = _vPoints.begin(); it != _vPoints.end(); it++) {
        Base::Vector3d clPoint(it->x,it->y,it->z);
        clPoint.TransformToCoordinateSystem(bs, ex, ey);
        double dU = clPoint.x;
        double dV = clPoint.y;
        double dW = clPoint.z;

        double dU2 = dU*dU;
        double dV2 = dV*dV;
        double dUV = dU*dV;

        A(0,0) = A(0,0) + dU2*dU2;
        A(0,1) = A(0,1) + dU2*dV2;
        A(0,2) = A(0,2) + dU2*dUV;
        A(0,3) = A(0,3) + dU2*dU ;
        A(0,4) = A(0,4) + dU2*dV ;
        A(0,5) = A(0,5) + dU2    ;
        b(0)   = b(0)   + dU2*dW ;

        A(1,1) = A(1,1) + dV2*dV2;
        A(1,2) = A(1,2) + dV2*dUV;
        A(1,3) = A(1,3) + dV2*dU ;
        A(1,4) = A(1,4) + dV2*dV ;
        A(1,5) = A(1,5) + dV2    ;
        b(1)   = b(1)   + dV2*dW ;

        A(2,2) = A(2,2) + dUV*dUV;
        A(2,3) = A(2,3) + dUV*dU ;
        A(2,4) = A(2,4) + dUV*dV ;
        A(2,5) = A(2,5) + dUV    ;
        b(3)   = b(3)   + dUV*dW ;

        A(3,3) = A(3,3) + dU *dU ;
        A(3,4) = A(3,4) + dU *dV ;
        A(3,5) = A(3,5) + dU     ;
        b(3)   = b(3)   + dU *dW ;

        A(4,4) = A(4,4) + dV *dV ;
        A(4,5) = A(4,5) + dV     ;
        b(5)   = b(5)   + dV *dW ;

        A(5,5) = A(5,5) + 1      ;
        b(5)   = b(5)   + 1  *dW ;
    }

    // Mat is symmetric
    //
    A(1,0) = A(0,1);
    A(2,0) = A(0,2);
    A(3,0) = A(0,3);
    A(4,0) = A(0,4);
    A(5,0) = A(0,5);

    A(2,1) = A(1,2);
    A(3,1) = A(1,3);
    A(4,1) = A(1,4);
    A(5,1) = A(1,5);

    A(3,2) = A(2,3);
    A(4,2) = A(2,4);
    A(5,2) = A(2,5);

    A(4,3) = A(3,4);
    A(5,3) = A(3,5);

    A(5,4) = A(4,5);

#if defined(FC_USE_BOOST)
    namespace lapack= boost::numeric::bindings::lapack;
    //std::clog << A << std::endl;
    //std::clog << b << std::endl;
    //lapack::gesv(A,b);
    ublas::vector<double> x(6);
    x = b;
    //std::clog << b << std::endl;
#else
    // A.llt().solve(b,&x); // not sure if always positive definite
    A.qr().solve(b,&x);
#endif

    _fCoeff[0] = (float)(-x(5));
    _fCoeff[1] = (float)(-x(3));
    _fCoeff[2] = (float)(-x(4));
    _fCoeff[3] = 1.0f;
    _fCoeff[4] = (float)(-x(0));
    _fCoeff[5] = (float)(-x(1));
    _fCoeff[6] = 0.0f;
    _fCoeff[7] = (float)(-x(2));
    _fCoeff[8] = 0.0f;
    _fCoeff[9] = 0.0f;

    return 0.0f;
}

double SurfaceFit::Value(double x, double y) const
{
    float z = 0.0f;
    if (_bIsFitted) {
        FunctionContainer clFuncCont(_fCoeff);
        z = (float) clFuncCont.F(x, y, 0.0f);
    }

    return z;
}

// -----------------------------------------------------------------------------

PolynomialFit::PolynomialFit()
{
    for (int i=0; i<9; i++)
        _fCoeff[i] = 0.0f;
}

PolynomialFit::~PolynomialFit()
{
}

float PolynomialFit::Fit()
{
    std::vector<float> x, y, z;
    x.reserve(_vPoints.size());
    y.reserve(_vPoints.size());
    z.reserve(_vPoints.size());
    for (std::list<Base::Vector3f>::const_iterator it = _vPoints.begin(); it != _vPoints.end(); ++it) {
        x.push_back(it->x);
        y.push_back(it->y);
        z.push_back(it->z);
    }

    try {
        float* coeff = Wm4::PolyFit3<float>(_vPoints.size(), &(x[0]), &(y[0]), &(z[0]), 2 , 2);
        for (int i=0; i<9; i++)
            _fCoeff[i] = coeff[i];
    }
    catch (const std::exception&) {
        return FLOAT_MAX;
    }

    return 0.0f;
}

float PolynomialFit::Value(float x, float y) const
{
    float fValue = 
    _fCoeff[0]                   +
    _fCoeff[1] * x               +
    _fCoeff[2] * x * x           +
    _fCoeff[3]         * y       +
    _fCoeff[4] * x     * y       +
    _fCoeff[5] * x * x * y       +
    _fCoeff[6]         * y * y   +      
    _fCoeff[7] * x     * y * y   +
    _fCoeff[8] * x * x * y * y;
    return fValue;
}
