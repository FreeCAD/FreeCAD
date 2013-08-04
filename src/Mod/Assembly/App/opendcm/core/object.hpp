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
    template<typename Sys, typename Derived, typename Sig> \
    template < \
    typename S  \
    BOOST_PP_ENUM_TRAILING_PARAMS(n, typename Arg) \
    > \
    void Object<Sys, Derived, Sig>::emitSignal( \
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

//few standart signal names
struct remove {};
struct recalculated {};

typedef boost::any Connection;

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
struct Object : public boost::enable_shared_from_this<Derived> {

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

    /**
      * @brief Access properties
      *
      * Returns a reference to the propertys actual value. The property type has to be registerd to the
      * System type which was given as template parameter to this object.
      * @tparam Prop property type which should be accessed
      * @return Prop::type& a reference to the properties actual value.
      **/
    template<typename Prop>
    typename Prop::type& getProperty();

    /**
       * @brief Set properties
       *
       * Set'S the value of a specified property. The property type has to be registerd to the
       * System type which was given as template parameter to this object. Note that setProperty(value)
       * is equivalent to getProperty() = value.
       * @tparam Prop property type which should be setProperty
       * @param value value of type Prop::type which should be set in this object
       **/
    template<typename Prop>
    void setProperty(typename Prop::type value);

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
    Connection connectSignal(typename mpl::at<Sig, S>::type function);

    /**
    * @brief Disconnects a slot for a specific signal.
    *
    * Disconnects a slot so that it dosn't get called at signal emittion. It's important to
    * disconnect the slot by the same boost:function it was connected with.
    *
    * @tparam S the signal type of interest
    * @param function boost::function with which the slot was connected
    * @return void
    **/
    template<typename S>
    void disconnectSignal(Connection c);

    /*properties
     * search the property map of the system class and get the mpl::vector of properties for the
     * derived type. It's imortant to not store the properties but their types. These types are
     * stored and accessed as fusion vector.
     * */
    typedef typename mpl::at<typename Sys::object_properties, Derived>::type Mapped;
    typedef typename mpl::if_< boost::is_same<Mapped, mpl::void_ >, mpl::vector0<>, Mapped>::type Sequence;
    typedef typename details::pts<Sequence>::type Properties;

    Properties m_properties;
    Sys* m_system;

protected:
    /*signal handling
     * extract all signal types to allow index search (inex search on signal functions would fail as same
     * signatures are supported for multiple signals). Create std::vectors to allow multiple slots per signal
     * and store these vectors in a fusion::vector for easy access.
     * */
    typedef typename mpl::fold< Sig, mpl::vector<>,
            mpl::push_back<mpl::_1, mpl::key_type<Sig, mpl::_2> > >::type sig_name;
    typedef typename mpl::fold< Sig, mpl::vector<>,
            mpl::push_back<mpl::_1, mpl::value_type<Sig, mpl::_2> > >::type sig_functions;
    typedef typename mpl::fold< sig_functions, mpl::vector<>,
            mpl::push_back<mpl::_1, std::list<mpl::_2> > >::type sig_vectors;
    typedef typename fusion::result_of::as_vector<sig_vectors>::type Signals;

    Signals m_signals;

public:
    //with no vararg templates before c++11 we need preprocessor to create the overloads of emit signal we need
    BOOST_PP_REPEAT(5, EMIT_CALL_DEF, ~)
};


/*****************************************************************************************************************/
/*****************************************************************************************************************/
/*****************************************************************************************************************/
/*****************************************************************************************************************/


template<typename Sys, typename Derived, typename Sig>
Object<Sys, Derived, Sig>::Object(Sys& system) : m_system(&system) {};

template<typename Sys, typename Derived, typename Sig>
boost::shared_ptr<Derived> Object<Sys, Derived, Sig>::clone(Sys& newSys) {

    boost::shared_ptr<Derived> np = boost::shared_ptr<Derived>(new Derived(*static_cast<Derived*>(this)));
    np->m_system = &newSys;
    return np;
};

template<typename Sys, typename Derived, typename Sig>
template<typename Prop>
typename Prop::type& Object<Sys, Derived, Sig>::getProperty() {
    typedef typename mpl::find<Sequence, Prop>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<Sequence>::type, iterator>::type distance;
    BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<Sequence>::type > >));
    return fusion::at<distance>(m_properties);
};

template<typename Sys, typename Derived, typename Sig>
template<typename Prop>
void Object<Sys, Derived, Sig>::setProperty(typename Prop::type value) {
    typedef typename mpl::find<Sequence, Prop>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<Sequence>::type, iterator>::type distance;
    BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<Sequence>::type > >));
    fusion::at<distance>(m_properties) = value;
};

template<typename Sys, typename Derived, typename Sig>
template<typename S>
Connection Object<Sys, Derived, Sig>::connectSignal(typename mpl::at<Sig, S>::type function) {
    typedef typename mpl::find<sig_name, S>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<sig_name>::type, iterator>::type distance;
    typedef typename fusion::result_of::value_at<Signals, distance>::type list_type;
    list_type& list = fusion::at<distance>(m_signals);
    return list.insert(list.begin(),function);
};

template<typename Sys, typename Derived, typename Sig>
template<typename S>
void Object<Sys, Derived, Sig>::disconnectSignal(Connection c) {
    typedef typename mpl::find<sig_name, S>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<sig_name>::type, iterator>::type distance;

    typedef typename fusion::result_of::value_at<Signals, distance>::type list_type;
    list_type& list = fusion::at<distance>(m_signals);
    list.erase(boost::any_cast<typename list_type::iterator>(c));
};

BOOST_PP_REPEAT(5, EMIT_CALL_DEC, ~)

}

#endif //GCM_OBJECT_H


