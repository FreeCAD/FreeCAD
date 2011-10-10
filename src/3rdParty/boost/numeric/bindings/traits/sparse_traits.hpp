/*
 * 
 * Copyright (c) 2003 Kresimir Fresl, Toon Knapen and Karl Meerbergen
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * KF author acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_SPARSE_TRAITS_H
#define BOOST_NUMERIC_BINDINGS_TRAITS_SPARSE_TRAITS_H

#include <boost/numeric/bindings/traits/config.hpp> 

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#include <boost/numeric/bindings/traits/traits.hpp>

namespace boost { namespace numeric { namespace bindings { namespace traits {

  /// There is no default implementation since there is no reasonable default. 
  /// Most matrix libraries provide totally different functions.

  /// Auxiliary traits class to reduce the number of specializations.
  template <typename MIdentifier, typename MType>
  struct sparse_matrix_detail_traits {
    typedef MIdentifier identifier_type;
    typedef MType       matrix_type; 
  };

  /// sparse_matrix_traits<> generic version: 
  template <typename M>
  struct sparse_matrix_traits 
    : sparse_matrix_detail_traits <typename boost::remove_const<M>::type, M>
  {
    // typedefs:
    //   matrix_structure 
    //   storage_format 
    //   ordering_type
    //   value_type
    //   value_pointer
    //   index_pointer 
    // enum (or static size_t const): 
    //   index_base
    // static functions:
    //   index_pointer index1_storage()
    //     - compressed column: array of column start locations 
    //     - compressed row: array of row start locations 
    //     - coordinate, column major: column indices of nonzeros
    //     - coordinate, row major: row indices of nonzeros
    //   index_pointer index2_storage()
    //     - compressed column: array of row indices of nonzeros
    //     - compressed row: array of column indices of nonzeros
    //     - coordinate, column major: row indices of nonzeros
    //     - coordinate, row major: column indices of nonzeros
    //   value_pointer value_storage()
    //     - array of nonzeros 
    //   int size1()
    //   int size2()
    //   int num_nonzeros() 
  }; 


  // storage format tags 
  struct compressed_t {};
  struct coordinate_t {}; 


  ///////////////////////////
  //
  // free accessor functions
  //
  ///////////////////////////

  template <typename M>
  inline
  typename sparse_matrix_traits<M>::index_pointer 
  spmatrix_index1_storage (M& m) { 
    return sparse_matrix_traits<M>::index1_storage (m); 
  }
  template <typename M>
  inline
  typename sparse_matrix_traits<M>::index_pointer 
  spmatrix_index2_storage (M& m) { 
    return sparse_matrix_traits<M>::index2_storage (m); 
  }
  
  template <typename M>
  inline
  typename sparse_matrix_traits<M>::value_pointer 
  spmatrix_value_storage (M& m) { 
    return sparse_matrix_traits<M>::value_storage (m); 
  }
  
  template <typename M>
  inline
  int spmatrix_size1 (M& m) { return sparse_matrix_traits<M>::size1 (m); }
  template <typename M>
  inline
  int spmatrix_size2 (M& m) { return sparse_matrix_traits<M>::size2 (m); }
  
  template <typename M>
  inline
  int spmatrix_num_nonzeros (M& m) { 
    return sparse_matrix_traits<M>::num_nonzeros (m); 
  }
  
  
}}}}  

#else // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#error with your compiler sparse matrices cannot be used in bindings

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_SPARSE_TRAITS_H
