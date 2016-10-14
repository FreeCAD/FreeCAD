/*
    openDCM, dimensional constraint manager
    Copyright (C) 2013  Stefan Troeger <stefantroeger@gmx.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef GCM_DISTANCE_SHAPE3D_H
#define GCM_DISTANCE_SHAPE3D_H

#include <opendcm/core/constraint.hpp>
#include <opendcm/moduleShape3d/geometry.hpp>
#include <math.h>
#include <Eigen/Dense>
#include <Eigen/Geometry>

namespace dcm {

template<typename Kernel>
struct Distance::type< Kernel, tag::point3D, tag::segment3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;
    typedef typename Kernel::Vector3     Vector3;
    typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

    Scalar sc_value, cross_n, cross_v12_n, v12_n;
    typename Distance::options values;
    Vector3 v01, v02, v12, cross;

#ifdef USE_LOGGING
    dcm_logger log;
    attrs::mutable_constant< std::string > tag;

    type() : tag("Distance point3D segment3D") {
        log.add_attribute("Tag", tag);
    };
#endif

    //template definition
    void calculatePseudo(typename Kernel::Vector& point, Vec& v1, typename Kernel::Vector& segment, Vec& v2) {
        Vector3 dir = (segment.template head<3>() - segment.template tail<3>()).normalized();
        Vector3 pp = segment.head(3) + (segment.head(3)-point.head(3)).norm()*dir;
#ifdef USE_LOGGING
        if(!boost::math::isnormal(pp.norm()))
            BOOST_LOG_SEV(log, error) << "Unnormal pseudopoint detected";
#endif
        v2.push_back(pp);
    };
    void setScale(Scalar scale) {
        sc_value = fusion::at_key<double>(values).second*scale;
    };

    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& point, const E::MatrixBase<DerivedB>& segment) {
        //diff = point1 - point2
        v01 = point-segment.template head<3>();
        v02 = point-segment.template tail<3>();
        v12 = segment.template head<3>() - segment.template tail<3>();
        cross = v01.cross(v02);
        v12_n = v12.norm();
        cross_n = cross.norm();
        cross_v12_n = cross_n/std::pow(v12_n,3);

        const Scalar res = cross.norm()/v12_n - sc_value;
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG_SEV(log, error) << "Unnormal residual detected: "<<res;
#endif
        return res;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& point,
                                  const E::MatrixBase<DerivedB>& segment,
                                  const E::MatrixBase<DerivedC>& dpoint) {

        const Vector3 d_point = dpoint; //eigen only acceppts vector3 for cross product
        const Vector3 d_cross = d_point.cross(v02) + v01.cross(d_point);
        const Scalar res = cross.dot(d_cross)/(cross_n*v12_n);
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG_SEV(log, error) << "Unnormal first cluster gradient detected: "<<res
                           <<" with point: "<<point.transpose()<<", segment: "<<segment.transpose()
                           <<" and dpoint: "<<dpoint.transpose();
#endif
        return res;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& point,
                                   const E::MatrixBase<DerivedB>& segment,
                                   const E::MatrixBase<DerivedC>& dsegment) {

        const Vector3 d_cross = - (dsegment.template head<3>().cross(v02) + v01.cross(dsegment.template tail<3>()));
        const Vector3 d_v12   = dsegment.template head<3>() - dsegment.template tail<3>();
        const Scalar res = cross.dot(d_cross)/(cross_n*v12_n) - v12.dot(d_v12)*cross_v12_n;
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG_SEV(log, error) << "Unnormal second cluster gradient detected: "<<res
                           <<" with point: "<<point.transpose()<<", segment: "<<segment.transpose()
                           << "and dsegment: "<<dsegment.transpose();
#endif
        return res;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& point,
                                        const E::MatrixBase<DerivedB>& segment,
                                        E::MatrixBase<DerivedC>& gradient) {
        //gradient = (v02.cross(cross)+cross.cross(v01))/(cross.norm()*v12_n);
        gradient = (v02-v01).cross(cross)/(cross_n*v12_n);
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& point,
                                         const E::MatrixBase<DerivedB>& segment,
                                         E::MatrixBase<DerivedC>& gradient) {
        gradient.template head<3>() = cross.cross(v02)/(cross_n*v12_n) - v12*cross_v12_n;
        gradient.template tail<3>() = v01.cross(cross)/(cross_n*v12_n) + v12*cross_v12_n;
    };
};

}

#endif //GCM_DISTANCE_SHAPE3D_H
