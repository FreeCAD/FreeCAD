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


#include <math.h>

#define FLOAT_EPS   1.0e-4f 

#ifndef  F_PI
# define F_PI  3.1415926f
#endif

#ifndef  D_PI
# define D_PI  3.141592653589793
#endif
  
#ifndef  FLOAT_MAX
# define FLOAT_MAX 1e30f
#endif

#ifndef  DOUBLE_MAX
# define DOUBLE_MAX 1.7976931348623157E+308    /* max decimal value of a "double"*/
#endif

#ifndef  DOUBLE_MIN
# define DOUBLE_MIN 2.2250738585072014E-308    /* min decimal value of a "double"*/
#endif


namespace Base {
template <class numT>
struct float_traits { };

template <>
struct float_traits<float> {
    typedef float float_type;
    static inline float_type pi() { return F_PI; }
    static inline float_type epsilon() { return FLOAT_EPS; }
    static inline float_type maximum() { return FLOAT_MAX; }
};

template <>
struct float_traits<double> {
    typedef double float_type;
    static inline float_type pi() { return D_PI; }
    static inline float_type epsilon() { return FLOAT_EPS; }
    static inline float_type maximum() { return FLOAT_MAX; }
};

/** The Vector Base class. */
template <class _Precision>
class Vector3
{
public:
    typedef _Precision num_type;
    typedef float_traits<num_type> traits_type;
    static inline num_type epsilon() { return traits_type::epsilon(); }

    /** @name Public data members */
    //@{
    _Precision x; /**< x-coordinate */
    _Precision y; /**< y-coordinate */
    _Precision z; /**< z-coordinate */
    //@}

    /// Construction
    explicit Vector3 (_Precision fx = 0.0f, _Precision fy = 0.0f, _Precision fz = 0.0f);
    /// Construction
    Vector3 (const Vector3<_Precision>& rcVct);

    /** @name Operator */
    //@{
    /// Returns a reference to a coordinate. \a usIndex must be in the range [0,2]
    _Precision & operator [] (unsigned short usIndex);
    /// Returns a const reference to a coordinate. \a usIndex must be in the range [0,2]
    const _Precision & operator [] (unsigned short usIndex) const;
    /// Vector addition
    Vector3 operator +  (const Vector3<_Precision>& rcVct) const;
    Vector3 operator &  (const Vector3<_Precision>& rcVct) const;
    /// Vector subtraction
    Vector3 operator -  (const Vector3<_Precision>& rcVct) const;
    /// Negative vector
    Vector3 operator - (void) const;
    /// Vector summation
    Vector3 & operator += (const Vector3<_Precision>& rcVct);
    /// Vector subtraction
    Vector3 & operator -= (const Vector3<_Precision>& rcVct);
    /// Vector scaling
    Vector3 operator * (_Precision fScale) const;
    Vector3 operator / (_Precision fDiv) const;
    Vector3 & operator *= (_Precision fScale);
    Vector3 & operator /= (_Precision fDiv);
    /// Assignment
    Vector3 & operator =  (const Vector3<_Precision>& rcVct);
    /// Scalar product
    _Precision operator *  (const Vector3<_Precision>& rcVct) const;
    /// Cross product
    Vector3 operator %  (const Vector3<_Precision>& rcVct) const;
    /// Comparing for inequality
    bool operator != (const Vector3<_Precision>& rcVct) const;
    /// Comparing for equality
    bool operator == (const Vector3<_Precision>& rcVct) const;
    //@}

    /** @name Modification */
    //@{
    void ScaleX (_Precision f);
    void ScaleY (_Precision f);
    void ScaleZ (_Precision f);
    void Scale (_Precision fX, _Precision fY, _Precision fZ);
    void MoveX (_Precision f);
    void MoveY (_Precision f);
    void MoveZ (_Precision f);
    void Move (_Precision fX, _Precision fY, _Precision fZ);
    void RotateX (_Precision f);
    void RotateY (_Precision f);
    void RotateZ (_Precision f);
    //@}

    void Set (_Precision fX, _Precision fY, _Precision fZ);

    /** @name Mathematics */
    //@{
    /// Length of the vector.
    _Precision Length (void) const;
    /// Squared length of the vector.
    _Precision Sqr (void) const;
    /// Set length to 1.
    Vector3 & Normalize (void);
    /// Get angle between both vectors. The returned value lies in the interval [0,pi].
    _Precision GetAngle (const Vector3 &rcVect) const;
    /** Transforms this point to the coordinate system defined by origin \a rclBase, 
    * vector \a vector rclDirX and vector \a vector rclDirY. 
    * \note \a rclDirX must be perpendicular to \a rclDirY, i.e. \a rclDirX * \a rclDirY = 0..
    */
    void TransformToCoordinateSystem (const Vector3 &rclBase, const Vector3 &rclDirX, const Vector3 &rclDirY);
    //bool Equal(const Vector3 &rclVect) const;
    /// Projects this point onto the plane given by the base \a rclBase and the normal \a rclNorm.
    Vector3 & ProjToPlane (const Vector3 &rclBase, const Vector3 &rclNorm);
    /// Projects this point onto the line given by the base \a rclPoint and the direction \a rclLine.
    /**
     * Projects a point \a rclPoint onto the line defined by the origin and the direction \a rclLine.
     * The result is a vector from \a rclPoint to the point on the line. The length of this vector 
     * is the distance from \a rclPoint to the line.
     * Note: The resulting vector does not depend on the current vector.
     */
    Vector3 & ProjToLine (const Vector3 &rclPoint, const Vector3 &rclLine);
    /**
     * Get the perpendicular of this point to the line defined by rclBase and rclDir.
     * Note: Do not mix up this method with ProjToLine.
     */
    Vector3 Perpendicular(const Vector3 &rclBase, const Vector3 &rclDir) const;
    /** Computes the distance to the given plane. Depending on the side this point is located
     * the distance can also be negative. The distance is positive if the point is at the same
     * side the plane normal points to, negative otherwise.
     */
    _Precision DistanceToPlane (const Vector3 &rclBase, const Vector3 &rclNorm) const;
    /// Computes the distance from this point to the line given by \a rclBase and \a rclDirect.
    _Precision DistanceToLine (const Vector3 &rclBase, const Vector3 &rclDirect) const;
    /** Computes the vector from this point to the point on the line segment with the shortest
     * distance. The line segment is defined by \a rclP1 and \a rclP2.
     * Note: If the projection of this point is outside the segment then the shortest distance
     * to \a rclP1 or \a rclP2 is computed.
     */
    Vector3 DistanceToLineSegment (const Vector3& rclP1, const Vector3& rclP2) const;
    //@}
};

// global functions

/// Returns the distance between two points
template <class _Precision>
inline _Precision Distance (const Vector3<_Precision> &v1, const Vector3<_Precision> &v2)
{
    _Precision x=v1.x-v2.x, y=v1.y-v2.y, z=v1.z-v2.z;
    return (_Precision)sqrt((x * x) + (y * y) + (z * z));
}

/// Returns the squared distance between two points
template <class _Precision>
inline _Precision DistanceP2 (const Vector3<_Precision> &v1, const Vector3<_Precision> &v2)
{
    _Precision x=v1.x-v2.x, y=v1.y-v2.y, z=v1.z-v2.z;
    return x * x + y * y + z * z;
}

/// Multiplication of scalar with vector.
template <class _Precision>
inline Vector3<_Precision> operator * (_Precision fFac, const Vector3<_Precision> &rcVct)
{
    return Vector3<_Precision>(rcVct.x * fFac, rcVct.y * fFac, rcVct.z * fFac);
}

template <class _Pr1, class _Pr2>
inline Vector3<_Pr1> toVector(const Vector3<_Pr2>& v)
{
    return Vector3<_Pr1>((_Pr1)v.x,(_Pr1)v.y,(_Pr1)v.z);
};

typedef Vector3<float>  Vector3f;
typedef Vector3<double> Vector3d;

template <class vecT>
struct vec_traits { };

template <>
struct vec_traits<Vector3f> {
    typedef Vector3f vec_type;
    typedef float float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline float_type x() { return v.x; }
    inline float_type y() { return v.y; }
    inline float_type z() { return v.z; }
private:
    const vec_type& v;
};

template <>
struct vec_traits<Vector3d> {
    typedef Vector3d vec_type;
    typedef double float_type;
    vec_traits(const vec_type& v) : v(v){}
    inline float_type x() { return v.x; }
    inline float_type y() { return v.y; }
    inline float_type z() { return v.z; }
private:
    const vec_type& v;
};

template <class _Vec1, class _Vec2>
inline _Vec1 convertTo(const _Vec2& v)
{
    typedef _Vec1 out_type;
    typedef _Vec2 inp_type;
    typedef vec_traits<inp_type> traits_type;
    typedef vec_traits<out_type> traits_out;
    typedef typename traits_out::float_type float_type;
    traits_type t(v);
    return _Vec1((float_type)t.x(),(float_type)t.y(),(float_type)t.z());
};


} // namespace Base

#endif // BASE_VECTOR3D_H

