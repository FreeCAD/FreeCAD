/*
 * 
 * Copyright (c) Toon Knapen, Karl Meerbergen & Kresimir Fresl 2003
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * KF acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_HEEV_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_HEEV_HPP

#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/traits/type.hpp>
#include <boost/numeric/bindings/lapack/lapack.h>
#include <boost/numeric/bindings/lapack/workspace.hpp>
#include <boost/numeric/bindings/traits/detail/array.hpp>
// #include <boost/numeric/bindings/traits/std_vector.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
#  include <boost/static_assert.hpp>
#  include <boost/type_traits.hpp>
#endif 


namespace boost { namespace numeric { namespace bindings { 

  namespace lapack {

    ///////////////////////////////////////////////////////////////////
    //
    // Eigendecomposition of a complex Hermitian matrix A = Q * D * Q'
    // 
    ///////////////////////////////////////////////////////////////////

    /* 
     * heev() computes the eigendecomposition of a N x N matrix
     * A = Q * D * Q',  where Q is a N x N unitary matrix and
     * D is a diagonal matrix. The diagonal element D(i,i) is an
     * eigenvalue of A and Q(:,i) is a corresponding eigenvector.
     * The eigenvalues are stored in ascending order.
     *
     * On return of heev, A is overwritten by Q and w contains the main
     * diagonal of D.
     *
     * int heev (char jobz, char uplo, A& a, W& w, minimal_workspace ) ;
     *    jobz : 'V' : compute eigenvectors
     *           'N' : do not compute eigenvectors
     *    uplo : 'U' : only the upper triangular part of A is used on input.
     *           'L' : only the lower triangular part of A is used on input.
     */ 

    namespace detail {

      inline 
      void heev (char const jobz, char const uplo, int const n,
		 traits::complex_f* a, int const lda,
                 float* w, traits::complex_f* work, int const lwork,
                 float* rwork, int& info) 
      {
        LAPACK_CHEEV (&jobz, &uplo, &n,
		      traits::complex_ptr(a), &lda, w,
		      traits::complex_ptr(work), &lwork,
		      rwork, &info);
      }

      inline 
      void heev (char const jobz, char const uplo, int const n,
		 traits::complex_d* a, int const lda,
                 double* w, traits::complex_d* work, int const lwork,
                 double* rwork, int& info) 
      {
        LAPACK_ZHEEV (&jobz, &uplo, &n,
		      traits::complex_ptr(a), &lda, w,
		      traits::complex_ptr(work), &lwork,
		      rwork, &info);
      }


      template <typename A, typename W, typename Work, typename RWork>
      inline
      int heev (char jobz, char uplo, A& a, W& w, Work& work, RWork& rwork) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<A>::matrix_structure, 
          traits::general_t
        >::value)); 
#endif 

        int const n = traits::matrix_size1 (a);
        assert (traits::matrix_size2 (a)==n); 
        assert (traits::vector_size (w)==n); 
        assert (2*n-1 <= traits::vector_size (work)); 
        assert (3*n-2 <= traits::vector_size (rwork)); 
        assert ( uplo=='U' || uplo=='L' );
        assert ( jobz=='N' || jobz=='V' );

        int info; 
        detail::heev (jobz, uplo, n,
                     traits::matrix_storage (a), 
                     traits::leading_dimension (a),
                     traits::vector_storage (w),  
                     traits::vector_storage (work),
                     traits::vector_size (work),
                     traits::vector_storage (rwork),
                     info);
        return info; 
      }
    } // namespace detail


    // Function that allocates temporary arrays
    template <typename A, typename W>
    int heev (char jobz, char uplo, A& a, W& w, minimal_workspace ) {
       typedef typename A::value_type                              value_type ;
       typedef typename traits::type_traits<value_type>::real_type real_type ;

       int const n = traits::matrix_size1 (a);

       traits::detail::array<value_type> work( std::max(1,2*n-1) );
       traits::detail::array<real_type> rwork( std::max(3*n-1,1) );

       return detail::heev( jobz, uplo, a, w, work, rwork );
    }


    // Function that allocates temporary arrays
    template <typename A, typename W>
    int heev (char jobz, char uplo, A& a, W& w, optimal_workspace ) {
       typedef typename A::value_type                              value_type ;
       typedef typename traits::type_traits<value_type>::real_type real_type ;

       int const n = traits::matrix_size1 (a);

       traits::detail::array<value_type> work( std::max(1,33*n) );
       traits::detail::array<real_type> rwork( std::max(3*n-1,1) );

       return detail::heev( jobz, uplo, a, w, work, rwork );
    }


    // Function that uses given workarrays
    template <typename A, typename W, typename WC, typename WR>
    int heev (char jobz, char uplo, A& a, W& w, detail::workspace2<WC,WR> workspace ) {
       typedef typename A::value_type                              value_type ;
       typedef typename traits::type_traits<value_type>::real_type real_type ;

       return detail::heev( jobz, uplo, a, w, workspace.w_, workspace.wr_ );
    }

  }

}}}

#endif 
