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

#ifndef DCM_ALIGNMENT_HPP
#define DCM_ALIGNMENT_HPP

#include <opendcm/core/equations.hpp>
#include "distance.hpp"
#include "coincident.hpp"

namespace dcm {

namespace details {
//we need a custom orientation type to allow coincidents with points. We can't use the ci_orietation
//as some geometries are supporte by align but not by coincident
struct al_orientation : public Equation<al_orientation, Direction, 6, rotation> {

    using Equation::operator=;
    using Equation::options;
    al_orientation() : Equation() {
        setDefault();
    };

    al_orientation& operator=(const al_orientation& d) {
        return Equation::assign(d);
    };

    void setDefault() {
        fusion::at_key<Direction>(values) = std::make_pair(false, parallel);
    };

    template< typename Kernel, typename Tag1, typename Tag2 >
    struct type : public PseudoScale<Kernel> {

        type() {
            throw constraint_error() <<  boost::errinfo_errno(103) << error_message("unsupported geometry in alignment orientation constraint")
                                     << error_type_first_geometry(typeid(Tag1).name()) << error_type_second_geometry(typeid(Tag2).name());
        };

        typedef typename Kernel::number_type Scalar;
        typedef typename Kernel::VectorMap   Vector;
        typedef std::vector<typename Kernel::Vector3, Eigen::aligned_allocator<typename Kernel::Vector3> > Vec;

        typename al_orientation::options values;
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
struct al_orientation::type< Kernel, tag::line3D, tag::line3D > : public ci_orientation::type< Kernel, tag::line3D, tag::line3D > {};

template< typename Kernel >
struct al_orientation::type< Kernel, tag::line3D, tag::plane3D > : public ci_orientation::type< Kernel, tag::line3D, tag::plane3D > {};

template< typename Kernel >
struct al_orientation::type< Kernel, tag::line3D, tag::cylinder3D > : public ci_orientation::type< Kernel, tag::line3D, tag::cylinder3D > {};

template< typename Kernel >
struct al_orientation::type< Kernel, tag::plane3D, tag::plane3D > : public ci_orientation::type< Kernel, tag::plane3D, tag::plane3D > {};

template< typename Kernel >
struct al_orientation::type< Kernel, tag::plane3D, tag::cylinder3D > : public dcm::Orientation::type< Kernel, tag::plane3D, tag::cylinder3D > {
    //we missuse the scale method to change whatever direction was set to the only valid one: perpendicular
    void setScale(typename Kernel::number_type scale) {
        fusion::at_key<Direction>(dcm::Orientation::type< Kernel, tag::line3D, tag::plane3D >::values).second = perpendicular;
    };
};

template< typename Kernel >
struct al_orientation::type< Kernel, tag::cylinder3D, tag::cylinder3D > : public ci_orientation::type< Kernel, tag::cylinder3D, tag::cylinder3D > {};

  
}; //namespace details

//use al_orientation to ensure the correct orientations for alignment (distance is only defined for special
//orientations)
struct Alignment : public constraint_sequence< fusion::vector2< Distance, details::al_orientation >, Alignment > {

    using constraint_sequence::operator=;
};

static Alignment alignment;

}//dcm


#endif //DCM_ALIGNMENT_HPP
