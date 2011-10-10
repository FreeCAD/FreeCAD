//
// Copyright Fabien Dekeyser, Quoc-Cuong Pham 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_ORGQR_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_ORGQR_HPP

#include <complex>
#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/lapack/lapack.h>
#include <boost/numeric/bindings/lapack/workspace.hpp>
#include <boost/numeric/bindings/traits/detail/array.hpp>
// #include <boost/numeric/bindings/traits/std_vector.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
#  include <boost/static_assert.hpp>
#  include <boost/type_traits.hpp>
#endif 


namespace boost { namespace numeric { namespace bindings { 

  namespace lapack {

   
    ///////////////////////////////////////////////////////////////////
    /* 
     * Generates an M-by-N real matrix Q with orthonormal columns,
	 * which is defined as the first N columns of a product of K elementary
	 *	  reflectors of order M
	 *        Q  =  H(1) H(2) . . . H(k)
	 * as returned by geqrf().
	 * The init value of matrix Q is the matrix A returned by geqrf()
     */ 
	 ///////////////////////////////////////////////////////////////////
    namespace detail {

      /**
       *
       * Generates an M-by-N real matrix Q with orthonormal columns,
       * which is defined as the first N columns of a product of K elementary
       *	  reflectors of order M
       *        Q  =  H(1) H(2) . . . H(k)
       *
       * \warning 
       * \todo 
       * \date 2005
       * \author CEA/DRT/DTSI/SARC 
       * \author Q.C. PHAM
       *
       **/
      inline 
      void orgqr(int const m, int const n, int const k,
                 float* a, int const lda,
                 float* tau, float* work, int const lwork, int& info) 
      {
        LAPACK_SORGQR (&m, &n, &k, a, &lda, tau, work, &lwork, &info);
      }

      inline 
      void orgqr(int const m, int const n, int const k,
                 double* a, int const lda,
                 double* tau, double* work, int const lwork, int& info) 
      {
        LAPACK_DORGQR (&m, &n, &k, a, &lda, tau, work, &lwork, &info);
      }

      inline 
      void orgqr(int const m, int const n, int const k,
                 traits::complex_f* a, int const lda,
                 traits::complex_f* tau, traits::complex_f* work, int const lwork, int& info) 
      {
        LAPACK_CUNGQR (&m, &n, &k, traits::complex_ptr(a), &lda, traits::complex_ptr(tau),
                       traits::complex_ptr(work), &lwork, &info);
      }

      inline 
      void orgqr(int const m, int const n, int const k,
                 traits::complex_d* a, int const lda,
                 traits::complex_d* tau, traits::complex_d* work, int const lwork, int& info) 
      {
        LAPACK_ZUNGQR (&m, &n, &k, traits::complex_ptr(a), &lda, traits::complex_ptr(tau),
                       traits::complex_ptr(work), &lwork, &info);
      }

    } // fin namespace detail


	//--------------------------------------------

   template <typename A, typename Tau, typename Work>
   inline int orgqr(A& a, Tau& tau, Work &work) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
        typename traits::matrix_traits<A>::matrix_structure, 
        traits::general_t
      >::value)); 
#endif 

	  const int m = traits::matrix_size1 (a);
	  const int n = traits::matrix_size2 (a);
	  const int k = n;

	  assert (std::min<int>(m,n) <= traits::vector_size (tau)); 
      assert (n <= traits::vector_size (work)); 
	 
	  int info; 
      detail::orgqr (m, n, k,
                     traits::matrix_storage (a), 
                     traits::leading_dimension (a),
                     traits::vector_storage (tau),  
                     traits::vector_storage (work),
                     traits::vector_size (work),
                     info);
      return info;
    }

    // Computation of Q.
    // Workspace is allocated dynamically so that the optimization of
    // blas 3 calls is optimal.
    template <typename A, typename Tau>
    inline
    int orgqr (A& a, Tau& tau, optimal_workspace ) {
       typedef typename A::value_type value_type ;
	   const int n = traits::matrix_size2 (a);
       traits::detail::array<value_type> work(std::max<int>(1, n*32));
       return orgqr( a, tau, work );
	   
    }

    // Computation of Q
    // Workspace is allocated dynamically to its minimum size.
    // Blas 3 calls are not optimal.
    template <typename A, typename Tau>
    inline
    int orgqr (A& a, Tau& tau, minimal_workspace ) {
       typedef typename A::value_type value_type ;
	   const int n = traits::matrix_size2 (a);
       traits::detail::array<value_type> work(std::max<int>(1, n));
       return orgqr( a, tau, work );
    }

    // Computation of the Q
    // Workspace is taken from the array in workspace.
   
    template <typename A, typename Tau, typename Work>
    inline
    int orgqr (A& a, Tau& tau, detail::workspace1<Work> workspace ) {
       typedef typename A::value_type value_type ;
	   const int n = traits::matrix_size2 (a);
       traits::detail::array<value_type> work(std::max<int>(1, n));
       return orgqr( a, tau, workspace.w_ );
    }

    // Function without workarray as argument
    template <typename A, typename Tau>
    inline
    int orgqr (A& a, Tau& tau) {
       return orgqr( a, tau, optimal_workspace() );
    }

  }

}}}

#endif 
