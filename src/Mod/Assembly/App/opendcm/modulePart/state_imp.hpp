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

#ifndef DCM_MODULEPART_STATE_IMP_HPP
#define DCM_MODULEPART_STATE_IMP_HPP

#include "state.hpp"

#include "module.hpp"

#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix/function/adapt_function.hpp>
#include <boost/phoenix/fusion/at.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/greater.hpp>
#include <boost/tokenizer.hpp>

namespace karma_ascii = boost::spirit::karma::ascii;
namespace qi_ascii = boost::spirit::qi::ascii;
namespace phx = boost::phoenix;

#include <ios>

namespace karma = boost::spirit::karma;
namespace qi = boost::spirit::qi;
namespace karma_ascii = boost::spirit::karma::ascii;
namespace qi_ascii = boost::spirit::qi::ascii;
namespace phx = boost::phoenix;

namespace dcm {

namespace details {

template <typename Transform, typename Row, typename Value>
bool TranslationOutput(Transform& v, Row& r, Value& val) {

    if(r < 4) {

        val = v.translation().vector()(r++);
        return true; // output continues
    }

    return false;    // fail the output
};

template <typename Transform, typename Row, typename Value>
bool TranslationInput(Transform& v, Row& r, Value& val) {

    v.translation().vector()(r++) = val;
    return true; // input continues
};

template <typename Transform, typename Row, typename Value>
bool RotationOutput(Transform& v, Row& r, Value& val) {

    if(r < 5) {
        val = v.rotation().vector()(r++);
        return true; // output continues
    }

    return false;    // fail the output
};

template <typename Transform, typename Row, typename Value>
bool RotationInput(Transform& v, Row& r, Value& val) {

    v.rotation().vector()(r++) = val;
    return true; // output continues
};

template<typename Geom>
struct inject_set {

    template<typename Vec, typename Obj>
    static void apply(Vec& v, Obj g) {
        Geom gt;
        (typename geometry_traits<Geom>::modell()).template inject<double,
        typename geometry_traits<Geom>::accessor >(gt, v);
        g->set(gt);
    };
};
//spezialisation if no type in the typelist has the right weight
template<>
struct inject_set<mpl::void_> {

    template<typename Obj, typename Vec>
    static void apply(Vec& v, Obj g) {
        //TODO:throw
    };
};

template<typename System>
bool Create(System* sys, std::string& type,
            boost::shared_ptr<typename details::getModule3D<System>::type::Geometry3D> geom,
            typename System::Kernel::Vector& v) {

    typedef typename details::getModule3D<System>::type::geometry_types Typelist;

    if(type.compare("direction") == 0) {
        inject_set<typename getWeightType<Typelist, tag::weight::direction>::type>::apply(v, geom);
    }
    else if(type.compare("point") == 0) {
        inject_set<typename getWeightType<Typelist, tag::weight::point>::type>::apply(v, geom);
    }
    else if(type.compare("line") == 0) {
        inject_set<typename getWeightType<Typelist, tag::weight::line>::type>::apply(v, geom);
    }
    else if(type.compare("plane") == 0) {
        inject_set<typename getWeightType<Typelist, tag::weight::plane>::type>::apply(v, geom);
    }
    else if(type.compare("cylinder") == 0) {
        inject_set<typename getWeightType<Typelist, tag::weight::cylinder>::type>::apply(v, geom);
    };

    return true;
};

// define a new real number formatting policy
template <typename Num>
struct scientific_policy : karma::real_policies<Num>
{
    // we want the numbers always to be in scientific format
    static int floatfield(Num n) {
        return std::ios::scientific;
    }
    static unsigned precision(Num n) {
        return 16;
    };
};

// define a new generator type based on the new policy
typedef karma::real_generator<double, scientific_policy<double> > science_type;
static science_type const scientific = science_type();
} //details
} //dcm

BOOST_PHOENIX_ADAPT_FUNCTION(bool, vector_out, dcm::details::VectorOutput, 3)
BOOST_PHOENIX_ADAPT_FUNCTION(bool, vector_in,  dcm::details::VectorInput, 3)
BOOST_PHOENIX_ADAPT_FUNCTION(bool, create,  dcm::details::Create, 4)

BOOST_FUSION_ADAPT_STRUCT(
    dcm::GlobalEdge,
    (int, ID)
    (int, source)
    (int, target)
)

namespace dcm {


template<typename System, typename iterator>
void parser_generator< typename details::getModule3D<System>::type::Geometry3D , System, iterator >::init(generator& r) {

    r = karma::lit("<type>Geometry3D</type>\n<class>")
        << karma_ascii::string[karma::_1 = phx::bind(&details::getWeight<Geometry>, karma::_val)]
        << "</class>" << karma::eol << "<value>"
        << (details::scientific[ boost::spirit::_pass = vector_out(karma::_val, karma::_a, karma::_1) ] % ' ')
        << "</value>";
};


template<typename System, typename iterator>
void parser_generator< typename details::getModule3D<System>::type::vertex_prop , System, iterator >::init(generator& r) {

    r = karma::lit("<type>Vertex</type>")
        << karma::eol << "<value>" << karma::int_ << "</value>";
};

template<typename System, typename iterator>
void parser_generator< typename details::getModule3D<System>::type::Constraint3D , System, iterator >::init(generator& r) {

    r = karma::lit("<type>Constraint3D</type>") << karma::eol
        << "<connect first=" << karma::int_[karma::_1 = phx::at_c<1>(phx::bind(&Constraint3D::template getProperty<edge_prop>, karma::_val))]
        << " second=" << karma::int_[karma::_1 = phx::at_c<2>(phx::bind(&Constraint3D::template getProperty<edge_prop>, karma::_val))] << "></connect>"
        << (*(karma::eol<<"<constraint type="<<karma_ascii::string<<">"<<*(karma::double_<<" ")<<"</constraint>"))[karma::_1 = phx::bind(&details::getConstraints<Constraint3D>, karma::_val)];
};

template<typename System, typename iterator>
void parser_generator< typename details::getModule3D<System>::type::edge_prop , System, iterator >::init(generator& r) {

    r %= karma::lit("<type>Edge</type>")
         << karma::eol << "<value>" << karma::int_ << " "
         << karma::int_ << " " << karma::int_ << "</value>";
};

template<typename System, typename iterator>
void parser_generator<typename details::getModule3D<System>::type::fix_prop, System, iterator>::init(generator& r) {

    r = karma::lit("<type>Fix</type>\n<value>") << karma::bool_ <<"</value>";
};


/****************************************************************************************************/
/****************************************************************************************************/

template<typename System, typename iterator>
void parser_parser< typename details::getModule3D<System>::type::Geometry3D, System, iterator >::init(parser& r) {

    r = qi::lit("<type>Geometry3D</type>")[ qi::_val =  phx::construct<boost::shared_ptr<object_type> >(phx::new_<object_type>(*qi::_r1))]
        >> "<class>" >> (+qi::char_("a-zA-Z"))[qi::_a = phx::construct<std::string>(phx::begin(qi::_1), phx::end(qi::_1))] >> "</class>"
        >> "<value>" >> *qi::double_[ vector_in(qi::_b, qi::_c, qi::_1) ] >> "</value>"
        >> qi::eps[ create(qi::_r1, qi::_a, qi::_val, qi::_b) ];
};

template<typename System, typename iterator>
void parser_parser< typename details::getModule3D<System>::type::vertex_prop, System, iterator >::init(parser& r) {

    r %= qi::lit("<type>Vertex</type>") >> "<value>" >> qi::int_ >> "</value>";
};

template<typename System, typename iterator>
void parser_parser< typename details::getModule3D<System>::type::Constraint3D, System, iterator >::init(parser& r) {

    r = qi::lit("<type>Constraint3D</type>")
        >> ("<connect first=" >> qi::int_ >> "second=" >> qi::int_ >> "></connect>")[
            qi::_val =  phx::construct<boost::shared_ptr<Constraint3D> >(
                            phx::new_<Constraint3D>(*qi::_r1,
                                    phx::bind(&System::Cluster::template getObject<Geometry3D, GlobalVertex>, phx::bind(&System::m_cluster, qi::_r1), qi::_1),
                                    phx::bind(&System::Cluster::template getObject<Geometry3D, GlobalVertex>, phx::bind(&System::m_cluster, qi::_r1), qi::_2)))
        ]
        >> (*("<constraint type=" >> *qi_ascii::alpha >> ">" >> *qi::double_ >>"</constraint>"))[phx::bind(&details::setConstraints<Constraint3D>, qi::_1, qi::_val)];
};

template<typename System, typename iterator>
void parser_parser< typename details::getModule3D<System>::type::edge_prop, System, iterator >::init(parser& r) {

    r %= qi::lit("<type>Edge</type>")
         >> "<value>" >> qi::int_ >> qi::int_ >> qi::int_ >> "</value>";
};

template<typename System, typename iterator>
void parser_parser< typename details::getModule3D<System>::type::fix_prop, System, iterator >::init(parser& r) {

    r = qi::lit("<type>Fix</type>") >> "<value>" >> qi::bool_ >> "</value>";
};


}


#endif //DCM_MODULE3D_STATE_HPP
