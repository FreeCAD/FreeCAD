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
#include <boost/fusion/include/copy.hpp>
#include <boost/math/special_functions.hpp>

namespace dcm {

template<typename Kernel>
struct Distance::type< Kernel, tag::point3D, tag::point3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;
    typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

    Scalar sc_value;
    typename Distance::options values;

    //template definition
    void calculatePseudo(typename Kernel::Vector& param1, Vec& v1, typename Kernel::Vector& param2, Vec& v2) {};
    void setScale(Scalar scale) {
        sc_value = fusion::at_key<double>(values).second*scale;
    };
    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
        return (param1-param2).norm() - sc_value;
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& param1,
                                  const E::MatrixBase<DerivedB>& param2,
                                  const E::MatrixBase<DerivedC>& dparam1) {
        return (param1-param2).dot(dparam1) / (param1-param2).norm();
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& param1,
                                   const E::MatrixBase<DerivedB>& param2,
                                   const E::MatrixBase<DerivedC>& dparam2) {
        return (param1-param2).dot(-dparam2) / (param1-param2).norm();
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& param1,
                                        const E::MatrixBase<DerivedB>& param2,
                                        E::MatrixBase<DerivedC>& gradient) {
        gradient = (param1-param2) / (param1-param2).norm();
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                         const E::MatrixBase<DerivedB>& param2,
                                         E::MatrixBase<DerivedC>& gradient) {
        gradient = (param2-param1) / (param1-param2).norm();
    };
};

template<typename Kernel>
struct Distance::type< Kernel, tag::point3D, tag::line3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;
    typedef typename Kernel::Vector3     Vector3;
    typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

    Scalar sc_value;
    typename Distance::options values;
    Vector3 diff, n, dist;

#ifdef USE_LOGGING
    dcm_logger log;
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
            BOOST_LOG_SEV(log, error) << "Unnormal pseudopoint detected";
#endif
        v2.push_back(pp);
    };
    void setScale(Scalar scale) {
        sc_value = fusion::at_key<double>(values).second*scale;
    };
    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& point,  const E::MatrixBase<DerivedB>& line) {
        //diff = point1 - point2
        n = line.template segment<3>(3);
        diff = line.template head<3>() - point.template head<3>();
        dist = diff - diff.dot(n)*n;
        const Scalar res = dist.norm() - sc_value;
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG_SEV(log, error) << "Unnormal residual detected: "<<res;
#endif
        return res;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& point,
                                  const E::MatrixBase<DerivedB>& line,
                                  const E::MatrixBase<DerivedC>& dpoint) {
        if(dist.norm() == 0)
            return 1.;

        const Vector3 d_diff = -dpoint.template head<3>();
        const Vector3 d_dist = d_diff - d_diff.dot(n)*n;
        const Scalar res = dist.dot(d_dist)/dist.norm();
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG_SEV(log, error) << "Unnormal first cluster gradient detected: "<<res
                           <<" with point: "<<point.transpose()<<", line: "<<line.transpose()
                           <<" and dpoint: "<<dpoint.transpose();
#endif
        return res;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& point,
                                   const E::MatrixBase<DerivedB>& line,
                                   const E::MatrixBase<DerivedC>& dline) {
        if(dist.norm() == 0)
            return 1.;

        const Vector3 d_diff = dline.template head<3>();
        const Vector3 d_n  = dline.template segment<3>(3);
        const Vector3 d_dist = d_diff - ((d_diff.dot(n)+diff.dot(d_n))*n + diff.dot(n)*d_n);
        const Scalar res = dist.dot(d_dist)/dist.norm();
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG_SEV(log, error) << "Unnormal second cluster gradient detected: "<<res
                           <<" with point: "<<point.transpose()<<", line: "<<line.transpose()
                           << "and dline: "<<dline.transpose();
#endif
        return res;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& point,
                                        const E::MatrixBase<DerivedB>& line,
                                        E::MatrixBase<DerivedC>& gradient) {
        if(dist.norm() == 0) {
            gradient.head(3).setOnes();
            return;
        }

        const Vector3 res = (n*n.transpose())*dist - dist;
        gradient.head(3) = res/dist.norm();
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& point,
                                         const E::MatrixBase<DerivedB>& line,
                                         E::MatrixBase<DerivedC>& gradient) {
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

    Scalar sc_value, result;
    SolutionSpace sspace;
    typename Distance::options values;

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
            BOOST_LOG_SEV(log, error) << "Unnormal pseudopoint detected";
#endif
    };
    void setScale(Scalar scale) {
        sc_value = fusion::at_key<double>(values).second*scale;
        sspace = fusion::at_key<SolutionSpace>(values).second;
    };
    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
        //(p1-p2)°n / |n| - distance
        result = (param1.head(3)-param2.head(3)).dot(param2.tail(3)) / param2.tail(3).norm();

        if(sspace == bidirectional)
            return std::abs(result) - sc_value;

        if(sspace == positiv_directional)
            return result - sc_value;

        if(sspace == negative_directional)
            return result + sc_value;
#ifdef USE_LOGGING
        if(!boost::math::isfinite(result))
            BOOST_LOG_SEV(log, error) << "Unnormal residual detected: " << result;
#endif
        return result;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& param1,
                                  const E::MatrixBase<DerivedB>& param2,
                                  const E::MatrixBase<DerivedC>& dparam1) {
        //dp1°n / |n|
        //if(dparam1.norm()!=1) return 0;
        const Scalar res = (dparam1.head(3)).dot(param2.tail(3)) / param2.tail(3).norm();
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG_SEV(log, error) << "Unnormal first cluster gradient detected: "<<res;
#endif
        //r  = sqrt(x^2) = (x^2)^(1/2)
        //r' = 1/2(x^2)^(-1/2) * (x^2)'
        //r' = 1/sqrt(x^2) * x * x'
        //r' = sign(x)*x'
        if(sspace == bidirectional && result<0.)
            return -res;

        return res;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& param1,
                                   const E::MatrixBase<DerivedB>& param2,
                                   const E::MatrixBase<DerivedC>& dparam2) {

        const typename Kernel::Vector3 p1 = param1.head(3);
        const typename Kernel::Vector3 p2 = param2.head(3);
        const typename Kernel::Vector3 dp2 = dparam2.head(3);
        const typename Kernel::Vector3 n = param2.tail(3);
        const typename Kernel::Vector3 dn = dparam2.tail(3);
        //if(dparam2.norm()!=1) return 0;
        const Scalar res = (((-dp2).dot(n) + (p1-p2).dot(dn)) / n.norm() - (p1-p2).dot(n)* n.dot(dn)/std::pow(n.norm(),3));
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG_SEV(log, error) << "Unnormal second cluster gradient detected: "<<res;
#endif
        if(sspace == bidirectional && result<0.)
            return -res;

        return res;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& param1,
                                        const E::MatrixBase<DerivedB>& param2,
                                        E::MatrixBase<DerivedC>& gradient) {
        gradient = param2.tail(3) / param2.tail(3).norm();

        if(sspace == bidirectional && result<0.)
            gradient *= -1.;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                         const E::MatrixBase<DerivedB>& param2,
                                         E::MatrixBase<DerivedC>& gradient) {
        const typename Kernel::Vector3 p1m2 = param1.head(3) - param2.head(3);
        const typename Kernel::Vector3 n = param2.tail(3);

        gradient.head(3) = -n / n.norm();
        gradient.tail(3) = (p1m2)/n.norm() - (p1m2).dot(n)*n/std::pow(n.norm(),3);

        if(sspace == bidirectional && result<0.)
            gradient *= -1.;
    };
};

template<typename Kernel>
struct Distance::type< Kernel, tag::point3D, tag::cylinder3D > : public Distance::type< Kernel, tag::point3D, tag::line3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    Scalar result;
    SolutionSpace sspace;
    using Distance::template type<Kernel, tag::point3D, tag::line3D>::sc_value;
    using Distance::template type<Kernel, tag::point3D, tag::line3D>::values;
#ifdef USE_LOGGING
    type() {
        Distance::template type< Kernel, tag::point3D, tag::line3D >::tag.set("Distance point3D cylinder3D");
    };
#endif

    void setScale(Scalar scale) {
        Distance::template type<Kernel, tag::point3D, tag::line3D>::setScale(scale);
        sspace = fusion::at_key<SolutionSpace>(values).second;
    };

    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
        //(p1-p2)°n / |n| - distance
        result = Distance::type< Kernel, tag::point3D, tag::line3D >::calculate(param1, param2) - param2(6);

        if(sspace==negative_directional || (sspace == bidirectional && (result+sc_value)<0.))
            return result+2*sc_value;

        return result;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& param1,
                                  const E::MatrixBase<DerivedB>& param2,
                                  const E::MatrixBase<DerivedC>& dparam1) {

        return Distance::type< Kernel, tag::point3D, tag::line3D >::calculateGradientFirst(param1,param2,dparam1);
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& param1,
                                   const E::MatrixBase<DerivedB>& param2,
                                   const E::MatrixBase<DerivedC>& dparam2) {

        return Distance::type< Kernel, tag::point3D, tag::line3D >::calculateGradientSecond(param1,param2,dparam2) - dparam2(6);
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& param1,
                                        const E::MatrixBase<DerivedB>& param2,
                                        E::MatrixBase<DerivedC>& gradient) {
        Distance::type< Kernel, tag::point3D, tag::line3D >::calculateGradientFirstComplete(param1,param2,gradient);
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& p1,
                                         const E::MatrixBase<DerivedB>& p2,
                                         E::MatrixBase<DerivedC>& g) {
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

    Scalar sc_value, cdn, nxn_n;
    typename Distance::options values;
    Vector3 c, n1, n2, nxn;

    //if the lines are parallel we need to fall back to point-line distance
    //to do this efficiently we just hold a point-line distance equation and use it instead
    Distance::type<Kernel, tag::point3D, tag::line3D> pl_eqn;

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

        //parallel lines are treated as point line
        if(Kernel::isSame(nxn.norm(), 0, 1e-6)) {
            pl_eqn.calculatePseudo(line1, v1, line2, v2);
            return;
        };

        const Vector3 r = c.cross(nxn)/nxn.squaredNorm();
        Vector3 pp1 = line1.template head<3>() + r.dot(n2)*n1;
        Vector3 pp2 = line2.template head<3>() + r.dot(n1)*n2;

#ifdef USE_LOGGING
        if(!boost::math::isfinite(pp1.norm()) || !boost::math::isfinite(pp2.norm()))
            BOOST_LOG_SEV(log, error) << "Unnormal pseudopoint detected";
#endif

        v1.push_back(pp1);
        v2.push_back(pp2);

    };
    void setScale(Scalar scale) {
        sc_value = fusion::at_key<double>(values).second*scale;
        fusion::copy(values, pl_eqn.values);
        pl_eqn.setScale(scale);
    };
    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& line1, const E::MatrixBase<DerivedB>& line2) {
        //diff = point1 - point2
        n1 = line1.template segment<3>(3);
        n2 = line2.template segment<3>(3);
        nxn = n1.cross(n2);
        nxn_n = nxn.norm();

        if(Kernel::isSame(nxn_n, 0, 1e-6))
            return pl_eqn.calculate(line1, line2);

        c = line2.template head<3>() - line1.template head<3>();
        cdn = c.dot(nxn);
        const Scalar res = std::abs(cdn) / nxn.norm();
#ifdef USE_LOGGING
        if(!boost::math::isfinite(res))
            BOOST_LOG_SEV(log, error) << "Unnormal residual detected: "<<res;
#endif
        return res;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& line1,
                                  const E::MatrixBase<DerivedB>& line2,
                                  const E::MatrixBase<DerivedC>& dline1) {
        if(Kernel::isSame(nxn_n, 0, 1e-6))
            return pl_eqn.calculateGradientFirst(line1, line2, dline1);

        const Vector3 nxn_diff = dline1.template segment<3>(3).cross(n2);
        Scalar diff = (-dline1.template head<3>().dot(nxn)+c.dot(nxn_diff))*nxn_n;
        diff -= c.dot(nxn)*nxn.dot(nxn_diff)/nxn_n;

        //absoulute value requires different differentation for different results
        if(cdn <= 0)
            diff *= -1;

        diff /= std::pow(nxn_n,2);

#ifdef USE_LOGGING
        if(!boost::math::isfinite(diff))
            BOOST_LOG_SEV(log, error) << "Unnormal first cluster gradient detected: "<<diff
                           <<" with line1: "<<line1.transpose()<<", line2: "<<line2.transpose()
                           <<" and dline1: "<<dline1.transpose();
#endif
        return diff;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& line1,
                                   const E::MatrixBase<DerivedB>& line2,
                                   const E::MatrixBase<DerivedC>& dline2) {
        if(Kernel::isSame(nxn_n, 0, 1e-6))
            return pl_eqn.calculateGradientSecond(line1, line2, dline2);

        const Vector3 nxn_diff = n1.cross(dline2.template segment<3>(3));
        Scalar diff = (dline2.template head<3>().dot(nxn)+c.dot(nxn_diff))*nxn_n;
        diff -= c.dot(nxn)*nxn.dot(nxn_diff)/nxn_n;

        //absoulute value requires different differentation for different results
        if(cdn <= 0)
            diff *= -1;

        diff /= std::pow(nxn_n,2);

#ifdef USE_LOGGING
        if(!boost::math::isfinite(diff))
            BOOST_LOG_SEV(log, error) << "Unnormal first cluster gradient detected: "<<diff
                           <<" with line1: "<<line1.transpose()<<", line2: "<<line2.transpose()
                           <<" and dline2: "<<dline2.transpose();
#endif
        return diff;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& line1,
                                        const E::MatrixBase<DerivedB>& line2,
                                        E::MatrixBase<DerivedC>& gradient) {
        if(Kernel::isSame(nxn_n, 0, 1e-6)) {
            pl_eqn.calculateGradientFirstComplete(line1, line2, gradient);
            return;
        }

        if(cdn >= 0) {
            gradient.template head<3>() = -nxn/nxn_n;
            gradient.template segment<3>(3) = (c.cross(-n2)*nxn_n-c.dot(nxn)*n2.cross(nxn)/nxn_n)/std::pow(nxn_n,2);
        }
        else {
            gradient.template head<3>() = nxn/nxn_n;
            gradient.template segment<3>(3) = (-c.cross(-n2)*nxn_n+c.dot(nxn)*n2.cross(nxn)/nxn_n)/std::pow(nxn_n,2);
        }
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& line1,
                                         const E::MatrixBase<DerivedB>& line2,
                                         E::MatrixBase<DerivedC>& gradient) {
        if(Kernel::isSame(nxn_n, 0, 1e-6)) {
            pl_eqn.calculateGradientFirstComplete(line1, line2, gradient);
            return;
        }

        if(cdn >= 0) {
            gradient.template head<3>() = nxn/nxn_n;
            gradient.template segment<3>(3) = (c.cross(n1)*nxn_n-c.dot(nxn)*((-n1).cross(nxn))/nxn_n)/std::pow(nxn_n,2);
        }
        else {
            gradient.template head<3>() = -nxn/nxn_n;
            gradient.template segment<3>(3) = (-c.cross(n1)*nxn_n+c.dot(nxn)*((-n1).cross(nxn))/nxn_n)/std::pow(nxn_n,2);
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

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& p1,
                                        const E::MatrixBase<DerivedB>& p2,
                                        E::MatrixBase<DerivedC>& g) {
        typename Kernel::VectorMap grad(&g(0), 3, typename Kernel::DynStride(1,1));
        Distance::type< Kernel, tag::point3D, tag::plane3D >::calculateGradientFirstComplete(p1,p2,grad);
        g.segment(3,3).setZero();
    };
};

template<typename Kernel>
struct Distance::type< Kernel, tag::line3D, tag::cylinder3D > : public Distance::type< Kernel, tag::line3D, tag::line3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;
    using Distance::type< Kernel, tag::line3D, tag::line3D >::sc_value;
    Scalar result;
    SolutionSpace sspace;

#ifdef USE_LOGGING
    type() : Distance::type< Kernel, tag::line3D, tag::line3D >() {
        Distance::type< Kernel, tag::line3D, tag::line3D >::tag.set("Distance line3D cylinder3D");
    };
#endif

    void setScale(Scalar scale) {
        Distance::type< Kernel, tag::line3D, tag::line3D >::setScale(scale);
        sspace = fusion::at_key<SolutionSpace>(Distance::type< Kernel, tag::line3D, tag::line3D >::values).second;
    };

    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
        //(p1-p2)°n / |n| - distance
        result = Distance::type< Kernel, tag::line3D, tag::line3D >::calculate(param1, param2) - param2(6);

        //for parallel line and cylinder we may use the solution space methods
        if(Kernel::isSame(Distance::type< Kernel, tag::line3D, tag::line3D >::nxn_n, 0, 1e-6) &&
                (sspace==negative_directional || (sspace == bidirectional && (result+sc_value)<0.)))
            return result+2*sc_value;

        return result;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& p1,
                                         const E::MatrixBase<DerivedB>& p2,
                                         E::MatrixBase<DerivedC>& g) {
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

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& p1,
                                        const E::MatrixBase<DerivedB>& p2,
                                        E::MatrixBase<DerivedC>& g) {
        typename Kernel::VectorMap grad(&g(0), 3, typename Kernel::DynStride(1,1));
        Distance::type< Kernel, tag::point3D, tag::plane3D >::calculateGradientFirstComplete(p1,p2,grad);
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

    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
        //(p1-p2)°n / |n| - distance
        const Scalar res = Distance::type< Kernel, tag::point3D, tag::plane3D >::calculate(param2, param1);

        if(Distance::type< Kernel, tag::point3D, tag::plane3D >::sspace == negative_directional)
            return res + param2(6);

        return res - param2(6);
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& p1,
                                  const E::MatrixBase<DerivedB>& p2,
                                  const E::MatrixBase<DerivedC>& dp1) {
        return Distance::type< Kernel, tag::point3D, tag::plane3D >::calculateGradientSecond(p2,p1,dp1);
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& p1,
                                   const E::MatrixBase<DerivedB>& p2,
                                   const E::MatrixBase<DerivedC>& dp2) {
        return Distance::type< Kernel, tag::point3D, tag::plane3D >::calculateGradientFirst(p2,p1,dp2);
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& p1,
                                        const E::MatrixBase<DerivedB>& p2,
                                        E::MatrixBase<DerivedC>& g) {
        Distance::type< Kernel, tag::point3D, tag::plane3D >::calculateGradientSecondComplete(p2,p1,g);
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& p1,
                                         const E::MatrixBase<DerivedB>& p2,
                                         E::MatrixBase<DerivedC>& g) {
        typename Kernel::VectorMap grad(&g(0), 3, typename Kernel::DynStride(1,1));
        Distance::type< Kernel, tag::point3D, tag::plane3D >::calculateGradientFirstComplete(p2,p1,grad);
        g.segment(3,3).setZero();

        if(Distance::type< Kernel, tag::point3D, tag::plane3D >::sspace == negative_directional)
            g(6) = 1;
        else
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

    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
        //(p1-p2)°n / |n| - distance
        const Scalar res = Distance::type< Kernel, tag::line3D, tag::line3D >::calculate(param1, param2);
        return res - param1(6) - param2(6);
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& p1,
                                        const E::MatrixBase<DerivedB>& p2,
                                        E::MatrixBase<DerivedC>& g) {
        Distance::type< Kernel, tag::line3D, tag::line3D >::calculateGradientFirstComplete(p1,p2,g);
        g(6) = -1;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& p1,
                                         const E::MatrixBase<DerivedB>& p2,
                                         E::MatrixBase<DerivedC>& g) {
        Distance::type< Kernel, tag::line3D, tag::line3D >::calculateGradientSecondComplete(p1,p2,g);
        g(6) = -1;
    };
};

}//namespace dcm

#endif //GCM_DISTANCE3D_H

