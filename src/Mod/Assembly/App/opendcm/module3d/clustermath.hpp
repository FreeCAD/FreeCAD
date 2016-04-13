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

#ifndef DCM_CLUSTERMATH_H
#define DCM_CLUSTERMATH_H

#include <vector>
#include <Eigen/StdVector>
#include <boost/noncopyable.hpp>
#include <opendcm/core/logging.hpp>
#include <opendcm/core/kernel.hpp>
#include "defines.hpp"

#define MAXFAKTOR 1.2   //the maximal distance allowd by a point normed to the cluster size
#define MINFAKTOR 0.8   //the minimal distance allowd by a point normed to the cluster size
#define SKALEFAKTOR 1.  //the faktor by which the biggest size is multiplied to get the scale value
#define NQFAKTOR 0.5    //the faktor by which the norm quaternion is multiplied with to get the RealScalar
//norm quaternion to generate the unit quaternion

namespace dcm {
namespace details {

enum Scalemode {
    one,
    two,
    three,
    multiple_inrange,
    multiple_outrange
};

template<typename Sys>
struct ClusterMath : public boost::noncopyable {

public:
    typedef typename Sys::Kernel Kernel;
    typedef typename Sys::Cluster Cluster;
    typedef typename system_traits<Sys>::template getModule<m3d>::type module3d;
    typedef typename module3d::Geometry3D Geometry3D;
    typedef boost::shared_ptr<Geometry3D> Geom;
    typedef typename module3d::math_prop math_prop;
    typedef typename module3d::fix_prop fix_prop;

    typedef typename Kernel::number_type Scalar;

    typename Kernel::Transform3D m_transform, m_ssrTransform, m_resetTransform;
    typename Kernel::DiffTransform3D m_diffTrans;
    typename Kernel::Vector3Map	 m_normQ;
    typename Kernel::Quaternion  m_resetQuaternion;

    int m_offset, m_offset_rot;
    bool init, fix;
    std::vector<Geom> m_geometry;

    typename Kernel::Vector3Map m_translation;
    //shift scale stuff
    typename Kernel::Vector3 midpoint, m_shift, scale_dir, maxm, minm, max, fixtrans;
    Scalemode mode;
    Scalar m_scale;

    typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;
    Vec m_points, m_pseudo;

#ifdef USE_LOGGING
    src::logger log;
#endif

public:
    ClusterMath();

    void setParameterOffset(int offset, AccessType t);
    int getParameterOffset(AccessType t);

    typename Kernel::Vector3Map& getNormQuaternionMap();
    typename Kernel::Vector3Map& getTranslationMap();
    void initMaps();
    void initFixMaps();

    typename Kernel::Transform3D& getTransform();
    typename Kernel::Transform3D::Translation const& getTranslation() const;
    typename Kernel::Transform3D::Rotation const& getRotation() const;
    void setTransform(typename Kernel::Transform3D const& t);
    void setTranslation(typename Kernel::Transform3D::Translation const& );
    void setRotation(typename Kernel::Transform3D::Rotation const&);
    
    void mapsToTransform(typename Kernel::Transform3D& trans);
    void transformToMaps(typename Kernel::Transform3D& trans);

    void finishCalculation();
    void finishFixCalculation();

    void resetClusterRotation(typename Kernel::Transform3D& trans);

    void calcDiffTransform(typename Kernel::DiffTransform3D& trans);
    void recalculate();

    void addGeometry(Geom g);
    void clearGeometry();
    std::vector<Geom>& getGeometry();

    struct map_downstream {

        details::ClusterMath<Sys>& 	m_clusterMath;
        typename Kernel::Transform3D	m_transform;
        bool m_isFixed;

        map_downstream(details::ClusterMath<Sys>& cm, bool fix);

        void operator()(Geom g);
        void operator()(boost::shared_ptr<Cluster> c);
    };

    void mapClusterDownstreamGeometry(boost::shared_ptr<Cluster> cluster);

    //Calculate the scale of the cluster. Therefore the midpoint is calculated and the scale is
    // defined as the max distance between the midpoint and the points.
    Scalar calculateClusterScale();

    void applyClusterScale(Scalar scale, bool isFixed);

private:
    Scalar calcOnePoint(const typename Kernel::Vector3& p);

    Scalar calcTwoPoints(const typename Kernel::Vector3& p1, const typename Kernel::Vector3& p2);

    Scalar calcThreePoints(const typename Kernel::Vector3& p1,
                           const typename Kernel::Vector3& p2, const typename Kernel::Vector3& p3);

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

}//details
}//dcm

#ifndef DCM_EXTERNAL_3D
#include "imp/clustermath_imp.hpp"
#endif

#endif //GCM_CLUSTERMATH_H






