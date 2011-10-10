//
//  Copyright (C) 2002, 2003 Si-Lab b.v.b.a and Toon Knapen 
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NUMERIC_BINDINGS_BLAS_BLAS_H
#define BOOST_NUMERIC_BINDINGS_BLAS_BLAS_H

/*
 * const-correct prototypes for BLAS functions
 * 
 */

#include <boost/numeric/bindings/blas/blas_names.h>
#include <boost/numeric/bindings/traits/type.h>

extern "C"
{
  //
  // Level 1
  //
  void   BLAS_SSCAL(const int *n, const float*      alpha, float*      x, const int* incx);
  void   BLAS_DSCAL(const int *n, const double*     alpha, double*     x, const int* incx);
  void   BLAS_CSCAL(const int *n, const fcomplex_t* alpha, fcomplex_t* x, const int* incx);
  void   BLAS_ZSCAL(const int *n, const dcomplex_t* alpha, dcomplex_t* x, const int* incx);

  void   BLAS_SAXPY(const int *n, const float*      alpha, const float*      x, const int* incx,  float*      y, const int* incy);
  void   BLAS_DAXPY(const int *n, const double*     alpha, const double*     x, const int* incx,  double*     y, const int* incy);
  void   BLAS_CAXPY(const int *n, const fcomplex_t* alpha, const fcomplex_t* x, const int* incx,  fcomplex_t* y, const int* incy);
  void   BLAS_ZAXPY(const int *n, const dcomplex_t* alpha, const dcomplex_t* x, const int* incx,  dcomplex_t* y, const int* incy);

  float  BLAS_SDOT (const int *n, const float  *x, const int *incx, const float  *y, const int *incy);
  double BLAS_DDOT (const int *n, const double *x, const int *incx, const double *y, const int *incy);

  void   BLAS_CDOTU(fcomplex_t* ret, const int *n, const fcomplex_t *x, const int *incx, const fcomplex_t *y, const int *incy);
  void   BLAS_ZDOTU(dcomplex_t* ret, const int *n, const dcomplex_t *x, const int *incx, const dcomplex_t *y, const int *incy); 
  
  void   BLAS_CDOTC(fcomplex_t* ret, const int *n, const fcomplex_t *x, const int *incx, const fcomplex_t *y, const int *incy);
  void   BLAS_ZDOTC(dcomplex_t* ret, const int *n, const dcomplex_t *x, const int *incx, const dcomplex_t *y, const int *incy);

  float   BLAS_SNRM2(const int *n, const float  *x, const int *incx);
  double  BLAS_DNRM2(const int *n, const double *x, const int *incx);
  float   BLAS_SCNRM2(const int *n, const fcomplex_t  *x, const int *incx);
  double  BLAS_DZNRM2(const int *n, const dcomplex_t *x, const int *incx);

  float   BLAS_SASUM(const int *n, const float  *x, const int *incx);
  double  BLAS_DASUM(const int *n, const double *x, const int *incx);
  float   BLAS_SCASUM(const int *n, const fcomplex_t  *x, const int *incx);
  double  BLAS_DZASUM(const int *n, const dcomplex_t *x, const int *incx);

  void BLAS_SCOPY( const int *n, const float  *x, const int *incx, float  *y, const int *incy);
  void BLAS_DCOPY( const int *n, const double  *x, const int *incx, double  *y, const int *incy);
  void BLAS_CCOPY( const int *n, const fcomplex_t  *x, const int *incx, fcomplex_t  *y, const int *incy);
  void BLAS_ZCOPY( const int *n, const dcomplex_t  *x, const int *incx, dcomplex_t  *y, const int *incy);


  //
  // Level 2
  //
  void   BLAS_SGEMV(const char *trans, const int *m, const int *n, const float    *alpha, const float    *a, const int *lda, const float    *x, const int *incx, const float    *beta, float    *y, const int *incy) ;
  void   BLAS_DGEMV(const char *trans, const int *m, const int *n, const double   *alpha, const double   *a, const int *lda, const double   *x, const int *incx, const double   *beta, double   *y, const int *incy) ;
  void   BLAS_CGEMV(const char *trans, const int *m, const int *n, const fcomplex_t *alpha, const fcomplex_t *a, const int *lda, const fcomplex_t *x, const int *incx, const fcomplex_t *beta, fcomplex_t *y, const int *incy) ;
  void   BLAS_ZGEMV(const char *trans, const int *m, const int *n, const dcomplex_t *alpha, const dcomplex_t *a, const int *lda, const dcomplex_t *x, const int *incx, const dcomplex_t *beta, dcomplex_t *y, const int *incy) ;

  void   BLAS_SGER (const int *m, const int *n, const float * alpha, const float * x, const int *incx, const float * y, const int *incy, float *a,  const int *lda);
  void   BLAS_DGER (const int *m, const int *n, const double *alpha, const double *x, const int *incx, const double *y, const int *incy, double *a, const int *lda);

  void   BLAS_CGERU(const int *m, const int *n, const fcomplex_t *alpha, const fcomplex_t *x, const int *incx, const fcomplex_t *y, const int *incy, fcomplex_t *a, const int *lda);
  void   BLAS_ZGERU(const int *m, const int *n, const dcomplex_t *alpha, const dcomplex_t *x, const int *incx, const dcomplex_t *y, const int *incy, dcomplex_t *a, const int *lda);

  void   BLAS_CGERC(const int *m, const int *n, const fcomplex_t *alpha, const fcomplex_t *x, const int *incx, const fcomplex_t *y, const int *incy, fcomplex_t *a, const int *lda);
  void   BLAS_ZGERC(const int *m, const int *n, const dcomplex_t *alpha, const dcomplex_t *x, const int *incx, const dcomplex_t *y, const int *incy, dcomplex_t *a, const int *lda);


  //
  // Level 3
  //
  void   BLAS_SGEMM(const char *transa, const char *transb, const int *m, const int *n, const int *k, const float      *alpha, const float      *a, const int *lda, const float      *b, const int *ldb, const float      *beta, float      *c, const int *ldc);
  void   BLAS_DGEMM(const char *transa, const char *transb, const int *m, const int *n, const int *k, const double     *alpha, const double     *a, const int *lda, const double     *b, const int *ldb, const double     *beta, double     *c, const int *ldc);
  void   BLAS_CGEMM(const char *transa, const char *transb, const int *m, const int *n, const int *k, const fcomplex_t *alpha, const fcomplex_t *a, const int *lda, const fcomplex_t *b, const int *ldb, const fcomplex_t *beta, fcomplex_t *c, const int *ldc);
  void   BLAS_ZGEMM(const char *transa, const char *transb, const int *m, const int *n, const int *k, const dcomplex_t *alpha, const dcomplex_t *a, const int *lda, const dcomplex_t *b, const int *ldb, const dcomplex_t *beta, dcomplex_t *c, const int *ldc);

  void   BLAS_SSYRK ( const char* uplo, const char* trans, const int* n, const int* k, const float* alpha,
                      const float* a, const int* lda, const float* beta, float* c, const int* ldc );
  void   BLAS_DSYRK ( const char* uplo, const char* trans, const int* n, const int* k, const double* alpha,
                      const double* a, const int* lda, const double* beta, double* c, const int* ldc );
  void   BLAS_CSYRK ( const char* uplo, const char* trans, const int* n, const int* k, const fcomplex_t* alpha,
                      const fcomplex_t* a, const int* lda, const fcomplex_t* beta, fcomplex_t* c, const int* ldc );
  void   BLAS_ZSYRK ( const char* uplo, const char* trans, const int* n, const int* k, const dcomplex_t* alpha,
                      const dcomplex_t* a, const int* lda, const dcomplex_t* beta, dcomplex_t* c, const int* ldc );
  void   BLAS_CHERK ( const char* uplo, const char* trans, const int* n, const int* k, const float* alpha,
                      const fcomplex_t* a, const int* lda, const float* beta, fcomplex_t* c, const int* ldc );
  void   BLAS_ZHERK ( const char* uplo, const char* trans, const int* n, const int* k, const double* alpha,
                      const dcomplex_t* a, const int* lda, const double* beta, dcomplex_t* c, const int* ldc );

  void BLAS_STRSM( const char* side, const char* uplo, const char* transa, const char* diag, const int* m,
                   const int* n, float const* alpha, float const* a, int const* lda, float* b, int const* ldb );
  void BLAS_DTRSM( const char* side, const char* uplo, const char* transa, const char* diag, const int* m,
                   const int* n, double const* alpha, double const* a, int const* lda, double* b, int const* ldb );
  void BLAS_CTRSM( const char* side, const char* uplo, const char* transa, const char* diag, const int* m,
                   const int* n, fcomplex_t const* alpha, fcomplex_t const* a, int const* lda, fcomplex_t* b, int const* ldb );
  void BLAS_ZTRSM( const char* side, const char* uplo, const char* transa, const char* diag, const int* m,
                   const int* n, dcomplex_t const* alpha, dcomplex_t const* a, int const* lda, dcomplex_t* b, int const* ldb );

}

#endif // BOOST_NUMERIC_BINDINGS_BLAS_BLAS_H
