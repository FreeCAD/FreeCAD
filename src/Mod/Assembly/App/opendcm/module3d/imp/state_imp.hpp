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

#ifndef DCM_MODULE3D_STATE_IMP_HPP
#define DCM_MODULE3D_STATE_IMP_HPP

#include "../state.hpp"

#include "../module.hpp"
#include "../coincident.hpp"
#include "../alignment.hpp"

#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix/function/adapt_function.hpp>
#include <boost/phoenix/bind.hpp>
#include <boost/fusion/include/adapt_adt.hpp>
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

struct geom_visitor : public boost::static_visitor<std::string> {

    template<typename T>
    std::string operator()(T& i) const {

        //we use stings in case new geometry gets added and the weights shift, meaning: backwards
        //compatible
        std::string type;

        switch(geometry_traits<T>::tag::weight::value) {
        case tag::weight::direction::value :
            return "direction";

        case tag::weight::point::value :
            return "point";

        case tag::weight::line::value :
            return "line";

        case tag::weight::plane::value :
            return "plane";

        case tag::weight::cylinder::value :
            return "cylinder";

        default:
            return "unknown";
        };
    };
};

template<typename T>
std::string getWeight(boost::shared_ptr<T> ptr) {
    geom_visitor v;
    return ptr->apply(v);
};

template<typename T>
struct get_weight {
    typedef typename geometry_traits<T>::tag::weight type;
};

//search the first type in the typevector with the given weight
template<typename Vector, typename Weight>
struct getWeightType {
    typedef typename mpl::find_if<Vector, boost::is_same<get_weight<mpl::_1>, Weight > >::type iter;
    typedef typename mpl::if_< boost::is_same<iter, typename mpl::end<Vector>::type >, mpl::void_, typename mpl::deref<iter>::type>::type type;
};

typedef std::vector< fusion::vector2<std::string, std::vector<double> > > string_vec;
typedef std::vector< fusion::vector2<std::vector<char>, std::vector<double> > > char_vec;

template<typename C>
string_vec getConstraints(boost::shared_ptr<C> con) {

    string_vec vec;
    std::vector<boost::any> cvec = con->getGenericConstraints();

    boost::any al_o, d;
    typename std::vector<boost::any>::iterator it;

    if(cvec.size()==1) {
        it = cvec.begin();

        if((*it).type() == typeid(dcm::Distance)) {
            double v = fusion::at_key<double>(boost::any_cast<dcm::Distance>(*it).values).second;
            SolutionSpace s = fusion::at_key<SolutionSpace>(boost::any_cast<dcm::Distance>(*it).values).second;
            std::vector<double> dvec;
            dvec.push_back(v);
            dvec.push_back(s);
            vec.push_back(fusion::make_vector(std::string("Distance"), dvec));
        }
        else if((*it).type() == typeid(dcm::Angle)) {
            double v = fusion::at_key<double>(boost::any_cast<dcm::Angle>(*it).values).second;
            std::vector<double> value;
            value.push_back(v);
            vec.push_back(fusion::make_vector(std::string("Angle"), value));
        }
        else if((*it).type() == typeid(dcm::Orientation)) {
            int v = fusion::at_key<dcm::Direction>(boost::any_cast<dcm::Orientation>(*it).values).second;
            std::vector<double> value;
            value.push_back(v);
            vec.push_back(fusion::make_vector(std::string("Orientation"), value));
        }
    }
    else {
        for(it=cvec.begin(); it!=cvec.end(); it++) {
            if((*it).type() == typeid(dcm::details::ci_orientation)) {
                int v = fusion::at_key<dcm::Direction>(boost::any_cast<dcm::details::ci_orientation>(*it).values).second;
                std::vector<double> value;
                value.push_back(v);
                vec.push_back(fusion::make_vector(std::string("Coincidence"), value));
            }
            else if((*it).type() == typeid(dcm::details::al_orientation)) {
                int o = fusion::at_key<dcm::Direction>(boost::any_cast<dcm::details::al_orientation>(*it).values).second;

                double v;
                SolutionSpace s;

                if(it==cvec.begin()) {
                    v = fusion::at_key<double>(boost::any_cast<dcm::Distance>(cvec.back()).values).second;
                    s = fusion::at_key<SolutionSpace>(boost::any_cast<dcm::Distance>(cvec.back()).values).second;
                }
                else {
                    v = fusion::at_key<double>(boost::any_cast<dcm::Distance>(cvec.front()).values).second;
                    s = fusion::at_key<SolutionSpace>(boost::any_cast<dcm::Distance>(cvec.front()).values).second;
                }

                std::vector<double> value;
                value.push_back(o);
                value.push_back(v);
                value.push_back(s);
                vec.push_back(fusion::make_vector(std::string("Alignment"), value));
            };
        }
    }

    return vec;
};

template<typename C>
void constraintCreation(typename char_vec::iterator it,
                        typename char_vec::iterator end,
                        boost::shared_ptr<C> con) {

    std::string first(fusion::at_c<0>(*it).begin(), fusion::at_c<0>(*it).end());
    std::vector<double> second = fusion::at_c<1>(*it);

    if(first.compare("Distance") == 0) {
        typename details::fusion_vec<dcm::Distance>::type val;
        (fusion::front(val)=second[0])= (SolutionSpace)second[1];
        con->template initialize(val);
        return;
    }
    else if(first.compare("Orientation") == 0) {
        typename details::fusion_vec<dcm::Orientation>::type val;
        fusion::front(val)= (dcm::Direction)second[0];
        con->template initialize(val);
        return;
    }
    else if(first.compare("Angle") == 0) {
        typename details::fusion_vec<dcm::Angle>::type val;
        fusion::front(val)=second[0];
        con->template initialize(val);
        return;
    }
    else if(first.compare("Coincidence") == 0) {
        typename details::fusion_vec<dcm::Coincidence>::type val;
        val= (dcm::Direction)second[0];
        con->template initialize(val);
        return;
    }
    else if(first.compare("Alignment") == 0) {
        typename details::fusion_vec<dcm::Alignment>::type val;
        ((val=(dcm::Direction)second[0])=second[1])=(dcm::SolutionSpace)second[2];
        con->template initialize(val);
        return;
    }
};

template<typename C>
void setConstraints(char_vec& vec, boost::shared_ptr<C> con) {
    constraintCreation<C>(vec.begin(), vec.end(), con);
};

template <typename Geom, typename Row, typename Value>
bool VectorOutput(Geom& v, Row& r, Value& val) {

    if(r < v->m_global.rows()) {

        val = v->m_global(r++);
        return true; // output continues
    }

    return false;    // fail the output
};

template <typename Geom, typename Row, typename Value>
bool VectorInput(Geom& v, Row& r, Value& val) {

    v.conservativeResize(r+1);
    v(r++) = val;
    return true; // output continues
};

template <typename Translation, typename Row, typename Value>
bool TranslationOutput(Translation& v, Row& r, Value& val) {

    if(r < 3) {

        val = v.getTranslation().vector()(r++);
        return true; // output continues
    }

    return false;    // fail the output
};

template <typename CM>
bool TranslationInput(CM& t, std::vector<double> const& val) {

    t.setTranslation(typename CM::Kernel::Transform3D::Translation(val[0],val[1],val[2]));
    return true; // output continues
};

template <typename CM, typename Row, typename Value>
bool RotationOutput(CM& v, Row& r, Value& val) {

    if(r < 3) {

        val = v.getRotation().vec()(r++);
        return true; // output continues
    }
    else if( r < 4 ) {
      val = v.getRotation().w();
      return true;
    }
    
    return false;    // fail the output
};

template <typename CM>
bool RotationInput(CM& t, std::vector<double> const& val) {

    t.setRotation(typename CM::Kernel::Transform3D::Rotation(val[3], val[0], val[1], val[2]));
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
//specialization if no type in the typelist has the right weight
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
BOOST_PHOENIX_ADAPT_FUNCTION(bool, translation_out, dcm::details::TranslationOutput, 3)
BOOST_PHOENIX_ADAPT_FUNCTION(bool, rotation_out, dcm::details::RotationOutput, 3)
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
/*
template<typename System, typename iterator>
void parser_generator< typename details::getModule3D<System>::type::math_prop , System, iterator >::init(generator& r) {

    r = karma::lit("<type>Math3D</type>")
        << karma::eol << "<Translation>" << (details::scientific[ boost::spirit::_pass = translation_out(karma::_val, karma::_a, karma::_1) ] % ' ') << "</Translation>"
	<< karma::eol << karma::eps[karma::_a = 0] << "<Rotation>" << (details::scientific[ boost::spirit::_pass = rotation_out(karma::_val, karma::_a, karma::_1) ] % ' ') << "</Rotation>";
};*/

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
/*
template<typename System, typename iterator>
void parser_parser< typename details::getModule3D<System>::type::math_prop, System, iterator >::init(parser& r) {

    //r = qi::lit("<type>Math3D</type>") >> "<Translation>" >> (*qi::double_)[ phx::bind(&details::TranslationInput<details::ClusterMath<System> >, qi::_val, qi::_1) ] >> "</Translation>"
	//  >> "<Rotation>" >> (*qi::double_)[ phx::bind(&details::RotationInput<details::ClusterMath<System> >,qi::_val, qi::_1) ] >> "</Rotation>";
};*/

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
