/*
 *             Automatically Tuned Linear Algebra Software v3.6.0
 *                    (C) Copyright 1999 R. Clint Whaley
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions, and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. The name of the ATLAS group or the names of its contributers may
 *      not be used to endorse or promote products derived from this
 *      software without specific written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE ATLAS GROUP OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef CLAPACK_H

#define CLAPACK_H
#include "cblas.h"

#ifndef ATLAS_ORDER
   #define ATLAS_ORDER CBLAS_ORDER
#endif
#ifndef ATLAS_UPLO
   #define ATLAS_UPLO CBLAS_UPLO
#endif
#ifndef ATLAS_DIAG
   #define ATLAS_DIAG CBLAS_DIAG
#endif
int clapack_sgesv(const enum CBLAS_ORDER Order, const int N, const int NRHS,
                  float *A, const int lda, int *ipiv,
                  float *B, const int ldb);
int clapack_sgetrf(const enum CBLAS_ORDER Order, const int M, const int N,
                   float *A, const int lda, int *ipiv);
int clapack_sgetrs
   (const enum CBLAS_ORDER Order, const enum CBLAS_TRANSPOSE Trans,
    const int N, const int NRHS, const float *A, const int lda,
    const int *ipiv, float *B, const int ldb);
int clapack_sgetri(const enum CBLAS_ORDER Order, const int N, float *A,
                   const int lda, const int *ipiv);
int clapack_sposv(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                  const int N, const int NRHS, float *A, const int lda,
                  float *B, const int ldb);
int clapack_spotrf(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                   const int N, float *A, const int lda);
int clapack_spotrs(const enum CBLAS_ORDER Order, const enum CBLAS_UPLO Uplo,
                   const int N, const int NRHS, const float *A, const int lda,
                   float *B, const int ldb);
int clapack_spotri(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                   const int N, float *A, const int lda);
int clapack_slauum(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                   const int N, float *A, const int lda);
int clapack_strtri(const enum ATLAS_ORDER Order,const enum ATLAS_UPLO Uplo,
                  const enum ATLAS_DIAG Diag,const int N, float *A, const int lda);

int clapack_dgesv(const enum CBLAS_ORDER Order, const int N, const int NRHS,
                  double *A, const int lda, int *ipiv,
                  double *B, const int ldb);
int clapack_dgetrf(const enum CBLAS_ORDER Order, const int M, const int N,
                   double *A, const int lda, int *ipiv);
int clapack_dgetrs
   (const enum CBLAS_ORDER Order, const enum CBLAS_TRANSPOSE Trans,
    const int N, const int NRHS, const double *A, const int lda,
    const int *ipiv, double *B, const int ldb);
int clapack_dgetri(const enum CBLAS_ORDER Order, const int N, double *A,
                   const int lda, const int *ipiv);
int clapack_dposv(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                  const int N, const int NRHS, double *A, const int lda,
                  double *B, const int ldb);
int clapack_dpotrf(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                   const int N, double *A, const int lda);
int clapack_dpotrs(const enum CBLAS_ORDER Order, const enum CBLAS_UPLO Uplo,
                   const int N, const int NRHS, const double *A, const int lda,
                   double *B, const int ldb);
int clapack_dpotri(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                   const int N, double *A, const int lda);
int clapack_dlauum(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                   const int N, double *A, const int lda);
int clapack_dtrtri(const enum ATLAS_ORDER Order,const enum ATLAS_UPLO Uplo,
                  const enum ATLAS_DIAG Diag,const int N, double *A, const int lda);

int clapack_cgesv(const enum CBLAS_ORDER Order, const int N, const int NRHS,
                  void *A, const int lda, int *ipiv,
                  void *B, const int ldb);
int clapack_cgetrf(const enum CBLAS_ORDER Order, const int M, const int N,
                   void *A, const int lda, int *ipiv);
int clapack_cgetrs
   (const enum CBLAS_ORDER Order, const enum CBLAS_TRANSPOSE Trans,
    const int N, const int NRHS, const void *A, const int lda,
    const int *ipiv, void *B, const int ldb);
int clapack_cgetri(const enum CBLAS_ORDER Order, const int N, void *A,
                   const int lda, const int *ipiv);
int clapack_cposv(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                  const int N, const int NRHS, void *A, const int lda,
                  void *B, const int ldb);
int clapack_cpotrf(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                   const int N, void *A, const int lda);
int clapack_cpotrs(const enum CBLAS_ORDER Order, const enum CBLAS_UPLO Uplo,
                   const int N, const int NRHS, const void *A, const int lda,
                   void *B, const int ldb);
int clapack_cpotri(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                   const int N, void *A, const int lda);
int clapack_clauum(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                   const int N, void *A, const int lda);
int clapack_ctrtri(const enum ATLAS_ORDER Order,const enum ATLAS_UPLO Uplo,
                  const enum ATLAS_DIAG Diag,const int N, void *A, const int lda);

int clapack_zgesv(const enum CBLAS_ORDER Order, const int N, const int NRHS,
                  void *A, const int lda, int *ipiv,
                  void *B, const int ldb);
int clapack_zgetrf(const enum CBLAS_ORDER Order, const int M, const int N,
                   void *A, const int lda, int *ipiv);
int clapack_zgetrs
   (const enum CBLAS_ORDER Order, const enum CBLAS_TRANSPOSE Trans,
    const int N, const int NRHS, const void *A, const int lda,
    const int *ipiv, void *B, const int ldb);
int clapack_zgetri(const enum CBLAS_ORDER Order, const int N, void *A,
                   const int lda, const int *ipiv);
int clapack_zposv(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                  const int N, const int NRHS, void *A, const int lda,
                  void *B, const int ldb);
int clapack_zpotrf(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                   const int N, void *A, const int lda);
int clapack_zpotrs(const enum CBLAS_ORDER Order, const enum CBLAS_UPLO Uplo,
                   const int N, const int NRHS, const void *A, const int lda,
                   void *B, const int ldb);
int clapack_zpotri(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                   const int N, void *A, const int lda);
int clapack_zlauum(const enum ATLAS_ORDER Order, const enum ATLAS_UPLO Uplo,
                   const int N, void *A, const int lda);
int clapack_ztrtri(const enum ATLAS_ORDER Order,const enum ATLAS_UPLO Uplo,
                  const enum ATLAS_DIAG Diag,const int N, void *A, const int lda);

#endif
