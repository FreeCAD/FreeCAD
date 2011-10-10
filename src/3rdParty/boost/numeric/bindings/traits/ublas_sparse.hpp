/*
 * 
 * Copyright (c) 2003 Kresimir Fresl, Toon Knapen and Karl Meerbergen
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * KF acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_SPARSE_MATRIX_H
#define BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_SPARSE_MATRIX_H

#include <cstddef> 
#ifndef BOOST_UBLAS_HAVE_BINDINGS
#  include <boost/numeric/ublas/matrix_sparse.hpp> 
#endif 
#include <boost/numeric/bindings/traits/sparse_traits.hpp>
#include <boost/numeric/bindings/traits/detail/ublas_ordering.hpp>
#include <algorithm>


namespace boost { namespace numeric { namespace bindings { namespace traits {

  // ublas::compressed_matrix<>
  template <typename T, typename F, std::size_t IB, typename IA, typename TA,
            typename MType
            >
  struct sparse_matrix_detail_traits<
    boost::numeric::ublas::compressed_matrix<T,F,IB,IA,TA>,
    MType 
  >
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( 
      (boost::is_same<
         boost::numeric::ublas::compressed_matrix<T,F,IB,IA,TA>,
         typename boost::remove_const<MType>::type
       >::value) 
    );
#endif

    typedef 
      boost::numeric::ublas::compressed_matrix<T,F,IB,IA,TA> identifier_type;
    typedef MType matrix_type;

    typedef general_t matrix_structure; 
    typedef compressed_t storage_format; 
    typedef typename detail::ublas_ordering<
      typename F::orientation_category
    >::type ordering_type; 
    typedef F layout_type;

    typedef T value_type; 

  private: 
    typedef typename detail::generate_const<MType,TA>::type val_array_t; 
    typedef typename detail::generate_const<MType,IA>::type idx_array_t; 

  public: 
    typedef typename vector_traits<val_array_t>::pointer value_pointer; 
    typedef typename vector_traits<idx_array_t>::pointer index_pointer; 

    BOOST_STATIC_CONSTANT (std::size_t, index_base = IB);

    static index_pointer index1_storage (matrix_type& cm) {
      //assert (cm.filled1() == layout_type::size1 (cm.size1(), cm.size2()) + 1);
      return vector_traits<idx_array_t>::storage (cm.index1_data()); 
    }
    static index_pointer index2_storage (matrix_type& cm) {
      return vector_traits<idx_array_t>::storage (cm.index2_data()); 
    }
    static value_pointer value_storage (matrix_type& cm) {
      return vector_traits<val_array_t>::storage (cm.value_data()); 
    }

    static int size1 (matrix_type& cm) { return cm.size1(); } 
    static int size2 (matrix_type& cm) { return cm.size2(); }
    static int num_nonzeros (matrix_type& cm) { 
      return cm.nnz(); 
      // Joerg, this isn't very intuitive :o(
      // return cm.non_zeros(); 
    } 
  }; 


  // ublas::coordinate_matrix<>
  template <typename T, typename F, std::size_t IB, typename IA, typename TA,
            typename MType
            >
  struct sparse_matrix_detail_traits<
    boost::numeric::ublas::coordinate_matrix<T,F,IB,IA,TA>,
    MType 
  >
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( 
      (boost::is_same<
         boost::numeric::ublas::coordinate_matrix<T,F,IB,IA,TA>,
         typename boost::remove_const<MType>::type
       >::value) 
    );
#endif

    typedef 
      boost::numeric::ublas::coordinate_matrix<T,F,IB,IA,TA> identifier_type;
    typedef MType matrix_type;

    typedef general_t matrix_structure; 
    typedef coordinate_t storage_format; 
    typedef typename detail::ublas_ordering<
      typename F::orientation_category
    >::type ordering_type; 

    typedef T value_type; 

  private: 
    typedef typename detail::generate_const<MType,TA>::type val_array_t; 
    typedef typename detail::generate_const<MType,IA>::type idx_array_t; 

  public: 
    typedef typename vector_traits<val_array_t>::pointer value_pointer; 
    typedef typename vector_traits<idx_array_t>::pointer index_pointer; 

    BOOST_STATIC_CONSTANT (std::size_t, index_base = IB);

    static index_pointer index1_storage (matrix_type& cm) {
      return vector_traits<idx_array_t>::storage (cm.index1_data()); 
    }
    static index_pointer index2_storage (matrix_type& cm) {
      return vector_traits<idx_array_t>::storage (cm.index2_data()); 
    }
    static value_pointer value_storage (matrix_type& cm) {
      return vector_traits<val_array_t>::storage (cm.value_data()); 
    }

    static int size1 (matrix_type& cm) { return cm.size1(); } 
    static int size2 (matrix_type& cm) { return cm.size2(); }
    static int num_nonzeros (matrix_type& cm) { 
      return cm.nnz(); 
      // Joerg, this isn't very intuitive :o(
      // return cm.non_zeros(); 
    } 
  }; 

}}}}

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_SPARSE_MATRIX_H
