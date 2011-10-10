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

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_ORMQR_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_ORMQR_HPP

#include <complex>
#include <boost/numeric/bindings/traits/traits.hpp>
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
    // Apply the orthogonal transformation abtained by geqrf() to
    // a general matrix.
    // 
    ///////////////////////////////////////////////////////////////////

    /* 
     * ormqr() overwrites the general M by N matrix C with
     *
     *                         SIDE = 'L'     SIDE = 'R'
     *  TRANS = 'N':             Q * C          C * Q
     *  TRANS = 'C' || 'T':      Q**H * C       C * Q**H
     *
     *  where Q is a complex unitary matrix defined as the product of k
     *  elementary reflectors
     *
     *        Q = H(1) H(2) . . . H(k)
     *
     *  as returned by geqrf(). Q is of order M if SIDE = 'L' and of order N
     *  if SIDE = 'R'.
     *
     *  Workspace is organized following the arguments in the calling sequence.
     *   optimal_workspace() : for optimizing use of blas 3 kernels
     *   minimal_workspace() : minimum size of workarrays, but does not allow for optimization
     *                         of blas 3 kernels
     *   workspace( work ) where is an array that is used as auxiliary memory with the same value_type
     *                     as C.
     *                     We must have that vector_size( work ) >= matrix_size2( c )
     *                     if SIDE=='L' otherwise  vector_size( work ) >= matrix_size1( c )
     */ 

    namespace detail {

      inline 
      void ormqr (char const side, char const trans, int const m, int const n,
		 int const k, const float* a, int const lda,
		 const float* tau, float* c,
		 int const ldc, float* work, int const lwork,
                 int& info) 
      {
        LAPACK_SORMQR (&side, &trans, &m, &n, &k,
		      a, &lda,
		      tau,
		      c, &ldc,
		      work, &lwork,
		      &info);
      }

      inline 
      void ormqr (char const side, char const trans, int const m, int const n,
		 int const k, const double* a, int const lda,
		 const double* tau, double* c,
		 int const ldc, double* work, int const lwork,
                 int& info) 
      {
        LAPACK_DORMQR (&side, &trans, &m, &n, &k,
		      a, &lda,
		      tau,
		      c, &ldc,
		      work, &lwork,
		      &info);
      }

      inline 
      void ormqr (char const side, char const trans, int const m, int const n,
		 int const k, const traits::complex_f* a, int const lda,
		 const traits::complex_f* tau, traits::complex_f* c,
		 int const ldc, traits::complex_f* work, int const lwork,
                 int& info) 
      {
        LAPACK_CUNMQR (&side, &trans, &m, &n, &k,
		      traits::complex_ptr(a), &lda,
		      traits::complex_ptr(tau),
		      traits::complex_ptr(c), &ldc,
		      traits::complex_ptr(work), &lwork,
		      &info);
      }

      inline 
      void ormqr (char const side, char const trans, int const m, int const n,
		 int const k, const traits::complex_d* a, int const lda,
		 const traits::complex_d* tau, traits::complex_d* c,
		 int const ldc, traits::complex_d* work, int const lwork,
                 int& info) 
      {
        LAPACK_ZUNMQR (&side, &trans, &m, &n, &k,
		      traits::complex_ptr(a), &lda,
		      traits::complex_ptr(tau),
		      traits::complex_ptr(c), &ldc,
		      traits::complex_ptr(work), &lwork,
		      &info);
      }


      template <typename A, typename Tau, typename C, typename Work>
      inline
      int ormqr (char side, char trans, const A& a, const Tau& tau, C& c,
                 Work& work) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<A>::matrix_structure, 
          traits::general_t
        >::value)); 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<C>::matrix_structure, 
          traits::general_t
        >::value)); 
#endif 

        int const m = traits::matrix_size1 (c);
        int const n = traits::matrix_size2 (c);
        int const k = traits::vector_size (tau);
        int const lwork = traits::vector_size (work);

        assert ( side=='L' || side=='R' );
        assert ( trans=='N' || trans=='C' || trans=='T' );
        assert ( (side=='L' ?  m >= k : n >= k ) );

        assert ( (side=='L' ?
                  m == traits::matrix_size1 (a) :
                  n == traits::matrix_size1 (a) ) );
        assert (traits::matrix_size2 (a)==k); 

        assert ( (side=='L' ?
                  lwork >= n : lwork >= m ) );

        int info; 
        ormqr (side, trans, m, n, k,
                       traits::matrix_storage (a), 
                       traits::leading_dimension (a),
                       traits::vector_storage (tau),  
                       traits::matrix_storage (c), 
                       traits::leading_dimension (c),
                       traits::vector_storage (work),
                       lwork,
                       info);
        return info; 
      }
    } // namespace detail


    // Function that allocates temporary arrays for optimal execution time.
    template <typename A, typename Tau, typename C>
    inline
    int ormqr (char side, char trans, const A& a, const Tau& tau, C& c, optimal_workspace ) {
       typedef typename A::value_type                              value_type ;

       int const n_w = (side=='L' ? traits::matrix_size2 (c)
                                  : traits::matrix_size1 (c) );

       traits::detail::array<value_type> work( std::max(1,n_w*32) );

       return detail::ormqr( side, trans, a, tau, c, work );
    }


    // Function that allocates temporary arrays with minimal size
    template <typename A, typename Tau, typename C>
    inline
    int ormqr (char side, char trans, const A& a, const Tau& tau, C& c, minimal_workspace ) {
       typedef typename A::value_type                              value_type ;

       int const n_w = (side=='L' ? traits::matrix_size2 (c)
                                  : traits::matrix_size1 (c) );

       traits::detail::array<value_type> work( std::max(1,n_w) );

       return detail::ormqr( side, trans, a, tau, c, work );
    }


    // Function that uses auxiliary array in workspace
    // The calling sequence is ormqr(side, trans, a, tau, c, workspace( work_array ) )
    // where work_array is an array with the same value_type as a and c .
    template <typename A, typename Tau, typename C, typename Work>
    inline
    int ormqr (char side, char trans, const A& a, const Tau& tau, C& c, detail::workspace1<Work> workspace ) {
       typedef typename A::value_type                              value_type ;

       return detail::ormqr( side, trans, a, tau, c, workspace.w_ );
    }

  }

}}}

#endif 
