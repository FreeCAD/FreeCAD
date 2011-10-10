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

#ifndef BOOST_NUMERIC_BINDINGS_CBLAS3_OVERLOADS_HPP
#define BOOST_NUMERIC_BINDINGS_CBLAS3_OVERLOADS_HPP

#include <complex> 
#include <boost/numeric/bindings/atlas/cblas_inc.hpp>
#include <boost/numeric/bindings/traits/type.hpp>


namespace boost { namespace numeric { namespace bindings { 

  namespace atlas { namespace detail {

    // C <- alpha * op (A) * op (B) + beta * C 
  
    inline 
    void gemm (CBLAS_ORDER const Order, 
               CBLAS_TRANSPOSE const TransA, CBLAS_TRANSPOSE const TransB, 
               int const M, int const N, int const K, 
               float const alpha, float const* A, int const lda,
               float const* B, int const ldb, 
               float const beta, float* C, int const ldc) 
    {
      cblas_sgemm (Order, TransA, TransB, M, N, K, 
                   alpha, A, lda, 
                   B, ldb,
                   beta, C, ldc); 
    }

    inline 
    void gemm (CBLAS_ORDER const Order, 
               CBLAS_TRANSPOSE const TransA, CBLAS_TRANSPOSE const TransB, 
               int const M, int const N, int const K, 
               double const alpha, double const* A, int const lda,
               double const* B, int const ldb, 
               double const beta, double* C, int const ldc) 
    {
      cblas_dgemm (Order, TransA, TransB, M, N, K, 
                   alpha, A, lda, 
                   B, ldb,
                   beta, C, ldc); 
    }

    inline 
    void gemm (CBLAS_ORDER const Order, 
               CBLAS_TRANSPOSE const TransA, CBLAS_TRANSPOSE const TransB, 
               int const M, int const N, int const K, 
               traits::complex_f const& alpha, 
               traits::complex_f const* A, int const lda,
               traits::complex_f const* B, int const ldb, 
               traits::complex_f const& beta, 
               traits::complex_f* C, int const ldc) 
    {
      cblas_cgemm (Order, TransA, TransB, M, N, K, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (A), lda, 
                   static_cast<void const*> (B), ldb,
                   static_cast<void const*> (&beta), 
                   static_cast<void*> (C), ldc); 
    }
    
    inline 
    void gemm (CBLAS_ORDER const Order, 
               CBLAS_TRANSPOSE const TransA, CBLAS_TRANSPOSE const TransB, 
               int const M, int const N, int const K, 
               traits::complex_d const& alpha, 
               traits::complex_d const* A, int const lda,
               traits::complex_d const* B, int const ldb, 
               traits::complex_d const& beta, 
               traits::complex_d* C, int const ldc) 
    {
      cblas_zgemm (Order, TransA, TransB, M, N, K, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (A), lda, 
                   static_cast<void const*> (B), ldb,
                   static_cast<void const*> (&beta), 
                   static_cast<void*> (C), ldc); 
    }

    
    // C <- alpha * A * B + beta * C 
    // C <- alpha * B * A + beta * C 
    // A == A^T

    inline 
    void symm (CBLAS_ORDER const Order, CBLAS_SIDE const Side,
               CBLAS_UPLO const Uplo, int const M, int const N, 
               float const alpha, float const* A, int const lda,
               float const* B, int const ldb, 
               float const beta, float* C, int const ldc) 
    {
      cblas_ssymm (Order, Side, Uplo, M, N, 
                   alpha, A, lda, 
                   B, ldb,
                   beta, C, ldc); 
    }
  
    inline 
    void symm (CBLAS_ORDER const Order, CBLAS_SIDE const Side,
               CBLAS_UPLO const Uplo, int const M, int const N, 
               double const alpha, double const* A, int const lda,
               double const* B, int const ldb, 
               double const beta, double* C, int const ldc) 
    {
      cblas_dsymm (Order, Side, Uplo, M, N, 
                   alpha, A, lda, 
                   B, ldb,
                   beta, C, ldc); 
    }
  
    inline 
    void symm (CBLAS_ORDER const Order, CBLAS_SIDE const Side,
               CBLAS_UPLO const Uplo, int const M, int const N, 
               traits::complex_f const& alpha, 
               traits::complex_f const* A, int const lda,
               traits::complex_f const* B, int const ldb, 
               traits::complex_f const& beta, 
               traits::complex_f* C, int const ldc) 
    {
      cblas_csymm (Order, Side, Uplo, M, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (A), lda, 
                   static_cast<void const*> (B), ldb,
                   static_cast<void const*> (&beta), 
                   static_cast<void*> (C), ldc); 
    }
  
    inline 
    void symm (CBLAS_ORDER const Order, CBLAS_SIDE const Side,
               CBLAS_UPLO const Uplo, int const M, int const N, 
               traits::complex_d const& alpha, 
               traits::complex_d const* A, int const lda,
               traits::complex_d const* B, int const ldb, 
               traits::complex_d const& beta, 
               traits::complex_d* C, int const ldc) 
    {
      cblas_zsymm (Order, Side, Uplo, M, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (A), lda, 
                   static_cast<void const*> (B), ldb,
                   static_cast<void const*> (&beta), 
                   static_cast<void*> (C), ldc); 
    }
  

    // C <- alpha * A * B + beta * C 
    // C <- alpha * B * A + beta * C 
    // A == A^H
  
    inline 
    void hemm (CBLAS_ORDER const Order, CBLAS_SIDE const Side,
               CBLAS_UPLO const Uplo, int const M, int const N, 
               traits::complex_f const& alpha, 
               traits::complex_f const* A, int const lda,
               traits::complex_f const* B, int const ldb, 
               traits::complex_f const& beta, 
               traits::complex_f* C, int const ldc) 
    {
      cblas_chemm (Order, Side, Uplo, M, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (A), lda, 
                   static_cast<void const*> (B), ldb,
                   static_cast<void const*> (&beta), 
                   static_cast<void*> (C), ldc); 
    }
  
    inline 
    void hemm (CBLAS_ORDER const Order, CBLAS_SIDE const Side,
               CBLAS_UPLO const Uplo, int const M, int const N, 
               traits::complex_d const& alpha, 
               traits::complex_d const* A, int const lda,
               traits::complex_d const* B, int const ldb, 
               traits::complex_d const& beta, 
               traits::complex_d* C, int const ldc) 
    {
      cblas_zhemm (Order, Side, Uplo, M, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (A), lda, 
                   static_cast<void const*> (B), ldb,
                   static_cast<void const*> (&beta), 
                   static_cast<void*> (C), ldc); 
    }


    // C <- alpha * A * A^T + beta * C
    // C <- alpha * A^T * A + beta * C
    // C == C^T

    inline
    void syrk (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               CBLAS_TRANSPOSE const Trans, int const N, int const K,
               float const alpha, float const* A, int const lda,
               float const beta, float* C, int const ldc) 
    {
      cblas_ssyrk (Order, Uplo, Trans, N, K, alpha, A, lda, beta, C, ldc); 
    }

    inline
    void syrk (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               CBLAS_TRANSPOSE const Trans, int const N, int const K,
               double const alpha, double const* A, int const lda,
               double const beta, double* C, int const ldc) 
    {
      cblas_dsyrk (Order, Uplo, Trans, N, K, alpha, A, lda, beta, C, ldc); 
    }

    inline
    void syrk (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               CBLAS_TRANSPOSE const Trans, int const N, int const K,
               traits::complex_f const& alpha, 
               traits::complex_f const* A, int const lda,
               traits::complex_f const& beta, 
               traits::complex_f* C, int const ldc) 
    {
      cblas_csyrk (Order, Uplo, Trans, N, K, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (A), lda, 
                   static_cast<void const*> (&beta), 
                   static_cast<void*> (C), ldc); 
    }

    inline
    void syrk (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               CBLAS_TRANSPOSE const Trans, int const N, int const K,
               traits::complex_d const& alpha, 
               traits::complex_d const* A, int const lda,
               traits::complex_d const& beta, 
               traits::complex_d* C, int const ldc) 
    {
      cblas_zsyrk (Order, Uplo, Trans, N, K, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (A), lda, 
                   static_cast<void const*> (&beta), 
                   static_cast<void*> (C), ldc); 
    }


    // C <- alpha * A * B^T + conj(alpha) * B * A^T + beta * C
    // C <- alpha * A^T * B + conj(alpha) * B^T * A + beta * C
    // C == C^T

    inline
    void syr2k (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
                CBLAS_TRANSPOSE const Trans, int const N, int const K,
                float const alpha, float const* A, int const lda,
                float const* B, int const ldb,
                float const beta, float* C, int const ldc) 
    {
      cblas_ssyr2k (Order, Uplo, Trans, N, K, 
                    alpha, A, lda, B, ldb, beta, C, ldc); 
    }

    inline
    void syr2k (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
                CBLAS_TRANSPOSE const Trans, int const N, int const K,
                double const alpha, double const* A, int const lda,
                double const* B, int const ldb,
                double const beta, double* C, int const ldc) 
    {
      cblas_dsyr2k (Order, Uplo, Trans, N, K, 
                    alpha, A, lda, B, ldb, beta, C, ldc); 
    }

    inline
    void syr2k (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
                CBLAS_TRANSPOSE const Trans, int const N, int const K,
                traits::complex_f const& alpha, 
                traits::complex_f const* A, int const lda,
                traits::complex_f const* B, int const ldb,
                traits::complex_f const& beta, 
                traits::complex_f* C, int const ldc) 
    {
      cblas_csyr2k (Order, Uplo, Trans, N, K, 
                    static_cast<void const*> (&alpha), 
                    static_cast<void const*> (A), lda, 
                    static_cast<void const*> (B), ldb, 
                    static_cast<void const*> (&beta), 
                    static_cast<void*> (C), ldc); 
    }

    inline
    void syr2k (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
                CBLAS_TRANSPOSE const Trans, int const N, int const K,
                traits::complex_d const& alpha, 
                traits::complex_d const* A, int const lda,
                traits::complex_d const* B, int const ldb,
                traits::complex_d const& beta, 
                traits::complex_d* C, int const ldc) 
    {
      cblas_zsyr2k (Order, Uplo, Trans, N, K, 
                    static_cast<void const*> (&alpha), 
                    static_cast<void const*> (A), lda, 
                    static_cast<void const*> (B), ldb, 
                    static_cast<void const*> (&beta), 
                    static_cast<void*> (C), ldc); 
    }


    // C <- alpha * A * A^H + beta * C
    // C <- alpha * A^H * A + beta * C
    // C == C^H

    inline
    void herk (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               CBLAS_TRANSPOSE const Trans, int const N, int const K,
               float alpha, traits::complex_f const* A, int const lda,
               float beta, traits::complex_f* C, int const ldc) 
    {
      cblas_cherk (Order, Uplo, Trans, N, K, 
                   alpha, static_cast<void const*> (A), lda, 
                   beta, static_cast<void*> (C), ldc); 
    }

    inline
    void herk (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               CBLAS_TRANSPOSE const Trans, int const N, int const K,
               double alpha, traits::complex_d const* A, int const lda,
               double beta, traits::complex_d* C, int const ldc) 
    {
      cblas_zherk (Order, Uplo, Trans, N, K, 
                   alpha, static_cast<void const*> (A), lda, 
                   beta, static_cast<void*> (C), ldc); 
    }


    // C <- alpha * A * B^H + conj(alpha) * B * A^H + beta * C
    // C <- alpha * A^H * B + conj(alpha) * B^H * A + beta * C
    // C == C^H

    inline
    void her2k (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
                CBLAS_TRANSPOSE const Trans, int const N, int const K,
                traits::complex_f const& alpha, 
                traits::complex_f const* A, int const lda,
                traits::complex_f const* B, int const ldb,
                float beta, traits::complex_f* C, int const ldc) 
    {
      cblas_cher2k (Order, Uplo, Trans, N, K, 
                    static_cast<void const*> (&alpha), 
                    static_cast<void const*> (A), lda, 
                    static_cast<void const*> (B), ldb, 
                    beta, static_cast<void*> (C), ldc); 
    }

    inline
    void her2k (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
                CBLAS_TRANSPOSE const Trans, int const N, int const K,
                traits::complex_d const& alpha, 
                traits::complex_d const* A, int const lda,
                traits::complex_d const* B, int const ldb,
                double beta, traits::complex_d* C, int const ldc) 
    {
      cblas_zher2k (Order, Uplo, Trans, N, K, 
                    static_cast<void const*> (&alpha), 
                    static_cast<void const*> (A), lda, 
                    static_cast<void const*> (B), ldb, 
                    beta, static_cast<void*> (C), ldc); 
    }

  
  }} // namepaces detail & atlas

}}} 


#endif // BOOST_NUMERIC_BINDINGS_CBLAS3_OVERLOADS_HPP
