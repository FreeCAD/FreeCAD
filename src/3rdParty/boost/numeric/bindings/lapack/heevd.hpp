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

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_HEEVD_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_HEEVD_HPP

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
     * heevd() computes all eigenvalues and, optionally, eigenvectors of a
     * complex Hermitian matrix A.  If eigenvectors are desired, it uses a
     * divide and conquer algorithm.
     *
     * heevd() computes the eigendecomposition of a N x N matrix
     * A = Q * D * Q',  where Q is a N x N unitary matrix and
     * D is a diagonal matrix. The diagonal element D(i,i) is an
     * eigenvalue of A and Q(:,i) is a corresponding eigenvector.
     * The eigenvalues are stored in ascending order.
     *
     * On return of heevd, A is overwritten with the eigenvectors from Q
     * and w contains the eigenvalues from the main diagonal of D.
     *
     * int heevd (char jobz, char uplo, A& a, W& w, Work work) {
     *    jobz :  'V' : compute eigenvectors
     *            'N' : do not compute eigenvectors
     *    uplo :  'U' : only the upper triangular part of A is used on input.
     *            'L' : only the lower triangular part of A is used on input.
     */

    namespace detail {

      inline void heevd (
        char const jobz, char const uplo, int const n,
        float* a, int const lda,
        float* w, float* work, int const lwork,
        int* iwork, int const liwork, int& info)
      {
        LAPACK_SSYEVD (
          &jobz, &uplo, &n,
          a, &lda,
          w, work, &lwork,
          iwork, &liwork, &info);
      }

      inline void heevd (
        char const jobz, char const uplo, int const n,
        double* a, int const lda,
        double* w, double* work, int const lwork,
        int* iwork, int const liwork, int& info)
      {
        LAPACK_DSYEVD (
          &jobz, &uplo, &n,
          a, &lda,
          w, work, &lwork,
          iwork, &liwork, &info);
      }

      inline void heevd (
        char const jobz, char const uplo, int const n,
        traits::complex_f* a, int const lda,
        float* w, traits::complex_f* work, int const lwork,
        float* rwork, int const lrwork, int* iwork, int const liwork, int& info)
      {
        LAPACK_CHEEVD (
          &jobz, &uplo, &n,
          traits::complex_ptr(a), &lda,
          w,
          traits::complex_ptr(work), &lwork,
          rwork, &lrwork, iwork, &liwork, &info);
      }

      inline void heevd (
        char const jobz, char const uplo, int const n,
        traits::complex_d* a, int const lda,
        double* w, traits::complex_d* work, int const lwork,
        double* rwork, int const lrwork, int* iwork, int const liwork, int& info)
      {
        LAPACK_ZHEEVD (
          &jobz, &uplo, &n,
          traits::complex_ptr(a), &lda,
          w,
          traits::complex_ptr(work), &lwork,
          rwork, &lrwork, iwork, &liwork, &info);
      }
    } // namespace detail

    namespace detail {

      template <int N>
      struct Heevd{};

      /// Handling of workspace in the case of one workarray.
      template <>
      struct Heevd< 1 > {
        // Function that allocates temporary arrays
        template <typename T, typename R>
        void operator() (
          char const jobz, char const uplo, int const n,
          T* a, int const lda,
          R* w, minimal_workspace, int& info) {

          traits::detail::array<T> work( jobz=='N' ? 1+2*n : 1+6*n+2*n*n );
          traits::detail::array<int> iwork( jobz=='N' ? 1 : 3+5*n );

          heevd( jobz, uplo, n, a, lda, w,
            traits::vector_storage (work), traits::vector_size (work),
            traits::vector_storage (iwork), traits::vector_size (iwork),
            info);
        }
        // Function that allocates temporary arrays
        template <typename T, typename R>
        void operator() (
          char const jobz, char const uplo, int const n,
          T* a, int const lda,
          R* w, optimal_workspace, int& info) {

          traits::detail::array<int> iwork( jobz=='N' ? 1 : 3+5*n );

          T workspace_query;
          heevd( jobz, uplo, n, a, lda, w,
            &workspace_query, -1,
            traits::vector_storage (iwork), traits::vector_size (iwork),
            info);

          traits::detail::array<T> work( static_cast<int>( workspace_query ) );

          heevd( jobz, uplo, n, a, lda, w,
            traits::vector_storage (work), traits::vector_size (work),
            traits::vector_storage (iwork), traits::vector_size (iwork),
            info);
        }
        // Function that uses given workarrays
        template <typename T, typename R, typename W, typename WI>
        void operator() (
          char const jobz, char const uplo, int const n,
          T* a, int const lda,
          R* w, std::pair<detail::workspace1<W>, detail::workspace1<WI> > work, int& info) {

          assert (traits::vector_size (work.first.w_) >= jobz=='N' ? 1+2*n : 1+6*n+2*n*n);
          assert (traits::vector_size (work.second.w_) >= jobz=='N' ? 1 : 3+5*n);

          heevd( jobz, uplo, n, a, lda, w,
            traits::vector_storage (work.first.w_), traits::vector_size (work.first.w_),
            traits::vector_storage (work.second.w_), traits::vector_size (work.second.w_),
            info);
        }
      };

      /// Handling of workspace in the case of two workarrays.
      template <>
      struct Heevd< 2 > {
        // Function that allocates temporary arrays
        template <typename T, typename R>
        void operator() (
          char const jobz, char const uplo, int const n,
          T* a, int const lda,
          R* w, minimal_workspace, int& info) {

          traits::detail::array<T> work( jobz=='N' ? 1+n : 2*n+n*n );
          traits::detail::array<R> rwork( jobz=='N' ? n : 1+5*n+2*n*n );
          traits::detail::array<int> iwork( jobz=='N' ? 1 : 3+5*n );

          heevd( jobz, uplo, n, a, lda, w,
            traits::vector_storage (work), traits::vector_size (work),
            traits::vector_storage (rwork), traits::vector_size (rwork),
            traits::vector_storage (iwork), traits::vector_size (iwork),
            info);
        }
        // Function that allocates temporary arrays
        template <typename T, typename R>
        void operator() (
          char const jobz, char const uplo, int const n,
          T* a, int const lda,
          R* w, optimal_workspace, int& info) {

          traits::detail::array<R> rwork( jobz=='N' ? n : 1+5*n+2*n*n );
          traits::detail::array<int> iwork( jobz=='N' ? 1 : 3+5*n );

          T workspace_query;
          heevd( jobz, uplo, n, a, lda, w,
            &workspace_query, -1,
            traits::vector_storage (rwork), traits::vector_size (rwork),
            traits::vector_storage (iwork), traits::vector_size (iwork),
            info);

          traits::detail::array<T> work( static_cast<int>( traits::real( workspace_query ) ) );

          heevd( jobz, uplo, n, a, lda, w,
            traits::vector_storage (work), traits::vector_size (work),
            traits::vector_storage (rwork), traits::vector_size (rwork),
            traits::vector_storage (iwork), traits::vector_size (iwork),
            info);
        }
        // Function that uses given workarrays
        template <typename T, typename R, typename WC, typename WR, typename WI>
        void operator() (
          char const jobz, char const uplo, int const n,
          T* a, int const lda,
          R* w, std::pair<detail::workspace2<WC,WR>, detail::workspace1<WI> > work, int& info) {

          assert (traits::vector_size (work.first.w_) >= jobz=='N' ? 1+n : 2*n+n*n);
          assert (traits::vector_size (work.first.wr_) >= jobz=='N' ? n : 1+5*n+2*n*n);
          assert (traits::vector_size (work.second.w_) >= jobz=='N' ? 1 : 3+5*n);

          heevd( jobz, uplo, n, a, lda, w,
            traits::vector_storage (work.first.w_), traits::vector_size (work.first.w_),
            traits::vector_storage (work.first.wr_), traits::vector_size (work.first.wr_),
            traits::vector_storage (work.second.w_), traits::vector_size (work.second.w_),
            info);
        }
      };
    } // namespace detail

    template <typename A, typename W, typename Work>
    inline int heevd (
      char jobz, char uplo, A& a,
      W& w, Work work = optimal_workspace() ) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<A>::matrix_structure, 
        traits::general_t
      >::value));
#endif

      int const n = traits::matrix_size1 (a);
      assert (traits::matrix_size2 (a) == n);
      assert (traits::vector_size (w) == n);
      assert ( uplo=='U' || uplo=='L' );
      assert ( jobz=='N' || jobz=='V' );

      int info; 
      detail::Heevd< n_workspace_args<typename A::value_type>::value >() (
        jobz, uplo, n,
        traits::matrix_storage (a),
        traits::leading_dimension (a),
        traits::vector_storage (w),
        work,
        info);
      return info;
    }
  }

}}}

#endif
