//
// Copyright Karl Meerbergen 2007
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_STEQR_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_STEQR_HPP

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
      void steqr ( char compz, int n, float* d, float* e, float* z, int ldz, float* work, int& info ) 
      {
        LAPACK_SSTEQR( &compz, &n, d, e, z, &ldz, work, &info ) ;
      }

      inline 
      void steqr ( char compz, int n, double* d, double* e, double* z, int ldz, double* work, int& info ) 
      {
        LAPACK_DSTEQR( &compz, &n, d, e, z, &ldz, work, &info ) ;
      }

    } // namespace detail


    template <typename D, typename E, typename Z, typename W>
    inline
    int steqr( char compz, D& d, E& e, Z& z, W& work ) {

      int const n = traits::vector_size (d);
      assert( traits::vector_size (e) == n-1 );
      assert( traits::matrix_size1 (z) == n );
      assert( traits::matrix_size2 (z) == n );
      assert( compz=='N' || compz=='V' || compz=='I' );

      int lwork = traits::vector_size( work ) ;

      int info; 
      detail::steqr( compz, n,
                     traits::vector_storage( d ), 
                     traits::vector_storage( e ), 
                     traits::matrix_storage( z ), 
                     traits::leading_dimension( z ), 
                     traits::vector_storage( work ),  
                     info ) ;
      return info; 
    } // steqr()


    template <typename D, typename E, typename Z>
    inline
    int steqr( char compz, D& d, E& e, Z& z, optimal_workspace ) {
      int lwork = 0 ;
      if (compz != 'N') lwork = 2 * traits::vector_size( d ) - 2 ;

      traits::detail::array<typename traits::vector_traits<D>::value_type> work( lwork );

      return steqr( compz, d, e, z, work ) ;
    }


    template <typename D, typename E, typename Z>
    inline
    int steqr( char compz, D& d, E& e, Z& z, minimal_workspace ) {
      int lwork = 1 ;
      if (compz != 'N') lwork = 2 * traits::vector_size( e ) ;

      traits::detail::array<typename traits::vector_traits<D>::value_type> work( lwork );

      return steqr( compz, d, e, z, work ) ;
    }

    template <typename D, typename E, typename Z>
    inline
    int steqr( char compz, D& d, E& e, Z& z ) {
      return steqr( compz, d, e, z, minimal_workspace() ) ;
    }


}}}}

#endif // BOOST_NUMERIC_BINDINGS_LAPACK_GBSV_HPP
