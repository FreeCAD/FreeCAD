/*
 * 
 * Copyright (c) Toon Knapen & Kresimir Fresl 2003
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * KF acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_GESV_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_GESV_HPP

#include <boost/numeric/bindings/traits/type_traits.hpp>
#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/lapack/lapack.h>
#include <boost/numeric/bindings/traits/detail/array.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
#  include <boost/static_assert.hpp>
#  include <boost/type_traits/is_same.hpp>
#endif 

#include <cassert>


namespace boost { namespace numeric { namespace bindings { 

  namespace lapack {

    ///////////////////////////////////////////////////////////////////
    //
    // general system of linear equations A * X = B
    // 
    ///////////////////////////////////////////////////////////////////

    /* 
     * gesv() computes the solution to a system of linear equations 
     * A * X = B, where A is an N-by-N matrix and X and B are N-by-NRHS 
     * matrices.
     *
     * The LU decomposition with partial pivoting and row interchanges 
     * is used to factor A as A = P * L * U, where P is a permutation 
     * matrix, L is unit lower triangular, and U is upper triangular.   
     * The factored form of A is then used to solve the system of 
     * equations A * X = B.
     */ 

    namespace detail {

      inline 
      void gesv (int const n, int const nrhs,
                 float* a, int const lda, int* ipiv, 
                 float* b, int const ldb, int* info) 
      {
        LAPACK_SGESV (&n, &nrhs, a, &lda, ipiv, b, &ldb, info);
      }

      inline 
      void gesv (int const n, int const nrhs,
                 double* a, int const lda, int* ipiv, 
                 double* b, int const ldb, int* info) 
      {
        LAPACK_DGESV (&n, &nrhs, a, &lda, ipiv, b, &ldb, info);
      }

      inline 
      void gesv (int const n, int const nrhs,
                 traits::complex_f* a, int const lda, int* ipiv, 
                 traits::complex_f* b, int const ldb, int* info) 
      {
        LAPACK_CGESV (&n, &nrhs, 
                      traits::complex_ptr (a), &lda, ipiv, 
                      traits::complex_ptr (b), &ldb, info);
      }
      
      inline 
      void gesv (int const n, int const nrhs,
                 traits::complex_d* a, int const lda, int* ipiv, 
                 traits::complex_d* b, int const ldb, int* info) 
      {
        LAPACK_ZGESV (&n, &nrhs, 
                      traits::complex_ptr (a), &lda, ipiv, 
                      traits::complex_ptr (b), &ldb, info);
      }

    } 

    template <typename MatrA, typename MatrB, typename IVec>
    inline
    int gesv (MatrA& a, IVec& ipiv, MatrB& b) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrA>::matrix_structure, 
        traits::general_t
      >::value)); 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (n == traits::matrix_size1 (b)); 
      assert (n == traits::vector_size (ipiv)); 

      int info; 
      detail::gesv (n, traits::matrix_size2 (b), 
                    traits::matrix_storage (a), 
                    traits::leading_dimension (a),
                    traits::vector_storage (ipiv),  
                    traits::matrix_storage (b),
                    traits::leading_dimension (b),
                    &info);
      return info; 
    }

    template <typename MatrA, typename MatrB>
    inline
    int gesv (MatrA& a, MatrB& b) {
      // with 'internal' pivot vector 
      
      // gesv() errors: 
      //   if (info == 0), successful
      //   if (info < 0), the -info argument had an illegal value
      //   -- we will use -101 if allocation fails
      //   if (info > 0), U(i-1,i-1) is exactly zero 
      int info = -101; 
      traits::detail::array<int> ipiv (traits::matrix_size1 (a)); 
      if (ipiv.valid()) 
        info = gesv (a, ipiv, b); 
      return info; 
    }


    /* 
     * getrf() computes an LU factorization of a general M-by-N matrix A  
     * using partial pivoting with row interchanges. The factorization 
     * has the form A = P * L * U, where P is a permutation matrix, 
     * L is lower triangular with unit diagonal elements (lower
     * trapezoidal if M > N), and U is upper triangular (upper 
     * trapezoidal if M < N).
     */ 

    namespace detail {

      inline 
      void getrf (int const n, int const m,
                  float* a, int const lda, int* ipiv, int* info) 
      {
        LAPACK_SGETRF (&n, &m, a, &lda, ipiv, info);
      }

      inline 
      void getrf (int const n, int const m,
                  double* a, int const lda, int* ipiv, int* info) 
      {
        LAPACK_DGETRF (&n, &m, a, &lda, ipiv, info);
      }

      inline 
      void getrf (int const n, int const m,
                  traits::complex_f* a, int const 
                  lda, int* ipiv, int* info) 
      {
        LAPACK_CGETRF (&n, &m, traits::complex_ptr (a), &lda, ipiv, info);
      }

      inline 
      void getrf (int const n, int const m,
                  traits::complex_d* a, int const lda, 
                  int* ipiv, int* info) 
      {
        LAPACK_ZGETRF (&n, &m, traits::complex_ptr (a), &lda, ipiv, info);
      }

    }

    template <typename MatrA, typename IVec>
    inline
    int getrf (MatrA& a, IVec& ipiv) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrA>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      int const m = traits::matrix_size2 (a); 
      assert (traits::vector_size (ipiv) == (m < n ? m : n));

      int info; 
      detail::getrf (n, m, 
                     traits::matrix_storage (a), 
                     traits::leading_dimension (a),
                     traits::vector_storage (ipiv),  
                     &info);
      return info; 
    }


    /*
     * getrs() solves a system of linear equations A * X = B 
     * or A^T * X = B with a general N-by-N matrix A using  
     * the LU factorization computed by getrf().
     */

    namespace detail {

      inline 
      void getrs (char const trans, int const n, int const nrhs,
                  float const* a, int const lda, int const* ipiv, 
                  float* b, int const ldb, int* info) 
      {
        LAPACK_SGETRS (&trans, &n, &nrhs, a, &lda, ipiv, b, &ldb, info);
      }

      inline 
      void getrs (char const trans, int const n, int const nrhs,
                  double const* a, int const lda, int const* ipiv, 
                  double* b, int const ldb, int* info) 
      {
        LAPACK_DGETRS (&trans, &n, &nrhs, a, &lda, ipiv, b, &ldb, info);
      }

      inline 
      void getrs (char const trans, int const n, int const nrhs,
                  traits::complex_f const* a, int const lda, 
                  int const* ipiv, 
                  traits::complex_f* b, int const ldb, int* info) 
      {
        LAPACK_CGETRS (&trans, &n, &nrhs, 
                       traits::complex_ptr (a), &lda, ipiv, 
                       traits::complex_ptr (b), &ldb, info);
      }

      inline 
      void getrs (char const trans, int const n, int const nrhs,
                  traits::complex_d const* a, int const lda, 
                  int const* ipiv, 
                  traits::complex_d* b, int const ldb, int* info) 
      {
        LAPACK_ZGETRS (&trans, &n, &nrhs, 
                       traits::complex_ptr (a), &lda, ipiv, 
                       traits::complex_ptr (b), &ldb, info);
      }

    }

    template <typename MatrA, typename MatrB, typename IVec>
    inline
    int getrs (char const trans, MatrA const& a, IVec const& ipiv, MatrB& b) 
    {
      assert (trans == 'N' || trans == 'T' || trans == 'C'); 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrA>::matrix_structure, 
        traits::general_t
      >::value)); 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (n == traits::matrix_size1 (b)); 
      assert (n == traits::vector_size (ipiv)); 

      int info; 
      detail::getrs (trans, n, traits::matrix_size2 (b), 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                     traits::matrix_storage (a), 
#else
                     traits::matrix_storage_const (a), 
#endif 
                     traits::leading_dimension (a),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                     traits::vector_storage (ipiv),  
#else
                     traits::vector_storage_const (ipiv),  
#endif
                     traits::matrix_storage (b),
                     traits::leading_dimension (b),
                     &info);
      return info; 
    }

    template <typename MatrA, typename MatrB, typename IVec>
    inline
    int getrs (MatrA const& a, IVec const& ipiv, MatrB& b) {
      char const no_transpose = 'N'; 
      return getrs (no_transpose, a, ipiv, b); 
    }

    // TO DO: getri() 

  }

}}}

#endif 
