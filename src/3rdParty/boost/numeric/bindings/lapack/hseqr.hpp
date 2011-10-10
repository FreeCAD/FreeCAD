/*
 * 
 * Copyright Jeremy Conlin 2008
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_HSEQR_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_HSEQR_HPP

#include <iostream>
#include <complex>
#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/traits/matrix_traits.hpp>
#include <boost/numeric/bindings/traits/type_traits.hpp>
#include <boost/numeric/bindings/lapack/lapack.h>
#include <boost/numeric/bindings/lapack/workspace.hpp>
#include <boost/numeric/bindings/traits/detail/array.hpp>

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
#  include <boost/static_assert.hpp>
#  include <boost/type_traits.hpp>
#endif 


namespace boost { namespace numeric { namespace bindings { 

  namespace lapack {

    ///////////////////////////////////////////////////////////////////
    //
    // Compute eigenvalues of an Hessenberg matrix, H.
    // 
    ///////////////////////////////////////////////////////////////////

    /* 
     * hseqr() computes the eigenvalues of a Hessenberg matrix H
     * and, optionally, the matrices T and Z from the Schur decomposition
     * H = Z U Z**T, where U is an upper quasi-triangular matrix (the
     * Schur form), and Z is the orthogonal matrix of Schur vectors.
     *
     * Optionally Z may be postmultiplied into an input orthogonal
     * matrix Q so that this routine can give the Schur factorization
     * of a matrix A which has been reduced to the Hessenberg form H
     * by the orthogonal matrix Q:  A = Q*H*Q**T = (QZ)*U*(QZ)**T.
     * 
     * There are two forms of the hseqr function:
     *
     * int hseqr( const char job, A& H, W& w)
     * int hseqr( const char job, const char compz, A& H, W& w, Z& z)
     *
     * The first form does not compute Schur vectors and is equivelant to
     * setting compz = 'N' in the second form.  hseqr returns a '0' if the
     * computation is successful.
     *
     * job.
     *   = 'E': compute eigenvalues only
     *   = 'S': compute eigenvalues and the Schur form U
     *
     * compz. (input)
     *   = 'N':  no Schur vectors are computed;  Equivalent to using the
     *           first form of the hseqr function.
     *   = 'I':  Z is initialized to the unit matrix and the matrix Z
     *           of Schur vectors of H is returned;
     *   = 'V':  Z must contain an orthogonal matrix Q on entry, and
     *           the product Q*Z is returned.
     * 
     * H is the Hessenberg matrix whose eigenpairs you're interested 
     * in. (input/output) On exit, if computation is successful and 
     * job = 'S', then H contains the
     * upper quasi-triangular matrix U from the Schur decomposition
     * (the Schur form); 2-by-2 diagonal blocks (corresponding to
     * complex conjugate pairs of eigenvalues) are returned in
     * standard form, with H(i,i) = H(i+1,i+1) and
     * H(i+1,i)*H(i,i+1) < 0. If computation is successful and 
     * job = 'E', the contents of H are unspecified on exit.  
     *
     * w (output) contains the computed eigenvalues of H which is the diagonal 
     * of U. Must be a complex object.
     *
     * Z. (input/output)
     * If compz = 'N', Z is not referenced.
     * If compz = 'I', on entry Z need not be set and on exit,
     * if computation is successful, Z contains the orthogonal matrix Z of the Schur
     * vectors of H.  If compz = 'V', on entry Z must contain an
     * N-by-N matrix Q, which is assumed to be equal to the unit
     * matrix . On exit, if computation is successful, Z contains Q*Z.
     *
     */ 

    namespace detail {
        // float
        inline
        int hseqr_backend(const char* job, const char* compz, const int* n, 
                const int ilo, const int ihi, float* H, const int ldH, 
                float* wr, float* wi, float* Z, const int ldz, float* work,
                const int* lwork){
            int info;
//          std::cout << "I'm inside lapack::detail::hseqr_backend for floats" 
//              << std::endl;
            LAPACK_SHSEQR(job, compz, n, &ilo, &ihi, H, &ldH, wr, wi, 
                    Z, &ldz, work, lwork, &info);
            return info;
        }

        // double
        inline
        int hseqr_backend(const char* job, const char* compz, const int* n, 
                const int ilo, const int ihi, double* H, const int ldH, 
                double* wr, double* wi, double* Z, const int ldz, double* work,
                const int* lwork){
            int info;
//          std::cout << "I'm inside lapack::detail::hseqr_backend for doubles" 
//              << std::endl;
            LAPACK_DHSEQR(job, compz, n, &ilo, &ihi, H, &ldH, wr, wi, 
                    Z, &ldz, work, lwork, &info);
            return info;
        }

        // complex<float>
        inline
        int hseqr_backend(const char* job, const char* compz, int* n, 
                const int ilo, const int ihi, traits::complex_f* H, const int ldH, 
                traits::complex_f* w, traits::complex_f* Z, int ldz, 
                traits::complex_f* work, const int* lwork){
            int info;
//          std::cout << "I'm inside lapack::detail::hseqr_backend for complex<float>" 
//              << std::endl;
            LAPACK_CHSEQR(job, compz, n, &ilo, &ihi, 
                    traits::complex_ptr(H), &ldH, 
                    traits::complex_ptr(w), 
                    traits::complex_ptr(Z), &ldz, 
                    traits::complex_ptr(work), lwork, &info);
            return info;
        }

        // complex<double>
        inline
        int hseqr_backend(const char* job, const char* compz, int* n, 
                const int ilo, const int ihi, traits::complex_d* H, const int ldH, 
                traits::complex_d* w, traits::complex_d* Z, int ldz, 
                traits::complex_d* work, const int* lwork){
            int info;
//          std::cout << "I'm inside lapack::detail::hseqr_backend for complex<double>" 
//              << std::endl;
            LAPACK_ZHSEQR(job, compz, n, &ilo, &ihi, 
                    traits::complex_ptr(H), &ldH, 
                    traits::complex_ptr(w), 
                    traits::complex_ptr(Z), &ldz, 
                    traits::complex_ptr(work), lwork, &info);
            return info;
        }

        template <int N>
        struct Hseqr{};

        template <>
        struct Hseqr< 1 >{
            template < typename A, typename W, typename V>
            int operator() ( const char job, const char compz, A& H, W& w, V& Z ){
//              std::cout << "Inside Hseqr<1>." << std::endl;

                int n = traits::matrix_size1(H);
                typedef typename A::value_type value_type;
                traits::detail::array<value_type> wr(n);
                traits::detail::array<value_type> wi(n);

                // workspace query
                int lwork = -1;
                value_type work_temp;
                int result = detail::hseqr_backend(&job, &compz, &n, 1, n,
                                            traits::matrix_storage(H), 
                                            traits::leading_dimension(H),
                                            wr.storage(), wi.storage(),
                                            traits::matrix_storage(Z),
                                            traits::leading_dimension(Z),
                                            &work_temp, &lwork);

                if( result !=0 ) return result;

                lwork = (int) work_temp;
                traits::detail::array<value_type> work(lwork);
                result = detail::hseqr_backend(&job, &compz, &n, 1, n,
                                            traits::matrix_storage(H), 
                                            traits::leading_dimension(H),
                                            wr.storage(), wi.storage(),
                                            traits::matrix_storage(Z),
                                            traits::leading_dimension(Z),
                                            work.storage(), &lwork);

                for (int i = 0; i < n; i++)
                    w[i] = std::complex<value_type>(wr[i], wi[i]);

                return result;
            }
        };

        template <>
        struct Hseqr< 2 >{
            template < typename A, typename W, typename V>
            int operator() ( const char job, const char compz, A& H, W& w, V& Z ){
//              std::cout << "Inside Hseqr<2>." << std::endl;

                int n = traits::matrix_size1(H);
                typedef typename A::value_type value_type;

                // workspace query
                int lwork = -1;
                value_type work_temp;
                int result = detail::hseqr_backend(&job, &compz, &n, 1, n,
                        traits::matrix_storage(H),
                        traits::leading_dimension(H), 
                        traits::vector_storage(w),
                        traits::matrix_storage(Z), traits::leading_dimension(Z),
                        &work_temp, &lwork);

                if( result !=0 ) return result;

                lwork = (int) std::real(work_temp);
                traits::detail::array<value_type> work(lwork);
                result = detail::hseqr_backend(&job, &compz, &n, 1, n,
                        traits::matrix_storage(H),
                        traits::leading_dimension(H), 
                        traits::vector_storage(w),
                        traits::matrix_storage(Z), traits::leading_dimension(Z),
                        work.storage(), &lwork);

                return result;
            }
        };
        
        template < typename A, typename W, typename V>
        int hseqr( const char job, const char compz, A& H, W& w, V& Z ){
//          std::cout << "I'm inside lapack::detail::hseqr." << std::endl;

            assert ( job == 'E' || job == 'S' );
            assert ( compz == 'N' || compz == 'I' || compz == 'V' );

            typedef typename A::value_type value_type;

            int result = detail::Hseqr< n_workspace_args<value_type>::value >()(
                    job, compz, H, w, Z);

            return result;
        }
    }   // namespace detail 

    // Compute eigenvalues without the Schur vectors
    template < typename A, typename W>
    int hseqr( const char job, A& H, W& w){
      // input checking
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
			   typename traits::matrix_traits<A>::matrix_structure, 
			   traits::general_t
			   >::value)); 
#endif 

#ifndef NDEBUG
        int const n = traits::matrix_size1(H);
#endif

        typedef typename A::value_type value_type;
        typedef typename W::value_type complex_value_type;

        assert(traits::matrix_size2(H) == n); // Square matrix
        assert(traits::vector_size(w) == n);  

        ublas::matrix<value_type, ublas::column_major> Z(1,1);
        return detail::hseqr( job, 'N', H, w, Z );
    }

    // Compute eigenvalues and the Schur vectors
    template < typename A, typename W, typename Z>
    int hseqr( const char job, const char compz, A& H, W& w, Z& z){
      // input checking
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
      BOOST_STATIC_ASSERT((boost::is_same<
			   typename traits::matrix_traits<A>::matrix_structure, 
			   traits::general_t
			   >::value)); 
#endif 

#ifndef NDEBUG
        int const n = traits::matrix_size1(H);
#endif

        typedef typename A::value_type value_type;
        assert(traits::matrix_size2(H) == n); // Square matrix
        assert(traits::vector_size(w) == n);  
        assert(traits::matrix_size2(z) == n);

        return detail::hseqr( job, compz, H, w, z );
    }

  }
}}}

#endif
