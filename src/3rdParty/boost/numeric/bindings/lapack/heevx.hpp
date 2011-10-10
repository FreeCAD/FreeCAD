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

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_HEEVX_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_HEEVX_HPP

#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/traits/type.hpp>
#include <boost/numeric/bindings/lapack/lapack.h>
#include <boost/numeric/bindings/lapack/workspace.hpp>
#include <boost/numeric/bindings/traits/detail/array.hpp>

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
     * heevx() computes selected eigenvalues and, optionally, eigenvectors
     * of a complex Hermitian matrix A.  Eigenvalues and eigenvectors can
     * be selected by specifying either a range of values or a range of
     * indices for the desired eigenvalues.
     *
     * heevx() computes the eigendecomposition of a N x N matrix
     * A = Q * D * Q',  where Q is a N x N unitary matrix and
     * D is a diagonal matrix. The diagonal element D(i,i) is an
     * eigenvalue of A and Q(:,i) is a corresponding eigenvector.
     * The eigenvalues are stored in ascending order.
     *
     * On return of heevx, A is overwritten, z contains selected eigenvectors from Q
     * and w contains selected eigenvalues from the main diagonal of D.
     *
     * int heevx (char jobz, char range, char uplo, A& a, T vl, T vu, int il, int iu, T abstol, int& m,
     *            W& w, Z& z, IFail& ifail, Work work) {
     *    jobz :  'V' : compute eigenvectors
     *            'N' : do not compute eigenvectors
     *    range : 'A': all eigenvalues will be found.
     *            'V': all eigenvalues in the half-open interval (vl,vu] will be found.
     *            'I': the il-th through iu-th eigenvalues will be found.
     *    uplo :  'U' : only the upper triangular part of A is used on input.
     *            'L' : only the lower triangular part of A is used on input.
     */

    namespace detail {

      inline void heevx (
        char const jobz, char const range, char const uplo, int const n,
        float* a, int const lda,
        float const vl, float const vu, int const il, int const iu,
        float const abstol, int& m,
        float* w, float* z, int const ldz, float* work, int const lwork,
        int* iwork, int* ifail, int& info)
      {
        LAPACK_SSYEVX (
          &jobz, &range, &uplo, &n,
          a, &lda,
          &vl, &vu, &il, &iu,
          &abstol, &m,
          w, z, &ldz, work, &lwork,
          iwork, ifail, &info);
      }

      inline void heevx (
        char const jobz, char const range, char const uplo, int const n,
        double* a, int const lda,
        double const vl, double const vu, int const il, int const iu,
        double const abstol, int& m,
        double* w, double* z, int const ldz, double* work, int const lwork,
        int* iwork, int* ifail, int& info)
      {
        LAPACK_DSYEVX (
          &jobz, &range, &uplo, &n,
          a, &lda,
          &vl, &vu, &il, &iu,
          &abstol, &m,
          w, z, &ldz, work, &lwork,
          iwork, ifail, &info);
      }

      inline void heevx (
        char const jobz, char const range, char const uplo, int const n,
        traits::complex_f* a, int const lda,
        float const vl, float const vu, int const il, int const iu,
        float const abstol, int& m,
        float* w, traits::complex_f* z, int const ldz, traits::complex_f* work, int const lwork,
        float* rwork, int* iwork, int* ifail, int& info)
      {
        LAPACK_CHEEVX (
          &jobz, &range, &uplo, &n,
          traits::complex_ptr(a), &lda,
          &vl, &vu, &il, &iu, &abstol, &m, w,
          traits::complex_ptr(z), &ldz,
          traits::complex_ptr(work), &lwork,
          rwork, iwork, ifail, &info);
      }

      inline void heevx (
        char const jobz, char const range, char const uplo, int const n,
        traits::complex_d* a, int const lda,
        double const vl, double const vu, int const il, int const iu,
        double const abstol, int& m,
        double* w, traits::complex_d* z, int const ldz, traits::complex_d* work, int const lwork,
        double* rwork, int* iwork, int* ifail, int& info)
      {
        LAPACK_ZHEEVX (
          &jobz, &range, &uplo, &n,
          traits::complex_ptr(a), &lda,
          &vl, &vu, &il, &iu, &abstol, &m, w,
          traits::complex_ptr(z), &ldz,
          traits::complex_ptr(work), &lwork,
          rwork, iwork, ifail, &info);
      }
    } // namespace detail

    namespace detail {

      template <int N>
      struct Heevx{};

      /// Handling of workspace in the case of one workarray.
      template <>
      struct Heevx< 1 > {
        // Function that allocates temporary arrays
        template <typename T, typename R>
        void operator() (
          char const jobz, char const range, char const uplo, int const n,
          T* a, int const lda,
          R vl, R vu, int const il, int const iu,
          R abstol, int& m,
          R* w, T* z, int const ldz, minimal_workspace, int* ifail, int& info) {

          traits::detail::array<T> work( 8*n );
          traits::detail::array<int> iwork( 5*n );

          heevx( jobz, range, uplo, n, a, lda, vl, vu, il, iu, abstol, m, w, z, ldz,
            traits::vector_storage (work), traits::vector_size (work),
            traits::vector_storage (iwork),
            ifail, info);
        }
        // Function that allocates temporary arrays
        template <typename T, typename R>
        void operator() (
          char const jobz, char const range, char const uplo, int const n,
          T* a, int const lda,
          R vl, R vu, int const il, int const iu,
          R abstol, int& m,
          R* w, T* z, int const ldz, optimal_workspace, int* ifail, int& info) {

          traits::detail::array<int> iwork( 5*n );

          T workspace_query;
          heevx( jobz, range, uplo, n, a, lda, vl, vu, il, iu, abstol, m, w, z, ldz,
            &workspace_query, -1,
            traits::vector_storage (iwork),
            ifail, info);

          traits::detail::array<T> work( static_cast<int>( workspace_query ) );

          heevx( jobz, range, uplo, n, a, lda, vl, vu, il, iu, abstol, m, w, z, ldz,
            traits::vector_storage (work), traits::vector_size (work),
            traits::vector_storage (iwork),
            ifail, info);
        }
        // Function that uses given workarrays
        template <typename T, typename R, typename W, typename WI>
        void operator() (
          char const jobz, char const range, char const uplo, int const n,
          T* a, int const lda,
          R vl, R vu, int const il, int const iu,
          R abstol, int& m,
          R* w, T* z, int const ldz, std::pair<detail::workspace1<W>, detail::workspace1<WI> > work, int* ifail, int& info) {

          assert (traits::vector_size (work.first.w_) >= 8*n);
          assert (traits::vector_size (work.second.w_) >= 5*n);

          heevx( jobz, range, uplo, n, a, lda, vl, vu, il, iu, abstol, m, w, z, ldz,
            traits::vector_storage (work.first.w_), traits::vector_size (work.first.w_),
            traits::vector_storage (work.second.w_),
            ifail, info);
        }
      };

      /// Handling of workspace in the case of two workarrays.
      template <>
      struct Heevx< 2 > {
        // Function that allocates temporary arrays
        template <typename T, typename R>
        void operator() (
          char const jobz, char const range, char const uplo, int const n,
          T* a, int const lda,
          R vl, R vu, int const il, int const iu,
          R abstol, int& m,
          R* w, T* z, int const ldz, minimal_workspace, int* ifail, int& info) {

          traits::detail::array<T> work( 2*n );
          traits::detail::array<R> rwork( 7*n );
          traits::detail::array<int> iwork( 5*n );

          heevx( jobz, range, uplo, n, a, lda, vl, vu, il, iu, abstol, m, w, z, ldz,
            traits::vector_storage (work), traits::vector_size (work),
            traits::vector_storage (rwork),
            traits::vector_storage (iwork),
            ifail, info);
        }
        // Function that allocates temporary arrays
        template <typename T, typename R>
        void operator() (
          char const jobz, char const range, char const uplo, int const n,
          T* a, int const lda,
          R vl, R vu, int const il, int const iu,
          R abstol, int& m,
          R* w, T* z, int const ldz, optimal_workspace, int* ifail, int& info) {

          traits::detail::array<R> rwork( 7*n );
          traits::detail::array<int> iwork( 5*n );

          T workspace_query;
          heevx( jobz, range, uplo, n, a, lda, vl, vu, il, iu, abstol, m, w, z, ldz,
            &workspace_query, -1,
            traits::vector_storage (rwork),
            traits::vector_storage (iwork),
            ifail, info);

          traits::detail::array<T> work( static_cast<int>( traits::real( workspace_query ) ) );

          heevx( jobz, range, uplo, n, a, lda, vl, vu, il, iu, abstol, m, w, z, ldz,
            traits::vector_storage (work), traits::vector_size (work),
            traits::vector_storage (rwork),
            traits::vector_storage (iwork),
            ifail, info);
        }
        // Function that uses given workarrays
        template <typename T, typename R, typename WC, typename WR, typename WI>
        void operator() (
          char const jobz, char const range, char const uplo, int const n,
          T* a, int const lda,
          R vl, R vu, int const il, int const iu,
          R abstol, int& m,
          R* w, T* z, int const ldz, std::pair<detail::workspace2<WC,WR>, detail::workspace1<WI> > work, int* ifail, int& info) {

          assert (traits::vector_size (work.first.w_) >= 2*n);
          assert (traits::vector_size (work.first.wr_) >= 7*n);
          assert (traits::vector_size (work.second.w_) >= 5*n);

          heevx( jobz, range, uplo, n, a, lda, vl, vu, il, iu, abstol, m, w, z, ldz,
            traits::vector_storage (work.first.w_), traits::vector_size (work.first.w_),
            traits::vector_storage (work.first.wr_),
            traits::vector_storage (work.second.w_),
            ifail, info);
        }
      };
    } // namespace detail

    template <typename A, typename T, typename W, typename Z, typename IFail, typename Work>
    inline int heevx (
      char jobz, char range, A& a, T vl, T vu, int il, int iu, T abstol, int& m,
      W& w, Z& z, IFail& ifail, Work work = optimal_workspace() ) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      typedef typename A::value_type                               value_type ;
      typedef typename traits::type_traits< value_type >::real_type real_type ;
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<A>::matrix_structure, 
        traits::hermitian_t
      >::value || (boost::is_same<
        typename traits::matrix_traits<A>::matrix_structure,
        traits::symmetric_t
      >::value && boost::is_same<value_type, real_type>::value)));
#endif

      int const n = traits::matrix_size1 (a);
      assert (traits::matrix_size2 (a) == n);
      assert (traits::vector_size (w) == n);
      assert (traits::vector_size (ifail) == n);
      assert ( range=='A' || range=='V' || range=='I' );
      char uplo = traits::matrix_uplo_tag (a);
      assert ( uplo=='U' || uplo=='L' );
      assert ( jobz=='N' || jobz=='V' );

      int info; 
      detail::Heevx< n_workspace_args<typename A::value_type>::value >() (
        jobz, range, uplo, n,
        traits::matrix_storage (a),
        traits::leading_dimension (a),
        vl, vu, il, iu, abstol, m,
        traits::vector_storage (w),
        traits::matrix_storage (z),
        traits::leading_dimension (z),
        work,
        traits::vector_storage (ifail),
        info);
      return info;
    }
  }

}}}

#endif
