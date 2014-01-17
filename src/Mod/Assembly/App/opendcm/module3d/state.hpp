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

#ifndef DCM_MODULE3D_STATE_HPP
#define DCM_MODULE3D_STATE_HPP

#include <opendcm/moduleState/traits.hpp>
#include <opendcm/core/clustergraph.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>

#include <ios>

namespace karma = boost::spirit::karma;
namespace qi = boost::spirit::qi;

namespace dcm {

namespace details {
template<typename Sys>
struct getModule3D {
    typedef typename system_traits<Sys>::template getModule<m3d>::type type;
};
}

template<typename System>
struct parser_generate< typename details::getModule3D<System>::type::Geometry3D , System>
        : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_generator< typename details::getModule3D<System>::type::Geometry3D , System, iterator > {

    typedef typename details::getModule3D<System>::type::Geometry3D  Geometry;
    typedef karma::rule<iterator, boost::shared_ptr<Geometry>(), karma::locals<int> > generator;
    static void init(generator& r);
};


template<typename System>
struct parser_generate< typename details::getModule3D<System>::type::vertex_prop , System>
        : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_generator< typename details::getModule3D<System>::type::vertex_prop , System, iterator > {

    typedef karma::rule<iterator, GlobalVertex()> generator;
    static void init(generator& r);
};
/*
template<typename System>
struct parser_generate< typename details::getModule3D<System>::type::math_prop , System>
        : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_generator< typename details::getModule3D<System>::type::math_prop , System, iterator > {

    typedef karma::rule<iterator, details::ClusterMath<System>&(), karma::locals<int> > generator;
    static void init(generator& r);
};*/

template<typename System>
struct parser_generate< typename details::getModule3D<System>::type::Constraint3D , System>
        : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_generator< typename details::getModule3D<System>::type::Constraint3D , System, iterator > {

    typedef typename details::getModule3D<System>::type::Geometry3D  Geometry3D;
    typedef typename details::getModule3D<System>::type::Constraint3D  Constraint3D;
    typedef typename details::getModule3D<System>::type::vertex_prop vertex_prop;
    typedef typename details::getModule3D<System>::type::edge_prop edge_prop;
    typedef karma::rule<iterator, boost::shared_ptr<Constraint3D>()> generator;
    static void init(generator& r);
};

template<typename System>
struct parser_generate< typename details::getModule3D<System>::type::edge_prop , System>
        : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_generator< typename details::getModule3D<System>::type::edge_prop , System, iterator > {

    typedef karma::rule<iterator, GlobalEdge&()> generator;
    static void init(generator& r);
};

template<typename System>
struct parser_generate<typename details::getModule3D<System>::type::fix_prop, System> : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_generator<typename details::getModule3D<System>::type::fix_prop, System, iterator> {
    typedef karma::rule<iterator, bool&()> generator;

    static void init(generator& r);
};

/****************************************************************************************************/
/****************************************************************************************************/

template<typename System>
struct parser_parse< typename details::getModule3D<System>::type::Geometry3D , System>
        : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_parser< typename details::getModule3D<System>::type::Geometry3D, System, iterator > {

    typedef typename details::getModule3D<System>::type::Geometry3D  object_type;
    typedef typename System::Kernel Kernel;

    typedef qi::rule<iterator, boost::shared_ptr<object_type>(System*), qi::space_type, qi::locals<std::string, typename Kernel::Vector, int> > parser;
    static void init(parser& r);
};

template<typename System>
struct parser_parse< typename details::getModule3D<System>::type::vertex_prop, System>
        : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_parser< typename details::getModule3D<System>::type::vertex_prop, System, iterator > {

    typedef qi::rule<iterator, GlobalVertex(), qi::space_type> parser;
    static void init(parser& r);
};
/*
template<typename System>
struct parser_parse< typename details::getModule3D<System>::type::math_prop, System>
        : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_parser< typename details::getModule3D<System>::type::math_prop, System, iterator > {

    typedef qi::rule<iterator, details::ClusterMath<System>(), qi::space_type > parser;
    static void init(parser& r);
};*/

template<typename System>
struct parser_parse< typename details::getModule3D<System>::type::Constraint3D , System>
        : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_parser< typename details::getModule3D<System>::type::Constraint3D, System, iterator > {

    typedef typename details::getModule3D<System>::type::Geometry3D  Geometry3D;
    typedef typename details::getModule3D<System>::type::Constraint3D  Constraint3D;
    typedef typename System::Kernel Kernel;

    typedef qi::rule<iterator, boost::shared_ptr<Constraint3D>(System*), qi::space_type > parser;
    static void init(parser& r);
};

template<typename System>
struct parser_parse< typename details::getModule3D<System>::type::edge_prop, System>
        : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_parser< typename details::getModule3D<System>::type::edge_prop, System, iterator > {

    typedef qi::rule<iterator, GlobalEdge(), qi::space_type> parser;
    static void init(parser& r);
};

template<typename System>
struct parser_parse< typename details::getModule3D<System>::type::fix_prop, System>
        : public mpl::true_ {};

template<typename System, typename iterator>
struct parser_parser< typename details::getModule3D<System>::type::fix_prop, System, iterator > {

    typedef qi::rule<iterator, bool(), qi::space_type> parser;
    static void init(parser& r);
};

}

#ifndef DCM_EXTERNAL_STATE
#include "imp/state_imp.hpp"
#endif

#endif //DCM_MODULE3D_STATE_HPP
