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

#ifndef DCM_SYSTEM_IMP_H
#define DCM_SYSTEM_IMP_H

#ifdef DCM_EXTERNAL_CORE
#include "kernel_imp.hpp"
#include "transformation_imp.hpp"
#include "clustergraph_imp.hpp"
#include "equations_imp.hpp"
#endif

#include "../system.hpp"

#include <boost/fusion/include/for_each.hpp>

namespace dcm {

struct clearer {
    template<typename T>
    void operator()(T& vector) const {
        vector.clear();
    };
};

template<typename System>
struct cloner {

    System& newSys;
    cloner(System& ns) : newSys(ns) {};

    template<typename T>
    struct test : mpl::and_<details::is_shared_ptr<T>,
            mpl::not_<boost::is_same<T, boost::shared_ptr<typename System::Cluster> > > > {};

    template<typename T>
    typename boost::enable_if< test<T>, void>::type operator()(T& p) const {
        p = p->clone(newSys);
        newSys.push_back(p);
    };
    template<typename T>
    typename boost::enable_if< mpl::not_<test<T> >, void>::type operator()(const T& p) const {};
};

template< typename KernelType, typename T1, typename T2, typename T3 >
System<KernelType, T1, T2, T3>::System() : m_cluster(new Cluster),
    m_storage(new Storage), m_options(new OptionOwner)
#ifdef USE_LOGGING
    , sink(init_log())
#endif
{
    Type1::system_init(*this);
    Type2::system_init(*this);
    Type3::system_init(*this);
};


template< typename KernelType, typename T1, typename T2, typename T3 >
System<KernelType, T1, T2, T3>::~System() {
#ifdef USE_LOGGING
    stop_log(sink);
#endif
};


template< typename KernelType, typename T1, typename T2, typename T3 >
void System<KernelType, T1, T2, T3>::clear() {

    m_cluster->clearClusters();
    m_cluster->clear();
    fusion::for_each(*m_storage, clearer());
};

template< typename KernelType, typename T1, typename T2, typename T3 >
SolverInfo System<KernelType, T1, T2, T3>::solve() {
    clock_t start = clock();
    m_sheduler.execute(*this);
    clock_t end = clock();
    
    SolverInfo info = m_kernel.getSolverInfo();
    info.time = (double(end-start)* 1000.) / double(CLOCKS_PER_SEC);

    //signal our successful solving
    BaseType::template emitSignal<solved>(this);
    
    return info;
};

template< typename KernelType, typename T1, typename T2, typename T3 >
boost::shared_ptr<System<KernelType, T1, T2, T3> > System<KernelType, T1, T2, T3>::createSubsystem() {

    boost::shared_ptr<System> s = boost::shared_ptr<System>(new System());
    s->m_cluster = m_cluster->createCluster().first;
    s->m_storage = m_storage;
    s->m_cluster->template setProperty<dcm::type_prop>(details::subcluster);
    s->m_options = m_options;
    
#ifdef USE_LOGGING
    stop_log(s->sink);
#endif
    
    //inform modules that we have a subsystem now
    Inheriter1::system_sub(s);
    Inheriter2::system_sub(s);
    Inheriter3::system_sub(s);
    
    m_subsystems.push_back(s);
      
    return s;
};

template< typename KernelType, typename T1, typename T2, typename T3 >
typename std::vector<boost::shared_ptr<System<KernelType, T1, T2, T3> > >::iterator System<KernelType, T1, T2, T3>::beginSubsystems() {
  return m_subsystems.begin();
};

template< typename KernelType, typename T1, typename T2, typename T3 >
typename std::vector<boost::shared_ptr<System<KernelType, T1, T2, T3> > >::iterator System<KernelType, T1, T2, T3>::endSubsystems() {
  return m_subsystems.end();
};

template< typename KernelType, typename T1, typename T2, typename T3 >
KernelType& System<KernelType, T1, T2, T3>::kernel() {
    return m_kernel;
};

template< typename KernelType, typename T1, typename T2, typename T3 >
void System<KernelType, T1, T2, T3>::copyInto(System<KernelType, T1, T2, T3>& into) const {

    //copy the clustergraph and clone all objects while at it. They are also pushed to the storage
    cloner<System> cl(into);
    m_cluster->copyInto(into.m_cluster, cl);

    //notify all modules that they are copied
    Type1::system_copy(*this, into);
    Type2::system_copy(*this, into);
    Type3::system_copy(*this, into);
};

template< typename KernelType, typename T1, typename T2, typename T3 >
System<KernelType, T1, T2, T3>* System<KernelType, T1, T2, T3>::clone() const {

    System* ns = new System();
    this->copyInto(*ns);
    return ns;
};

}
#endif //GCM_SYSTEM_H











