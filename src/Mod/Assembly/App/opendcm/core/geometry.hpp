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


#ifndef GCM_GEOMETRY_H
#define GCM_GEOMETRY_H

#include <iostream>

#include <Eigen/Core>

#include <boost/type_traits.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/plus.hpp>

#include <boost/variant.hpp>

#include "traits.hpp"
#include "logging.hpp"
#include "transformation.hpp"

namespace mpl = boost::mpl;

namespace dcm {

//signal we use for recalculation
struct recalculated {};

//all supported geometry types for easy access and comparison
namespace geometry {
enum types {
    parameter = 0,
    direction,
    point,
    line,
    segment,
    circle,
    arc,
    geometry,
    ellipse,
    elliptical_arc,
    plane,
    cylinder
};
}//namespace geometry

namespace tag {

struct undefined {
    typedef mpl::int_<0> parameters;
    typedef mpl::int_<0> transformations;
};

//we need to order tags, this values make it easy for module tags
namespace weight {
struct parameter 	: mpl::int_<geometry::parameter> {};
struct direction 	: mpl::int_<geometry::direction> {};
struct point 		: mpl::int_<geometry::point> {};
struct line  		: mpl::int_<geometry::line> {};
struct segment  	: mpl::int_<geometry::segment> {};
struct circle 		: mpl::int_<geometry::circle> {};
struct arc  		: mpl::int_<geometry::arc> {};
struct ellipse  	: mpl::int_<geometry::ellipse> {};
struct elliptical_arc  	: mpl::int_<geometry::elliptical_arc> {};
struct plane 		: mpl::int_<geometry::plane> {};
struct cylinder 	: mpl::int_<geometry::cylinder> {};
}
} // tag

namespace details {

struct bg {}; //struct to allow test for basic geometry

template< typename weight_type, int params, bool rotatable, bool translatable>
struct basic_geometry : public bg {

    typedef mpl::int_<params> parameters;
    typedef typename mpl::if_c<translatable, mpl::int_<1>, mpl::int_<0> >::type translations;
    typedef typename mpl::if_c<rotatable, mpl::int_<1>, mpl::int_<0> >::type rotations;
    typedef weight_type weight;
    typedef mpl::vector0<> sub_stack;
};

//build up stacked geometry. these are geometrys which can be splitted into multiple basic geometries. For
//example lines can be splittet into a point and a direction. Make sure you order the basic geometry in a
//sensible rotation/translation manner. Remember: geometrie is first rotated, than translated. Therefore
//everything that gets rotated and translated needs to be first, than the rotation only stuff, then the
//untransformed. For a line this would be <point, direction>
template<typename weight_type, typename T1, typename T2>
struct stacked2_geometry {

    //be sure we only stack base geometrys
    BOOST_MPL_ASSERT((boost::is_base_of< bg, T1 >));
    BOOST_MPL_ASSERT((boost::is_base_of< bg, T2 >));

    typedef typename mpl::plus<typename T1::parameters, typename T2::parameters>::type parameters;
    typedef typename mpl::plus<typename T1::rotations, typename T2::rotations>::type rotations;
    typedef typename mpl::plus<typename T1::translations, typename T2::translations>::type translations;
    typedef weight_type weight;
    typedef mpl::vector2<T1, T2> sub_stack;
};

template<typename weight_type, typename T1, typename T2, typename T3>
struct stacked3_geometry {

    //be sure we only stack base geometrys
    BOOST_MPL_ASSERT((boost::is_base_of< bg, T1 >));
    BOOST_MPL_ASSERT((boost::is_base_of< bg, T2 >));
    BOOST_MPL_ASSERT((boost::is_base_of< bg, T3 >));

    typedef typename mpl::plus<typename T1::parameters, typename T2::parameters, typename T3::parameters>::type parameters;
    typedef typename mpl::plus<typename T1::rotations, typename T2::rotations, typename T3::rotations>::type rotations;
    typedef typename mpl::plus<typename T1::translations, typename T2::translations, typename T3::translations>::type translations;
    typedef weight_type weight;
    typedef mpl::vector3<T1, T2, T3> sub_stack;
};
} //details

namespace tag {
//a parameter is universal, so let's define it here
struct parameter : details::basic_geometry<weight::parameter, 1, false, false> {};
} //tag

struct orderd_bracket_accessor {

    template<typename Scalar, int ID, typename T>
    Scalar get(T& t) {
        return t[ID];
    };
    template<typename Scalar, int ID, typename T>
    void set(Scalar value, T& t) {
        t[ID] = value;
    };
};

struct orderd_roundbracket_accessor {

    template<typename Scalar, int ID, typename T>
    Scalar get(T& t) {
        return t(ID);
    };
    template<typename Scalar, int ID,  typename T>
    void set(Scalar value, T& t) {
        t(ID) = value;
    };
};

//tag ordering (smaller weight is first tag)
template<typename T1, typename T2>
struct tag_order {

    BOOST_MPL_ASSERT((mpl::not_< mpl::or_<
                      boost::is_same< typename T1::weight, mpl::int_<0> >,
                      boost::is_same< typename T2::weight, mpl::int_<0> >  >  >));

    typedef typename mpl::less<typename T2::weight, typename T1::weight>::type swapt;
    typedef typename mpl::if_<swapt, T2, T1>::type first_tag;
    typedef typename mpl::if_<swapt, T1, T2>::type second_tag;
};


//template<typename T1, typename T2>
//struct type_order : public tag_order< typename geometry_traits<T1>::tag, typename geometry_traits<T2>::tag > {};


template< typename T>
struct geometry_traits {
    BOOST_MPL_ASSERT_MSG(false, NO_GEOMETRY_TRAITS_SPECIFIED_FOR_TYPE, (T));
};

template< typename T>
struct geometry_clone_traits {
    T operator()(T& val) {
        return T(val);
    };
};

namespace details {

// the parameter a geometr needs in a mapped equation system need to be managed separate, as
// we may want to access the same parameter space from different geometries (if they are linked)
// this is done by the parameter space class
template<typename Kernel>
struct parameter_space {

    void init(typename Kernel::MappedEquationSystem* mes);

};

template<typename Kernel, int Dim, typename TagList = mpl::vector0<> >
class Geometry {

    typedef typename Kernel::number_type 		Scalar;
    typedef typename Kernel::DynStride 			DS;
    typedef typename Kernel::template transform_type<Dim>::type		Transform;
    typedef typename Kernel::template transform_type<Dim>::diff_type	DiffTransform;

#ifdef USE_LOGGING
protected:
    dcm_logger log;
#endif

public:
    typedef mpl::int_<Dim> Dimension;

    Geometry();

    //basic ations
    template<typename tag, typename Derived>
    void  setValue(const Eigen::MatrixBase<Derived>& t) {
        init<tag>();
        m_global = t;
    };
    typename Kernel::Vector& getValue() {
        return m_global;
    };
    void transform(const Transform& t);

    geometry::types getGeometryType() {
        return geometry::types(m_general_type);
    };

    int getExactType() {
        return m_exact_type;
    };

    //allow accessing the internal values in unittests without making them public,
    //so that access control of the internal classes is not changed and can be tested
#ifdef TESTING
    typename Kernel::Vector& toplocal() {
        return m_toplocal;
    };
    typename Kernel::Vector& rotated()  {
        return m_rotated;
    };
    int& offset()  {
        return m_offset;
    };
    void clusterMode(bool iscluster, bool isFixed) {
        setClusterMode(iscluster, isFixed);
    };
    void trans(const Transform& t) {
        transform(t);
    };
    void recalc(DiffTransform& trans) {
        recalculate(trans);
    };
    typename Kernel::Vector3 point() {
        return getPoint();
    };
    int parameterCount() {
        return  m_parameterCount;
    };
    template<typename T>
    void test_linkTo(boost::shared_ptr< Geometry< Kernel, Dim > > geom, int offset) {
        linkTo<T>(geom, offset);
    };
    bool test_isLinked() {
        return isLinked();
    };
#endif

//protected would be the right way, however, visual studio 10 does not find a way to access them even when constraint::holder structs
//are declared friend
//protected:
    int     m_general_type, m_exact_type; //hold the type numbers for easy identification
    int     m_BaseParameterCount; //count of the parameters the variant geometry type needs
    int     m_parameterCount; //count of the used parameters (when in cluster:6, else m_BaseParameterCount)
    int     m_offset, m_offset_rot; //the starting point of our parameters in the math system parameter vector
    int     m_rotations; //count of rotations to be done when original vector gets rotated
    int     m_translations; //count of translations to be done when original vector gets rotated
    bool    m_isInCluster, m_clusterFixed, m_init;
    typename Kernel::Vector      m_toplocal; //the local value in the toplevel cluster used for cuttent solving
    typename Kernel::Vector      m_global; //the global value outside of all clusters
    typename Kernel::Vector      m_rotated; //the global value as the rotation of toplocal (used as temp)
    typename Kernel::Matrix      m_diffparam; //gradient vectors combined as matrix when in cluster
    typename Kernel::VectorMap   m_parameter; //map to the parameters in the solver

    template<typename tag>
    void init();

    void normalize();

    typename Kernel::VectorMap& getParameterMap();
    void initMap(typename Kernel::MappedEquationSystem* mes);
    bool isInitialised() {
        return m_init;
    };

    void setClusterMode(bool iscluster, bool isFixed);
    bool getClusterMode() {
        return m_isInCluster;
    };
    bool isClusterFixed() {
        return m_clusterFixed;
    };

    int m_link_offset;
    boost::shared_ptr<Geometry<Kernel, Dim, TagList> > m_link;

    template<typename T>
    void linkTo(boost::shared_ptr< Geometry< Kernel, Dim, TagList > > geom, int offset);
    bool isLinked() {
        return m_link!=0;
    };

    void recalculate(DiffTransform& trans);

    typename Kernel::Vector3 getPoint() {
            return m_toplocal.template segment<Dim>(0);
    };

    //use m_value or parametermap as new value, dependend on the solving mode
    void finishCalculation();

    template<typename VectorType>
    void transform(const Transform& t, VectorType& vec);
    void scale(Scalar value);

    //let the derived class decide what happens on significant events
    virtual void reset() = 0;
    virtual void recalculated() = 0;
    virtual void removed() = 0;
};

} //details
} //dcm

#ifndef DCM_EXTERNAL_CORE
#include "imp/geometry_imp.hpp"
#endif
#endif // GCM_GEOMETRY_H
