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


#ifndef BASE_VECTOR3D_H
#define BASE_VECTOR3D_H


#include <cmath>
#include <cfloat>

#ifndef F_PI
#define F_PI 3.1415926f
#endif

#ifndef D_PI
#define D_PI 3.141592653589793
#endif

#ifndef FLOAT_MAX
#define FLOAT_MAX 3.402823466E+38F
#endif

#ifndef FLOAT_MIN
#define FLOAT_MIN 1.175494351E-38F
#endif

#ifndef DOUBLE_MAX
#define DOUBLE_MAX 1.7976931348623157E+308 /* max decimal value of a "double"*/
#endif

#ifndef DOUBLE_MIN
#define DOUBLE_MIN 2.2250738585072014E-308 /* min decimal value of a "double"*/
#endif


namespace Base
{
template<class numT>
struct float_traits
{
};

template<>
struct float_traits<float>
{
    using float_type = float;
    static inline float_type pi()
    {
        return F_PI;
    }
    static inline float_type epsilon()
    {
        return FLT_EPSILON;
    }
    static inline float_type maximum()
    {
        return FLT_MAX;
    }
};

template<>
struct float_traits<double>
{
    using float_type = double;
    static inline float_type pi()
    {
        return D_PI;
    }
    static inline float_type epsilon()
    {
        return DBL_EPSILON;
    }
    static inline float_type maximum()
    {
        return DBL_MAX;
    }
};

/** The Vector Base class. */
template<class float_type>
class Vector3
{
public:
    using num_type = float_type;
    using traits_type = float_traits<num_type>;
    static inline num_type epsilon()
    {
        return traits_type::epsilon();
    }

    /** @name Public data members */
    //@{
    float_type x; /**< x-coordinate */
    float_type y; /**< y-coordinate */
    float_type z; /**< z-coordinate */
    //@}

    /// Construction
    explicit Vector3(float_type fx = 0.0, float_type fy = 0.0, float_type fz = 0.0);
    Vector3(const Vector3<float_type>& v) = default;
    Vector3(Vector3<float_type>&& v) noexcept = default;
    ~Vector3() = default;

    /** @name Operator */
    //@{
    /// Returns a reference to a coordinate. \a usIndex must be in the range [0,2]
    float_type& operator[](unsigned short usIndex);
    /// Returns a const reference to a coordinate. \a usIndex must be in the range [0,2]
    const float_type& operator[](unsigned short usIndex) const;
    /// Vector addition
    Vector3 operator+(const Vector3<float_type>& rcVct) const;
    Vector3 operator&(const Vector3<float_type>& rcVct) const;
    /// Vector subtraction
    Vector3 operator-(const Vector3<float_type>& rcVct) const;
    /// Negative vector
    Vector3 operator-() const;
    /// Vector summation
    Vector3& operator+=(const Vector3<float_type>& rcVct);
    /// Vector subtraction
    Vector3& operator-=(const Vector3<float_type>& rcVct);
    /// Vector scaling
    Vector3 operator*(float_type fScale) const;
    Vector3 operator/(float_type fDiv) const;
    Vector3& operator*=(float_type fScale);
    Vector3& operator/=(float_type fDiv);
    /// Assignment
    Vector3& operator=(const Vector3<float_type>& v) = default;
    Vector3& operator=(Vector3<float_type>&& v) noexcept = default;
    /// Scalar product
    float_type operator*(const Vector3<float_type>& rcVct) const;
    /// Scalar product
    float_type Dot(const Vector3<float_type>& rcVct) const;
    /// Cross product
    Vector3 operator%(const Vector3<float_type>& rcVct) const;
    /// Cross product
    Vector3 Cross(const Vector3<float_type>& rcVct) const;

    /// Comparing for inequality
    bool operator!=(const Vector3<float_type>& rcVct) const;
    /// Comparing for equality
    bool operator==(const Vector3<float_type>& rcVct) const;
    //@}

    /// Check if Vector is on a line segment
    bool IsOnLineSegment(const Vector3<float_type>& startVct,
                         const Vector3<float_type>& endVct) const;

    /** @name Modification */
    //@{
    void ScaleX(float_type f);
    void ScaleY(float_type f);
    void ScaleZ(float_type f);
    void Scale(float_type fX, float_type fY, float_type fZ);
    void MoveX(float_type f);
    void MoveY(float_type f);
    void MoveZ(float_type f);
    void Move(float_type fX, float_type fY, float_type fZ);
    void RotateX(float_type f);
    void RotateY(float_type f);
    void RotateZ(float_type f);
    //@}

    void Set(float_type fX, float_type fY, float_type fZ);

    /** @name Mathematics */
    //@{
    /// Length of the vector.
    float_type Length() const;
    /// Squared length of the vector.
    float_type Sqr() const;
    /// Set length to 1.
    Vector3& Normalize();
    /// Checks whether this is the null vector
    bool IsNull() const;
    /// Get angle between both vectors. The returned value lies in the interval [0,pi].
    float_type GetAngle(const Vector3& rcVect) const;
    /** Transforms this point to the coordinate system defined by origin \a rclBase,
     * vector \a vector rclDirX and vector \a vector rclDirY.
     * \note \a rclDirX must be perpendicular to \a rclDirY, i.e. \a rclDirX * \a rclDirY = 0..
     */
    void TransformToCoordinateSystem(const Vector3& rclBase,
                                     const Vector3& rclDirX,
                                     const Vector3& rclDirY);
    /**
     * @brief IsEqual
     * @param rclPnt
     * @param tol
     * @return true or false
     * If the distance to point \a rclPnt is within the tolerance \a tol both points are considered
     * equal.
     */
    bool IsEqual(const Vector3& rclPnt, float_type tol) const;
    /// Projects this point onto the plane given by the base \a rclBase and the normal \a rclNorm.
    Vector3& ProjectToPlane(const Vector3& rclBase, const Vector3& rclNorm);
    /**
     * Projects this point onto the plane given by the base \a rclBase and the normal \a rclNorm
     * and stores the result in rclProj.
     */
    void ProjectToPlane(const Vector3& rclBase, const Vector3& rclNorm, Vector3& rclProj) const;
    /// Projects this point onto the line given by the base \a rclPoint and the direction \a
    /// rclLine.
    /**
     * Projects a point \a rclPoint onto the line defined by the origin and the direction \a
     * rclLine. The result is a vector from \a rclPoint to the point on the line. The length of this
     * vector is the distance from \a rclPoint to the line. Note: The resulting vector does not
     * depend on the current vector.
     */
    Vector3& ProjectToLine(const Vector3& rclPoint, const Vector3& rclLine);
    /**
     * Get the perpendicular of this point to the line defined by rclBase and rclDir.
     * Note: Do not mix up this method with ProjectToLine.
     */
    Vector3 Perpendicular(const Vector3& rclBase, const Vector3& rclDir) const;
    /** Computes the distance to the given plane. Depending on the side this point is located
     * the distance can also be negative. The distance is positive if the point is at the same
     * side the plane normal points to, negative otherwise.
     */
    float_type DistanceToPlane(const Vector3& rclBase, const Vector3& rclNorm) const;
    /// Computes the distance from this point to the line given by \a rclBase and \a rclDirect.
    float_type DistanceToLine(const Vector3& rclBase, const Vector3& rclDirect) const;
    /** Computes the vector from this point to the point on the line segment with the shortest
     * distance. The line segment is defined by \a rclP1 and \a rclP2.
     * Note: If the projection of this point is outside the segment then the shortest distance
     * to \a rclP1 or \a rclP2 is computed.
     */
    Vector3 DistanceToLineSegment(const Vector3& rclP1, const Vector3& rclP2) const;
    //@}
};

// global functions

/// Returns the distance between two points
template<class float_type>
inline float_type Distance(const Vector3<float_type>& v1, const Vector3<float_type>& v2)
{
    float_type x = v1.x - v2.x;
    float_type y = v1.y - v2.y;
    float_type z = v1.z - v2.z;
    return static_cast<float_type>(sqrt((x * x) + (y * y) + (z * z)));
}

/// Returns the squared distance between two points
template<class float_type>
inline float_type DistanceP2(const Vector3<float_type>& v1, const Vector3<float_type>& v2)
{
    float_type x = v1.x - v2.x;
    float_type y = v1.y - v2.y;
    float_type z = v1.z - v2.z;
    return x * x + y * y + z * z;
}

/// Multiplication of scalar with vector.
template<class float_type>
inline Vector3<float_type> operator*(float_type fFac, const Vector3<float_type>& rcVct)
{
    return Vector3<float_type>(rcVct.x * fFac, rcVct.y * fFac, rcVct.z * fFac);
}

template<class Pr1, class Pr2>
inline Vector3<Pr1> toVector(const Vector3<Pr2>& v)
{
    return Vector3<Pr1>(static_cast<Pr1>(v.x), static_cast<Pr1>(v.y), static_cast<Pr1>(v.z));
}

using Vector3f = Vector3<float>;
using Vector3d = Vector3<double>;

}  // namespace Base

#endif  // BASE_VECTOR3D_H
