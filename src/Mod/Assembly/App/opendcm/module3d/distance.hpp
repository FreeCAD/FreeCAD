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

#ifndef GCM_DISTANCE3D_H
#define GCM_DISTANCE3D_H

#include "geometry.hpp"
#include <opendcm/core/constraint.hpp>

namespace dcm {

template<typename Kernel>
struct Distance::type< Kernel, tag::point3D, tag::point3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;
    typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

    Scalar value, sc_value;

    //template definition
    void calculatePseudo(typename Kernel::Vector& param1, Vec& v1, typename Kernel::Vector& param2, Vec& v2) {};
    void setScale(Scalar scale) {
        sc_value = value*scale;
    };
    Scalar calculate(Vector& param1,  Vector& param2) {
        return (param1-param2).norm() - sc_value;
    };
    Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
        return (param1-param2).dot(dparam1) / (param1-param2).norm();
    };
    Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
        return (param1-param2).dot(-dparam2) / (param1-param2).norm();
    };
    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient = (param1-param2) / (param1-param2).norm();
    };
    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient = (param2-param1) / (param1-param2).norm();
    };
};

template<typename Kernel>
struct Distance::type< Kernel, tag::point3D, tag::line3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;
    typedef typename Kernel::Vector3     Vector3;
    typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

    Scalar value, sc_value;
    Vector3 diff, n, dist;

#ifdef USE_LOGGING
    src::logger log;
    attrs::mutable_constant< std::string > tag;

    type() : tag("Distance point3D line3D") {
        log.add_attribute("Tag", tag);
    };
#endif

    //template definition
    void calculatePseudo(typename Kernel::Vector& point, Vec& v1, typename Kernel::Vector& line, Vec& v2) {
        Vector3 pp = line.head(3) + (line.head(3)-point.head(3)).norm()*line.template segment<3>(3);
#ifdef USE_LOGGING
        if(!boost::math::isnormal(pp.norm()))
            BOOST_LOG(log) << "Unnormal pseudopoint detected";
#endif
        v2.push_back(pp);
    };
    void setScale(Scalar scale) {
        sc_value = value*scale;
    };
    Scalar calculate(Vector& point, Vector& line) {
        //diff = point1 - point2
        n = line.template segment<3>(3);
        diff = line.template head<3>() - point.template head<3>();
        dist = diff - diff.dot(n)*n;
        const Scalar res = dist.norm() - sc_value;
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG(log) << "Unnormal residual detected: "<<res;
#endif
        return res;
    };

    Scalar calculateGradientFirst(Vector& point, Vector& line, Vector& dpoint) {
        if(dist.norm() == 0)
            return 1.;

        const Vector3 d_diff = -dpoint.template head<3>();
        const Vector3 d_dist = d_diff - d_diff.dot(n)*n;
        const Scalar res = dist.dot(d_dist)/dist.norm();
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG(log) << "Unnormal first cluster gradient detected: "<<res
                           <<" with point: "<<point.transpose()<<", line: "<<line.transpose()
                           <<" and dpoint: "<<dpoint.transpose();
#endif
        return res;
    };

    Scalar calculateGradientSecond(Vector& point, Vector& line, Vector& dline) {
        if(dist.norm() == 0)
            return 1.;

        const Vector3 d_diff = dline.template head<3>();
        const Vector3 d_n  = dline.template segment<3>(3);
        const Vector3 d_dist = d_diff - ((d_diff.dot(n)+diff.dot(d_n))*n + diff.dot(n)*d_n);
        const Scalar res = dist.dot(d_dist)/dist.norm();
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG(log) << "Unnormal second cluster gradient detected: "<<res
                           <<" with point: "<<point.transpose()<<", line: "<<line.transpose()
                           << "and dline: "<<dline.transpose();
#endif
        return res;
    };

    void calculateGradientFirstComplete(Vector& point, Vector& line, Vector& gradient) {
        if(dist.norm() == 0) {
            gradient.head(3).setOnes();
            return;
        }

        const Vector3 res = (n*n.transpose())*dist - dist;
        gradient.head(3) = res/dist.norm();
    };

    void calculateGradientSecondComplete(Vector& point, Vector& line, Vector& gradient) {
        if(dist.norm() == 0) {
            gradient.head(6).setOnes();
            return;
        }

        const Vector3 res = (-n*n.transpose())*dist + dist;
        gradient.head(3) = res/dist.norm();

        const Scalar mult = n.transpose()*dist;
        gradient.template segment<3>(3) = -(mult*diff + diff.dot(n)*dist)/dist.norm();
    };
};

template<typename Kernel>
struct Distance::type< Kernel, tag::point3D, tag::plane3D > {
    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;
    typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

    Scalar value, sc_value;

#ifdef USE_LOGGING
    src::logger log;
    attrs::mutable_constant< std::string > tag;

    type() : tag("Distance point3D plane3D") {
        log.add_attribute("Tag", tag);
    };
#endif

    //template definition
    void calculatePseudo(typename Kernel::Vector& param1, Vec& v1, typename Kernel::Vector& param2, Vec& v2) {
        //typename Kernel::Vector3 pp = param1.head(3)- ((param1.head(3)-param2.head(3)).dot(param2.tail(3)) / param2.tail(3).norm()*(param2.tail(3)));
        //v2.push_back(pp);
        typename Kernel::Vector3 dir = (param1.template head<3>()-param2.template head<3>()).cross(param2.template segment<3>(3));
        dir = param2.template segment<3>(3).cross(dir).normalized();
        typename Kernel::Vector3 pp = param2.head(3) + (param1.head(3)-param2.head(3)).norm()*dir;
        v2.push_back(pp);
#ifdef USE_LOGGING
        if(!boost::math::isnormal(pp.norm()))
            BOOST_LOG(log) << "Unnormal pseudopoint detected";
#endif
    };
    void setScale(Scalar scale) {
        sc_value = value*scale;
    };
    Scalar calculate(Vector& param1,  Vector& param2) {
        //(p1-p2)°n / |n| - distance
        const Scalar res = (param1.head(3)-param2.head(3)).dot(param2.tail(3)) / param2.tail(3).norm() - sc_value;
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG(log) << "Unnormal residual detected: "<<res;
#endif
        return res;
    };

    Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
        //dp1°n / |n|
        //if(dparam1.norm()!=1) return 0;
        const Scalar res = (dparam1.head(3)).dot(param2.tail(3)) / param2.tail(3).norm();
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG(log) << "Unnormal first cluster gradient detected: "<<res;
#endif
        return res;
    };

    Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {

        const typename Kernel::Vector3 p1 = param1.head(3);
        const typename Kernel::Vector3 p2 = param2.head(3);
        const typename Kernel::Vector3 dp2 = dparam2.head(3);
        const typename Kernel::Vector3 n = param2.tail(3);
        const typename Kernel::Vector3 dn = dparam2.tail(3);
        //if(dparam2.norm()!=1) return 0;
        const Scalar res = (((-dp2).dot(n) + (p1-p2).dot(dn)) / n.norm() - (p1-p2).dot(n)* n.dot(dn)/std::pow(n.norm(),3));
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG(log) << "Unnormal second cluster gradient detected: "<<res;
#endif
        return res;
    };

    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient = param2.tail(3) / param2.tail(3).norm();
    };

    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        const typename Kernel::Vector3 p1m2 = param1.head(3) - param2.head(3);
        const typename Kernel::Vector3 n = param2.tail(3);

        gradient.head(3) = -n / n.norm();
        gradient.tail(3) = (p1m2)/n.norm() - (p1m2).dot(n)*n/std::pow(n.norm(),3);
    };
};

template<typename Kernel>
struct Distance::type< Kernel, tag::point3D, tag::cylinder3D > : public Distance::type< Kernel, tag::point3D, tag::line3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

#ifdef USE_LOGGING
    type() {
        Distance::type< Kernel, tag::point3D, tag::line3D >::tag.set("Distance point3D cylinder3D");
    };
#endif
    Scalar calculate(Vector& param1,  Vector& param2) {
        //(p1-p2)°n / |n| - distance
        const Scalar res = Distance::type< Kernel, tag::point3D, tag::line3D >::calculate(param1, param2);
        return res - param2(6);
    };

    void calculateGradientSecondComplete(Vector& p1, Vector& p2, Vector& g) {
        Distance::type< Kernel, tag::point3D, tag::line3D >::calculateGradientSecondComplete(p1,p2,g);
        g(6) = -1;
    };
};

//TODO: this won't work for parallel lines. switch to point-line distance when lines are parallel
template<typename Kernel>
struct Distance::type< Kernel, tag::line3D, tag::line3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;
    typedef typename Kernel::Vector3     Vector3;
    typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

    Scalar value, sc_value, cdn;
    Vector3 c, n1, n2, nxn;

#ifdef USE_LOGGING
    src::logger log;
    attrs::mutable_constant< std::string > tag;

    type() : tag("Distance line3D line3D") {
        log.add_attribute("Tag", tag);
    };
#endif

    //template definition
    void calculatePseudo(typename Kernel::Vector& line1, Vec& v1, typename Kernel::Vector& line2, Vec& v2) {

        //add the line points of shortest distance as pseudopoints
        const Vector3 c = line2.template head<3>() - line1.template head<3>();
        const Vector3 n1 = line1.template segment<3>(3);
        const Vector3 n2 = line2.template segment<3>(3);
        const Vector3 nxn = n1.cross(n2);
        const Vector3 r = c.cross(nxn)/nxn.squaredNorm();
        Vector3 pp1 = line1.template head<3>() + r.dot(n2)*n1;
        Vector3 pp2 = line2.template head<3>() + r.dot(n1)*n2;

#ifdef USE_LOGGING
        if(!boost::math::isfinite(pp1.norm()) || !boost::math::isfinite(pp2.norm()))
            BOOST_LOG(log) << "Unnormal pseudopoint detected";
#endif

        v1.push_back(pp1);
        v2.push_back(pp2);

    };
    void setScale(Scalar scale) {
        sc_value = value*scale;
    };
    Scalar calculate(Vector& line1, Vector& line2) {
        //diff = point1 - point2
        n1 = line1.template segment<3>(3);
        n2 = line2.template segment<3>(3);
        nxn = n1.cross(n2);
        c = line2.template head<3>() - line1.template head<3>();
        cdn = c.dot(nxn);
        const Scalar res = std::abs(cdn) / nxn.norm();
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG(log) << "Unnormal residual detected: "<<res;
#endif
        return res;
    };

    Scalar calculateGradientFirst(Vector& line1, Vector& line2, Vector& dline1) {
        if(nxn.norm() == 0)
            return 1.;

        const Vector3 nxn_diff = dline1.template segment<3>(3).cross(n2);
        Scalar diff = (-dline1.template head<3>().dot(nxn)+c.dot(nxn_diff))*nxn.norm();
        diff -= c.dot(nxn)*nxn.dot(nxn_diff)/nxn.norm();

        //absoulute value requires diffrent differentation for diffrent results
        if(cdn <= 0) diff *= -1;

        diff /= std::pow(nxn.norm(),2);

#ifdef USE_LOGGING
        if(!boost::math::isfinite(diff))
            BOOST_LOG(log) << "Unnormal first cluster gradient detected: "<<diff
                           <<" with line1: "<<line1.transpose()<<", line2: "<<line2.transpose()
                           <<" and dline1: "<<dline1.transpose();
#endif
        return diff;
    };

    Scalar calculateGradientSecond(Vector& line1, Vector& line2, Vector& dline2) {
        if(nxn.norm() == 0)
            return 1.;

        const Vector3 nxn_diff = n1.cross(dline2.template segment<3>(3));
        Scalar diff = (dline2.template head<3>().dot(nxn)+c.dot(nxn_diff))*nxn.norm();
        diff -= c.dot(nxn)*nxn.dot(nxn_diff)/nxn.norm();

        //absoulute value requires diffrent differentation for diffrent results
        if(cdn <= 0) diff *= -1;

        diff /= std::pow(nxn.norm(),2);

#ifdef USE_LOGGING
        if(!boost::math::isfinite(diff))
            BOOST_LOG(log) << "Unnormal first cluster gradient detected: "<<diff
                           <<" with line1: "<<line1.transpose()<<", line2: "<<line2.transpose()
                           <<" and dline2: "<<dline2.transpose();
#endif
        return diff;
    };

    void calculateGradientFirstComplete(Vector& line1, Vector& line2, Vector& gradient) {
        if(nxn.norm() == 0) {
            gradient.head(3).setOnes();
            return;
        }

        if(cdn >= 0) {
            gradient.template head<3>() = -nxn/nxn.norm();
            gradient.template segment<3>(3) = (c.cross(-n2)*nxn.norm()-c.dot(nxn)*n2.cross(nxn)/nxn.norm())/std::pow(nxn.norm(),2);
        } else {
            gradient.template head<3>() = nxn/nxn.norm();
            gradient.template segment<3>(3) = (-c.cross(-n2)*nxn.norm()+c.dot(nxn)*n2.cross(nxn)/nxn.norm())/std::pow(nxn.norm(),2);
        }
    };

    void calculateGradientSecondComplete(Vector& line1, Vector& line2, Vector& gradient) {
        if(nxn.norm() == 0) {
            gradient.head(3).setOnes();
            return;
        }

        if(cdn >= 0) {
            gradient.template head<3>() = nxn/nxn.norm();
            gradient.template segment<3>(3) = (c.cross(n1)*nxn.norm()-c.dot(nxn)*((-n1).cross(nxn))/nxn.norm())/std::pow(nxn.norm(),2);
        } else {
            gradient.template head<3>() = -nxn/nxn.norm();
            gradient.template segment<3>(3) = (-c.cross(n1)*nxn.norm()+c.dot(nxn)*((-n1).cross(nxn))/nxn.norm())/std::pow(nxn.norm(),2);
        }
    };
};

//only defined when line and plane are parallel, therefore it's the same as the point-plane distance
template<typename Kernel>
struct Distance::type< Kernel, tag::line3D, tag::plane3D > : public Distance::type< Kernel, tag::point3D, tag::plane3D > {

#ifdef USE_LOGGING
    type() : Distance::type< Kernel, tag::point3D, tag::plane3D >() {
        Distance::type< Kernel, tag::point3D, tag::plane3D >::tag.set("Distance line3D plane3D");
    };
#endif
    typedef typename Kernel::VectorMap Vector;
    void calculateGradientFirstComplete(Vector& p1, Vector& p2, Vector& g) {
        Distance::type< Kernel, tag::point3D, tag::plane3D >::calculateGradientFirstComplete(p1,p2,g);
        g.segment(3,3).setZero();
    };
};

template<typename Kernel>
struct Distance::type< Kernel, tag::line3D, tag::cylinder3D > : public Distance::type< Kernel, tag::line3D, tag::line3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

#ifdef USE_LOGGING
    type() : Distance::type< Kernel, tag::line3D, tag::line3D >() {
        Distance::type< Kernel, tag::line3D, tag::line3D >::tag.set("Distance line3D cylinder3D");
    };
#endif

    Scalar calculate(Vector& param1,  Vector& param2) {
        //(p1-p2)°n / |n| - distance
        const Scalar res = Distance::type< Kernel, tag::line3D, tag::line3D >::calculate(param1, param2);
        return res - param2(6);
    };

    void calculateGradientSecondComplete(Vector& p1, Vector& p2, Vector& g) {
        Distance::type< Kernel, tag::line3D, tag::line3D >::calculateGradientSecondComplete(p1,p2,g);
        g(6) = -1;
    };
};

//only defined when planes are parallel, therefore it's the same as the point-plane distance
template<typename Kernel>
struct Distance::type< Kernel, tag::plane3D, tag::plane3D > : public Distance::type< Kernel, tag::point3D, tag::plane3D > {

#ifdef USE_LOGGING
    type() : Distance::type< Kernel, tag::point3D, tag::plane3D >() {
        Distance::type< Kernel, tag::point3D, tag::plane3D >::tag.set("Distance plane3D plane3D");
    };
#endif
    typedef typename Kernel::VectorMap Vector;
    void calculateGradientFirstComplete(Vector& p1, Vector& p2, Vector& g) {
        Distance::type< Kernel, tag::point3D, tag::plane3D >::calculateGradientFirstComplete(p1,p2,g);
        g.segment(3,3).setZero();
    };
};

//only defined when plane and cylinder are parallel, therefore it's the same as the point-plane distance
template<typename Kernel>
struct Distance::type< Kernel, tag::plane3D, tag::cylinder3D > : public Distance::type< Kernel, tag::point3D, tag::plane3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

#ifdef USE_LOGGING
    type() : Distance::type< Kernel, tag::point3D, tag::plane3D >() {
        Distance::type< Kernel, tag::point3D, tag::plane3D >::tag.set("Distance plane3D cylinder3D");
    };
#endif

    Scalar calculate(Vector& param1,  Vector& param2) {
        //(p1-p2)°n / |n| - distance
        const Scalar res = Distance::type< Kernel, tag::point3D, tag::plane3D >::calculate(param2, param1);
        return res - param2(6);
    };

    Scalar calculateGradientFirst(Vector& p1, Vector& p2, Vector& dp1) {
        return Distance::type< Kernel, tag::point3D, tag::plane3D >::calculateGradientSecond(p2,p1,dp1);
    };

    Scalar calculateGradientSecond(Vector& p1, Vector& p2, Vector& dp2) {
        return Distance::type< Kernel, tag::point3D, tag::plane3D >::calculateGradientFirst(p2,p1,dp2);
    };

    void calculateGradientFirstComplete(Vector& p1, Vector& p2, Vector& g) {
        Distance::type< Kernel, tag::point3D, tag::plane3D >::calculateGradientSecondComplete(p2,p1,g);
    };

    void calculateGradientSecondComplete(Vector& p1, Vector& p2, Vector& g) {
        Distance::type< Kernel, tag::point3D, tag::plane3D >::calculateGradientFirstComplete(p2,p1,g);
        g.segment(3,3).setZero();
        g(6) = -1;
    };
};

template<typename Kernel>
struct Distance::type< Kernel, tag::cylinder3D, tag::cylinder3D > : public Distance::type< Kernel, tag::line3D, tag::line3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

#ifdef USE_LOGGING
    type() : Distance::type< Kernel, tag::line3D, tag::line3D >() {
        Distance::type< Kernel, tag::line3D, tag::line3D >::tag.set("Distance cylinder3D cylinder3D");
    };
#endif

    Scalar calculate(Vector& param1,  Vector& param2) {
        //(p1-p2)°n / |n| - distance
        const Scalar res = Distance::type< Kernel, tag::line3D, tag::line3D >::calculate(param1, param2);
        return res - param1(6) - param2(6);
    };

    void calculateGradientFirstComplete(Vector& p1, Vector& p2, Vector& g) {
        Distance::type< Kernel, tag::line3D, tag::line3D >::calculateGradientFirstComplete(p1,p2,g);
        g(6) = -1;
    };

    void calculateGradientSecondComplete(Vector& p1, Vector& p2, Vector& g) {
        Distance::type< Kernel, tag::line3D, tag::line3D >::calculateGradientSecondComplete(p1,p2,g);
        g(6) = -1;
    };
};

}//namespace dcm

#endif //GCM_DISTANCE3D_H
