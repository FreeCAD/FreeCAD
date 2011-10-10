//
// Copyright Karl Meerbergen 2007
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_SYTRD_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_SYTRD_HPP

#include <boost/numeric/bindings/lapack/workspace.hpp>
#include <boost/numeric/bindings/traits/vector_traits.hpp>
#include <boost/numeric/bindings/traits/matrix_traits.hpp>
#include <boost/numeric/bindings/traits/type_traits.hpp>
#include <boost/numeric/bindings/traits/detail/array.hpp>
#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/lapack/lapack.h>

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
#  include <boost/static_assert.hpp>
#  include <boost/type_traits/is_same.hpp>
#endif 


  /********************************************************************/
  /*                      eigenvalue problems                         */
  /********************************************************************/

  /* tridiagonal symmetric */


namespace boost { namespace numeric { namespace bindings { namespace lapack {

    namespace detail {

      inline 
      void sytrd ( char uplo, int n, float* a, int lda, float* d, float* e, float* tau, float* work, int lwork, int& info ) 
      {
        LAPACK_SSYTRD( &uplo, &n, a, &lda, d, e, tau, work, &lwork, &info ) ;
      }

      inline 
      void sytrd ( char uplo, int n, double* a, int lda, double* d, double* e, double* tau, double* work, int lwork, int& info ) 
      {
        LAPACK_DSYTRD( &uplo, &n, a, &lda, d, e, tau, work, &lwork, &info ) ;
      }

    } // namespace detail


    template <typename A, typename D, typename E, typename Tau, typename W>
    inline
    int sytrd( char uplo, A& a, D& d, E& e, Tau& tau, W& work ) {

      int const n = traits::matrix_size1 (a);
      assert( traits::matrix_size2 (a) == n );
      assert( traits::vector_size (d) == n );
      assert( traits::vector_size (e) == n-1 );
      assert( traits::vector_size (tau) == n-1 );
      assert( uplo=='U' || uplo=='L' );

      int lwork = traits::vector_size( work ) ;
      assert( lwork >= 1 );

      int info; 
      detail::sytrd( uplo, n,
                     traits::matrix_storage( a ), 
                     traits::leading_dimension( a ), 
                     traits::vector_storage( d ), 
                     traits::vector_storage( e ), 
                     traits::vector_storage( tau ), 
                     traits::vector_storage( work ), lwork,
                     info ) ;
      return info; 
    } // sytrd()


    template <typename A, typename D, typename E, typename Tau>
    inline
    int sytrd( char uplo, A& a, D& d, E& e, Tau& tau, optimal_workspace=optimal_workspace() ) {
      int info; 
      detail::sytrd( uplo, traits::matrix_size1( a ),
                     traits::matrix_storage( a ), 
                     traits::leading_dimension( a ), 
                     traits::vector_storage( d ), 
                     traits::vector_storage( e ), 
                     traits::vector_storage( tau ), 
                     traits::vector_storage( tau ), -1,
                     info ) ;
      if (info) return info ;
      int lwork = * traits::vector_storage( tau ) ;

      traits::detail::array<typename traits::vector_traits<D>::value_type> work( lwork );

      return sytrd( uplo, a, d, e, tau, work ) ;
    }


    template <typename A, typename D, typename E, typename Tau>
    inline
    int sytrd( char uplo, A& a, D& d, E& e, Tau& tau, minimal_workspace ) {
      int lwork = 1 ;
      traits::detail::array<typename traits::vector_traits<D>::value_type> work( lwork );

      return sytrd( uplo, a, d, e, tau, work ) ;
    }

}}}}

#endif // BOOST_NUMERIC_BINDINGS_LAPACK_GBSV_HPP
