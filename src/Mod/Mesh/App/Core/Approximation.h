// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <Mod/Mesh/App/WildMagic4/Wm4QuadricSurface.h>
#include <Mod/Mesh/MeshGlobal.h>

#include <algorithm>
#include <limits>
#include <list>
#include <set>
#include <vector>

#include <Base/BoundBox.h>
#include <Base/Matrix.h>
#include <Base/Vector3D.h>


namespace Wm4
{

/**
 * An implicit surface is defined by F(x,y,z) = 0.
 * This polynomial surface is actually defined as z = f(x,y) = ax^2 + by^2 + cx + dy + exy + g.
 * To use Wm3 routines for implicit surfaces we can write the surface also as F(x,y,z) = f(x,y) - z
 * = 0.
 * @author Werner Mayer
 */
template<class Real>
class PolynomialSurface: public ImplicitSurface<Real>
{
public:
    explicit PolynomialSurface(const Real afCoeff[6])
    {
        for (int i = 0; i < 6; i++) {
            m_afCoeff[i] = afCoeff[i];
        }
    }

    // the function
    Real F(const Vector3<Real>& rkP) const override
    {
        return (
            m_afCoeff[0] * rkP.X() * rkP.X() + m_afCoeff[1] * rkP.Y() * rkP.Y()
            + m_afCoeff[2] * rkP.X() + m_afCoeff[3] * rkP.Y() + m_afCoeff[4] * rkP.X() * rkP.Y()
            + m_afCoeff[5] - rkP.Z()
        );
    }

    // first-order partial derivatives
    Real FX(const Vector3<Real>& rkP) const override
    {
        return (Real)(2.0 * m_afCoeff[0] * rkP.X() + m_afCoeff[2] + m_afCoeff[4] * rkP.Y());
    }
    Real FY(const Vector3<Real>& rkP) const override
    {
        return (Real)(2.0 * m_afCoeff[1] * rkP.Y() + m_afCoeff[3] + m_afCoeff[4] * rkP.X());
    }
    Real FZ(const Vector3<Real>& /*rkP*/) const override
    {
        return (Real)-1.0;
    }

    // second-order partial derivatives
    Real FXX(const Vector3<Real>& /*rkP*/) const override
    {
        return (Real)(2.0 * m_afCoeff[0]);
    }
    Real FXY(const Vector3<Real>& /*rkP*/) const override
    {
        return (Real)(m_afCoeff[4]);
    }
    Real FXZ(const Vector3<Real>& /*rkP*/) const override
    {
        return (Real)0.0;
    }
    Real FYY(const Vector3<Real>& /*rkP*/) const override
    {
        return (Real)(2.0 * m_afCoeff[1]);
    }
    Real FYZ(const Vector3<Real>& /*rkP*/) const override
    {
        return (Real)0.0;
    }
    Real FZZ(const Vector3<Real>& /*rkP*/) const override
    {
        return (Real)0.0;
    }

private:
    Real m_afCoeff[6];
};

}  // namespace Wm4

namespace MeshCore
{
class MeshPointArray;

/**
 * Abstract base class for approximation of a geometry to a given set of points.
 */
class MeshExport Approximation
{
public:
    /**
     * Construction
     */
    Approximation();
    /**
     * Destroys the object and frees any allocated resources.
     */
    virtual ~Approximation();
    /**
     * Add point for the fit algorithm.
     */
    void AddPoint(const Base::Vector3f& point);
    /**
     * Add points for the fit algorithm.
     */
    void AddPoints(const std::vector<Base::Vector3f>& points);
    /**
     * Add points for the fit algorithm.
     */
    void AddPoints(const std::set<Base::Vector3f>& points);
    /**
     * Add points for the fit algorithm.
     */
    void AddPoints(const std::list<Base::Vector3f>& points);
    /**
     * Add points for the fit algorithm.
     */
    void AddPoints(const MeshPointArray& points);
    /**
     * Get all added points.
     */
    const std::list<Base::Vector3f>& GetPoints() const
    {
        return _vPoints;
    }
    /**
     * Returns the center of gravity of the current added points.
     * @return Base::Vector3f
     */
    Base::Vector3f GetGravity() const;
    /**
     * Determines the number of the current added points.
     * @return Number of points
     */
    std::size_t CountPoints() const;
    /**
     * Deletes the inserted points and frees any allocated resources.
     */
    void Clear();
    /**
     * Returns the result of the last fit.
     * @return float Quality of the last fit.
     */
    float GetLastResult() const;
    /**
     * Pure virtual function to fit the geometry to the given points. This function
     * must be implemented by every subclass.
     */
    virtual float Fit() = 0;
    /**
     * Returns true if Fit() has been called for the current set of points, false otherwise.
     */
    bool Done() const;

protected:
    /**
     * Creates a vector of Wm4::Vector3 elements.
     */
    void GetMgcVectorArray(std::vector<Wm4::Vector3<double>>& rcPts) const;

    Approximation(const Approximation&) = default;
    Approximation(Approximation&&) = default;
    Approximation& operator=(const Approximation&) = default;
    Approximation& operator=(Approximation&&) = default;

protected:
    // NOLINTBEGIN
    std::list<Base::Vector3f> _vPoints; /**< Holds the points for the fit algorithm.  */
    bool _bIsFitted {false};            /**< Flag, whether the fit has been called. */
    float _fLastResult {std::numeric_limits<float>::max()}; /**< Stores the last result of the fit */
    // NOLINTEND
};

// -------------------------------------------------------------------------------

/**
 * Approximation of a plane into a given set of points.
 */
class MeshExport PlaneFit: public Approximation
{
public:
    PlaneFit();
    Base::Vector3f GetBase() const;
    Base::Vector3f GetDirU() const;
    Base::Vector3f GetDirV() const;
    /**
     * Returns the normal of the fitted plane. If Fit() has not been called the null vector is
     * returned.
     */
    Base::Vector3f GetNormal() const;
    /**
     * Fit a plane into the given points. We must have at least three non-collinear points
     * to succeed. If the fit fails FLOAT_MAX is returned.
     */
    float Fit() override;
    /**
     * Returns the distance from the point \a rcPoint to the fitted plane. If Fit() has not been
     * called FLOAT_MAX is returned.
     */
    float GetDistanceToPlane(const Base::Vector3f& rcPoint) const;
    /**
     * Returns the standard deviation from the points to the fitted plane. If Fit() has not been
     * called FLOAT_MAX is returned.
     */
    float GetStdDeviation() const;
    /**
     * Returns the standard deviation from the points to the fitted plane with respect to the
     * orientation of the plane's normal. If Fit() has not been called FLOAT_MAX is returned.
     */
    float GetSignedStdDeviation() const;
    /**
     * Projects the points onto the fitted plane.
     */
    void ProjectToPlane();
    /**
     * Get the dimension of the fitted plane.
     */
    void Dimension(float& length, float& width) const;
    /**
     * Returns an array of the transformed points relative to the coordinate system
     * of the plane. If this method is called before the plane is computed an empty
     * array is returned.
     */
    std::vector<Base::Vector3f> GetLocalPoints() const;
    /**
     * Returns the local bounding box of the transformed points relative to the
     * coordinate system of the plane. If this method is called before the plane is
     *  computed an invalid bounding box is returned.
     */
    Base::BoundBox3f GetBoundings() const;

protected:
    // NOLINTBEGIN
    Base::Vector3f _vBase; /**< Base vector of the plane. */
    Base::Vector3f _vDirU;
    Base::Vector3f _vDirV;
    Base::Vector3f _vDirW; /**< Normal of the plane. */
    // NOLINTEND
};

// -------------------------------------------------------------------------------

/**
 * Approximation of a quadratic surface into a given set of points. The implicit form of the surface
 * is defined by F(x,y,z) = a * x^2 + b * y^2 + c * z^2 +
 *                       2d * x * y + 2e * x * z + 2f * y * z +
 *                          g * x + h * y + * i * z + k
 *                        = 0
 * Depending on the parameters (a,..,k) this surface defines a sphere, ellipsoid, cylinder, cone
 * and so on.
 */
class MeshExport QuadraticFit: public Approximation
{
public:
    QuadraticFit() = default;
    /**
     * Get the quadric coefficients
     * @param ulIndex Number of coefficient (0..9)
     * @return double value of coefficient
     */
    double GetCoeff(std::size_t ulIndex) const;
    /**
     * Get the quadric coefficients as reference to the
     * internal array
     * @return const double& Reference to the double array
     */
    const double& GetCoeffArray() const;
    /**
     * Invocation of fitting algorithm
     * @return float Quality of fit.
     */
    float Fit() override;

    void CalcZValues(double x, double y, double& dZ1, double& dZ2) const;
    /**
     * Calculate the curvatures of the quadric at a given point.
     * @param x X-coordinate
     * @param y Y-coordinate
     * @param z Z-coordinate
     * @param rfCurv0 1. principal curvature
     * @param rfCurv1 2. principal curvature
     * @param rkDir0  Direction of 1. principal curvature
     * @param rkDir1  Direction of 2. principal curvature
     * @param dDistance
     * @return bool Success = true, otherwise false
     */
    bool GetCurvatureInfo(
        double x,
        double y,
        double z,
        double& rfCurv0,
        double& rfCurv1,
        Base::Vector3f& rkDir0,
        Base::Vector3f& rkDir1,
        double& dDistance
    );

    bool GetCurvatureInfo(double x, double y, double z, double& rfCurv0, double& rfcurv1);
    /**
     * Compute form matrix A and calculate Eigenvalues.
     * @param dLambda1 Eigenvalue 1
     * @param dLambda2 Eigenvalue 2
     * @param dLambda3 Eigenvalue 3
     * @param clEV1    Eigenvector 1
     * @param clEV2    Eigenvector 2
     * @param clEV3    Eigenvector 3
     */
    void CalcEigenValues(
        double& dLambda1,
        double& dLambda2,
        double& dLambda3,
        Base::Vector3f& clEV1,
        Base::Vector3f& clEV2,
        Base::Vector3f& clEV3
    ) const;

private:
    double _fCoeff[10] {}; /**< Coefficients of the fit */
};

// -------------------------------------------------------------------------------

/**
 * This is an 2.5D approach which first determines the bestfit plane of the point set (P_i =
 * (x,y,z), i=1,...,n) to get a parametrisation of the points afterwards. The coordinates of the
 * points with respect to the local coordinate system of the plane are determined and then a
 * quadratic polynomial function of the form: w = f(u,v) = a*u^2 + b*v^2 + c*u*v + d*u + e*v + f is
 * determined. This approach was developed as an alternative for the 3D approach with quadrics
 * because the latter suffers from strange artifacts in planar areas.
 */
class MeshExport SurfaceFit: public PlaneFit
{
public:
    SurfaceFit();

    bool GetCurvatureInfo(
        double x,
        double y,
        double z,
        double& rfCurv0,
        double& rfCurv1,
        Base::Vector3f& rkDir0,
        Base::Vector3f& rkDir1,
        double& dDistance
    );
    bool GetCurvatureInfo(double x, double y, double z, double& rfCurv0, double& rfcurv1);
    float Fit() override;
    double Value(double x, double y) const;
    void GetCoefficients(double& a, double& b, double& c, double& d, double& e, double& f) const;
    /**
     * @brief Transform
     * Transforms points from the local coordinate system to the world coordinate system
     */
    void Transform(std::vector<Base::Vector3f>&) const;
    void Transform(std::vector<Base::Vector3d>&) const;
    /**
     * @brief toBezier
     * @param umin Parameter range
     * @param umax Parameter range
     * @param vmin Parameter range
     * @param vmax Parameter range
     * @return control points of the Bezier surface
     */
    std::vector<Base::Vector3d> toBezier(
        double umin = 0.0,
        double umax = 1.0,
        double vmin = 0.0,
        double vmax = 1.0
    ) const;

private:
    double PolynomFit();
    double _fCoeff[10]; /**< Ziel der Koeffizienten aus dem Fit */
};

// -------------------------------------------------------------------------------

/**
 * Approximation of a cylinder into a given set of points.
 */
class MeshExport CylinderFit: public Approximation
{
public:
    CylinderFit();
    float GetRadius() const;
    Base::Vector3f GetBase() const;
    void SetInitialValues(const Base::Vector3f&, const Base::Vector3f&);
    /**
     * Returns the axis of the fitted cylinder. If Fit() has not been called the null vector is
     * returned.
     */
    Base::Vector3f GetAxis() const;
    /**
     * Returns an initial axis based on point normals.
     */
    Base::Vector3f GetInitialAxisFromNormals(const std::vector<Base::Vector3f>& n) const;
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
    /**
     * Get the bottom and top points of the cylinder. The distance of these
     * points gives the height of the cylinder.
     */
    void GetBounding(Base::Vector3f& bottom, Base::Vector3f& top) const;

private:
    Base::Vector3f _vBase; /**< Base vector of the cylinder. */
    Base::Vector3f _vAxis; /**< Axis of the cylinder. */
    float _fRadius {0};    /**< Radius of the cylinder. */
    bool _initialGuess {false};
};

// -------------------------------------------------------------------------------

/**
 * Approximation of a sphere into a given set of points.
 */
class MeshExport SphereFit: public Approximation
{
public:
    SphereFit();
    float GetRadius() const;
    Base::Vector3f GetCenter() const;
    /**
     * Fit a sphere into the given points. If the fit fails FLOAT_MAX is returned.
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

private:
    Base::Vector3f _vCenter; /**< Center of the sphere. */
    float _fRadius {0};      /**< Radius of the cylinder. */
};

// -------------------------------------------------------------------------------

/**
 * Helper class for the quadric fit. Includes the
 * partial derivates of the quadric and serves for
 * calculation of the quadric properties.
 */
class FunctionContainer
{
public:
    /**
     * WildMagic library uses function with this interface
     */
    using Function = double (*)(double, double, double);
    /**
     * The constructor expects an array of quadric coefficients.
     * @param pKoef Pointer to the quadric coefficients
     *        (double [10])
     */
    // NOLINTBEGIN
    explicit FunctionContainer(const double* pKoef)
    {
        Assign(pKoef);
        pImplSurf = new Wm4::QuadricSurface<double>(dKoeff);
    }
    // NOLINTEND

    FunctionContainer(const FunctionContainer&) = delete;
    FunctionContainer(FunctionContainer&&) = delete;
    FunctionContainer& operator=(const FunctionContainer&) = delete;
    FunctionContainer& operator=(FunctionContainer&&) = delete;

    /**
     * Apply quadric coefficients
     * @param pKoef Pointer to the quadric coefficients
     *        (double [10])
     */
    void Assign(const double* pKoef)
    {
        for (long ct = 0; ct < 10; ct++) {
            dKoeff[ct] = pKoef[ct];
        }
    }
    /**
     * Destructor. Deletes the ImpicitSurface instance
     * of the WildMagic library
     */
    ~FunctionContainer()
    {
        delete pImplSurf;
    }
    /**
     * Access to the quadric coefficients
     * @param idx Index to coefficient
     * @return double& coefficient
     */
    double& operator[](int idx)
    {
        return dKoeff[idx];
    }
    /**
     * Redirector to a method of the WildMagic library. Determines
     * the principal curvatures and their directions at the given point.
     * @param x X-coordinate
     * @param y Y-coordinate
     * @param z Z-coordinate
     * @param rfCurv0 1. principal curvature
     * @param rfCurv1 2. principal curvature
     * @param rkDir0  direction of 1. principal curvature
     * @param rkDir1  direction of 2. principal curvature
     * @param dDistance Gives distances from the point to the quadric.
     * @return bool Success = true, otherwise false
     */
    bool CurvatureInfo(
        double x,
        double y,
        double z,
        double& rfCurv0,
        double& rfCurv1,
        Wm4::Vector3<double>& rkDir0,
        Wm4::Vector3<double>& rkDir1,
        double& dDistance
    )
    {
        (void)dDistance;
        return pImplSurf->ComputePrincipalCurvatureInfo(
            Wm4::Vector3<double>(x, y, z),
            rfCurv0,
            rfCurv1,
            rkDir0,
            rkDir1
        );
    }

    Base::Vector3f GetGradient(double x, double y, double z) const
    {
        Wm4::Vector3<double> grad = pImplSurf->GetGradient(Wm4::Vector3<double>(x, y, z));
        return Base::Vector3f(
            static_cast<float>(grad.X()),
            static_cast<float>(grad.Y()),
            static_cast<float>(grad.Z())
        );
    }

    Base::Matrix4D GetHessian(double x, double y, double z) const
    {
        Wm4::Matrix3<double> hess = pImplSurf->GetHessian(Wm4::Vector3<double>(x, y, z));
        Base::Matrix4D cMat;
        cMat.setToUnity();
        cMat[0][0] = hess[0][0];
        cMat[0][1] = hess[0][1];
        cMat[0][2] = hess[0][2];
        cMat[1][0] = hess[1][0];
        cMat[1][1] = hess[1][1];
        cMat[1][2] = hess[1][2];
        cMat[2][0] = hess[2][0];
        cMat[2][1] = hess[2][1];
        cMat[2][2] = hess[2][2];
        return cMat;
    }

    bool CurvatureInfo(double x, double y, double z, double& rfCurv0, double& rfCurv1)
    {
        double dQuot = Fz(x, y, z);
        double zx = -(Fx(x, y, z) / dQuot);
        double zy = -(Fy(x, y, z) / dQuot);

        double zxx = -(2.0 * (dKoeff[5] + dKoeff[6] * zx * zx + dKoeff[8] * zx)) / dQuot;
        double zyy = -(2.0 * (dKoeff[5] + dKoeff[6] * zy * zy + dKoeff[9] * zy)) / dQuot;
        double zxy = -(dKoeff[6] * zx * zy + dKoeff[7] + dKoeff[8] * zy + dKoeff[9] * zx) / dQuot;

        double dNen = 1 + zx * zx + zy * zy;
        double dNenSqrt = sqrt(dNen);
        double K = (zxx * zyy - zxy * zxy) / (dNen * dNen);
        double H = 0.5
            * ((1.0 + zx * zx - 2 * zx * zy * zxy + (1.0 + zy * zy) * zxx)
               / (dNenSqrt * dNenSqrt * dNenSqrt));

        double dDiscr = sqrt(fabs(H * H - K));
        rfCurv0 = H - dDiscr;
        rfCurv1 = H + dDiscr;

        return true;
    }

    //+++++++++ Quadric +++++++++++++++++++++++++++++++++++++++
    double F(double x, double y, double z)
    {
        return (
            dKoeff[0] + dKoeff[1] * x + dKoeff[2] * y + dKoeff[3] * z + dKoeff[4] * x * x
            + dKoeff[5] * y * y + dKoeff[6] * z * z + dKoeff[7] * x * y + dKoeff[8] * x * z
            + dKoeff[9] * y * z
        );
    }

    //+++++++++ 1. derivations ++++++++++++++++++++++++++++++++
    double Fx(double x, double y, double z)
    {
        return (dKoeff[1] + 2.0 * dKoeff[4] * x + dKoeff[7] * y + dKoeff[8] * z);
    }
    double Fy(double x, double y, double z)
    {
        return (dKoeff[2] + 2.0 * dKoeff[5] * y + dKoeff[7] * x + dKoeff[9] * z);
    }
    double Fz(double x, double y, double z)
    {
        return (dKoeff[3] + 2.0 * dKoeff[6] * z + dKoeff[8] * x + dKoeff[9] * y);
    }

    //+++++++++ 2. derivations ++++++++++++++++++++++++++++++++
    double Fxx(double x, double y, double z)
    {
        (void)x;
        (void)y;
        (void)z;
        return (2.0 * dKoeff[4]);
    }
    double Fxy(double x, double y, double z)
    {
        (void)x;
        (void)y;
        (void)z;
        return (dKoeff[7]);
    }
    double Fxz(double x, double y, double z)
    {
        (void)x;
        (void)y;
        (void)z;
        return (dKoeff[8]);
    }
    double Fyy(double x, double y, double z)
    {
        (void)x;
        (void)y;
        (void)z;
        return (2.0 * dKoeff[5]);
    }
    double Fyz(double x, double y, double z)
    {
        (void)x;
        (void)y;
        (void)z;
        return (dKoeff[9]);
    }
    double Fzz(double x, double y, double z)
    {
        (void)x;
        (void)y;
        (void)z;
        return (2.0 * dKoeff[6]);
    }

private:
    double dKoeff[10];                       /**< Coefficients of quadric */
    Wm4::ImplicitSurface<double>* pImplSurf; /**< Access to the WildMagic library */

private:
    /**
     * Private construction.
     */
    FunctionContainer() = default;
};

class MeshExport PolynomialFit: public Approximation
{
public:
    PolynomialFit();

    float Fit() override;
    float Value(float x, float y) const;

private:
    float _fCoeff[9];
};

}  // namespace MeshCore
