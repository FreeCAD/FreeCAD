// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include "Approximation.h"
#include <Eigen/Eigenvalues>


// -------------------------------------------------------------------------------
namespace MeshCoreFit
{

using Matrix4x4 = Eigen::Matrix<double, 4, 4, Eigen::RowMajor>;

/**
 * Best-fit sphere for a given set of points.
 */
class MeshExport SphereFit: public MeshCore::Approximation
{
public:
    /**
     * Construction
     */
    SphereFit();

    /**
     * Set approximations before calling Fit()
     */
    void SetApproximations(double radius, const Base::Vector3d& center);
    /**
     * Set iteration convergence criteria for the fit if special values are needed.
     * The default values set in the constructor are suitable for most uses
     */
    void SetConvergenceCriteria(double posConvLimit, double vConvLimit, int maxIter);
    /**
     * Returns the radius of the fitted sphere. If Fit() has not been called then zero is returned.
     */
    double GetRadius() const;
    /**
     * Returns the center of the fitted sphere. If Fit() has not been called the null vector is
     * returned.
     */
    Base::Vector3d GetCenter() const;
    /**
     * Returns the number of iterations that Fit() needed to converge. If Fit() has not been called
     * then zero is returned.
     */
    int GetNumIterations() const;
    /**
     * Compute approximations for the parameters using all points
     */
    void ComputeApproximations();
    /**
     * Fit a sphere onto the given points. If the fit fails FLOAT_MAX is returned.
     */
    float Fit() override;
    /**
     * Returns the distance from the point \a rcPoint to the fitted sphere. If Fit() has not been
     * called FLOAT_MAX is returned.
     */
    float GetDistanceToSphere(const Base::Vector3f& rcPoint) const;
    /**
     * Returns the standard deviation from the points to the fitted sphere. If Fit() has not been
     * called FLOAT_MAX is returned.
     */
    float GetStdDeviation() const;
    /**
     * Projects the points onto the fitted sphere.
     */
    void ProjectToSphere();

protected:
    /**
     * Set up the normal equations
     */
    void setupNormalEquationMatrices(
        const std::vector<Base::Vector3d>& residuals,
        Matrix4x4& atpa,
        Eigen::VectorXd& atpl
    ) const;
    /**
     * Sets up contributions of given observation to the normal equation matrices.
     */
    void setupObservation(
        const Base::Vector3f& point,
        const Base::Vector3d& residual,
        double a[4],
        double& f0,
        double& qw,
        double b[3]
    ) const;
    /**
     * Computes contribution of the given observation equation on the normal equation matrices
     */
    void addObservationU(double a[4], double li, double pi, Matrix4x4& atpa, Eigen::VectorXd& atpl) const;
    /**
     * Set the lower part of the normal matrix equal to the upper part
     */
    void setLowerPart(Matrix4x4& atpa) const;

    /**
     * Compute the residuals and sigma0 and check the residual convergence
     */
    bool computeResiduals(
        const Eigen::VectorXd& x,
        std::vector<Base::Vector3d>& residuals,
        double& sigma0,
        double vConvLimit,
        bool& vConverged
    ) const;

private:
    Base::Vector3d _vCenter;       /**< Center of sphere. */
    double _dRadius {0};           /**< Radius of the sphere. */
    int _numIter {0};              /**< Number of iterations for solution to converge. */
    double _posConvLimit {0.0001}; /**< Position and radius parameter convergence threshold. */
    double _vConvLimit {0.001};    /**< Residual convergence threshold. */
    int _maxIter {50};             /**< Maximum number of iterations. */
};


}  // namespace MeshCoreFit
