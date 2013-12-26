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
struct ci_orientation : public Equation<ci_orientation, Direction, 4, rotation> {

    using Equation::operator=;
    using Equation::options;
    ci_orientation() : Equation() {
        setDefault();
    };

    ci_orientation& operator=(const ci_orientation& d) {
        return Equation::assign(d);
    };

    void setDefault() {
        fusion::at_key<Direction>(values) = std::make_pair(false, parallel);
    };

    template< typename Kernel, typename Tag1, typename Tag2 >
    struct type : public PseudoScale<Kernel> {

        type() {
            throw constraint_error() <<  boost::errinfo_errno(103) << error_message("unsupported geometry in coincidence orientation constraint")
                                     << error_type_first_geometry(typeid(Tag1).name()) << error_type_second_geometry(typeid(Tag2).name());
        };

        typedef typename Kernel::number_type Scalar;
        typedef typename Kernel::VectorMap   Vector;
        typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

        typename ci_orientation::options values;
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

template< typename Kernel >
struct ci_orientation::type< Kernel, tag::point3D, tag::point3D > : public dcm::PseudoScale<Kernel> {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    typename ci_orientation::options values;
    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
        return 0;
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& param1,
                                  const E::MatrixBase<DerivedB>& param2,
                                  const E::MatrixBase<DerivedC>& dparam1) {
        return 0;
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& param1,
                                   const E::MatrixBase<DerivedB>& param2,
                                   const E::MatrixBase<DerivedC>& dparam2) {
        return 0;
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& param1,
                                        const E::MatrixBase<DerivedB>& param2,
                                        E::MatrixBase<DerivedC>& gradient) {
        gradient.setZero();
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                         const E::MatrixBase<DerivedB>& param2,
                                         E::MatrixBase<DerivedC>& gradient) {
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
struct ci_orientation::type< Kernel, tag::line3D, tag::line3D > : public dcm::Orientation::type< Kernel, tag::line3D, tag::line3D > {
    //we missuse the scale method to prevent a unallowed direcion: perpendicular (ad distance is not defined for it)
    void setScale(typename Kernel::number_type scale) {
        if(fusion::at_key<Direction>(dcm::Orientation::type< Kernel, tag::line3D, tag::line3D >::values).second == perpendicular)
            fusion::at_key<Direction>(dcm::Orientation::type< Kernel, tag::line3D, tag::line3D >::values).second = parallel;
    };
};

template< typename Kernel >
struct ci_orientation::type< Kernel, tag::line3D, tag::plane3D > : public dcm::Orientation::type< Kernel, tag::line3D, tag::plane3D > {
    //we missuse the scale method to change whatever direction was set to the only valid one: perpendicular
    void setScale(typename Kernel::number_type scale) {
        fusion::at_key<Direction>(dcm::Orientation::type< Kernel, tag::line3D, tag::plane3D >::values).second = perpendicular;
    };
};

template< typename Kernel >
struct ci_orientation::type< Kernel, tag::line3D, tag::cylinder3D > : public dcm::Orientation::type< Kernel, tag::line3D, tag::cylinder3D > {
    //we missuse the scale method to prevent a unallowed direcion: perpendicular (ad distance is not defined for it)
    void setScale(typename Kernel::number_type scale) {
        if(fusion::at_key<Direction>(dcm::Orientation::type< Kernel, tag::line3D, tag::cylinder3D >::values).second == perpendicular)
            fusion::at_key<Direction>(dcm::Orientation::type< Kernel, tag::line3D, tag::cylinder3D >::values).second = parallel;
    };
};

template< typename Kernel >
struct ci_orientation::type< Kernel, tag::plane3D, tag::plane3D > : public dcm::Orientation::type< Kernel, tag::plane3D, tag::plane3D > {
//we missuse the scale method to prevent a unallowed direcion: perpendicular (ad distance is not defined for it)
    void setScale(typename Kernel::number_type scale) {
        if(fusion::at_key<Direction>(dcm::Orientation::type< Kernel, tag::plane3D, tag::plane3D >::values).second == perpendicular)
            fusion::at_key<Direction>(dcm::Orientation::type< Kernel, tag::plane3D, tag::plane3D >::values).second = parallel;
    };
};

template< typename Kernel >
struct ci_orientation::type< Kernel, tag::cylinder3D, tag::cylinder3D > : public dcm::Orientation::type< Kernel, tag::cylinder3D, tag::cylinder3D > {
    //we missuse the scale method to prevent a unallowed direcion: perpendicular (ad distance is not defined for it)
    void setScale(typename Kernel::number_type scale) {
        if(fusion::at_key<Direction>(dcm::Orientation::type< Kernel, tag::cylinder3D, tag::cylinder3D >::values).second == perpendicular)
            fusion::at_key<Direction>(dcm::Orientation::type< Kernel, tag::cylinder3D, tag::cylinder3D >::values).second = parallel;
    };
};



//we need a custom distance type to use point-distance functions instead of real geometry distance
struct ci_distance : public Equation<ci_distance, mpl::vector2<double, SolutionSpace>, 5 > {

    using Equation::operator=;
    using Equation::options;
    ci_distance() : Equation() {
        setDefault();
    };

    ci_distance& operator=(const ci_distance& d) {
        return Equation::assign(d);
    };

    void setDefault() {
        fusion::at_key<double>(values) = std::make_pair(false, 0.);
        fusion::at_key<SolutionSpace>(values) = std::make_pair(false, bidirectional);
    };

    template< typename Kernel, typename Tag1, typename Tag2 >
    struct type : public PseudoScale<Kernel> {

        type() {
            throw constraint_error() <<  boost::errinfo_errno(104) << error_message("unsupported geometry in coincidence distance constraint")
                                     << error_type_first_geometry(typeid(Tag1).name()) << error_type_second_geometry(typeid(Tag2).name());
        };

        typedef typename Kernel::number_type Scalar;
        typedef typename Kernel::VectorMap   Vector;
        typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

        typename ci_distance::options values;
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

struct Coincidence : public dcm::constraint_sequence< fusion::vector2< details::ci_distance, details::ci_orientation >, Coincidence > {
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
