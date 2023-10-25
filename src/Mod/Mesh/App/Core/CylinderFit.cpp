/***************************************************************************
 *   Copyright (c) 2020 Graeme van der Vlugt                               *
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

// Definitions:
// Cylinder axis goes through a point (Xc,Yc,Zc) and has direction (L,M,N)
// Cylinder radius is R
// A point on the axis (X0i,Y0i,Z0i) can be described by:
//	(X0i,Y0i,Z0i) = (Xc,Yc,Zc) + s(L,M,N)
// where s is the distance from (Xc,Yc,Zc) to (X0i,Y0i,Z0i) when (L,M,N) is
// of unit length (normalized).
// The distance between a cylinder surface point (Xi,Yi,Zi) and its
// projection onto the axis (X0i,Y0i,Z0i) is the radius:
// (Xi - X0i)^2 + (Yi - Y0i)^2 + (Zi - Z0i)^2 = R^2
// Also the vector to a cylinder surface point (Xi,Yi,Zi) from its
// projection onto the axis (X0i,Y0i,Z0i) is orthogonal to the axis so we can
// write:
// (Xi - X0i, Yi - Y0i, Zi - Z0i).(L,M,N) = 0 or
// L(Xi - X0i) + M(Yi - Y0i) + N(Zi - Z0i) = 0
// If we substitute these various equations into each other and further add
// the constraint that L^2 + M^2 + N^2 = 1 then we can arrive at a single
// equation for the cylinder surface points:
// (Xi - Xc + L*L*(Xc - Xi) + L*M*(Yc - Yi) + L*N*(Zc - Zi))^2 +
// (Yi - Yc + M*L*(Xc - Xi) + M*M*(Yc - Yi) + M*N*(Zc - Zi))^2 +
// (Zi - Zc + N*L*(Xc - Xi) + N*M*(Yc - Yi) + N*N*(Zc - Zi))^2 - R^2 = 0
// This equation is what is used in the least squares solution below. Because
// we are constraining the direction vector to a unit length and also because
// we need to stop the axis point from moving along the axis we need to fix one
// of the ordinates in the solution. So from our initial approximations for the
// axis direction (L0,M0,N0):
//      if (L0 > M0) && (L0 > N0) then fix Xc = Mx and use L = sqrt(1 - M^2 - N^2)
// else if (M0 > L0) && (M0 > N0) then fix Yc = My and use M = sqrt(1 - L^2 - N^2)
// else if (N0 > L0) && (N0 > M0) then fix Zc = Mz and use N = sqrt(1 - L^2 - M^2)
// where (Mx,My,Mz) is the mean of the input points (centre of gravity)
// We thus solve for 5 unknown parameters.
// Thus for the solution to succeed the initial axis direction should be reasonable.

#include "PreCompiled.h"

#ifndef _PreComp_
#include <algorithm>
#include <cstdlib>
#include <iterator>
#endif

#include <Base/Console.h>
#include <Mod/Mesh/App/WildMagic4/Wm4ApprLineFit3.h>

#include "CylinderFit.h"


using namespace MeshCoreFit;

CylinderFit::CylinderFit()
    : _vBase(0, 0, 0)
    , _vAxis(0, 0, 1)
{}

// Set approximations before calling the fitting
void CylinderFit::SetApproximations(double radius,
                                    const Base::Vector3d& base,
                                    const Base::Vector3d& axis)
{
    _bIsFitted = false;
    _fLastResult = FLOAT_MAX;
    _numIter = 0;
    _dRadius = radius;
    _vBase = base;
    _vAxis = axis;
    _vAxis.Normalize();
}

// Set approximations before calling the fitting. This version computes the radius
// using the given axis and the existing surface points (which must already be added!)
void CylinderFit::SetApproximations(const Base::Vector3d& base, const Base::Vector3d& axis)
{
    _bIsFitted = false;
    _fLastResult = FLOAT_MAX;
    _numIter = 0;
    _vBase = base;
    _vAxis = axis;
    _vAxis.Normalize();
    _dRadius = 0.0;
    if (!_vPoints.empty()) {
        for (std::list<Base::Vector3f>::const_iterator cIt = _vPoints.begin();
             cIt != _vPoints.end();
             ++cIt) {
            _dRadius += Base::Vector3d(cIt->x, cIt->y, cIt->z).DistanceToLine(_vBase, _vAxis);
        }
        _dRadius /= (double)_vPoints.size();
    }
}

// Set iteration convergence criteria for the fit if special values are needed.
// The default values set in the constructor are suitable for most uses
void CylinderFit::SetConvergenceCriteria(double posConvLimit,
                                         double dirConvLimit,
                                         double vConvLimit,
                                         int maxIter)
{
    if (posConvLimit > 0.0) {
        _posConvLimit = posConvLimit;
    }
    if (dirConvLimit > 0.0) {
        _dirConvLimit = dirConvLimit;
    }
    if (vConvLimit > 0.0) {
        _vConvLimit = vConvLimit;
    }
    if (maxIter > 0) {
        _maxIter = maxIter;
    }
}


double CylinderFit::GetRadius() const
{
    if (_bIsFitted) {
        return _dRadius;
    }
    else {
        return 0.0;
    }
}

Base::Vector3d CylinderFit::GetBase() const
{
    if (_bIsFitted) {
        return _vBase;
    }
    else {
        return Base::Vector3d();
    }
}

Base::Vector3d CylinderFit::GetAxis() const
{
    if (_bIsFitted) {
        return _vAxis;
    }
    else {
        return Base::Vector3d();
    }
}

int CylinderFit::GetNumIterations() const
{
    if (_bIsFitted) {
        return _numIter;
    }
    else {
        return 0;
    }
}

float CylinderFit::GetDistanceToCylinder(const Base::Vector3f& rcPoint) const
{
    float fResult = FLOAT_MAX;
    if (_bIsFitted) {
        fResult = Base::Vector3d(rcPoint.x, rcPoint.y, rcPoint.z).DistanceToLine(_vBase, _vAxis)
            - _dRadius;
    }
    return fResult;
}

float CylinderFit::GetStdDeviation() const
{
    // Mean: M=(1/N)*SUM Xi
    // Variance: VAR=(N/N-1)*[(1/N)*SUM(Xi^2)-M^2]
    // Standard deviation: SD=SQRT(VAR)
    if (!_bIsFitted) {
        return FLOAT_MAX;
    }

    double sumXi = 0.0, sumXi2 = 0.0, dist = 0.0;
    for (auto it : _vPoints) {
        dist = GetDistanceToCylinder(it);
        sumXi += dist;
        sumXi2 += (dist * dist);
    }

    double N = static_cast<double>(CountPoints());
    double mean = sumXi / N;
    return sqrt((N / (N - 1.0)) * (sumXi2 / N - mean * mean));
}

void CylinderFit::ProjectToCylinder()
{
    Base::Vector3f cBase(_vBase.x, _vBase.y, _vBase.z);
    Base::Vector3f cAxis(_vAxis.x, _vAxis.y, _vAxis.z);

    for (auto& cPnt : _vPoints) {
        if (cPnt.DistanceToLine(cBase, cAxis) > 0) {
            Base::Vector3f proj;
            cBase.ProjectToPlane(cPnt, cAxis, proj);
            Base::Vector3f diff = cPnt - proj;
            diff.Normalize();
            cPnt = proj + diff * _dRadius;
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
            cPnt = proj + diff * _dRadius;
        }
    }
}

// Compute approximations for the parameters using all points by computing a
// line through the points. This doesn't work well if the points are only from
// one small surface area.
// In that case rather use SetApproximations() with a better estimate.
void CylinderFit::ComputeApproximationsLine()
{
    _bIsFitted = false;
    _fLastResult = FLOAT_MAX;
    _numIter = 0;
    _vBase.Set(0.0, 0.0, 0.0);
    _vAxis.Set(0.0, 0.0, 0.0);
    _dRadius = 0.0;
    if (!_vPoints.empty()) {
        std::vector<Wm4::Vector3d> input;
        std::transform(_vPoints.begin(),
                       _vPoints.end(),
                       std::back_inserter(input),
                       [](const Base::Vector3f& v) {
                           return Wm4::Vector3d(v.x, v.y, v.z);
                       });
        Wm4::Line3<double> kLine = Wm4::OrthogonalLineFit3(input.size(), input.data());
        _vBase.Set(kLine.Origin.X(), kLine.Origin.Y(), kLine.Origin.Z());
        _vAxis.Set(kLine.Direction.X(), kLine.Direction.Y(), kLine.Direction.Z());

        for (std::list<Base::Vector3f>::const_iterator cIt = _vPoints.begin();
             cIt != _vPoints.end();
             ++cIt) {
            _dRadius += Base::Vector3d(cIt->x, cIt->y, cIt->z).DistanceToLine(_vBase, _vAxis);
        }
        _dRadius /= (double)_vPoints.size();
    }
}

float CylinderFit::Fit()
{
    _bIsFitted = false;
    _fLastResult = FLOAT_MAX;
    _numIter = 0;

    // A minimum of 5 surface points is needed to define a cylinder
    if (CountPoints() < 5) {
        return FLOAT_MAX;
    }

    // If approximations have not been set/computed then compute some now using the line fit method
    if (_dRadius == 0.0) {
        ComputeApproximationsLine();
    }

    // Check parameters to define the best solution direction
    // There are 7 parameters but 2 are actually dependent on the others
    // so we are actually solving for 5 parameters.
    // order of parameters depending on the solution direction:
    //		solL:	Yc, Zc, M, N, R
    //		solM:	Xc, Zc, L, N, R
    //		solN:	Xc, Yc, L, M, R
    SolutionD solDir {};
    findBestSolDirection(solDir);

    // Initialise some matrices and vectors
    std::vector<Base::Vector3d> residuals(CountPoints(), Base::Vector3d(0.0, 0.0, 0.0));
    Matrix5x5 atpa;
    Eigen::VectorXd atpl(5);

    // Iteration loop...
    double sigma0 {};
    bool cont = true;
    while (cont && (_numIter < _maxIter)) {
        ++_numIter;

        // Set up the quasi parametric normal equations
        setupNormalEquationMatrices(solDir, residuals, atpa, atpl);

        // Solve the equations for the unknown corrections
        Eigen::LLT<Matrix5x5> llt(atpa);
        if (llt.info() != Eigen::Success) {
            return FLOAT_MAX;
        }
        Eigen::VectorXd x = llt.solve(atpl);

        // Check parameter convergence
        cont = false;
        if ((fabs(x(0)) > _posConvLimit) || (fabs(x(1)) > _posConvLimit)
            ||  // the two position parameter corrections
            (fabs(x(2)) > _dirConvLimit) || (fabs(x(3)) > _dirConvLimit)
            ||                               // the two direction parameter corrections
            (fabs(x(4)) > _posConvLimit)) {  // the radius correction
            cont = true;
        }

        // Before updating the unknowns, compute the residuals and sigma0 and check the residual
        // convergence
        bool vConverged {};
        if (!computeResiduals(solDir, x, residuals, sigma0, _vConvLimit, vConverged)) {
            return FLOAT_MAX;
        }
        if (!vConverged) {
            cont = true;
        }

        // Update the parameters
        if (!updateParameters(solDir, x)) {
            return FLOAT_MAX;
        }
    }

    // Check for convergence
    if (cont) {
        return FLOAT_MAX;
    }

    _bIsFitted = true;
    _fLastResult = sigma0;

    return _fLastResult;
}

// Checks initial parameter values and defines the best solution direction to use
// Axis direction = (L,M,N)
// solution L: L is biggest axis component and L = f(M,N) and X = Mx (we move the base point along
// axis to this x-value) solution M: M is biggest axis component and M = f(L,N) and Y = My (we move
// the base point along axis to this y-value) solution N: N is biggest axis component and N = f(L,M)
// and Z = Mz (we move the base point along axis to this z-value) where (Mx,My,Mz) is the mean of
// the input points (centre of gravity)
void CylinderFit::findBestSolDirection(SolutionD& solDir)
{
    // Choose the best of the three solution 'directions' to use
    // This is to avoid a square root of a negative number when computing the dependent parameters
    Base::Vector3d dir = _vAxis;
    Base::Vector3d pos = _vBase;
    dir.Normalize();
    double biggest = dir.x;
    solDir = solL;
    if (fabs(dir.y) > fabs(biggest)) {
        biggest = dir.y;
        solDir = solM;
    }
    if (fabs(dir.z) > fabs(biggest)) {
        biggest = dir.z;
        solDir = solN;
    }
    if (biggest < 0.0) {
        dir.Set(-dir.x, -dir.y, -dir.z);  // multiplies by -1
    }

    double fixedVal = 0.0;
    double lambda;
    switch (solDir) {
        case solL:
            fixedVal = meanXObs();
            lambda = (fixedVal - pos.x) / dir.x;
            pos.x = fixedVal;
            pos.y = pos.y + lambda * dir.y;
            pos.z = pos.z + lambda * dir.z;
            break;
        case solM:
            fixedVal = meanYObs();
            lambda = (fixedVal - pos.y) / dir.y;
            pos.x = pos.x + lambda * dir.x;
            pos.y = fixedVal;
            pos.z = pos.z + lambda * dir.z;
            break;
        case solN:
            fixedVal = meanZObs();
            lambda = (fixedVal - pos.z) / dir.z;
            pos.x = pos.x + lambda * dir.x;
            pos.y = pos.y + lambda * dir.y;
            pos.z = fixedVal;
            break;
    }
    _vAxis = dir;
    _vBase = pos;
}

double CylinderFit::meanXObs()
{
    double mx = 0.0;
    if (!_vPoints.empty()) {
        for (std::list<Base::Vector3f>::const_iterator cIt = _vPoints.begin();
             cIt != _vPoints.end();
             ++cIt) {
            mx += cIt->x;
        }
        mx /= double(_vPoints.size());
    }
    return mx;
}

double CylinderFit::meanYObs()
{
    double my = 0.0;
    if (!_vPoints.empty()) {
        for (std::list<Base::Vector3f>::const_iterator cIt = _vPoints.begin();
             cIt != _vPoints.end();
             ++cIt) {
            my += cIt->y;
        }
        my /= double(_vPoints.size());
    }
    return my;
}

double CylinderFit::meanZObs()
{
    double mz = 0.0;
    if (!_vPoints.empty()) {
        for (std::list<Base::Vector3f>::const_iterator cIt = _vPoints.begin();
             cIt != _vPoints.end();
             ++cIt) {
            mz += cIt->z;
        }
        mz /= double(_vPoints.size());
    }
    return mz;
}

// Set up the normal equation matrices
// atpa ... 5x5 normal matrix
// atpl ... 5x1 matrix (right-hand side of equation)
void CylinderFit::setupNormalEquationMatrices(SolutionD solDir,
                                              const std::vector<Base::Vector3d>& residuals,
                                              Matrix5x5& atpa,
                                              Eigen::VectorXd& atpl) const
{
    // Zero matrices
    atpa.setZero();
    atpl.setZero();

    // For each point, setup the observation equation coefficients and add their
    // contribution into the normal equation matrices
    double a[5] {}, b[3] {};
    double f0 {}, qw {};
    std::vector<Base::Vector3d>::const_iterator vIt = residuals.begin();
    std::list<Base::Vector3f>::const_iterator cIt;
    for (cIt = _vPoints.begin(); cIt != _vPoints.end(); ++cIt, ++vIt) {
        // if (using this point) { // currently all given points are used (could modify this if
        // eliminating outliers, etc....
        setupObservation(solDir, *cIt, *vIt, a, f0, qw, b);
        addObservationU(a, f0, qw, atpa, atpl);
        // }
    }
    setLowerPart(atpa);
}

// Sets up contributions of given observation to the quasi parametric
// normal equation matrices. Assumes uncorrelated coordinates.
// point ... point
// residual ... residual for this point computed from previous iteration (zero for first iteration)
// a[5] ... parameter partials
// f0   ... reference to f0 term
// qw   ... reference to quasi weight (here we are assuming equal unit weights for each observed
// point coordinate) b[3] ... observation partials
void CylinderFit::setupObservation(SolutionD solDir,
                                   const Base::Vector3f& point,
                                   const Base::Vector3d& residual,
                                   double a[5],
                                   double& f0,
                                   double& qw,
                                   double b[3]) const
{
    // This adjustment requires an update of the observation approximations
    // because the residuals do not have a linear relationship.
    // New estimates for the observations:
    double xEstimate = (double)point.x + residual.x;
    double yEstimate = (double)point.y + residual.y;
    double zEstimate = (double)point.z + residual.z;

    // intermediate parameters
    double lambda = _vAxis.x * (xEstimate - _vBase.x) + _vAxis.y * (yEstimate - _vBase.y)
        + _vAxis.z * (zEstimate - _vBase.z);
    double x0 = _vBase.x + lambda * _vAxis.x;
    double y0 = _vBase.y + lambda * _vAxis.y;
    double z0 = _vBase.z + lambda * _vAxis.z;
    double dx = xEstimate - x0;
    double dy = yEstimate - y0;
    double dz = zEstimate - z0;
    double dx00 = _vBase.x - xEstimate;
    double dy00 = _vBase.y - yEstimate;
    double dz00 = _vBase.z - zEstimate;

    // partials of the observations
    b[0] =
        2.0 * (dx - _vAxis.x * _vAxis.x * dx - _vAxis.x * _vAxis.y * dy - _vAxis.x * _vAxis.z * dz);
    b[1] =
        2.0 * (dy - _vAxis.x * _vAxis.y * dx - _vAxis.y * _vAxis.y * dy - _vAxis.y * _vAxis.z * dz);
    b[2] =
        2.0 * (dz - _vAxis.x * _vAxis.z * dx - _vAxis.y * _vAxis.z * dy - _vAxis.z * _vAxis.z * dz);

    double ddxdl {}, ddydl {}, ddzdl {};
    double ddxdm {}, ddydm {}, ddzdm {};
    double ddxdn {}, ddydn {}, ddzdn {};

    // partials of the parameters
    switch (solDir) {
        case solL:
            // order of parameters: Yc, Zc, M, N, R
            ddxdm = -2.0 * _vAxis.y * dx00 + (_vAxis.x - _vAxis.y * _vAxis.y / _vAxis.x) * dy00
                - (_vAxis.y * _vAxis.z / _vAxis.x) * dz00;
            ddydm = (_vAxis.x - _vAxis.y * _vAxis.y / _vAxis.x) * dx00 + 2.0 * _vAxis.y * dy00
                + _vAxis.z * dz00;
            ddzdm = -(_vAxis.y * _vAxis.z / _vAxis.x) * dx00 + _vAxis.z * dy00;
            ddxdn = -2.0 * _vAxis.z * dx00 - (_vAxis.y * _vAxis.z / _vAxis.x) * dy00
                + (_vAxis.x - _vAxis.z * _vAxis.z / _vAxis.x) * dz00;
            ddydn = -(_vAxis.y * _vAxis.z / _vAxis.x) * dx00 + _vAxis.y * dz00;
            ddzdn = (_vAxis.x - _vAxis.z * _vAxis.z / _vAxis.x) * dx00 + _vAxis.y * dy00
                + 2.0 * _vAxis.z * dz00;
            a[0] = -b[1];
            a[1] = -b[2];
            a[2] = 2.0 * (dx * ddxdm + dy * ddydm + dz * ddzdm);
            a[3] = 2.0 * (dx * ddxdn + dy * ddydn + dz * ddzdn);
            a[4] = -2.0 * _dRadius;
            break;
        case solM:
            // order of parameters: Xc, Zc, L, N, R
            ddxdl = 2.0 * _vAxis.x * dx00 + (_vAxis.y - _vAxis.x * _vAxis.x / _vAxis.y) * dy00
                + _vAxis.z * dz00;
            ddydl = (_vAxis.y - _vAxis.x * _vAxis.x / _vAxis.y) * dx00 - 2.0 * _vAxis.x * dy00
                - (_vAxis.x * _vAxis.z / _vAxis.y) * dz00;
            ddzdl = _vAxis.z * dx00 - (_vAxis.x * _vAxis.z / _vAxis.y) * dy00;
            ddxdn = -(_vAxis.x * _vAxis.z / _vAxis.y) * dy00 + _vAxis.x * dz00;
            ddydn = -(_vAxis.x * _vAxis.z / _vAxis.y) * dx00 - 2.0 * _vAxis.z * dy00
                + (_vAxis.y - _vAxis.z * _vAxis.z / _vAxis.y) * dz00;
            ddzdn = _vAxis.x * dx00 + (_vAxis.y - _vAxis.z * _vAxis.z / _vAxis.y) * dy00
                + 2.0 * _vAxis.z * dz00;
            a[0] = -b[0];
            a[1] = -b[2];
            a[2] = 2.0 * (dx * ddxdl + dy * ddydl + dz * ddzdl);
            a[3] = 2.0 * (dx * ddxdn + dy * ddydn + dz * ddzdn);
            a[4] = -2.0 * _dRadius;
            break;
        case solN:
            // order of parameters: Xc, Yc, L, M, R
            ddxdl = 2.0 * _vAxis.x * dx00 + _vAxis.y * dy00
                + (_vAxis.z - _vAxis.x * _vAxis.x / _vAxis.z) * dz00;
            ddydl = _vAxis.y * dx00 - (_vAxis.x * _vAxis.y / _vAxis.z) * dz00;
            ddzdl = (_vAxis.z - _vAxis.x * _vAxis.x / _vAxis.z) * dx00
                - (_vAxis.x * _vAxis.y / _vAxis.z) * dy00 - 2.0 * _vAxis.x * dz00;
            ddxdm = _vAxis.x * dy00 - (_vAxis.x * _vAxis.y / _vAxis.z) * dz00;
            ddydm = _vAxis.x * dx00 + 2.0 * _vAxis.y * dy00
                + (_vAxis.z - _vAxis.y * _vAxis.y / _vAxis.z) * dz00;
            ddzdm = -(_vAxis.x * _vAxis.y / _vAxis.z) * dx00
                + (_vAxis.z - _vAxis.y * _vAxis.y / _vAxis.z) * dy00 - 2.0 * _vAxis.y * dz00;
            a[0] = -b[0];
            a[1] = -b[1];
            a[2] = 2.0 * (dx * ddxdl + dy * ddydl + dz * ddzdl);
            a[3] = 2.0 * (dx * ddxdm + dy * ddydm + dz * ddzdm);
            a[4] = -2.0 * _dRadius;
            break;
    }

    // free term
    f0 = _dRadius * _dRadius - dx * dx - dy * dy - dz * dz + b[0] * residual.x + b[1] * residual.y
        + b[2] * residual.z;

    // quasi weight (using equal weights for cylinder point coordinate observations)
    // w[0] = 1.0;
    // w[1] = 1.0;
    // w[2] = 1.0;
    // qw = 1.0 / (b[0] * b[0] / w[0] + b[1] * b[1] / w[1] + b[2] * b[2] / w[2]);
    qw = 1.0 / (b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
}

// Computes contribution of the given observation equation on the normal equation matrices
// Call this for each observation (point)
// Here we only add the contribution to  the upper part of the normal matrix
// and then after all observations have been added we need to set the lower part
// (which is symmetrical to the upper part)
// a[5] ... parameter partials
// li   ... free term (f0)
// pi   ... weight of observation (= quasi weight qw for this solution)
// atpa ... 5x5 normal equation matrix
// atpl ... 5x1 matrix/vector (right-hand side of equations)
void CylinderFit::addObservationU(double a[5],
                                  double li,
                                  double pi,
                                  Matrix5x5& atpa,
                                  Eigen::VectorXd& atpl) const
{
    for (int i = 0; i < 5; ++i) {
        double aipi = a[i] * pi;
        for (int j = i; j < 5; ++j) {
            atpa(i, j) += aipi * a[j];
            // atpa(j, i) = atpa(i, j);	// it's a symmetrical matrix, we'll set this later after all
            // observations processed
        }
        atpl(i) += aipi * li;
    }
}

// Set the lower part of the normal matrix equal to the upper part
// This is done after all the observations have been added
void CylinderFit::setLowerPart(Matrix5x5& atpa) const
{
    for (int i = 0; i < 5; ++i) {
        for (int j = i + 1; j < 5; ++j) {  // skip the diagonal elements
            atpa(j, i) = atpa(i, j);
        }
    }
}

// Compute the residuals and sigma0 and check the residual convergence
bool CylinderFit::computeResiduals(SolutionD solDir,
                                   const Eigen::VectorXd& x,
                                   std::vector<Base::Vector3d>& residuals,
                                   double& sigma0,
                                   double vConvLimit,
                                   bool& vConverged) const
{
    vConverged = true;
    int nPtsUsed = 0;
    sigma0 = 0.0;
    double a[5] {}, b[3] {};
    double f0 {}, qw {};
    // double maxdVx = 0.0;
    // double maxdVy = 0.0;
    // double maxdVz = 0.0;
    // double rmsVv = 0.0;
    std::vector<Base::Vector3d>::iterator vIt = residuals.begin();
    std::list<Base::Vector3f>::const_iterator cIt;
    for (cIt = _vPoints.begin(); cIt != _vPoints.end(); ++cIt, ++vIt) {
        // if (using this point) { // currently all given points are used (could modify this if
        // eliminating outliers, etc....
        ++nPtsUsed;
        Base::Vector3d& v = *vIt;
        setupObservation(solDir, *cIt, v, a, f0, qw, b);
        double qv = -f0;
        for (int i = 0; i < 5; ++i) {
            qv += a[i] * x(i);
        }

        // We are using equal weights for cylinder point coordinate observations (see
        // setupObservation) i.e. w[0] = w[1] = w[2] = 1.0;
        // double vx = -qw * qv * b[0] / w[0];
        // double vy = -qw * qv * b[1] / w[1];
        // double vz = -qw * qv * b[2] / w[2];
        double vx = -qw * qv * b[0];
        double vy = -qw * qv * b[1];
        double vz = -qw * qv * b[2];
        double dVx = fabs(vx - v.x);
        double dVy = fabs(vy - v.y);
        double dVz = fabs(vz - v.z);
        v.x = vx;
        v.y = vy;
        v.z = vz;

        // double vv = v.x * v.x + v.y * v.y + v.z * v.z;
        // rmsVv += vv * vv;

        // sigma0 += v.x * w[0] * v.x + v.y * w[1] * v.y + v.z * w[2] * v.z;
        sigma0 += v.x * v.x + v.y * v.y + v.z * v.z;

        if ((dVx > vConvLimit) || (dVy > vConvLimit) || (dVz > vConvLimit)) {
            vConverged = false;
        }

        // if (dVx > maxdVx)
        //	maxdVx = dVx;
        // if (dVy > maxdVy)
        //	maxdVy = dVy;
        // if (dVz > maxdVz)
        //	maxdVz = dVz;
    }

    // Compute degrees of freedom and sigma0
    if (nPtsUsed < 5)  // A minimum of 5 surface points is needed to define a cylinder
    {
        sigma0 = 0.0;
        return false;
    }
    int df = nPtsUsed - 5;
    if (df == 0) {
        sigma0 = 0.0;
    }
    else {
        sigma0 = sqrt(sigma0 / (double)df);
    }

    // rmsVv = sqrt(rmsVv / (double)nPtsUsed);
    // Base::Console().Message("X: %0.3e %0.3e %0.3e %0.3e %0.3e , Max dV: %0.4f %0.4f %0.4f , RMS
    // Vv: %0.4f\n", x(0), x(1), x(2), x(3), x(4), maxdVx, maxdVy, maxdVz, rmsVv);

    return true;
}

// Update the parameters after solving the normal equations
bool CylinderFit::updateParameters(SolutionD solDir, const Eigen::VectorXd& x)
{
    // Update the parameters used as unknowns in the solution
    switch (solDir) {
        case solL:  // order of parameters: Yc, Zc, M, N, R
            _vBase.y += x(0);
            _vBase.z += x(1);
            _vAxis.y += x(2);
            _vAxis.z += x(3);
            _dRadius += x(4);
            break;
        case solM:  // order of parameters: Xc, Zc, L, N, R
            _vBase.x += x(0);
            _vBase.z += x(1);
            _vAxis.x += x(2);
            _vAxis.z += x(3);
            _dRadius += x(4);
            break;
        case solN:  // order of parameters: Xc, Yc, L, M, R
            _vBase.x += x(0);
            _vBase.y += x(1);
            _vAxis.x += x(2);
            _vAxis.y += x(3);
            _dRadius += x(4);
            break;
    }

    // Update the dependent axis direction parameter
    double l2 {}, m2 {}, n2 {};
    switch (solDir) {
        case solL:
            l2 = 1.0 - _vAxis.y * _vAxis.y - _vAxis.z * _vAxis.z;
            if (l2 <= 0.0) {
                return false;  // L*L <= 0 !
            }
            _vAxis.x = sqrt(l2);
            //_vBase.x is fixed
            break;
        case solM:
            m2 = 1.0 - _vAxis.x * _vAxis.x - _vAxis.z * _vAxis.z;
            if (m2 <= 0.0) {
                return false;  // M*M <= 0 !
            }
            _vAxis.y = sqrt(m2);
            //_vBase.y is fixed
            break;
        case solN:
            n2 = 1.0 - _vAxis.x * _vAxis.x - _vAxis.y * _vAxis.y;
            if (n2 <= 0.0) {
                return false;  // N*N <= 0 !
            }
            _vAxis.z = sqrt(n2);
            //_vBase.z is fixed
            break;
    }

    return true;
}
