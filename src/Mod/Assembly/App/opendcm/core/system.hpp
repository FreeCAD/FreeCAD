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
#include <boost/mpl/map.hpp>
#include <boost/mpl/vector/vector0.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/count.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/less_equal.hpp>

#include <boost/function.hpp>

#include "property.hpp"
#include "clustergraph.hpp"
#include "sheduler.hpp"
#include "logging.hpp"
#include "traits.hpp"
#include "object.hpp"
#include "kernel.hpp"

namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

namespace dcm {

struct No_Identifier {}; //struct to show that no identifier shall be used
struct Unspecified_Identifier {}; //struct to show that it doesn't matter which identifier type is used
struct solved {}; //signal emitted when solving is done

namespace details {
  
enum { subcluster = 10};

/**
 * @brief Appends a mpl sequences to another
 *
 * Makes two sequence to one by appending all types of the first to the second sequence. The new
 * mpl sequence can be accessed by the ::type typedef.
 * Usage: @code vector_fold<Seq1, Seq2>::type @endcode
 *
 * @tparam state the mpl sequence which will be expanded
 * @tparam seq the mpl sequence which will be appended
 **/
template<typename seq, typename state>
struct vector_fold : mpl::fold < seq, state,
        mpl::push_back<mpl::_1, mpl::_2> > {};
	
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

template<typename T>
struct get_identifier {
    typedef typename T::Identifier type;
};

template<typename seq, typename state>
struct map_fold : mpl::fold< seq, state, mpl::insert<mpl::_1,mpl::_2> > {};


template<typename state, typename seq1, typename seq2, typename seq3>
struct map_fold_3 {
  typedef typename details::map_fold<seq1,
            typename details::map_fold<seq2,
            typename details::map_fold<seq3,
            state >::type >::type>::type type;
};

struct nothing {};

template<int v>
struct EmptyModule {

    template<typename T>
    struct type {
        struct inheriter {
	  void system_sub(boost::shared_ptr<T> subsys) {};
	};
        typedef mpl::vector<>	properties;
        typedef mpl::vector<>   objects;
	typedef mpl::vector<>   geometries;
	typedef mpl::map<>	signals;
        typedef Unspecified_Identifier Identifier;

        static void system_init(T& sys) {};
        static void system_copy(const T& from, T& into) {};
    };
};

template <class T>
struct is_shared_ptr : boost::mpl::false_ {};
template <class T>
struct is_shared_ptr<boost::shared_ptr<T> > : boost::mpl::true_ {};

template<typename Sys, typename M1, typename M2, typename  M3>
struct inheriter : public M1::inheriter, public M2::inheriter, public M3::inheriter,
		   dcm::SignalOwner<typename details::map_fold_3< mpl::map<mpl::pair<solved, boost::function<void (Sys*)> > >, 
		   typename M1::signals, typename M2::signals, typename M3::signals>::type> {};
}

template< typename KernelType,
          typename  T1 = details::EmptyModule<1>,
          typename  T2 = details::EmptyModule<2>,
          typename  T3 = details::EmptyModule<3> >
class System : 	public details::inheriter<System<KernelType,T1,T2,T3>, typename T1::template type< System<KernelType,T1,T2,T3> >,
   	typename T2::template type< System<KernelType,T1,T2,T3> >,
	typename T3::template type< System<KernelType,T1,T2,T3> > > {

    typedef System<KernelType,T1,T2,T3> BaseType;
public:
  
    typedef T1 Module1;
    typedef T2 Module2;
    typedef T3 Module3;
    typedef typename T1::template type< BaseType > Type1;
    typedef typename T2::template type< BaseType > Type2;
    typedef typename T3::template type< BaseType > Type3;
    typedef mpl::vector3<Type1, Type2, Type3> TypeVector;
    typedef typename Type1::inheriter Inheriter1;
    typedef typename Type2::inheriter Inheriter2;
    typedef typename Type3::inheriter Inheriter3;

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
	    
    //get all geometries we support
    typedef typename details::vector_fold<typename Type3::geometries,
            typename details::vector_fold<typename Type2::geometries,
            typename details::vector_fold<typename Type1::geometries,
            mpl::vector<> >::type >::type>::type geometries;

    //make the subcomponent lists of objects and properties
    typedef typename details::edge_fold< properties, 
	    mpl::vector1<edge_index_prop> >::type 	edge_properties;
    typedef typename details::vertex_fold< properties, 
	    mpl::vector1<vertex_index_prop> >::type 	vertex_properties;
    typedef typename details::cluster_fold< properties,
            mpl::vector2<changed_prop, type_prop>  >::type cluster_properties;
	    
    //we hold our own PropertyOwner which we use for system settings. Don't inherit it as the user 
    //should not access the settings via the proeprty getter and setter functions.
    typedef PropertyOwner<typename details::properties_by_kind<properties, setting_property>::type> OptionOwner;
    boost::shared_ptr<OptionOwner> m_options;


protected:
    //object storage
    typedef typename mpl::transform<objects, boost::shared_ptr<mpl::_1> >::type sp_objects;
    typedef typename mpl::fold< sp_objects, mpl::vector<>,
            mpl::push_back<mpl::_1, std::vector<mpl::_2> > >::type 	object_vectors;
    typedef typename fusion::result_of::as_vector<object_vectors>::type Storage;

    template<typename FT1, typename FT2, typename FT3>
    friend struct Object;

#ifdef USE_LOGGING
    boost::shared_ptr< sink_t > sink;
#endif

public:
    typedef ClusterGraph<edge_properties, vertex_properties, cluster_properties, objects> Cluster;
    typedef Sheduler< BaseType > Shedule;
    typedef KernelType Kernel;

public:
    System();
    ~System();

    void clear();

    template<typename Object>
    typename std::vector< boost::shared_ptr<Object> >::iterator begin();

    template<typename Object>
    typename std::vector< boost::shared_ptr<Object> >::iterator end();

    template<typename Object>
    std::vector< boost::shared_ptr<Object> >& objectVector();

    template<typename Object>
    void push_back(boost::shared_ptr<Object> ptr);

    template<typename Object>
    void erase(boost::shared_ptr<Object> ptr);

    SolverInfo solve();
    
    boost::shared_ptr< System > createSubsystem();
    typename std::vector< boost::shared_ptr<System> >::iterator beginSubsystems();
    typename std::vector< boost::shared_ptr<System> >::iterator endSubsystems();
    
    //a kernel has it's own settings, therefore we need to decide which is accessed
    template<typename Option>
    typename boost::enable_if< boost::is_same< typename mpl::find<typename Kernel::PropertySequence, Option>::type,
    typename mpl::end<typename Kernel::PropertySequence>::type >, typename Option::type& >::type getOption();
    
    template<typename Option>
    typename boost::disable_if< boost::is_same< typename mpl::find<typename Kernel::PropertySequence, Option>::type,
    typename mpl::end<typename Kernel::PropertySequence>::type >, typename Option::type& >::type getOption();
    
    template<typename Option>
    typename boost::enable_if< boost::is_same< typename mpl::find<typename Kernel::PropertySequence, Option>::type,
    typename mpl::end<typename Kernel::PropertySequence>::type >, void >::type setOption(typename Option::type value);
    
    template<typename Option>
    typename boost::disable_if< boost::is_same< typename mpl::find<typename Kernel::PropertySequence, Option>::type,
    typename mpl::end<typename Kernel::PropertySequence>::type >, void >::type setOption(typename Option::type value);
    
    //convenience function
    template<typename Option>
    typename Option::type& option();
    
    //let evryone access and use our math kernel
    Kernel& kernel();

    void copyInto(System& into) const;

    System* clone() const;

    boost::shared_ptr<Cluster> m_cluster;
    boost::shared_ptr<Storage> m_storage;
    Shedule m_sheduler;
    Kernel  m_kernel;
    std::vector<boost::shared_ptr<System> > m_subsystems;

#ifdef USE_LOGGING
    template<typename Expr>
    void setLoggingFilter(const Expr& ex) {
      sink->set_filter(ex);
    }
#endif
};

//implementations which always need to be with the definition as they can't be externalised
//*****************************************************************************************

template< typename KernelType, typename T1, typename T2, typename T3 >
template<typename Object>
typename std::vector< boost::shared_ptr<Object> >::iterator System<KernelType, T1, T2, T3>::begin() {

    typedef typename mpl::find<objects, Object>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<objects>::type, iterator>::type distance;
    BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<objects>::type > >));
    return fusion::at<distance>(*m_storage).begin();
};

template< typename KernelType, typename T1, typename T2, typename T3 >
template<typename Object>
typename std::vector< boost::shared_ptr<Object> >::iterator System<KernelType, T1, T2, T3>::end() {

    typedef typename mpl::find<objects, Object>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<objects>::type, iterator>::type distance;
    BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<objects>::type > >));
    return fusion::at<distance>(*m_storage).end();
};

template< typename KernelType, typename T1, typename T2, typename T3 >
template<typename Object>
std::vector< boost::shared_ptr<Object> >& System<KernelType, T1, T2, T3>::objectVector() {

    typedef typename mpl::find<objects, Object>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<objects>::type, iterator>::type distance;
    BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<objects>::type > >));
    return fusion::at<distance>(*m_storage);
};

template< typename KernelType, typename T1, typename T2, typename T3 >
template<typename Object>
void System<KernelType, T1, T2, T3>::push_back(boost::shared_ptr<Object> ptr) {
    objectVector<Object>().push_back(ptr);
};

template< typename KernelType, typename T1, typename T2, typename T3 >
template<typename Object>
void System<KernelType, T1, T2, T3>::erase(boost::shared_ptr<Object> ptr) {

    std::vector< boost::shared_ptr<Object> >& vec = objectVector<Object>();
    vec.erase(std::remove(vec.begin(), vec.end(), ptr), vec.end());
};

template< typename KernelType, typename T1, typename T2, typename T3 >
template<typename Option>
typename boost::enable_if< boost::is_same< typename mpl::find<typename KernelType::PropertySequence, Option>::type,
         typename mpl::end<typename KernelType::PropertySequence>::type >, typename Option::type& >::type
System<KernelType, T1, T2, T3>::getOption() {
    return m_options->template getProperty<Option>();
};

template< typename KernelType, typename T1, typename T2, typename T3 >
template<typename Option>
typename boost::disable_if< boost::is_same< typename mpl::find<typename KernelType::PropertySequence, Option>::type,
         typename mpl::end<typename KernelType::PropertySequence>::type >, typename Option::type& >::type
System<KernelType, T1, T2, T3>::getOption() {
    return m_kernel.template getProperty<Option>();
};

template< typename KernelType, typename T1, typename T2, typename T3 >
template<typename Option>
typename boost::enable_if< boost::is_same< typename mpl::find<typename KernelType::PropertySequence, Option>::type,
         typename mpl::end<typename KernelType::PropertySequence>::type >, void >::type
System<KernelType, T1, T2, T3>::setOption(typename Option::type value) {
    m_options->template setProperty<Option>(value);
};

template< typename KernelType, typename T1, typename T2, typename T3 >
template<typename Option>
typename boost::disable_if< boost::is_same< typename mpl::find<typename KernelType::PropertySequence, Option>::type,
         typename mpl::end<typename KernelType::PropertySequence>::type >, void >::type
System<KernelType, T1, T2, T3>::setOption(typename Option::type value) {
    m_kernel.template setProperty<Option>(value);
};

template< typename KernelType, typename T1, typename T2, typename T3 >
template<typename Option>
typename Option::type&
System<KernelType, T1, T2, T3>::option() {
    return getOption<Option>();
};

}

#ifndef DCM_EXTERNAL_CORE
#include "imp/system_imp.hpp"
#endif

#endif //GCM_SYSTEM_H













