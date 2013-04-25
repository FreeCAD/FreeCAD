/*
    openDCM, dimensional constraint manager
    Copyright (C) 2012  Stefan Troeger <stefantroeger@gmx.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more detemplate tails.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef GCM_ANGLE_HPP
#define GCM_ANGLE_HPP

#include "geometry.hpp"

namespace dcm {
  
//the calculations( same as we always calculate directions we can outsource the work to this functions)
namespace angle_detail {

template<typename Kernel, typename T>
inline typename Kernel::number_type calc(T d1,
        T d2,
        typename Kernel::number_type angle)  {

    return d1.dot(d2) / (d1.norm()*d2.norm()) - angle;
};


template<typename Kernel, typename T>
inline typename Kernel::number_type calcGradFirst(T d1,
        T d2,
        T dd1)  {

    typename Kernel::number_type norm = d1.norm()*d2.norm();
    return  dd1.dot(d2)/norm - (d1.dot(d2) * (d1.dot(dd1)/d1.norm())*d2.norm()) / std::pow(norm,2);
};

template<typename Kernel, typename T>
inline typename Kernel::number_type calcGradSecond(T d1,
        T d2,
        T dd2)  {

    typename Kernel::number_type norm = d1.norm()*d2.norm();
    return  d1.dot(dd2)/norm - (d1.dot(d2) * (d2.dot(dd2)/d2.norm())*d1.norm()) / std::pow(norm,2);
};

template<typename Kernel, typename T>
inline void calcGradFirstComp(T d1,
                              T d2,
                              T grad)  {

    typename Kernel::number_type norm = d1.norm()*d2.norm();
    grad = d2/norm - (d1.dot(d2)/std::pow(norm,2))*d1/d1.norm();
};

template<typename Kernel, typename T>
inline void calcGradSecondComp(T d1,
                               T d2,
                               T grad)  {

    typename Kernel::number_type norm = d1.norm()*d2.norm();
    grad = d1/norm - (d1.dot(d2)/std::pow(norm,2))*d2/d2.norm();
};

}
/*
template< typename Kernel, typename Tag1, typename Tag2 >
struct Angle3D {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;
    Scalar m_angle;

    Angle3D(Scalar d = 0.) : m_angle(std::cos(d)) {};

    //template definition
    void setScale(Scalar scale){
            assert(false);
    };
    Scalar calculate(Vector& param1,  Vector& param2) {
        assert(false);
    };
    Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
        assert(false);
    };
    Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
        assert(false);
    };
    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
        assert(false);
    };
    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        assert(false);
    };
};

template< typename Kernel >
struct Angle3D< Kernel, tag::line3D, tag::line3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    Scalar m_angle;

    Angle3D(Scalar d = 0.) : m_angle(std::cos(d)) {};
    
    Scalar getEquationScaling(typename Kernel::Vector& local1, typename Kernel::Vector& local2) {
      return 1.;
    }

    //template definition
    Scalar calculate(Vector& param1,  Vector& param2) {
        return angle::calc<Kernel>(param1.template tail<3>(), param2.template tail<3>(), m_angle);
    };
    Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
        return angle::calcGradFirst<Kernel>(param1.template tail<3>(), param2.template tail<3>(), dparam1.template tail<3>());
    };
    Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
        return angle::calcGradSecond<Kernel>(param1.template tail<3>(), param2.template tail<3>(), dparam2.template tail<3>());
    };
    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient.template head<3>().setZero();
        angle::calcGradFirstComp<Kernel>(param1.template tail<3>(), param2.template tail<3>(), gradient.template tail<3>());
    };
    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient.template head<3>().setZero();
        angle::calcGradSecondComp<Kernel>(param1.template tail<3>(), param2.template tail<3>(), gradient.template tail<3>());
    };
};

//planes like lines have the direction as segment 3-5, so we can use the same implementations
template< typename Kernel >
struct Angle3D< Kernel, tag::plane3D, tag::plane3D > : public Angle3D<Kernel, tag::line3D, tag::line3D> {};
template< typename Kernel >
struct Angle3D< Kernel, tag::line3D, tag::plane3D > : public Angle3D<Kernel, tag::line3D, tag::line3D> {};
*/
}

#endif //GCM_ANGLE_HPP