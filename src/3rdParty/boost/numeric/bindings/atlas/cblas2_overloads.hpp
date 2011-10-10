/*
 * 
 * Copyright (c) Kresimir Fresl 2002, 2003
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Author acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_CBLAS2_OVERLOADS_HPP
#define BOOST_NUMERIC_BINDINGS_CBLAS2_OVERLOADS_HPP

#include <complex> 
#include <boost/numeric/bindings/atlas/cblas_inc.hpp>
#include <boost/numeric/bindings/traits/type.hpp>


namespace boost { namespace numeric { namespace bindings { 

  namespace atlas { namespace detail {


    // y <- alpha * op (A) * x + beta * y

    inline 
    void gemv (CBLAS_ORDER const Order, 
               CBLAS_TRANSPOSE const TransA, int const M, int const N,
               float const alpha, float const* A, int const lda,
               float const* X, int const incX, 
               float const beta, float* Y, int const incY) 
    {
      cblas_sgemv (Order, TransA, M, N, alpha, A, lda, 
                   X, incX,
                   beta, Y, incY); 
    }
    
    inline 
    void gemv (CBLAS_ORDER const Order, 
               CBLAS_TRANSPOSE const TransA, int const M, int const N,
               double const alpha, double const* A, int const lda,
               double const* X, int const incX, 
               double const beta, double* Y, int const incY) 
    {
      cblas_dgemv (Order, TransA, M, N, alpha, A, lda, 
                   X, incX,
                   beta, Y, incY); 
    }
    
    inline 
    void gemv (CBLAS_ORDER const Order, 
               CBLAS_TRANSPOSE const TransA, int const M, int const N,
               traits::complex_f const& alpha, 
               traits::complex_f const* A, int const lda,
               traits::complex_f const* X, int const incX, 
               traits::complex_f const& beta, 
               traits::complex_f* Y, int const incY) 
    {
      cblas_cgemv (Order, TransA, M, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (A), lda, 
                   static_cast<void const*> (X), incX,
                   static_cast<void const*> (&beta), 
                   static_cast<void*> (Y), incY); 
    }
    
    inline 
    void gemv (CBLAS_ORDER const Order, 
               CBLAS_TRANSPOSE const TransA, int const M, int const N,
               traits::complex_d const& alpha, 
               traits::complex_d const* A, int const lda,
               traits::complex_d const* X, int const incX, 
               traits::complex_d const& beta, 
               traits::complex_d* Y, int const incY) 
    {
      cblas_zgemv (Order, TransA, M, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (A), lda, 
                   static_cast<void const*> (X), incX,
                   static_cast<void const*> (&beta), 
                   static_cast<void*> (Y), incY); 
    }


    // y <- alpha * A * x + beta * y
    // A real symmetric

    inline 
    void symv (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, float const alpha, float const* A,
               int const lda, float const* X, int const incX,
               float const beta, float* Y, int const incY) 
    {
      cblas_ssymv (Order, Uplo, N, alpha, A, lda, 
                   X, incX, beta, Y, incY);
    }

    inline 
    void symv (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, double const alpha, double const* A,
               int const lda, double const* X, int const incX,
               double const beta, double* Y, int const incY) 
    {
      cblas_dsymv (Order, Uplo, N, alpha, A, lda, 
                   X, incX, beta, Y, incY);
    }


    // y <- alpha * A * x + beta * y
    // A real symmetric in packed form 

    inline
    void spmv (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, float const alpha, float const* Ap,
               float const* X, int const incX,
               float const beta, float* Y, int const incY) 
    {
      cblas_sspmv (Order, Uplo, N, alpha, Ap, X, incX, beta, Y, incY);
    }

    inline
    void spmv (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, double const alpha, double const* Ap,
               double const* X, int const incX,
               double const beta, double* Y, int const incY) 
    {
      cblas_dspmv (Order, Uplo, N, alpha, Ap, X, incX, beta, Y, incY);
    }


    // y <- alpha * A * x + beta * y
    // A complex hermitian 

    inline 
    void hemv (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, traits::complex_f const& alpha, 
               traits::complex_f const* A, int const lda, 
               traits::complex_f const* X, int const incX,
               traits::complex_f const& beta, 
               traits::complex_f* Y, int const incY) 
    {
      cblas_chemv (Order, Uplo, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (A), lda, 
                   static_cast<void const*> (X), incX, 
                   static_cast<void const*> (&beta), 
                   static_cast<void*> (Y), incY);
    }

    inline 
    void hemv (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, traits::complex_d const& alpha, 
               traits::complex_d const* A, int const lda, 
               traits::complex_d const* X, int const incX,
               traits::complex_d const& beta, 
               traits::complex_d* Y, int const incY) 
    {
      cblas_zhemv (Order, Uplo, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (A), lda, 
                   static_cast<void const*> (X), incX, 
                   static_cast<void const*> (&beta), 
                   static_cast<void*> (Y), incY);
    }


    // y <- alpha * A * x + beta * y
    // A complex hermitian in packed form 

    inline 
    void hpmv (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, traits::complex_f const& alpha, 
               traits::complex_f const* Ap, 
               traits::complex_f const* X, int const incX,
               traits::complex_f const& beta, 
               traits::complex_f* Y, int const incY) 
    {
      cblas_chpmv (Order, Uplo, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (Ap), 
                   static_cast<void const*> (X), incX, 
                   static_cast<void const*> (&beta), 
                   static_cast<void*> (Y), incY);
    }

    inline 
    void hpmv (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, traits::complex_d const& alpha, 
               traits::complex_d const* Ap, 
               traits::complex_d const* X, int const incX,
               traits::complex_d const& beta, 
               traits::complex_d* Y, int const incY) 
    {
      cblas_zhpmv (Order, Uplo, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (Ap), 
                   static_cast<void const*> (X), incX, 
                   static_cast<void const*> (&beta), 
                   static_cast<void*> (Y), incY);
    }


    // A <- alpha * x * y^T + A 
    // .. real types:    calls cblas_xger
    // .. complex types: calls cblas_xgeru

    inline 
    void ger (CBLAS_ORDER const Order, int const M, int const N,
              float const alpha, float const* X, int const incX,
              float const* Y, int const incY, float* A, int const lda)
    {
      cblas_sger (Order, M, N, alpha, X, incX, Y, incY, A, lda); 
    }

    inline 
    void ger (CBLAS_ORDER const Order, int const M, int const N,
              double const alpha, double const* X, int const incX,
              double const* Y, int const incY, double* A, int const lda)
    {
      cblas_dger (Order, M, N, alpha, X, incX, Y, incY, A, lda); 
    }

    inline 
    void ger (CBLAS_ORDER const Order, int const M, int const N,
              traits::complex_f const& alpha, 
              traits::complex_f const* X, int const incX,
              traits::complex_f const* Y, int const incY, 
              traits::complex_f* A, int const lda)
    {
      cblas_cgeru (Order, M, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (X), incX, 
                   static_cast<void const*> (Y), incY, 
                   static_cast<void*> (A), lda); 
    }

    inline 
    void ger (CBLAS_ORDER const Order, int const M, int const N,
              traits::complex_d const& alpha, 
              traits::complex_d const* X, int const incX,
              traits::complex_d const* Y, int const incY, 
              traits::complex_d* A, int const lda)
    {
      cblas_zgeru (Order, M, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (X), incX, 
                   static_cast<void const*> (Y), incY, 
                   static_cast<void*> (A), lda); 
    }

    // A <- alpha * x * y^T + A 
    // .. complex types only 

    inline 
    void geru (CBLAS_ORDER const Order, int const M, int const N,
               traits::complex_f const& alpha, 
               traits::complex_f const* X, int const incX,
               traits::complex_f const* Y, int const incY, 
               traits::complex_f* A, int const lda)
    {
      cblas_cgeru (Order, M, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (X), incX, 
                   static_cast<void const*> (Y), incY, 
                   static_cast<void*> (A), lda); 
    }

    inline 
    void geru (CBLAS_ORDER const Order, int const M, int const N,
               traits::complex_d const& alpha, 
               traits::complex_d const* X, int const incX,
               traits::complex_d const* Y, int const incY, 
               traits::complex_d* A, int const lda)
    {
      cblas_zgeru (Order, M, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (X), incX, 
                   static_cast<void const*> (Y), incY, 
                   static_cast<void*> (A), lda); 
    }
    
    // A <- alpha * x * y^H + A 
    // .. complex types only 

    inline 
    void gerc (CBLAS_ORDER const Order, int const M, int const N,
               traits::complex_f const& alpha, 
               traits::complex_f const* X, int const incX,
               traits::complex_f const* Y, int const incY, 
               traits::complex_f* A, int const lda)
    {
      cblas_cgerc (Order, M, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (X), incX, 
                   static_cast<void const*> (Y), incY, 
                   static_cast<void*> (A), lda); 
    }

    inline 
    void gerc (CBLAS_ORDER const Order, int const M, int const N,
               traits::complex_d const& alpha, 
               traits::complex_d const* X, int const incX,
               traits::complex_d const* Y, int const incY, 
               traits::complex_d* A, int const lda)
    {
      cblas_zgerc (Order, M, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (X), incX, 
                   static_cast<void const*> (Y), incY, 
                   static_cast<void*> (A), lda); 
    }


    // A <- alpha * x * x^T + A 
    // A real symmetric 

    inline 
    void syr (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
              int const N, float const alpha, 
              float const* X, int const incX, float* A, int const lda)
    {
      cblas_ssyr (Order, Uplo, N, alpha, X, incX, A, lda); 
    }

    inline 
    void syr (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
              int const N, double const alpha, 
              double const* X, int const incX, double* A, int const lda)
    {
      cblas_dsyr (Order, Uplo, N, alpha, X, incX, A, lda); 
    }


    // A <- alpha * x * x^T + A 
    // A real symmetric in packed form 
    
    inline 
    void spr (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
              int const N, float const alpha, 
              float const* X, int const incX, float* Ap)
    {
      cblas_sspr (Order, Uplo, N, alpha, X, incX, Ap); 
    }

    inline 
    void spr (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
              int const N, double const alpha, 
              double const* X, int const incX, double* Ap)
    {
      cblas_dspr (Order, Uplo, N, alpha, X, incX, Ap); 
    }


    // A <- alpha * x * y^T + alpha * y * x^T + A 
    // A real symmetric 

    inline 
    void syr2 (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, float const alpha, 
               float const* X, int const incX, 
               float const* Y, int const incY, 
               float* A, int const lda)
    {
      cblas_ssyr2 (Order, Uplo, N, alpha, X, incX, Y, incY, A, lda); 
    }

    inline 
    void syr2 (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, double const alpha, 
               double const* X, int const incX, 
               double const* Y, int const incY, 
               double* A, int const lda)
    {
      cblas_dsyr2 (Order, Uplo, N, alpha, X, incX, Y, incY, A, lda); 
    }


    // A <- alpha * x * y^T + alpha * y * x^T + A 
    // A real symmetric in packed form 
    
    inline 
    void spr2 (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, float const alpha, 
               float const* X, int const incX, 
               float const* Y, int const incY, float* Ap)
    {
      cblas_sspr2 (Order, Uplo, N, alpha, X, incX, Y, incY, Ap); 
    }

    inline 
    void spr2 (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, double const alpha, 
               double const* X, int const incX, 
               double const* Y, int const incY, double* Ap)
    {
      cblas_dspr2 (Order, Uplo, N, alpha, X, incX, Y, incY, Ap); 
    }


    // A <- alpha * x * x^H + A 
    // A hermitian

    inline 
    void her (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
              int const N, float const alpha, 
              traits::complex_f const* X, int const incX, 
              traits::complex_f* A, int const lda)
    {
      cblas_cher (Order, Uplo, N, alpha, 
                  static_cast<void const*> (X), incX, 
                  static_cast<void*> (A), lda); 
    }

    inline 
    void her (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
              int const N, double const alpha, 
              traits::complex_d const* X, int const incX, 
              traits::complex_d* A, int const lda)
    {
      cblas_zher (Order, Uplo, N, alpha, 
                  static_cast<void const*> (X), incX, 
                  static_cast<void*> (A), lda); 
    }


    // A <- alpha * x * x^H + A 
    // A hermitian in packed form 
    
    inline 
    void hpr (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
              int const N, float const alpha, 
              traits::complex_f const* X, int const incX, 
              traits::complex_f* Ap)
    {
      cblas_chpr (Order, Uplo, N, alpha, 
                  static_cast<void const*> (X), incX, 
                  static_cast<void*> (Ap)); 
    }

    inline 
    void hpr (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
              int const N, double const alpha, 
              traits::complex_d const* X, int const incX, 
              traits::complex_d* Ap)
    {
      cblas_zhpr (Order, Uplo, N, alpha, 
                  static_cast<void const*> (X), incX, 
                  static_cast<void*> (Ap)); 
    }


    // A <- alpha * x * y^H + y * (alpha * x)^H + A 
    // A hermitian

    inline 
    void her2 (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, traits::complex_f const& alpha, 
               traits::complex_f const* X, int const incX, 
               traits::complex_f const* Y, int const incY, 
               traits::complex_f* A, int const lda)
    {
      cblas_cher2 (Order, Uplo, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (X), incX, 
                   static_cast<void const*> (Y), incY, 
                   static_cast<void*> (A), lda); 
    }

    inline 
    void her2 (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, traits::complex_d const& alpha, 
               traits::complex_d const* X, int const incX, 
               traits::complex_d const* Y, int const incY, 
               traits::complex_d* A, int const lda)
    {
      cblas_zher2 (Order, Uplo, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (X), incX, 
                   static_cast<void const*> (Y), incY, 
                   static_cast<void*> (A), lda); 
    }


    // A <- alpha * x * y^H + y * (alpha * x)^H + A 
    // A hermitian in packed form 
    
    inline 
    void hpr2 (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, traits::complex_f const& alpha, 
               traits::complex_f const* X, int const incX, 
               traits::complex_f const* Y, int const incY, 
               traits::complex_f* Ap)
    {
      cblas_chpr2 (Order, Uplo, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (X), incX, 
                   static_cast<void const*> (Y), incY, 
                   static_cast<void*> (Ap)); 
    }

    inline 
    void hpr2 (CBLAS_ORDER const Order, CBLAS_UPLO const Uplo,
               int const N, traits::complex_d const& alpha, 
               traits::complex_d const* X, int const incX, 
               traits::complex_d const* Y, int const incY, 
               traits::complex_d* Ap)
    {
      cblas_zhpr2 (Order, Uplo, N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (X), incX, 
                   static_cast<void const*> (Y), incY, 
                   static_cast<void*> (Ap)); 
    }


  }} // namepaces detail & atlas

}}} 


#endif // BOOST_NUMERIC_BINDINGS_CBLAS2_OVERLOADS_HPP
