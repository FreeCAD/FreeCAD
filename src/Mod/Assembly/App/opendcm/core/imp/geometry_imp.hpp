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


#ifndef GCM_GEOMETRY_IMP_H
#define GCM_GEOMETRY_IMP_H

#include <iostream>

#include "../geometry.hpp"

#include <Eigen/Core>

#include <boost/type_traits.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/plus.hpp>
#include <boost/mpl/find.hpp>

#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/concept_check.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/function.hpp>

#include <boost/variant.hpp>

#ifdef USE_LOGGING
#include <boost/math/special_functions.hpp>
#endif

namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

namespace dcm {

namespace details {

template< typename Kernel, int Dim, typename TagList>
Geometry<Kernel, Dim, TagList>::Geometry()
    : m_isInCluster(false), m_parameter(NULL,0,DS(0,0)),
      m_clusterFixed(false), m_init(false) {

#ifdef USE_LOGGING
    log.add_attribute("Tag", attrs::constant< std::string >("Geometry"));
#endif

};

template< typename Kernel, int Dim, typename TagList>
void Geometry<Kernel, Dim, TagList>::transform(const Transform& t) {

    if(m_isInCluster)
        transform(t, m_toplocal);
    else
        if(m_init)
            transform(t, m_rotated);
        else
            transform(t, m_global);
};

template< typename Kernel, int Dim, typename TagList>
template<typename tag>
void Geometry<Kernel, Dim, TagList>::init() {

    m_BaseParameterCount = tag::parameters::value;
    m_parameterCount = m_BaseParameterCount;
    m_rotations = tag::rotations::value;
    m_translations = tag::translations::value;

    m_toplocal.setZero(m_parameterCount);
    m_global.resize(m_parameterCount);
    m_rotated.resize(m_parameterCount);
    m_rotated.setZero();

    m_diffparam.resize(m_parameterCount,6);
    m_diffparam.setZero();

    m_general_type = tag::weight::value;
    m_exact_type = mpl::find<TagList, tag>::type::pos::value;

    normalize();

    //new value which is not set into parameter, so init is false
    m_init = false;

#ifdef USE_LOGGING
    BOOST_LOG_SEV(log, information) << "Init: "<<m_global.transpose();
#endif

};

template< typename Kernel, int Dim, typename TagList>
void Geometry<Kernel, Dim, TagList>::normalize() {
    //directions are not nessessarily normalized, but we need to ensure this in cluster mode
    for(int i=m_translations; i!=m_rotations; i++)
        m_global.template segment<Dim>(i*Dim).normalize();
};

template< typename Kernel, int Dim, typename TagList>
typename Kernel::VectorMap& Geometry<Kernel, Dim, TagList>::getParameterMap() {
    m_isInCluster = false;
    m_parameterCount = m_BaseParameterCount;
    return m_parameter;
};

template< typename Kernel, int Dim, typename TagList>
template<typename T>
void Geometry<Kernel, Dim, TagList>::linkTo(boost::shared_ptr<Geometry<Kernel, Dim, TagList> > geom, int offset) {

    init<T>();
    m_link = geom;
    m_link_offset = offset;
    m_global = geom->m_global.segment(offset, m_BaseParameterCount);
};

template< typename Kernel, int Dim, typename TagList>
void Geometry<Kernel, Dim, TagList>::initMap(typename Kernel::MappedEquationSystem* mes) {

    //check should not be needed, but how knows...
    if(!m_init) {

        if(!isLinked()) {
            m_offset = mes->setParameterMap(m_parameterCount, getParameterMap());
            m_parameter = m_global;
            m_init = true;
        }
        else {
            //it's important that the linked geometry is initialised, as we going to access its parameter map
            if(!m_link->isInitialised())
                m_link->initMap(mes);

            m_offset = m_link->m_offset + m_link_offset;
            new(&getParameterMap()) typename Kernel::VectorMap(&m_link->getParameterMap()(m_link_offset), m_parameterCount, typename Kernel::DynStride(1,1));
        }
    }
};

template< typename Kernel, int Dim, typename TagList>
void Geometry<Kernel, Dim, TagList>::setClusterMode(bool iscluster, bool isFixed) {

    m_isInCluster = iscluster;
    m_clusterFixed = isFixed;
    if(iscluster) {
        //we are in cluster, therefore the parameter map should not point to a solver value but to
        //the rotated original value;
        new(&m_parameter) typename Kernel::VectorMap(&m_rotated(0), m_parameterCount, DS(1,1));
        //the local value is the global one as no transformation was applied  yet
        m_toplocal = m_global;
        m_rotated = m_global;
    }
    else
        new(&m_parameter) typename Kernel::VectorMap(&m_global(0), m_parameterCount, DS(1,1));
};

template< typename Kernel, int Dim, typename TagList>
void Geometry<Kernel, Dim, TagList>::recalculate(DiffTransform& trans) {
    if(!m_isInCluster)
        return;

    for(int i=0; i!=m_rotations; i++) {
        //first rotate the original to the transformed value
        m_rotated.block(i*Dim,0,Dim,1) = trans.rotation()*m_toplocal.template segment<Dim>(i*Dim);

        //now calculate the gradient vectors and add them to diffparam
        for(int j=0; j<Dim; j++)
            m_diffparam.block(i*Dim,j,Dim,1) = trans.differential().block(0,j*3,Dim,Dim) * m_toplocal.template segment<Dim>(i*Dim);
    }
    //after rotating the needed parameters we translate the stuff that needs to be moved
    for(int i=0; i!=m_translations; i++) {
        m_rotated.block(i*Dim,0,Dim,1) += trans.translation().vector();
        m_rotated.block(i*Dim,0,Dim,1) *= trans.scaling().factor();
        //calculate the gradient vectors and add them to diffparam
        m_diffparam.block(i*Dim,Dim,Dim,Dim).setIdentity();
    }

#ifdef USE_LOGGING
    if(!boost::math::isnormal(m_rotated.norm()) || !boost::math::isnormal(m_diffparam.norm())) {
        BOOST_LOG_SEV(log, error) << "Unnormal recalculated value detected: "<<m_rotated.transpose()<<std::endl
                       << "or unnormal recalculated diff detected: "<<std::endl<<m_diffparam<<std::endl
                       <<" with Transform: "<<std::endl<<trans;
    }
#endif
};


template< typename Kernel, int Dim, typename TagList>
void Geometry<Kernel, Dim, TagList>::finishCalculation() {
    //if fixed nothing needs to be changed
    if(m_isInCluster) {
        //recalculate(1.); //remove scaling to get right global value
        m_global = m_rotated;
#ifdef USE_LOGGING
        BOOST_LOG_SEV(log, information) << "Finish cluster calculation";
#endif
    }
    //TODO:non cluster parameter scaling
    else {
        m_global = m_parameter;
        normalize();
#ifdef USE_LOGGING
        BOOST_LOG_SEV(log, information) << "Finish calculation";
#endif
    };

    m_init = false;
    m_isInCluster = false;

    recalculated();
};

template< typename Kernel, int Dim, typename TagList>
template<typename VectorType>
void Geometry<Kernel, Dim, TagList>::transform(const Transform& t, VectorType& vec) {

    //everything that needs to be translated needs to be fully transformed
    for(int i=0; i!=m_translations; i++) {
        typename Kernel::Vector3 v = vec.template segment<Dim>(i*Dim);
        vec.template segment<Dim>(i*Dim) = t*v;
    }

    for(int i=m_translations; i!=m_rotations; i++) {
        typename Kernel::Vector3 v = vec.template segment<Dim>(i*Dim);
        vec.template segment<Dim>(i*Dim) = t.rotate(v);
    }

#ifdef USE_LOGGING
    BOOST_LOG_SEV(log, manipulation) << "Transformed with cluster: "<<m_isInCluster
                   << ", init: "<<m_init<<" into: "<< vec.transpose();
#endif
}

template< typename Kernel, int Dim, typename TagList>
void Geometry<Kernel, Dim, TagList>::scale(Scalar value) {

    for(int i=0; i!=m_translations; i++)
        m_parameter.template segment<Dim>(i*Dim) *= 1./value;

};

}
}

#endif // GCM_GEOMETRY_H
