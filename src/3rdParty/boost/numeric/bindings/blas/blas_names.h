//
//  Copyright (C) 2002, 2003 Si-Lab b.v.b.a and Toon Knapen 
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NUMERIC_BINDINGS_BLAS_BLAS_NAMES_H
#define BOOST_NUMERIC_BINDINGS_BLAS_BLAS_NAMES_H

#include <boost/numeric/bindings/traits/fortran.h>

//
// level 1
//
#define BLAS_SSCAL FORTRAN_ID( sscal )
#define BLAS_DSCAL FORTRAN_ID( dscal )
#define BLAS_CSCAL FORTRAN_ID( cscal )
#define BLAS_ZSCAL FORTRAN_ID( zscal )

#define BLAS_SAXPY FORTRAN_ID( saxpy )
#define BLAS_DAXPY FORTRAN_ID( daxpy )
#define BLAS_CAXPY FORTRAN_ID( caxpy )
#define BLAS_ZAXPY FORTRAN_ID( zaxpy )

#define BLAS_SDOT  FORTRAN_ID( sdot )
#define BLAS_DDOT  FORTRAN_ID( ddot )

#define BLAS_CDOTU FORTRAN_ID( cdotu )
#define BLAS_ZDOTU FORTRAN_ID( zdotu )

#define BLAS_CDOTC FORTRAN_ID( cdotc )
#define BLAS_ZDOTC FORTRAN_ID( zdotc )

#define BLAS_SNRM2 FORTRAN_ID( snrm2 )
#define BLAS_DNRM2 FORTRAN_ID( dnrm2 )
#define BLAS_SCNRM2 FORTRAN_ID( scnrm2 )
#define BLAS_DZNRM2 FORTRAN_ID( dznrm2 )

#define BLAS_SASUM FORTRAN_ID( sasum )
#define BLAS_DASUM FORTRAN_ID( dasum )
#define BLAS_SCASUM FORTRAN_ID( scasum )
#define BLAS_DZASUM FORTRAN_ID( dzasum )

#define BLAS_SCOPY FORTRAN_ID( scopy )
#define BLAS_DCOPY FORTRAN_ID( dcopy )
#define BLAS_CCOPY FORTRAN_ID( ccopy )
#define BLAS_ZCOPY FORTRAN_ID( zcopy )

//
// level 2
//
#define BLAS_SGEMV FORTRAN_ID( sgemv )
#define BLAS_DGEMV FORTRAN_ID( dgemv )
#define BLAS_CGEMV FORTRAN_ID( cgemv )
#define BLAS_ZGEMV FORTRAN_ID( zgemv )

#define BLAS_SGER  FORTRAN_ID( sger )
#define BLAS_DGER  FORTRAN_ID( dger )

#define BLAS_CGERU FORTRAN_ID( cgeru )
#define BLAS_ZGERU FORTRAN_ID( zgeru )

#define BLAS_CGERC FORTRAN_ID( cgerc )
#define BLAS_ZGERC FORTRAN_ID( zgerc )

//
// level 3
//
#define BLAS_SGEMM FORTRAN_ID( sgemm )
#define BLAS_DGEMM FORTRAN_ID( dgemm )
#define BLAS_CGEMM FORTRAN_ID( cgemm )
#define BLAS_ZGEMM FORTRAN_ID( zgemm )

#define BLAS_SSYRK FORTRAN_ID( ssyrk )
#define BLAS_DSYRK FORTRAN_ID( dsyrk )
#define BLAS_CSYRK FORTRAN_ID( csyrk )
#define BLAS_ZSYRK FORTRAN_ID( zsyrk )
#define BLAS_CHERK FORTRAN_ID( cherk )
#define BLAS_ZHERK FORTRAN_ID( zherk )

#define BLAS_STRSM FORTRAN_ID( strsm )
#define BLAS_DTRSM FORTRAN_ID( dtrsm )
#define BLAS_CTRSM FORTRAN_ID( ctrsm )
#define BLAS_ZTRSM FORTRAN_ID( ztrsm )

#endif // BOOST_NUMERIC_BINDINGS_BLAS_BLAS_NAMES_H
