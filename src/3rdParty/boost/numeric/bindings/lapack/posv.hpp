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

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_POSV_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_POSV_HPP

#include <boost/numeric/bindings/traits/type_traits.hpp>
#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/lapack/lapack.h>

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
#  include <boost/numeric/bindings/traits/detail/symm_herm_traits.hpp>
#  include <boost/static_assert.hpp>
#  include <boost/type_traits/is_same.hpp>
#endif 

#include <cassert>

namespace boost { namespace numeric { namespace bindings { 

  namespace lapack {

    /////////////////////////////////////////////////////////////////////
    //
    // system of linear equations A * X = B
    // with A symmetric or Hermitian positive definite matrix
    //
    /////////////////////////////////////////////////////////////////////

    /*
     * posv() computes the solution to a system of linear equations 
     * A * X = B, where A is an N-by-N symmetric or Hermitian positive 
     * definite matrix and X and B are N-by-NRHS matrices.
     *
     * The Cholesky decomposition is used to factor A as
     *   A = U^T * U or A = U^H * U,  if UPLO = 'U', 
     *   A = L * L^T or A = L * L^H,  if UPLO = 'L',
     * where U is an upper triangular matrix and L is a lower triangular
     * matrix. The factored form of A is then used to solve the system of
     * equations A * X = B.
     * 
     * If UPLO = 'U', the leading N-by-N upper triangular part of A 
     * contains the upper triangular part of the matrix A, and the 
     * strictly lower triangular part of A is not referenced. 
     * If UPLO = 'L', the leading N-by-N lower triangular part of A 
     * contains the lower triangular part of the matrix A, and the 
     * strictly upper triangular part of A is not referenced.
     */

    namespace detail {

      inline 
      void posv (char const uplo, int const n, int const nrhs,
                 float* a, int const lda, 
                 float* b, int const ldb, int* info) 
      {
        LAPACK_SPOSV (&uplo, &n, &nrhs, a, &lda, b, &ldb, info);
      }

      inline 
      void posv (char const uplo, int const n, int const nrhs,
                 double* a, int const lda, 
                 double* b, int const ldb, int* info) 
      {
        LAPACK_DPOSV (&uplo, &n, &nrhs, a, &lda, b, &ldb, info);
      }

      inline 
      void posv (char const uplo, int const n, int const nrhs,
                 traits::complex_f* a, int const lda, 
                 traits::complex_f* b, int const ldb, int* info) 
      {
        LAPACK_CPOSV (&uplo, &n, &nrhs, 
                      traits::complex_ptr (a), &lda, 
                      traits::complex_ptr (b), &ldb, info);
      }

      inline 
      void posv (char const uplo, int const n, int const nrhs,
                 traits::complex_d* a, int const lda, 
                 traits::complex_d* b, int const ldb, int* info) 
      {
        LAPACK_ZPOSV (&uplo, &n, &nrhs, 
                      traits::complex_ptr (a), &lda, 
                      traits::complex_ptr (b), &ldb, info);
      }

      template <typename SymmMatrA, typename MatrB>
      inline
      int posv (char const uplo, SymmMatrA& a, MatrB& b) {
        int const n = traits::matrix_size1 (a);
        assert (n == traits::matrix_size2 (a));
        assert (n == traits::matrix_size1 (b));
        int info; 
        posv (uplo, n, traits::matrix_size2 (b),
              traits::matrix_storage (a), 
              traits::leading_dimension (a),
              traits::matrix_storage (b), 
              traits::leading_dimension (b), 
              &info);
        return info; 
      }

    }

    template <typename SymmMatrA, typename MatrB>
    inline
    int posv (char const uplo, SymmMatrA& a, MatrB& b) {

      assert (uplo == 'U' || uplo == 'L'); 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmMatrA>::matrix_structure, 
        traits::general_t
      >::value));
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
      >::value));
#endif

      return detail::posv (uplo, a, b); 
    }

    template <typename SymmMatrA, typename MatrB>
    inline
    int posv (SymmMatrA& a, MatrB& b) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      typedef traits::matrix_traits<SymmMatrA> matraits;
      typedef typename matraits::value_type val_t;
      BOOST_STATIC_ASSERT( (traits::detail::symm_herm_compatible< val_t, typename matraits::matrix_structure >::value ) ) ;
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
      >::value));
#endif

      char uplo = traits::matrix_uplo_tag (a);
      return detail::posv (uplo, a, b); 
    }


    /*
     * potrf() computes the Cholesky factorization of a symmetric
     * or Hermitian positive definite matrix A. The factorization has 
     * the form
     *   A = U^T * U or A = U^H * U,  if UPLO = 'U', 
     *   A = L * L^T or A = L * L^H,  if UPLO = 'L',
     * where U is an upper triangular matrix and L is lower triangular.
     */

    namespace detail {

      inline 
      void potrf (char const uplo, int const n, 
                  float* a, int const lda, int* info) 
      {
        LAPACK_SPOTRF (&uplo, &n, a, &lda, info);
      }

      inline 
      void potrf (char const uplo, int const n, 
                  double* a, int const lda, int* info) 
      {
        LAPACK_DPOTRF (&uplo, &n, a, &lda, info);
      }

      inline 
      void potrf (char const uplo, int const n, 
                  traits::complex_f* a, int const lda, int* info) 
      {
        LAPACK_CPOTRF (&uplo, &n, traits::complex_ptr (a), &lda, info);
      }

      inline 
      void potrf (char const uplo, int const n, 
                  traits::complex_d* a, int const lda, int* info) 
      {
        LAPACK_ZPOTRF (&uplo, &n, traits::complex_ptr (a), &lda, info);
      }

      template <typename SymmMatrA> 
      inline
      int potrf (char const uplo, SymmMatrA& a) {
        int const n = traits::matrix_size1 (a);
        assert (n == traits::matrix_size2 (a));
        int info; 
        potrf (uplo, n, traits::matrix_storage (a), 
               traits::leading_dimension (a), &info);
        return info; 
      }

    }

    template <typename SymmMatrA> 
    inline
    int potrf (char const uplo, SymmMatrA& a) {

      assert (uplo == 'U' || uplo == 'L'); 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmMatrA>::matrix_structure, 
        traits::general_t
      >::value));
#endif

      return detail::potrf (uplo, a); 
    }

    template <typename SymmMatrA>
    inline
    int potrf (SymmMatrA& a) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      typedef traits::matrix_traits<SymmMatrA> matraits;
      typedef typename matraits::value_type val_t;
      BOOST_STATIC_ASSERT((boost::is_same<
        typename matraits::matrix_structure,
        typename traits::detail::symm_herm_t<val_t>::type
      >::value));
#endif
      
      char uplo = traits::matrix_uplo_tag (a);
      return detail::potrf (uplo, a); 
    }


    /*
     * potrs() solves a system of linear equations A*X = B with 
     * a symmetric or Hermitian positive definite matrix A using 
     * the Cholesky factorization computed by potrf().
     */

    namespace detail {

      inline 
      void potrs (char const uplo, int const n, int const nrhs,
                  float const* a, int const lda, 
                  float* b, int const ldb, int* info) 
      {
        LAPACK_SPOTRS (&uplo, &n, &nrhs, a, &lda, b, &ldb, info);
      }

      inline 
      void potrs (char const uplo, int const n, int const nrhs,
                  double const* a, int const lda, 
                  double* b, int const ldb, int* info) 
      {
        LAPACK_DPOTRS (&uplo, &n, &nrhs, a, &lda, b, &ldb, info);
      }

      inline 
      void potrs (char const uplo, int const n, int const nrhs,
                  traits::complex_f const* a, int const lda, 
                  traits::complex_f* b, int const ldb, int* info) 
      {
        LAPACK_CPOTRS (&uplo, &n, &nrhs, 
                       traits::complex_ptr (a), &lda, 
                       traits::complex_ptr (b), &ldb, info);
      }

      inline 
      void potrs (char const uplo, int const n, int const nrhs,
                  traits::complex_d const* a, int const lda, 
                  traits::complex_d* b, int const ldb, int* info) 
      {
        LAPACK_ZPOTRS (&uplo, &n, &nrhs, 
                       traits::complex_ptr (a), &lda, 
                       traits::complex_ptr (b), &ldb, info);
      }

      template <typename SymmMatrA, typename MatrB>
      inline
      int potrs (char const uplo, SymmMatrA const& a, MatrB& b) {
        int const n = traits::matrix_size1 (a);
        assert (n == traits::matrix_size2 (a));
        assert (n == traits::matrix_size1 (b));
        int info; 
        potrs (uplo, n, traits::matrix_size2 (b),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
               traits::matrix_storage (a), 
#else
               traits::matrix_storage_const (a), 
#endif 
               traits::leading_dimension (a),
               traits::matrix_storage (b), 
               traits::leading_dimension (b), 
               &info);
        return info; 
      }

    }

    template <typename SymmMatrA, typename MatrB>
    inline
    int potrs (char const uplo, SymmMatrA const& a, MatrB& b) {

      assert (uplo == 'U' || uplo == 'L'); 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmMatrA>::matrix_structure, 
        traits::general_t
      >::value));
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
      >::value));
#endif

      return detail::potrs (uplo, a, b); 
    }

    template <typename SymmMatrA, typename MatrB>
    inline
    int potrs (SymmMatrA const& a, MatrB& b) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      typedef traits::matrix_traits<SymmMatrA> matraits;
      typedef traits::matrix_traits<MatrB> mbtraits;
      typedef typename matraits::value_type val_t;
      BOOST_STATIC_ASSERT((boost::is_same<
        typename matraits::matrix_structure,
        typename traits::detail::symm_herm_t<val_t>::type
      >::value));
      BOOST_STATIC_ASSERT((boost::is_same<
        typename mbtraits::matrix_structure, traits::general_t
      >::value));
#endif

      char uplo = traits::matrix_uplo_tag (a);
      return detail::potrs (uplo, a, b); 
    }

    // TO DO: potri() 

  }

}}}

#endif 
