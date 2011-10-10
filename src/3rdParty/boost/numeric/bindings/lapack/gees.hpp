/*
 * 
 * Copyright (c) Toon Knapen, Karl Meerbergen & Kresimir Fresl 2003
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * KF acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_GEES_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_GEES_HPP

#include <boost/numeric/bindings/traits/type.hpp>
#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/traits/type_traits.hpp>
#include <boost/numeric/bindings/lapack/lapack.h>
#include <boost/numeric/bindings/lapack/workspace.hpp>
#include <boost/numeric/bindings/traits/detail/array.hpp>
#include <boost/numeric/bindings/traits/detail/utils.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
#  include <boost/static_assert.hpp>
#  include <boost/type_traits.hpp>
#endif 


namespace boost { namespace numeric { namespace bindings { 

  namespace lapack {

    ///////////////////////////////////////////////////////////////////
    //
    // Schur factorization of general matrix.
    // 
    ///////////////////////////////////////////////////////////////////

    /* 
     * gees() computes a Schur factorization of an N-by-N matrix A.
     *
     * The Schur decomposition is A = U S * herm(U)  where  U  is a
     * unitary matrix and S is upper triangular. The eigenvalues of A
     * are on the main diagonal of S. If A is real, S is in pseudo
     * upper triangular form.
     *
     * Workspace is organized following the arguments in the calling sequence.
     *  optimal_workspace() : for optimizing use of blas 3 kernels
     *  minimal_workspace() : minimum size of workarrays, but does not allow for optimization
     *                        of blas 3 kernels
     *  workspace( work ) for real matrices where work is a real array with
     *                    vector_size( work ) >= 3*matrix_size1( a )
     *  workspace( work, rwork ) for complex matrices where work is a complex
     *                           array with vector_size( work ) >= 2*matrix_size1( a )
     *                           and rwork is a real array with
     *                           vector_size( rwork ) >= matrix_size1( a ).
     */ 

    namespace detail {
      inline 
      void gees (char const jobvs, char const sort, logical_t* select, int const n,
                 float* a, int const lda, int& sdim, traits::complex_f* w,
                 float* vs, int const ldvs, float* work, int const lwork,
                 bool* bwork, int& info) 
      {
        traits::detail::array<float> wr(n);
        traits::detail::array<float> wi(n);
        LAPACK_SGEES (&jobvs, &sort, select, &n, a, &lda, &sdim,
                      traits::vector_storage(wr), traits::vector_storage(wi),
                      vs, &ldvs, work, &lwork, bwork, &info);
        traits::detail::interlace(traits::vector_storage(wr),
                                  traits::vector_storage(wr)+n,
                                  traits::vector_storage(wi),
                                  w);
      }


      inline 
      void gees (char const jobvs, char const sort, logical_t* select, int const n,
                 double* a, int const lda, int& sdim, traits::complex_d* w,
                 double* vs, int const ldvs, double* work, int const lwork,
                 bool* bwork, int& info) 
      {
        traits::detail::array<double> wr(n);
        traits::detail::array<double> wi(n);
        LAPACK_DGEES (&jobvs, &sort, select, &n, a, &lda, &sdim,
                      traits::vector_storage(wr), traits::vector_storage(wi),
                      vs, &ldvs, work, &lwork, bwork, &info);
        traits::detail::interlace(traits::vector_storage(wr),
                                  traits::vector_storage(wr)+n,
                                  traits::vector_storage(wi),
                                  w);
      }


      inline 
      void gees (char const jobvs, char const sort, logical_t* select, int const n,
                 traits::complex_f* a, int const lda, int& sdim, traits::complex_f* w,
                 traits::complex_f* vs, int const ldvs,
                 traits::complex_f* work, int lwork, float* rwork, bool* bwork,
                 int& info) 
      {
        LAPACK_CGEES (&jobvs, &sort, select, &n, traits::complex_ptr(a), &lda, &sdim,
                      traits::complex_ptr(w), traits::complex_ptr (vs), &ldvs,
                      traits::complex_ptr(work), &lwork, rwork, bwork, &info);
      }


      inline 
      void gees (char const jobvs, char const sort, logical_t* select, int const n,
                 traits::complex_d* a, int const lda, int& sdim, traits::complex_d* w,
                 traits::complex_d* vs, int const ldvs,
                 traits::complex_d* work, int lwork, double* rwork, bool* bwork,
                 int& info) 
      {
        LAPACK_ZGEES (&jobvs, &sort, select, &n, traits::complex_ptr(a), &lda, &sdim,
                      traits::complex_ptr(w), traits::complex_ptr(vs), &ldvs,
                      traits::complex_ptr(work), &lwork, rwork, bwork, &info);
      }

    } 


    namespace detail {
       /// Compute Schur factorization, passing one work array.
       template <typename MatrA, typename SchVec, typename EigVal, typename Work>
       inline
       int gees (char jobvs, MatrA& a, EigVal& w, SchVec& vs, Work& work) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
         BOOST_STATIC_ASSERT((boost::is_same<
           typename traits::matrix_traits<MatrA>::matrix_structure, 
           traits::general_t
         >::value)); 
         BOOST_STATIC_ASSERT((boost::is_same<
           typename traits::matrix_traits<SchVec>::matrix_structure, 
           traits::general_t
         >::value)); 
#endif 

         typedef typename MatrA::value_type                            value_type ;

         int const n = traits::matrix_size1 (a);
         assert (n == traits::matrix_size2 (a)); 
         assert (n == traits::matrix_size1 (vs)); 
         assert (n == traits::matrix_size2 (vs)); 
         assert (n == traits::vector_size (w)); 
         assert (3*n <= traits::vector_size (work)); 

         logical_t* select=0;
         bool* bwork=0;

         int info, sdim; 
         detail::gees (jobvs, 'N', select, n,
                       traits::matrix_storage (a), 
                       traits::leading_dimension (a),
                       sdim,
                       traits::vector_storage (w),
                       traits::matrix_storage (vs),
                       traits::leading_dimension (vs),
		       traits::vector_storage( work ),
		       traits::vector_size( work ),
                       bwork, info);
	 return info ;
       } // gees()


       /// Compute Schur factorization, passing two work arrays.
       template <typename MatrA, typename SchVec, typename EigVal,
                 typename Work, typename RWork>
       inline
       int gees (char jobvs, MatrA& a, EigVal& w, SchVec& vs,
		 Work& work, RWork& rwork) {

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
         BOOST_STATIC_ASSERT((boost::is_same<
           typename traits::matrix_traits<MatrA>::matrix_structure, 
           traits::general_t
         >::value)); 
         BOOST_STATIC_ASSERT((boost::is_same<
           typename traits::matrix_traits<SchVec>::matrix_structure, 
           traits::general_t
         >::value)); 
#endif 

         typedef typename MatrA::value_type                            value_type ;

         int const n = traits::matrix_size1 (a);
         assert (n == traits::matrix_size2 (a)); 
         assert (n == traits::matrix_size1 (vs)); 
         assert (n == traits::matrix_size2 (vs)); 
         assert (n == traits::vector_size (w)); 
         assert (2*n <= traits::vector_size (work)); 
         assert (n <= traits::vector_size (rwork)); 

         logical_t* select=0;
         bool* bwork=0;

         int info, sdim; 
         detail::gees (jobvs, 'N', select, n,
                       traits::matrix_storage (a), 
                       traits::leading_dimension (a),
                       sdim,
                       traits::vector_storage (w),
                       traits::matrix_storage (vs),
                       traits::leading_dimension (vs),
		       traits::vector_storage( work ),
		       traits::vector_size( work ),
		       traits::vector_storage( rwork ),
                       bwork, info);
	 return info ;
       } // gees()


       /// Compute Schur factorization, depending on whether we have one or
       /// two workspace arrays. N= the number of workspace arrays.
       template <int N>
       struct Gees {};


       template <>
       struct Gees< 2 > {
          template <typename MatrA, typename SchVec, typename EigVal>
          inline
          int operator() (char jobvs, MatrA& a, EigVal& w, SchVec& vs, optimal_workspace ) const {
             typedef typename MatrA::value_type                            value_type ;
             typedef typename traits::type_traits< value_type >::real_type real_type ;

             int n = traits::matrix_size1( a );

             traits::detail::array<value_type> work( 2*n );
             traits::detail::array<real_type>  rwork( n );

             return gees( jobvs, a, w, vs, work, rwork );
          } // gees()

          template <typename MatrA, typename SchVec, typename EigVal>
          inline
          int operator() (char jobvs, MatrA& a, EigVal& w, SchVec& vs, minimal_workspace ) const {
             typedef typename MatrA::value_type                            value_type ;
             typedef typename traits::type_traits< value_type >::real_type real_type ;

             int n = traits::matrix_size1( a );

             traits::detail::array<value_type> work( 2*n );
             traits::detail::array<real_type>  rwork( n );

             return gees( jobvs, a, w, vs, work, rwork );
          } // gees()

          /// Compute Schur factorization, passing workspace2 as workspace
          template <typename MatrA, typename SchVec, typename EigVal, typename RWork, typename Work>
          inline
          int operator() (char jobvs, MatrA& a, EigVal& w, SchVec& vs, workspace2<Work,RWork>& workspace ) const {
             return gees( jobvs, a, w, vs, workspace.w_, workspace.wr_ );
          } // gees()
       }; // Gees<2>


       template <>
       struct Gees< 1 > {
          template <typename MatrA, typename SchVec, typename EigVal>
          inline
          int operator() (char jobvs, MatrA& a, EigVal& w, SchVec& vs, optimal_workspace ) const {
             typedef typename MatrA::value_type                            value_type ;
             typedef typename traits::type_traits< value_type >::real_type real_type ;

             int n = traits::matrix_size1( a );

             traits::detail::array<value_type> work( 3*n );

             return gees( jobvs, a, w, vs, work );
          } // gees()

          template <typename MatrA, typename SchVec, typename EigVal>
          inline
          int operator() (char jobvs, MatrA& a, EigVal& w, SchVec& vs, minimal_workspace ) const {
             typedef typename MatrA::value_type                            value_type ;
             typedef typename traits::type_traits< value_type >::real_type real_type ;

             int n = traits::matrix_size1( a );

             traits::detail::array<value_type> work( 3*n );

             return gees( jobvs, a, w, vs, work );
          } // gees()

          /// Compute Schur factorization, passing workspace1 as workspace
          template <typename MatrA, typename SchVec, typename EigVal, typename Work>
          inline
          int operator() (char jobvs, MatrA& a, EigVal& w, SchVec& vs, detail::workspace1<Work> workspace ) const {
             return gees( jobvs, a, w, vs, workspace.w_ );
          } // gees()
       }; // Gees<1>

    } // namespace detail


    /// Compute Schur factorization with Schur vectors.
    ///
    /// Workspace can be the following :
    /// optimal_workspace() : for optimizing use of blas 3 kernels
    /// minimal_workspace() : minimum size of workarrays, but does not allow for optimization
    ///                       of blas 3 kernels
    /// workspace( real_work ) for real matrices where
    ///                       vector_size( real_work ) >= 3*matrix_size1( a )
    /// workspace( complex_work, real_work ) for complex matrices where
    ///                       vector_size( complex_work ) >= 2*matrix_size1( a )
    ///                       and vector_size( real_work ) >= matrix_size1( a ).
    template <typename MatrA, typename SchVec, typename EigVal, typename Workspace>
    inline
    int gees (MatrA& a, EigVal& e, SchVec& vs, Workspace workspace ) {
       return detail::Gees< n_workspace_args<typename MatrA::value_type>::value>()
               ( 'V', a, e, vs, workspace );
    } // gees()


    // Compute Schur factorization without Schur vectors.
    ///
    /// Workspace can be the following :
    /// optimal_workspace() : for optimizing use of blas 3 kernels
    /// minimal_workspace() : minimum size of workarrays, but does not allow for optimization
    ///                       of blas 3 kernels
    /// workspace( real_work ) for real matrices where
    ///                       vector_size( real_work ) >= 3*matrix_size1( a )
    /// workspace( complex_work, real_work ) for complex matrices where
    ///                       vector_size( complex_work ) >= 2*matrix_size1( a )
    ///                       and vector_size( real_work ) >= matrix_size1( a ).
    template <typename MatrA, typename EigVal, typename Workspace>
    inline
    int gees (MatrA& a, EigVal& e, Workspace workspace) {
      return detail::Gees< n_workspace_args<typename MatrA::value_type>::value>()
              ('N', a, e, a, workspace );
    }

  }

}}}

#endif 
