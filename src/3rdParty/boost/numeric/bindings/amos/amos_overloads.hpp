//
//  Copyright Toon Knapen
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NUMERIC_BINDINGS_AMOS_AMOS_OVERLOADS_HPP
#define BOOST_NUMERIC_BINDINGS_AMOS_AMOS_OVERLOADS_HPP

#include <boost/numeric/bindings/amos/amos.h>
#include <boost/numeric/bindings/traits/type.hpp>
#include <boost/numeric/bindings/traits/type_traits.hpp>

namespace boost { namespace numeric { namespace bindings { namespace amos { namespace detail {

  using namespace ::boost::numeric::bindings::traits ;

  //
  // BESI
  //

  //inline
  //void besi(const fcomplex * z, const fcomplex * fnu, const int * kode, const int * n, fcomplex* cy, int * nz, int * error) ;

  inline
  void besi(const double& z, const double& fnu, const int& kode, const int& n, double* cy, int& nz, int& error) 
  { AMOS_DBESI( &z, &fnu, &kode, &n, cy, &nz ) ; }

  inline
  void besi(const complex_f& z, const float&  fnu, const int& kode, const int& n, complex_f* cy, int & nz, int & error) 
  { AMOS_CBESI( complex_ptr( &z ), &fnu, &kode, &n, complex_ptr( cy ), &nz, &error ) ; }

  // inline
  // void besi(const complex_d* z, const double* fnu, const int * kode, const int * n, complex_d* cy, int * nz, int * error)  ;
  
  //
  // BESJ
  //
  
  inline
  void besj(const double& z, const double& fnu, const int& kode, const int& n, double* cy, int& nz, int& error) 
  { AMOS_DBESJ( &z, &fnu, &n, cy, &nz ) ; }

  inline
  void besj(const complex_f& z, const float&  fnu, const int& kode, const int& n, complex_f* cy, int & nz, int & error) 
  { AMOS_CBESJ( complex_ptr( &z ), &fnu, &kode, &n, complex_ptr( cy ), &nz, &error ) ; }

  

  //
  // BESY
  //
  
  inline
  void besy(const double& z, const double& fnu, const int& kode, const int& n, double* cy, int& nz, double* wrk, int& error) 
  { AMOS_DBESY( &z, &fnu, &n, cy ) ; }

}}}}}

#endif // BOOST_NUMERIC_BINDINGS_AMOS_AMOS_OVERLOADS_HPP
