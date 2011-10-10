/*
 * 
 * Copyright (c) Kresimir Fresl Toon Knapen 2003
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * KF acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */
/* Modified to include xPPTRI by Kian Ming A. Chai (14 May 2008) */

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_PPSV_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_PPSV_HPP

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
    // stored in packed format 
    //
    /////////////////////////////////////////////////////////////////////

    /*
     * ppsv() computes the solution to a system of linear equations 
     * A * X = B, where A is an N-by-N symmetric or Hermitian positive 
     * definite matrix stored in packed format and X and B are N-by-NRHS 
     * matrices.
     *
     * The Cholesky decomposition is used to factor A as
     *   A = U^T * U or A = U^H * U,  if UPLO = 'U', 
     *   A = L * L^T or A = L * L^H,  if UPLO = 'L',
     * where U is an upper triangular matrix and L is a lower triangular
     * matrix. The factored form of A is then used to solve the system of
     * equations A * X = B.
     * 
     * Only upper or lower triangle of the symmetric matrix A is stored,  
     * packed columnwise in a linear array AP. 
     */

    namespace detail {

      inline 
      void ppsv (char const uplo, int const n, int const nrhs,
                 float* ap, float* b, int const ldb, int* info) 
      {
        LAPACK_SPPSV (&uplo, &n, &nrhs, ap, b, &ldb, info);
      }

      inline 
      void ppsv (char const uplo, int const n, int const nrhs,
                 double* ap, double* b, int const ldb, int* info) 
      {
        LAPACK_DPPSV (&uplo, &n, &nrhs, ap, b, &ldb, info);
      }

      inline 
      void ppsv (char const uplo, int const n, int const nrhs,
                 traits::complex_f* ap, traits::complex_f* b, int const ldb, 
                 int* info) 
      {
        LAPACK_CPPSV (&uplo, &n, &nrhs, 
                      traits::complex_ptr (ap), 
                      traits::complex_ptr (b), &ldb, info);
      }

      inline 
      void ppsv (char const uplo, int const n, int const nrhs,
                 traits::complex_d* ap, traits::complex_d* b, int const ldb, 
                 int* info) 
      {
        LAPACK_ZPPSV (&uplo, &n, &nrhs, 
                      traits::complex_ptr (ap), 
                      traits::complex_ptr (b), &ldb, info);
      }

    }

    template <typename SymmMatrA, typename MatrB>
    inline
    int ppsv (SymmMatrA& a, MatrB& b) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmMatrA>::matrix_structure,
        typename traits::detail::symm_herm_pack_t<
          typename traits::matrix_traits<SymmMatrA>::value_type
        >::type
      >::value));
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
      >::value));
#endif

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a));
      assert (n == traits::matrix_size1 (b));

      char uplo = traits::matrix_uplo_tag (a);
      int info; 
      detail::ppsv (uplo, n, traits::matrix_size2 (b),
                    traits::matrix_storage (a), 
                    traits::matrix_storage (b), 
                    traits::leading_dimension (b), 
                    &info);
      return info; 
    }


    /*
     * pptrf() computes the Cholesky factorization of a symmetric
     * or Hermitian positive definite matrix A in packed storage. 
     * The factorization has the form
     *   A = U^T * U or A = U^H * U,  if UPLO = 'U', 
     *   A = L * L^T or A = L * L^H,  if UPLO = 'L',
     * where U is an upper triangular matrix and L is lower triangular.
     */

    namespace detail {

      inline 
      void pptrf (char const uplo, int const n, float* ap, int* info) {
        LAPACK_SPPTRF (&uplo, &n, ap, info);
      }

      inline 
      void pptrf (char const uplo, int const n, double* ap, int* info) {
        LAPACK_DPPTRF (&uplo, &n, ap, info);
      }

      inline 
      void pptrf (char const uplo, int const n, 
                  traits::complex_f* ap, int* info) 
      {
        LAPACK_CPPTRF (&uplo, &n, traits::complex_ptr (ap), info);
      }

      inline 
      void pptrf (char const uplo, int const n, 
                  traits::complex_d* ap, int* info) 
      {
        LAPACK_ZPPTRF (&uplo, &n, traits::complex_ptr (ap), info);
      }

    }

    template <typename SymmMatrA>
    inline
    int pptrf (SymmMatrA& a) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmMatrA>::matrix_structure,
        typename traits::detail::symm_herm_pack_t<
          typename traits::matrix_traits<SymmMatrA>::value_type
        >::type
      >::value));
#endif

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a));
      char uplo = traits::matrix_uplo_tag (a);
      int info; 
      detail::pptrf (uplo, n, traits::matrix_storage (a), &info);
      return info; 
    }


    /*
     * pptrs() solves a system of linear equations A*X = B with 
     * a symmetric or Hermitian positive definite matrix A in packed 
     * storage using the Cholesky factorization computed by pptrf().
     */

    namespace detail {

      inline 
      void pptrs (char const uplo, int const n, int const nrhs,
                  float const* ap, float* b, int const ldb, int* info) 
      {
        LAPACK_SPPTRS (&uplo, &n, &nrhs, ap, b, &ldb, info);
      }

      inline 
      void pptrs (char const uplo, int const n, int const nrhs,
                  double const* ap, double* b, int const ldb, int* info) 
      {
        LAPACK_DPPTRS (&uplo, &n, &nrhs, ap, b, &ldb, info);
      }

      inline 
      void pptrs (char const uplo, int const n, int const nrhs,
                  traits::complex_f const* ap, 
                  traits::complex_f* b, int const ldb, int* info) 
      {
        LAPACK_CPPTRS (&uplo, &n, &nrhs, 
                       traits::complex_ptr (ap), 
                       traits::complex_ptr (b), &ldb, info);
      }

      inline 
      void pptrs (char const uplo, int const n, int const nrhs,
                  traits::complex_d const* ap, 
                  traits::complex_d* b, int const ldb, int* info) 
      {
        LAPACK_ZPPTRS (&uplo, &n, &nrhs, 
                       traits::complex_ptr (ap), 
                       traits::complex_ptr (b), &ldb, info);
      }

    }

    template <typename SymmMatrA, typename MatrB>
    inline
    int pptrs (SymmMatrA const& a, MatrB& b) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmMatrA>::matrix_structure,
        typename traits::detail::symm_herm_pack_t<
          typename traits::matrix_traits<SymmMatrA>::value_type
        >::type
      >::value));
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
      >::value));
#endif

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a));
      assert (n == traits::matrix_size1 (b));
      
      char uplo = traits::matrix_uplo_tag (a);
      int info; 
      detail::pptrs (uplo, n, traits::matrix_size2 (b),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                     traits::matrix_storage (a), 
#else
                     traits::matrix_storage_const (a), 
#endif 
                     traits::matrix_storage (b), 
                     traits::leading_dimension (b), 
                     &info);
      return info; 
    }


    /*
    *  pptri() computes the inverse of a real symmetric positive definite
    *  matrix A using the Cholesky factorization A = U**T*U or A = L*L**T
    *  computed by pptrf().
    */
    
    namespace detail {

      inline 
      void pptri (char const uplo, int const n, float* ap, int* info) {
        LAPACK_SPPTRI (&uplo, &n, ap, info);
      }

      inline 
      void pptri (char const uplo, int const n, double* ap, int* info) {
        LAPACK_DPPTRI (&uplo, &n, ap, info);
      }

      inline 
      void pptri (char const uplo, int const n, 
                  traits::complex_f* ap, int* info) 
      {
        LAPACK_CPPTRI (&uplo, &n, traits::complex_ptr (ap), info);
      }

      inline 
      void pptri (char const uplo, int const n, 
                  traits::complex_d* ap, int* info) 
      {
        LAPACK_ZPPTRI(&uplo, &n, traits::complex_ptr (ap), info);
      }

    }

    template <typename SymmMatrA>
    inline
    int pptri (SymmMatrA& a) { //ISSUE: More correctly, triangular matrix

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmMatrA>::matrix_structure,
        typename traits::detail::symm_herm_pack_t<
          typename traits::matrix_traits<SymmMatrA>::value_type
        >::type
      >::value));
#endif

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a));
      char uplo = traits::matrix_uplo_tag (a);
      int info; 
      detail::pptri (uplo, n, traits::matrix_storage (a), &info);
      return info; 
    }


  }

}}}

#endif 
