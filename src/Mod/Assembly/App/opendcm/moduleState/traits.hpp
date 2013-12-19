/*
    openDCM, dimensional constraint manager
    Copyright (C) 2013  Stefan Troeger <stefantroeger@gmx.net>

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

#ifndef DCM_PARSER_TRAITS_H
#define DCM_PARSER_TRAITS_H

#ifndef BOOST_SPIRIT_USE_PHOENIX_V3
#define BOOST_SPIRIT_USE_PHOENIX_V3
#endif

#include <boost/mpl/bool.hpp>
#include <assert.h>

namespace dcm {

template<typename type, typename System>
struct parser_generate : public boost::mpl::false_ {};

template<typename type, typename System, typename iterator>
struct parser_generator {
    typedef int generator;

    static void init(generator& r) {
        assert(false);
    };
};

template<typename type, typename System>
struct parser_parse : public boost::mpl::false_ {};

template<typename type, typename System, typename iterator>
struct parser_parser {
    typedef int parser;

    static void init(parser& r) {
        assert(false);
    };
};

}

#ifndef DCM_EXTERNAL_STATE
#include "imp/traits_impl.hpp"
#endif
#endif //DCM_PARSER_TRAITS_H
