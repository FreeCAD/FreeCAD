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

#ifndef GCM_TRAITS_H
#define GCM_TRAITS_H

#include <boost/type_traits.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/void.hpp>
#include <boost/mpl/vector.hpp>

#include <boost/preprocessor/repetition/enum_shifted.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/repetition/enum.hpp>

#include <string.h>

namespace mpl = boost::mpl;

namespace dcm {

template< typename T >
struct system_traits {
    //typedef typename T::Kernel  Kernel;
    //typedef typename T::Cluster Cluster;

    template<typename M>
    struct getModule {

        typedef typename mpl::if_< boost::is_base_of<M, typename T::Type1>, typename T::Type1, typename T::Type2 >::type test1;
        typedef typename mpl::if_< boost::is_base_of<M, test1>, test1, typename T::Type3 >::type test2;
        typedef typename mpl::if_< boost::is_base_of<M, test2>, test2, mpl::void_ >::type type;

        typedef mpl::not_<boost::is_same<type, mpl::void_> > has_module;
    };
};

template<typename T>
struct compare_traits {

    BOOST_MPL_ASSERT_MSG((mpl::not_<boost::is_same<T, const char*> >::value),
                         YOU_SHOULD_NOT_USE_THIS_TYPE_AS_IDENTIFIER,
                         (const char*));

    static bool compare(T& first, T& second) {
        return first == second;
    };
};

template<>
struct compare_traits<std::string> {

    static bool compare(std::string& first, std::string& second) {
        return !(first.compare(second));
    };
};

template<typename seq, int size>
struct vector_shrink {
    typedef typename mpl::copy< seq, mpl::back_inserter< mpl::vector<> > >::type type;
};

template<typename seq>
struct vector_shrink< seq, 0 > {
    typedef mpl::vector0<> type;
};
template<typename seq>
struct vector_shrink< seq, 1 > {
    typedef mpl::vector1< typename mpl::at_c< seq, 0 >::type >  type;
};

#define SEQ_TYPES(z, n, data) \
    typename mpl::at_c< seq, n >::type
    
#define VECTOR_SHRINK(z, n, data) \
 template< typename seq> \
 struct vector_shrink<seq, n> {  \
 typedef BOOST_PP_CAT(mpl::vector, n) < \
  BOOST_PP_ENUM(n, SEQ_TYPES, ~) > type; \
 };
 
 BOOST_PP_REPEAT_FROM_TO(2,10, VECTOR_SHRINK, ~)

}//namespace dcm

#endif //GCM_TRAITS_H
