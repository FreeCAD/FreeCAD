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

#ifndef GCM_EQUATIONS_H
#define GCM_EQUATIONS_H

#include <assert.h>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
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

#include <boost/exception/exception.hpp>

namespace fusion = boost::fusion;
namespace mpl = boost::mpl;

#include "kernel.hpp"

namespace dcm {

//the possible directions
enum Direction { parallel, equal, opposite, perpendicular };

//the possible solution spaces
enum SolutionSpace {bidirectional, positiv_directional, negative_directional};

struct no_option {};

template<typename Kernel>
struct Pseudo {
    typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

    template <typename DerivedA,typename DerivedB>
    void calculatePseudo(const E::MatrixBase<DerivedA>& param1, Vec& v1, const E::MatrixBase<DerivedB>& param2, Vec& v2) {};
};

template<typename Kernel>
struct Scale {
    void setScale(typename Kernel::number_type scale) {};
};

template<typename Kernel>
struct PseudoScale {
    typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

    template <typename DerivedA,typename DerivedB>
    void calculatePseudo(const E::MatrixBase<DerivedA>& param1, Vec& v1, const E::MatrixBase<DerivedB>& param2, Vec& v2) {};
    void setScale(typename Kernel::number_type scale) {};
};

//type to allow a metaprogramming check for a Equation
struct EQ {};

template<typename Seq, typename T>
struct pushed_seq;

//metafunctions to retrieve the options of an equation
template<typename T>
struct options {
    typedef typename T::options type;
};
template<>
struct options< mpl::arg<-1> > {
    typedef fusion::map<> type;
};
template<typename Eq, typename Opt>
struct has_option : public mpl::if_<mpl::has_key<typename options<Eq>::type, Opt>, boost::true_type, boost::false_type>::type {
    typedef typename mpl::has_key<typename options<Eq>::type, Opt>::type type;
};
template<typename cs, typename T>
struct seq_has_option {
    typedef typename mpl::transform<cs, has_option<mpl::_1, T> >::type bool_seq;
    typedef typename mpl::not_<boost::is_same<typename mpl::find<bool_seq, mpl::true_>::type, mpl::end<bool_seq> > >::type type;
};

template<typename seq>
struct constraint_sequence : public seq {

    template<typename T>
    void pretty(T type) {
        std::cout<<"pretty: "<<__PRETTY_FUNCTION__<<std::endl;
    };

    //an equation gets added to this sequence
    template<typename T>
    typename boost::enable_if< boost::is_base_of< dcm::EQ, T>, typename pushed_seq<seq, T>::type >::type operator &(T& val) {

        typedef typename pushed_seq<seq, T>::type Sequence;
        typedef typename fusion::result_of::begin<Sequence>::type Begin;
        typedef typename fusion::result_of::find<Sequence, typename fusion::result_of::back<typename pushed_seq<seq, T>::S1>::type >::type EndOld;


        //create the new sequence
        Sequence vec;

        //copy the old values into the new sequence
        Begin b(vec);
        EndOld eo(vec);

        fusion::iterator_range<Begin, EndOld> range(b, eo);
        fusion::copy(*this, range);

        //insert this object at the end of the sequence
        *fusion::find<T>(vec) = val;

        //and return our new extendet sequence
        return vec;
    };

    //an sequence gets added to this sequence (happens only if sequenced equations like coincident are used)
    template<typename T>
    typename boost::enable_if< mpl::is_sequence<T>, typename pushed_seq<T, seq>::type >::type operator &(T& val) {

        typedef typename pushed_seq<T, seq>::type Sequence;
        typedef typename fusion::result_of::begin<Sequence>::type Begin;
        typedef typename fusion::result_of::find<Sequence, typename fusion::result_of::back<typename pushed_seq<T, seq>::S1>::type >::type EndF;

        //create the new sequence
        Sequence vec;

        Begin b(vec);
        EndF ef(vec);

        fusion::iterator_range<Begin, EndF> range(b, ef);
        fusion::copy(val, range);

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

    //we also allow to set values directly into the equation, as this makes it more compftable for multi constraints
    //as align. Note that this only works if all option types of all equations in this sequence are distinguishable
    template<typename T>
    typename boost::enable_if<typename seq_has_option<seq, T>::type, constraint_sequence<seq>&>::type
    operator=(const T& val) {

        fusion::filter_view<constraint_sequence, has_option<mpl::_, T > > view(*this);
        fusion::front(view) = val;
        return *this;
    };
};

template<typename Seq, typename T>
struct pushed_seq {
    typedef typename mpl::if_<mpl::is_sequence<Seq>, Seq, fusion::vector1<Seq> >::type S1;
    typedef typename mpl::if_<mpl::is_sequence<T>, T, fusion::vector1<T> >::type S2;

    typedef typename mpl::fold< S2, S1, mpl::if_< boost::is_same<
    mpl::find<mpl::_1, mpl::_2>, mpl::end<mpl::_1> >, mpl::push_back<mpl::_1,mpl::_2>, mpl::_1> >::type unique_vector;

    typedef typename fusion::result_of::as_vector< unique_vector >::type vec;
    typedef constraint_sequence<vec> type;
};

template<typename Derived, typename Option, bool rotation_only = false>
struct Equation : public EQ {

    typedef typename mpl::if_<mpl::is_sequence<Option>, Option, mpl::vector<Option> >::type option_sequence;
    typedef typename mpl::fold<option_sequence, fusion::map<>, fusion::result_of::push_back<mpl::_1, fusion::pair<mpl::_2, std::pair<bool, mpl::_2> > > > ::type option_set_map;
    typedef typename fusion::result_of::as_map<option_set_map>::type options;

    options values;
    bool pure_rotation;

    struct option_copy {

        options* values;
        option_copy(options& op) : values(&op) {};

        template<typename T>
        void operator()(const T& val) const {
            if(val.second.first)
                fusion::at_key<typename T::first_type>(*values) = val.second;
        };
    };

    Equation() : pure_rotation(rotation_only) {};

    //assign option
    template<typename T>
    typename boost::enable_if<fusion::result_of::has_key<options, T>, Derived&>::type operator()(const T& val) {
        fusion::at_key<T>(values).second = val;
        fusion::at_key<T>(values).first  = true;
        return *(static_cast<Derived*>(this));
    };
    //assign option
    template<typename T>
    typename boost::enable_if<fusion::result_of::has_key<options, T>, Derived&>::type operator=(const T& val) {
        return operator()(val);
    };
    //assign complete equation
    Derived& operator=(const Derived& eq) {

        //we only copy the values which were set and are therefore valid
        option_copy oc(values);
        fusion::for_each(eq.values, oc);

        //the assigned eqution can be set back to default for convinience in further usage
        const_cast<Derived*>(&eq)->setDefault();

        return *static_cast<Derived*>(this);
    };
    /*
        Derived& operator=(const Derived& eq) {
    	option_copy oc(values);
            fusion::for_each(eq.values, oc);
    	return *static_cast<Derived*>(this);
        };*/

    //an equation gets added to this equation
    template<typename T>
    typename boost::enable_if< boost::is_base_of< dcm::EQ, T>, typename pushed_seq<T, Derived>::type >::type operator &(T& val) {

        typename pushed_seq<T, Derived>::type vec;
        *fusion::find<T>(vec) = val;
        *fusion::find<Derived>(vec) = *(static_cast<Derived*>(this));
        return vec;
    };

    //an sequence gets added to this equation (happens only if sequenced equations like coincident are used)
    template<typename T>
    typename boost::enable_if< mpl::is_sequence<T>, typename pushed_seq<T, Derived>::type >::type operator &(T& val) {

        typedef typename pushed_seq<T, Derived>::type Sequence;
        typedef typename fusion::result_of::begin<Sequence>::type Begin;
        typedef typename fusion::result_of::find<Sequence, typename fusion::result_of::back<typename pushed_seq<T, Derived>::S1>::type >::type EndOld;

        //create the new sequence
        Sequence vec;

        //copy the old values into the new sequence
        Begin b(vec);
        EndOld eo(vec);

        fusion::iterator_range<Begin, EndOld> range(b, eo);
        fusion::copy(val, range);

        //insert this object at the end of the sequence
        *fusion::find<Derived>(vec) = *static_cast<Derived*>(this);

        //and return our new extendet sequence
        return vec;
    };

    //set default option values, neeeded for repedability and to prevent unexpected behaviour
    virtual void setDefault() = 0;
};

//convinience stream functions for debugging
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
operator << (std::basic_ostream<charT,traits> & stream, const Eq& equation)
{
    print_pair<charT, traits> pr;
    pr.stream = &stream;
    stream << typeid(equation).name() << ": ";
    fusion::for_each(equation.values, pr);
    return stream;
}

struct Distance : public Equation<Distance, mpl::vector2<double, SolutionSpace> > {

    using Equation::operator=;
    using Equation::options;
    Distance() : Equation() {
        setDefault();
    };

    //override needed ass assignmend operator is always created by the compiler
    //and we need to ensure that our custom one is used
    Distance& operator=(const Distance& d) {
        return Equation::operator=(d);
    };

    void setDefault() {
        fusion::at_key<double>(values) = std::make_pair(false, 0.);
        fusion::at_key<SolutionSpace>(values) = std::make_pair(false, bidirectional);
    };

    template< typename Kernel, typename Tag1, typename Tag2 >
    struct type {

        type() {
            throw constraint_error() << boost::errinfo_errno(100) << error_message("unsupported geometry in distance constraint")
                                     << error_type_first_geometry(typeid(Tag1).name()) << error_type_second_geometry(typeid(Tag2).name());
        };

        typedef typename Kernel::number_type Scalar;
        typedef typename Kernel::VectorMap   Vector;
        typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

        options values;
        //template definition
        template <typename DerivedA,typename DerivedB>
        void calculatePseudo(const E::MatrixBase<DerivedA>& param1, Vec& v1, const E::MatrixBase<DerivedB>& param2, Vec& v2) {
            assert(false);
        };
        void setScale(Scalar scale) {
            assert(false);
        };
        template <typename DerivedA,typename DerivedB>
        Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
            assert(false);
            return 0;
        };
        template <typename DerivedA,typename DerivedB, typename DerivedC>
        Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& param1,
                                      const E::MatrixBase<DerivedB>& param2,
                                      const E::MatrixBase<DerivedC>& dparam1) {
            assert(false);
            return 0;
        };
        template <typename DerivedA,typename DerivedB, typename DerivedC>
        Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& param1,
                                       const E::MatrixBase<DerivedB>& param2,
                                       const E::MatrixBase<DerivedC>& dparam2) {
            assert(false);
            return 0;
        };
        template <typename DerivedA,typename DerivedB, typename DerivedC>
        void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& param1,
                                            const E::MatrixBase<DerivedB>& param2,
                                            E::MatrixBase<DerivedC>& gradient) {
            assert(false);
        };
        template <typename DerivedA,typename DerivedB, typename DerivedC>
        void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                             const E::MatrixBase<DerivedB>& param2,
                                             E::MatrixBase<DerivedC>& gradient) {
            assert(false);
        };
    };
};

struct Orientation : public Equation<Orientation, Direction, true> {

    using Equation::operator=;
    using Equation::options;
    Orientation() : Equation() {
        setDefault();
    };

    //override needed ass assignmend operator is always created by the compiler
    //and we need to ensure that our custom one is used
    Orientation& operator=(const Orientation& d) {
        return Equation::operator=(d);
    };

    void setDefault() {
        fusion::at_key<Direction>(values) = std::make_pair(false, parallel);
    };

    template< typename Kernel, typename Tag1, typename Tag2 >
    struct type : public PseudoScale<Kernel> {

        type() {
            throw constraint_error() << boost::errinfo_errno(101) << error_message("unsupported geometry in orientation constraint")
                                     << error_type_first_geometry(typeid(Tag1).name()) << error_type_second_geometry(typeid(Tag2).name());
        };

        typedef typename Kernel::number_type Scalar;
        typedef typename Kernel::VectorMap   Vector;

        options values;

        //template definition
        template <typename DerivedA,typename DerivedB>
        Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
            assert(false);
            return 0;
        };
        template <typename DerivedA,typename DerivedB, typename DerivedC>
        Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& param1,
                                      const E::MatrixBase<DerivedB>& param2,
                                      const E::MatrixBase<DerivedC>& dparam1) {
            assert(false);
            return 0;
        };
        template <typename DerivedA,typename DerivedB, typename DerivedC>
        Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& param1,
                                       const E::MatrixBase<DerivedB>& param2,
                                       const E::MatrixBase<DerivedC>& dparam2) {
            assert(false);
            return 0;
        };
        template <typename DerivedA,typename DerivedB, typename DerivedC>
        void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& param1,
                                            const E::MatrixBase<DerivedB>& param2,
                                            E::MatrixBase<DerivedC>& gradient) {
            assert(false);
        };
        template <typename DerivedA,typename DerivedB, typename DerivedC>
        void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                             const E::MatrixBase<DerivedB>& param2,
                                             E::MatrixBase<DerivedC>& gradient) {
            assert(false);
        };
    };
};

struct Angle : public Equation<Angle, mpl::vector2<double, SolutionSpace>, true> {

    using Equation::operator=;
    Angle() : Equation() {
        setDefault();
    };

    //override needed ass assignmend operator is always created by the compiler
    //and we need to ensure that our custom one is used
    Angle& operator=(const Angle& d) {
        return Equation::operator=(d);
    };

    void setDefault() {
        fusion::at_key<double>(values) = std::make_pair(false, 0.);
        fusion::at_key<SolutionSpace>(values) = std::make_pair(false, bidirectional);
    };

    template< typename Kernel, typename Tag1, typename Tag2 >
    struct type : public PseudoScale<Kernel> {

        type() {
            throw constraint_error() << boost::errinfo_errno(102) << error_message("unsupported geometry in angle constraint")
                                     << error_type_first_geometry(typeid(Tag1).name()) << error_type_second_geometry(typeid(Tag2).name());
        };

        typedef typename Kernel::number_type Scalar;
        typedef typename Kernel::VectorMap   Vector;

        options values;

        //template definition
        template <typename DerivedA,typename DerivedB>
        Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
            assert(false);
            return 0;
        };
        template <typename DerivedA,typename DerivedB, typename DerivedC>
        Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& param1,
                                      const E::MatrixBase<DerivedB>& param2,
                                      const E::MatrixBase<DerivedC>& dparam1) {
            assert(false);
            return 0;
        };
        template <typename DerivedA,typename DerivedB, typename DerivedC>
        Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& param1,
                                       const E::MatrixBase<DerivedB>& param2,
                                       const E::MatrixBase<DerivedC>& dparam2) {
            assert(false);
            return 0;
        };
        template <typename DerivedA,typename DerivedB, typename DerivedC>
        void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& param1,
                                            const E::MatrixBase<DerivedB>& param2,
                                            E::MatrixBase<DerivedC>& gradient) {
            assert(false);
        };
        template <typename DerivedA,typename DerivedB, typename DerivedC>
        void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                             const E::MatrixBase<DerivedB>& param2,
                                             E::MatrixBase<DerivedC>& gradient) {
            assert(false);
        };
    };
};

//static is needed to restrain the scope of the objects to the current compilation unit. Without it
//every compiled file including this header would define these as global and the linker would find
//multiple definitions of the same objects
static Distance distance;
static Orientation orientation;
static Angle    angle;

};

#endif //GCM_EQUATIONS_H


