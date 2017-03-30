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

//move the traits specializations outside of the traits definition to avoid the spirit header parsing every
//time this module is included and just parse it in externalization mode when the generator is built

#ifndef DCM_PARSER_TRAITS_IMPL_H
#define DCM_PARSER_TRAITS_IMPL_H

#include "../traits.hpp"
#include "../defines.hpp"
#include "opendcm/core/kernel.hpp"

#include <boost/mpl/bool.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_int.hpp>
#include <boost/spirit/include/karma_bool.hpp>
#include <boost/spirit/include/karma_rule.hpp>
#include <boost/spirit/include/karma_auto.hpp>

namespace karma = boost::spirit::karma;
namespace qi = boost::spirit::qi;

namespace boost {
namespace spirit {
namespace traits {
template <>
struct create_generator<dcm::No_Identifier> {

    typedef BOOST_TYPEOF(karma::eps(false)) type;
    static type call()  {
        return karma::eps(false);
    }
};
}
}
}

namespace dcm {
  
  template<typename System>
struct parser_generate<type_prop, System> : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_generator<type_prop, System, iterator> {
    typedef karma::rule<iterator, int&()> generator;

    static void init(generator& r) {
        r = karma::lit("<type>clustertype</type>\n<value>") << karma::int_ <<"</value>";
    };
};

template<typename System>
struct parser_generate<changed_prop, System> : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_generator<changed_prop, System, iterator> {
    typedef karma::rule<iterator, bool&()> generator;

    static void init(generator& r) {
        r = karma::lit("<type>clusterchanged</type>\n<value>") << karma::bool_ <<"</value>";
    };
};

template<typename System>
struct parser_generate<precision, System> : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_generator<precision, System, iterator> {
    typedef karma::rule<iterator, double&()> generator;

    static void init(generator& r) {
        r = karma::lit("<type>precision</type>\n<value>") << karma::double_ <<"</value>";
    };
};

template<typename System>
struct parser_generate<id_prop<typename System::Identifier>, System>
        : public mpl::not_<boost::is_same<typename System::Identifier, No_Identifier> > {};

template<typename System, typename iterator>
struct parser_generator<id_prop<typename System::Identifier>, System, iterator> {
    typedef karma::rule<iterator, typename System::Identifier()> generator;

    static void init(generator& r) {
        r = karma::lit("<type>id</type>\n<value>") << karma::auto_ <<"</value>";
    };
};

  template<typename System>
struct parser_parse<type_prop, System> : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_parser<type_prop, System, iterator> {
    typedef qi::rule<iterator, int(), qi::space_type> parser;

    static void init(parser& r) {
        r = qi::lit("<type>clustertype</type>") >> ("<value>") >> qi::int_ >>"</value>";
    };
};

template<typename System>
struct parser_parse<changed_prop, System> : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_parser<changed_prop, System, iterator> {
    typedef qi::rule<iterator, bool(), qi::space_type> parser;

    static void init(parser& r) {
        r = qi::lit("<type>clusterchanged</type>") >> ("<value>") >> qi::bool_ >>"</value>" ;
    };
};

template<typename System>
struct parser_parse<precision, System> : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_parser<precision, System, iterator> {
    typedef qi::rule<iterator, double(), qi::space_type> parser;

    static void init(parser& r) {
        r = qi::lit("<type>precision</type>") >> ("<value>") >> qi::double_ >>"</value>" ;
    };
};

template<typename System>
struct parser_parse<id_prop<typename System::Identifier>, System>
        : public mpl::not_<boost::is_same<typename System::Identifier, No_Identifier> > {};

template<typename System, typename iterator>
struct parser_parser<id_prop<typename System::Identifier>, System, iterator> {
    typedef qi::rule<iterator, typename System::Identifier(), qi::space_type> parser;

    static void init(parser& r) {
        r = qi::lit("<type>id</type>") >> ("<value>") >> qi::auto_ >>"</value>";
    };
};
/*
template<typename System>
struct parser_generate<details::cluster_vertex_prop, System>
        : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_generator<details::cluster_vertex_prop, System, iterator> {
    typedef karma::rule<iterator, int()> generator;

    static void init(generator& r) {
        r = karma::lit("<type>id</type>\n<value>") << karma::int_ <<"</value>";
    };
};

template<typename System>
struct parser_parse<details::cluster_vertex_prop, System> : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_parser<details::cluster_vertex_prop, System, iterator> {
    typedef qi::rule<iterator, int(), qi::space_type> parser;

    static void init(parser& r) {
        r = qi::lit("<type>id</type>") >> ("<value>") >> qi::int_ >>"</value>";
    };
};*/

} //namespace dcm

#endif //DCM_PARSER_TRAITS_IMPL_H