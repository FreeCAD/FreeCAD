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
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef DCM_CLUSTERMATH_IMP_H
#define DCM_CLUSTERMATH_IMP_H

#include "../clustermath.hpp"

#ifdef DCM_EXTERNAL_CORE
#include "opendcm/core/imp/transformation_imp.hpp"
#endif

//include it here as it is in the same external compilation unit as clustermath
#include "solver_imp.hpp"

namespace dcm {
namespace details {
  
template<typename Sys>
ClusterMath<Sys>::ClusterMath() : m_normQ(NULL), m_translation(NULL), init(false) {

    m_resetTransform = Eigen::AngleAxisd(M_PI*2./3., Eigen::Vector3d(1,1,1).normalized());
    m_shift.setZero();

#ifdef USE_LOGGING
    log.add_attribute("Tag", attrs::constant< std::string >("Clustermath3D"));
#endif
};

template<typename Sys>
void ClusterMath<Sys>::setParameterOffset(int offset, dcm::AccessType t) {
    if(t == general)
        m_offset = offset;
    else
        m_offset_rot = offset;
};

template<typename Sys>
int ClusterMath<Sys>::getParameterOffset(AccessType t) {
    if(t == general)
        return m_offset;
    else
        return m_offset_rot;
};

template<typename Sys>
typename ClusterMath<Sys>::Kernel::Vector3Map& ClusterMath<Sys>::getNormQuaternionMap() {
    return m_normQ;
};

template<typename Sys>
typename ClusterMath<Sys>::Kernel::Vector3Map& ClusterMath<Sys>::getTranslationMap() {
    return m_translation;
};

template<typename Sys>
void ClusterMath<Sys>::initMaps() {

    transformToMaps(m_transform);
    init = true;
    midpoint.setZero();
    m_shift.setZero();
    m_ssrTransform.setIdentity();
    m_diffTrans = m_transform;
    fix=false;
#ifdef USE_LOGGING
    BOOST_LOG(log) << "Init transform: "<<m_transform;
#endif
};

template<typename Sys>
void ClusterMath<Sys>::initFixMaps() {
    //when fixed no maps exist
    new(&m_translation) typename Kernel::Vector3Map(&fixtrans(0));
    m_translation = m_transform.translation().vector();
    init = true;
    midpoint.setZero();
    m_shift.setZero();
    m_ssrTransform.setIdentity();
    m_diffTrans = m_transform;
    fix=true;
#ifdef USE_LOGGING
    BOOST_LOG(log) << "Init fix transform: "<<m_transform;
#endif
};

template<typename Sys>
typename ClusterMath<Sys>::Kernel::Transform3D& ClusterMath<Sys>::getTransform() {
    return m_transform;
};

template<typename Sys>
typename ClusterMath<Sys>::Kernel::Transform3D::Translation const& ClusterMath<Sys>::getTranslation() const {
    return m_transform.translation();
};

template<typename Sys>
typename ClusterMath<Sys>::Kernel::Transform3D::Rotation const& ClusterMath<Sys>::getRotation() const {
    return m_transform.rotation();
};

template<typename Sys>
void ClusterMath<Sys>::setTransform(typename ClusterMath<Sys>::Kernel::Transform3D const& t) {
    m_transform = t;
};

template<typename Sys>
void ClusterMath<Sys>::setTranslation(typename ClusterMath<Sys>::Kernel::Transform3D::Translation const& t) {
    m_transform.setTranslation(t);
};

template<typename Sys>
void ClusterMath<Sys>::setRotation(typename ClusterMath<Sys>::Kernel::Transform3D::Rotation const& r) {
    m_transform.setRotation(r);
};

template<typename Sys>
void ClusterMath<Sys>::mapsToTransform(typename ClusterMath<Sys>::Kernel::Transform3D& trans) {
    //add scale only after possible reset
    typename Kernel::Transform3D::Scaling scale(m_transform.scaling());
    trans = m_diffTrans;
    trans *= scale;
};

template<typename Sys>
void ClusterMath<Sys>::transformToMaps(typename ClusterMath<Sys>::Kernel::Transform3D& trans) {

    const typename Kernel::Quaternion& m_quaternion = trans.rotation();
    if(m_quaternion.w() < 1.) {
        Scalar s = std::acos(m_quaternion.w())/std::sin(std::acos(m_quaternion.w()));
        m_normQ = m_quaternion.vec()*s;
        m_normQ /= NQFAKTOR;
    } else {
        m_normQ.setZero();
    }
    m_translation = trans.translation().vector();
};

template<typename Sys>
void ClusterMath<Sys>::finishCalculation() {

#ifdef USE_LOGGING
    BOOST_LOG(log) << "Finish calculation";
#endif

    mapsToTransform(m_transform);
    init=false;

    m_transform = m_ssrTransform*m_transform;

    //scale all geometries back to the original size
    m_diffTrans *= typename Kernel::Transform3D::Scaling(1./m_ssrTransform.scaling().factor());
    typedef typename std::vector<Geom>::iterator iter;
    for(iter it = m_geometry.begin(); it != m_geometry.end(); it++)
        (*it)->recalculate(m_diffTrans);
#ifdef USE_LOGGING
    BOOST_LOG(log) << "Finish transform:"<<std::endl<<m_transform;
#endif
};

template<typename Sys>
void ClusterMath<Sys>::finishFixCalculation() {
#ifdef USE_LOGGING
    BOOST_LOG(log) << "Finish fix calculation";
#endif
    typedef typename std::vector<Geom>::iterator iter;
    m_transform *= m_ssrTransform.inverse();
    typename Kernel::DiffTransform3D diff(m_transform);
    for(iter it = m_geometry.begin(); it != m_geometry.end(); it++)
        (*it)->recalculate(diff);
#ifdef USE_LOGGING
    BOOST_LOG(log) << "Finish fix transform:"<<std::endl<<m_transform;
#endif
};

template<typename Sys>
void ClusterMath<Sys>::resetClusterRotation(typename ClusterMath<Sys>::Kernel::Transform3D& trans) {

#ifdef USE_LOGGING
    BOOST_LOG(log) << "Reset cluster rotation:"<<std::endl<<m_diffTrans;
#endif

    trans  = m_resetTransform.inverse()*trans;
    m_ssrTransform *= m_resetTransform;
    //m_transform = m_resetTransform.inverse()*m_transform;

    //apply the needed transformation to all geometries local values
    typedef typename std::vector<Geom>::iterator iter;
    for(iter it = m_geometry.begin(); it != m_geometry.end(); it++) {
        (*it)->transform(m_resetTransform);
    };
};

template<typename Sys>
void ClusterMath<Sys>::calcDiffTransform(typename ClusterMath<Sys>::Kernel::DiffTransform3D& trans) {

    Scalar norm = m_normQ.norm();
    trans.setIdentity();

    if(norm < 0.1) {
        if(norm == 0) {
            trans *= typename Kernel::Transform3D::Translation(m_translation);
            resetClusterRotation(trans);
        } else {
            const Scalar fac = std::sin(NQFAKTOR*norm)/norm;
            trans =  typename Kernel::Quaternion(std::cos(NQFAKTOR*norm), m_normQ(0)*fac, m_normQ(1)*fac,m_normQ(2)*fac);
            trans *= typename Kernel::Transform3D::Translation(m_translation);
            resetClusterRotation(trans);
        }
        transformToMaps(trans);
        return;
    }

    const Scalar fac = std::sin(NQFAKTOR*norm)/norm;
    trans = typename Kernel::Quaternion(std::cos(NQFAKTOR*norm), m_normQ(0)*fac, m_normQ(1)*fac, m_normQ(2)*fac);
    trans *= typename Kernel::Transform3D::Translation(m_translation);
};

template<typename Sys>
void ClusterMath<Sys>::recalculate() {


    calcDiffTransform(m_diffTrans);

    const typename Kernel::Quaternion Q = m_diffTrans.rotation();

    // now calculate the gradient quaternions and calculate the diff rotation matrices
    // m_normQ = (a,b,c)
    // n = ||m_normQ||
    //
    // Q = (a/n sin(n), b/n sin(n), c/n sin(n), cos(n))
    //

    //n=||m_normQ||, sn = sin(n)/n, sn3 = sin(n)/n^3, cn = cos(n)/n, divn = 1/n;
    const Scalar n    = m_normQ.norm();
    const Scalar sn   = std::sin(NQFAKTOR*n)/n;
    const Scalar mul  = (NQFAKTOR*std::cos(NQFAKTOR*n)-sn)/std::pow(n,2);

    //dxa = dQx/da
    const Scalar dxa = sn + std::pow(m_normQ(0),2)*mul;
    const Scalar dxb = m_normQ(0)*m_normQ(1)*mul;
    const Scalar dxc = m_normQ(0)*m_normQ(2)*mul;

    const Scalar dya = m_normQ(1)*m_normQ(0)*mul;
    const Scalar dyb = sn + std::pow(m_normQ(1),2)*mul;
    const Scalar dyc = m_normQ(1)*m_normQ(2)*mul;

    const Scalar dza = m_normQ(2)*m_normQ(0)*mul;
    const Scalar dzb = m_normQ(2)*m_normQ(1)*mul;
    const Scalar dzc = sn + std::pow(m_normQ(2),2)*mul;

    const Scalar dwa = -sn*NQFAKTOR*m_normQ(0);
    const Scalar dwb = -sn*NQFAKTOR*m_normQ(1);
    const Scalar dwc = -sn*NQFAKTOR*m_normQ(2);

    //write in the diffrot matrix, starting with dQ/dx
    m_diffTrans.at(0,0) = -4.0*(Q.y()*dya+Q.z()*dza);
    m_diffTrans.at(0,1) = -2.0*(Q.w()*dza+dwa*Q.z())+2.0*(Q.x()*dya+dxa*Q.y());
    m_diffTrans.at(0,2) = 2.0*(dwa*Q.y()+Q.w()*dya)+2.0*(dxa*Q.z()+Q.x()*dza);
    m_diffTrans.at(1,0) = 2.0*(Q.w()*dza+dwa*Q.z())+2.0*(Q.x()*dya+dxa*Q.y());
    m_diffTrans.at(1,1) = -4.0*(Q.x()*dxa+Q.z()*dza);
    m_diffTrans.at(1,2) = -2.0*(dwa*Q.x()+Q.w()*dxa)+2.0*(dya*Q.z()+Q.y()*dza);
    m_diffTrans.at(2,0) = -2.0*(dwa*Q.y()+Q.w()*dya)+2.0*(dxa*Q.z()+Q.x()*dza);
    m_diffTrans.at(2,1) = 2.0*(dwa*Q.x()+Q.w()*dxa)+2.0*(dya*Q.z()+Q.y()*dza);
    m_diffTrans.at(2,2) = -4.0*(Q.x()*dxa+Q.y()*dya);

    //dQ/dy
    m_diffTrans.at(0,3) = -4.0*(Q.y()*dyb+Q.z()*dzb);
    m_diffTrans.at(0,4) = -2.0*(Q.w()*dzb+dwb*Q.z())+2.0*(Q.x()*dyb+dxb*Q.y());
    m_diffTrans.at(0,5) = 2.0*(dwb*Q.y()+Q.w()*dyb)+2.0*(dxb*Q.z()+Q.x()*dzb);
    m_diffTrans.at(1,3) = 2.0*(Q.w()*dzb+dwb*Q.z())+2.0*(Q.x()*dyb+dxb*Q.y());
    m_diffTrans.at(1,4) = -4.0*(Q.x()*dxb+Q.z()*dzb);
    m_diffTrans.at(1,5) = -2.0*(dwb*Q.x()+Q.w()*dxb)+2.0*(dyb*Q.z()+Q.y()*dzb);
    m_diffTrans.at(2,3) = -2.0*(dwb*Q.y()+Q.w()*dyb)+2.0*(dxb*Q.z()+Q.x()*dzb);
    m_diffTrans.at(2,4) = 2.0*(dwb*Q.x()+Q.w()*dxb)+2.0*(dyb*Q.z()+Q.y()*dzb);
    m_diffTrans.at(2,5) = -4.0*(Q.x()*dxb+Q.y()*dyb);

    //dQ/dz
    m_diffTrans.at(0,6) = -4.0*(Q.y()*dyc+Q.z()*dzc);
    m_diffTrans.at(0,7) = -2.0*(Q.w()*dzc+dwc*Q.z())+2.0*(Q.x()*dyc+dxc*Q.y());
    m_diffTrans.at(0,8) = 2.0*(dwc*Q.y()+Q.w()*dyc)+2.0*(dxc*Q.z()+Q.x()*dzc);
    m_diffTrans.at(1,6) = 2.0*(Q.w()*dzc+dwc*Q.z())+2.0*(Q.x()*dyc+dxc*Q.y());
    m_diffTrans.at(1,7) = -4.0*(Q.x()*dxc+Q.z()*dzc);
    m_diffTrans.at(1,8) = -2.0*(dwc*Q.x()+Q.w()*dxc)+2.0*(dyc*Q.z()+Q.y()*dzc);
    m_diffTrans.at(2,6) = -2.0*(dwc*Q.y()+Q.w()*dyc)+2.0*(dxc*Q.z()+Q.x()*dzc);
    m_diffTrans.at(2,7) = 2.0*(dwc*Q.x()+Q.w()*dxc)+2.0*(dyc*Q.z()+Q.y()*dzc);
    m_diffTrans.at(2,8) = -4.0*(Q.x()*dxc+Q.y()*dyc);

    //recalculate all geometries
    typedef typename std::vector<Geom>::iterator iter;
    for(iter it = m_geometry.begin(); it != m_geometry.end(); it++)
        (*it)->recalculate(m_diffTrans);
};

template<typename Sys>
void ClusterMath<Sys>::addGeometry(Geom g) {
    m_geometry.push_back(g);
};

template<typename Sys>
void ClusterMath<Sys>::clearGeometry() {
    m_geometry.clear();
};

template<typename Sys>
std::vector<typename ClusterMath<Sys>::Geom>& ClusterMath<Sys>::getGeometry() {
    return m_geometry;
};

template<typename Sys>
ClusterMath<Sys>::map_downstream::map_downstream(details::ClusterMath<Sys>& cm, bool fix)
    : m_clusterMath(cm), m_isFixed(fix) {
    m_transform = m_clusterMath.getTransform().inverse();
};

template<typename Sys>
void ClusterMath<Sys>::map_downstream::operator()(Geom g) {
    //allow iteration over all maped geometries
    m_clusterMath.addGeometry(g);
    //set the offsets so that geometry knows where it is in the parameter map
    g->m_offset = m_clusterMath.getParameterOffset(general);
    g->m_offset_rot = m_clusterMath.getParameterOffset(rotation);
    //position and offset of the parameters must be set to the clusters values
    g->setClusterMode(true, m_isFixed);
    //calculate the appropriate local values
    g->transform(m_transform);
};

template<typename Sys>
void ClusterMath<Sys>::map_downstream::operator()(boost::shared_ptr<Cluster> c) {
    //we transform the GLOBAL geometries to local ones in the subcluster! therefore
    //we are not interested in the successive transformations, we only transform the
    //global geometries with the cluster transform we want them to be local in, and thats
    //the one supplied in the constructor
    //m_transform *= c->template getClusterProperty<math_prop>().getTransform().inverse();
};


template<typename Sys>
void ClusterMath<Sys>::mapClusterDownstreamGeometry(boost::shared_ptr<Cluster> cluster) {

#ifdef USE_LOGGING
    BOOST_LOG(log) << "Map downstream geometry";
#endif

    map_downstream down(cluster->template getProperty<math_prop>(),
                        cluster->template getProperty<fix_prop>());
    cluster->template for_each<Geometry3D>(down, true);
    //TODO: if one subcluster is fixed the hole cluster should be too, as there are no
    //	dof's remaining between parts and so nothing can be moved when one part is fixed.
};

template<typename Sys>
typename ClusterMath<Sys>::Scalar ClusterMath<Sys>::calculateClusterScale() {

#ifdef USE_LOGGING
    BOOST_LOG(log) << "Calculate cluster scale with transform scale: "<<m_transform.scaling().factor();
#endif
    //ensure the usage of the right transformation
    if(!fix)
        mapsToTransform(m_transform);
#ifdef USE_LOGGING
    BOOST_LOG(log) << "Calculate cluster scale sec transform scale: "<<m_transform.scaling().factor();
#endif

    //collect all points together
    m_points.clear();
    typename Kernel::Transform3D trans(m_transform.rotation(), m_transform.translation());
    trans.invert();
    typedef typename Vec::iterator iter;
    for(iter it = m_pseudo.begin(); it != m_pseudo.end(); it++) {
        m_points.push_back(trans*(*it));
    }
    typedef typename std::vector<Geom>::iterator g_iter;
    for(g_iter it = m_geometry.begin(); it != m_geometry.end(); it++)
        m_points.push_back((*it)->getPoint());

    //start scale calculation
    if(m_points.empty()) return 1.;
    else if(m_points.size() == 1) {
        const typename Kernel::Vector3 p = m_points[0];
        return calcOnePoint(p);
    } else if(m_points.size() == 2) {
        const typename Kernel::Vector3 p1 = m_points[0];
        const typename Kernel::Vector3 p2 = m_points[1];

        if(Kernel::isSame((p1-p2).norm(), 0., 1e-10))
            return calcOnePoint(p1);

        return calcTwoPoints(p1, p2);
    } else if(m_points.size() == 3) {

        const typename Kernel::Vector3 p1 = m_points[0];
        const typename Kernel::Vector3 p2 = m_points[1];
        const typename Kernel::Vector3 p3 = m_points[2];

        const typename Kernel::Vector3 d = p2-p1;
        const typename Kernel::Vector3 e = p3-p1;

        if(Kernel::isSame(d.norm(), 0., 1e-10)) {

            if(Kernel::isSame(e.norm(), 0., 1e-10))
                return calcOnePoint(p1);

            return calcTwoPoints(p1, p3);
        } else if(Kernel::isSame(e.norm(), 0., 1e-10)) {
            return calcTwoPoints(p1, p2);
        } else if(!Kernel::isSame((d/d.norm() - e/e.norm()).norm(), 0., 1e-10) &&
                  !Kernel::isSame((d/d.norm() + e/e.norm()).norm(), 0., 1e-10)) {
            return calcThreePoints(p1, p2, p3);
        }
        //three points on a line need to be treaded as multiple points
    }

    //more than 3 points don't have a exact solution. we search for a midpoint from which all points
    //are at least MAXFAKTOR*scale away, but not closer than MINFAKTOR*scale

    //get the bonding box to get the center of points
    Scalar xmin=1e10, xmax=1e-10, ymin=1e10, ymax=1e-10, zmin=1e10, zmax=1e-10;
    for(iter it = m_points.begin(); it != m_points.end(); it++) {
        typename Kernel::Vector3 v = (*it);
        xmin = (v(0)<xmin) ? v(0) : xmin;
        xmax = (v(0)<xmax) ? xmax : v(0);
        ymin = (v(1)<ymin) ? v(1) : ymin;
        ymax = (v(1)<ymax) ? ymax : v(1);
        zmin = (v(2)<zmin) ? v(2) : zmin;
        zmax = (v(2)<zmax) ? zmax : v(2);
    };
    //now calculate the midpoint
    midpoint << xmin+xmax, ymin+ymax, zmin+zmax;
    midpoint /= 2.;


    //get the scale direction an the resulting nearest point indexes
    double xh = xmax-xmin;
    double yh = ymax-ymin;
    double zh = zmax-zmin;
    int i1, i2, i3;
    if((xh<=yh) && (xh<=zh)) {
        i1=1;
        i2=2;
        i3=0;
    } else if((yh<xh) && (yh<zh)) {
        i1=0;
        i2=2;
        i3=1;
    } else {
        i1=0;
        i2=1;
        i3=2;
    }
    scale_dir.setZero();
    scale_dir(i3) = 1;
    max = Eigen::Vector3d(xmin,ymin,zmin);
    m_scale = (midpoint-max).norm()/MAXFAKTOR;
    mode = multiple_inrange;

    maxm = max-midpoint;
    Scalar minscale = 1e10;

    //get the closest point
    for(iter it = m_points.begin(); it != m_points.end(); it++) {

        const Eigen::Vector3d point = (*it)-midpoint;
        if(point.norm()<MINFAKTOR*m_scale) {

            const double h = std::abs(point(i3)-maxm(i3));
            const double k = std::pow(MINFAKTOR/MAXFAKTOR,2);
            double q = std::pow(point(i1),2) + std::pow(point(i2),2);
            q -= (std::pow(maxm(i1),2) + std::pow(maxm(i2),2) + std::pow(h,2))*k;
            q /= 1.-k;

            const double p = h*k/(1.-k);

            if(std::pow(p,2)<q) assert(false);

            midpoint(i3) += p + std::sqrt(std::pow(p,2)-q);
            maxm = max-midpoint;
            m_scale = maxm.norm()/MAXFAKTOR;

            mode = multiple_outrange;
            minm = (*it)-midpoint;

            it = m_points.begin();
        } else if(point.norm()<minscale) {
            minscale = point.norm();
        }
    }

    if(mode==multiple_inrange) {
        //we are in the range, let's get the perfect balanced scale value
        m_scale = (minscale+maxm.norm())/2.;
    }
    return m_scale;
};

template<typename Sys>
void ClusterMath<Sys>::applyClusterScale(Scalar scale, bool isFixed) {

#ifdef USE_LOGGING
    BOOST_LOG(log) << "Apply cluster scale: "<<scale;
    BOOST_LOG(log) << "initial transform scale: "<<m_transform.scaling().factor();
#endif
    //ensure the usage of the right transform
    if(!fix)
        mapsToTransform(m_transform);



    //when fixed, the geometries never get recalculated. therefore we have to do a calculate now
    //to alow the adoption of the scale. and no shift should been set.
    if(isFixed) {
        m_transform*=typename Kernel::Transform3D::Scaling(1./(scale*SKALEFAKTOR));
        m_ssrTransform*=typename Kernel::Transform3D::Scaling(1./(scale*SKALEFAKTOR));
        typename Kernel::DiffTransform3D diff(m_transform);
        //now calculate the scaled geometrys
        typedef typename std::vector<Geom>::iterator iter;
        for(iter it = m_geometry.begin(); it != m_geometry.end(); it++) {
            (*it)->recalculate(diff);
#ifdef USE_LOGGING
            BOOST_LOG(log) << "Fixed cluster geometry value:" << (*it)->m_rotated.transpose();
#endif
        };
        return;
    }

    //if this is our scale then just applie the midpoint as shift
    if(Kernel::isSame(scale, m_scale, 1e-10)) {

    }
    //if only one point exists we extend the origin-point-line to match the scale
    else if(mode==details::one) {
        if(Kernel::isSame(midpoint.norm(),0, 1e-10))
            midpoint << scale, 0, 0;
        else midpoint += scale*scale_dir;
    }
    //two and three points form a rectangular triangle, so same procedure
    else if(mode==details::two || mode==details::three) {

        midpoint+= scale_dir*std::sqrt(std::pow(scale,2) - std::pow(m_scale,2));
    }
    //multiple points
    else if(mode==details::multiple_outrange) {

        if(scale_dir(0)) {
            Scalar d = std::pow(maxm(1),2) + std::pow(maxm(2),2);
            Scalar h = std::sqrt(std::pow(MAXFAKTOR*scale,2)-d);
            midpoint(0) += maxm(0) + h;
        } else if(scale_dir(1)) {
            Scalar d = std::pow(maxm(0),2) + std::pow(maxm(2),2);
            Scalar h = std::sqrt(std::pow(MAXFAKTOR*scale,2)-d);
            midpoint(1) += maxm(1) + h;
        } else {
            Scalar d = std::pow(maxm(0),2) + std::pow(maxm(1),2);
            Scalar h = std::sqrt(std::pow(MAXFAKTOR*scale,2)-d);
            midpoint(2) += maxm(2) + h;
        }
    } else {

        //TODO: it's possible that for this case we get too far away from the outer points.
        //	    The m_scale for "midpoint outside the bounding box" may be bigger than the
        //      scale to applie, so it results in an error.
        //get the closest point
        typedef typename Vec::iterator iter;
        for(iter it = m_points.begin(); it != m_points.end(); it++) {

            const Eigen::Vector3d point = (*it)-midpoint;
            if(point.norm()<MINFAKTOR*scale) {

                if(scale_dir(0)) {
                    Scalar d = std::pow(point(1),2) + std::pow(point(2),2);
                    Scalar h = std::sqrt(std::pow(MINFAKTOR*scale,2)-d);
                    midpoint(0) += point(0) + h;
                } else if(scale_dir(1)) {
                    Scalar d = std::pow(point(0),2) + std::pow(point(2),2);
                    Scalar h = std::sqrt(std::pow(MINFAKTOR*scale,2)-d);
                    midpoint(1) += point(1) + h;
                } else {
                    Scalar d = std::pow(point(0),2) + std::pow(point(1),2);
                    Scalar h = std::sqrt(std::pow(MINFAKTOR*scale,2)-d);
                    midpoint(2) += point(2) + h;
                }
            }
        }
    }

    typename Kernel::Transform3D ssTrans;
    ssTrans = typename Kernel::Transform3D::Translation(-midpoint);
    ssTrans *= typename Kernel::Transform3D::Scaling(1./(scale*SKALEFAKTOR));

    //recalculate all geometries
    typedef typename std::vector<Geom>::iterator iter;
    for(iter it = m_geometry.begin(); it != m_geometry.end(); it++)
        (*it)->transform(ssTrans);

    //set the new rotation and translation
    m_transform = ssTrans.inverse()*m_transform;
    m_ssrTransform *= ssTrans;

    transformToMaps(m_transform);

#ifdef USE_LOGGING
    BOOST_LOG(log) << "sstrans scale: "<<ssTrans.scaling().factor();
    BOOST_LOG(log) << "finish transform scale: "<<m_transform.scaling().factor();
    //we may want to access the scale points for debugging (I mean you, freecad assembly debug!), so 
    //it is important to transform them too to ensure the points are in the same coordinate system
    typename Vec::iterator it;
    for(it=m_points.begin(); it!=m_points.end(); it++) {
      (*it) = ssTrans * (*it);
    };
#endif
};

template<typename Sys>
typename ClusterMath<Sys>::Scalar ClusterMath<Sys>::calcOnePoint(const typename ClusterMath<Sys>::Kernel::Vector3& p) {

    //one point can have every scale when moving the midpoint on the origin - point vector
    midpoint  = p;
    scale_dir = -midpoint;
    scale_dir.normalize();
    mode = details::one;
    m_scale = 0.;
    return 0.;
};

template<typename Sys>
typename ClusterMath<Sys>::Scalar ClusterMath<Sys>::calcTwoPoints(const typename ClusterMath<Sys>::Kernel::Vector3& p1,
        const typename ClusterMath<Sys>::Kernel::Vector3& p2) {

    //two points have their minimal scale at the mid position. Scaling perpendicular to this
    //line allows arbitrary scale values. Best is to have the scale dir move towards the origin
    //as good as possible.
    midpoint  = p1+(p2-p1)/2.;
    scale_dir = (p2-p1).cross(midpoint);
    scale_dir = scale_dir.cross(p2-p1);
    if(!Kernel::isSame(scale_dir.norm(),0, 1e-10)) scale_dir.normalize();
    else scale_dir(0) = 1;
    mode = details::two;
    m_scale = (p2-p1).norm()/2.;
    return m_scale;
};

template<typename Sys>
typename ClusterMath<Sys>::Scalar ClusterMath<Sys>::calcThreePoints(const typename ClusterMath<Sys>::Kernel::Vector3& p1,
        const typename ClusterMath<Sys>::Kernel::Vector3& p2, const typename ClusterMath<Sys>::Kernel::Vector3& p3) {

    //Three points form a triangle with it's minimal scale at the center of it's outer circle.
    //Arbitrary scale values can be achieved by moving perpendicular to the triangle plane.
    typename Kernel::Vector3 d = p2-p1;
    typename Kernel::Vector3 e = p3-p1;

    typename Kernel::Vector3 f = p1+0.5*d;
    typename Kernel::Vector3 g = p1+0.5*e;
    scale_dir = d.cross(e);

    typename Kernel::Matrix3 m;
    m.row(0) = d.transpose();
    m.row(1) = e.transpose();
    m.row(2) = scale_dir.transpose();

    typename Kernel::Vector3 res(d.transpose()*f, e.transpose()*g, scale_dir.transpose()*p1);

    midpoint =  m.colPivHouseholderQr().solve(res);
    scale_dir.normalize();
    mode = details::three;
    m_scale = (midpoint-p1).norm();

    return m_scale;

};

}//details
}//dcm


#endif //GCM_CLUSTERMATH_H






