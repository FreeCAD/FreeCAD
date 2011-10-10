/*
 * 
 * Copyright (c) Toon Knapen & Kresimir Fresl 2003
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * KF acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_LAPACK_NAMES_H
#define BOOST_NUMERIC_BINDINGS_LAPACK_LAPACK_NAMES_H

#ifndef BOOST_NUMERIC_BINDINGS_USE_CLAPACK
#  include <boost/numeric/bindings/traits/fortran.h>
#else
#  define FORTRAN_ID( id ) id##_
#endif 

/* linear systems */

/* general */

#define LAPACK_SGESV FORTRAN_ID( sgesv )
#define LAPACK_DGESV FORTRAN_ID( dgesv )
#define LAPACK_CGESV FORTRAN_ID( cgesv )
#define LAPACK_ZGESV FORTRAN_ID( zgesv )

#define LAPACK_SGETRF FORTRAN_ID( sgetrf )
#define LAPACK_DGETRF FORTRAN_ID( dgetrf )
#define LAPACK_CGETRF FORTRAN_ID( cgetrf )
#define LAPACK_ZGETRF FORTRAN_ID( zgetrf )

#define LAPACK_SGETRS FORTRAN_ID( sgetrs )
#define LAPACK_DGETRS FORTRAN_ID( dgetrs )
#define LAPACK_CGETRS FORTRAN_ID( cgetrs )
#define LAPACK_ZGETRS FORTRAN_ID( zgetrs )

#define LAPACK_SGETRI FORTRAN_ID( sgetri )
#define LAPACK_DGETRI FORTRAN_ID( dgetri )
#define LAPACK_CGETRI FORTRAN_ID( cgetri )
#define LAPACK_ZGETRI FORTRAN_ID( zgetri )


/* symmetric/Hermitian positive definite */

#define LAPACK_SPOSV FORTRAN_ID( sposv )
#define LAPACK_DPOSV FORTRAN_ID( dposv )
#define LAPACK_CPOSV FORTRAN_ID( cposv )
#define LAPACK_ZPOSV FORTRAN_ID( zposv )

#define LAPACK_SPOTRF FORTRAN_ID( spotrf )
#define LAPACK_DPOTRF FORTRAN_ID( dpotrf )
#define LAPACK_CPOTRF FORTRAN_ID( cpotrf )
#define LAPACK_ZPOTRF FORTRAN_ID( zpotrf )

#define LAPACK_SPOTRS FORTRAN_ID( spotrs )
#define LAPACK_DPOTRS FORTRAN_ID( dpotrs )
#define LAPACK_CPOTRS FORTRAN_ID( cpotrs )
#define LAPACK_ZPOTRS FORTRAN_ID( zpotrs )

#define LAPACK_SPOTRI FORTRAN_ID( spotri )
#define LAPACK_DPOTRI FORTRAN_ID( dpotri )
#define LAPACK_CPOTRI FORTRAN_ID( cpotri )
#define LAPACK_ZPOTRI FORTRAN_ID( zpotri )


/* symmetric/Hermitian positive definite in packed storage */

#define LAPACK_SPPSV FORTRAN_ID( sppsv )
#define LAPACK_DPPSV FORTRAN_ID( dppsv )
#define LAPACK_CPPSV FORTRAN_ID( cppsv )
#define LAPACK_ZPPSV FORTRAN_ID( zppsv )

#define LAPACK_SPPTRF FORTRAN_ID( spptrf )
#define LAPACK_DPPTRF FORTRAN_ID( dpptrf )
#define LAPACK_CPPTRF FORTRAN_ID( cpptrf )
#define LAPACK_ZPPTRF FORTRAN_ID( zpptrf )

#define LAPACK_SPPTRS FORTRAN_ID( spptrs )
#define LAPACK_DPPTRS FORTRAN_ID( dpptrs )
#define LAPACK_CPPTRS FORTRAN_ID( cpptrs )
#define LAPACK_ZPPTRS FORTRAN_ID( zpptrs )

#define LAPACK_SPPTRI FORTRAN_ID( spptri )
#define LAPACK_DPPTRI FORTRAN_ID( dpptri )
#define LAPACK_CPPTRI FORTRAN_ID( cpptri )
#define LAPACK_ZPPTRI FORTRAN_ID( zpptri )


/* symmetric/Hermitian indefinite and complex symmetric */

#define LAPACK_SSYSV FORTRAN_ID( ssysv )
#define LAPACK_DSYSV FORTRAN_ID( dsysv )
#define LAPACK_CSYSV FORTRAN_ID( csysv )
#define LAPACK_ZSYSV FORTRAN_ID( zsysv )
#define LAPACK_CHESV FORTRAN_ID( chesv )
#define LAPACK_ZHESV FORTRAN_ID( zhesv )

#define LAPACK_SSYTRF FORTRAN_ID( ssytrf )
#define LAPACK_DSYTRF FORTRAN_ID( dsytrf )
#define LAPACK_CSYTRF FORTRAN_ID( csytrf )
#define LAPACK_ZSYTRF FORTRAN_ID( zsytrf )
#define LAPACK_CHETRF FORTRAN_ID( chetrf )
#define LAPACK_ZHETRF FORTRAN_ID( zhetrf )

#define LAPACK_SSYTRS FORTRAN_ID( ssytrs )
#define LAPACK_DSYTRS FORTRAN_ID( dsytrs )
#define LAPACK_CSYTRS FORTRAN_ID( csytrs )
#define LAPACK_ZSYTRS FORTRAN_ID( zsytrs )
#define LAPACK_CHETRS FORTRAN_ID( chetrs )
#define LAPACK_ZHETRS FORTRAN_ID( zhetrs )


/* symmetric/Hermitian indefinite and complex symmetric in packed storage */

#define LAPACK_SSPSV FORTRAN_ID( sspsv )
#define LAPACK_DSPSV FORTRAN_ID( dspsv )
#define LAPACK_CSPSV FORTRAN_ID( cspsv )
#define LAPACK_ZSPSV FORTRAN_ID( zspsv )
#define LAPACK_CHPSV FORTRAN_ID( chpsv )
#define LAPACK_ZHPSV FORTRAN_ID( zhpsv )

#define LAPACK_SSPTRF FORTRAN_ID( ssptrf )
#define LAPACK_DSPTRF FORTRAN_ID( dsptrf )
#define LAPACK_CSPTRF FORTRAN_ID( csptrf )
#define LAPACK_ZSPTRF FORTRAN_ID( zsptrf )
#define LAPACK_CHPTRF FORTRAN_ID( chptrf )
#define LAPACK_ZHPTRF FORTRAN_ID( zhptrf )

#define LAPACK_SSPTRS FORTRAN_ID( ssptrs )
#define LAPACK_DSPTRS FORTRAN_ID( dsptrs )
#define LAPACK_CSPTRS FORTRAN_ID( csptrs )
#define LAPACK_ZSPTRS FORTRAN_ID( zsptrs )
#define LAPACK_CHPTRS FORTRAN_ID( chptrs )
#define LAPACK_ZHPTRS FORTRAN_ID( zhptrs )


/********************************************/
/* eigenproblems */ 

#define LAPACK_SGEES FORTRAN_ID( sgees )
#define LAPACK_DGEES FORTRAN_ID( dgees )
#define LAPACK_CGEES FORTRAN_ID( cgees )
#define LAPACK_ZGEES FORTRAN_ID( zgees )

#define LAPACK_SGEEV FORTRAN_ID( sgeev )
#define LAPACK_DGEEV FORTRAN_ID( dgeev )
#define LAPACK_CGEEV FORTRAN_ID( cgeev )
#define LAPACK_ZGEEV FORTRAN_ID( zgeev )

#define LAPACK_SSYEV FORTRAN_ID( ssyev )
#define LAPACK_DSYEV FORTRAN_ID( dsyev )
#define LAPACK_CHEEV FORTRAN_ID( cheev )
#define LAPACK_ZHEEV FORTRAN_ID( zheev )

#define LAPACK_SSYEVD FORTRAN_ID( ssyevd )
#define LAPACK_DSYEVD FORTRAN_ID( dsyevd )
#define LAPACK_CHEEVD FORTRAN_ID( cheevd )
#define LAPACK_ZHEEVD FORTRAN_ID( zheevd )

#define LAPACK_SSYEVX FORTRAN_ID( ssyevx )
#define LAPACK_DSYEVX FORTRAN_ID( dsyevx )
#define LAPACK_CHEEVX FORTRAN_ID( cheevx )
#define LAPACK_ZHEEVX FORTRAN_ID( zheevx )


#define LAPACK_STREVC FORTRAN_ID( strevc )
#define LAPACK_DTREVC FORTRAN_ID( dtrevc )
#define LAPACK_CTREVC FORTRAN_ID( ctrevc )
#define LAPACK_ZTREVC FORTRAN_ID( ztrevc )

#define LAPACK_STREXC FORTRAN_ID( strexc )
#define LAPACK_DTREXC FORTRAN_ID( dtrexc )
#define LAPACK_CTREXC FORTRAN_ID( ctrexc )
#define LAPACK_ZTREXC FORTRAN_ID( ztrexc )


/********************************************/
/* eigenproblems for Hessenberg matrices */ 

#define LAPACK_SHSEQR FORTRAN_ID( shseqr )
#define LAPACK_DHSEQR FORTRAN_ID( dhseqr )
#define LAPACK_CHSEQR FORTRAN_ID( chseqr )
#define LAPACK_ZHSEQR FORTRAN_ID( zhseqr )

/********************************************/
/* eigenproblems for banded matrices */ 

#define LAPACK_SSBEV FORTRAN_ID( ssbev )
#define LAPACK_DSBEV FORTRAN_ID( dsbev )
#define LAPACK_CHBEV FORTRAN_ID( chbev )
#define LAPACK_ZHBEV FORTRAN_ID( zhbev )

#define LAPACK_SSBEVX FORTRAN_ID( ssbevx )
#define LAPACK_DSBEVX FORTRAN_ID( dsbevx )
#define LAPACK_CHBEVX FORTRAN_ID( chbevx )
#define LAPACK_ZHBEVX FORTRAN_ID( zhbevx )


/********************************************/
/* eigenproblems for tridiagonal matrices */ 

#define LAPACK_SSTEQR FORTRAN_ID( ssteqr )
#define LAPACK_DSTEQR FORTRAN_ID( dsteqr )


/********************************************/
/* QR factorization */

#define LAPACK_SGEQRF FORTRAN_ID( sgeqrf )
#define LAPACK_DGEQRF FORTRAN_ID( dgeqrf )
#define LAPACK_CGEQRF FORTRAN_ID( cgeqrf )
#define LAPACK_ZGEQRF FORTRAN_ID( zgeqrf )

// Apply orthogonal transformation

#define LAPACK_SORMQR FORTRAN_ID( sormqr )
#define LAPACK_DORMQR FORTRAN_ID( dormqr )
#define LAPACK_CUNMQR FORTRAN_ID( cunmqr )
#define LAPACK_ZUNMQR FORTRAN_ID( zunmqr )

#define LAPACK_SORGQR FORTRAN_ID( sorgqr )
#define LAPACK_DORGQR FORTRAN_ID( dorgqr )
#define LAPACK_CUNGQR FORTRAN_ID( cungqr )
#define LAPACK_ZUNGQR FORTRAN_ID( zungqr )

#define LAPACK_SSYTRD FORTRAN_ID( ssytrd )
#define LAPACK_DSYTRD FORTRAN_ID( dsytrd )


/********************************************/
/* SVD */

#define LAPACK_SGESVD FORTRAN_ID( sgesvd )
#define LAPACK_DGESVD FORTRAN_ID( dgesvd )
#define LAPACK_CGESVD FORTRAN_ID( cgesvd )
#define LAPACK_ZGESVD FORTRAN_ID( zgesvd )

#define LAPACK_SGESDD FORTRAN_ID( sgesdd )
#define LAPACK_DGESDD FORTRAN_ID( dgesdd )
#define LAPACK_CGESDD FORTRAN_ID( cgesdd )
#define LAPACK_ZGESDD FORTRAN_ID( zgesdd )


/********************************************/
/* auxiliary */ 

#define LAPACK_ILAENV FORTRAN_ID( ilaenv )


#endif 

