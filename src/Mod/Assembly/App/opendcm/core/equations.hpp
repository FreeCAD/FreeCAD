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

struct Distance {

    typedef double option_type;
    option_type value;

    Distance() : value(0) {};

    Distance& operator=(const option_type val) {
        value = val;
        return *this;
    };

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
enum Direction { Same, Opposite, Both };

struct Parallel {

    typedef Direction option_type;
    option_type value;

    Parallel() : value(Both) {};

    Parallel& operator=(const option_type val) {
        value = val;
        return *this;
    };

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

struct Angle {

    typedef double option_type;
    option_type value;

    Angle() : value(0) {};

    Angle& operator=(const option_type val) {
        value = val;
        return *this;
    };

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
static Parallel parallel;
static Angle    angle;

};

#endif //GCM_EQUATIONS_H

