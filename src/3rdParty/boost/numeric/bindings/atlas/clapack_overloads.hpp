/*
 * 
 * Copyright (c) Kresimir Fresl 2002 
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Author acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_CLAPACK_OVERLOADS_HPP
#define BOOST_NUMERIC_BINDINGS_CLAPACK_OVERLOADS_HPP

#include <boost/numeric/bindings/atlas/clapack_inc.hpp>
#include <boost/numeric/bindings/traits/type.hpp>


namespace boost { namespace numeric { namespace bindings { 

  namespace atlas { namespace detail {

    //
    // general system of linear equations A * X = B
    //

    // 'driver' function -- factor and solve
    inline 
    int gesv (CBLAS_ORDER const Order, 
              int const N, int const NRHS,
              float* A, int const lda, int* ipiv, 
              float* B, int const ldb) 
    {
      return clapack_sgesv (Order, N, NRHS, A, lda, ipiv, B, ldb);
    }
    
    inline 
    int gesv (CBLAS_ORDER const Order, 
              int const N, int const NRHS,
              double* A, int const lda, int* ipiv, 
              double* B, int const ldb) 
    {
      return clapack_dgesv (Order, N, NRHS, A, lda, ipiv, B, ldb);
    }
    
    inline 
    int gesv (CBLAS_ORDER const Order, 
              int const N, int const NRHS,
              traits::complex_f* A, int const lda, int* ipiv, 
              traits::complex_f* B, int const ldb) 
    {
      return clapack_cgesv (Order, N, NRHS, 
                            static_cast<void*> (A), lda, ipiv, 
                            static_cast<void*> (B), ldb);
    }
    
    inline 
    int gesv (CBLAS_ORDER const Order, 
              int const N, int const NRHS,
              traits::complex_d* A, int const lda, int* ipiv, 
              traits::complex_d* B, int const ldb) 
    {
      return clapack_zgesv (Order, N, NRHS, 
                            static_cast<void*> (A), lda, ipiv, 
                            static_cast<void*> (B), ldb);
    }
    
    // LU factorization 
    inline 
    int getrf (CBLAS_ORDER const Order, 
               int const M, int const N,
               float* A, int const lda, int* ipiv)
    {
      return clapack_sgetrf (Order, M, N, A, lda, ipiv);
    }
    
    inline 
    int getrf (CBLAS_ORDER const Order, 
               int const M, int const N,
               double* A, int const lda, int* ipiv)
    {
      return clapack_dgetrf (Order, M, N, A, lda, ipiv);
    }
    
    inline 
    int getrf (CBLAS_ORDER const Order, 
               int const M, int const N,
               traits::complex_f* A, int const lda, int* ipiv)
    {
      return clapack_cgetrf (Order, M, N, static_cast<void*> (A), lda, ipiv); 
    }
    
    inline 
    int getrf (CBLAS_ORDER const Order, 
               int const M, int const N,
               traits::complex_d* A, int const lda, int* ipiv)
    {
      return clapack_zgetrf (Order, M, N, static_cast<void*> (A), lda, ipiv); 
    }

    // solve (using factorization computed by getrf()) 
    inline 
    int getrs (CBLAS_ORDER const Order, CBLAS_TRANSPOSE const Trans, 
               int const N, int const NRHS,
               float const* A, int const lda, int const* ipiv, 
               float* B, int const ldb) 
    {
      return clapack_sgetrs (Order, Trans, N, NRHS, A, lda, ipiv, B, ldb);
    }
    
    inline 
    int getrs (CBLAS_ORDER const Order, CBLAS_TRANSPOSE const Trans, 
               int const N, int const NRHS,
               double const* A, int const lda, int const* ipiv, 
               double* B, int const ldb) 
    {
      return clapack_dgetrs (Order, Trans, N, NRHS, A, lda, ipiv, B, ldb);
    }
    
    inline 
    int getrs (CBLAS_ORDER const Order, CBLAS_TRANSPOSE const Trans, 
               int const N, int const NRHS,
               traits::complex_f const* A, int const lda, 
               int const* ipiv, 
               traits::complex_f* B, int const ldb) 
    {
      return clapack_cgetrs (Order, Trans, N, NRHS, 
                             static_cast<void const*> (A), lda, ipiv, 
                             static_cast<void*> (B), ldb);
    }
    
    inline 
    int getrs (CBLAS_ORDER const Order, CBLAS_TRANSPOSE const Trans, 
               int const N, int const NRHS,
               traits::complex_d const* A, int const lda, 
               int const* ipiv, 
               traits::complex_d* B, int const ldb) 
    {
      return clapack_zgetrs (Order, Trans, N, NRHS, 
                             static_cast<void const*> (A), lda, ipiv, 
                             static_cast<void*> (B), ldb);
    }

    // invert (using factorization computed by getrf()) 
    inline 
    int getri (CBLAS_ORDER const Order, 
               int const N, float* A, int const lda,
               int const* ipiv) 
    {
      return clapack_sgetri (Order, N, A, lda, ipiv);
    }

    inline 
    int getri (CBLAS_ORDER const Order, 
               int const N, double* A, int const lda,
               int const* ipiv) 
    {
      return clapack_dgetri (Order, N, A, lda, ipiv);
    }

    inline 
    int getri (CBLAS_ORDER const Order, 
               int const N, traits::complex_f* A, int const lda,
               int const* ipiv) 
    {
      return clapack_cgetri (Order, N, static_cast<void*> (A), lda, ipiv);
    }

    inline 
    int getri (CBLAS_ORDER const Order, 
               int const N, traits::complex_d* A, int const lda,
               int const* ipiv) 
    {
      return clapack_zgetri (Order, N, static_cast<void*> (A), lda, ipiv);
    }


    //
    // system of linear equations A * X = B
    // with A symmetric positive definite matrix
    //

    // 'driver' function -- factor and solve
    inline 
    int posv (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
              int const N, int const NRHS,
              float* A, int const lda, float* B, int const ldb) 
    {
      return clapack_sposv (Order, Uplo, N, NRHS, A, lda, B, ldb);
    }
    
    inline 
    int posv (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
              int const N, int const NRHS,
              double* A, int const lda, double* B, int const ldb) 
    {
      return clapack_dposv (Order, Uplo, N, NRHS, A, lda, B, ldb);
    }
    
    inline 
    int posv (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
              int const N, int const NRHS,
              traits::complex_f* A, int const lda, 
              traits::complex_f* B, int const ldb) 
    {
      return clapack_cposv (Order, Uplo, N, NRHS, 
                            static_cast<void*> (A), lda, 
                            static_cast<void*> (B), ldb);
    }
    
    inline 
    int posv (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
              int const N, int const NRHS,
              traits::complex_d* A, int const lda, 
              traits::complex_d* B, int const ldb) 
    {
      return clapack_zposv (Order, Uplo, N, NRHS, 
                            static_cast<void*> (A), lda, 
                            static_cast<void*> (B), ldb);
    }

    // Cholesky factorization
    inline 
    int potrf (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, float* A, int const lda) 
    {
      return clapack_spotrf (Order, Uplo, N, A, lda);
    }
    
    inline 
    int potrf (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, double* A, int const lda) 
    {
      return clapack_dpotrf (Order, Uplo, N, A, lda);
    }
    
    inline 
    int potrf (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, traits::complex_f* A, int const lda) 
    {
      return clapack_cpotrf (Order, Uplo, N, static_cast<void*> (A), lda);
    }
    
    inline 
    int potrf (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, traits::complex_d* A, int const lda) 
    {
      return clapack_zpotrf (Order, Uplo, N, static_cast<void*> (A), lda);
    }
    
    // solve (using factorization computed by potrf()) 
    inline 
    int potrs (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, int const NRHS,
               float const* A, int const lda, float* B, int const ldb) 
    {
      return clapack_spotrs (Order, Uplo, N, NRHS, A, lda, B, ldb);
    }

    inline 
    int potrs (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, int const NRHS,
               double const* A, int const lda, double* B, int const ldb) 
    {
      return clapack_dpotrs (Order, Uplo, N, NRHS, A, lda, B, ldb);
    }
   
    inline 
    int potrs (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, int const NRHS,
               traits::complex_f const* A, int const lda, 
               traits::complex_f* B, int const ldb) 
    {
      return clapack_cpotrs (Order, Uplo, N, NRHS, 
                             static_cast<void const*> (A), lda, 
                             static_cast<void*> (B), ldb);
    }
   
    inline 
    int potrs (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, int const NRHS,
               traits::complex_d const* A, int const lda, 
               traits::complex_d* B, int const ldb) 
    {
      return clapack_zpotrs (Order, Uplo, N, NRHS, 
                             static_cast<void const*> (A), lda, 
                             static_cast<void*> (B), ldb);
    }

#ifdef BOOST_NUMERIC_BINDINGS_ATLAS_POTRF_BUG 
    // .. ATLAS bug with row major hermitian matrices 
    // .... symmetric matrices are OK, but to simplify generic potrs() ... 
    inline 
    int potrs_bug (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
                   int const N, int const NRHS,
                   float const* A, int const lda, float* B, int const ldb) 
    {
      return clapack_spotrs (Order, Uplo, N, NRHS, A, lda, B, ldb);
    }

    inline 
    int potrs_bug (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
                   int const N, int const NRHS,
                   double const* A, int const lda, double* B, int const ldb) 
    {
      return clapack_dpotrs (Order, Uplo, N, NRHS, A, lda, B, ldb);
    }
   
    inline 
    int potrs_bug (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
                   int const N, int const NRHS,
                   traits::complex_f const* A, int const lda, 
                   traits::complex_f* B, int const ldb) 
    {
      int sz = N * lda; 
      traits::complex_f* A1 = new traits::complex_f[sz]; 
      for (int i = 0; i < sz; ++i) 
        A1[i] = std::conj (A[i]); 
      int r = clapack_cpotrs (Order, Uplo, N, NRHS, 
                              static_cast<void const*> (A1), lda, 
                              static_cast<void*> (B), ldb);
      delete[] A1; 
      return r; 
    }
   
    inline 
    int potrs_bug (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
                   int const N, int const NRHS,
                   traits::complex_d const* A, int const lda, 
                   traits::complex_d* B, int const ldb) 
    {
      int sz = N * lda; 
      traits::complex_d* A1 = new traits::complex_d[sz]; 
      for (int i = 0; i < sz; ++i) 
        A1[i] = std::conj (A[i]); 
      int r = clapack_zpotrs (Order, Uplo, N, NRHS, 
                              static_cast<void const*> (A1), lda, 
                              static_cast<void*> (B), ldb);
      delete[] A1; 
      return r; 
    }
#endif // BOOST_NUMERIC_BINDINGS_ATLAS_POTRF_BUG 

    // invert (using factorization computed by potrf()) 
    inline 
    int potri (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, float* A, int const lda) 
    {
      return clapack_spotri (Order, Uplo, N, A, lda);
    }

    inline 
    int potri (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, double* A, int const lda) 
    {
      return clapack_dpotri (Order, Uplo, N, A, lda);
    }

    inline 
    int potri (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, traits::complex_f* A, int const lda) 
    {
      return clapack_cpotri (Order, Uplo, N, static_cast<void*> (A), lda);
    }

    inline 
    int potri (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, traits::complex_d* A, int const lda) 
    {
      return clapack_zpotri (Order, Uplo, N, static_cast<void*> (A), lda);
    }


  }} // namepaces detail & atlas

}}} 


#endif // BOOST_NUMERIC_BINDINGS_CLAPACK_OVERLOADS_HPP
