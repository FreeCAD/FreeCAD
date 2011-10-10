/*
 * 
 * Copyright (c) Karl Meerbergen & Kresimir Fresl 2003
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * KF acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_WORKSPACE_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_WORKSPACE_HPP

#include <boost/numeric/bindings/traits/detail/array_impl.hpp>
#include <boost/numeric/bindings/traits/type.hpp>
#include <boost/numeric/bindings/traits/type_traits.hpp>
#include <boost/numeric/bindings/traits/vector_traits.hpp>
#include <memory>

namespace boost { namespace numeric { namespace bindings { 

  /*
   * Organization of workspace in Lapack.
   * We allow one of the following arguments in a number of Lapack functions
   * - minimal_workspace() : the function allocates the minimum workspace required for the function
   * - optimal_workspace() : the function allocates the amount of workspace that allows optimal
   *                         execution.
   * - workspace( work )   : the function uses the workspace array in work.
   * - workspace( rwork, work ) : the function uses a real array rwork and a compolex array work as
   *                              workspace. (There are Lapack functions for complex matrices
   *                              that require two workarrays)
   * */

  namespace lapack {


     // Four classes are introduced to distinguish between the different type of memory allocations

     struct minimal_workspace {} ;

     struct optimal_workspace {} ;

     namespace detail {

        template <typename W>
        struct workspace1 {
           W& w_ ;

           workspace1(W& w)
           : w_( w )
           {}
        }; // struct workspace1

        template <typename W, typename WR>
        struct workspace2 {
           W& w_ ;
           WR& wr_ ;

           workspace2(W& w, WR& wr)
           : w_(w)
           , wr_(wr)
           {}
        }; // struct workspace2

     }

     template <typename W>
     detail::workspace1<W> workspace(W& w) {
        return detail::workspace1<W>(w) ;
     } // workspace()

     template <typename W, typename WR>
     detail::workspace2<W,WR> workspace(W& w, WR& wr) {
        return detail::workspace2<W,WR>(w, wr) ;
     } // workspace()


     /// Select the number of workspaces depending on the value_type
     template <typename T>
     struct n_workspace_args { };

     template <>
     struct n_workspace_args< float > {
        static const int value = 1 ;
     };

     template <>
     struct n_workspace_args< double > {
        static const int value = 1 ;
     };

     template <>
     struct n_workspace_args< traits::complex_f > {
        static const int value = 2 ;
     };

     template <>
     struct n_workspace_args< traits::complex_d > {
        static const int value = 2 ;
     };

  }

}}}

#endif
