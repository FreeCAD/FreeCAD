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

#ifndef GCM_SYSTEM_H
#define GCM_SYSTEM_H

#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector/vector0.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/count.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/less_equal.hpp>
#include <boost/mpl/print.hpp>
#include <boost/mpl/and.hpp>

#include <boost/graph/adjacency_list.hpp>

#include <iostream>

#include "property.hpp"
#include "clustergraph.hpp"
#include "sheduler.hpp"
#include "logging.hpp"
#include "traits.hpp"

namespace mpl = boost::mpl;

namespace dcm {

struct No_Identifier {};
struct Unspecified_Identifier {};


namespace details {
  
enum { subcluster = 10};

template<typename seq, typename state>
struct vector_fold : mpl::fold< seq, state,
        mpl::push_back<mpl::_1,mpl::_2> > {};

template<typename seq, typename state>
struct edge_fold : mpl::fold< seq, state,
        mpl::if_< is_edge_property<mpl::_2>,
        mpl::push_back<mpl::_1,mpl::_2>, mpl::_1 > > {};

template<typename seq, typename state>
struct vertex_fold : mpl::fold< seq, state,
        mpl::if_< is_vertex_property<mpl::_2>,
        mpl::push_back<mpl::_1,mpl::_2>, mpl::_1 > > {};

template<typename seq, typename state>
struct cluster_fold : mpl::fold< seq, state,
        mpl::if_< is_cluster_property<mpl::_2>,
        mpl::push_back<mpl::_1,mpl::_2>, mpl::_1 > > {};

template<typename seq, typename state, typename obj>
struct obj_fold : mpl::fold< seq, state,
        mpl::if_< mpl::or_<
        boost::is_same< details::property_kind<mpl::_2>, obj>, is_object_property<mpl::_2> >,
        mpl::push_back<mpl::_1,mpl::_2>, mpl::_1 > > {};

template<typename objects, typename properties>
struct property_map {
    typedef typename mpl::fold<
    objects, mpl::map<>, mpl::insert< mpl::_1, mpl::pair<
    mpl::_2, details::obj_fold<properties, mpl::vector<>, mpl::_2 > > > >::type type;
};
template<typename T>
struct get_identifier {
    typedef typename T::Identifier type;
};

template<typename seq, typename state>
struct map_fold : mpl::fold< seq, state, mpl::insert<mpl::_1,mpl::_2> > {};

struct nothing {};

template<int v>
struct EmptyModule {

    template<typename T>
    struct type {
        struct inheriter {};
        typedef mpl::vector<>	properties;
        typedef mpl::vector<>   objects;
        typedef Unspecified_Identifier Identifier;

        static void system_init(T& sys) {};
        static void system_copy(const T& from, T& into) {};
    };
};

template <class T>
struct is_shared_ptr : boost::mpl::false_ {};
template <class T>
struct is_shared_ptr<boost::shared_ptr<T> > : boost::mpl::true_ {};

}

template< typename KernelType,
          typename  T1 = details::EmptyModule<1>,
          typename  T2 = details::EmptyModule<2>,
          typename  T3 = details::EmptyModule<3> >
class System : 	public T1::template type< System<KernelType,T1,T2,T3> >::inheriter,
    public T2::template type< System<KernelType,T1,T2,T3> >::inheriter,
    public T3::template type< System<KernelType,T1,T2,T3> >::inheriter {

    typedef System<KernelType,T1,T2,T3> BaseType;
public:
    typedef typename T1::template type< BaseType > Type1;
    typedef typename T2::template type< BaseType > Type2;
    typedef typename T3::template type< BaseType > Type3;
    typedef mpl::vector3<Type1, Type2, Type3> TypeVector;

    //Check if all Identifiers are the same and find out which type it is
    typedef typename mpl::fold<TypeVector, mpl::vector<>,
            mpl::if_<boost::is_same<details::get_identifier<mpl::_2>, Unspecified_Identifier>,
            mpl::_1, mpl::push_back<mpl::_1, details::get_identifier<mpl::_2> > > >::type Identifiers;
    BOOST_MPL_ASSERT((mpl::or_<
                      mpl::less_equal<typename mpl::size<Identifiers>::type, mpl::int_<1> >,
                      mpl::equal< typename mpl::count<Identifiers,
                      typename mpl::at_c<Identifiers,0> >::type, typename mpl::size<Identifiers>::type > >));

    typedef typename mpl::if_< mpl::empty<Identifiers>,
            No_Identifier, typename mpl::at_c<Identifiers, 0>::type >::type Identifier;


public:
    //get all module objects and properties
    typedef typename details::vector_fold<typename Type3::objects,
            typename details::vector_fold<typename Type2::objects,
            typename details::vector_fold<typename Type1::objects,
            mpl::vector<> >::type >::type>::type objects;

    typedef typename details::vector_fold<typename Type3::properties,
            typename details::vector_fold<typename Type2::properties,
            typename details::vector_fold<typename Type1::properties,
            mpl::vector<id_prop<Identifier> > >::type >::type>::type properties;

    //make the subcomponent lists of objects and properties
    typedef typename details::edge_fold< properties, mpl::vector<> >::type 	edge_properties;
    typedef typename details::vertex_fold< properties, mpl::vector<> >::type 	vertex_properties;
    typedef typename details::cluster_fold< properties,
            mpl::vector<changed_prop, type_prop>  >::type 			cluster_properties;

    typedef typename details::property_map<objects, properties>::type 		object_properties;

protected:
    //object storage
    typedef typename mpl::transform<objects, boost::shared_ptr<mpl::_1> >::type sp_objects;
    typedef typename mpl::fold< sp_objects, mpl::vector<>,
            mpl::push_back<mpl::_1, std::vector<mpl::_2> > >::type 	object_vectors;
    typedef typename fusion::result_of::as_vector<object_vectors>::type Storage;

    template<typename FT1, typename FT2, typename FT3>
    friend struct Object;

    struct clearer {
        template<typename T>
        void operator()(T& vector) const {
            vector.clear();
        };
    };

#ifdef USE_LOGGING
    boost::shared_ptr< sink_t > sink;
#endif

public:
    typedef ClusterGraph<edge_properties, vertex_properties, cluster_properties, objects> Cluster;
    typedef Sheduler< BaseType > Shedule;
    typedef KernelType Kernel;

public:
    System() : m_cluster(new Cluster), m_storage(new Storage)
#ifdef USE_LOGGING
    , sink(init_log())
#endif
    {
        Type1::system_init(*this);
        Type2::system_init(*this);
        Type3::system_init(*this);

    };


    ~System() {
#ifdef USE_LOGGING
        stop_log(sink);
#endif
    };

    void clear() {

        m_cluster->clearClusters();
        m_cluster->clear();
        fusion::for_each(*m_storage, clearer());
    };

    template<typename Object>
    typename std::vector< boost::shared_ptr<Object> >::iterator begin() {

        typedef typename mpl::find<objects, Object>::type iterator;
        typedef typename mpl::distance<typename mpl::begin<objects>::type, iterator>::type distance;
        BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<objects>::type > >));
        return fusion::at<distance>(*m_storage).begin();
    };

    template<typename Object>
    typename std::vector< boost::shared_ptr<Object> >::iterator end() {

        typedef typename mpl::find<objects, Object>::type iterator;
        typedef typename mpl::distance<typename mpl::begin<objects>::type, iterator>::type distance;
        BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<objects>::type > >));
        return fusion::at<distance>(*m_storage).end();
    };

    template<typename Object>
    std::vector< boost::shared_ptr<Object> >& objectVector() {

        typedef typename mpl::find<objects, Object>::type iterator;
        typedef typename mpl::distance<typename mpl::begin<objects>::type, iterator>::type distance;
        BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<objects>::type > >));
        return fusion::at<distance>(*m_storage);
    };

    template<typename Object>
    void push_back(boost::shared_ptr<Object> ptr) {
        objectVector<Object>().push_back(ptr);
    };

    template<typename Object>
    void erase(boost::shared_ptr<Object> ptr) {

        std::vector< boost::shared_ptr<Object> >& vec = objectVector<Object>();
        vec.erase(std::remove(vec.begin(), vec.end(), ptr), vec.end());
    };

    void solve() {
        clock_t start = clock();
        m_sheduler.execute(*this);
        clock_t end = clock();
        double ms = (double(end-start)* 1000.) / double(CLOCKS_PER_SEC);
        //Base::Console().Message("overall solving time in ms: %f\n", ms);

    };
    
    System* createSubsystem() {
	
      System* s = new System();
      s->m_cluster = m_cluster->createCluster().first;
      s->m_storage = m_storage;
      s->m_cluster->template setClusterProperty<dcm::type_prop>(details::subcluster);
      return s;
    };

private:
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

public:
    void copyInto(System& into) const {

        //copy the clustergraph and clone all objects while at it. They are also pushed to the storage
        cloner cl(into);
        m_cluster->copyInto(into.m_cluster, cl);

        //notify all modules that they are copied
        Type1::system_copy(*this, into);
        Type2::system_copy(*this, into);
        Type3::system_copy(*this, into);
    };

    System* clone() const {

        System* ns = new System();
        this->copyInto(*ns);
        return ns;
    };

    boost::shared_ptr<Cluster> m_cluster;
    boost::shared_ptr<Storage> m_storage;
    Shedule m_sheduler;
    Kernel  m_kernel;
};

}
#endif //GCM_SYSTEM_H



