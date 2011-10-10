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

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_HBEV_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_HBEV_HPP

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
     * hbev() computes the eigenvalues and optionally the associated
     * eigenvectors of a banded Hermitian matrix A. A matrix is Hermitian
     * when herm( A ) == A. When A is real, a Hermitian matrix is also
     * called symmetric.
     *
     * The eigen decomposition is A = U S * herm(U)  where  U  is a
     * unitary matrix and S is a diagonal matrix. The eigenvalues of A
     * are on the main diagonal of S. The eigenvalues are real.
     *
     * Workspace is organized following the arguments in the calling sequence.
     *  optimal_workspace() : for optimizing use of blas 3 kernels
     *  minimal_workspace() : minimum size of workarrays, but does not allow for optimization
     *                        of blas 3 kernels
     *  workspace( work ) for real matrices where work is a real array with
     *                    vector_size( work ) >= 3*matrix_size1( a ) - 2
     *  workspace( work, rwork ) for complex matrices where work is a complex
     *                           array with vector_size( work ) >= matrix_size1( a )
     *                           and rwork is a real array with
     *                           vector_size( rwork ) >= 3 * matrix_size1( a ) - 2.
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
      void hbev (char const jobz, char const uplo, int const n, int const kd,
                 float* ab, int const ldab, float* w, float* z, int const ldz,
                 float* work, int& info) 
      {
	      //for (int i=0; i<n*kd; ++i) std::cout << *(ab+i) << " " ;
	      //std::cout << "\n" ;
        LAPACK_SSBEV (&jobz, &uplo, &n, &kd, ab, &ldab, w, z, &ldz,
                      work, &info);
      }

      inline 
      void hbev (char const jobz, char const uplo, int const n, int const kd,
                 double* ab, int const ldab, double* w, double* z, int const ldz,
                 double* work, int& info) 
      {
        LAPACK_DSBEV (&jobz, &uplo, &n, &kd, ab, &ldab, w, z, &ldz,
                      work, &info);
      }

      inline 
      void hbev (char const jobz, char const uplo, int const n, int const kd,
                 traits::complex_f* ab, int const ldab, float* w,
                 traits::complex_f* z, int const ldz,
                 traits::complex_f* work, float* rwork, int& info) 
      {
        LAPACK_CHBEV (&jobz, &uplo, &n, &kd, traits::complex_ptr(ab), &ldab,
                      w, traits::complex_ptr(z), &ldz,
                      traits::complex_ptr(work), rwork, &info);
      }

      inline 
      void hbev (char const jobz, char const uplo, int const n, int const kd,
                 traits::complex_d* ab, int const ldab, double* w,
                 traits::complex_d* z, int const ldz,
                 traits::complex_d* work, double* rwork, int& info) 
      {
        LAPACK_ZHBEV (&jobz, &uplo, &n, &kd, traits::complex_ptr(ab), &ldab,
                      w, traits::complex_ptr(z), &ldz,
                      traits::complex_ptr(work), rwork, &info);
      }
    } 


    namespace detail {
       template <int N>
       struct Hbev{};


       /// Handling of workspace in the case of one workarray.
       template <>
       struct Hbev< 1 > {
          template <typename T, typename R>
          void operator() (char const jobz, char const uplo, int const n,
                           int const kd, T* ab, int const ldab, R* w, T* z,
                           int const ldz, minimal_workspace , int& info ) const {
             traits::detail::array<T> work( 3*n-2 );
             hbev( jobz, uplo, n, kd, ab, ldab, w, z, ldz,
                   traits::vector_storage( work ),
                   info );
          }

          template <typename T, typename R>
          void operator() (char const jobz, char const uplo, int const n,
                           int const kd, T* ab, int const ldab, R* w, T* z,
                           int const ldz, optimal_workspace , int& info ) const {
             traits::detail::array<T> work( 3*n-2 );

             hbev( jobz, uplo, n, kd, ab, ldab, w, z, ldz,
                   traits::vector_storage( work ),
                   info );
          }

          template <typename T, typename R, typename W>
          void operator() (char const jobz, char const uplo, int const n,
                           int const kd, T* ab, int const ldab, R* w, T* z,
                           int const ldz, detail::workspace1<W> work,
                           int& info ) const {
             assert( traits::vector_size( work.w_ ) >= 3*n-2 );

             hbev( jobz, uplo, n, kd, ab, ldab, w, z, ldz,
                   traits::vector_storage( work.w_ ),
                   info );
          }
       }; // Hbev< 1 >


       /// Handling of workspace in the case of two workarrays.
       template <>
       struct Hbev< 2 > {
          template <typename T, typename R>
          void operator() (char const jobz, char const uplo, int const n,
                           int const kd, T* ab, int const ldab, R* w, T* z,
                           int const ldz, minimal_workspace , int& info ) const {
             traits::detail::array<T> work( n );
             traits::detail::array<R> rwork( 3*n-2 );

             hbev( jobz, uplo, n, kd, ab, ldab, w, z, ldz,
                   traits::vector_storage( work ),
                   traits::vector_storage( rwork ),
                   info );
          }

          template <typename T, typename R>
          void operator() (char const jobz, char const uplo, int const n,
                           int const kd, T* ab, int const ldab, R* w, T* z,
                           int const ldz, optimal_workspace , int& info ) const {
             traits::detail::array<T> work( n );
             traits::detail::array<R> rwork( 3*n-2 );

             hbev( jobz, uplo, n, kd, ab, ldab, w, z, ldz,
                   traits::vector_storage( work ),
                   traits::vector_storage( rwork ),
                   info );
          }

          template <typename T, typename R, typename W, typename RW>
          void operator() (char const jobz, char const uplo, int const n,
                           int const kd, T* ab, int const ldab, R* w, T* z,
                           int const ldz, detail::workspace2<W,RW> work,
                           int& info ) const {
             assert( traits::vector_size( work.wr_ ) >= 3*n-2 );
             assert( traits::vector_size( work.w_ ) >= n );

             hbev( jobz, uplo, n, kd, ab, ldab, w, z, ldz,
                   traits::vector_storage( work.w_ ),
                   traits::vector_storage( work.wr_ ),
                   info );
          }
       }; // Hbev< 2 >
    


       /// Compute eigendecomposition of the banded Hermitian matrix ab.
       /// if jobz=='N' only the eigenvalues are computed.
       /// if jobz=='V' compute the eigenvalues a and the eigenvectors.
       ///
       /// Workspace is organized following the arguments in the calling sequence.
       ///  optimal_workspace() : for optimizing use of blas 3 kernels
       ///  minimal_workspace() : minimum size of workarrays, but does not allow for optimization
       ///                       of blas 3 kernels
       ///  workspace( work ) for real matrices where work is a real array with
       ///                    vector_size( work ) >= 3*matrix_size1( a )-2
       ///  workspace( work, rwork ) for complex matrices where work is a complex
       ///                           array with vector_size( work ) >= matrix_size1( a )
       ///                           and rwork is a real array with
       ///                           vector_size( rwork ) >= 3*matrix_size1( a )-2.
       template <typename AB, typename Z, typename W, typename Work>
       int hbev( char const jobz, AB& ab, W& w, Z& z, Work work ) {
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
         assert ( jobz=='N' || jobz=='V' );

         int info ; 
         detail::Hbev< n_workspace_args<value_type>::value >() (jobz,
                       traits::matrix_uplo_tag( ab ), n,
                       traits::matrix_upper_bandwidth(ab),
                       traits::matrix_storage (ab), 
                       traits::leading_dimension (ab),
                       traits::vector_storage (w),
                       traits::matrix_storage (z),
                       traits::leading_dimension (z),
                       work, info);
	 return info ;
       } // hbev()
       
       } // namespace detail


       /// Compute eigendecomposition without eigenvectors
       template <typename AB, typename W, typename Work>
       inline
       int hbev (AB& ab, W& w, Work work) {
          return detail::hbev( 'N', ab, w, ab, work );
       } // hbev()


       /// Compute eigendecomposition with eigenvectors
       template <typename AB, typename W, typename Z, typename Work>
       inline
       int hbev (AB& ab, W& w, Z& z, Work work) {
         BOOST_STATIC_ASSERT((boost::is_same<
           typename traits::matrix_traits<Z>::matrix_structure, 
           traits::general_t
         >::value)); 
         int const n = traits::matrix_size2 (ab);
          assert (n == traits::matrix_size1 (z)); 
          assert (n == traits::matrix_size2 (z)); 
          return detail::hbev( 'V', ab, w, z, work );
       } // hbev()

  }

}}}

#endif 
