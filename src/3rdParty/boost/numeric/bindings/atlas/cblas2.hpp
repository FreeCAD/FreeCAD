/*
 * 
 * Copyright (c) Kresimir Fresl 2002, 2003
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Author acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_CBLAS_LEVEL_2_HPP
#define BOOST_NUMERIC_BINDINGS_CBLAS_LEVEL_2_HPP

#include <cassert>

#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/traits/type_traits.hpp>
#include <boost/numeric/bindings/atlas/cblas2_overloads.hpp>
#include <boost/numeric/bindings/atlas/cblas_enum.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
#  include <boost/static_assert.hpp>
#  include <boost/type_traits/same_traits.hpp>
#endif


namespace boost { namespace numeric { namespace bindings { 

  namespace atlas {

    // y <- alpha * op (A) * x + beta * y
    // op (A) == A || A^T || A^H
    template <typename T, typename Matr, typename VctX, typename VctY>
    inline 
    void gemv (CBLAS_TRANSPOSE const TransA, 
               T const& alpha, Matr const& a, VctX const& x, 
               T const& beta, VctY& y
               )
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<Matr>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      int const m = traits::matrix_size1 (a);
      int const n = traits::matrix_size2 (a);
      assert (traits::vector_size (x) >= (TransA == CblasNoTrans ? n : m)); 
      assert (traits::vector_size (y) >= (TransA == CblasNoTrans ? m : n)); 
      // .. what about AtlasConj? 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<Matr>::ordering_type
#else
           typename Matr::orientation_category 
#endif 
         >::value); 

      detail::gemv (stor_ord, TransA, m, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::matrix_storage (a), 
#else
                    traits::matrix_storage_const (a), 
#endif
                    traits::leading_dimension (a),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (x), 
#else
                    traits::vector_storage_const (x), 
#endif
                    traits::vector_stride (x),
                    beta, 
                    traits::vector_storage (y), 
                    traits::vector_stride (y));
    }

    // y <- alpha * A * x + beta * y 
    template <typename T, typename Matr, typename VctX, typename VctY>
    inline 
    void gemv (T const& alpha, Matr const& a, VctX const& x, 
               T const& beta, VctY& y) {
      gemv (CblasNoTrans, alpha, a, x, beta, y); 
    }

    // y <- A * x
    template <typename Matr, typename VctX, typename VctY>
    inline 
    void gemv (Matr const& a, VctX const& x, VctY& y) {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<Matr>::value_type val_t; 
#else
      typedef typename Matr::value_type val_t; 
#endif 
      gemv (CblasNoTrans, (val_t) 1, a, x, (val_t) 0, y);
    }


    // y <- alpha * A * x + beta * y
    // A real symmetric matrix (T == float | double)
    // [from 'dsymv.f':]
    /* [...] with UPLO = 'U' or 'u', the  leading n by n upper 
     * triangular part of the array A must contain the upper triangular
     * part of the symmetric matrix and the strictly lower triangular 
     * part of A is not referenced.
     * [...] with UPLO = 'L' or 'l', the leading n by n lower
     * triangular part of the array A must contain the lower triangular  
     * part of the symmetric matrix and the strictly  upper
     * triangular part of A is not referenced.
     */ 
    template <typename T, typename SymmMatr, typename VctX, typename VctY>
    inline 
    void symv (CBLAS_UPLO const uplo, T const& alpha, SymmMatr const& a, 
               VctX const& x, T const& beta, VctY& y)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmMatr>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmMatr>::ordering_type
#else
           typename SymmMatr::orientation_category 
#endif
         >::value); 

      detail::symv (stor_ord, uplo, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::matrix_storage (a), 
#else
                    traits::matrix_storage_const (a), 
#endif
                    traits::leading_dimension (a),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (x), 
#else
                    traits::vector_storage_const (x), 
#endif
                    traits::vector_stride (x),
                    beta, 
                    traits::vector_storage (y), 
                    traits::vector_stride (y));
    }

    template <typename T, typename SymmMatr, typename VctX, typename VctY>
    inline 
    void symv (T const& alpha, SymmMatr const& a, VctX const& x, 
               T const& beta, VctY& y)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmMatr>::matrix_structure, 
        traits::symmetric_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmMatr>::ordering_type
#else
           typename SymmMatr::orientation_category 
#endif
         >::value); 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmMatr>::uplo_type
#else
           typename SymmMatr::packed_category 
#endif 
         >::value); 

      detail::symv (stor_ord, uplo, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::matrix_storage (a), 
#else
                    traits::matrix_storage_const (a), 
#endif
                    traits::leading_dimension (a),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (x), 
#else
                    traits::vector_storage_const (x), 
#endif
                    traits::vector_stride (x),
                    beta, 
                    traits::vector_storage (y), 
                    traits::vector_stride (y));
    }

    // y <- A * x
    template <typename SymmMatr, typename VctX, typename VctY>
    inline 
    void symv (SymmMatr const& a, VctX const& x, VctY& y) {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<SymmMatr>::value_type val_t; 
#else
      typedef typename SymmMatr::value_type val_t; 
#endif
      symv ((val_t) 1, a, x, (val_t) 0, y);
    }


    // y <- alpha * A * x + beta * y
    // A real symmetric matrix (T == float | double) in packed form
    // [from 'dspmv.f' (description assumes column major order):]
    /* A is an array of DIMENSION ( ( n*( n + 1 ) )/2 ).
     * Before entry with UPLO = 'U' or 'u', the array A must contain
     * the upper triangular  part of the symmetric matrix packed 
     * sequentially, column by column, so that A( 1 ) contains a(1,1),
     * A( 2 ) and A( 3 ) contain a(1,2) and a(2,2) respectively, and
     * so on.
     * Before entry with UPLO = 'L' or 'l', the array A must contain
     * the lower triangular  part of the symmetric matrix packed 
     * sequentially, column by column, so that A( 1 ) contains a(1,1),
     * A( 2 ) and A( 3 ) contain a(2,1) and a(3,1) respectively, and
     * so on. 
     */ 
    template <typename T, typename SymmMatr, typename VctX, typename VctY>
    inline 
    void spmv (T const& alpha, SymmMatr const& a, VctX const& x, 
               T const& beta, VctY& y)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmMatr>::matrix_structure, 
        traits::symmetric_packed_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmMatr>::ordering_type
#else
           typename SymmMatr::orientation_category 
#endif
         >::value); 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmMatr>::uplo_type
#else
           typename SymmMatr::packed_category 
#endif 
         >::value); 

      detail::spmv (stor_ord, uplo, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::matrix_storage (a), 
                    traits::vector_storage (x), 
#else
                    traits::matrix_storage_const (a), 
                    traits::vector_storage_const (x), 
#endif
                    traits::vector_stride (x),
                    beta, 
                    traits::vector_storage (y), 
                    traits::vector_stride (y));
    }

    // y <- A * x
    template <typename SymmMatr, typename VctX, typename VctY>
    inline 
    void spmv (SymmMatr const& a, VctX const& x, VctY& y) {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<SymmMatr>::value_type val_t; 
#else
      typedef typename SymmMatr::value_type val_t; 
#endif
      spmv ((val_t) 1, a, x, (val_t) 0, y);
    }


    // y <- alpha * A * x + beta * y
    // A complex hermitian matrix 
    // (T == std::complex<float> | std::complex<double>)
    template <typename T, typename HermMatr, typename VctX, typename VctY>
    inline 
    void hemv (CBLAS_UPLO const uplo, T const& alpha, HermMatr const& a, 
               VctX const& x, T const& beta, VctY& y)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermMatr>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermMatr>::ordering_type
#else
           typename HermMatr::orientation_category 
#endif
         >::value); 

      detail::hemv (stor_ord, uplo, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::matrix_storage (a), 
#else
                    traits::matrix_storage_const (a), 
#endif
                    traits::leading_dimension (a),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (x), 
#else
                    traits::vector_storage_const (x), 
#endif
                    traits::vector_stride (x),
                    beta, 
                    traits::vector_storage (y), 
                    traits::vector_stride (y));
    }

    template <typename T, typename HermMatr, typename VctX, typename VctY>
    inline 
    void hemv (T const& alpha, HermMatr const& a, VctX const& x, 
               T const& beta, VctY& y)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermMatr>::matrix_structure, 
        traits::hermitian_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermMatr>::ordering_type
#else
           typename HermMatr::orientation_category 
#endif
         >::value); 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermMatr>::uplo_type
#else
           typename HermMatr::packed_category 
#endif 
         >::value); 

      detail::hemv (stor_ord, uplo, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::matrix_storage (a), 
#else
                    traits::matrix_storage_const (a), 
#endif
                    traits::leading_dimension (a),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::vector_storage (x), 
#else
                    traits::vector_storage_const (x), 
#endif
                    traits::vector_stride (x),
                    beta, 
                    traits::vector_storage (y), 
                    traits::vector_stride (y));
    }

    // y <- A * x
    template <typename HermMatr, typename VctX, typename VctY>
    inline 
    void hemv (HermMatr const& a, VctX const& x, VctY& y) {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<HermMatr>::value_type val_t; 
#else
      typedef typename HermMatr::value_type val_t; 
#endif
      hemv ((val_t) 1, a, x, (val_t) 0, y);
    }


    // y <- alpha * A * x + beta * y
    // A complex hermitian matrix in packed form 
    // (T == std::complex<float> | std::complex<double>)
    template <typename T, typename HermMatr, typename VctX, typename VctY>
    inline 
    void hpmv (T const& alpha, HermMatr const& a, VctX const& x, 
               T const& beta, VctY& y)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermMatr>::matrix_structure, 
        traits::hermitian_packed_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermMatr>::ordering_type
#else
           typename HermMatr::orientation_category 
#endif
         >::value); 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermMatr>::uplo_type
#else
           typename HermMatr::packed_category 
#endif 
         >::value); 

      detail::hpmv (stor_ord, uplo, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::matrix_storage (a), 
                    traits::vector_storage (x), 
#else
                    traits::matrix_storage_const (a), 
                    traits::vector_storage_const (x), 
#endif
                    traits::vector_stride (x),
                    beta, 
                    traits::vector_storage (y), 
                    traits::vector_stride (y));
    }

    // y <- A * x
    template <typename HermMatr, typename VctX, typename VctY>
    inline 
    void hpmv (HermMatr const& a, VctX const& x, VctY& y) {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<HermMatr>::value_type val_t; 
#else
      typedef typename HermMatr::value_type val_t; 
#endif
      hpmv ((val_t) 1, a, x, (val_t) 0, y);
    }


    // A <- alpha * x * y^T + A 
    // .. real & complex types
    template <typename T, typename Matr, typename VctX, typename VctY>
    inline 
    void ger (T const& alpha, VctX const& x, VctY const& y, Matr& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<Matr>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      int const m = traits::matrix_size1 (a);
      int const n = traits::matrix_size2 (a);
      assert (traits::vector_size (x) >= m); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<Matr>::ordering_type
#else
           typename Matr::orientation_category 
#endif 
         >::value); 

      detail::ger (stor_ord, m, n, alpha, 
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
                   traits::matrix_storage (a), 
                   traits::leading_dimension (a)); 
    }

    // A <- x * y^T + A 
    template <typename Matr, typename VctX, typename VctY>
    inline 
    void ger (VctX const& x, VctY const& y, Matr& a) {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<Matr>::value_type val_t; 
#else
      typedef typename Matr::value_type val_t; 
#endif 
      ger ((val_t) 1, x, y, a); 
    }

    // A <- alpha * x * y^T + A 
    // .. complex types only
    template <typename T, typename Matr, typename VctX, typename VctY>
    inline 
    void geru (T const& alpha, VctX const& x, VctY const& y, Matr& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<Matr>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      int const m = traits::matrix_size1 (a);
      int const n = traits::matrix_size2 (a);
      assert (traits::vector_size (x) >= m); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<Matr>::ordering_type
#else
           typename Matr::orientation_category 
#endif 
         >::value); 

      detail::geru (stor_ord, m, n, alpha, 
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
                    traits::matrix_storage (a), 
                    traits::leading_dimension (a)); 
    }

    // A <- x * y^T + A 
    template <typename Matr, typename VctX, typename VctY>
    inline 
    void geru (VctX const& x, VctY const& y, Matr& a) {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<Matr>::value_type val_t; 
#else
      typedef typename Matr::value_type val_t; 
#endif 
      geru ((val_t) 1, x, y, a); 
    }

    // A <- alpha * x * y^H + A 
    // .. complex types only
    template <typename T, typename Matr, typename VctX, typename VctY>
    inline 
    void gerc (T const& alpha, VctX const& x, VctY const& y, Matr& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<Matr>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      int const m = traits::matrix_size1 (a);
      int const n = traits::matrix_size2 (a);
      assert (traits::vector_size (x) >= m); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<Matr>::ordering_type
#else
           typename Matr::orientation_category 
#endif 
         >::value); 

      detail::gerc (stor_ord, m, n, alpha, 
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
                    traits::matrix_storage (a), 
                    traits::leading_dimension (a)); 
    }

    // A <- x * y^H + A 
    template <typename Matr, typename VctX, typename VctY>
    inline 
    void gerc (VctX const& x, VctY const& y, Matr& a) {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<Matr>::value_type val_t; 
#else
      typedef typename Matr::value_type val_t; 
#endif 
      gerc ((val_t) 1, x, y, a); 
    }


    // A <- alpha * x * x^T + A 
    // A real symmetric (see leading comments for 'symv()') 
    template <typename T, typename SymmM, typename VctX>
    inline 
    void syr (CBLAS_UPLO const uplo, T const& alpha, VctX const& x, SymmM& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmM>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmM>::ordering_type
#else
           typename SymmM::orientation_category 
#endif
         >::value); 

      detail::syr (stor_ord, uplo, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                   traits::vector_storage (x), 
#else
                   traits::vector_storage_const (x), 
#endif
                   traits::vector_stride (x),
                   traits::matrix_storage (a), 
                   traits::leading_dimension (a)); 
    }

    template <typename T, typename SymmM, typename VctX>
    inline 
    void syr (T const& alpha, VctX const& x, SymmM& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmM>::matrix_structure, 
        traits::symmetric_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmM>::ordering_type
#else
           typename SymmM::orientation_category 
#endif
         >::value); 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmM>::uplo_type
#else
           typename SymmM::packed_category 
#endif 
         >::value); 

      detail::syr (stor_ord, uplo, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                   traits::vector_storage (x), 
#else
                   traits::vector_storage_const (x), 
#endif
                   traits::vector_stride (x),
                   traits::matrix_storage (a), 
                   traits::leading_dimension (a)); 
    }

    // A <- x * x^T + A 
    template <typename SymmM, typename VctX>
    inline 
    void syr (VctX const& x, SymmM& a) { 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<SymmM>::value_type val_t; 
#else
      typedef typename SymmM::value_type val_t; 
#endif 
      syr ((val_t) 1, x, a); 
    }


    // A <- alpha * x * x^T + A 
    // A real symmetric in packed form (see leading comments for 'spmv()') 
    template <typename T, typename SymmM, typename VctX>
    inline 
    void spr (T const& alpha, VctX const& x, SymmM& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmM>::matrix_structure, 
        traits::symmetric_packed_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmM>::ordering_type
#else
           typename SymmM::orientation_category 
#endif
         >::value); 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmM>::uplo_type
#else
           typename SymmM::packed_category 
#endif 
         >::value); 

      detail::spr (stor_ord, uplo, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                   traits::vector_storage (x), 
#else
                   traits::vector_storage_const (x), 
#endif
                   traits::vector_stride (x),
                   traits::matrix_storage (a));
    }

    // A <- x * x^T + A 
    template <typename SymmM, typename VctX>
    inline 
    void spr (VctX const& x, SymmM& a) { 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<SymmM>::value_type val_t; 
#else
      typedef typename SymmM::value_type val_t; 
#endif 
      spr ((val_t) 1, x, a); 
    }


    // A <- alpha * x * y^T + alpha * y * x^T + A 
    // A real symmetric (see leading comments for 'symv()') 
    template <typename T, typename SymmM, typename VctX, typename VctY>
    inline 
    void syr2 (CBLAS_UPLO const uplo, T const& alpha, 
               VctX const& x, VctY const& y, SymmM& a) 
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmM>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmM>::ordering_type
#else
           typename SymmM::orientation_category 
#endif
         >::value); 

      detail::syr2 (stor_ord, uplo, n, alpha, 
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
                    traits::matrix_storage (a), 
                    traits::leading_dimension (a)); 
    }

    template <typename T, typename SymmM, typename VctX, typename VctY>
    inline 
    void syr2 (T const& alpha, VctX const& x, VctY const& y, SymmM& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmM>::matrix_structure, 
        traits::symmetric_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmM>::ordering_type
#else
           typename SymmM::orientation_category 
#endif
         >::value); 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmM>::uplo_type
#else
           typename SymmM::packed_category 
#endif 
         >::value); 

      detail::syr2 (stor_ord, uplo, n, alpha, 
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
                    traits::matrix_storage (a), 
                    traits::leading_dimension (a)); 
    }

    // A <- x * y^T + y * x^T + A 
    template <typename SymmM, typename VctX, typename VctY>
    inline 
    void syr2 (VctX const& x, VctY const& y, SymmM& a) { 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<SymmM>::value_type val_t; 
#else
      typedef typename SymmM::value_type val_t; 
#endif 
      syr2 ((val_t) 1, x, y, a); 
    }


    // A <- alpha * x * y^T + alpha * y * x^T + A 
    // A real symmetric in packed form (see leading comments for 'spmv()') 
    template <typename T, typename SymmM, typename VctX, typename VctY>
    inline 
    void spr2 (T const& alpha, VctX const& x, VctY const& y, SymmM& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmM>::matrix_structure, 
        traits::symmetric_packed_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmM>::ordering_type
#else
           typename SymmM::orientation_category 
#endif
         >::value); 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<SymmM>::uplo_type
#else
           typename SymmM::packed_category 
#endif 
         >::value); 

      detail::spr2 (stor_ord, uplo, n, alpha, 
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
                    traits::matrix_storage (a));
    }

    // A <- x * y^T + y * x^T + A 
    template <typename SymmM, typename VctX, typename VctY>
    inline 
    void spr2 (VctX const& x, VctY const& y, SymmM& a) { 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<SymmM>::value_type val_t; 
#else
      typedef typename SymmM::value_type val_t; 
#endif 
      spr2 ((val_t) 1, x, y, a); 
    }


    // A <- alpha * x * x^H + A 
    // A hermitian 
    template <typename T, typename HermM, typename VctX>
    inline 
    void her (CBLAS_UPLO const uplo, T const& alpha, VctX const& x, HermM& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermM>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermM>::ordering_type
#else
           typename HermM::orientation_category 
#endif
         >::value); 

      detail::her (stor_ord, uplo, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                   traits::vector_storage (x), 
#else
                   traits::vector_storage_const (x), 
#endif
                   traits::vector_stride (x),
                   traits::matrix_storage (a), 
                   traits::leading_dimension (a)); 
    }

    template <typename T, typename HermM, typename VctX>
    inline 
    void her (T const& alpha, VctX const& x, HermM& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermM>::matrix_structure, 
        traits::hermitian_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermM>::ordering_type
#else
           typename HermM::orientation_category 
#endif
         >::value); 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermM>::uplo_type
#else
           typename HermM::packed_category 
#endif 
         >::value); 

      detail::her (stor_ord, uplo, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                   traits::vector_storage (x), 
#else
                   traits::vector_storage_const (x), 
#endif
                   traits::vector_stride (x),
                   traits::matrix_storage (a), 
                   traits::leading_dimension (a)); 
    }

    // A <- x * x^H + A 
    template <typename HermM, typename VctX>
    inline 
    void her (VctX const& x, HermM& a) { 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<HermM>::value_type val_t; 
#else
      typedef typename HermM::value_type val_t; 
#endif 
      typedef typename traits::type_traits<val_t>::real_type real_t; 
      her ((real_t) 1, x, a); 
    }


    // A <- alpha * x * x^H + A 
    // A hermitian in packed form 
    template <typename T, typename HermM, typename VctX>
    inline 
    void hpr (T const& alpha, VctX const& x, HermM& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermM>::matrix_structure, 
        traits::hermitian_packed_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermM>::ordering_type
#else
           typename HermM::orientation_category 
#endif
         >::value); 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermM>::uplo_type
#else
           typename HermM::packed_category 
#endif 
         >::value); 

      detail::hpr (stor_ord, uplo, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                   traits::vector_storage (x), 
#else
                   traits::vector_storage_const (x), 
#endif
                   traits::vector_stride (x),
                   traits::matrix_storage (a));
    }

    // A <- x * x^H + A 
    template <typename HermM, typename VctX>
    inline 
    void hpr (VctX const& x, HermM& a) { 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<HermM>::value_type val_t; 
#else
      typedef typename HermM::value_type val_t; 
#endif 
      typedef typename traits::type_traits<val_t>::real_type real_t; 
      hpr ((real_t) 1, x, a); 
    }


    // A <- alpha * x * y^H + y * (alpha * x)^H + A 
    // A hermitian
    template <typename T, typename HermM, typename VctX, typename VctY>
    inline 
    void her2 (CBLAS_UPLO const uplo, T const& alpha, 
               VctX const& x, VctY const& y, HermM& a) 
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermM>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermM>::ordering_type
#else
           typename HermM::orientation_category 
#endif
         >::value); 

      detail::her2 (stor_ord, uplo, n, alpha, 
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
                    traits::matrix_storage (a), 
                    traits::leading_dimension (a)); 
    }

    template <typename T, typename HermM, typename VctX, typename VctY>
    inline 
    void her2 (T const& alpha, VctX const& x, VctY const& y, HermM& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermM>::matrix_structure, 
        traits::hermitian_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermM>::ordering_type
#else
           typename HermM::orientation_category 
#endif
         >::value); 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermM>::uplo_type
#else
           typename HermM::packed_category 
#endif 
         >::value); 

      detail::her2 (stor_ord, uplo, n, alpha, 
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
                    traits::matrix_storage (a), 
                    traits::leading_dimension (a)); 
    }

    // A <- x * y^H + y * x^H + A 
    template <typename HermM, typename VctX, typename VctY>
    inline 
    void her2 (VctX const& x, VctY const& y, HermM& a) { 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<HermM>::value_type val_t; 
#else
      typedef typename HermM::value_type val_t; 
#endif 
      her2 ((val_t) 1, x, y, a); 
    }


    // A <- alpha * x * y^H + y * (alpha * x)^H + A 
    // A hermitian in packed form 
    template <typename T, typename HermM, typename VctX, typename VctY>
    inline 
    void hpr2 (T const& alpha, VctX const& x, VctY const& y, HermM& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermM>::matrix_structure, 
        traits::hermitian_packed_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (x) >= n); 
      assert (traits::vector_size (y) >= n); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermM>::ordering_type
#else
           typename HermM::orientation_category 
#endif
         >::value); 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<HermM>::uplo_type
#else
           typename HermM::packed_category 
#endif 
         >::value); 

      detail::hpr2 (stor_ord, uplo, n, alpha, 
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
                    traits::matrix_storage (a));
    }

    // A <- x * y^H + y * x^H + A 
    template <typename HermM, typename VctX, typename VctY>
    inline 
    void hpr2 (VctX const& x, VctY const& y, HermM& a) { 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<HermM>::value_type val_t; 
#else
      typedef typename HermM::value_type val_t; 
#endif 
      hpr2 ((val_t) 1, x, y, a); 
    }


  } // namespace atlas

}}} 

#endif // BOOST_NUMERIC_BINDINGS_CBLAS_LEVEL_2_HPP
