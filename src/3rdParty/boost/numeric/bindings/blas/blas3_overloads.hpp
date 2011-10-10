//
//  Copyright (C) Toon Knapen 2003
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NUMERIC_BINDINGS_BLAS_BLAS3_OVERLOADS_HPP
#define BOOST_NUMERIC_BINDINGS_BLAS_BLAS3_OVERLOADS_HPP

#include <boost/numeric/bindings/blas/blas.h>
#include <boost/numeric/bindings/traits/type_traits.hpp>

namespace boost { namespace numeric { namespace bindings { namespace blas { namespace detail {

  using namespace boost::numeric::bindings::traits ;

  inline
  void gemm( char TRANSA, char TRANSB, const int& m, const int& n, const int& k, const float    & alpha, const float    * a_ptr, const int& lda, const float    * b_ptr, const int& ldb, const float    & beta, float    * c_ptr, const int& ldc ) { BLAS_SGEMM( &TRANSA, &TRANSB, &m, &n, &k,          ( &alpha ),          ( a_ptr ), &lda,          ( b_ptr ), &ldb,          ( &beta ),          ( c_ptr ), &ldc ) ; }
  inline
  void gemm( char TRANSA, char TRANSB, const int& m, const int& n, const int& k, const double   & alpha, const double   * a_ptr, const int& lda, const double   * b_ptr, const int& ldb, const double   & beta, double   * c_ptr, const int& ldc ) { BLAS_DGEMM( &TRANSA, &TRANSB, &m, &n, &k,          ( &alpha ),          ( a_ptr ), &lda,          ( b_ptr ), &ldb,          ( &beta ),          ( c_ptr ), &ldc ) ; }
  inline
  void gemm( char TRANSA, char TRANSB, const int& m, const int& n, const int& k, const complex_f& alpha, const complex_f* a_ptr, const int& lda, const complex_f* b_ptr, const int& ldb, const complex_f& beta, complex_f* c_ptr, const int& ldc ) { BLAS_CGEMM( &TRANSA, &TRANSB, &m, &n, &k, complex_ptr( &alpha ), complex_ptr( a_ptr ), &lda, complex_ptr( b_ptr ), &ldb, complex_ptr( &beta ), complex_ptr( c_ptr ), &ldc ) ; }
  inline
  void gemm( char TRANSA, char TRANSB, const int& m, const int& n, const int& k, const complex_d& alpha, const complex_d* a_ptr, const int& lda, const complex_d* b_ptr, const int& ldb, const complex_d& beta, complex_d* c_ptr, const int& ldc ) { BLAS_ZGEMM( &TRANSA, &TRANSB, &m, &n, &k, complex_ptr( &alpha ), complex_ptr( a_ptr ), &lda, complex_ptr( b_ptr ), &ldb, complex_ptr( &beta ), complex_ptr( c_ptr ), &ldc ) ; }


  //
  // SYRK
  //
  inline
  void syrk( char uplo, char trans, const int& n, const int& k, const float& alpha,
             const float* a_ptr, const int lda, const float& beta, float* c_ptr,
             const int& ldc)
  {
     BLAS_SSYRK( &uplo, &trans, &n, &k, &alpha, a_ptr, &lda, &beta, c_ptr, &ldc);
  }

  inline
  void syrk( char uplo, char trans, const int& n, const int& k, const double& alpha,
             const double* a_ptr, const int lda, const double& beta, double* c_ptr,
             const int& ldc)
  {
     BLAS_DSYRK( &uplo, &trans, &n, &k, &alpha, a_ptr, &lda, &beta, c_ptr, &ldc);
  }

  inline
  void syrk( char uplo, char trans, const int& n, const int& k, const complex_f& alpha,
             const complex_f* a_ptr, const int lda, const complex_f& beta, complex_f* c_ptr,
             const int& ldc)
  {
     BLAS_CSYRK( &uplo, &trans, &n, &k, complex_ptr( &alpha ), complex_ptr( a_ptr ),
                 &lda, complex_ptr( &beta ), complex_ptr( c_ptr ), &ldc);
  }

  inline
  void syrk( char uplo, char trans, const int& n, const int& k, const complex_d& alpha,
             const complex_d* a_ptr, const int lda, const complex_d& beta, complex_d* c_ptr,
             const int& ldc)
  {
     BLAS_ZSYRK( &uplo, &trans, &n, &k, complex_ptr( &alpha ), complex_ptr( a_ptr ),
                 &lda, complex_ptr( &beta ), complex_ptr( c_ptr ), &ldc);
  }

  //
  // HERK
  //
  inline
  void herk( char uplo, char trans, const int& n, const int& k, const float& alpha,
             const float* a_ptr, const int lda, const float& beta, float* c_ptr,
             const int& ldc)
  {
     BLAS_SSYRK( &uplo, &trans, &n, &k, &alpha, a_ptr, &lda, &beta, c_ptr, &ldc);
  }

  inline
  void herk( char uplo, char trans, const int& n, const int& k, const double& alpha,
             const double* a_ptr, const int lda, const double& beta, double* c_ptr,
             const int& ldc)
  {
     BLAS_DSYRK( &uplo, &trans, &n, &k, &alpha, a_ptr, &lda, &beta, c_ptr, &ldc);
  }


  inline
  void herk( char uplo, char trans, const int& n, const int& k, const float& alpha,
             const complex_f* a_ptr, const int lda, const float& beta, complex_f* c_ptr,
             const int& ldc)
  {
     BLAS_CHERK( &uplo, &trans, &n, &k, &alpha, complex_ptr( a_ptr ),
                 &lda, &beta, complex_ptr( c_ptr ), &ldc);
  }

  inline
  void herk( char uplo, char trans, const int& n, const int& k, const double& alpha,
             const complex_d* a_ptr, const int lda, const double& beta, complex_d* c_ptr,
             const int& ldc)
  {
     BLAS_ZHERK( &uplo, &trans, &n, &k, &alpha, complex_ptr( a_ptr ),
                 &lda, &beta, complex_ptr( c_ptr ), &ldc);
  }

  //
  // trsm
  //
  inline
  void trsm( char side, char uplo, char transa, char diag, int m, int n,
             float const& alpha, float const* a_ptr, int lda,
             float* b_ptr, int ldb )
  {
     BLAS_STRSM( &side, &uplo, &transa, &diag, &m, &n, &alpha, a_ptr, &lda, b_ptr, &ldb ) ;
  }

  inline
  void trsm( char side, char uplo, char transa, char diag, int m, int n,
             double const& alpha, double const* a_ptr, int lda,
             double* b_ptr, int ldb )
  {
     BLAS_DTRSM( &side, &uplo, &transa, &diag, &m, &n, &alpha, a_ptr, &lda, b_ptr, &ldb ) ;
  }

  inline
  void trsm( char side, char uplo, char transa, char diag, int m, int n,
             complex_f const& alpha, complex_f const* a_ptr, int lda,
             complex_f* b_ptr, int ldb )
  {
     BLAS_CTRSM( &side, &uplo, &transa, &diag, &m, &n, complex_ptr( &alpha ), complex_ptr( a_ptr ), &lda, complex_ptr( b_ptr ), &ldb ) ;
  }

  inline
  void trsm( char side, char uplo, char transa, char diag, int m, int n,
             complex_d const& alpha, complex_d const* a_ptr, int lda,
             complex_d* b_ptr, int ldb )
  {
     BLAS_ZTRSM( &side, &uplo, &transa, &diag, &m, &n, complex_ptr( &alpha ), complex_ptr( a_ptr ), &lda, complex_ptr( b_ptr ), &ldb ) ;
  }

}}}}}

#endif // BOOST_NUMERIC_BINDINGS_BLAS_BLAS3_OVERLOADS_HPP

