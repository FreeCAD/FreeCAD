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

#ifndef GCM_OBJECT_IMP_H
#define GCM_OBJECT_IMP_H

#include <iostream>
#include <map>

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

#include <boost/enable_shared_from_this.hpp>

#include "../property.hpp"

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
      if(m_emit_signals) {\
          typedef typename mpl::find<sig_name, S>::type iterator; \
          typedef typename mpl::distance<typename mpl::begin<sig_name>::type, iterator>::type distance; \
          typedef typename fusion::result_of::value_at<Signals, distance>::type map_type; \
          map_type& map = fusion::at<distance>(m_signals); \
          for (typename map_type::iterator it=map.begin(); it != map.end(); it++) \
              (it->second)(BOOST_PP_ENUM(n, EMIT_ARGUMENTS, arg)); \
      }\
    };

namespace dcm {

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
SignalOwner<SigMap>::SignalOwner() : m_emit_signals(true), m_signal_count(0) {};

template<typename SigMap>
template<typename S>
Connection SignalOwner<SigMap>::connectSignal(typename mpl::at<SigMap, S>::type function)
{
    typedef typename mpl::find<sig_name, S>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<sig_name>::type, iterator>::type distance;
    typedef typename fusion::result_of::value_at<Signals, distance>::type map_type;
    map_type& map = fusion::at<distance>(m_signals);
    map[++m_signal_count] = function;
    return m_signal_count;
};

template<typename SigMap>
template<typename S>
void SignalOwner<SigMap>::disconnectSignal(Connection c)
{
    typedef typename mpl::find<sig_name, S>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<sig_name>::type, iterator>::type distance;

    typedef typename fusion::result_of::value_at<Signals, distance>::type map_type;
    map_type& map = fusion::at<distance>(m_signals);
    map.erase(c);
};

template<typename SigMap>
void SignalOwner<SigMap>::enableSignals(bool onoff)
{
    m_emit_signals = onoff;
};

BOOST_PP_REPEAT(5, EMIT_CALL_DEC, ~)

}

#endif //GCM_OBJECT_H


