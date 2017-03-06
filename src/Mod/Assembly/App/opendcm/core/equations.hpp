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

#ifndef DCM_EQUATIONS_H
#define DCM_EQUATIONS_H

#include <assert.h>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/not.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/as_map.hpp>
#include <boost/fusion/include/at_key.hpp>
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

//template<typename Seq, typename T>
//struct pushed_seq;

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

template<typename seq, typename Derived>
struct constraint_sequence : public seq {

    template<typename T>
    void pretty(T type) {
        std::cout<<"pretty: "<<__PRETTY_FUNCTION__<<std::endl;
    };

    //don't allow expression equation stacking: the compile time impact is huge if we want to allow
    //text parsing
    /*
        //an equation gets added to this sequence
        template<typename T>
        typename boost::enable_if< boost::is_base_of< dcm::EQ, T>, typename pushed_seq<seq, T>::type >::type operator &(T& val);

        //an sequence gets added to this sequence (happens only if sequenced equations like coincident are used)
        template<typename T>
        typename boost::enable_if< mpl::is_sequence<T>, typename pushed_seq<T, seq>::type >::type operator &(T& val);
    */
    //we also allow to set values directly into the equation, as this makes it more compftable for multi constraints
    //as align. Note that this only works if all option types of all equations in this sequence are distinguishable
    template<typename T>
    typename boost::enable_if<typename seq_has_option<seq, T>::type, Derived&>::type
    operator=(const T& val) {

        fusion::filter_view<constraint_sequence, has_option<mpl::_, T > > view(*this);
        fusion::front(view) = val;
        return *((Derived*)this);
    };

};

/*
template<typename T>
struct get_equation_id {
    typedef typename T::ID type;
};

template<typename Seq, typename T>
struct pushed_seq {
    typedef typename mpl::if_<mpl::is_sequence<Seq>, Seq, fusion::vector1<Seq> >::type S1;
    typedef typename mpl::if_<mpl::is_sequence<T>, T, fusion::vector1<T> >::type S2;

    typedef typename mpl::fold< S2, S1, mpl::if_< boost::is_same<
    mpl::find<mpl::_1, mpl::_2>, mpl::end<mpl::_1> >, mpl::push_back<mpl::_1,mpl::_2>, mpl::_1> >::type unique_vector;
    typedef typename mpl::sort<unique_vector, mpl::less< get_equation_id<mpl::_1>, get_equation_id<mpl::_2> > >::type sorted_vector;

    typedef typename fusion::result_of::as_vector< sorted_vector >::type vec;
    typedef constraint_sequence<vec> type;
};*/

template<typename Derived, typename Option, int id, AccessType a = general>
struct Equation : public EQ {

    typedef typename mpl::if_<mpl::is_sequence<Option>, Option, mpl::vector<Option> >::type option_sequence;
    typedef typename mpl::fold<option_sequence, fusion::map<>, fusion::result_of::push_back<mpl::_1, fusion::pair<mpl::_2, std::pair<bool, mpl::_2> > > > ::type option_set_map;
    typedef typename fusion::result_of::as_map<option_set_map>::type options;

    options values;
    AccessType access;

    typedef mpl::int_<id> ID;

    Equation() : access(a) {};

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

    //assign complete equation (we need to override the operator= in the derived class anyway as it
    //is automatically created by the compiler, so we use a different name here to avoid duplicate
    //operator= warning on msvc)
    Derived& assign(const Derived& eq);

    //don't allow expression equation stacking: the compile time impact is huge if we want to allow
    //text parsing
    /*
    //an equation gets added to this equation
    template<typename T>
    typename boost::enable_if< boost::is_base_of< dcm::EQ, T>, typename pushed_seq<T, Derived>::type >::type operator &(T& val);

    //an sequence gets added to this equation (happens only if sequenced equations like coincident are used)
    template<typename T>
    typename boost::enable_if< mpl::is_sequence<T>, typename pushed_seq<T, Derived>::type >::type operator &(T& val);
    */

    //set default option values, neeeded for repedability and to prevent unexpected behaviour
    virtual void setDefault() {};
};

template <typename charT, typename traits, typename Eq>
typename boost::enable_if<boost::is_base_of<EQ, Eq>, std::basic_ostream<charT,traits>&>::type
operator << (std::basic_ostream<charT,traits>& stream, const Eq& equation);

struct Distance : public Equation<Distance, mpl::vector2<double, SolutionSpace>, 1 > {

    using Equation::operator=;
    using Equation::options;
    Distance() {
        setDefault();
    };

    //override needed ass assignmend operator is always created by the compiler
    //and we need to ensure that our custom one is used
    Distance& operator=(const Distance& d) {
        return Equation::assign(d);
    };

    void setDefault() {};

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

struct Orientation : public Equation<Orientation, Direction, 2, rotation> {

    using Equation::operator=;
    using Equation::options;
    Orientation() {
        setDefault();
    };

    //override needed ass assignmend operator is always created by the compiler
    //and we need to ensure that our custom one is used
    Orientation& operator=(const Orientation& d) {
        return Equation::assign(d);
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

struct Angle : public Equation<Angle, mpl::vector2<double, SolutionSpace>, 3, rotation> {

    using Equation::operator=;

    Angle() {
        setDefault();
    };

    //override needed ass assignmend operator is always created by the compiler
    //and we need to ensure that our custom one is used
    Angle& operator=(const Angle& d) {
        return Equation::assign(d);
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

#ifndef DCM_EXTERNAL_CORE
#include "imp/equations_imp.hpp"
#endif

#endif //GCM_EQUATIONS_H


