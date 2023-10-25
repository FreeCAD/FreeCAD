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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <algorithm>
#include <cstdlib>
#include <iterator>
#endif

#include "SphereFit.h"


using namespace MeshCoreFit;

SphereFit::SphereFit()
    : _vCenter(0, 0, 0)
{}

// Set approximations before calling the fitting
void SphereFit::SetApproximations(double radius, const Base::Vector3d& center)
{
    _bIsFitted = false;
    _fLastResult = FLOAT_MAX;
    _numIter = 0;
    _dRadius = radius;
    _vCenter = center;
}

// Set iteration convergence criteria for the fit if special values are needed.
// The default values set in the constructor are suitable for most uses
void SphereFit::SetConvergenceCriteria(double posConvLimit, double vConvLimit, int maxIter)
{
    if (posConvLimit > 0.0) {
        _posConvLimit = posConvLimit;
    }
    if (vConvLimit > 0.0) {
        _vConvLimit = vConvLimit;
    }
    if (maxIter > 0) {
        _maxIter = maxIter;
    }
}


double SphereFit::GetRadius() const
{
    if (_bIsFitted) {
        return _dRadius;
    }
    else {
        return 0.0;
    }
}

Base::Vector3d SphereFit::GetCenter() const
{
    if (_bIsFitted) {
        return _vCenter;
    }
    else {
        return Base::Vector3d();
    }
}

int SphereFit::GetNumIterations() const
{
    if (_bIsFitted) {
        return _numIter;
    }
    else {
        return 0;
    }
}

float SphereFit::GetDistanceToSphere(const Base::Vector3f& rcPoint) const
{
    float fResult = FLOAT_MAX;
    if (_bIsFitted) {
        fResult = Base::Vector3d((double)rcPoint.x - _vCenter.x,
                                 (double)rcPoint.y - _vCenter.y,
                                 (double)rcPoint.z - _vCenter.z)
                      .Length()
            - _dRadius;
    }
    return fResult;
}

float SphereFit::GetStdDeviation() const
{
    // Mean: M=(1/N)*SUM Xi
    // Variance: VAR=(N/N-1)*[(1/N)*SUM(Xi^2)-M^2]
    // Standard deviation: SD=SQRT(VAR)
    if (!_bIsFitted) {
        return FLOAT_MAX;
    }

    double sumXi = 0.0, sumXi2 = 0.0, dist = 0.0;
    for (auto it : _vPoints) {
        dist = GetDistanceToSphere(it);
        sumXi += dist;
        sumXi2 += (dist * dist);
    }

    double N = static_cast<double>(CountPoints());
    double mean = sumXi / N;
    return sqrt((N / (N - 1.0)) * (sumXi2 / N - mean * mean));
}

void SphereFit::ProjectToSphere()
{
    for (auto& cPnt : _vPoints) {
        // Compute unit vector from sphere centre to point.
        // Because this vector is orthogonal to the sphere's surface at the
        // intersection point we can easily compute the projection point on the
        // closest surface point using the radius of the sphere
        Base::Vector3d diff((double)cPnt.x - _vCenter.x,
                            (double)cPnt.y - _vCenter.y,
                            (double)cPnt.z - _vCenter.z);
        double length = diff.Length();
        if (length == 0.0) {
            // Point is exactly at the sphere center, so it can be projected in any direction onto
            // the sphere! So here just project in +Z direction
            cPnt.z += (float)_dRadius;
        }
        else {
            diff /= length;  // normalizing the vector
            Base::Vector3d proj = _vCenter + diff * _dRadius;
            cPnt.x = (float)proj.x;
            cPnt.y = (float)proj.y;
            cPnt.z = (float)proj.z;
        }
    }
}

// Compute approximations for the parameters using all points:
// Set centre to centre of gravity of points and radius to the average
// distance from the centre of gravity to the points.
void SphereFit::ComputeApproximations()
{
    _bIsFitted = false;
    _fLastResult = FLOAT_MAX;
    _numIter = 0;
    _vCenter.Set(0.0, 0.0, 0.0);
    _dRadius = 0.0;
    if (!_vPoints.empty()) {
        std::list<Base::Vector3f>::const_iterator cIt;
        for (cIt = _vPoints.begin(); cIt != _vPoints.end(); ++cIt) {
            _vCenter.x += cIt->x;
            _vCenter.y += cIt->y;
            _vCenter.z += cIt->z;
        }
        _vCenter /= (double)_vPoints.size();

        for (cIt = _vPoints.begin(); cIt != _vPoints.end(); ++cIt) {
            Base::Vector3d diff((double)cIt->x - _vCenter.x,
                                (double)cIt->y - _vCenter.y,
                                (double)cIt->z - _vCenter.z);
            _dRadius += diff.Length();
        }
        _dRadius /= (double)_vPoints.size();
    }
}

float SphereFit::Fit()
{
    _bIsFitted = false;
    _fLastResult = FLOAT_MAX;
    _numIter = 0;

    // A minimum of 4 surface points is needed to define a sphere
    if (CountPoints() < 4) {
        return FLOAT_MAX;
    }

    // If approximations have not been set/computed then compute some now
    if (_dRadius == 0.0) {
        ComputeApproximations();
    }

    // Initialise some matrices and vectors
    std::vector<Base::Vector3d> residuals(CountPoints(), Base::Vector3d(0.0, 0.0, 0.0));
    Matrix4x4 atpa;
    Eigen::VectorXd atpl(4);

    // Iteration loop...
    double sigma0 {};
    bool cont = true;
    while (cont && (_numIter < _maxIter)) {
        ++_numIter;

        // Set up the quasi parametric normal equations
        setupNormalEquationMatrices(residuals, atpa, atpl);

        // Solve the equations for the unknown corrections
        Eigen::LLT<Matrix4x4> llt(atpa);
        if (llt.info() != Eigen::Success) {
            return FLOAT_MAX;
        }
        Eigen::VectorXd x = llt.solve(atpl);

        // Check parameter convergence (order of parameters: X,Y,Z,R)
        cont = false;
        if ((fabs(x(0)) > _posConvLimit) || (fabs(x(1)) > _posConvLimit)
            || (fabs(x(2)) > _posConvLimit) || (fabs(x(3)) > _posConvLimit)) {
            cont = true;
        }

        // Before updating the unknowns, compute the residuals and sigma0 and check the residual
        // convergence
        bool vConverged {};
        if (!computeResiduals(x, residuals, sigma0, _vConvLimit, vConverged)) {
            return FLOAT_MAX;
        }
        if (!vConverged) {
            cont = true;
        }

        // Update the parameters (order of parameters: X,Y,Z,R)
        _vCenter.x += x(0);
        _vCenter.y += x(1);
        _vCenter.z += x(2);
        _dRadius += x(3);
    }

    // Check for convergence
    if (cont) {
        return FLOAT_MAX;
    }

    _bIsFitted = true;
    _fLastResult = sigma0;

    return _fLastResult;
}

// Set up the normal equation matrices
// atpa ... 4x4 normal matrix
// atpl ... 4x1 matrix (right-hand side of equation)
void SphereFit::setupNormalEquationMatrices(const std::vector<Base::Vector3d>& residuals,
                                            Matrix4x4& atpa,
                                            Eigen::VectorXd& atpl) const
{
    // Zero matrices
    atpa.setZero();
    atpl.setZero();

    // For each point, setup the observation equation coefficients and add their
    // contribution into the normal equation matrices
    double a[4] {}, b[3] {};
    double f0 {}, qw {};
    std::vector<Base::Vector3d>::const_iterator vIt = residuals.begin();
    std::list<Base::Vector3f>::const_iterator cIt;
    for (cIt = _vPoints.begin(); cIt != _vPoints.end(); ++cIt, ++vIt) {
        // if (using this point) { // currently all given points are used (could modify this if
        // eliminating outliers, etc....
        setupObservation(*cIt, *vIt, a, f0, qw, b);
        addObservationU(a, f0, qw, atpa, atpl);
        // }
    }
    setLowerPart(atpa);
}

// Sets up contributions of given observation to the quasi parametric
// normal equation matrices. Assumes uncorrelated coordinates.
// point ... point
// residual ... residual for this point computed from previous iteration (zero for first iteration)
// a[4] ... parameter partials (order of parameters: X,Y,Z,R)
// f0   ... reference to f0 term
// qw   ... reference to quasi weight (here we are assuming equal unit weights for each observed
// point coordinate) b[3] ... observation partials
void SphereFit::setupObservation(const Base::Vector3f& point,
                                 const Base::Vector3d& residual,
                                 double a[4],
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

    // partials of the observations
    double dx = xEstimate - _vCenter.x;
    double dy = yEstimate - _vCenter.y;
    double dz = zEstimate - _vCenter.z;
    b[0] = 2.0 * dx;
    b[1] = 2.0 * dy;
    b[2] = 2.0 * dz;

    // partials of the parameters
    a[0] = -b[0];
    a[1] = -b[1];
    a[2] = -b[2];
    a[3] = -2.0 * _dRadius;

    // free term
    f0 = _dRadius * _dRadius - dx * dx - dy * dy - dz * dz + b[0] * residual.x + b[1] * residual.y
        + b[2] * residual.z;

    // quasi weight (using equal weights for sphere point coordinate observations)
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
// a[4] ... parameter partials
// li   ... free term (f0)
// pi   ... weight of observation (= quasi weight qw for this solution)
// atpa ... 4x4 normal equation matrix
// atpl ... 4x1 matrix/vector (right-hand side of equations)
void SphereFit::addObservationU(double a[4],
                                double li,
                                double pi,
                                Matrix4x4& atpa,
                                Eigen::VectorXd& atpl) const
{
    for (int i = 0; i < 4; ++i) {
        double aipi = a[i] * pi;
        for (int j = i; j < 4; ++j) {
            atpa(i, j) += aipi * a[j];
            // atpa(j, i) = atpa(i, j);	// it's a symmetrical matrix, we'll set this later after all
            // observations processed
        }
        atpl(i) += aipi * li;
    }
}

// Set the lower part of the normal matrix equal to the upper part
// This is done after all the observations have been added
void SphereFit::setLowerPart(Matrix4x4& atpa) const
{
    for (int i = 0; i < 4; ++i) {
        for (int j = i + 1; j < 4; ++j) {  // skip the diagonal elements
            atpa(j, i) = atpa(i, j);
        }
    }
}

// Compute the residuals and sigma0 and check the residual convergence
bool SphereFit::computeResiduals(const Eigen::VectorXd& x,
                                 std::vector<Base::Vector3d>& residuals,
                                 double& sigma0,
                                 double vConvLimit,
                                 bool& vConverged) const
{
    vConverged = true;
    int nPtsUsed = 0;
    sigma0 = 0.0;
    double a[4] {}, b[3] {};
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
        setupObservation(*cIt, v, a, f0, qw, b);
        double qv = -f0;
        for (int i = 0; i < 4; ++i) {
            qv += a[i] * x(i);
        }

        // We are using equal weights for sphere point coordinate observations (see
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
    if (nPtsUsed < 4)  // A minimum of 4 surface points is needed to define a sphere
    {
        sigma0 = 0.0;
        return false;
    }
    int df = nPtsUsed - 4;
    if (df == 0) {
        sigma0 = 0.0;
    }
    else {
        sigma0 = sqrt(sigma0 / (double)df);
    }

    // rmsVv = sqrt(rmsVv / (double)nPtsUsed);
    // Base::Console().Message("X: %0.3e %0.3e %0.3e %0.3e , Max dV: %0.4f %0.4f %0.4f , RMS Vv:
    // %0.4f\n", x(0), x(1), x(2), x(3), maxdVx, maxdVy, maxdVz, rmsVv);

    return true;
}
