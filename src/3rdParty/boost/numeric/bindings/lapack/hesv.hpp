/*
 * 
 * Copyright (c) Toon Knapen & Kresimir Fresl 2003
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * KF acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_HESV_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_HESV_HPP

#include <boost/numeric/bindings/traits/type_traits.hpp>
#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/lapack/lapack.h>
#include <boost/numeric/bindings/lapack/ilaenv.hpp>
#include <boost/numeric/bindings/traits/detail/array.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
#  include <boost/static_assert.hpp>
#  include <boost/type_traits/is_same.hpp>
#endif 

#include <cassert>


namespace boost { namespace numeric { namespace bindings { 

  namespace lapack {

    /////////////////////////////////////////////////////////////////////
    //
    // system of linear equations A * X = B
    // with A Hermitian indefinite matrix
    //
    /////////////////////////////////////////////////////////////////////

    namespace detail {

      inline 
      int hetrf_block (traits::complex_f, 
                       int const ispec, char const ul, int const n) 
      {
        char ul2[2] = "x"; ul2[0] = ul; 
        return ilaenv (ispec, "CHETRF", ul2, n); 
      }
      inline 
      int hetrf_block (traits::complex_d, 
                       int const ispec, char const ul, int const n) 
      {
        char ul2[2] = "x"; ul2[0] = ul; 
        return ilaenv (ispec, "ZHETRF", ul2, n); 
      }

    }


    template <typename HermA>
    inline
    int hetrf_block (char const q, char const ul, HermA const& a) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermA>::matrix_structure, 
        traits::general_t
      >::value));
#endif
      assert (q == 'O' || q == 'M'); 
      assert (ul == 'U' || ul == 'L'); 

      int n = traits::matrix_size1 (a); 
      assert (n == traits::matrix_size2 (a)); 

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<HermA>::value_type val_t; 
#else 
      typedef typename HermA::value_type val_t; 
#endif 
      int ispec = (q == 'O' ? 1 : 2); 
      return detail::hetrf_block (val_t(), ispec, ul, n); 
    }

    template <typename HermA>
    inline
    int hetrf_block (char const q, HermA const& a) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermA>::matrix_structure, 
        traits::hermitian_t
      >::value));
#endif
      assert (q == 'O' || q == 'M'); 

      char ul = traits::matrix_uplo_tag (a);
      int n = traits::matrix_size1 (a); 
      assert (n == traits::matrix_size2 (a)); 

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<HermA>::value_type val_t; 
#else 
      typedef typename HermA::value_type val_t; 
#endif 
      int ispec = (q == 'O' ? 1 : 2); 
      return detail::hetrf_block (val_t(), ispec, ul, n); 
    }

    template <typename HermA>
    inline
    int hetrf_work (char const q, char const ul, HermA const& a) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermA>::matrix_structure, 
        traits::general_t
      >::value));
#endif
      assert (q == 'O' || q == 'M'); 
      assert (ul == 'U' || ul == 'L'); 

      int n = traits::matrix_size1 (a); 
      assert (n == traits::matrix_size2 (a)); 

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<HermA>::value_type val_t; 
#else 
      typedef typename HermA::value_type val_t; 
#endif 
      int lw = -13; 
      if (q == 'M') 
        lw = 1;
      if (q == 'O') 
        lw = n * detail::hetrf_block (val_t(), 1, ul, n); 
      return lw; 
    }

    template <typename HermA>
    inline
    int hetrf_work (char const q, HermA const& a) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermA>::matrix_structure, 
        traits::hermitian_t
      >::value));
#endif
      assert (q == 'O' || q == 'M'); 

      char ul = traits::matrix_uplo_tag (a);
      int n = traits::matrix_size1 (a); 
      assert (n == traits::matrix_size2 (a)); 

#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<HermA>::value_type val_t; 
#else 
      typedef typename HermA::value_type val_t; 
#endif 
      int lw = -13; 
      if (q == 'M') 
        lw = 1;
      if (q == 'O') 
        lw = n * detail::hetrf_block (val_t(), 1, ul, n); 
      return lw; 
    }


    template <typename HermA>
    inline
    int hesv_work (char const q, char const ul, HermA const& a) {
      return hetrf_work (q, ul, a); 
    }

    template <typename HermA>
    inline
    int hesv_work (char const q, HermA const& a) { return hetrf_work (q, a); }


    /*
     * hesv() computes the solution to a system of linear equations 
     * A * X = B, where A is an N-by-N Hermitian matrix and X and B 
     * are N-by-NRHS matrices.
     *
     * The diagonal pivoting method is used to factor A as
     *   A = U * D * U^H,  if UPLO = 'U', 
     *   A = L * D * L^H,  if UPLO = 'L',
     * where  U (or L) is a product of permutation and unit upper 
     * (lower) triangular matrices, and D is Hermitian and block 
     * diagonal with 1-by-1 and 2-by-2 diagonal blocks. The factored 
     * form of A is then used to solve the system of equations A * X = B.
     */

    namespace detail {

      inline 
      void hesv (char const uplo, int const n, int const nrhs,
                 traits::complex_f* a, int const lda, int* ipiv,  
                 traits::complex_f* b, int const ldb, 
                 traits::complex_f* w, int const lw, int* info) 
      {
        LAPACK_CHESV (&uplo, &n, &nrhs, 
                      traits::complex_ptr (a), &lda, ipiv, 
                      traits::complex_ptr (b), &ldb, 
                      traits::complex_ptr (w), &lw, info);
      }

      inline 
      void hesv (char const uplo, int const n, int const nrhs,
                 traits::complex_d* a, int const lda, int* ipiv, 
                 traits::complex_d* b, int const ldb, 
                 traits::complex_d* w, int const lw, int* info) 
      {
        LAPACK_ZHESV (&uplo, &n, &nrhs, 
                      traits::complex_ptr (a), &lda, ipiv, 
                      traits::complex_ptr (b), &ldb, 
                      traits::complex_ptr (w), &lw, info);
      }

      template <typename HermA, typename MatrB, typename IVec, typename Work>
      inline
      int hesv (char const ul, HermA& a, IVec& i, MatrB& b, Work& w) {

        int const n = traits::matrix_size1 (a);
        assert (n == traits::matrix_size2 (a)); 
        assert (n == traits::matrix_size1 (b)); 
        assert (n == traits::vector_size (i)); 

        int info; 
        hesv (ul, n, traits::matrix_size2 (b), 
              traits::matrix_storage (a), 
              traits::leading_dimension (a),
              traits::vector_storage (i),  
              traits::matrix_storage (b),
              traits::leading_dimension (b),
              traits::vector_storage (w), 
              traits::vector_size (w), 
              &info);
        return info; 
      }

    }

    template <typename HermA, typename MatrB, typename IVec, typename Work>
    inline
    int hesv (char const ul, HermA& a, IVec& i, MatrB& b, Work& w) {

      assert (ul == 'U' || ul == 'L'); 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermA>::matrix_structure, 
        traits::general_t
      >::value));
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
      >::value));
#endif

      int const lw = traits::vector_size (w); 
      assert (lw >= 1); 
      return detail::hesv (ul, a, i, b, w); 
    }

    template <typename HermA, typename MatrB, typename IVec, typename Work>
    inline
    int hesv (HermA& a, IVec& i, MatrB& b, Work& w) {

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

      int const lw = traits::vector_size (w); 
      assert (lw >= 1); 
      char uplo = traits::matrix_uplo_tag (a);
      return detail::hesv (uplo, a, i, b, w); 
    }

    template <typename HermA, typename MatrB>
    inline
    int hesv (char const ul, HermA& a, MatrB& b) {
      // with 'internal' pivot and work vectors 

      assert (ul == 'U' || ul == 'L'); 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermA>::matrix_structure, 
        traits::general_t
      >::value));
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
      >::value));
#endif

      int const n = traits::matrix_size1 (a); 
      int info = -101; 
      traits::detail::array<int> i (n); 

      if (i.valid()) {
        info = -102; 
        int lw = hetrf_work ('O', ul, a); 
        assert (lw >= 1); // paranoia ? 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
        typedef typename traits::matrix_traits<HermA>::value_type val_t; 
#else 
        typedef typename HermA::value_type val_t; 
#endif 
        traits::detail::array<val_t> w (lw); 
        if (w.valid()) 
          info = detail::hesv (ul, a, i, b, w); 
      }
      return info; 
    }

    template <typename HermA, typename MatrB>
    inline
    int hesv (HermA& a, MatrB& b) {
      // with 'internal' pivot and work vectors 

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

      int const n = traits::matrix_size1 (a); 
      char uplo = traits::matrix_uplo_tag (a);
      int info = -101; 
      traits::detail::array<int> i (n); 

      if (i.valid()) {
        info = -102; 
        int lw = hetrf_work ('O', a); 
        assert (lw >= 1); // paranoia ? 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
        typedef typename traits::matrix_traits<HermA>::value_type val_t; 
#else 
        typedef typename HermA::value_type val_t; 
#endif 
        traits::detail::array<val_t> w (lw); 
        w.resize (lw); 
        if (w.valid()) 
          info = detail::hesv (uplo, a, i, b, w); 
      }
      return info; 
    }


    /*
     * hetrf() computes the factorization of a Hermitian matrix A using
     * the  Bunch-Kaufman diagonal pivoting method. The form of the 
     * factorization is
     *    A = U * D * U^H  or  A = L * D * L^H
     * where U (or L) is a product of permutation and unit upper (lower)  
     * triangular matrices, and D is Hermitian and block diagonal with 
     * 1-by-1 and 2-by-2 diagonal blocks.
     */

    namespace detail {

      inline 
      void hetrf (char const uplo, int const n, 
                  traits::complex_f* a, int const lda, int* ipiv,  
                  traits::complex_f* w, int const lw, int* info) 
      {
        LAPACK_CHETRF (&uplo, &n, 
                       traits::complex_ptr (a), &lda, ipiv, 
                       traits::complex_ptr (w), &lw, info);
      }

      inline 
      void hetrf (char const uplo, int const n, 
                  traits::complex_d* a, int const lda, int* ipiv, 
                  traits::complex_d* w, int const lw, int* info) 
      {
        LAPACK_ZHETRF (&uplo, &n, 
                       traits::complex_ptr (a), &lda, ipiv, 
                       traits::complex_ptr (w), &lw, info);
      }

      template <typename HermA, typename IVec, typename Work>
      inline
      int hetrf (char const ul, HermA& a, IVec& i, Work& w) {

        int const n = traits::matrix_size1 (a);
        assert (n == traits::matrix_size2 (a)); 
        assert (n == traits::vector_size (i)); 

        int info; 
        hetrf (ul, n, traits::matrix_storage (a), 
               traits::leading_dimension (a),
               traits::vector_storage (i),  
               traits::vector_storage (w), 
               traits::vector_size (w), 
               &info);
        return info; 
      }

    }

    template <typename HermA, typename IVec, typename Work>
    inline
    int hetrf (char const ul, HermA& a, IVec& i, Work& w) {

      assert (ul == 'U' || ul == 'L'); 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermA>::matrix_structure, 
        traits::general_t
      >::value));
#endif

      int const lw = traits::vector_size (w); 
      assert (lw >= 1); 
      return detail::hetrf (ul, a, i, w); 
    }

    template <typename HermA, typename IVec, typename Work>
    inline
    int hetrf (HermA& a, IVec& i, Work& w) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermA>::matrix_structure, 
        traits::hermitian_t
      >::value));
#endif

      int const lw = traits::vector_size (w); 
      assert (lw >= 1); 
      char uplo = traits::matrix_uplo_tag (a);
      return detail::hetrf (uplo, a, i, w); 
    }

    template <typename HermA, typename Ivec>
    inline
    int hetrf (char const ul, HermA& a, Ivec& i) {
      // with 'internal' work vector

      assert (ul == 'U' || ul == 'L'); 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermA>::matrix_structure, 
        traits::general_t
      >::value));
#endif

      int info = -101; 
      int lw = hetrf_work ('O', ul, a); 
      assert (lw >= 1); // paranoia ? 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<HermA>::value_type val_t; 
#else 
      typedef typename HermA::value_type val_t; 
#endif 
      traits::detail::array<val_t> w (lw); 
      if (w.valid()) 
        info = detail::hetrf (ul, a, i, w); 
      return info; 
    }

    template <typename HermA, typename Ivec>
    inline
    int hetrf (HermA& a, Ivec& i) {
      // with 'internal' work vector 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermA>::matrix_structure, 
        traits::hermitian_t
      >::value));
#endif

      char uplo = traits::matrix_uplo_tag (a);
      int info = -101; 
      int lw = hetrf_work ('O', a); 
      assert (lw >= 1); // paranoia ? 
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
      typedef typename traits::matrix_traits<HermA>::value_type val_t; 
#else 
      typedef typename HermA::value_type val_t; 
#endif 
      traits::detail::array<val_t> w (lw); 
      if (w.valid()) 
        info = detail::hetrf (uplo, a, i, w); 
      return info; 
    }


    /*
     * hetrs() solves a system of linear equations A*X = B with 
     * a Hermitian matrix A using the factorization 
     *    A = U * D * U^H   or  A = L * D * L^H
     * computed by hetrf().
     */

    namespace detail {

      inline 
      void hetrs (char const uplo, int const n, int const nrhs,
                  traits::complex_f const* a, int const lda, 
                  int const* ipiv,  
                  traits::complex_f* b, int const ldb, int* info) 
      {
        LAPACK_CHETRS (&uplo, &n, &nrhs, 
                       traits::complex_ptr (a), &lda, ipiv, 
                       traits::complex_ptr (b), &ldb, info);
      }

      inline 
      void hetrs (char const uplo, int const n, int const nrhs,
                  traits::complex_d const* a, int const lda, 
                  int const* ipiv, 
                  traits::complex_d* b, int const ldb, int* info) 
      {
        LAPACK_ZHETRS (&uplo, &n, &nrhs, 
                       traits::complex_ptr (a), &lda, ipiv, 
                       traits::complex_ptr (b), &ldb, info);
      }

      template <typename HermA, typename MatrB, typename IVec>
      inline
      int hetrs (char const ul, HermA const& a, IVec const& i, MatrB& b) {

        int const n = traits::matrix_size1 (a);
        assert (n == traits::matrix_size2 (a)); 
        assert (n == traits::matrix_size1 (b)); 
        assert (n == traits::vector_size (i)); 

        int info; 
        hetrs (ul, n, traits::matrix_size2 (b), 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
               traits::matrix_storage (a), 
#else
               traits::matrix_storage_const (a), 
#endif 
               traits::leading_dimension (a),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
               traits::vector_storage (i),  
#else
               traits::vector_storage_const (i),  
#endif
               traits::matrix_storage (b),
               traits::leading_dimension (b), &info);
        return info; 
      }

    }

    template <typename HermA, typename MatrB, typename IVec>
    inline
    int hetrs (char const ul, HermA const& a, IVec const& i, MatrB& b) {

      assert (ul == 'U' || ul == 'L'); 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<HermA>::matrix_structure, 
        traits::general_t
      >::value));
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
      >::value));
#endif

      return detail::hetrs (ul, a, i, b); 
    }

    template <typename HermA, typename MatrB, typename IVec>
    inline
    int hetrs (HermA const& a, IVec const& i, MatrB& b) {

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

      char uplo = traits::matrix_uplo_tag (a);
      return detail::hetrs (uplo, a, i, b); 
    }


    // TO DO: hetri

  }

}}}

#endif 
