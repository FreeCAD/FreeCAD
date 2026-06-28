// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <Geom_BSplineSurface.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <math_Matrix.hxx>

#include <Base/Vector3D.h>
#include <Mod/ReverseEngineering/ReverseEngineeringGlobal.h>


namespace Base
{
class SequencerLauncher;
}

// TODO: Replace OCC stuff with ublas & co

namespace Reen
{

class ReenExport SplineBasisfunction
{
public:
    enum ValueT
    {
        Zero = 0,
        Full,
        Other
    };
    /**
     * Constructor
     * @param iSize Length of Knots vector
     */
    explicit SplineBasisfunction(int iSize);

    /**
     * Constructor
     * @param vKnots Knot vector
     * @param iOrder Order (degree + 1) of the basic polynomial
     */
    explicit SplineBasisfunction(TColStd_Array1OfReal& vKnots, int iOrder = 1);

    /**
     * Constructor
     * @param vKnots Knot vector of shape (value)
     * @param vMults Knot vector of shape (multiplicity)
     * @param iSize Length of the knot vector
     * The arrays @a vKnots and @a vMults have to be of the same size
     * and the sum of the values in @a vMults has to be identical to @a iSize.
     * @param iOrder Order (degree + 1) of the basic polynomial
     */
    SplineBasisfunction(
        TColStd_Array1OfReal& vKnots,
        TColStd_Array1OfInteger& vMults,
        int iSize,
        int iOrder = 1
    );

    virtual ~SplineBasisfunction();

    /**
     * Indicates whether the function value Nik(t) at the point fParam
     * results in 0, 1 or a value in between.
     * This serves to speed up the calculation under certain circumstances.
     *
     * @param iIndex Index
     * @param fParam Parameter value
     * @return ValueT
     */
    virtual ValueT LocalSupport(int iIndex, double fParam) = 0;
    /**
     * Calculates the function value Nik(t) at the point fParam
     * (from: Piegl/Tiller 96 The NURBS-Book)
     *
     * @param iIndex Index
     * @param fParam Parameter value
     * @return Function value Nik(t)
     */
    virtual double BasisFunction(int iIndex, double fParam) = 0;
    /**
     * Calculates the function values of the first iMaxDer derivatives on the
     * fParam position (from: Piegl/Tiller 96 The NURBS-Book)
     *
     * @param iIndex  Index
     * @param iMaxDer max. derivative
     * @param fParam  Parameter value.
     * @return Derivative list of function values
     *
     * The list must be sufficiently long for iMaxDer+1 elements.
     */
    virtual void DerivativesOfBasisFunction(
        int iIndex,
        int iMaxDer,
        double fParam,
        TColStd_Array1OfReal& Derivat
    ) = 0;

    /**
     * Calculates the kth derivative at the point fParam
     */
    virtual double DerivativeOfBasisFunction(int iIndex, int k, double fParam) = 0;

    /**
     * Sets the knot vector and the order. The size of the knot vector has to be exactly as
     * large as defined in the constructor.
     */
    virtual void SetKnots(TColStd_Array1OfReal& vKnots, int iOrder = 1);

    /**
     * Sets the knot vector and the order. The knot vector in the form of (Value, Multiplicity)
     * is passed on. Internally, this is converted into a knot vector in the form of (value, 1).
     * The size of this new vector has to be exactly as big as specified in the constructor.
     */
    virtual void SetKnots(TColStd_Array1OfReal& vKnots, TColStd_Array1OfInteger& vMults, int iOrder = 1);

protected:  // Member
    // Knot vector
    TColStd_Array1OfReal _vKnotVector;

    // Order (=Degree+1)
    int _iOrder;
};

class ReenExport BSplineBasis: public SplineBasisfunction
{
public:
    /**
     * Constructor
     * @param iSize Length of the knot vector
     */
    explicit BSplineBasis(int iSize);

    /**
     * Constructor
     * @param vKnots Knot vector
     * @param iOrder Order (degree + 1) of the basic polynomial
     */
    explicit BSplineBasis(TColStd_Array1OfReal& vKnots, int iOrder = 1);

    /**
     * Constructor
     * @param vKnots Knot vector of shape (value)
     * @param vMults Knot vector of shape (multiplicity)
     * @param iSize Length of the knot vector
     * The arrays @a vKnots and @a vMults have to be of the same size and the
     * sum of the values in @a vMults has to be identical to @a iSize.
     * @param iOrder Order (degree + 1) of the basic polynomial
     */
    BSplineBasis(TColStd_Array1OfReal& vKnots, TColStd_Array1OfInteger& vMults, int iSize, int iOrder = 1);

    /**
     * Specifies the knot index for the parameter value (from: Piegl/Tiller 96 The NURBS-Book)
     * @param fParam Parameter value
     * @return Knot index
     */
    virtual int FindSpan(double fParam);

    /**
     * Calculates the function values of the basic functions that do not vanish at fParam.
     * It must be ensured that the list for d (= degree of the B-spline)
     * elements (0, ..., d-1) is sufficient (from: Piegl/Tiller 96 The NURBS-Book)
     * @param fParam Parameter
     * @param vFuncVals List of function values
     * Index, Parameter value
     */
    virtual void AllBasisFunctions(double fParam, TColStd_Array1OfReal& vFuncVals);

    /**
     * Specifies whether the function value Nik(t) at the position fParam
     * results in 0, 1 or a value in between.
     * This serves to speed up the calculation under certain circumstances.
     *
     * @param iIndex Index
     * @param fParam Parameter value
     * @return ValueT
     */
    ValueT LocalSupport(int iIndex, double fParam) override;

    /**
     * Calculates the function value Nik(t) at the point fParam
     * (from: Piegl/Tiller 96 The NURBS-Book)
     * @param iIndex Index
     * @param fParam Parameter value
     * @return Function value Nik(t)
     */
    double BasisFunction(int iIndex, double fParam) override;

    /**
     * Calculates the function values of the first iMaxDer derivatives at the point fParam
     * (from: Piegl/Tiller 96 The NURBS-Book)
     * @param iIndex Index
     * @param iMaxDer max. derivative
     * @param fParam Parameter value
     * @param Derivat
     * The list must be sufficiently long for iMaxDer+1 elements.
     * @return List of function values
     */
    void DerivativesOfBasisFunction(
        int iIndex,
        int iMaxDer,
        double fParam,
        TColStd_Array1OfReal& Derivat
    ) override;

    /**
     * Calculates the kth derivative at the point fParam
     */
    double DerivativeOfBasisFunction(int iIndex, int k, double fParam) override;

    /**
     * Calculates the integral of the product of two B-splines or their derivatives.
     * The integration area extends over the entire domain of definition.
     * The integral is calculated by means of the Gaussian quadrature formulas.
     */
    virtual double GetIntegralOfProductOfBSplines(int i, int j, int r, int s);

    /**
     * Destructor
     */
    ~BSplineBasis() override;

protected:
    /**
     * Calculates the roots of the Legendre-Polynomials and the corresponding weights
     */
    virtual void GenerateRootsAndWeights(
        TColStd_Array1OfReal& vAbscissas,
        TColStd_Array1OfReal& vWeights
    );

    /**
     * Calculates the limits of integration (Indexes of the knots)
     */
    virtual void FindIntegrationArea(int iIdx1, int iIdx2, int& iBegin, int& iEnd);

    /**
     * Calculates the number of roots/weights of the Legendre-Polynomials to be used as a function
     * of the degree
     */
    int CalcSize(int r, int s);
};

class ReenExport ParameterCorrection
{

public:
    // Constructor
    explicit ParameterCorrection(
        unsigned usUOrder = 4,       // Order in u-direction (order = degree + 1)
        unsigned usVOrder = 4,       // Order in v-direction
        unsigned usUCtrlpoints = 6,  // Qty. of the control points in the u-direction
        unsigned usVCtrlpoints = 6
    );  // Qty. of the control points in the v-direction

    virtual ~ParameterCorrection()
    {
        delete _pvcPoints;
        delete _pvcUVParam;
    }

protected:
    /**
     * Calculates the eigenvectors of the covariance matrix
     */
    virtual void CalcEigenvectors();

    /**
     * Projects the control points onto the fit plane
     */
    void ProjectControlPointsOnPlane();

    /**
     * Calculates an initial area at the beginning of the algorithm.
     * For this purpose, the best-fit plane for the point cloud is calculated.
     * The points are calculated with respect to the base consisting of the
     * eigenvectors of the covariance matrix and projected onto the best-fit plane.
     * The bounding box is calculated from these points, then the u/v parameters for
     * the points are calculated.
     */
    virtual bool DoInitialParameterCorrection(double fSizeFactor = 0.0f);

    /**
     * Calculates the (u, v) values of the points
     */
    virtual bool GetUVParameters(double fSizeFactor);

    /**
     * Carries out a parameter correction.
     */
    virtual void DoParameterCorrection(int iIter) = 0;

    /**
     * Solves system of equations
     */
    virtual bool SolveWithoutSmoothing() = 0;

    /**
     * Solve a regular system of equations
     */
    virtual bool SolveWithSmoothing(double fWeight) = 0;

public:
    /**
     * Calculates a B-spline surface from the given points
     */
    virtual Handle(Geom_BSplineSurface) CreateSurface(
        const TColgp_Array1OfPnt& points,
        int iIter,
        bool bParaCor,
        double fSizeFactor = 0.0f
    );
    /**
     * Setting the u/v directions
     * The third parameter specifies whether the directions should actually be used.
     */
    virtual void SetUV(const Base::Vector3d& clU, const Base::Vector3d& clV, bool bUseDir = true);

    /**
     * Returns the u/v/w directions
     */
    virtual void GetUVW(Base::Vector3d& clU, Base::Vector3d& clV, Base::Vector3d& clW) const;

    /**
     * Get the center of gravity
     */
    virtual Base::Vector3d GetGravityPoint() const;

    /**
     * Use smoothing-terms
     */
    virtual void EnableSmoothing(bool bSmooth = true, double fSmoothInfl = 1.0f);

protected:
    bool _bGetUVDir;                           //! Determines whether u/v direction is given
    bool _bSmoothing;                          //! Use smoothing
    double _fSmoothInfluence;                  //! Influence of smoothing
    unsigned _usUOrder;                        //! Order in u-direction
    unsigned _usVOrder;                        //! Order in v-direction
    unsigned _usUCtrlpoints;                   //! Number of control points in the u-direction
    unsigned _usVCtrlpoints;                   //! Number of control points in the v-direction
    Base::Vector3d _clU;                       //! u-direction
    Base::Vector3d _clV;                       //! v-direction
    Base::Vector3d _clW;                       //! w-direction (perpendicular to u & v directions)
    TColgp_Array1OfPnt* _pvcPoints {nullptr};  //! Raw data point list
    TColgp_Array1OfPnt2d* _pvcUVParam {nullptr};  //! Parameter value for the points in the list
    TColgp_Array2OfPnt _vCtrlPntsOfSurf;          //! Array of control points
    TColStd_Array1OfReal _vUKnots;     //! Knot vector of the B-spline surface in the u-direction
    TColStd_Array1OfReal _vVKnots;     //! Knot vector of the B-spline surface in the v-direction
    TColStd_Array1OfInteger _vUMults;  //! Multiplicity of the knots in the knot vector
    TColStd_Array1OfInteger _vVMults;  //! Multiplicity of the knots in the knot vector
};

///////////////////////////////////////////////////////////////////////////////////////////////

/**
 * This class calculates a B-spline area on any point cloud (AKA scattered data).
 * The surface is generated iteratively with the help of a parameter correction.
 * See Hoschek/Lasser 2nd ed. (1992).
 * The approximation is expanded to include smoothing terms so that smooth surfaces
 * can be generated.
 */

class ReenExport BSplineParameterCorrection: public ParameterCorrection
{
public:
    // Constructor
    explicit BSplineParameterCorrection(
        unsigned usUOrder = 4,       // Order in u-direction (order = degree + 1)
        unsigned usVOrder = 4,       // Order in the v-direction
        unsigned usUCtrlpoints = 6,  // Qty. of the control points in u-direction
        unsigned usVCtrlpoints = 6
    );  // Qty. of the control points in v-direction

    ~BSplineParameterCorrection() override = default;

protected:
    /**
     * Initialization
     */
    virtual void Init();

    /**
     * Carries out a parameter correction.
     */
    void DoParameterCorrection(int iIter) override;

    /**
     * Solve an overdetermined LGS with the help of the Householder-Tansformation
     */
    bool SolveWithoutSmoothing() override;

    /**
     * Solve a regular system of equations by LU decomposition. Depending on the weighting,
     * smoothing terms are included
     */
    bool SolveWithSmoothing(double fWeight) override;

public:
    /**
     * Setting the knot vector
     */
    void SetUKnots(const std::vector<double>& afKnots);

    /**
     * Setting the knot vector
     */
    void SetVKnots(const std::vector<double>& afKnots);

    /**
     * Returns the first matrix of smoothing terms, if calculated
     */
    virtual const math_Matrix& GetFirstSmoothMatrix() const;

    /**
     * Returns the second matrix of smoothing terms, if calculated
     */
    virtual const math_Matrix& GetSecondSmoothMatrix() const;

    /**
     * Returns the third matrix of smoothing terms, if calculated
     */
    virtual const math_Matrix& GetThirdSmoothMatrix() const;

    /**
     * Sets the first matrix of the smoothing terms
     */
    virtual void SetFirstSmoothMatrix(const math_Matrix& rclMat);

    /**
     * Sets the second matrix of smoothing terms
     */
    virtual void SetSecondSmoothMatrix(const math_Matrix& rclMat);

    /**
     * Sets the third matrix of smoothing terms
     */
    virtual void SetThirdSmoothMatrix(const math_Matrix& rclMat);

    /**
     * Use smoothing-terms
     */
    void EnableSmoothing(bool bSmooth = true, double fSmoothInfl = 1.0f) override;

    /**
     * Use smoothing-terms
     */
    virtual void EnableSmoothing(bool bSmooth, double fSmoothInfl, double fFirst, double fSec, double fThird);

protected:
    /**
     * Calculates the matrix for the smoothing terms
     * (see U.Dietz dissertation)
     */
    virtual void CalcSmoothingTerms(bool bRecalc, double fFirst, double fSecond, double fThird);

    /**
     * Calculates the matrix for the first smoothing term
     * (see U.Dietz dissertation)
     */
    virtual void CalcFirstSmoothMatrix(Base::SequencerLauncher&);

    /**
     * Calculates the matrix for the second smoothing term
     * (see U.Dietz dissertation)
     */
    virtual void CalcSecondSmoothMatrix(Base::SequencerLauncher&);

    /**
     * Calculates the matrix for the third smoothing term
     */
    virtual void CalcThirdSmoothMatrix(Base::SequencerLauncher&);

protected:
    BSplineBasis _clUSpline;      //! B-spline basic function in the u-direction
    BSplineBasis _clVSpline;      //! B-spline basic function in the v-direction
    math_Matrix _clSmoothMatrix;  //! Matrix of smoothing functionals
    math_Matrix _clFirstMatrix;   //! Matrix of the 1st smoothing functionals
    math_Matrix _clSecondMatrix;  //! Matrix of the 2nd smoothing functionals
    math_Matrix _clThirdMatrix;   //! Matrix of the 3rd smoothing functionals
};

}  // namespace Reen
