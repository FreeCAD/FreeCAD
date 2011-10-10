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

#ifndef BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_MATRIX_H
#define BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_MATRIX_H

#include <boost/numeric/bindings/traits/traits.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#ifndef BOOST_UBLAS_HAVE_BINDINGS
#  include <boost/numeric/ublas/matrix.hpp> 
#endif 
#include <boost/numeric/bindings/traits/detail/ublas_ordering.hpp>

#if defined (BOOST_NUMERIC_BINDINGS_FORTRAN) || !defined (BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK)
#  include <boost/static_assert.hpp>
#  include <boost/type_traits/same_traits.hpp>
#endif


namespace boost { namespace numeric { namespace bindings { namespace traits {

  // ublas::matrix<>
  // Specialization using matrix_detail_traits so that we can specialize for
  // matrix_detail_traits< matrix<T, F, ArrT>, matrix<T, F, ArrT> >
  // matrix_detail_traits< matrix<T, F, ArrT>, matrix<T, F, ArrT> const >
  // at once.
  template <typename T, typename F, typename ArrT, typename M>
  struct matrix_detail_traits< boost::numeric::ublas::matrix<T, F, ArrT>, M > 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same<boost::numeric::ublas::matrix<T, F, ArrT>, typename boost::remove_const<M>::type>::value) );
#endif
#ifdef BOOST_NUMERIC_BINDINGS_FORTRAN
    BOOST_STATIC_ASSERT((boost::is_same<
      typename F::orientation_category, 
      boost::numeric::ublas::column_major_tag
    >::value)); 
#endif 

    typedef boost::numeric::ublas::matrix<T, F, ArrT>   identifier_type ;
    typedef M                                           matrix_type;
    typedef general_t                                   matrix_structure; 
    typedef typename detail::ublas_ordering<
      typename F::orientation_category
    >::type                                             ordering_type; 

    typedef T                                           value_type; 
    typedef typename detail::generate_const<M,T>::type* pointer; 

      typedef typename identifier_type::orientation_category                      orientation_category; 
      typedef typename detail::ublas_ordering<orientation_category>::functor_type functor_t ;

    static pointer storage (matrix_type& m) {
      typedef typename detail::generate_const<M,ArrT>::type array_type ;
      return vector_traits<array_type>::storage (m.data()); 
    }
    static int size1 (matrix_type& m) { return m.size1(); } 
    static int size2 (matrix_type& m) { return m.size2(); }
    static int storage_size (matrix_type& m) { return size1 (m) * size2 (m); }
    static int leading_dimension (matrix_type& m) {
      // g++ 2.95.4 and 3.0.4 (with -pedantic) dislike 
      //   identifier_type::functor_type::size2()
      //return functor_t::size_m (m.size1(), m.size2());
      return detail::ublas_ordering<orientation_category>::leading_dimension( m ) ;
    }

    // stride1 == distance (m (i, j), m (i+1, j)) 
    static int stride1 (matrix_type& m) { 
      //return functor_t::one1 (m.size1(), m.size2());
      return detail::ublas_ordering<orientation_category>::stride1( m ) ;
    } 
    // stride2 == distance (m (i, j), m (i, j+1)) 
    static int stride2 (matrix_type& m) { 
      //return functor_t::one2 (m.size1(), m.size2());
      return detail::ublas_ordering<orientation_category>::stride2( m ) ;
    }
  }; 


  // ublas::matrix_reference<> 
  template <typename M, typename MR>
  struct matrix_detail_traits<boost::numeric::ublas::matrix_reference<M>, MR >
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same< boost::numeric::ublas::matrix_reference<M>, typename boost::remove_const<MR>::type>::value) ) ;
#endif 

    typedef boost::numeric::ublas::matrix_reference<M>  identifier_type;
    typedef MR                                          matrix_type;
    typedef typename matrix_traits<M>::matrix_structure matrix_structure; 
    typedef typename matrix_traits<M>::ordering_type    ordering_type; 

    typedef typename M::value_type                                value_type;
    typedef typename detail::generate_const<MR,value_type>::type* pointer; 

  private:
    typedef typename detail::generate_const<MR, M>::type m_type; 

  public:
    static pointer storage (matrix_type& mr) {
      return matrix_traits<m_type>::storage (mr.expression());
    }

    static int size1 (matrix_type& mr) { return mr.size1(); } 
    static int size2 (matrix_type& mr) { return mr.size2(); }
    static int leading_dimension (matrix_type& mr) {
      return matrix_traits<m_type>::leading_dimension (mr.expression()); 
    }

    static int stride1 (matrix_type& mr) { 
      return matrix_traits<m_type>::stride1 (mr.expression()); 
    } 
    static int stride2 (matrix_type& mr) { 
      return matrix_traits<m_type>::stride2 (mr.expression()); 
    }
    // Only for banded matrices
    static int upper_bandwidth(matrix_type& mr) {
      return matrix_traits<m_type>::upper_bandwidth(mr.expression());
    }
    static int lower_bandwidth(matrix_type& mr) {
      return matrix_traits<m_type>::lower_bandwidth(mr.expression());
    }
  }; 


  // ublas::matrix_range<> 
  template <typename M, typename MR>
  struct matrix_detail_traits<boost::numeric::ublas::matrix_range<M>, MR >
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same< boost::numeric::ublas::matrix_range<M>, typename boost::remove_const<MR>::type>::value) ) ;
#endif 

    typedef boost::numeric::ublas::matrix_range<M>      identifier_type;
    typedef MR                                          matrix_type;
    typedef typename matrix_traits<M>::matrix_structure matrix_structure;
    typedef typename matrix_traits<M>::ordering_type    ordering_type; 

  private:
    typedef typename detail::generate_const<MR, typename MR::matrix_closure_type>::type m_type; 

  public:
    typedef typename matrix_traits<m_type>::value_type            value_type;
    typedef typename matrix_traits<m_type>::pointer               pointer ;

  public:
    static pointer storage (matrix_type& mr) {
      m_type& mt = mr.data(); 
      pointer ptr = matrix_traits<m_type>::storage (mt);
      ptr += mr.start1() * matrix_traits<m_type>::stride1 (mt); 
      ptr += mr.start2() * matrix_traits<m_type>::stride2 (mt); 
      return ptr; 
    }

    static int size1 (matrix_type& mr) { return mr.size1(); } 
    static int size2 (matrix_type& mr) { return mr.size2(); }
    static int leading_dimension (matrix_type& mr) {
      return matrix_traits<m_type>::leading_dimension (mr.data()); 
    }

    static int stride1 (matrix_type& mr) { 
      return matrix_traits<m_type>::stride1 (mr.data()); 
    } 
    static int stride2 (matrix_type& mr) { 
      return matrix_traits<m_type>::stride2 (mr.data()); 
    }
    // For band matrices only
    static int upper_bandwidth (matrix_type& mr) {
       return matrix_traits<m_type>::upper_bandwidth(mr.data());
    }
    static int lower_bandwidth (matrix_type& mr) {
       return matrix_traits<m_type>::lower_bandwidth(mr.data());
    }
  }; 


  // ublas::matrix_slice<> 
  template <typename M, typename MS>
  struct matrix_detail_traits<boost::numeric::ublas::matrix_slice<M>, MS > 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same< boost::numeric::ublas::matrix_slice<M>, typename boost::remove_const<MS>::type>::value) ) ;
#endif 

    typedef boost::numeric::ublas::matrix_slice<M>   identifier_type;
    typedef MS                                       matrix_type;
    typedef unknown_structure_t                      matrix_structure; 
    typedef typename matrix_traits<M>::ordering_type ordering_type; 

    typedef typename M::value_type                                value_type;
    typedef typename detail::generate_const<MS,value_type>::type* pointer; 

  private:
    typedef typename detail::generate_const<MS, typename MS::matrix_closure_type>::type m_type; 

  public:
    static pointer storage (matrix_type& ms) {
      m_type& mt = ms.data(); 
      pointer ptr = matrix_traits<M>::storage (mt);
      ptr += ms.start1() * matrix_traits<M>::stride1 (mt); 
      ptr += ms.start2() * matrix_traits<M>::stride2 (mt); 
      return ptr; 
    }

    static int size1 (matrix_type& ms) { return ms.size1(); } 
    static int size2 (matrix_type& ms) { return ms.size2(); }

  private:
    static int ld (int s1, int s2, boost::numeric::ublas::row_major_tag) {
      return s1; 
    }
    static int ld (int s1, int s2, boost::numeric::ublas::column_major_tag) {
      return s2; 
    }
  public:
    static int leading_dimension (matrix_type& ms) {
      typedef typename identifier_type::orientation_category oc_t; 
      return ld (ms.stride1(), ms.stride2(), oc_t())
	* matrix_traits<m_type>::leading_dimension (ms.data()); 
    }

    static int stride1 (matrix_type& ms) { 
      return ms.stride1() * matrix_traits<m_type>::stride1 (ms.data()); 
    } 
    static int stride2 (matrix_type& ms) { 
      return ms.stride2() * matrix_traits<m_type>::stride2 (ms.data()); 
    }

  }; 


  // matrix_row<> and matrix_column<> are vectors:

  // ublas::matrix_row<>
  template <typename M, typename MR>
  struct vector_detail_traits< boost::numeric::ublas::matrix_row<M>, MR > 
  : default_vector_traits< MR, typename M::value_type > 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same< boost::numeric::ublas::matrix_row<M>, typename boost::remove_const<MR>::type>::value) ) ;
#endif 

    typedef boost::numeric::ublas::matrix_row<M>                   identifier_type;
    typedef MR                                                     vector_type;
    typedef typename M::value_type                                 value_type;
    typedef typename default_vector_traits<MR,value_type>::pointer pointer; 

  private:
    typedef typename detail::generate_const<MR, typename MR::matrix_closure_type>::type m_type; 

  public:
    static pointer storage (vector_type& mr) {
      m_type& mt = mr.data(); 
      pointer ptr = matrix_traits<m_type>::storage (mt); 
      ptr += mr.index() * matrix_traits<m_type>::stride1 (mt);
      return ptr; 
    }
    static int stride (vector_type& mr) { 
      return matrix_traits<m_type>::stride2 (mr.data());
    } 
  }; 


  // ublas::matrix_column<>
  template <typename M, typename MC>
  struct vector_detail_traits< boost::numeric::ublas::matrix_column<M>, MC > 
  : default_vector_traits< MC, typename M::value_type > 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same< boost::numeric::ublas::matrix_column<M>, typename boost::remove_const<MC>::type>::value) ) ;
#endif 

    typedef boost::numeric::ublas::matrix_column<M>                identifier_type; 
    typedef MC                                                     vector_type;
    typedef typename M::value_type                                 value_type ;
    typedef typename default_vector_traits<MC,value_type>::pointer pointer; 

  private:
    typedef typename detail::generate_const<MC, typename MC::matrix_closure_type>::type m_type; 

  public:
    static pointer storage (vector_type& mc) {
      m_type& mt = mc.data(); 
      pointer ptr = matrix_traits<m_type>::storage (mt); 
      ptr += mc.index() * matrix_traits<m_type>::stride2 (mt);
      return ptr; 
    }
    static int stride (vector_type& mc) { 
      return matrix_traits<m_type>::stride1 (mc.data());
    } 
  }; 


#ifndef BOOST_NUMERIC_BINDINGS_FORTRAN 
  
  // (undocumented) ublas::c_matrix<> 
  template <typename T, std::size_t M, std::size_t N, typename Matr>
  struct matrix_detail_traits< boost::numeric::ublas::c_matrix<T,M,N>, Matr > 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_SANITY_CHECK
    BOOST_STATIC_ASSERT( (boost::is_same<boost::numeric::ublas::c_matrix<T,M,N>, typename boost::remove_const<Matr>::type>::value) );
#endif

    typedef boost::numeric::ublas::c_matrix<T,M,N>   identifier_type ;
    typedef Matr                                     matrix_type;
    typedef general_t                                matrix_structure; 
    typedef row_major_t                              ordering_type; 

    typedef T                                              value_type; 
    typedef typename detail::generate_const<Matr,T>::type* pointer; 

    static pointer storage (matrix_type& m) { return m.data(); }
    static int size1 (matrix_type& m) { return m.size1(); } 
    static int size2 (matrix_type& m) { return m.size2(); }
    static int storage_size (matrix_type& m) { return M * N; }
    static int leading_dimension (matrix_type& m) { return N; }

    // stride1 == distance (m (i, j), m (i+1, j)) 
    static int stride1 (matrix_type& m) { return N; }
    // stride2 == distance (m (i, j), m (i, j+1)) 
    static int stride2 (matrix_type& m) { return 1; }
  }; 

#endif // BOOST_NUMERIC_BINDINGS_FORTRAN 


  // TO DO: matrix_vector_range<>, matrix_vector_slice<> 

}}}}

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS 

#endif // BOOST_NUMERIC_BINDINGS_TRAITS_UBLAS_MATRIX_H
