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

#include <array>
#include <cmath>
#include <string>

#include "Vector3D.h"
#include <FCGlobal.h>


namespace Base
{

enum class ScaleType
{
    Other = -1,
    NoScaling = 0,
    NonUniformRight = 1,
    NonUniformLeft = 2,
    Uniform = 3
};

/**
 * The Matrix4D class.
 */
class BaseExport Matrix4D  // NOLINT(cppcoreguidelines-special-member-functions)
{
    using traits_type = float_traits<double>;

public:
    /// Default constructor
    /*!
     * Initialises to an identity matrix
     */
    Matrix4D();

    // clang-format off
    /// Construction
    Matrix4D(float a11, float a12, float a13, float a14,
             float a21, float a22, float a23, float a24,
             float a31, float a32, float a33, float a34,
             float a41, float a42, float a43, float a44);
    /// Construction
    Matrix4D(double a11, double a12, double a13, double a14,
             double a21, double a22, double a23, double a24,
             double a31, double a32, double a33, double a34,
             double a41, double a42, double a43, double a44);
    // clang-format on
    /// Construction
    Matrix4D(const Matrix4D& mat);
    /// Construction with an Axis
    Matrix4D(const Vector3f& rclBase, const Vector3f& rclDir, float fAngle);
    Matrix4D(const Vector3d& rclBase, const Vector3d& rclDir, double fAngle);
    /// Destruction
    ~Matrix4D() = default;

    /** @name Operators */
    //@{
    /// Matrix addition
    inline Matrix4D operator+(const Matrix4D& mat) const;
    inline Matrix4D& operator+=(const Matrix4D& mat);
    /// Matrix subtraction
    inline Matrix4D operator-(const Matrix4D& mat) const;
    inline Matrix4D& operator-=(const Matrix4D& mat);
    /// Matrix multiplication
    inline Matrix4D& operator*=(const Matrix4D& mat);
    /// Assignment
    inline Matrix4D& operator=(const Matrix4D& mat);
    /// Matrix multiplication
    inline Matrix4D operator*(const Matrix4D& mat) const;
    /// Multiplication matrix with vector
    inline Vector3f operator*(const Vector3f& vec) const;
    inline Vector3d operator*(const Vector3d& vec) const;
    inline void multVec(const Vector3d& src, Vector3d& dst) const;
    inline void multVec(const Vector3f& src, Vector3f& dst) const;
    inline Matrix4D operator*(double scalar) const;
    inline Matrix4D& operator*=(double scalar);
    /// Comparison
    inline bool operator!=(const Matrix4D& mat) const;
    /// Comparison
    inline bool operator==(const Matrix4D& mat) const;
    /// Index operator
    inline std::array<double, 4>& operator[](unsigned int usNdx);
    /// Index operator
    inline const std::array<double, 4>& operator[](unsigned int usNdx) const;
    /// Get vector of row
    inline Vector3d getRow(unsigned int usNdx) const;
    /// Get vector of column
    inline Vector3d getCol(unsigned int usNdx) const;
    /// Get vector of diagonal
    inline Vector3d diagonal() const;
    /// Get trace of the 3x3 matrix
    inline double trace3() const;
    /// Get trace of the 4x4 matrix
    inline double trace() const;
    /// Set row to vector
    inline void setRow(unsigned int usNdx, const Vector3d& vec);
    /// Set column to vector
    inline void setCol(unsigned int usNdx, const Vector3d& vec);
    /// Set diagonal to vector
    inline void setDiagonal(const Vector3d& vec);
    /// Compute the determinant of the matrix
    double determinant() const;
    /// Compute the determinant of the 3x3 sub-matrix
    double determinant3() const;
    /// Analyse the transformation
    std::string analyse() const;
    /// Outer product (Dyadic product)
    Matrix4D& Outer(const Vector3f& rV1, const Vector3f& rV2);
    Matrix4D& Outer(const Vector3d& rV1, const Vector3d& rV2);
    /// Hat operator (skew symmetric)
    Matrix4D& Hat(const Vector3f& rV);
    Matrix4D& Hat(const Vector3d& rV);
    //@}

    void getMatrix(double dMtrx[16]) const;
    void setMatrix(const double dMtrx[16]);
    /// get the matrix in OpenGL style
    void getGLMatrix(double dMtrx[16]) const;
    /// set the matrix in OpenGL style
    void setGLMatrix(const double dMtrx[16]);

    unsigned long getMemSpace();

    /** @name Manipulation */
    //@{
    /// Makes unity matrix
    void setToUnity();
    /// Checks if this is the unit matrix
    bool isUnity() const;
    /// Checks if this is the unit matrix
    bool isUnity(double tol) const;
    /// Makes a null matrix
    void nullify();
    /// Checks if this is the null matrix
    bool isNull() const;
    /// moves the coordinatesystem for the x,y,z value
    void move(float x, float y, float z)
    {
        move(Vector3f(x, y, z));
    }
    void move(double x, double y, double z)
    {
        move(Vector3d(x, y, z));
    }
    /// moves the coordinatesystem for the vector
    void move(const Vector3f& vec);
    void move(const Vector3d& vec);
    /// scale for the vector
    void scale(float x, float y, float z)
    {
        scale(Vector3f(x, y, z));
    }
    void scale(double x, double y, double z)
    {
        scale(Vector3d(x, y, z));
    }
    /// scale for the x,y,z value
    void scale(const Vector3f& vec);
    void scale(const Vector3d& vec);
    /// uniform scale
    void scale(float scalexyz)
    {
        scale(Vector3f(scalexyz, scalexyz, scalexyz));
    }
    void scale(double scalexyz)
    {
        scale(Vector3d(scalexyz, scalexyz, scalexyz));
    }
    /// Check for scaling factor
    ScaleType hasScale(double tol = 0.0) const;
    /// Decompose matrix into pure shear, scale, rotation and move
    std::array<Matrix4D, 4> decompose() const;
    /// Rotate around the X axis (in transformed space) for the given value in radians
    void rotX(double fAngle);
    /// Rotate around the Y axis (in transformed space) for the given value in radians
    void rotY(double fAngle);
    /// Rotate around the Z axis (in transformed space) for the given value in radians
    void rotZ(double fAngle);
    /// Rotate around an arbitrary axis passing the origin in radians
    void rotLine(const Vector3f& vec, float fAngle);
    /// Rotate around an arbitrary axis passing the origin in radians
    void rotLine(const Vector3d& vec, double fAngle);
    /// Rotate around an arbitrary axis that needn't necessarily pass the origin in radians
    void rotLine(const Vector3f& rclBase, const Vector3f& rclDir, float fAngle);
    /// Rotate around an arbitrary axis that needn't necessarily pass the origin in radians
    void rotLine(const Vector3d& rclBase, const Vector3d& rclDir, double fAngle);
    /// Extract the rotation axis and angle. Therefore the 3x3 submatrix must be orthogonal.
    bool toAxisAngle(Vector3f& rclBase, Vector3f& rclDir, float& fAngle, float& fTranslation) const;
    bool toAxisAngle(Vector3d& rclBase, Vector3d& rclDir, double& fAngle, double& fTranslation) const;
    /// transform (move,scale,rotate) around a point
    void transform(const Vector3f& vec, const Matrix4D& mat);
    void transform(const Vector3d& vec, const Matrix4D& mat);
    /// Matrix is expected to have a 3x3 rotation submatrix.
    void inverse();
    /// Matrix is expected to have a 3x3 rotation submatrix.
    void inverseOrthogonal();
    /// Arbitrary, non-singular matrix
    void inverseGauss();
    void transpose();
    //@}

    void Print() const;
    /// write the 16 double of the matrix into a string
    std::string toString() const;
    /// read the 16 double of the matrix from a string
    void fromString(const std::string& str);

private:
    using Array2d = std::array<std::array<double, 4>, 4>;
    Array2d dMtrx4D;
};

inline Matrix4D Matrix4D::operator+(const Matrix4D& mat) const
{
    Matrix4D newMat(*this);
    return newMat += mat;
}

inline Matrix4D& Matrix4D::operator+=(const Matrix4D& mat)
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            dMtrx4D[i][j] += mat[i][j];
        }
    }

    return *this;
}

inline Matrix4D Matrix4D::operator-(const Matrix4D& mat) const
{
    Matrix4D newMat(*this);
    return newMat -= mat;
}

inline Matrix4D& Matrix4D::operator-=(const Matrix4D& mat)
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            dMtrx4D[i][j] -= mat[i][j];
        }
    }

    return *this;
}

inline Matrix4D& Matrix4D::operator*=(const Matrix4D& mat)
{
    (*this) = (*this) * mat;
    return *this;
}

inline Matrix4D Matrix4D::operator*(const Matrix4D& mat) const
{
    Matrix4D clMat;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            clMat.dMtrx4D[i][j] = 0;
            for (int e = 0; e < 4; e++) {
                clMat.dMtrx4D[i][j] += dMtrx4D[i][e] * mat.dMtrx4D[e][j];
            }
        }
    }

    return clMat;
}

inline Matrix4D& Matrix4D::operator=(const Matrix4D& mat)
{
    if (this == &mat) {
        return *this;
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            dMtrx4D[i][j] = mat.dMtrx4D[i][j];
        }
    }

    return *this;
}

inline Vector3f Matrix4D::operator*(const Vector3f& vec) const
{
    Vector3f dst;
    multVec(vec, dst);
    return dst;
}

inline Vector3d Matrix4D::operator*(const Vector3d& vec) const
{
    Vector3d dst;
    multVec(vec, dst);
    return dst;
}

inline void Matrix4D::multVec(const Vector3d& src, Vector3d& dst) const
{
    // clang-format off
    double dx = (dMtrx4D[0][0] * src.x + dMtrx4D[0][1] * src.y + dMtrx4D[0][2] * src.z + dMtrx4D[0][3]);
    double dy = (dMtrx4D[1][0] * src.x + dMtrx4D[1][1] * src.y + dMtrx4D[1][2] * src.z + dMtrx4D[1][3]);
    double dz = (dMtrx4D[2][0] * src.x + dMtrx4D[2][1] * src.y + dMtrx4D[2][2] * src.z + dMtrx4D[2][3]);
    dst.Set(dx, dy, dz);
    // clang-format on
}

inline void Matrix4D::multVec(const Vector3f& src, Vector3f& dst) const
{
    double sx = static_cast<double>(src.x);
    double sy = static_cast<double>(src.y);
    double sz = static_cast<double>(src.z);

    double dx = (dMtrx4D[0][0] * sx + dMtrx4D[0][1] * sy + dMtrx4D[0][2] * sz + dMtrx4D[0][3]);
    double dy = (dMtrx4D[1][0] * sx + dMtrx4D[1][1] * sy + dMtrx4D[1][2] * sz + dMtrx4D[1][3]);
    double dz = (dMtrx4D[2][0] * sx + dMtrx4D[2][1] * sy + dMtrx4D[2][2] * sz + dMtrx4D[2][3]);
    dst.Set(static_cast<float>(dx), static_cast<float>(dy), static_cast<float>(dz));
}

inline Matrix4D Matrix4D::operator*(double scalar) const
{
    Matrix4D newMat(*this);
    return newMat *= scalar;
}

inline Matrix4D& Matrix4D::operator*=(double scalar)
{
    // NOLINTBEGIN
    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            dMtrx4D[i][j] *= scalar;
        }
    }
    // NOLINTEND
    return *this;
}

inline bool Matrix4D::operator==(const Matrix4D& mat) const
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (fabs(dMtrx4D[i][j] - mat.dMtrx4D[i][j]) > traits_type::epsilon()) {
                return false;
            }
        }
    }

    return true;
}

inline bool Matrix4D::operator!=(const Matrix4D& mat) const
{
    return !((*this) == mat);
}

inline Vector3f& operator*=(Vector3f& vec, const Matrix4D& mat)
{
    mat.multVec(vec, vec);
    return vec;
}

inline std::array<double, 4>& Matrix4D::operator[](unsigned int usNdx)
{
    return dMtrx4D[usNdx];
}

inline const std::array<double, 4>& Matrix4D::operator[](unsigned int usNdx) const
{
    return dMtrx4D[usNdx];
}

inline Vector3d Matrix4D::getRow(unsigned int usNdx) const
{
    return Vector3d(dMtrx4D[usNdx][0], dMtrx4D[usNdx][1], dMtrx4D[usNdx][2]);
}

inline Vector3d Matrix4D::getCol(unsigned int usNdx) const
{
    return Vector3d(dMtrx4D[0][usNdx], dMtrx4D[1][usNdx], dMtrx4D[2][usNdx]);
}

inline Vector3d Matrix4D::diagonal() const
{
    return Vector3d(dMtrx4D[0][0], dMtrx4D[1][1], dMtrx4D[2][2]);
}

inline double Matrix4D::trace3() const
{
    return dMtrx4D[0][0] + dMtrx4D[1][1] + dMtrx4D[2][2];
}

inline double Matrix4D::trace() const
{
    return dMtrx4D[0][0] + dMtrx4D[1][1] + dMtrx4D[2][2] + dMtrx4D[3][3];
}

inline void Matrix4D::setRow(unsigned int usNdx, const Vector3d& vec)
{
    dMtrx4D[usNdx][0] = vec.x;
    dMtrx4D[usNdx][1] = vec.y;
    dMtrx4D[usNdx][2] = vec.z;
}

inline void Matrix4D::setCol(unsigned int usNdx, const Vector3d& vec)
{
    dMtrx4D[0][usNdx] = vec.x;
    dMtrx4D[1][usNdx] = vec.y;
    dMtrx4D[2][usNdx] = vec.z;
}

inline void Matrix4D::setDiagonal(const Vector3d& vec)
{
    dMtrx4D[0][0] = vec.x;
    dMtrx4D[1][1] = vec.y;
    dMtrx4D[2][2] = vec.z;
}

}  // namespace Base
