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

#ifndef MESH_CYLINDER_FIT_H
#define MESH_CYLINDER_FIT_H

#include <Eigen/Eigenvalues>

#include "Approximation.h"


// -------------------------------------------------------------------------------
namespace MeshCoreFit
{

using Matrix5x5 = Eigen::Matrix<double, 5, 5, Eigen::RowMajor>;

/**
 * Best-fit cylinder for a given set of points.
 * Doesn't expect points on any top or bottom end-planes, only points on the side surface
 */
class MeshExport CylinderFit: public MeshCore::Approximation
{
protected:
    // Solution 'direction' enumeration
    enum SolutionD
    {
        solL = 0,  // solution L: L is biggest axis component and L = f(M,N)
        solM = 1,  // solution M: M is biggest axis component and M = f(L,N)
        solN = 2   // solution N: N is biggest axis component and N = f(L,M)
    };

public:
    /**
     * Construction
     */
    CylinderFit();
    /**
     * Set approximations before calling Fit()
     */
    void SetApproximations(double radius, const Base::Vector3d& base, const Base::Vector3d& axis);
    /**
     * Set approximations before calling Fit(). This version computes the radius
     * using the given axis and the existing surface points.
     */
    void SetApproximations(const Base::Vector3d& base, const Base::Vector3d& axis);
    /**
     * Set iteration convergence criteria for the fit if special values are needed.
     * The default values set in the constructor are suitable for most uses
     */
    void SetConvergenceCriteria(double posConvLimit,
                                double dirConvLimit,
                                double vConvLimit,
                                int maxIter);
    /**
     * Returns the radius of the fitted cylinder. If Fit() has not been called then zero is
     * returned.
     */
    double GetRadius() const;
    /**
     * Returns the base of the fitted cylinder. If Fit() has not been called the null vector is
     * returned.
     */
    Base::Vector3d GetBase() const;
    /**
     * Returns the axis of the fitted cylinder. If Fit() has not been called the null vector is
     * returned.
     */
    Base::Vector3d GetAxis() const;
    /**
     * Returns the number of iterations that Fit() needed to converge. If Fit() has not been called
     * then zero is returned.
     */
    int GetNumIterations() const;
    /**
     * Fit a cylinder into the given points. If the fit fails FLOAT_MAX is returned.
     */
    float Fit() override;
    /**
     * Returns the distance from the point \a rcPoint to the fitted cylinder. If Fit() has not been
     * called FLOAT_MAX is returned.
     */
    float GetDistanceToCylinder(const Base::Vector3f& rcPoint) const;
    /**
     * Returns the standard deviation from the points to the fitted cylinder. If Fit() has not been
     * called FLOAT_MAX is returned.
     */
    float GetStdDeviation() const;
    /**
     * Projects the points onto the fitted cylinder.
     */
    void ProjectToCylinder();

protected:
    /**
     * Compute approximations for the parameters using all points using the line fit method
     */
    void ComputeApproximationsLine();
    /**
     * Checks initial parameter values and defines the best solution direction to use
     */
    void findBestSolDirection(SolutionD& solDir);
    /**
     * Compute the mean X-value of all of the points (observed/input surface points)
     */
    double meanXObs();
    /**
     * Compute the mean Y-value of all of the points (observed/input surface points)
     */
    double meanYObs();
    /**
     * Compute the mean Z-value of all of the points (observed/input surface points)
     */
    double meanZObs();
    /**
     * Set up the normal equations
     */
    void setupNormalEquationMatrices(SolutionD solDir,
                                     const std::vector<Base::Vector3d>& residuals,
                                     Matrix5x5& atpa,
                                     Eigen::VectorXd& atpl) const;
    /**
     * Sets up contributions of given observation to the normal equation matrices.
     */
    void setupObservation(SolutionD solDir,
                          const Base::Vector3f& point,
                          const Base::Vector3d& residual,
                          double a[5],
                          double& f0,
                          double& qw,
                          double b[3]) const;
    /**
     * Computes contribution of the given observation equation on the normal equation matrices
     */
    void addObservationU(double a[5],
                         double li,
                         double pi,
                         Matrix5x5& atpa,
                         Eigen::VectorXd& atpl) const;
    /**
     * Set the lower part of the normal matrix equal to the upper part
     */
    void setLowerPart(Matrix5x5& atpa) const;

    /**
     * Compute the residuals and sigma0 and check the residual convergence
     */
    bool computeResiduals(SolutionD solDir,
                          const Eigen::VectorXd& x,
                          std::vector<Base::Vector3d>& residuals,
                          double& sigma0,
                          double vConvLimit,
                          bool& vConverged) const;
    /**
     * Update the parameters after solving the normal equations
     */
    bool updateParameters(SolutionD solDir, const Eigen::VectorXd& x);

private:
    Base::Vector3d _vBase;           /**< Base vector of the cylinder (point on axis). */
    Base::Vector3d _vAxis;           /**< Axis of the cylinder. */
    double _dRadius {0};             /**< Radius of the cylinder. */
    int _numIter {0};                /**< Number of iterations for solution to converge. */
    double _posConvLimit {0.0001};   /**< Position and radius parameter convergence threshold. */
    double _dirConvLimit {0.000001}; /**< Direction parameter convergence threshold. */
    double _vConvLimit {0.001};      /**< Residual convergence threshold. */
    int _maxIter {50};               /**< Maximum number of iterations. */
};


}  // namespace MeshCoreFit

#endif  // MESH_CYLINDER_FIT_H
