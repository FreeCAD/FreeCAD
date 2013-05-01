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

namespace fusion = boost::fusion;

#include "kernel.hpp"

namespace dcm {

struct no_option {};

template<typename Kernel>
struct Pseudo {
    typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;
    void calculatePseudo(typename Kernel::Vector& param1, Vec& v1, typename Kernel::Vector& param2, Vec& v2) {};
};

template<typename Kernel>
struct Scale {
    void setScale(typename Kernel::number_type scale) {};
};

template<typename Kernel>
struct PseudoScale {
    typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;
    void calculatePseudo(typename Kernel::Vector& param1, Vec& v1, typename Kernel::Vector& param2, Vec& v2) {};
    void setScale(typename Kernel::number_type scale) {};
};

//type to allow a metaprogramming check for a Equation
struct EQ {};

template<typename Seq, typename T>
struct pushed_seq;

template<typename seq>
struct op_seq : public seq {
  
    template<typename T>
    typename boost::enable_if< boost::is_base_of< dcm::EQ, T>, typename pushed_seq<seq, T>::type >::type operator &(T val) {

        typedef typename pushed_seq<seq, T>::type Sequence;
        typedef typename fusion::result_of::begin<Sequence>::type Begin;
        typedef typename fusion::result_of::end<Sequence>::type End;
        typedef typename fusion::result_of::prior<End>::type EndOld;

        //create the new sequence
        Sequence vec;

        //copy the old values into the new sequence
        Begin b(vec);
        EndOld eo(vec);

        fusion::iterator_range<Begin, EndOld> range(b, eo);
        fusion::copy(*this, range);

        //insert this object at the end of the sequence
        fusion::back(vec) = val;

        //and return our new extendet sequence
        return vec;
    };
};

template<typename Seq, typename T>
struct pushed_seq {
    typedef typename boost::mpl::if_<boost::mpl::is_sequence<Seq>, Seq, fusion::vector1<Seq> >::type S;
    typedef typename fusion::result_of::as_vector< typename boost::mpl::push_back<S, T>::type >::type vec;
    typedef op_seq<vec> type;
};

template<typename Derived, typename Option>
struct Equation : public EQ {

    typedef Option option_type;
    option_type value;

    Equation(option_type val = option_type()) : value(val) {};

    Derived& operator()(const option_type val) {
        value = val;
        return *(static_cast<Derived*>(this));
    };
    Derived& operator=(const option_type val) {
        return operator()(val);
    };

    template<typename T>
    typename boost::enable_if< boost::is_base_of< dcm::EQ, T>, typename pushed_seq<T, Derived>::type >::type operator &(T val) {

        typename pushed_seq<T, Derived>::type vec;
        fusion::at_c<0>(vec) = val;
        fusion::at_c<1>(vec) = *(static_cast<Derived*>(this));
        return vec;
    };
};

struct Distance : public Equation<Distance, double> {

    using Equation::operator=;
    Distance() : Equation(0) {};

    template< typename Kernel, typename Tag1, typename Tag2 >
    struct type {

        typedef typename Kernel::number_type Scalar;
        typedef typename Kernel::VectorMap   Vector;
        typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

        Scalar value;
        //template definition
        void calculatePseudo(typename Kernel::Vector& param1, Vec& v1, typename Kernel::Vector& param2, Vec& v2) {
            assert(false);
        };
        void setScale(Scalar scale) {
            assert(false);
        };
        Scalar calculate(Vector& param1,  Vector& param2) {
            assert(false);
            return 0;
        };
        Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
            assert(false);
            return 0;
        };
        Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
            assert(false);
            return 0;
        };
        void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
            assert(false);
        };
        void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
            assert(false);
        };
    };
};

//the possible directions
enum Direction { equal, opposite, parallel, perpendicular };

struct Orientation : public Equation<Orientation, Direction> {

    using Equation::operator=;
    Orientation() : Equation(parallel) {};

    template< typename Kernel, typename Tag1, typename Tag2 >
    struct type : public PseudoScale<Kernel> {

        typedef typename Kernel::number_type Scalar;
        typedef typename Kernel::VectorMap   Vector;

        option_type value;

        //template definition
        Scalar calculate(Vector& param1,  Vector& param2) {
            assert(false);
            return 0;
        };
        Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
            assert(false);
            return 0;
        };
        Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
            assert(false);
            return 0;
        };
        void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
            assert(false);
        };
        void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
            assert(false);
        };
    };
};

struct Angle : public Equation<Angle, double> {

    using Equation::operator=;
    Angle() : Equation(0) {};

    template< typename Kernel, typename Tag1, typename Tag2 >
    struct type : public PseudoScale<Kernel> {

        typedef typename Kernel::number_type Scalar;
        typedef typename Kernel::VectorMap   Vector;

        option_type value;

        //template definition
        Scalar calculate(Vector& param1,  Vector& param2) {
            assert(false);
        };
        Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
            assert(false);
        };
        Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
            assert(false);
        };
        void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
            assert(false);
        };
        void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
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

