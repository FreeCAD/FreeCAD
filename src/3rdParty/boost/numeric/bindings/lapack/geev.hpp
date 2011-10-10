/*
 * 
 * Copyright (c) Andreas Kloeckner 2004
 *               Toon Knapen, Karl Meerbergen & Kresimir Fresl 2003
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * KF acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_GEEV_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_GEEV_HPP

#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/traits/type.hpp>
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
    //
    // Eigendecomposition of a general matrix A * V = V * D
    // 
    ///////////////////////////////////////////////////////////////////

    /* 
     * geev() computes the eigendecomposition of a N x N matrix,
     * where V is a N x N matrix and D is a diagonal matrix. The 
     * diagonal element D(i,i) is an eigenvalue of A and Q(:,i) is 
     * a corresponding eigenvector.
     *
     *
     * int geev (char jobz, char uplo, A& a, W& w, V* vl, V* vr, optimal_workspace);
     *
     * a is the matrix whose eigendecomposition you're interested in. (input)
     *
     * w contains the diagonal of D, above. w must always be complex. (output)
     *
     * vl is an N x N matrix containing the left eigenvectors of a in its
     * columns. See remark on complex vs. real below. May be left NULL to indicate
     * that you do not want left eigenvectors.
     *
     * vr is an N x N matrix containing the right ("usual") eigenvectors of a in its
     * columns. See remark on complex vs. real below. As a matrix, vr fulfills
     * A * VR = VR * D. (except if real, see below). May be left NULL to indicate
     * that you do not want right eigenvectors.
     *
     *
     * For real A, vr and vl may be either complex or real, at your option.
     * If you choose to leave them real, you have to pick apart the complex-conjugate
     * eigenpairs as per the LAPACK documentation. If you choose them complex,
     * the code will do the picking-apart on your behalf, at the expense of 4*N
     * extra storage. Only if vr is complex, it will really fulfill its invariant 
     * on exit to the code in all cases, since complex pairs spoil that relation.
     */ 

    namespace detail {

      inline
      int geev_backend(const char* jobvl, const char* jobvr, const int* n, float* a,
	       const int* lda, float* wr, float* wi, float* vl, const int* ldvl,
	       float* vr, const int* ldvr, float* work, const int* lwork)
      {
	int info;
	LAPACK_SGEEV(jobvl, jobvr, n, a, lda, wr, wi, vl, ldvl, vr, ldvr, work, lwork, &info);
	return info;
      }

      inline
      int geev_backend(const char* jobvl, const char* jobvr, const int* n, double* a,
	       const int* lda, double* wr, double* wi, double* vl, const int* ldvl,
	       double* vr, const int* ldvr, double* work, const int* lwork)
      {
	int info;
	LAPACK_DGEEV(jobvl, jobvr, n, a, lda, wr, wi, vl, ldvl, vr, ldvr, work, lwork, &info);
	return info;
      }

      inline
      int geev_backend(const char* jobvl, const char* jobvr, const int* n, traits::complex_f* a,
	       const int* lda, traits::complex_f* w, traits::complex_f* vl, const int* ldvl,
	       traits::complex_f* vr, const int* ldvr, traits::complex_f* work, const int* lwork,
	       float* rwork)
      {
	int info;
	LAPACK_CGEEV(jobvl, jobvr, n, 
		     traits::complex_ptr(a), lda, 
		     traits::complex_ptr(w), 
		     traits::complex_ptr(vl), ldvl, 
		     traits::complex_ptr(vr), ldvr, 
		     traits::complex_ptr(work), lwork, 
		     rwork, &info);
	return info;
      }

      inline
      int geev_backend(const char* jobvl, const char* jobvr, const int* n, traits::complex_d* a,
	       const int* lda, traits::complex_d* w, traits::complex_d* vl, const int* ldvl,
	       traits::complex_d* vr, const int* ldvr, traits::complex_d* work, const int* lwork,
	       double* rwork)
      {
	int info;
	LAPACK_ZGEEV(jobvl, jobvr, n, 
		     traits::complex_ptr(a), lda, 
		     traits::complex_ptr(w), 
		     traits::complex_ptr(vl), ldvl, 
		     traits::complex_ptr(vr), ldvr, 
		     traits::complex_ptr(work), lwork, 
		     rwork, &info);
	return info;
      }


      struct real_case {};
      struct mixed_case {};
      struct complex_case {};




      // real case
      template <typename A, typename W, typename V>
      int geev(real_case, const char jobvl, const char jobvr, A& a, W& w, 
	       V* vl, V *vr)
      {
	int const n = traits::matrix_size1(a);
	typedef typename A::value_type value_type;
	traits::detail::array<value_type> wr(n);
	traits::detail::array<value_type> wi(n);

	traits::detail::array<value_type> vl2(vl ? 0 : n);
	traits::detail::array<value_type> vr2(vr ? 0 : n);
	value_type* vl_real = vl ? traits::matrix_storage(*vl) : vl2.storage();
	const int ldvl = vl ? traits::matrix_size2(*vl) : 1;
	value_type* vr_real = vr ? traits::matrix_storage(*vr) : vr2.storage();
	const int ldvr = vr ? traits::matrix_size2(*vr) : 1;


	// workspace query
	int lwork = -1;
	value_type work_temp;
	int result = geev_backend(&jobvl, &jobvr, &n,
				  traits::matrix_storage(a), &n, 
				  wr.storage(), wi.storage(), 
				  vl_real, &ldvl, vr_real, &ldvr,
				  &work_temp, &lwork);
	if (result != 0)
	  return result;

	lwork = (int) work_temp;
	traits::detail::array<value_type> work(lwork);
	result = geev_backend(&jobvl, &jobvr, &n,
			      traits::matrix_storage(a), &n, 
			      wr.storage(), wi.storage(), 
			      vl_real, &ldvl, vr_real, &ldvr,
			      work.storage(), &lwork);

	for (int i = 0; i < n; i++)
	  w[i] = std::complex<value_type>(wr[i], wi[i]);
	return result;
      }

      // mixed (i.e. real with complex vectors) case
      template <typename A, typename W, typename V>
      int geev(mixed_case, const char jobvl, const char jobvr, A& a, W& w, 
	       V* vl, V *vr)
      {
	int const n = traits::matrix_size1(a);
	typedef typename A::value_type value_type;
	traits::detail::array<value_type> wr(n);
	traits::detail::array<value_type> wi(n);

	traits::detail::array<value_type> vl2(vl ? n*n : n);
	traits::detail::array<value_type> vr2(vr ? n*n : n);
	const int ldvl2 = vl ? n : 1;
	const int ldvr2 = vr ? n : 1;

	// workspace query
	int lwork = -1;
	value_type work_temp;
	int result = geev_backend(&jobvl, &jobvr, &n,
				  traits::matrix_storage(a), &n, 
				  wr.storage(), wi.storage(), 
				  vl2.storage(), &ldvl2, vr2.storage(), &ldvr2,
				  &work_temp, &lwork);
	if (result != 0)
	  return result;

	lwork = (int) work_temp;
	traits::detail::array<value_type> work(lwork);
	result = geev_backend(&jobvl, &jobvr, &n,
			      traits::matrix_storage(a), &n, 
			      wr.storage(), wi.storage(), 
			      vl2.storage(), &ldvl2, vr2.storage(), &ldvr2,
			      work.storage(), &lwork);

	typedef typename V::value_type vec_value_type;
	vec_value_type* vl_stor = NULL;
	vec_value_type* vr_stor = NULL;
	int ldvl = 0, ldvr = 0;
	if (vl)
	{
	  vl_stor = traits::matrix_storage(*vl);
	  ldvl = traits::matrix_size2(*vl);
	}
	if (vr)
	{
	  vr_stor = traits::matrix_storage(*vr);
	  ldvr = traits::matrix_size2(*vr);
	}
	
	for (int i = 0; i < n; i++)
        {
	  w[i] = std::complex<value_type>(wr[i], wi[i]);
	  if (wi[i] != 0)
	  {
	    assert(i+1 < n);
	    assert(wr[i+1] == wr[i]);
	    assert(wi[i+1] == -wi[i]);

	    w[i+1] = std::complex<value_type>(wr[i+1], wi[i+1]);
	    for (int j = 0; j < n; j++)
	    {
	      if (vl)
	      {
		vl_stor[i*ldvl+j] = std::complex<value_type>(vl2[i*n+j], vl2[(i+1)*n+j]);
		vl_stor[(i+1)*ldvl+j] = std::complex<value_type>(vl2[i*n+j], -vl2[(i+1)*n+j]);
	      }
	      if (vr)
	      {
		vr_stor[i*ldvr+j] = std::complex<value_type>(vr2[i*n+j], vr2[(i+1)*n+j]);
		vr_stor[(i+1)*ldvr+j] = std::complex<value_type>(vr2[i*n+j], -vr2[(i+1)*n+j]);
	      }
	    }

	    i++;
	  }
	  else
	  {
	    for (int j = 0; j < n; j++)
	    {
	      if (vl)
		vl_stor[i*ldvl+j] = vl2[i*n+j];
	      if (vr)
		vr_stor[i*ldvr+j] = vr2[i*n+j];
	    }
	  }
	}
	return result;
      }

      // complex case
      template <typename A, typename W, typename V>
      int geev(complex_case, const char jobvl, const char jobvr, A& a, W& w, 
	       V* vl, V *vr)
      {
	typedef typename A::value_type value_type;
	typedef typename traits::type_traits<value_type>::real_type real_type;

	int const n = traits::matrix_size1(a);
	traits::detail::array<real_type> rwork(2*n);

	traits::detail::array<value_type> vl2(vl ? 0 : n);
	traits::detail::array<value_type> vr2(vr ? 0 : n);
	value_type* vl_real = vl ? traits::matrix_storage(*vl) : vl2.storage();
	const int ldvl = vl ? traits::matrix_size2(*vl) : 1;
	value_type* vr_real = vr ? traits::matrix_storage(*vr) : vr2.storage();
	const int ldvr = vr ? traits::matrix_size2(*vr) : 1;

	// workspace query
	int lwork = -1;
	value_type work_temp;
	int result = geev_backend(&jobvl, &jobvr, &n,
				  traits::matrix_storage(a), &n, 
				  traits::vector_storage(w),
				  vl_real, &ldvl, vr_real, &ldvr,
				  &work_temp, &lwork, rwork.storage());
	if (result != 0)
	  return result;

	lwork = (int) std::real(work_temp);
	traits::detail::array<value_type> work(lwork);
	result = geev_backend(&jobvl, &jobvr, &n,
			      traits::matrix_storage(a), &n, 
			      traits::vector_storage(w),
			      vl_real, &ldvl, vr_real, &ldvr,
			      work.storage(), &lwork, 
			      rwork.storage());

	return result;
      }

    } // namespace detail


    // gateway / dispatch routine
    template <typename A, typename W, typename V>
    int geev(A& a, W& w,  V* vl, V* vr, optimal_workspace) 
    {
      // input checking
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
			   typename traits::matrix_traits<A>::matrix_structure, 
			   traits::general_t
			   >::value)); 
#endif 

#ifndef NDEBUG
      int const n = traits::matrix_size1(a);
#endif

      assert(traits::matrix_size2(a)==n); 
      assert(traits::vector_size(w)==n); 
      assert(traits::vector_size(w)==n); 
      assert(!vr || traits::matrix_size1(*vr)==n); 
      assert(!vl || traits::matrix_size1(*vl)==n); 

      // preparation
      typedef typename A::value_type value_type;
      typedef typename V::value_type vec_value_type;
      typedef typename traits::type_traits<value_type>::real_type real_type;

      // dispatch
      return detail::geev(typename boost::mpl::if_<
			  boost::is_same<value_type, real_type>,
			  typename boost::mpl::if_<
			  boost::is_same<vec_value_type, real_type>,
			  detail::real_case,
			  detail::mixed_case>::type,
			  detail::complex_case>::type(),
			  vl != 0 ? 'V' : 'N', 
			  vr != 0 ? 'V' : 'N',
			  a, w, vl, vr);
    }

  }

}}}

#endif 
