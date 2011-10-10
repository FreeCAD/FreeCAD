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

#ifndef BOOST_NUMERIC_BINDINGS_CLAPACK_HPP
#define BOOST_NUMERIC_BINDINGS_CLAPACK_HPP

#include <cassert>
#include <new>

#include <boost/numeric/bindings/traits/traits.hpp>
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
#  include <boost/numeric/bindings/traits/detail/symm_herm_traits.hpp>
#endif 
#include <boost/numeric/bindings/atlas/cblas_enum.hpp>

// see libs/numeric/bindings/atlas/doc/index.html, section 2.5.2
//#define BOOST_NUMERIC_BINDINGS_ATLAS_POTRF_BUG

#include <boost/numeric/bindings/atlas/clapack_overloads.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
#  include <boost/static_assert.hpp>
#  include <boost/type_traits/same_traits.hpp>
#endif


namespace boost { namespace numeric { namespace bindings { 

  namespace atlas {

    /////////////////////////////////////////////////////////////////////
    //
    // general system of linear equations A * X = B
    // 
    /////////////////////////////////////////////////////////////////////

    // gesv(): 'driver' function 
    //
    // [comments from 'clapack_dgesv.c':]
    /* clapack_xgesv computes the solution to a system of linear equations
     *   A * X = B,
     * where A is an N-by-N matrix and X and B are N-by-NRHS matrices.
     */
    // [but ATLAS FAQ says:]
    /* What's the deal with the RHS in the row-major factorization/solves?
     * Most users are confused by the row major factorization and related 
     * solves. The right-hand side vectors are probably the biggest source 
     * of confusion. The RHS array does not represent a matrix in the 
     * mathematical sense, it is instead a pasting together of the various 
     * RHS into one array for calling convenience. As such, RHS vectors are 
     * always stored contiguously, regardless of the row/col major that is 
     * chosen. This means that ldb/ldx is always independent of NRHS, and 
     * dependant on N, regardless of the row/col major setting. 
     */ 
    // That is, it seems that, if B is row-major, it should be NRHS-by-N, 
    // and RHS vectors should be its rows, not columns. 
    //
    // [comments from 'clapack_dgesv.c':]
    /* The LU factorization used to factor A is dependent on the Order 
     * parameter, as detailed in the leading comments of clapack_dgetrf.
     * The factored form of A is then used to solve the system of equations 
     *   A * X = B.
     * A is overwritten with the appropriate LU factorization, and B [...]
     * is overwritten with the solution X on output.
     */
    // If B is row-major, solution vectors are its rows. 
    template <typename MatrA, typename MatrB, typename IVec>
    inline
    int gesv (MatrA& a, IVec& ipiv, MatrB& b) {
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
        typename traits::matrix_traits<MatrB>::ordering_type
      >::value)); 
#endif 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<MatrA>::ordering_type
#else
           typename MatrA::orientation_category 
#endif 
         >::value); 

      int const n = traits::matrix_size1 (a);
      int const nrhs = stor_ord == CblasColMajor
        ? traits::matrix_size2 (b)
        : traits::matrix_size1 (b); 
      assert (n == traits::matrix_size2 (a)); 
      assert (n == (stor_ord == CblasColMajor
                    ? traits::matrix_size1 (b)
                    : traits::matrix_size2 (b))); 
      assert (n == traits::vector_size (ipiv)); 

      return detail::gesv (stor_ord, n, nrhs, 
                           traits::matrix_storage (a), 
                           traits::leading_dimension (a),
                           traits::vector_storage (ipiv),  
                           traits::matrix_storage (b),
                           traits::leading_dimension (b));
    }

    template <typename MatrA, typename MatrB>
    inline
    int gesv (MatrA& a, MatrB& b) {
      // with 'internal' pivot vector
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
        typename traits::matrix_traits<MatrB>::ordering_type
      >::value)); 
#endif 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<MatrA>::ordering_type
#else
           typename MatrA::orientation_category 
#endif 
         >::value); 

      int const n = traits::matrix_size1 (a);
      int const nrhs = stor_ord == CblasColMajor
        ? traits::matrix_size2 (b)
        : traits::matrix_size1 (b); 
      assert (n == traits::matrix_size2 (a)); 
      assert (n == (stor_ord == CblasColMajor
                    ? traits::matrix_size1 (b)
                    : traits::matrix_size2 (b))); 

      int *ipiv = new (std::nothrow) int[n]; 
      int ierr = -101;  
      // clapack_dgesv() errors: 
      //   if (ierr == 0), successful
      //   if (ierr < 0), the -ierr argument had an illegal value
      //   -- we will use -101 if allocation fails
      //   if (ierr > 0), U(i-1,i-1) (or L(i-1,i-1)) is exactly zero 
 
      if (ipiv) {
        ierr = detail::gesv (stor_ord, n, nrhs, 
                             traits::matrix_storage (a), 
                             traits::leading_dimension (a),
                             ipiv,  
                             traits::matrix_storage (b),
                             traits::leading_dimension (b));
        delete[] ipiv; 
      }
      return ierr; 
    }

    template <typename MatrA, typename MatrB>
    inline
    int lu_solve (MatrA& a, MatrB& b) {
      return gesv (a, b); 
    }


    // getrf(): LU factorization of A
    // [comments from 'clapack_dgetrf.c':]
    /* Computes one of two LU factorizations based on the setting of 
     * the Order parameter, as follows:
     * ---------------------------------------------------------------
     *                     Order == CblasColMajor
     * Column-major factorization of form
     *   A = P * L * U
     * where P is a row-permutation matrix, L is lower triangular with
     * unit diagonal elements (lower trapezoidal if M > N), and U is 
     * upper triangular (upper trapezoidal if M < N).
     * ---------------------------------------------------------------
     *                     Order == CblasRowMajor
     * Row-major factorization of form
     *   A = P * L * U
     * where P is a column-permutation matrix, L is lower triangular 
     * (lower trapezoidal if M > N), and U is upper triangular with 
     * unit diagonals (upper trapezoidal if M < N).
     */
    template <typename MatrA, typename IVec> 
    inline
    int getrf (MatrA& a, IVec& ipiv) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrA>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<MatrA>::ordering_type
#else
           typename MatrA::orientation_category 
#endif 
         >::value); 

      int const m = traits::matrix_size1 (a);
      int const n = traits::matrix_size2 (a); 
      assert (traits::vector_size (ipiv) == (m < n ? m : n)); 

      return detail::getrf (stor_ord, m, n, 
                            traits::matrix_storage (a), 
                            traits::leading_dimension (a),
                            traits::vector_storage (ipiv)); 
    }

    template <typename MatrA, typename IVec> 
    inline
    int lu_factor (MatrA& a, IVec& ipiv) {
      return getrf (a, ipiv); 
    }


    // getrs(): solves a system of linear equations
    //          A * X = B  or  A' * X = B
    //          using the LU factorization previously computed by getrf()
    template <typename MatrA, typename MatrB, typename IVec>
    inline
    int getrs (CBLAS_TRANSPOSE const Trans, 
               MatrA const& a, IVec const& ipiv, MatrB& b) 
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
        typename traits::matrix_traits<MatrB>::ordering_type
      >::value)); 
#endif 

      assert (Trans == CblasNoTrans 
              || Trans == CblasTrans 
              || Trans == CblasConjTrans); 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<MatrA>::ordering_type
#else
           typename MatrA::orientation_category 
#endif 
         >::value); 

      int const n = traits::matrix_size1 (a);
      int const nrhs = stor_ord == CblasColMajor
        ? traits::matrix_size2 (b)
        : traits::matrix_size1 (b); 
      assert (n == traits::matrix_size2 (a)); 
      assert (n == (stor_ord == CblasColMajor
                    ? traits::matrix_size1 (b)
                    : traits::matrix_size2 (b))); 
      assert (n == traits::vector_size (ipiv)); 
      
      return detail::getrs (stor_ord, Trans, n, nrhs, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                            traits::matrix_storage (a), 
#else
                            traits::matrix_storage_const (a), 
#endif 
                            traits::leading_dimension (a),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                            traits::vector_storage (ipiv),  
#else
                            traits::vector_storage_const (ipiv),  
#endif
                            traits::matrix_storage (b),
                            traits::leading_dimension (b)); 
    }

    // getrs(): solves A * X = B (after getrf())
    template <typename MatrA, typename MatrB, typename IVec>
    inline
    int getrs (MatrA const& a, IVec const& ipiv, MatrB& b) {
      return getrs (CblasNoTrans, a, ipiv, b); 
    }

    template <typename MatrA, typename MatrB, typename IVec>
    inline
    int lu_substitute (MatrA const& a, IVec const& ipiv, MatrB& b) {
      return getrs (CblasNoTrans, a, ipiv, b); 
    }


    // getri(): computes the inverse of a matrix A 
    //          using the LU factorization previously computed by getrf() 
    template <typename MatrA, typename IVec> 
    inline
    int getri (MatrA& a, IVec const& ipiv) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrA>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      CBLAS_ORDER const stor_ord
        = enum_cast<CBLAS_ORDER const>
        (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
           typename traits::matrix_traits<MatrA>::ordering_type
#else
           typename MatrA::orientation_category 
#endif 
         >::value); 

      int const n = traits::matrix_size1 (a);
      assert (n == traits::matrix_size2 (a)); 
      assert (traits::vector_size (ipiv) == n); 

      return detail::getri (stor_ord, n, 
                            traits::matrix_storage (a), 
                            traits::leading_dimension (a),
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                            traits::vector_storage (ipiv)
#else
                            traits::vector_storage_const (ipiv)
#endif
                            ); 
    }

    template <typename MatrA, typename IVec> 
    inline
    int lu_invert (MatrA& a, IVec& ipiv) {
      return getri (a, ipiv); 
    }



    /////////////////////////////////////////////////////////////////////
    //
    // system of linear equations A * X = B
    // with A symmetric or Hermitian positive definite matrix
    //
    /////////////////////////////////////////////////////////////////////

#ifndef BOOST_NUMERIC_BINDINGS_ATLAS_POTRF_BUG
    // posv(): 'driver' function 
    //
    // [from 'dposv.f' (slightly edited):]
    /* XPOSV computes the solution to a system of linear equations
     *    A * X = B,
     * where A is an N-by-N symmetric/Hermitian positive definite matrix 
     * and X and B are N-by-NRHS matrices. [See also comments of gesv().]
     *
     * A -- On entry, the symmetric/Hermitian matrix A.  
     * If UPLO = 'U', the leading N-by-N upper triangular part of A 
     * contains the upper triangular part of the matrix A, and the 
     * strictly lower triangular part of A is not referenced.  
     * If UPLO = 'L', the leading N-by-N lower triangular part of A 
     * contains the lower triangular part of the matrix A, and the 
     * strictly upper triangular part of A is not referenced.
     *
     * On exit, if INFO = 0, the factor U or L from the Cholesky
     * factorization A = U**T*U or A = L*L**T
     * [or A = U**H*U or A = L*L**H]. 
     *
     * B -- On entry, the right hand side matrix B.
     * On exit, if INFO = 0, the solution matrix X.
     */
    namespace detail {

      template <typename SymmA, typename MatrB>
      inline
      int posv (CBLAS_UPLO const uplo, SymmA& a, MatrB& b) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<SymmA>::ordering_type,
          typename traits::matrix_traits<MatrB>::ordering_type
        >::value)); 
#endif 

        CBLAS_ORDER const stor_ord
          = enum_cast<CBLAS_ORDER const>
          (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
            typename traits::matrix_traits<SymmA>::ordering_type
#else
            typename SymmA::orientation_category 
#endif 
           >::value); 

        int const n = traits::matrix_size1 (a);
        int const nrhs = stor_ord == CblasColMajor
          ? traits::matrix_size2 (b)
          : traits::matrix_size1 (b); 
        assert (n == traits::matrix_size2 (a)); 
        assert (n == (stor_ord == CblasColMajor
                      ? traits::matrix_size1 (b)
                      : traits::matrix_size2 (b))); 

        return posv (stor_ord, uplo, n, nrhs, 
                     traits::matrix_storage (a), 
                     traits::leading_dimension (a),
                     traits::matrix_storage (b),
                     traits::leading_dimension (b));
      }

    } // detail 

    template <typename SymmA, typename MatrB>
    inline
    int posv (CBLAS_UPLO const uplo, SymmA& a, MatrB& b) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmA>::matrix_structure, 
        traits::general_t
      >::value));
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
      >::value));
#endif
      assert (uplo == CblasUpper || uplo == CblasLower); 
      return detail::posv (uplo, a, b); 
    }

    template <typename SymmA, typename MatrB>
    inline
    int posv (SymmA& a, MatrB& b) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      typedef typename traits::matrix_traits<SymmA>::value_type val_t;
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmA>::matrix_structure, 
        typename traits::detail::symm_herm_t<val_t>::type
      >::value)); 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
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
      
      return detail::posv (uplo, a, b); 
    }

    template <typename SymmA, typename MatrB>
    inline
    int cholesky_solve (SymmA& a, MatrB& b) { return posv (a, b); }
#endif // BOOST_NUMERIC_BINDINGS_ATLAS_POTRF_BUG


    // potrf(): Cholesky factorization of A 
    namespace detail {

      template <typename SymmA>
      inline
      int potrf (CBLAS_UPLO const uplo, SymmA& a) {

        CBLAS_ORDER const stor_ord
          = enum_cast<CBLAS_ORDER const>
          (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
            typename traits::matrix_traits<SymmA>::ordering_type
#else
            typename SymmA::orientation_category 
#endif 
           >::value); 

        int const n = traits::matrix_size1 (a);
        assert (n == traits::matrix_size2 (a)); 

        return potrf (stor_ord, uplo, n, 
                      traits::matrix_storage (a), 
                      traits::leading_dimension (a));
      }

    } // detail 

    template <typename SymmA>
    inline
    int potrf (CBLAS_UPLO const uplo, SymmA& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmA>::matrix_structure, 
        traits::general_t
      >::value));
#endif
      assert (uplo == CblasUpper || uplo == CblasLower); 
      return detail::potrf (uplo, a); 
    } 

    template <typename SymmA>
    inline
    int potrf (SymmA& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      typedef typename traits::matrix_traits<SymmA>::value_type val_t;
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmA>::matrix_structure, 
        typename traits::detail::symm_herm_t<val_t>::type
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
      
      return detail::potrf (uplo, a); 
    }

    template <typename SymmA>
    inline
    int cholesky_factor (SymmA& a) { return potrf (a); }


    // potrs(): solves a system of linear equations A * X = B
    //          using the Cholesky factorization computed by potrf()
    namespace detail {

      template <typename SymmA, typename MatrB>
      inline
      int potrs (CBLAS_UPLO const uplo, SymmA const& a, MatrB& b) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
        BOOST_STATIC_ASSERT((boost::is_same<
          typename traits::matrix_traits<SymmA>::ordering_type,
          typename traits::matrix_traits<MatrB>::ordering_type
        >::value)); 
#endif 

        CBLAS_ORDER const stor_ord
          = enum_cast<CBLAS_ORDER const>
          (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
            typename traits::matrix_traits<SymmA>::ordering_type
#else
            typename SymmA::orientation_category 
#endif 
           >::value); 

        int const n = traits::matrix_size1 (a);
        int const nrhs = stor_ord == CblasColMajor
          ? traits::matrix_size2 (b)
          : traits::matrix_size1 (b); 
        assert (n == traits::matrix_size2 (a)); 
        assert (n == (stor_ord == CblasColMajor
                      ? traits::matrix_size1 (b)
                      : traits::matrix_size2 (b))); 

#ifndef BOOST_NUMERIC_BINDINGS_ATLAS_POTRF_BUG
        return potrs (stor_ord, uplo, n, nrhs, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                      traits::matrix_storage (a), 
#else
                      traits::matrix_storage_const (a), 
#endif 
                      traits::leading_dimension (a),
                      traits::matrix_storage (b),
                      traits::leading_dimension (b));
#else // BOOST_NUMERIC_BINDINGS_ATLAS_POTRF_BUG
        int ierr; 
        if (stor_ord == CblasColMajor)
          ierr = potrs (stor_ord, uplo, n, nrhs, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                        traits::matrix_storage (a), 
#else
                        traits::matrix_storage_const (a), 
#endif 
                        traits::leading_dimension (a),
                        traits::matrix_storage (b),
                        traits::leading_dimension (b));
        else // ATLAS bug with CblasRowMajor 
          ierr = potrs_bug (stor_ord, uplo, n, nrhs, 
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
                            traits::matrix_storage (a), 
#else
                            traits::matrix_storage_const (a), 
#endif 
                            traits::leading_dimension (a),
                            traits::matrix_storage (b),
                            traits::leading_dimension (b));
        return ierr; 
#endif // BOOST_NUMERIC_BINDINGS_ATLAS_POTRF_BUG
      }

    } // detail 

    template <typename SymmA, typename MatrB>
    inline
    int potrs (CBLAS_UPLO const uplo, SymmA const& a, MatrB& b) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmA>::matrix_structure, 
        traits::general_t
      >::value));
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
      >::value));
#endif
      assert (uplo == CblasUpper || uplo == CblasLower); 
      return detail::potrs (uplo, a, b); 
    }

    template <typename SymmA, typename MatrB>
    inline
    int potrs (SymmA const& a, MatrB& b) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      typedef typename traits::matrix_traits<SymmA>::value_type val_t;
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmA>::matrix_structure, 
        typename traits::detail::symm_herm_t<val_t>::type
      >::value)); 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrB>::matrix_structure, 
        traits::general_t
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
      
      return detail::potrs (uplo, a, b); 
    }

    template <typename SymmA, typename MatrB>
    inline 
    int cholesky_substitute (SymmA const& a, MatrB& b) { return potrs (a, b); }


#ifdef BOOST_NUMERIC_BINDINGS_ATLAS_POTRF_BUG
    // posv(): 'driver' function 
    template <typename SymmA, typename MatrB>
    inline
    int posv (CBLAS_UPLO const uplo, SymmA& a, MatrB& b) {
      int ierr = potrf (uplo, a); 
      if (ierr == 0)
        ierr = potrs (uplo, a, b);
      return ierr; 
    }

    template <typename SymmA, typename MatrB>
    inline
    int posv (SymmA& a, MatrB& b) {
      int ierr = potrf (a); 
      if (ierr == 0)
        ierr = potrs (a, b);
      return ierr; 
    }

    template <typename SymmA, typename MatrB>
    inline
    int cholesky_solve (SymmA& a, MatrB& b) {
      return posv (a, b); 
    }
#endif // BOOST_NUMERIC_BINDINGS_ATLAS_POTRF_BUG 


    // potri(): computes the inverse of a symmetric or Hermitian positive 
    //          definite matrix A using the Cholesky factorization 
    //          previously computed by potrf() 
    namespace detail {

      template <typename SymmA>
      inline
      int potri (CBLAS_UPLO const uplo, SymmA& a) {

        CBLAS_ORDER const stor_ord
          = enum_cast<CBLAS_ORDER const>
          (storage_order<
#ifndef BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
            typename traits::matrix_traits<SymmA>::ordering_type
#else
            typename SymmA::orientation_category 
#endif 
           >::value); 

        int const n = traits::matrix_size1 (a);
        assert (n == traits::matrix_size2 (a)); 

        return potri (stor_ord, uplo, n, 
                      traits::matrix_storage (a), 
                      traits::leading_dimension (a));
      }

    } // detail 

    template <typename SymmA>
    inline
    int potri (CBLAS_UPLO const uplo, SymmA& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmA>::matrix_structure, 
        traits::general_t
      >::value));
#endif
      assert (uplo == CblasUpper || uplo == CblasLower); 
      return detail::potri (uplo, a); 
    } 

    template <typename SymmA>
    inline
    int potri (SymmA& a) {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      typedef typename traits::matrix_traits<SymmA>::value_type val_t;
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<SymmA>::matrix_structure, 
        typename traits::detail::symm_herm_t<val_t>::type
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
      
      return detail::potri (uplo, a); 
    }

    template <typename SymmA>
    inline
    int cholesky_invert (SymmA& a) { return potri (a); }


  } // namespace atlas

}}} 

#endif // BOOST_NUMERIC_BINDINGS_CLAPACK_HPP
