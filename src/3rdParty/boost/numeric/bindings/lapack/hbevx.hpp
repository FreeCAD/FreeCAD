/*
 * 
 * Copyright Toon Knapen, Karl Meerbergen & Kresimir Fresl 2003
 * Copyright Thomas Klimpel 2008
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * KF acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_HBEVX_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_HBEVX_HPP

#include <boost/numeric/bindings/traits/type.hpp>
#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/traits/type_traits.hpp>
#include <boost/numeric/bindings/lapack/lapack.h>
#include <boost/numeric/bindings/lapack/workspace.hpp>
#include <boost/numeric/bindings/traits/detail/array.hpp>
#include <boost/numeric/bindings/traits/detail/utils.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
#  include <boost/static_assert.hpp>
#  include <boost/type_traits.hpp>
#endif 


namespace boost { namespace numeric { namespace bindings { 

  namespace lapack {

    ///////////////////////////////////////////////////////////////////
    //
    // Eigendecomposition of a banded Hermitian matrix.
    // 
    ///////////////////////////////////////////////////////////////////

    /* 
     * hbevx() computes the eigenvalues and optionally the associated
     * eigenvectors of a banded Hermitian matrix A. A matrix is Hermitian
     * when herm( A ) == A. When A is real, a Hermitian matrix is also
     * called symmetric.
     *
     * The eigen decomposition is A = U S * herm(U)  where  U  is a
     * unitary matrix and S is a diagonal matrix. The eigenvalues of A
     * are on the main diagonal of S. The eigenvalues are real.
     */

    /*
     * If uplo=='L' only the lower triangular part is stored.
     * If uplo=='U' only the upper triangular part is stored.
     *
     * The matrix is assumed to be stored in LAPACK band format, i.e.
     * matrices are stored columnwise, in a compressed format so that when e.g. uplo=='U'
     * the (i,j) element with j>=i is in position  (i-j) + j * (KD+1) + KD  where KD is the
     * half bandwidth of the matrix. For a triadiagonal matrix, KD=1, for a diagonal matrix
     * KD=0.
     * When uplo=='L', the (i,j) element with j>=i is in position  (i-j) + j * (KD+1).
     *
     * The matrix A is thus a rectangular matrix with KD+1 rows and N columns.
     */ 

    namespace detail {
      inline 
      void hbevx (
        char const jobz, char const range, char const uplo, int const n, int const kd,
        float* ab, int const ldab, float* q, int const ldq,
        float const vl, float const vu, int const il, int const iu,
        float const abstol, int& m,
        float* w, float* z, int const ldz,
        float* work, int* iwork, int* ifail, int& info) 
      {
        LAPACK_SSBEVX (
          &jobz, &range, &uplo, &n, &kd, ab, &ldab, q, &ldq,
          &vl, &vu, &il, &iu, &abstol, &m,
          w, z, &ldz,
          work, iwork, ifail, &info);
      }

      inline 
      void hbevx (
        char const jobz, char const range, char const uplo, int const n, int const kd,
        double* ab, int const ldab, double* q, int const ldq,
        double const vl, double const vu, int const il, int const iu,
        double const abstol, int& m,
        double* w, double* z, int const ldz,
        double* work, int* iwork, int* ifail, int& info) 
      {
        LAPACK_DSBEVX (
          &jobz, &range, &uplo, &n, &kd, ab, &ldab, q, &ldq,
          &vl, &vu, &il, &iu, &abstol, &m,
          w, z, &ldz,
          work, iwork, ifail, &info);
      }

      inline 
      void hbevx (
        char const jobz, char const range, char const uplo, int const n, int const kd,
        traits::complex_f* ab, int const ldab, traits::complex_f* q, int const ldq,
        float const vl, float const vu, int const il, int const iu,
        float const abstol, int& m,
        float* w, traits::complex_f* z, int const ldz,
        traits::complex_f* work, float* rwork, int* iwork, int* ifail, int& info) 
      {
        LAPACK_CHBEVX (
          &jobz, &range, &uplo, &n, &kd, traits::complex_ptr(ab), &ldab,
          traits::complex_ptr(q), &ldq,
          &vl, &vu, &il, &iu, &abstol, &m,
          w, traits::complex_ptr(z), &ldz,
          traits::complex_ptr(work), rwork, iwork, ifail, &info);
      }

      inline 
      void hbevx (
        char const jobz, char const range, char const uplo, int const n, int const kd,
        traits::complex_d* ab, int const ldab, traits::complex_d* q, int const ldq,
        double const vl, double const vu, int const il, int const iu,
        double const abstol, int& m,
        double* w, traits::complex_d* z, int const ldz,
        traits::complex_d* work, double* rwork, int* iwork, int* ifail, int& info) 
      {
        LAPACK_ZHBEVX (
          &jobz, &range, &uplo, &n, &kd, traits::complex_ptr(ab), &ldab,
          traits::complex_ptr(q), &ldq,
          &vl, &vu, &il, &iu, &abstol, &m,
          w, traits::complex_ptr(z), &ldz,
          traits::complex_ptr(work), rwork, iwork, ifail, &info);
      }
    } 


    namespace detail {
      template <int N>
      struct Hbevx{};


      /// Handling of workspace in the case of one workarray.
      template <>
      struct Hbevx< 1 > {
        template <typename T, typename R>
        void operator() (char const jobz, char const range, char const uplo, int const n,
          int const kd, T* ab, int const ldab, T* q, int const ldq,
          R vl, R vu, int const il, int const iu, R abstol, int& m,
          R* w, T* z, int const ldz,
          minimal_workspace, int* ifail, int& info ) const {

          traits::detail::array<T> work( 7*n );
          traits::detail::array<int> iwork( 5*n );
          hbevx( jobz, range, uplo, n, kd, ab, ldab, q, ldq,
            vl, vu, il, iu, abstol, m,
            w, z, ldz,
            traits::vector_storage( work ),
            traits::vector_storage (iwork),
            ifail, info );
        }

        template <typename T, typename R>
        void operator() (char const jobz, char const range, char const uplo, int const n,
          int const kd, T* ab, int const ldab, T* q, int const ldq,
          R vl, R vu, int const il, int const iu, R abstol, int& m,
          R* w, T* z, int const ldz,
          optimal_workspace, int* ifail, int& info ) const {

          traits::detail::array<T> work( 7*n );
          traits::detail::array<int> iwork( 5*n );
          hbevx( jobz, range, uplo, n, kd, ab, ldab, q, ldq,
            vl, vu, il, iu, abstol, m,
            w, z, ldz,
            traits::vector_storage( work ),
            traits::vector_storage (iwork),
            ifail, info );
        }

        template <typename T, typename R, typename W, typename WI>
        void operator() (char const jobz, char const range, char const uplo, int const n,
          int const kd, T* ab, int const ldab, T* q, int const ldq,
          R vl, R vu, int const il, int const iu, R abstol, int& m,
          R* w, T* z, int const ldz,
          std::pair<detail::workspace1<W>, detail::workspace1<WI> > work,
          int* ifail, int& info ) const {

          assert( traits::vector_size( work.first.w_ ) >= 7*n );
          assert( traits::vector_size( work.second.w_ ) >= 5*n );
          hbevx( jobz, range, uplo, n, kd, ab, ldab, q, ldq,
            vl, vu, il, iu, abstol, m,
            w, z, ldz,
            traits::vector_storage( work.first.w_ ),
            traits::vector_storage( work.second.w_ ),
            ifail, info );
        }
      }; // Hbevx< 1 >


      /// Handling of workspace in the case of two workarrays.
      template <>
      struct Hbevx< 2 > {
        template <typename T, typename R>
        void operator() (char const jobz, char const range, char const uplo, int const n,
          int const kd, T* ab, int const ldab, T* q, int const ldq,
          R vl, R vu, int const il, int const iu, R abstol, int& m,
          R* w, T* z, int const ldz,
          minimal_workspace, int* ifail, int& info ) const {

          traits::detail::array<T> work( n );
          traits::detail::array<R> rwork( 7*n );
          traits::detail::array<int> iwork( 5*n );
          hbevx( jobz, range, uplo, n, kd, ab, ldab, q, ldq,
            vl, vu, il, iu, abstol, m,
            w, z, ldz,
            traits::vector_storage( work ),
            traits::vector_storage( rwork ),
            traits::vector_storage (iwork),
            ifail, info );
        }

        template <typename T, typename R>
        void operator() (char const jobz, char const range, char const uplo, int const n,
          int const kd, T* ab, int const ldab, T* q, int const ldq,
          R vl, R vu, int const il, int const iu, R abstol, int& m,
          R* w, T* z, int const ldz,
          optimal_workspace, int* ifail, int& info ) const {

          traits::detail::array<T> work( n );
          traits::detail::array<R> rwork( 7*n );
          traits::detail::array<int> iwork( 5*n );
          hbevx( jobz, range, uplo, n, kd, ab, ldab, q, ldq,
            vl, vu, il, iu, abstol, m,
            w, z, ldz,
            traits::vector_storage( work ),
            traits::vector_storage( rwork ),
            traits::vector_storage (iwork),
            ifail, info );
        }

        template <typename T, typename R, typename W, typename RW, typename WI>
        void operator() (char const jobz, char const range, char const uplo, int const n,
          int const kd, T* ab, int const ldab, T* q, int const ldq,
          R vl, R vu, int const il, int const iu, R abstol, int& m,
          R* w, T* z, int const ldz,
          std::pair<detail::workspace2<W,RW>, detail::workspace1<WI> > work,
          int* ifail, int& info ) const {

          assert( traits::vector_size( work.first.w_ ) >= n );
          assert( traits::vector_size( work.first.wr_ ) >= 7*n );
          assert( traits::vector_size( work.second.w_ ) >= 5*n );
          hbevx( jobz, range, uplo, n, kd, ab, ldab, q, ldq,
            vl, vu, il, iu, abstol, m,
            w, z, ldz,
            traits::vector_storage( work.first.w_ ),
            traits::vector_storage( work.first.wr_ ),
            traits::vector_storage( work.second.w_ ),
            ifail, info );
        }
      }; // Hbevx< 2 >
    } // namespace detail

    template <typename AB, typename Q, typename R, typename Z, typename W, typename IFail, typename Work>
    int hbevx( char const jobz, char const range, AB& ab, Q& q, R vl, R vu, int il, int iu, R abstol, int& m,
      W& w, Z& z, IFail& ifail, Work work ) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<AB>::matrix_structure, 
        traits::hermitian_t
      >::value)); 
#endif 

      typedef typename AB::value_type                            value_type ;

      int const n = traits::matrix_size2 (ab);
      assert (n == traits::matrix_size1 (z)); 
      assert (n == traits::vector_size (w));
      assert (n == traits::vector_size (ifail));
      assert ( jobz=='N' || jobz=='V' );

      int info ; 
      detail::Hbevx< n_workspace_args<value_type>::value >() (jobz, range,
        traits::matrix_uplo_tag( ab ), n,
        traits::matrix_upper_bandwidth(ab),
        traits::matrix_storage (ab), 
        traits::leading_dimension (ab),
        traits::matrix_storage (q), 
        traits::leading_dimension (q),
        vl, vu, il, iu, abstol, m,
        traits::vector_storage (w),
        traits::matrix_storage (z),
        traits::leading_dimension (z),
        work,
        traits::vector_storage (ifail),
        info);
      return info ;
    } // hbevx()
  }

}}}

#endif
