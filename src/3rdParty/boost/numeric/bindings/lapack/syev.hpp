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

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_SYEV_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_SYEV_HPP

#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/lapack/lapack.h>
#include <boost/numeric/bindings/lapack/workspace.hpp>
#include <boost/numeric/bindings/traits/detail/array.hpp>
// #include <boost/numeric/bindings/traits/std_vector.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
#  include <boost/static_assert.hpp>
#  include <boost/type_traits.hpp>
#endif 

#include <cassert>


namespace boost { namespace numeric { namespace bindings { 

  namespace lapack {

    ///////////////////////////////////////////////////////////////////
    //
    // Eigendecomposition of a real symmetric matrix A = Q * D * Q'
    // 
    ///////////////////////////////////////////////////////////////////

    /* 
     * syev() computes the eigendecomposition of a N x N matrix
     * A = Q * D * Q',  where Q is a N x N orthogonal matrix and
     * D is a diagonal matrix. The diagonal elements D(i,i) is an
     * eigenvalue of A and Q(:,i) is a corresponding eigenvector.
     *
     * On return of syev, A is overwritten by Q and w contains the main
     * diagonal of D.
     *
     * int syev (char jobz, char uplo, A& a, W& w, minimal_workspace ) ;
     *    jobz : 'V' : compute eigenvectors
     *           'N' : do not compute eigenvectors
     *    uplo : 'U' : only the upper triangular part of A is used on input.
     *           'L' : only the lower triangular part of A is used on input.
     */ 

    namespace detail {

      inline 
      void syev (char const jobz, char const uplo, int const n,
                 float* a, int const lda,
                 float* w, float* work, int const lwork, int& info) 
      {
        LAPACK_SSYEV (&jobz, &uplo, &n, a, &lda, w, work, &lwork, &info);
      }

      inline 
      void syev (char const jobz, char const uplo, int const n,
                 double* a, int const lda,
                 double* w, double* work, int const lwork, int& info) 
      {
        LAPACK_DSYEV (&jobz, &uplo, &n, a, &lda, w, work, &lwork, &info);
      }


      template <typename A, typename W, typename Work>
      inline
      int syev (char jobz, char uplo, A& a, W& w, Work& work) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<A>::matrix_structure, 
          traits::general_t
        >::value)); 
#endif 

        int const n = traits::matrix_size1 (a);
        assert ( n>0 );
        assert (traits::matrix_size2 (a)==n); 
        assert (traits::leading_dimension (a)>=n); 
        assert (traits::vector_size (w)==n); 
        assert (3*n-1 <= traits::vector_size (work)); 
        assert ( uplo=='U' || uplo=='L' );
        assert ( jobz=='N' || jobz=='V' );

        int info; 
        detail::syev (jobz, uplo, n,
                     traits::matrix_storage (a), 
                     traits::leading_dimension (a),
                     traits::vector_storage (w),  
                     traits::vector_storage (work),
                     traits::vector_size (work),
                     info);
        return info; 
      }
    }  // namespace detail


    // Function that allocates work arrays
    template <typename A, typename W>
    inline
    int syev (char jobz, char uplo, A& a, W& w, optimal_workspace ) {
       typedef typename A::value_type value_type ;

       int const n = traits::matrix_size1 (a);

       traits::detail::array<value_type> work( std::max(1,34*n) );
       return detail::syev(jobz, uplo, a, w, work);
    } // syev()


    // Function that allocates work arrays
    template <typename A, typename W>
    inline
    int syev (char jobz, char uplo, A& a, W& w, minimal_workspace ) {
       typedef typename A::value_type value_type ;

       int const n = traits::matrix_size1 (a);

       traits::detail::array<value_type> work( std::max(1,3*n-1) );
       return detail::syev(jobz, uplo, a, w, work);
    } // syev()


    // Function that allocates work arrays
    template <typename A, typename W, typename Work>
    inline
    int syev (char jobz, char uplo, A& a, W& w, detail::workspace1<Work> workspace ) {
       typedef typename A::value_type value_type ;

       return detail::syev(jobz, uplo, a, w, workspace.w_);
    } // syev()

  }

}}}

#endif 
