/*
 * 
 * Copyright (c) 2002, 2003 Kresimir Fresl, Toon Knapen and Karl Meerbergen
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * KF acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_VECTOR_H
#define BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_VECTOR_H

#include <boost/numeric/bindings/traits/vector_traits.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS

#ifndef BOOST_UBLAS_HAVE_BINDINGS
#  include <boost/numeric/ublas/vector.hpp>
#endif 


namespace boost { namespace numeric { namespace bindings { namespace traits {

  // ublas::vector<>
  template <typename T, typename ArrT, typename V>
  struct vector_detail_traits< boost::numeric::ublas::vector<T, ArrT>, V > 
  : default_vector_traits< V, T > 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same< boost::numeric::ublas::vector<T, ArrT>, typename boost::remove_const<V>::type >::value) );
#endif

    typedef boost::numeric::ublas::vector<T, ArrT>          identifier_type; 
    typedef V                                               vector_type;
    typedef typename default_vector_traits< V, T >::pointer pointer;

    static pointer storage (vector_type& v) {
      typedef typename detail::generate_const<V,ArrT>::type array_type ;
      return vector_traits<array_type>::storage (v.data()); 
    }
  }; 

  // ublas::vector_reference<>
  template <typename V, typename VR>
  struct vector_detail_traits< boost::numeric::ublas::vector_reference<V>, VR > 
  : default_vector_traits< VR, typename V::value_type > 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same< boost::numeric::ublas::vector_reference<V>, typename boost::remove_const<VR>::type >::value) );
#endif

    typedef boost::numeric::ublas::vector_reference<V>             identifier_type; 
    typedef VR                                                     vector_type;
    typedef typename V::value_type                                 value_type ;
    typedef typename default_vector_traits<VR,value_type>::pointer pointer; 

  private:
    typedef typename detail::generate_const<VR,V>::type vct_t;

  public:
    static pointer storage (vector_type& v) {
      return vector_traits<vct_t>::storage (v.expression()); 
    }
    static int stride (vector_type& v) {
      return vector_traits<vct_t>::stride (v.expression()); 
    }
  }; 

  // ublas::vector_range<>
  template <typename V, typename VR>
  struct vector_detail_traits< boost::numeric::ublas::vector_range<V>, VR > 
  : default_vector_traits< VR, typename V::value_type > 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same< boost::numeric::ublas::vector_range<V>, typename boost::remove_const<VR>::type >::value) );
#endif

    typedef boost::numeric::ublas::vector_range<V>                 identifier_type; 
    typedef VR                                                     vector_type;
    typedef typename V::value_type                                 value_type ;
    typedef typename default_vector_traits<VR,value_type>::pointer pointer; 

  private:
    typedef typename detail::generate_const<VR, typename VR::vector_closure_type>::type v_type; 

  public:
    static pointer storage (vector_type& vr) {
      pointer ptr = vector_traits<v_type>::storage (vr.data()); 
      ptr += vr.start() * vector_traits<v_type>::stride (vr.data());
      return ptr; 
    }
    static int stride (vector_type& vr) {
      return vector_traits<v_type>::stride (vr.data()); 
    }
  }; 


  // ublas::vector_slice<>
  template <typename V, typename VS>
  struct vector_detail_traits<boost::numeric::ublas::vector_slice<V>, VS > 
  : default_vector_traits< VS, typename V::value_type > 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same< boost::numeric::ublas::vector_slice<V>, typename boost::remove_const<VS>::type >::value) );
#endif

    typedef boost::numeric::ublas::vector_slice<V>                 identifier_type; 
    typedef VS                                                     vector_type;
    typedef typename V::value_type                                 value_type ;
    typedef typename default_vector_traits<VS,value_type>::pointer pointer; 

  private:
    typedef typename detail::generate_const<VS, typename VS::vector_closure_type>::type v_type; 

  public:
    static pointer storage (vector_type& vs) {
      pointer ptr = vector_traits<v_type>::storage (vs.data()); 
      ptr += vs.start() * vector_traits<v_type>::stride (vs.data());
      return ptr; 
    }
    static int stride (vector_type& vs) {
      return vs.stride() * vector_traits<v_type>::stride (vs.data()); 
    }
  }; 


#ifndef BOOST_NUMERIC_BINDINGS_FORTRAN 
  
  // (undocumented) ublas::c_vector<>
  template <typename T, std::size_t N, typename V>
  struct vector_detail_traits< boost::numeric::ublas::c_vector<T, N>, V > 
  : default_vector_traits< V, T > 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same< boost::numeric::ublas::c_vector<T,N>, typename boost::remove_const<V>::type >::value) );
#endif

    typedef boost::numeric::ublas::c_vector<T,N>         identifier_type; 
    typedef V                                            vector_type;
    typedef typename default_vector_traits<V,T>::pointer pointer;

    static pointer storage (vector_type& v) { return v.data(); }
  }; 

#endif // BOOST_NUMERIC_BINDINGS_FORTRAN 


}}}}  

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_VECTOR_H
