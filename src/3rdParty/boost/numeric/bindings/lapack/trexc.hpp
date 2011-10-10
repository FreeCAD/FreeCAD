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

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_TREXC_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_TREXC_HPP

#include <complex>
#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/traits/type_traits.hpp>
#include <boost/numeric/bindings/lapack/lapack.h>
#include <boost/numeric/bindings/traits/detail/array.hpp>
// #include <boost/numeric/bindings/traits/std_vector.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
#  include <boost/static_assert.hpp>
#  include <boost/type_traits.hpp>
#endif 


namespace boost { namespace numeric { namespace bindings { 

  namespace lapack {

    ///////////////////////////////////////////////////////////////////
    //
    // Reorder the Schur factorization of a matrix.
    // 
    ///////////////////////////////////////////////////////////////////

    /* 
     * trexc()  reorders the Schur factorization of a matrix
     * A =  Q*T*Q**T, so that the diagonal block of T with row
     * index IFST is  moved to row ILST.
     */ 

    namespace detail {
      inline 
      int trexc_work_size( int const n, float ) {return n;}

      inline 
      int trexc_work_size( int const n, double ) {return n;}

      inline 
      int trexc_work_size( int const n, traits::complex_f ) {return 0;}

      inline 
      int trexc_work_size( int const n, traits::complex_d ) {return 0;}
    }

    // Get the minimum size of the work array.
    template <typename MatrT>
    int trexc_work_size(const MatrT& t) {
       return detail::trexc_work_size( traits::matrix_size1(t), typename MatrT::value_type() );
    }

    namespace detail {
      inline 
      void trexc (char const compq, int const n,
                 float* t, int const ldt, float* q, int const ldq, int& ifst, int& ilst,
		 float* work, int& info) 
      {
        LAPACK_STREXC (&compq, &n, t, &ldt, q, &ldq, &ifst, &ilst, work, &info);
      }

      inline 
      void trexc (char const compq, int const n,
                 double* t, int const ldt, double* q, int const ldq, int& ifst, int& ilst,
		 double* work, int& info) 
      {
        LAPACK_DTREXC (&compq, &n, t, &ldt, q, &ldq, &ifst, &ilst, work, &info);
      }

      inline 
      void trexc (char const compq, int const n,
                 traits::complex_f* t, int const ldt, traits::complex_f* q, int const ldq, int& ifst, int& ilst,
		 float* work, int& info) 
      {
        LAPACK_CTREXC (&compq, &n, traits::complex_ptr(t), &ldt, traits::complex_ptr(q), &ldq, &ifst, &ilst, &info);
      }

      inline 
      void trexc (char const compq, int const n,
                 traits::complex_d* t, int const ldt, traits::complex_d* q, int const ldq, int& ifst, int& ilst,
		 double* work, int& info) 
      {
        LAPACK_ZTREXC (&compq, &n, traits::complex_ptr(t), &ldt, traits::complex_ptr(q), &ldq, &ifst, &ilst, &info);
      }

    } 

    // Reorder Schur factorization with Schur vectors
    template <typename MatrT, typename Q, typename Work>
    inline
    int trexc (char const compq, MatrT& t, Q& q, int& ifst, int& ilst, Work& work) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<MatrT>::matrix_structure, 
        traits::general_t
      >::value)); 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<Q>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

      int const n = traits::matrix_size1 (t);
      assert (n == traits::matrix_size2 (t)); 
      assert (n == traits::matrix_size1 (q)); 
      assert (n == traits::matrix_size2 (q)); 
      assert (trexc_work_size(t) <= traits::vector_size (work)); 

      int info; 
      detail::trexc (compq, n,
                    traits::matrix_storage (t), 
                    traits::leading_dimension (t),
                    traits::matrix_storage (q),
                    traits::leading_dimension (q),
		    ifst, ilst,
                    traits::vector_storage (work),
                    info);
      return info; 
    }

  }

}}}

#endif 
