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

#ifndef BOOST_NUMERIC_BINDINGS_CBLAS_LEVEL_3_HPP
#define BOOST_NUMERIC_BINDINGS_CBLAS_LEVEL_3_HPP

#include <cassert>

#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/traits/type_traits.hpp>
#include <boost/numeric/bindings/atlas/cblas3_overloads.hpp>
#include <boost/numeric/bindings/atlas/cblas_enum.hpp>
#include <boost/type_traits/same_traits.hpp>
#include <boost/mpl/if.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
#  include <boost/static_assert.hpp>
#endif

namespace boost { namespace numeric { namespace bindings { 

  namespace atlas {

    // C <- alpha * op (A) * op (B) + beta * C 
    // op (A) == A || A^T || A^H
    template <typename T, typename MatrA, typename MatrB, typename MatrC>
    inline
    void gemm (CBLAS_TRANSPOSE const TransA, CBLAS_TRANSPOSE const TransB, 
               T const& alpha, MatrA const& a, MatrB const& b, 
               T const& beta, MatrC& c
               )
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrA>::matrix_structure, 
        traits::general_t
      >::value)); 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
      >::value)); 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrC>::matrix_structure, 
        traits::general_t
      >::value)); 

      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrA>::ordering_type,
        typename traits::matrix_traits<MatrB>::ordering_type
      >::value)); 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrA>::ordering_type,
        typename traits::matrix_traits<MatrC>::ordering_type
      >::value)); 
#endif 

      assert (TransA == CblasNoTrans 
              || TransA == CblasTrans 
              || TransA == CblasConjTrans); 
      assert (TransB == CblasNoTrans 
              || TransB == CblasTrans 
              || TransB == CblasConjTrans); 

      int const m = TransA == CblasNoTrans
        ? traits::matrix_size1 (a)
        : traits::matrix_size2 (a);
      int const n = TransB == CblasNoTrans
        ? traits::matrix_size2 (b)
        : traits::matrix_size1 (b);
      int const k = TransA == CblasNoTrans
        ? traits::matrix_size2 (a)
        : traits::matrix_size1 (a); 
      assert (m == traits::matrix_size1 (c)); 
      assert (n == traits::matrix_size2 (c)); 
#ifndef NDEBUG
      int const k1 = TransB == CblasNoTrans
        ? traits::matrix_size1 (b)
        : traits::matrix_size2 (b);
      assert (k == k1); 
#endif
      // .. what about AtlasConj? 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<MatrA>::ordering_type
#else
           typename MatrA::orientation_category 
#endif 
         >::value); 

      detail::gemm (stor_ord, TransA, TransB, m, n, k, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::matrix_storage (a), 
#else
                    traits::matrix_storage_const (a), 
#endif
                    traits::leading_dimension (a),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                    traits::matrix_storage (b), 
#else
                    traits::matrix_storage_const (b), 
#endif
                    traits::leading_dimension (b),
                    beta, 
                    traits::matrix_storage (c), 
                    traits::leading_dimension (c)); 
    }


    // C <- alpha * A * B + beta * C 
    template <typename T, typename MatrA, typename MatrB, typename MatrC>
    inline
    void gemm (T const& alpha, MatrA const& a, MatrB const& b, 
               T const& beta, MatrC& c) 
    {
      gemm (CblasNoTrans, CblasNoTrans, alpha, a, b, beta, c) ;
    }
    

    // C <- A * B 
    template <typename MatrA, typename MatrB, typename MatrC>
    inline
    void gemm (MatrA const& a, MatrB const& b, MatrC& c) {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<MatrC>::value_type val_t; 
#else
      typedef typename MatrC::value_type val_t; 
#endif 
      gemm (CblasNoTrans, CblasNoTrans, (val_t) 1, a, b, (val_t) 0, c);
    }



    // C <- alpha * A * B + beta * C 
    // C <- alpha * B * A + beta * C 
    // A == A^T

    namespace detail {

      template <typename T, typename SymmA, typename MatrB, typename MatrC>
      inline
      void symm (CBLAS_SIDE const side, CBLAS_UPLO const uplo, 
                 T const& alpha, SymmA const& a, MatrB const& b, 
                 T const& beta, MatrC& c)
      {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrB>::matrix_structure, 
          traits::general_t
        >::value));
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrC>::matrix_structure, 
          traits::general_t
        >::value)); 

        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<SymmA>::ordering_type,
          typename traits::matrix_traits<MatrB>::ordering_type
        >::value)); 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<SymmA>::ordering_type,
          typename traits::matrix_traits<MatrC>::ordering_type
        >::value)); 
#endif 

        assert (side == CblasLeft || side == CblasRight);
        assert (uplo == CblasUpper || uplo == CblasLower); 

        int const m = traits::matrix_size1 (c);
        int const n = traits::matrix_size2 (c);

        assert (side == CblasLeft 
                ? m == traits::matrix_size1 (a) 
                  && m == traits::matrix_size2 (a)
                : n == traits::matrix_size1 (a) 
                  && n == traits::matrix_size2 (a)); 
        assert (m == traits::matrix_size1 (b) 
                && n == traits::matrix_size2 (b)); 

        CBLAS_ORDER const stor_ord
          = enum_cast<CBLAS_ORDER const>
          (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
            typename traits::matrix_traits<SymmA>::ordering_type
#else
            typename SymmA::orientation_category 
#endif 
           >::value); 

        symm (stor_ord, side, uplo,  
              m, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
              traits::matrix_storage (a), 
#else
              traits::matrix_storage_const (a), 
#endif
              traits::leading_dimension (a),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
              traits::matrix_storage (b), 
#else
              traits::matrix_storage_const (b), 
#endif
              traits::leading_dimension (b),
              beta, 
              traits::matrix_storage (c), 
              traits::leading_dimension (c)); 
      }

    } // detail 
 
    // C <- alpha * A * B + beta * C 
    // C <- alpha * B * A + beta * C 
    // A == A^T
    template <typename T, typename SymmA, typename MatrB, typename MatrC>
    inline
    void symm (CBLAS_SIDE const side, CBLAS_UPLO const uplo, 
               T const& alpha, SymmA const& a, MatrB const& b, 
               T const& beta, MatrC& c)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmA>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      detail::symm (side, uplo, alpha, a, b, beta, c); 
    }

    template <typename T, typename SymmA, typename MatrB, typename MatrC>
    inline
    void symm (CBLAS_SIDE const side, 
               T const& alpha, SymmA const& a, MatrB const& b, 
               T const& beta, MatrC& c)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmA>::matrix_structure, 
        traits::symmetric_t
      >::value)); 
#endif 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
          typename traits::matrix_traits<SymmA>::uplo_type
#else
          typename SymmA::packed_category 
#endif 
         >::value); 

      detail::symm (side, uplo, alpha, a, b, beta, c); 
    }


#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS

    namespace detail {

      // C <- alpha * A * B + beta * C ;  A == A^T
      struct symm_left {
        template <typename T, typename SymmA, typename MatrB, typename MatrC>
        static void f (T const& alpha, SymmA const& a, MatrB const& b, 
                       T const& beta, MatrC& c) 
        {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
          BOOST_STATIC_ASSERT((boost::is_same<
            typename traits::matrix_traits<SymmA>::matrix_structure, 
            traits::symmetric_t
          >::value)); 
          BOOST_STATIC_ASSERT((boost::is_same<
            typename traits::matrix_traits<MatrB>::matrix_structure, 
            traits::general_t
          >::value));
#endif 

          int const m = traits::matrix_size1 (c);
          int const n = traits::matrix_size2 (c);

          assert (m == traits::matrix_size1 (a) 
                  && m == traits::matrix_size2 (a)); 
          assert (m == traits::matrix_size1 (b) 
                  && n == traits::matrix_size2 (b)); 

          CBLAS_ORDER const stor_ord
            = enum_cast<CBLAS_ORDER const>
            (storage_order<
              typename traits::matrix_traits<SymmA>::ordering_type
             >::value); 

          CBLAS_UPLO const uplo
            = enum_cast<CBLAS_UPLO const>
            (uplo_triang<
              typename traits::matrix_traits<SymmA>::uplo_type
             >::value); 

          symm (stor_ord, CblasLeft, uplo,  
                m, n, alpha, 
                traits::matrix_storage (a), traits::leading_dimension (a),
                traits::matrix_storage (b), traits::leading_dimension (b),
                beta, 
                traits::matrix_storage (c), traits::leading_dimension (c)); 
        }
      }; 

      // C <- alpha * A * B + beta * C ;  B == B^T
      struct symm_right {
        template <typename T, typename MatrA, typename SymmB, typename MatrC>
        static void f (T const& alpha, MatrA const& a, SymmB const& b, 
                       T const& beta, MatrC& c) 
        {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
          BOOST_STATIC_ASSERT((boost::is_same<
            typename traits::matrix_traits<MatrA>::matrix_structure, 
            traits::general_t
          >::value));
          BOOST_STATIC_ASSERT((boost::is_same<
            typename traits::matrix_traits<SymmB>::matrix_structure, 
            traits::symmetric_t
          >::value));
#endif 

          int const m = traits::matrix_size1 (c);
          int const n = traits::matrix_size2 (c);

          assert (n == traits::matrix_size1 (b) 
                  && n == traits::matrix_size2 (b)); 
          assert (m == traits::matrix_size1 (a) 
                  && n == traits::matrix_size2 (a)); 

          CBLAS_ORDER const stor_ord
            = enum_cast<CBLAS_ORDER const>
            (storage_order<
              typename traits::matrix_traits<SymmB>::ordering_type
             >::value); 
 
          CBLAS_UPLO const uplo
            = enum_cast<CBLAS_UPLO const>
            (uplo_triang<
              typename traits::matrix_traits<SymmB>::uplo_type
             >::value); 

          symm (stor_ord, CblasRight, uplo,  
                m, n, alpha, 
                traits::matrix_storage (b), traits::leading_dimension (b),
                traits::matrix_storage (a), traits::leading_dimension (a),
                beta, 
                traits::matrix_storage (c), traits::leading_dimension (c)); 
        }
      }; 

    } // detail 
    
    // C <- alpha * A * B + beta * C 
    // C <- alpha * B * A + beta * C 
    // A == A^T
    template <typename T, typename MatrA, typename MatrB, typename MatrC>
    inline
    void symm (T const& alpha, MatrA const& a, MatrB const& b, 
               T const& beta, MatrC& c)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrC>::matrix_structure, 
        traits::general_t
      >::value)); 

      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrA>::ordering_type,
        typename traits::matrix_traits<MatrB>::ordering_type
      >::value)); 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrA>::ordering_type,
        typename traits::matrix_traits<MatrC>::ordering_type
      >::value)); 
#endif 

      typedef typename
        boost::mpl::if_c<
          boost::is_same<
            typename traits::matrix_traits<MatrA>::matrix_structure, 
            traits::symmetric_t
          >::value,
          detail::symm_left, 
          detail::symm_right
        >::type functor; 

      functor::f (alpha, a, b, beta, c); 
    }

    // C <- A * B  
    // C <- B * A  
    template <typename MatrA, typename MatrB, typename MatrC>
    inline
    void symm (MatrA const& a, MatrB const& b, MatrC& c) {
      typedef typename traits::matrix_traits<MatrC>::value_type val_t; 
      symm ((val_t) 1, a, b, (val_t) 0, c);
    }

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS



    // C <- alpha * A * B + beta * C 
    // C <- alpha * B * A + beta * C 
    // A == A^H

    namespace detail {

      template <typename T, typename HermA, typename MatrB, typename MatrC>
      inline
      void hemm (CBLAS_SIDE const side, CBLAS_UPLO const uplo, 
                 T const& alpha, HermA const& a, MatrB const& b, 
                 T const& beta, MatrC& c)
      {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrB>::matrix_structure, 
          traits::general_t
        >::value));
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrC>::matrix_structure, 
          traits::general_t
        >::value)); 

        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<HermA>::ordering_type,
          typename traits::matrix_traits<MatrB>::ordering_type
        >::value)); 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<HermA>::ordering_type,
          typename traits::matrix_traits<MatrC>::ordering_type
        >::value)); 
#endif 

        assert (side == CblasLeft || side == CblasRight);
        assert (uplo == CblasUpper || uplo == CblasLower); 

        int const m = traits::matrix_size1 (c);
        int const n = traits::matrix_size2 (c);

        assert (side == CblasLeft 
                ? m == traits::matrix_size1 (a) 
                  && m == traits::matrix_size2 (a)
                : n == traits::matrix_size1 (a) 
                  && n == traits::matrix_size2 (a)); 
        assert (m == traits::matrix_size1 (b) 
                && n == traits::matrix_size2 (b)); 

        CBLAS_ORDER const stor_ord
          = enum_cast<CBLAS_ORDER const>
          (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
            typename traits::matrix_traits<HermA>::ordering_type
#else
            typename HermA::orientation_category 
#endif 
           >::value); 

        hemm (stor_ord, side, uplo,  
              m, n, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
              traits::matrix_storage (a), 
#else
              traits::matrix_storage_const (a), 
#endif
              traits::leading_dimension (a),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
              traits::matrix_storage (b), 
#else
              traits::matrix_storage_const (b), 
#endif
              traits::leading_dimension (b),
              beta, 
              traits::matrix_storage (c), 
              traits::leading_dimension (c)); 
      }

    } // detail 
 
    // C <- alpha * A * B + beta * C 
    // C <- alpha * B * A + beta * C 
    // A == A^H
    template <typename T, typename HermA, typename MatrB, typename MatrC>
    inline
    void hemm (CBLAS_SIDE const side, CBLAS_UPLO const uplo, 
               T const& alpha, HermA const& a, MatrB const& b, 
               T const& beta, MatrC& c)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermA>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      detail::hemm (side, uplo, alpha, a, b, beta, c); 
    }

    template <typename T, typename HermA, typename MatrB, typename MatrC>
    inline
    void hemm (CBLAS_SIDE const side, 
               T const& alpha, HermA const& a, MatrB const& b, 
               T const& beta, MatrC& c)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermA>::matrix_structure, 
        traits::hermitian_t
      >::value)); 
#endif 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
          typename traits::matrix_traits<HermA>::uplo_type
#else
          typename HermA::packed_category 
#endif 
         >::value); 

      detail::hemm (side, uplo, alpha, a, b, beta, c); 
    }


#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS

    namespace detail {

      // C <- alpha * A * B + beta * C ;  A == A^H
      struct hemm_left {
        template <typename T, typename HermA, typename MatrB, typename MatrC>
        static void f (T const& alpha, HermA const& a, MatrB const& b, 
                       T const& beta, MatrC& c) 
        {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
          BOOST_STATIC_ASSERT((boost::is_same<
            typename traits::matrix_traits<HermA>::matrix_structure, 
            traits::hermitian_t
          >::value)); 
          BOOST_STATIC_ASSERT((boost::is_same<
            typename traits::matrix_traits<MatrB>::matrix_structure, 
            traits::general_t
          >::value));
#endif 

          int const m = traits::matrix_size1 (c);
          int const n = traits::matrix_size2 (c);

          assert (m == traits::matrix_size1 (a) 
                  && m == traits::matrix_size2 (a)); 
          assert (m == traits::matrix_size1 (b) 
                  && n == traits::matrix_size2 (b)); 

          CBLAS_ORDER const stor_ord
            = enum_cast<CBLAS_ORDER const>
            (storage_order<
              typename traits::matrix_traits<HermA>::ordering_type
             >::value); 

          CBLAS_UPLO const uplo
            = enum_cast<CBLAS_UPLO const>
            (uplo_triang<
              typename traits::matrix_traits<HermA>::uplo_type
             >::value); 

          hemm (stor_ord, CblasLeft, uplo,  
                m, n, alpha, 
                traits::matrix_storage (a), traits::leading_dimension (a),
                traits::matrix_storage (b), traits::leading_dimension (b),
                beta, 
                traits::matrix_storage (c), traits::leading_dimension (c)); 
        }
      }; 

      // C <- alpha * A * B + beta * C ;  B == B^H
      struct hemm_right {
        template <typename T, typename MatrA, typename HermB, typename MatrC>
        static void f (T const& alpha, MatrA const& a, HermB const& b, 
                       T const& beta, MatrC& c) 
        {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
          BOOST_STATIC_ASSERT((boost::is_same<
            typename traits::matrix_traits<MatrA>::matrix_structure, 
            traits::general_t
          >::value));
          BOOST_STATIC_ASSERT((boost::is_same<
            typename traits::matrix_traits<HermB>::matrix_structure, 
            traits::hermitian_t
          >::value));
#endif 

          int const m = traits::matrix_size1 (c);
          int const n = traits::matrix_size2 (c);

          assert (n == traits::matrix_size1 (b) 
                  && n == traits::matrix_size2 (b)); 
          assert (m == traits::matrix_size1 (a) 
                  && n == traits::matrix_size2 (a)); 

          CBLAS_ORDER const stor_ord
            = enum_cast<CBLAS_ORDER const>
            (storage_order<
              typename traits::matrix_traits<HermB>::ordering_type
             >::value); 
 
          CBLAS_UPLO const uplo
            = enum_cast<CBLAS_UPLO const>
            (uplo_triang<
              typename traits::matrix_traits<HermB>::uplo_type
             >::value); 

          hemm (stor_ord, CblasRight, uplo,  
                m, n, alpha, 
                traits::matrix_storage (b), traits::leading_dimension (b),
                traits::matrix_storage (a), traits::leading_dimension (a),
                beta, 
                traits::matrix_storage (c), traits::leading_dimension (c)); 
        }
      }; 

    } 
    
    template <typename T, typename MatrA, typename MatrB, typename MatrC>
    inline
    void hemm (T const& alpha, MatrA const& a, MatrB const& b, 
               T const& beta, MatrC& c)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrC>::matrix_structure, 
        traits::general_t
      >::value)); 

      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrA>::ordering_type,
        typename traits::matrix_traits<MatrB>::ordering_type
      >::value)); 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrA>::ordering_type,
        typename traits::matrix_traits<MatrC>::ordering_type
      >::value)); 
#endif 

      typedef typename
        boost::mpl::if_c<
          boost::is_same<
            typename traits::matrix_traits<MatrA>::matrix_structure, 
            traits::hermitian_t
          >::value,
          detail::hemm_left, 
          detail::hemm_right
        >::type functor; 

      functor::f (alpha, a, b, beta, c); 
    }

    // C <- A * B  
    // C <- B * A  
    template <typename MatrA, typename MatrB, typename MatrC>
    inline
    void hemm (MatrA const& a, MatrB const& b, MatrC& c) {
      typedef typename traits::matrix_traits<MatrC>::value_type val_t; 
      hemm ((val_t) 1, a, b, (val_t) 0, c);
    }

#endif // BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS

    
    // C <- alpha * A * A^T + beta * C
    // C <- alpha * A^T * A + beta * C
    // C == C^T

    namespace detail {

      template <typename T, typename MatrA, typename SymmC>
      inline
      void syrk (CBLAS_UPLO const uplo, CBLAS_TRANSPOSE trans, 
                 T const& alpha, MatrA const& a, 
                 T const& beta, SymmC& c)
      {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrA>::matrix_structure, 
          traits::general_t
        >::value));
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrA>::ordering_type,
          typename traits::matrix_traits<SymmC>::ordering_type
        >::value)); 
#endif 

        assert (uplo == CblasUpper || uplo == CblasLower); 
        assert (trans == CblasNoTrans 
                || trans == CblasTrans 
                || trans == CblasConjTrans); 

        int const n = traits::matrix_size1 (c);
        assert (n == traits::matrix_size2 (c)); 
        
        int const k = trans == CblasNoTrans
          ? traits::matrix_size2 (a)
          : traits::matrix_size1 (a); 
        assert (n == (trans == CblasNoTrans
                      ? traits::matrix_size1 (a)
                      : traits::matrix_size2 (a))); 

        CBLAS_ORDER const stor_ord
          = enum_cast<CBLAS_ORDER const>
          (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
            typename traits::matrix_traits<SymmC>::ordering_type
#else
            typename SymmC::orientation_category 
#endif 
           >::value); 

        syrk (stor_ord, uplo, trans, 
              n, k, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
              traits::matrix_storage (a), 
#else
              traits::matrix_storage_const (a), 
#endif
              traits::leading_dimension (a),
              beta, 
              traits::matrix_storage (c), 
              traits::leading_dimension (c)); 
      }

    } // detail 
 
    // C <- alpha * A * A^T + beta * C
    // C <- alpha * A^T * A + beta * C
    // C == C^T
    template <typename T, typename MatrA, typename SymmC>
    inline
    void syrk (CBLAS_UPLO const uplo, CBLAS_TRANSPOSE trans, 
               T const& alpha, MatrA const& a, 
               T const& beta, SymmC& c)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmC>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      detail::syrk (uplo, trans, alpha, a, beta, c); 
    }

    template <typename T, typename MatrA, typename SymmC>
    inline
    void syrk (CBLAS_TRANSPOSE trans, 
               T const& alpha, MatrA const& a, 
               T const& beta, SymmC& c)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmC>::matrix_structure, 
        traits::symmetric_t
      >::value)); 
#endif 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
          typename traits::matrix_traits<SymmC>::uplo_type
#else
          typename SymmC::packed_category 
#endif 
         >::value); 

      detail::syrk (uplo, trans, alpha, a, beta, c); 
    }

    // C <- A * A^T + C
    // C <- A^T * A + C
    template <typename MatrA, typename SymmC>
    inline
    void syrk (CBLAS_TRANSPOSE trans, MatrA const& a, SymmC& c) {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<SymmC>::value_type val_t; 
#else
      typedef typename SymmC::value_type val_t; 
#endif 
      syrk (trans, (val_t) 1, a, (val_t) 0, c);
    }

    
    // C <- alpha * A * B^T + conj(alpha) * B * A^T + beta * C
    // C <- alpha * A^T * B + conj(alpha) * B^T * A + beta * C
    // C == C^T

    namespace detail {

      template <typename T, typename MatrA, typename MatrB, typename SymmC>
      inline
      void syr2k (CBLAS_UPLO const uplo, CBLAS_TRANSPOSE trans, 
                  T const& alpha, MatrA const& a, MatrB const& b, 
                  T const& beta, SymmC& c)
      {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrA>::matrix_structure, 
          traits::general_t
        >::value));
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrB>::matrix_structure, 
          traits::general_t
        >::value));
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrA>::ordering_type,
          typename traits::matrix_traits<SymmC>::ordering_type
        >::value)); 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrB>::ordering_type,
          typename traits::matrix_traits<SymmC>::ordering_type
        >::value)); 
#endif 

        assert (uplo == CblasUpper || uplo == CblasLower); 
        assert (trans == CblasNoTrans 
                || trans == CblasTrans 
                || trans == CblasConjTrans); 

        int const n = traits::matrix_size1 (c);
        assert (n == traits::matrix_size2 (c)); 
        
        int const k = trans == CblasNoTrans
          ? traits::matrix_size2 (a)
          : traits::matrix_size1 (a); 
        assert (k == (trans == CblasNoTrans
                      ? traits::matrix_size2 (b)
                      : traits::matrix_size1 (b))); 
        assert (n == (trans == CblasNoTrans
                      ? traits::matrix_size1 (a)
                      : traits::matrix_size2 (a))); 
        assert (n == (trans == CblasNoTrans
                      ? traits::matrix_size1 (b)
                      : traits::matrix_size2 (b))); 

        CBLAS_ORDER const stor_ord
          = enum_cast<CBLAS_ORDER const>
          (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
            typename traits::matrix_traits<SymmC>::ordering_type
#else
            typename SymmC::orientation_category 
#endif 
           >::value); 

        syr2k (stor_ord, uplo, trans, 
               n, k, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
               traits::matrix_storage (a), 
#else
               traits::matrix_storage_const (a), 
#endif
               traits::leading_dimension (a),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
               traits::matrix_storage (b), 
#else
               traits::matrix_storage_const (b), 
#endif
               traits::leading_dimension (b),
               beta, 
               traits::matrix_storage (c), 
               traits::leading_dimension (c)); 
      }

    } // detail 
 
    // C <- alpha * A * B^T + conj(alpha) * B * A^T + beta * C
    // C <- alpha * A^T * B + conj(alpha) * B^T * A + beta * C
    // C == C^T
    template <typename T, typename MatrA, typename MatrB, typename SymmC>
    inline
    void syr2k (CBLAS_UPLO const uplo, CBLAS_TRANSPOSE trans, 
               T const& alpha, MatrA const& a, MatrB const& b, 
               T const& beta, SymmC& c)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmC>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      detail::syr2k (uplo, trans, alpha, a, b, beta, c); 
    }

    template <typename T, typename MatrA, typename MatrB, typename SymmC>
    inline
    void syr2k (CBLAS_TRANSPOSE trans, 
               T const& alpha, MatrA const& a, MatrB const& b,  
               T const& beta, SymmC& c)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmC>::matrix_structure, 
        traits::symmetric_t
      >::value)); 
#endif 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
          typename traits::matrix_traits<SymmC>::uplo_type
#else
          typename SymmC::packed_category 
#endif 
         >::value); 

      detail::syr2k (uplo, trans, alpha, a, b, beta, c); 
    }

    // C <- A * B^T + B * A^T + C
    // C <- A^T * B + B^T * A + C
    template <typename MatrA, typename MatrB, typename SymmC>
    inline
    void syr2k (CBLAS_TRANSPOSE trans, 
                MatrA const& a, MatrB const& b, SymmC& c) 
    {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<SymmC>::value_type val_t; 
#else
      typedef typename SymmC::value_type val_t; 
#endif 
      syr2k (trans, (val_t) 1, a, b, (val_t) 0, c);
    }

    
    // C <- alpha * A * A^H + beta * C
    // C <- alpha * A^H * A + beta * C
    // C == C^H

    namespace detail {

      template <typename T, typename MatrA, typename HermC>
      inline
      void herk (CBLAS_UPLO const uplo, CBLAS_TRANSPOSE trans, 
                 T const& alpha, MatrA const& a, 
                 T const& beta, HermC& c)
      {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrA>::matrix_structure, 
          traits::general_t
        >::value));
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrA>::ordering_type,
          typename traits::matrix_traits<HermC>::ordering_type
        >::value)); 
#endif 

        assert (uplo == CblasUpper || uplo == CblasLower); 
        assert (trans == CblasNoTrans || trans == CblasConjTrans); 

        int const n = traits::matrix_size1 (c);
        assert (n == traits::matrix_size2 (c)); 
        
        int const k = trans == CblasNoTrans
          ? traits::matrix_size2 (a)
          : traits::matrix_size1 (a); 
        assert (n == (trans == CblasNoTrans
                      ? traits::matrix_size1 (a)
                      : traits::matrix_size2 (a))); 

        CBLAS_ORDER const stor_ord
          = enum_cast<CBLAS_ORDER const>
          (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
            typename traits::matrix_traits<HermC>::ordering_type
#else
            typename HermC::orientation_category 
#endif 
           >::value); 

        herk (stor_ord, uplo, trans, 
              n, k, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
              traits::matrix_storage (a), 
#else
              traits::matrix_storage_const (a), 
#endif
              traits::leading_dimension (a),
              beta, 
              traits::matrix_storage (c), 
              traits::leading_dimension (c)); 
      }

    } // detail 
 
    // C <- alpha * A * A^H + beta * C
    // C <- alpha * A^H * A + beta * C
    // C == C^H
    template <typename T, typename MatrA, typename HermC>
    inline
    void herk (CBLAS_UPLO const uplo, CBLAS_TRANSPOSE trans, 
               T const& alpha, MatrA const& a, 
               T const& beta, HermC& c)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermC>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      detail::herk (uplo, trans, alpha, a, beta, c); 
    }

    template <typename T, typename MatrA, typename HermC>
    inline
    void herk (CBLAS_TRANSPOSE trans, 
               T const& alpha, MatrA const& a, 
               T const& beta, HermC& c)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermC>::matrix_structure, 
        traits::hermitian_t
      >::value)); 
#endif 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
          typename traits::matrix_traits<HermC>::uplo_type
#else
          typename HermC::packed_category 
#endif 
         >::value); 

      detail::herk (uplo, trans, alpha, a, beta, c); 
    }

    // C <- A * A^H + C
    // C <- A^H * A + C
    template <typename MatrA, typename HermC>
    inline
    void herk (CBLAS_TRANSPOSE trans, MatrA const& a, HermC& c) {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<HermC>::value_type val_t; 
#else
      typedef typename HermC::value_type val_t; 
#endif 
      typedef typename traits::type_traits<val_t>::real_type real_t; 
      herk (trans, (real_t) 1, a, (real_t) 0, c);
    }


    // C <- alpha * A * B^H + conj(alpha) * B * A^H + beta * C
    // C <- alpha * A^H * B + conj(alpha) * B^H * A + beta * C
    // C == C^H

    namespace detail {

      template <typename T1, typename T2, 
                typename MatrA, typename MatrB, typename HermC>
      inline
      void her2k (CBLAS_UPLO const uplo, CBLAS_TRANSPOSE trans, 
                  T1 const& alpha, MatrA const& a, MatrB const& b, 
                  T2 const& beta, HermC& c)
      {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrA>::matrix_structure, 
          traits::general_t
        >::value));
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrB>::matrix_structure, 
          traits::general_t
        >::value));
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrA>::ordering_type,
          typename traits::matrix_traits<HermC>::ordering_type
        >::value)); 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<MatrB>::ordering_type,
          typename traits::matrix_traits<HermC>::ordering_type
        >::value)); 
#endif 

        assert (uplo == CblasUpper || uplo == CblasLower); 
        assert (trans == CblasNoTrans || trans == CblasConjTrans); 

        int const n = traits::matrix_size1 (c);
        assert (n == traits::matrix_size2 (c)); 
        
        int const k = trans == CblasNoTrans
          ? traits::matrix_size2 (a)
          : traits::matrix_size1 (a); 
        assert (k == (trans == CblasNoTrans
                      ? traits::matrix_size2 (b)
                      : traits::matrix_size1 (b))); 
        assert (n == (trans == CblasNoTrans
                      ? traits::matrix_size1 (a)
                      : traits::matrix_size2 (a))); 
        assert (n == (trans == CblasNoTrans
                      ? traits::matrix_size1 (b)
                      : traits::matrix_size2 (b))); 

        CBLAS_ORDER const stor_ord
          = enum_cast<CBLAS_ORDER const>
          (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
            typename traits::matrix_traits<HermC>::ordering_type
#else
            typename HermC::orientation_category 
#endif 
           >::value); 

        her2k (stor_ord, uplo, trans, 
               n, k, alpha, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
               traits::matrix_storage (a), 
#else
               traits::matrix_storage_const (a), 
#endif
               traits::leading_dimension (a),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
               traits::matrix_storage (b), 
#else
               traits::matrix_storage_const (b), 
#endif
               traits::leading_dimension (b),
               beta, 
               traits::matrix_storage (c), 
               traits::leading_dimension (c)); 
      }

    } // detail 
 
    // C <- alpha * A * B^H + conj(alpha) * B * A^H + beta * C
    // C <- alpha * A^H * B + conj(alpha) * B^H * A + beta * C
    // C == C^H
    template <typename T1, typename T2, 
              typename MatrA, typename MatrB, typename HermC>
    inline
    void her2k (CBLAS_UPLO const uplo, CBLAS_TRANSPOSE trans, 
               T1 const& alpha, MatrA const& a, MatrB const& b, 
               T2 const& beta, HermC& c)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermC>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      detail::her2k (uplo, trans, alpha, a, b, beta, c); 
    }

    template <typename T1, typename T2, 
              typename MatrA, typename MatrB, typename HermC>
    inline
    void her2k (CBLAS_TRANSPOSE trans, 
               T1 const& alpha, MatrA const& a, MatrB const& b,  
               T2 const& beta, HermC& c)
    {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermC>::matrix_structure, 
        traits::hermitian_t
      >::value)); 
#endif 

      CBLAS_UPLO const uplo
        = enum_cast<CBLAS_UPLO const>
        (uplo_triang<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
          typename traits::matrix_traits<HermC>::uplo_type
#else
          typename HermC::packed_category 
#endif 
         >::value); 

      detail::her2k (uplo, trans, alpha, a, b, beta, c); 
    }

    // C <- A * B^H + B * A^H + C
    // C <- A^H * B + B^H * A + C
    template <typename MatrA, typename MatrB, typename HermC>
    inline
    void her2k (CBLAS_TRANSPOSE trans, 
                MatrA const& a, MatrB const& b, HermC& c) 
    {
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<HermC>::value_type val_t; 
#else
      typedef typename HermC::value_type val_t; 
#endif 
      typedef typename traits::type_traits<val_t>::real_type real_t; 
      her2k (trans, (val_t) 1, a, b, (real_t) 0, c);
    }

    
  } // namespace atlas

}}} 

#endif // BOOST_NUMERIC_BINDINGS_CBLAS_LEVEL_3_HPP
