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

#ifndef GCM_OBJECT_H
#define GCM_OBJECT_H

#include <iostream>
#include <list>

#include <boost/mpl/at.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/void.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/key_type.hpp>
#include <boost/mpl/value_type.hpp>

#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/at.hpp>

#include <boost/preprocessor.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#include <boost/enable_shared_from_this.hpp>
#include <boost/any.hpp>

#include "property.hpp"


namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

/* Preprocessor implementation of emit signal. As we need many overloads with diffrent number of
 * templated parameters we use boost preprocessor to do the hard repetive work. The definition and
 * implementation are definded first as they need to be known before usage 
 * */
#define EMIT_ARGUMENTS(z, n, data) \
    BOOST_PP_CAT(data, n)

#define EMIT_CALL_DEF(z, n, data) \
    template < \
    typename S  \
    BOOST_PP_ENUM_TRAILING_PARAMS(n, typename Arg) \
    > \
    void emitSignal( \
                     BOOST_PP_ENUM_BINARY_PARAMS(n, Arg, const& arg) \
                   );

#define EMIT_CALL_DEC(z, n, data) \
    template<typename SigMap> \
    template < \
    typename S  \
    BOOST_PP_ENUM_TRAILING_PARAMS(n, typename Arg) \
    > \
    void SignalOwner<SigMap>::emitSignal( \
            BOOST_PP_ENUM_BINARY_PARAMS(n, Arg, const& arg) \
                                              ) \
    { \
        typedef typename mpl::find<sig_name, S>::type iterator; \
        typedef typename mpl::distance<typename mpl::begin<sig_name>::type, iterator>::type distance; \
        typedef typename fusion::result_of::value_at<Signals, distance>::type list_type; \
        list_type& list = fusion::at<distance>(m_signals); \
        for (typename list_type::iterator it=list.begin(); it != list.end(); it++) \
            (*it)(BOOST_PP_ENUM(n, EMIT_ARGUMENTS, arg)); \
    };

namespace dcm {

/** @defgroup Objects Objects
 *
 * @brief Concept and functionality of the dcm objects
 *
 *
 **/

//few standart signal names
struct remove {};

typedef boost::any Connection;

template<typename SigMap>
struct SignalOwner {

    /**
    * @brief Connects a slot to a specified signal.
    *
    * Slots are boost::functions which get called when the signal is emitted. Any valid boost::function
    * which ressembles the signal tyes signature can be registert. It is important that the signal type
    * was registerd to this object on creation by the appropriate template parameter.
    *
    * @tparam S the signal which should be intercepted
    * @param function boost::function which resembles the signal type's signature
    * @return void
    **/
    template<typename S>
    Connection connectSignal(typename mpl::at<SigMap, S>::type function);

    /**
    * @brief Disconnects a slot for a specific signal.
    *
    * Disconnects a slot so that it dosn't get called at signal emittion. It's important to
    * disconnect the slot by the same boost:function it was connected with.
    *
    * @tparam S the signal type of interest
    * @param c connection with which the slot was initialy connected
    * @return void
    **/
    template<typename S>
    void disconnectSignal(Connection c);

    //with no vararg templates before c++11 we need preprocessor to create the overloads of emit signal we need
    BOOST_PP_REPEAT(5, EMIT_CALL_DEF, ~)
    
protected:
    /*signal handling
     * extract all signal types to allow index search (inex search on signal functions would fail as same
     * signatures are supported for multiple signals). Create std::vectors to allow multiple slots per signal
     * and store these vectors in a fusion::vector for easy access.
     * */
    typedef typename mpl::fold < SigMap, mpl::vector<>,
            mpl::push_back<mpl::_1, mpl::key_type<SigMap, mpl::_2> > >::type sig_name;
    typedef typename mpl::fold < SigMap, mpl::vector<>,
            mpl::push_back<mpl::_1, mpl::value_type<SigMap, mpl::_2> > >::type sig_functions;
    typedef typename mpl::fold < sig_functions, mpl::vector<>,
            mpl::push_back<mpl::_1, std::list<mpl::_2> > >::type sig_vectors;
    typedef typename fusion::result_of::as_vector<sig_vectors>::type Signals;

    Signals m_signals;
};

/**
 * @brief Base class for all object types
 *
 * This class add's property and signal capabilitys to all deriving classes. For properties it is tigthly
 * integrated with the system class: It searches systems property list for the derived class as specified by
 * the second template parameter and makes it accessible via appopriate functions. Signals are speciefied by a
 * mpl::map with signal name type as key and a boost::function as values.
 *
 * \tparam Sys class of type System of which the properties are taken
 * \tparam Obj the type of the derived object
 * \tparam Sig a mpl::map specifing the object's signals by (type -  boost::function) pairs
 **/
template<typename Sys, typename Derived, typename Sig>
struct Object : public PropertyOwner<typename details::properties_by_object<typename Sys::properties, Derived>::type>,
	public SignalOwner<Sig>,
        boost::enable_shared_from_this<Derived> {

    Object() {};
    Object(Sys& system);

    /**
      * @brief Create a new object of the same type with the same values
      *
      * Returns a new shared_ptr for the Drived type with the same properties as the initial one. If
      * the new pointer should be used in a new system the parameter \param newSys needs to be that
      * new system. Overload this function if you have datamembers in any derived class wich are not
      * copyconstructable.
      * @tparam Prop property type which should be accessed
      * @return Prop::type& a reference to the properties actual value.
      **/
    virtual boost::shared_ptr<Derived> clone(Sys& newSys);

    Sys* m_system;
};


/*****************************************************************************************************************/
/*****************************************************************************************************************/
/*****************************************************************************************************************/
/*****************************************************************************************************************/


template<typename Sys, typename Derived, typename Sig>
Object<Sys, Derived, Sig>::Object(Sys& system) : m_system(&system) {};

template<typename Sys, typename Derived, typename Sig>
boost::shared_ptr<Derived> Object<Sys, Derived, Sig>::clone(Sys& newSys)
{

    boost::shared_ptr<Derived> np = boost::shared_ptr<Derived>(new Derived(*static_cast<Derived*>(this)));
    np->m_system = &newSys;
    return np;
};

template<typename SigMap>
template<typename S>
Connection SignalOwner<SigMap>::connectSignal(typename mpl::at<SigMap, S>::type function)
{
    typedef typename mpl::find<sig_name, S>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<sig_name>::type, iterator>::type distance;
    typedef typename fusion::result_of::value_at<Signals, distance>::type list_type;
    list_type& list = fusion::at<distance>(m_signals);
    return list.insert(list.begin(), function);
};

template<typename SigMap>
template<typename S>
void SignalOwner<SigMap>::disconnectSignal(Connection c)
{
    typedef typename mpl::find<sig_name, S>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<sig_name>::type, iterator>::type distance;

    typedef typename fusion::result_of::value_at<Signals, distance>::type list_type;
    list_type& list = fusion::at<distance>(m_signals);
    list.erase(boost::any_cast<typename list_type::iterator>(c));
};

BOOST_PP_REPEAT(5, EMIT_CALL_DEC, ~)

}

#endif //GCM_OBJECT_H


