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
    GNU Lesser General Public License for more detemplate tails.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef DCM_COINCIDENT_HPP
#define DCM_COINCIDENT_HPP

#include <opendcm/core/equations.hpp>
#include "distance.hpp"

namespace dcm {

namespace details {

//we need a custom orientation type to allow coincidents with points
struct ci_orientation : public Equation<ci_orientation, Direction, true> {

    using Equation::operator=;
    ci_orientation() : Equation(parallel) {};


    template< typename Kernel, typename Tag1, typename Tag2 >
    struct type : public PseudoScale<Kernel> {

        type() {
            throw constraint_error() <<  boost::errinfo_errno(103) << error_message("unsupported geometry in coincidence orientation constraint")
                                     << error_type_first_geometry(typeid(Tag1).name()) << error_type_second_geometry(typeid(Tag2).name());
        };

        typedef typename Kernel::number_type Scalar;
        typedef typename Kernel::VectorMap   Vector;
        typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

        option_type value;
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

template< typename Kernel >
struct ci_orientation::type< Kernel, tag::point3D, tag::point3D > : public dcm::PseudoScale<Kernel> {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    option_type value;
    Scalar calculate(Vector& param1,  Vector& param2) {
        return 0;
    };
    Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
        return 0;
    };
    Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
        return 0;
    };
    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient.setZero();
    };
    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient.setZero();
    };
};

template< typename Kernel >
struct ci_orientation::type< Kernel, tag::point3D, tag::line3D > : public ci_orientation::type< Kernel, tag::point3D, tag::point3D > {};

template< typename Kernel >
struct ci_orientation::type< Kernel, tag::point3D, tag::plane3D > : public ci_orientation::type< Kernel, tag::point3D, tag::point3D > {};

template< typename Kernel >
struct ci_orientation::type< Kernel, tag::point3D, tag::cylinder3D > : public ci_orientation::type< Kernel, tag::point3D, tag::point3D > {};

template< typename Kernel >
struct ci_orientation::type< Kernel, tag::line3D, tag::line3D > : public dcm::Orientation::type< Kernel, tag::line3D, tag::line3D > {};

template< typename Kernel >
struct ci_orientation::type< Kernel, tag::line3D, tag::plane3D > : public dcm::Orientation::type< Kernel, tag::line3D, tag::plane3D > {};

template< typename Kernel >
struct ci_orientation::type< Kernel, tag::line3D, tag::cylinder3D > : public dcm::Orientation::type< Kernel, tag::line3D, tag::cylinder3D > {};

template< typename Kernel >
struct ci_orientation::type< Kernel, tag::plane3D, tag::plane3D > : public dcm::Orientation::type< Kernel, tag::plane3D, tag::plane3D > {};

template< typename Kernel >
struct ci_orientation::type< Kernel, tag::cylinder3D, tag::cylinder3D > : public dcm::Orientation::type< Kernel, tag::cylinder3D, tag::cylinder3D > {};



//we need a custom distance type to use point-distance functions instead of real geometry distance
struct ci_distance : public Equation<ci_distance, double> {

    using Equation::operator=;
    ci_distance() : Equation(0) {};


    template< typename Kernel, typename Tag1, typename Tag2 >
    struct type : public PseudoScale<Kernel> {

        type() {
            throw constraint_error() <<  boost::errinfo_errno(104) << error_message("unsupported geometry in coincidence distance constraint")
                                     << error_type_first_geometry(typeid(Tag1).name()) << error_type_second_geometry(typeid(Tag2).name());
        };

        typedef typename Kernel::number_type Scalar;
        typedef typename Kernel::VectorMap   Vector;
        typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

        option_type value;
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

template< typename Kernel >
struct ci_distance::type< Kernel, tag::point3D, tag::point3D > : public dcm::Distance::type< Kernel, tag::point3D, tag::point3D > {};

template< typename Kernel >
struct ci_distance::type< Kernel, tag::point3D, tag::line3D > : public dcm::Distance::type< Kernel, tag::point3D, tag::line3D > {};

template< typename Kernel >
struct ci_distance::type< Kernel, tag::point3D, tag::plane3D > : public dcm::Distance::type< Kernel, tag::point3D, tag::plane3D > {};

template< typename Kernel >
struct ci_distance::type< Kernel, tag::point3D, tag::cylinder3D > : public dcm::Distance::type< Kernel, tag::point3D, tag::line3D > {};

template< typename Kernel >
struct ci_distance::type< Kernel, tag::line3D, tag::line3D > : public dcm::Distance::type< Kernel, tag::point3D, tag::line3D > {};

template< typename Kernel >
struct ci_distance::type< Kernel, tag::line3D, tag::plane3D > : public dcm::Distance::type< Kernel, tag::line3D, tag::plane3D > {};

template< typename Kernel >
struct ci_distance::type< Kernel, tag::line3D, tag::cylinder3D > : public dcm::Distance::type< Kernel, tag::point3D, tag::line3D > {};

template< typename Kernel >
struct ci_distance::type< Kernel, tag::plane3D, tag::plane3D > : public dcm::Distance::type< Kernel, tag::plane3D, tag::plane3D > {};

template< typename Kernel >
struct ci_distance::type< Kernel, tag::cylinder3D, tag::cylinder3D > : public dcm::Distance::type< Kernel, tag::point3D, tag::line3D > {};


}//details

struct Coincidence : public dcm::constraint_sequence< fusion::vector2< details::ci_distance, details::ci_orientation > > {
    //allow to set the distance
    Coincidence& operator()(Direction val) {
        fusion::at_c<1>(*this) = val;
        return *this;
    };
    Coincidence& operator=(Direction val) {
        fusion::at_c<1>(*this) = val;
        return *this;
    };
};

//no standart equation, create our own object
static Coincidence coincidence;

}//dcm


#endif //DCM_COINCIDENT_HPP
