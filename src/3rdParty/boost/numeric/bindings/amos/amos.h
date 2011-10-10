//
//  Copyright Toon Knapen
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NUMERIC_BINDINGS_AMOS_AMOS_H
#define BOOST_NUMERIC_BINDINGS_AMOS_AMOS_H

#include <boost/numeric/bindings/amos/amos_names.h>
#include <boost/numeric/bindings/traits/type.h>

extern "C"
{
  void AMOS_DBESI(const double*     z,                const double* fnu, const int* kode, const int* n, double*     cy,           int* nz           ) ;
  void AMOS_CBESI(const fcomplex_t* z,                const float*  fnu, const int* kode, const int* n, fcomplex_t* cy,           int* nz, int* ierr) ;
  void AMOS_ZBESI(const double* zr, const double* zi, const double* fnu, const int* kode, const int* n, double* cyr, double* cyi, int* nz, int* ierr) ;

  void AMOS_DBESJ(const double  * z,                  const double* fnu,                  const int* n, double*     cy,             int* nz           ) ;
  void AMOS_CBESJ(const fcomplex_t* z,                const float*  fnu, const int* kode, const int* n, fcomplex_t* cy,             int* nz, int* ierr) ;
  void AMOS_ZBESJ(const double* zr, const double* zi, const double* fnu, const int* kode, const int* n, double* cyr, double* cyi, int* nz, int* ierr) ;

  void AMOS_DBESY(const double  * z,                  const double* fnu,                  const int* n, double*     cy                                                            ) ;
  void AMOS_CBESY(const fcomplex_t* z,                const float*  fnu, const int* kode, const int* n, fcomplex_t* cy,           int* nz, fcomplex_t *cwrk,             int* ierr) ;
  void AMOS_ZBESY(const double* zr, const double* zi, const double* fnu, const int* kode, const int* n, double* cyr, double* cyi, int* nz, double *cwrkr, double *cwrki, int* ierr) ;
}

#endif // BOOST_NUMERIC_BINDINGS_AMOS_AMOS_HPP
