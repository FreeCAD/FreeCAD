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

#ifndef BOOST_NUMERIC_BINDINGS_CBLAS1_OVERLOADS_HPP
#define BOOST_NUMERIC_BINDINGS_CBLAS1_OVERLOADS_HPP

#include <complex> 
#include <boost/numeric/bindings/atlas/cblas_inc.hpp>
#include <boost/numeric/bindings/traits/type.hpp>


namespace boost { namespace numeric { namespace bindings { 

  namespace atlas { namespace detail {

    // dot <- x^T * y
    // .. real types:    calls cblas_xdot
    // .. complex types: calls cblas_xdotu
    inline 
    float dot (int const N, float const* X, int const incX,
               float const* Y, int const incY) {
      return cblas_sdot (N, X, incX, Y, incY); 
    }
    inline 
    double dot (int const N, double const* X, int const incX,
                double const* Y, int const incY) {
      return cblas_ddot (N, X, incX, Y, incY); 
    }
    inline 
    traits::complex_f 
    dot (int const N, traits::complex_f const* X, int const incX,
         traits::complex_f const* Y, int const incY) {
      traits::complex_f val; 
      cblas_cdotu_sub (N, 
                       static_cast<void const*> (X), incX, 
                       static_cast<void const*> (Y), incY, 
                       static_cast<void*> (&val)); 
      return val; 
    }
    inline 
    traits::complex_d 
    dot (int const N, traits::complex_d const* X, int const incX,
         traits::complex_d const* Y, int const incY) {
      traits::complex_d val; 
      cblas_zdotu_sub (N, 
                       static_cast<void const*> (X), incX, 
                       static_cast<void const*> (Y), incY, 
                       static_cast<void*> (&val)); 
      return val; 
    }

    // dotu <- x^T * y  
    // .. complex types only
    inline 
    void dotu (int const N, traits::complex_f const* X, int const incX,
               traits::complex_f const* Y, int const incY,
               traits::complex_f* val) 
    {
      cblas_cdotu_sub (N, 
                       static_cast<void const*> (X), incX, 
                       static_cast<void const*> (Y), incY, 
                       static_cast<void*> (val)); 
    }
    inline 
    void dotu (int const N, traits::complex_d const* X, int const incX,
               traits::complex_d const* Y, int const incY,
               traits::complex_d* val) 
    {
      cblas_zdotu_sub (N, 
                       static_cast<void const*> (X), incX, 
                       static_cast<void const*> (Y), incY, 
                       static_cast<void*> (val)); 
    }

    // dotc <- x^H * y  
    // .. complex types only 
    inline 
    void dotc (int const N, traits::complex_f const* X, int const incX,
               traits::complex_f const* Y, int const incY,
               traits::complex_f* val) 
    {
      cblas_cdotc_sub (N, 
                       static_cast<void const*> (X), incX, 
                       static_cast<void const*> (Y), incY, 
                       static_cast<void*> (val)); 
    }
    inline 
    void dotc (int const N, traits::complex_d const* X, int const incX,
               traits::complex_d const* Y, int const incY,
               traits::complex_d* val) 
    {
      cblas_zdotc_sub (N, 
                       static_cast<void const*> (X), incX, 
                       static_cast<void const*> (Y), incY, 
                       static_cast<void*> (val)); 
    }

    // nrm2 <- ||x||_2
    inline 
    float nrm2 (int const N, float const* X, int const incX) {
      return cblas_snrm2 (N, X, incX);
    }
    inline 
    double nrm2 (int const N, double const* X, int const incX) {
      return cblas_dnrm2 (N, X, incX);
    }
    inline 
    float nrm2 (int const N, traits::complex_f const* X, int const incX) {
      return cblas_scnrm2 (N, static_cast<void const*> (X), incX);
    }
    inline 
    double nrm2 (int const N, traits::complex_d const* X, int const incX) {
      return cblas_dznrm2 (N, static_cast<void const*> (X), incX);
    }

    // asum <- ||re (x)|| + ||im (x)||
    inline 
    float asum (int const N, float const* X, int const incX) {
      return cblas_sasum (N, X, incX);
    }
    inline 
    double asum (int const N, double const* X, int const incX) {
      return cblas_dasum (N, X, incX);
    }
    inline 
    float asum (int const N, traits::complex_f const* X, int const incX) {
      return cblas_scasum (N, static_cast<void const*> (X), incX);
    }
    inline 
    double asum (int const N, traits::complex_d const* X, int const incX) {
      return cblas_dzasum (N, static_cast<void const*> (X), incX);
    }

    // iamax <- 1st i: max (|re (x_i)| + |im (x_i)|)
    inline 
    CBLAS_INDEX iamax (int const N, float const* X, int const incX) {
      return cblas_isamax (N, X, incX);
    }
    inline 
    CBLAS_INDEX iamax (int const N, double const* X, int const incX) {
      return cblas_idamax (N, X, incX);
    }
    inline 
    CBLAS_INDEX 
    iamax (int const N, traits::complex_f const* X, int const incX) {
      return cblas_icamax (N, static_cast<void const*> (X), incX);
    }
    inline 
    CBLAS_INDEX 
    iamax (int const N, traits::complex_d const* X, int const incX) {
      return cblas_izamax (N, static_cast<void const*> (X), incX);
    }

    // x <-> y
    inline 
    void swap (int const N, float* X, int const incX,
               float* Y, int const incY) {
      cblas_sswap (N, X, incX, Y, incY); 
    }
    inline 
    void swap (int const N, double* X, int const incX,
               double* Y, int const incY) {
      cblas_dswap (N, X, incX, Y, incY); 
    }
    inline 
    void swap (int const N, traits::complex_f* X, int const incX,
               traits::complex_f* Y, int const incY) {
      cblas_cswap (N, 
                   static_cast<void*> (X), incX, 
                   static_cast<void*> (Y), incY); 
    }
    inline 
    void swap (int const N, traits::complex_d* X, int const incX,
               traits::complex_d* Y, int const incY) {
      cblas_zswap (N, 
                   static_cast<void*> (X), incX, 
                   static_cast<void*> (Y), incY); 
    }

    // y <- x
    inline
    void copy (int const N, float const* X, int const incX,
               float* Y, int const incY) {
      cblas_scopy (N, X, incX, Y, incY); 
    }
    inline
    void copy (int const N, double const* X, int const incX,
               double* Y, int const incY) {
      cblas_dcopy (N, X, incX, Y, incY); 
    }
    inline 
    void copy (int const N, traits::complex_f const* X, int const incX,
               traits::complex_f* Y, int const incY) {
      cblas_ccopy (N, 
                   static_cast<void const*> (X), incX, 
                   static_cast<void*> (Y), incY); 
    }
    inline 
    void copy (int const N, traits::complex_d const* X, int const incX,
               traits::complex_d* Y, int const incY) {
      cblas_zcopy (N, 
                   static_cast<void const*> (X), incX, 
                   static_cast<void*> (Y), incY); 
    }

    // y <- alpha * x + y
    inline
    void axpy (int const N, 
               float const alpha, float const* X, int const incX, 
               float* Y, int const incY) 
    {
      cblas_saxpy (N, alpha, X, incX, Y, incY);
    }
    inline
    void axpy (int const N, 
               double const alpha, double const* X, int const incX, 
               double* Y, int const incY) 
    {
      cblas_daxpy (N, alpha, X, incX, Y, incY);
    }
    inline
    void axpy (int const N, 
               traits::complex_f const& alpha, 
               traits::complex_f const* X, int const incX, 
               traits::complex_f* Y, int const incY) 
    {
      cblas_caxpy (N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (X), incX, 
                   static_cast<void*> (Y), incY);
    }
    inline
    void axpy (int const N, 
               traits::complex_d const& alpha, 
               traits::complex_d const* X, int const incX, 
               traits::complex_d* Y, int const incY) 
    {
      cblas_zaxpy (N, 
                   static_cast<void const*> (&alpha), 
                   static_cast<void const*> (X), incX, 
                   static_cast<void*> (Y), incY);
    }

    // y <- alpha * x + beta * y
    inline
    void axpby (int const N, 
                float const alpha, float const* X, int const incX, 
                float const beta, float* Y, int const incY) 
    {
      catlas_saxpby (N, alpha, X, incX, beta, Y, incY);
    }
    inline
    void axpby (int const N, 
                double const alpha, double const* X, int const incX, 
                double const beta, double* Y, int const incY) 
    {
      catlas_daxpby (N, alpha, X, incX, beta, Y, incY);
    }
    inline
    void axpby (int const N, 
                traits::complex_f const& alpha, 
                traits::complex_f const* X, int const incX, 
                traits::complex_f const& beta, 
                traits::complex_f* Y, int const incY) 
    {
      catlas_caxpby (N, 
                     static_cast<void const*> (&alpha), 
                     static_cast<void const*> (X), incX, 
                     static_cast<void const*> (&beta), 
                     static_cast<void*> (Y), incY);
    }
    inline
    void axpby (int const N, 
                traits::complex_d const& alpha, 
                traits::complex_d const* X, int const incX, 
                traits::complex_d const& beta, 
                traits::complex_d* Y, int const incY) 
    {
      catlas_zaxpby (N, 
                     static_cast<void const*> (&alpha), 
                     static_cast<void const*> (X), incX, 
                     static_cast<void const*> (&beta), 
                     static_cast<void*> (Y), incY);
    }

    // x_i <- alpha for all i
    inline
    void set (int const N, float const alpha, float* X, int const incX) {
      catlas_sset (N, alpha, X, incX); 
    }
    inline
    void set (int const N, double const alpha, double* X, int const incX) {
      catlas_dset (N, alpha, X, incX); 
    }
    inline
    void set (int const N, traits::complex_f const& alpha, 
              traits::complex_f* X, int const incX) {
      catlas_cset (N, static_cast<void const*> (&alpha), 
                   static_cast<void*> (X), incX);
    }
    inline
    void set (int const N, traits::complex_d const& alpha, 
              traits::complex_d* X, int const incX) {
      catlas_zset (N, static_cast<void const*> (&alpha), 
                   static_cast<void*> (X), incX);
    }

    // x <- alpha * x
    inline
    void scal (int const N, float const alpha, float* X, int const incX) {
      cblas_sscal (N, alpha, X, incX); 
    }
    inline
    void scal (int const N, double const alpha, double* X, int const incX) {
      cblas_dscal (N, alpha, X, incX); 
    }
    inline
    void scal (int const N, traits::complex_f const& alpha, 
               traits::complex_f* X, int const incX) {
      cblas_cscal (N, static_cast<void const*> (&alpha), 
                   static_cast<void*> (X), incX);
    }
    inline
    void scal (int const N, float const alpha, 
               traits::complex_f* X, int const incX) {
      cblas_csscal (N, alpha, static_cast<void*> (X), incX);
    }
    inline
    void scal (int const N, traits::complex_d const& alpha, 
               traits::complex_d* X, int const incX) {
      cblas_zscal (N, static_cast<void const*> (&alpha), 
                   static_cast<void*> (X), incX);
    }
    inline
    void scal (int const N, double const alpha, 
               traits::complex_d* X, int const incX) {
      cblas_zdscal (N, alpha, static_cast<void*> (X), incX);
    }

  }} // namepaces detail & atlas

}}} 


#endif // BOOST_NUMERIC_BINDINGS_CBLAS1_OVERLOADS_HPP
