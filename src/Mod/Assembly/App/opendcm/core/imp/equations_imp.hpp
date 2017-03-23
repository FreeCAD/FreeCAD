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
    GNU Lesser General Public License for more detemplate tails.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef DCM_EQUATIONS_IMP_H
#define DCM_EQUATIONS_IMP_H

#include <assert.h>

#include "../equations.hpp"

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/sort.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/iterator_range.hpp>
#include <boost/fusion/include/copy.hpp>
#include <boost/fusion/include/advance.hpp>
#include <boost/fusion/include/back.hpp>
#include <boost/fusion/include/iterator_range.hpp>
#include <boost/fusion/include/nview.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/as_map.hpp>
#include <boost/fusion/include/filter_view.hpp>
#include <boost/fusion/include/size.hpp>

#include <boost/exception/exception.hpp>

namespace fusion = boost::fusion;
namespace mpl = boost::mpl;

namespace dcm {

template<typename Seq, typename T>
struct pushed_seq;

/*
template<typename seq>
template<typename T>
typename boost::enable_if< boost::is_base_of< dcm::EQ, T>, typename pushed_seq<seq, T>::type >::type
constraint_sequence<seq>::operator &(T& val) {

    typedef typename pushed_seq<seq, T>::type Sequence;

    //create the new sequence
    Sequence vec;

    //get a index vector for this sequence
    typedef typename mpl::transform<typename pushed_seq<seq, T>::S1,
            fusion::result_of::distance<typename fusion::result_of::begin<Sequence>::type,
            fusion::result_of::find<Sequence, mpl::_1> > >::type position_vector_added;

    //and copy the types in
    fusion::nview<Sequence, position_vector_added> view_added(vec);
    fusion::copy(*this, view_added);

    //insert this object at the end of the sequence
    *fusion::find<T>(vec) = val;

    //and return our new extendet sequence
    return vec;
};

template<typename seq>
template<typename T>
typename boost::enable_if< mpl::is_sequence<T>, typename pushed_seq<T, seq>::type >::type
constraint_sequence<seq>::operator &(T& val) {

    typedef typename pushed_seq<T, seq>::type Sequence;

    //create the new sequence
    Sequence vec;

    //get a index vector for the added sequence
    typedef typename mpl::transform<typename pushed_seq<T, seq>::S1,
            fusion::result_of::distance<typename fusion::result_of::begin<Sequence>::type,
            fusion::result_of::find<Sequence, mpl::_1> > >::type position_vector_added;

    //and copy the types in
    fusion::nview<Sequence, position_vector_added> view_added(vec);
    fusion::copy(val, view_added);

    //to copy the types of the second sequence is not as easy as before. If types were already present in
    //the original sequence they are not added again. therefore we need to find all types of the second sequence
    //in the new one and assign the objects to this positions.

    //get a index vector for all second-sequence-elements
    typedef typename mpl::transform<typename pushed_seq<T, seq>::S2,
            fusion::result_of::distance<typename fusion::result_of::begin<Sequence>::type,
            fusion::result_of::find<Sequence, mpl::_1> > >::type position_vector;

    //and copy the types in
    fusion::nview<Sequence, position_vector> view(vec);
    fusion::copy(*this, view);

    //and return our new extendet sequence
    return vec;
};
*/
template<typename options>
struct option_copy {

    options* values;
    option_copy(options& op) : values(&op) {};

    template<typename T>
    void operator()(const T& val) const {
        if(val.second.first)
            fusion::at_key<typename T::first_type>(*values) = val.second;
    };
};

template<typename Derived, typename Option, int id, AccessType a >
Derived& Equation<Derived, Option, id, a>::assign(const Derived& eq) {

    //we only copy the values which were set and are therefore valid
    option_copy<options> oc(values);
    fusion::for_each(eq.values, oc);

    //the assigned eqution can be set back to default for convenience in further usage
    const_cast<Derived*>(&eq)->setDefault();

    return *static_cast<Derived*>(this);
};

/*
template<typename Derived, typename Option, int id, AccessType a >
template<typename T>
typename boost::enable_if< boost::is_base_of< dcm::EQ, T>, typename pushed_seq<T, Derived>::type >::type
Equation<Derived, Option, id, a>::operator &(T& val) {

    typename pushed_seq<T, Derived>::type vec;
    *fusion::find<T>(vec) = val;
    *fusion::find<Derived>(vec) = *(static_cast<Derived*>(this));
    return vec;
};

template<typename Derived, typename Option, int id, AccessType a >
template<typename T>
typename boost::enable_if< mpl::is_sequence<T>, typename pushed_seq<T, Derived>::type >::type
Equation<Derived, Option, id, a>::operator &(T& val) {

    typedef typename pushed_seq<T, Derived>::type Sequence;

    //create the new sequence
    Sequence vec;

    //get a index vector for the added sequence
    typedef typename mpl::transform<typename pushed_seq<T, Derived>::S1,
            fusion::result_of::distance<typename fusion::result_of::begin<Sequence>::type,
            fusion::result_of::find<Sequence, mpl::_1> > >::type position_vector;

    //and copy the types in
    fusion::nview<Sequence, position_vector> view(vec);
    fusion::copy(val, view);

    //insert this object into the sequence
    *fusion::find<Derived>(vec) = *static_cast<Derived*>(this);

    //and return our new extendet sequence
    return vec;
};
*/

//convenience stream functions for debugging
template <typename charT, typename traits>
struct print_pair {
    std::basic_ostream<charT,traits>* stream;

    template<typename T>
    void operator()(const T& t) const {
        *stream << "("<<t.second.first << ", "<<t.second.second<<") ";
    };
};

template <typename charT, typename traits, typename Eq>
typename boost::enable_if<boost::is_base_of<EQ, Eq>, std::basic_ostream<charT,traits>&>::type
operator << (std::basic_ostream<charT,traits>& stream, const Eq& equation)
{
    print_pair<charT, traits> pr;
    pr.stream = &stream;
    stream << typeid(equation).name() << ": ";
    fusion::for_each(equation.values, pr);
    return stream;
}

/*
Distance::Distance() : Equation() {
    setDefault();
};

Distance& Distance::operator=(const Distance& d) {
    return Equation::assign(d);
};

void Distance::setDefault() {};



Orientation::Orientation() : Equation() {
    setDefault();
};

Orientation& Orientation::operator=(const Orientation& d) {
  return Equation::assign(d);
};

void Orientation::setDefault() {
    fusion::at_key<Direction>(values) = std::make_pair(false, parallel);
};

Angle::Angle() : Equation() {
    setDefault();
};

Angle& Angle::operator=(const Angle& d) {
    return Equation::assign(d);
};

void Angle::setDefault() {
    fusion::at_key<double>(values) = std::make_pair(false, 0.);
    fusion::at_key<SolutionSpace>(values) = std::make_pair(false, bidirectional);
};
*/

};

#endif //GCM_EQUATIONS_H


