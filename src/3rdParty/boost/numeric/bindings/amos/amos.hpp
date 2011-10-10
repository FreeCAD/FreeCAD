//
//  Copyright Toon Knapen
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NUMERIC_BINDINGS_AMOS_AMOS_HPP
#define BOOST_NUMERIC_BINDINGS_AMOS_AMOS_HPP

#include <boost/numeric/bindings/amos/amos_overloads.hpp>
#include <boost/numeric/bindings/traits/type_traits.hpp>
#include <boost/numeric/bindings/traits/vector_traits.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>

namespace boost { namespace numeric { namespace bindings { namespace amos {

  template < typename vector_type, typename value_type >
  int besi(const value_type&                                             z,   // std::complex< float > or std::complex< double >
           const typename traits::type_traits< value_type >::real_type   fnu, // float or double
           int                                                           kode, 
           vector_type&                                                  cy, 
           int&                                                          nz) 
  {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
    BOOST_STATIC_ASSERT( ( boost::is_same< value_type, typename traits::vector_traits<vector_type>::value_type >::value ) ) ;
#else
    BOOST_STATIC_ASSERT( ( boost::is_same< value_type, typename vector_type::value_type >::value ) ) ;
#endif

    int n = traits::vector_size( cy ) ;
    value_type * cy_ptr = traits::vector_storage( cy ) ;

    int error = 0 ;
    detail::besi( z, fnu, kode, n, cy_ptr, nz, error ) ;
    return error ;
  }

  template < typename vector_type, typename value_type >
  int besj(const value_type&                                            z, 
           const typename traits::type_traits< value_type >::real_type  fnu, 
           int                                                          kode, 
           vector_type&                                                 cy, 
           int&                                                         nz) 
  {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
    BOOST_STATIC_ASSERT( ( boost::is_same< value_type, typename traits::vector_traits<vector_type>::value_type >::value ) ) ;
#else
    BOOST_STATIC_ASSERT( ( boost::is_same< value_type, typename vector_type::value_type >::value ) ) ;
#endif

    int n = traits::vector_size( cy ) ;
    value_type * cy_ptr = traits::vector_storage( cy ) ;

    int error = 0 ;
    detail::besj( z, fnu, kode, n, cy_ptr, nz, error ) ;
    return error ;
  }

  template < typename vector_type, typename value_type >
  int besy(const value_type&                                           z, 
           const typename traits::type_traits< value_type >::real_type fnu, 
           int                                                         kode, 
           vector_type&                                                cy, 
           int&                                                        nz) 
  {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
    BOOST_STATIC_ASSERT( ( boost::is_same< value_type, typename traits::vector_traits<vector_type>::value_type >::value ) ) ;
#else
    BOOST_STATIC_ASSERT( ( boost::is_same< value_type, typename vector_type::value_type >::value ) ) ;
#endif

    int n = traits::vector_size( cy ) ;
    value_type * cy_ptr = traits::vector_storage( cy ) ;

    int error = 0 ;
    value_type * cwrk = new value_type[n];
    detail::besy( z, fnu, kode, n, cy_ptr, nz, cwrk, error ) ;
    delete[] cwrk ;
    return error ;
  }

}}}}

#endif // BOOST_NUMERIC_BINDINGS_AMOS_AMOS_HPP
