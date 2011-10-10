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

#ifndef BOOST_NUMERIC_BINDINGS_CBLAS_LEVEL_1_HPP
#define BOOST_NUMERIC_BINDINGS_CBLAS_LEVEL_1_HPP

#include <cassert>

#include <boost/numeric/bindings/traits/type_traits.hpp>
#include <boost/numeric/bindings/traits/vector_traits.hpp>
#include <boost/numeric/bindings/atlas/cblas1_overloads.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_NO_TYPE_CHECK
#  include <boost/type_traits/same_traits.hpp>
#  include <boost/static_assert.hpp>
#endif 

namespace boost { namespace numeric { namespace bindings { 

  namespace atlas {

    // x_i <- alpha for all i
    template <typename T, typename Vct> 
    inline 
    void set (T const& alpha, Vct& x) {
      detail::set (traits::vector_size (x), alpha, 
                   traits::vector_storage (x), traits::vector_stride (x)); 
    }

    // y <- x
    template <typename VctX, typename VctY>
    inline 
    void copy (VctX const& x, VctY& y) {
      assert (traits::vector_size (y) >= traits::vector_size (x));
      detail::copy (traits::vector_size (x),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (x), 
#else
                    traits::vector_storage_const (x), 
#endif 
                    traits::vector_stride (x), 
                    traits::vector_storage (y), traits::vector_stride (y)); 
    }

    // x <-> y
    template <typename VctX, typename VctY>
    inline 
    void swap (VctX& x, VctY& y) {
      assert (traits::vector_size (y) >= traits::vector_size (x));
      detail::swap (traits::vector_size (x),
                    traits::vector_storage (x), traits::vector_stride (x), 
                    traits::vector_storage (y), traits::vector_stride (y)); 
    }

    // x <- alpha * x
    template <typename T, typename Vct> 
    inline 
    void scal (T const& alpha, Vct& x) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_TYPE_CHECK
      typedef traits::vector_traits<Vct> vtraits;
      BOOST_STATIC_ASSERT(
       (boost::is_same<T, typename vtraits::value_type>::value
        || 
        boost::is_same<T, 
          typename traits::type_traits<typename vtraits::value_type>::real_type
        >::value
        ));
#endif 
      detail::scal (traits::vector_size (x), alpha, 
                    traits::vector_storage (x), traits::vector_stride (x)); 
    }

    // y <- alpha * x + y
    template <typename T, typename VctX, typename VctY>
    inline 
    void axpy (T const& alpha, VctX const& x, VctY& y) {
      assert (traits::vector_size (y) >= traits::vector_size (x));
      detail::axpy (traits::vector_size (x), alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (x), 
#else
                    traits::vector_storage_const (x), 
#endif
                    traits::vector_stride (x), 
                    traits::vector_storage (y), traits::vector_stride (y)); 
    }

    // y <- x + y
    template <typename VctX, typename VctY>
    inline 
    void xpy (VctX const& x, VctY& y) {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::vector_traits<VctX>::value_type val_t; 
#else
      typedef typename VctX::value_type val_t; 
#endif
      axpy ((val_t) 1, x, y); 
    }

    // y <- alpha * x + beta * y
    template <typename T, typename VctX, typename VctY>
    inline 
    void axpby (T const& alpha, VctX const& x, 
                T const& beta, VctY& y) { 
      assert (traits::vector_size (y) >= traits::vector_size (x));
      detail::axpby (traits::vector_size (x), alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                     traits::vector_storage (x), 
#else
                     traits::vector_storage_const (x), 
#endif
                     traits::vector_stride (x), 
                     beta, 
                     traits::vector_storage (y), traits::vector_stride (y)); 
    }

    ///////////////////////////////////////////

    // dot <- x^T * y 
    // .. real & complex types
    template <typename VctX, typename VctY>
    inline 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
    typename traits::vector_traits<VctX>::value_type 
#else
    typename VctX::value_type 
#endif
    dot (VctX const& x, VctY const& y) {
      assert (traits::vector_size (y) >= traits::vector_size (x));
      return detail::dot (traits::vector_size (x),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                          traits::vector_storage (x), 
#else
                          traits::vector_storage_const (x), 
#endif
                          traits::vector_stride (x), 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                          traits::vector_storage (y), 
#else
                          traits::vector_storage_const (y), 
#endif
                          traits::vector_stride (y)); 
    }

    // dot <- x^T * y 
    // .. float only -- with double accumulation
    template <typename VctX, typename VctY>
    inline 
    double dsdot (VctX const& x, VctY const& y) {
      assert (traits::vector_size (y) >= traits::vector_size (x));
      return cblas_dsdot (traits::vector_size (x),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                          traits::vector_storage (x), 
#else
                          traits::vector_storage_const (x), 
#endif
                          traits::vector_stride (x), 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                          traits::vector_storage (y), 
#else
                          traits::vector_storage_const (y), 
#endif
                          traits::vector_stride (y)); 
    }

    // apdot <- alpha + x^T * y    
    // .. float only -- computation uses double precision 
    template <typename VctX, typename VctY>
    inline 
    float sdsdot (float const alpha, VctX const& x, VctY const& y) {
      assert (traits::vector_size (y) >= traits::vector_size (x));
      return cblas_sdsdot (traits::vector_size (x), alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                           traits::vector_storage (x), 
#else
                           traits::vector_storage_const (x), 
#endif
                           traits::vector_stride (x), 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                           traits::vector_storage (y), 
#else
                           traits::vector_storage_const (y), 
#endif
                           traits::vector_stride (y)); 
    }

    // dotu <- x^T * y 
    // .. complex types only
    // .. function
    template <typename VctX, typename VctY>
    inline 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
    typename traits::vector_traits<VctX>::value_type 
#else
    typename VctX::value_type 
#endif
    dotu (VctX const& x, VctY const& y) {
      assert (traits::vector_size (y) >= traits::vector_size (x));
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typename traits::vector_traits<VctX>::value_type val;
#else
      typename VctX::value_type val; 
#endif
      detail::dotu (traits::vector_size (x),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (x), 
#else
                    traits::vector_storage_const (x), 
#endif
                    traits::vector_stride (x), 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (y), 
#else
                    traits::vector_storage_const (y), 
#endif
                    traits::vector_stride (y), 
                    &val);
      return val; 
    }
    // .. procedure 
    template <typename VctX, typename VctY>
    inline 
    void dotu (VctX const& x, VctY const& y, 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
               typename traits::vector_traits<VctX>::value_type& val
#else
               typename VctX::value_type& val
#endif
    ) {
      assert (traits::vector_size (y) >= traits::vector_size (x));
      detail::dotu (traits::vector_size (x),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (x), 
#else
                    traits::vector_storage_const (x), 
#endif
                    traits::vector_stride (x), 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (y), 
#else
                    traits::vector_storage_const (y), 
#endif
                    traits::vector_stride (y), 
                    &val);
    }

    // dotc <- x^H * y 
    // .. complex types only
    // .. function
    template <typename VctX, typename VctY>
    inline 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
    typename traits::vector_traits<VctX>::value_type 
#else
    typename VctX::value_type 
#endif
    dotc (VctX const& x, VctY const& y) {
      assert (traits::vector_size (y) >= traits::vector_size (x));
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typename traits::vector_traits<VctX>::value_type val;
#else
      typename VctX::value_type val; 
#endif
      detail::dotc (traits::vector_size (x),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (x), 
#else
                    traits::vector_storage_const (x), 
#endif
                    traits::vector_stride (x), 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (y), 
#else
                    traits::vector_storage_const (y), 
#endif
                    traits::vector_stride (y),
                    &val);
      return val; 
    }
    // .. procedure 
    template <typename VctX, typename VctY>
    inline 
    void dotc (VctX const& x, VctY const& y, 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
               typename traits::vector_traits<VctX>::value_type& val
#else
               typename VctX::value_type& val
#endif
    ) {
      assert (traits::vector_size (y) >= traits::vector_size (x));
      detail::dotc (traits::vector_size (x),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (x), 
#else
                    traits::vector_storage_const (x), 
#endif
                    traits::vector_stride (x), 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (y), 
#else
                    traits::vector_storage_const (y), 
#endif
                    traits::vector_stride (y), 
                    &val);
    }

    // nrm2 <- ||x||_2
    template <typename Vct> 
    inline 
    typename traits::type_traits<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typename traits::vector_traits<Vct>::value_type 
#else 
      typename Vct::value_type 
#endif 
    >::real_type 
    nrm2 (Vct const& x) {
      return detail::nrm2 (traits::vector_size (x),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                           traits::vector_storage (x), 
#else
                           traits::vector_storage_const (x), 
#endif
                           traits::vector_stride (x)); 
    }

    // asum <- ||re (x)|| + ||im (x)||
    template <typename Vct> 
    inline 
    typename traits::type_traits<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typename traits::vector_traits<Vct>::value_type 
#else 
      typename Vct::value_type 
#endif 
    >::real_type  
    asum (Vct const& x) {
      return detail::asum (traits::vector_size (x),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                           traits::vector_storage (x), 
#else
                           traits::vector_storage_const (x), 
#endif
                           traits::vector_stride (x)); 
    }

    // iamax <- 1st i: max (|re (x_i)| + |im (x_i)|)
    template <typename Vct> 
    inline 
    CBLAS_INDEX iamax (Vct const& x) {
      return detail::iamax (traits::vector_size (x),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                            traits::vector_storage (x), 
#else
                            traits::vector_storage_const (x), 
#endif
                            traits::vector_stride (x)); 
    }

    // TO DO: plane rotations 

  } // namespace atlas

}}} 

#endif // BOOST_NUMERIC_BINDINGS_CBLAS_LEVEL_1_HPP
