/*
 * 
 * Copyright (c) Kresimir Fresl 2003 
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Author acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

/* for UMFPACK Copyright, License and Availability see umfpack_inc.hpp */ 


#ifndef BOOST_NUMERIC_BINDINGS_UMFPACK_HPP
#define BOOST_NUMERIC_BINDINGS_UMFPACK_HPP


#include <boost/noncopyable.hpp> 
#include <boost/numeric/bindings/traits/traits.hpp>
#include <boost/numeric/bindings/traits/sparse_traits.hpp>
#include <boost/numeric/bindings/umfpack/umfpack_overloads.hpp>


namespace boost { namespace numeric { namespace bindings {  namespace umfpack {


  template <typename T = double>
  struct symbolic_type : private noncopyable { 
    void *ptr; 
    ~symbolic_type() { 
      if (ptr)
        detail::free_symbolic (T(), 0, &ptr); 
    }
    void free() {
      if (ptr)
        detail::free_symbolic (T(), 0, &ptr); 
      ptr = 0; 
    }
  }; 

  template <typename T>
  void free (symbolic_type<T>& s) { s.free(); }

  template <typename T = double>
  struct numeric_type : private noncopyable { 
    void *ptr; 
    ~numeric_type() { 
      if (ptr)
        detail::free_numeric (T(), 0, &ptr); 
    }
    void free() { 
      if (ptr)
        detail::free_numeric (T(), 0, &ptr); 
      ptr = 0; 
    }
  }; 

  template <typename T>
  void free (numeric_type<T>& n) { n.free(); }


  template <typename T = double>
  struct control_type : private noncopyable {
    double ptr[UMFPACK_CONTROL]; 
    control_type() { detail::defaults (T(), 0, ptr); }
    double operator[] (int i) const { return ptr[i]; }
    double& operator[] (int i) { return ptr[i]; }
    void defaults() { detail::defaults (T(), 0, ptr); }
  }; 

  template <typename T>
  void defaults (control_type<T>& c) { c.defaults(); } 

  template <typename T = double>
  struct info_type : private noncopyable {
    double ptr[UMFPACK_INFO]; 
    double operator[] (int i) const { return ptr[i]; }
    double& operator[] (int i) { return ptr[i]; }
  }; 


  /////////////////////////////////////
  // solving system of linear equations
  /////////////////////////////////////


  // symbolic 
  /* 
   * Given nonzero pattern of a sparse matrix A in column-oriented form,
   * umfpack_*_symbolic performs a column pre-ordering to reduce fill-in
   * (using COLAMD or AMD) and a symbolic factorisation.  This is required
   * before the matrix can be numerically factorised with umfpack_*_numeric.
   */
  namespace detail {

    template <typename MatrA>
    inline
    int symbolic (traits::compressed_t, 
                  MatrA const& A, void **Symbolic, 
                  double const* Control = 0, double* Info = 0) 
    {
      return detail::symbolic (traits::spmatrix_size1 (A),
                               traits::spmatrix_size2 (A),
                               traits::spmatrix_index1_storage (A),
                               traits::spmatrix_index2_storage (A),
                               traits::spmatrix_value_storage (A),
                               Symbolic, Control, Info); 
    }

    template <typename MatrA, typename QVec>
    inline
    int symbolic (traits::compressed_t, 
                  MatrA const& A, QVec const& Qinit, void **Symbolic, 
                  double const* Control = 0, double* Info = 0) 
    {
      return detail::qsymbolic (traits::spmatrix_size1 (A),
                                traits::spmatrix_size2 (A),
                                traits::spmatrix_index1_storage (A),
                                traits::spmatrix_index2_storage (A),
                                traits::spmatrix_value_storage (A),
                                traits::vector_storage (Qinit), 
                                Symbolic, Control, Info); 
    }

    template <typename MatrA>
    inline
    int symbolic (traits::coordinate_t, 
                  MatrA const& A, void **Symbolic, 
                  double const* Control = 0, double* Info = 0) 
    {
      int n_row = traits::spmatrix_size1 (A); 
      int n_col = traits::spmatrix_size2 (A); 
      int nnz = traits::spmatrix_num_nonzeros (A); 

      typedef typename traits::sparse_matrix_traits<MatrA>::value_type val_t; 

      int const* Ti = traits::spmatrix_index2_storage (A);
      int const* Tj = traits::spmatrix_index1_storage (A); 
      traits::detail::array<int> Ap (n_col+1); 
      if (!Ap.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<int> Ai (nnz); 
      if (!Ai.valid()) return UMFPACK_ERROR_out_of_memory;

      int status = detail::triplet_to_col (n_row, n_col, nnz, 
                                           Ti, Tj, static_cast<val_t*> (0),
                                           Ap.storage(), Ai.storage(), 
                                           static_cast<val_t*> (0), 0); 
      if (status != UMFPACK_OK) return status; 

      return detail::symbolic (n_row, n_col, 
                               Ap.storage(), Ai.storage(),
                               traits::spmatrix_value_storage (A),
                               Symbolic, Control, Info); 
    }

    template <typename MatrA, typename QVec>
    inline
    int symbolic (traits::coordinate_t, 
                  MatrA const& A, QVec const& Qinit, void **Symbolic, 
                  double const* Control = 0, double* Info = 0) 
    {
      int n_row = traits::spmatrix_size1 (A); 
      int n_col = traits::spmatrix_size2 (A); 
      int nnz = traits::spmatrix_num_nonzeros (A); 

      typedef typename traits::sparse_matrix_traits<MatrA>::value_type val_t; 

      int const* Ti = traits::spmatrix_index2_storage (A);
      int const* Tj = traits::spmatrix_index1_storage (A); 
      traits::detail::array<int> Ap (n_col+1); 
      if (!Ap.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<int> Ai (nnz); 
      if (!Ai.valid()) return UMFPACK_ERROR_out_of_memory;

      int status = detail::triplet_to_col (n_row, n_col, nnz, 
                                           Ti, Tj, static_cast<val_t*> (0),
                                           Ap.storage(), Ai.storage(), 
                                           static_cast<val_t*> (0), 0); 
      if (status != UMFPACK_OK) return status; 

      return detail::qsymbolic (n_row, n_col, 
                                Ap.storage(), Ai.storage(),
                                traits::spmatrix_value_storage (A),
                                traits::vector_storage (Qinit), 
                                Symbolic, Control, Info); 
    }

  } // detail 
 
  template <typename MatrA>
  inline
  int symbolic (MatrA const& A, 
                symbolic_type<
                  typename traits::sparse_matrix_traits<MatrA>::value_type
                >& Symbolic, 
                double const* Control = 0, double* Info = 0) 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
    BOOST_STATIC_ASSERT((boost::is_same<
      typename traits::sparse_matrix_traits<MatrA>::matrix_structure, 
      traits::general_t
    >::value)); 
    BOOST_STATIC_ASSERT((boost::is_same<
      typename traits::sparse_matrix_traits<MatrA>::ordering_type,
      traits::column_major_t
    >::value)); 
    BOOST_STATIC_ASSERT(traits::sparse_matrix_traits<MatrA>::index_base == 0);
#endif 

    typedef 
      typename traits::sparse_matrix_traits<MatrA>::storage_format storage_f; 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
    BOOST_STATIC_ASSERT(
      (boost::is_same<storage_f, traits::compressed_t>::value
       || 
       boost::is_same<storage_f, traits::coordinate_t>::value
       )); 
#endif 

    return detail::symbolic (storage_f(), A, &Symbolic.ptr, Control, Info); 
  }

  template <typename MatrA>
  inline
  int symbolic (MatrA const& A, 
                symbolic_type<
                  typename traits::sparse_matrix_traits<MatrA>::value_type
                >& Symbolic, 
                control_type<
                  typename traits::sparse_matrix_traits<MatrA>::value_type
                > const& Control, 
                info_type<
                  typename traits::sparse_matrix_traits<MatrA>::value_type
                >& Info) 
  {
    return symbolic (A, Symbolic, Control.ptr, Info.ptr); 
  }

  template <typename MatrA>
  inline
  int symbolic (MatrA const& A, 
                symbolic_type<
                  typename traits::sparse_matrix_traits<MatrA>::value_type
                >& Symbolic, 
                control_type<
                  typename traits::sparse_matrix_traits<MatrA>::value_type
                > const& Control)
  {
    return symbolic (A, Symbolic, Control.ptr); 
  }

  template <typename MatrA, typename QVec>
  inline
  int symbolic (MatrA const& A, QVec const& Qinit, 
                symbolic_type<
                  typename traits::sparse_matrix_traits<MatrA>::value_type
                >& Symbolic, 
                double const* Control = 0, double* Info = 0) 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
    BOOST_STATIC_ASSERT((boost::is_same<
      typename traits::sparse_matrix_traits<MatrA>::matrix_structure, 
      traits::general_t
    >::value)); 
    BOOST_STATIC_ASSERT((boost::is_same<
      typename traits::sparse_matrix_traits<MatrA>::ordering_type,
      traits::column_major_t
    >::value)); 
    BOOST_STATIC_ASSERT(traits::sparse_matrix_traits<MatrA>::index_base == 0);
#endif 

    typedef 
      typename traits::sparse_matrix_traits<MatrA>::storage_format storage_f; 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
    BOOST_STATIC_ASSERT(
      (boost::is_same<storage_f, traits::compressed_t>::value
       || 
       boost::is_same<storage_f, traits::coordinate_t>::value
       )); 
#endif 

    assert (traits::spmatrix_size2 (A) == traits::vector_size (Qinit)); 

    return detail::symbolic (storage_f(), A, Qinit, 
                             &Symbolic.ptr, Control, Info); 
  }

  template <typename MatrA, typename QVec>
  inline
  int symbolic (MatrA const& A, QVec const& Qinit, 
                symbolic_type<
                  typename traits::sparse_matrix_traits<MatrA>::value_type
                >& Symbolic, 
                control_type<
                  typename traits::sparse_matrix_traits<MatrA>::value_type
                > const& Control, 
                info_type<
                  typename traits::sparse_matrix_traits<MatrA>::value_type
                >& Info) 
  {
    return symbolic (A, Qinit, Symbolic, Control.ptr, Info.ptr); 
  }

  template <typename MatrA, typename QVec>
  inline
  int symbolic (MatrA const& A, QVec const& Qinit,  
                symbolic_type<
                  typename traits::sparse_matrix_traits<MatrA>::value_type
                >& Symbolic, 
                control_type<
                  typename traits::sparse_matrix_traits<MatrA>::value_type
                > const& Control)
  {
    return symbolic (A, Qinit, Symbolic, Control.ptr); 
  }


  // numeric 
  /*
   * Given a sparse matrix A in column-oriented form, and a symbolic analysis
   * computed by umfpack_*_*symbolic, the umfpack_*_numeric routine performs 
   * the numerical factorisation, PAQ=LU, PRAQ=LU, or P(R\A)Q=LU, where P 
   * and Q are permutation matrices (represented as permutation vectors), 
   * R is the row scaling, L is unit-lower triangular, and U is upper 
   * triangular.  This is required before the system Ax=b (or other related 
   * linear systems) can be solved.  
   */
  namespace detail {

    template <typename MatrA>
    inline
    int numeric (traits::compressed_t, MatrA const& A, 
                 void *Symbolic, void** Numeric, 
                 double const* Control = 0, double* Info = 0) 
    {
      return detail::numeric (traits::spmatrix_size1 (A),
                              traits::spmatrix_size2 (A),
                              traits::spmatrix_index1_storage (A),
                              traits::spmatrix_index2_storage (A),
                              traits::spmatrix_value_storage (A),
                              Symbolic, Numeric, Control, Info); 
    }

    template <typename MatrA>
    inline
    int numeric (traits::coordinate_t, MatrA const& A, 
                 void *Symbolic, void** Numeric, 
                 double const* Control = 0, double* Info = 0) 
    {
      int n_row = traits::spmatrix_size1 (A); 
      int n_col = traits::spmatrix_size2 (A); 
      int nnz = traits::spmatrix_num_nonzeros (A); 

      typedef typename traits::sparse_matrix_traits<MatrA>::value_type val_t; 

      int const* Ti = traits::spmatrix_index2_storage (A);
      int const* Tj = traits::spmatrix_index1_storage (A); 
      traits::detail::array<int> Ap (n_col+1); 
      if (!Ap.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<int> Ai (nnz); 
      if (!Ai.valid()) return UMFPACK_ERROR_out_of_memory;

      int status = detail::triplet_to_col (n_row, n_col, nnz, 
                                           Ti, Tj, static_cast<val_t*> (0),
                                           Ap.storage(), Ai.storage(), 
                                           static_cast<val_t*> (0), 0); 
      if (status != UMFPACK_OK) return status; 

      return detail::numeric (n_row, n_col, 
                              Ap.storage(), Ai.storage(),
                              traits::spmatrix_value_storage (A),
                              Symbolic, Numeric, Control, Info); 
    }

  } // detail 

  template <typename MatrA>
  inline
  int numeric (MatrA const& A, 
               symbolic_type<
                 typename traits::sparse_matrix_traits<MatrA>::value_type
               > const& Symbolic, 
               numeric_type<
                 typename traits::sparse_matrix_traits<MatrA>::value_type
               >& Numeric, 
               double const* Control = 0, double* Info = 0) 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
    BOOST_STATIC_ASSERT((boost::is_same<
      typename traits::sparse_matrix_traits<MatrA>::matrix_structure, 
      traits::general_t
    >::value)); 
    BOOST_STATIC_ASSERT((boost::is_same<
      typename traits::sparse_matrix_traits<MatrA>::ordering_type,
      traits::column_major_t
    >::value)); 
    BOOST_STATIC_ASSERT(traits::sparse_matrix_traits<MatrA>::index_base == 0);
#endif 

    typedef 
      typename traits::sparse_matrix_traits<MatrA>::storage_format storage_f; 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
    BOOST_STATIC_ASSERT(
      (boost::is_same<storage_f, traits::compressed_t>::value
       || 
       boost::is_same<storage_f, traits::coordinate_t>::value
       )); 
#endif 

    return detail::numeric (storage_f(), A, 
                            Symbolic.ptr, &Numeric.ptr, Control, Info); 
  }

  template <typename MatrA>
  inline
  int numeric (MatrA const& A, 
               symbolic_type<
                 typename traits::sparse_matrix_traits<MatrA>::value_type
               > const& Symbolic, 
               numeric_type<
                 typename traits::sparse_matrix_traits<MatrA>::value_type
               >& Numeric, 
               control_type<
                 typename traits::sparse_matrix_traits<MatrA>::value_type
               > const& Control, 
               info_type<
                 typename traits::sparse_matrix_traits<MatrA>::value_type
               >& Info) 

  {
    // g++ (3.2) is unable to distinguish 
    //           function numeric() and namespace boost::numeric ;o) 
    return umfpack::numeric (A, Symbolic, Numeric, Control.ptr, Info.ptr); 
  }
    
  template <typename MatrA>
  inline
  int numeric (MatrA const& A, 
               symbolic_type<
                 typename traits::sparse_matrix_traits<MatrA>::value_type
               > const& Symbolic, 
               numeric_type<
                 typename traits::sparse_matrix_traits<MatrA>::value_type
               >& Numeric, 
               control_type<
                 typename traits::sparse_matrix_traits<MatrA>::value_type
               > const& Control)
  {
    return umfpack::numeric (A, Symbolic, Numeric, Control.ptr); 
  }
    

  // factor 
  /* 
   * symbolic and numeric
   */
  namespace detail {

    template <typename MatrA>
    inline
    int factor (traits::compressed_t, MatrA const& A, 
                void** Numeric, double const* Control = 0, double* Info = 0) 
    {
      symbolic_type<typename traits::sparse_matrix_traits<MatrA>::value_type>
        Symbolic; 

      int status;
      status = detail::symbolic (traits::spmatrix_size1 (A),
                                 traits::spmatrix_size2 (A),
                                 traits::spmatrix_index1_storage (A),
                                 traits::spmatrix_index2_storage (A),
                                 traits::spmatrix_value_storage (A),
                                 &Symbolic.ptr, Control, Info); 
      if (status != UMFPACK_OK) return status; 

      return detail::numeric (traits::spmatrix_size1 (A),
                              traits::spmatrix_size2 (A),
                              traits::spmatrix_index1_storage (A),
                              traits::spmatrix_index2_storage (A),
                              traits::spmatrix_value_storage (A),
                              Symbolic.ptr, Numeric, Control, Info); 
    }

    template <typename MatrA>
    inline
    int factor (traits::coordinate_t, MatrA const& A, 
                void** Numeric, double const* Control = 0, double* Info = 0) 
    {
      int n_row = traits::spmatrix_size1 (A); 
      int n_col = traits::spmatrix_size2 (A); 
      int nnz = traits::spmatrix_num_nonzeros (A); 

      typedef typename traits::sparse_matrix_traits<MatrA>::value_type val_t; 

      int const* Ti = traits::spmatrix_index2_storage (A);
      int const* Tj = traits::spmatrix_index1_storage (A); 
      traits::detail::array<int> Ap (n_col+1); 
      if (!Ap.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<int> Ai (nnz); 
      if (!Ai.valid()) return UMFPACK_ERROR_out_of_memory;

      int status = detail::triplet_to_col (n_row, n_col, nnz, 
                                           Ti, Tj, static_cast<val_t*> (0),
                                           Ap.storage(), Ai.storage(), 
                                           static_cast<val_t*> (0), 0); 
      if (status != UMFPACK_OK) return status; 

      symbolic_type<typename traits::sparse_matrix_traits<MatrA>::value_type>
        Symbolic; 

      status = detail::symbolic (n_row, n_col, 
                                 Ap.storage(), Ai.storage(),
                                 traits::spmatrix_value_storage (A),
                                 &Symbolic.ptr, Control, Info); 
      if (status != UMFPACK_OK) return status; 

      return detail::numeric (n_row, n_col, 
                              Ap.storage(), Ai.storage(),
                              traits::spmatrix_value_storage (A),
                              Symbolic.ptr, Numeric, Control, Info); 
    }

  } // detail 

  template <typename MatrA>
  inline
  int factor (MatrA const& A, 
              numeric_type<
                typename traits::sparse_matrix_traits<MatrA>::value_type
              >& Numeric, 
              double const* Control = 0, double* Info = 0) 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
    BOOST_STATIC_ASSERT((boost::is_same<
      typename traits::sparse_matrix_traits<MatrA>::matrix_structure, 
      traits::general_t
    >::value)); 
    BOOST_STATIC_ASSERT((boost::is_same<
      typename traits::sparse_matrix_traits<MatrA>::ordering_type,
      traits::column_major_t
    >::value)); 
    BOOST_STATIC_ASSERT(traits::sparse_matrix_traits<MatrA>::index_base == 0);
#endif 

    typedef 
      typename traits::sparse_matrix_traits<MatrA>::storage_format storage_f; 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
    BOOST_STATIC_ASSERT(
      (boost::is_same<storage_f, traits::compressed_t>::value
       || 
       boost::is_same<storage_f, traits::coordinate_t>::value
       )); 
#endif 

    return detail::factor (storage_f(), A, &Numeric.ptr, Control, Info); 
  }

  template <typename MatrA>
  inline
  int factor (MatrA const& A, 
              numeric_type<
                typename traits::sparse_matrix_traits<MatrA>::value_type
              >& Numeric, 
              control_type<
                typename traits::sparse_matrix_traits<MatrA>::value_type
              > const& Control, 
              info_type<
                typename traits::sparse_matrix_traits<MatrA>::value_type
              >& Info) 
  {
    return factor (A, Numeric, Control.ptr, Info.ptr); 
  }
    
  template <typename MatrA>
  inline
  int factor (MatrA const& A, 
              numeric_type<
                typename traits::sparse_matrix_traits<MatrA>::value_type
              >& Numeric, 
              control_type<
                typename traits::sparse_matrix_traits<MatrA>::value_type
              > const& Control)
  {
    return factor (A, Numeric, Control.ptr); 
  }
    
  
  // solve
  /*
   * Given LU factors computed by umfpack_*_numeric and the right-hand-side, 
   * B, solve a linear system for the solution X.  Iterative refinement is 
   * optionally performed.  Only square systems are handled. 
   */
  namespace detail {

    template <typename MatrA, typename VecX, typename VecB> 
    inline 
    int solve (traits::compressed_t, int sys, 
               MatrA const& A, VecX& X, VecB const& B, 
               void *Numeric, double const* Control = 0, double* Info = 0) 
    {
      return detail::solve (sys, traits::spmatrix_size1 (A),
                            traits::spmatrix_index1_storage (A),
                            traits::spmatrix_index2_storage (A),
                            traits::spmatrix_value_storage (A),
                            traits::vector_storage (X),
                            traits::vector_storage (B),
                            Numeric, Control, Info); 
    }

    template <typename MatrA, typename VecX, typename VecB> 
    inline 
    int solve (traits::coordinate_t, int sys, 
               MatrA const& A, VecX& X, VecB const& B, 
               void *Numeric, double const* Control = 0, double* Info = 0) 
    {

      int n = traits::spmatrix_size1 (A); 
      int nnz = traits::spmatrix_num_nonzeros (A); 

      typedef typename traits::sparse_matrix_traits<MatrA>::value_type val_t; 

      int const* Ti = traits::spmatrix_index2_storage (A);
      int const* Tj = traits::spmatrix_index1_storage (A); 
      traits::detail::array<int> Ap (n+1); 
      if (!Ap.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<int> Ai (nnz); 
      if (!Ai.valid()) return UMFPACK_ERROR_out_of_memory;

      int status = detail::triplet_to_col (n, n, nnz, 
                                           Ti, Tj, static_cast<val_t*> (0),
                                           Ap.storage(), Ai.storage(), 
                                           static_cast<val_t*> (0), 0); 
      if (status != UMFPACK_OK) return status; 
 
      return detail::solve (sys, n, Ap.storage(), Ai.storage(),
                            traits::spmatrix_value_storage (A),
                            traits::vector_storage (X),
                            traits::vector_storage (B),
                            Numeric, Control, Info); 
    }

  } // detail 

  template <typename MatrA, typename VecX, typename VecB> 
  inline 
  int solve (int sys, MatrA const& A, VecX& X, VecB const& B, 
             numeric_type<
               typename traits::sparse_matrix_traits<MatrA>::value_type
             > const& Numeric, 
             double const* Control = 0, double* Info = 0) 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
    BOOST_STATIC_ASSERT((boost::is_same<
      typename traits::sparse_matrix_traits<MatrA>::matrix_structure, 
      traits::general_t
    >::value)); 
    BOOST_STATIC_ASSERT((boost::is_same<
      typename traits::sparse_matrix_traits<MatrA>::ordering_type,
      traits::column_major_t
    >::value)); 
    BOOST_STATIC_ASSERT(traits::sparse_matrix_traits<MatrA>::index_base == 0);
#endif 

    typedef 
      typename traits::sparse_matrix_traits<MatrA>::storage_format storage_f; 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
    BOOST_STATIC_ASSERT(
      (boost::is_same<storage_f, traits::compressed_t>::value
       || 
       boost::is_same<storage_f, traits::coordinate_t>::value
       )); 
#endif 

    assert (traits::spmatrix_size1 (A) == traits::spmatrix_size1 (A)); 
    assert (traits::spmatrix_size2 (A) == traits::vector_size (X)); 
    assert (traits::spmatrix_size2 (A) == traits::vector_size (B)); 

    return detail::solve (storage_f(), sys, A, X, B, 
                          Numeric.ptr, Control, Info); 
  }

  template <typename MatrA, typename VecX, typename VecB> 
  inline 
  int solve (int sys, MatrA const& A, VecX& X, VecB const& B, 
             numeric_type<
               typename traits::sparse_matrix_traits<MatrA>::value_type
             > const& Numeric, 
             control_type<
               typename traits::sparse_matrix_traits<MatrA>::value_type
             > const& Control, 
             info_type<
               typename traits::sparse_matrix_traits<MatrA>::value_type
             >& Info) 
  {
    return solve (sys, A, X, B, Numeric, Control.ptr, Info.ptr); 
  }

  template <typename MatrA, typename VecX, typename VecB> 
  inline 
  int solve (int sys, MatrA const& A, VecX& X, VecB const& B, 
             numeric_type<
               typename traits::sparse_matrix_traits<MatrA>::value_type
             > const& Numeric, 
             control_type<
               typename traits::sparse_matrix_traits<MatrA>::value_type
             > const& Control)
  {
    return solve (sys, A, X, B, Numeric, Control.ptr); 
  }

  template <typename MatrA, typename VecX, typename VecB> 
  inline 
  int solve (MatrA const& A, VecX& X, VecB const& B, 
             numeric_type<
               typename traits::sparse_matrix_traits<MatrA>::value_type
             > const& Numeric, 
             double const* Control = 0, double* Info = 0) 
  {
    return solve (UMFPACK_A, A, X, B, Numeric, Control, Info); 
  }

  template <typename MatrA, typename VecX, typename VecB> 
  inline 
  int solve (MatrA const& A, VecX& X, VecB const& B, 
             numeric_type<
               typename traits::sparse_matrix_traits<MatrA>::value_type
             > const& Numeric, 
             control_type<
               typename traits::sparse_matrix_traits<MatrA>::value_type
             > const& Control, 
             info_type<
               typename traits::sparse_matrix_traits<MatrA>::value_type
             >& Info) 
  {
    return solve (UMFPACK_A, A, X, B, Numeric, 
                  Control.ptr, Info.ptr); 
  }

  template <typename MatrA, typename VecX, typename VecB> 
  inline 
  int solve (MatrA const& A, VecX& X, VecB const& B, 
             numeric_type<
               typename traits::sparse_matrix_traits<MatrA>::value_type
             > const& Numeric, 
             control_type<
               typename traits::sparse_matrix_traits<MatrA>::value_type
             > const& Control)
  {
    return solve (UMFPACK_A, A, X, B, Numeric, Control.ptr); 
  }


  // umf_solve 
  /* 
   * symbolic, numeric and solve 
   */
  namespace detail {

    template <typename MatrA, typename VecX, typename VecB>
    inline
    int umf_solve (traits::compressed_t, 
                   MatrA const& A, VecX& X, VecB const& B, 
                   double const* Control = 0, double* Info = 0) 
    {
      symbolic_type<typename traits::sparse_matrix_traits<MatrA>::value_type>
        Symbolic; 
      numeric_type<typename traits::sparse_matrix_traits<MatrA>::value_type>
        Numeric; 

      int status;
      status = detail::symbolic (traits::spmatrix_size1 (A),
                                 traits::spmatrix_size2 (A),
                                 traits::spmatrix_index1_storage (A),
                                 traits::spmatrix_index2_storage (A),
                                 traits::spmatrix_value_storage (A),
                                 &Symbolic.ptr, Control, Info); 
      if (status != UMFPACK_OK) return status; 

      status = detail::numeric (traits::spmatrix_size1 (A),
                                traits::spmatrix_size2 (A),
                                traits::spmatrix_index1_storage (A),
                                traits::spmatrix_index2_storage (A),
                                traits::spmatrix_value_storage (A),
                                Symbolic.ptr, &Numeric.ptr, Control, Info); 
      if (status != UMFPACK_OK) return status; 

      return detail::solve (UMFPACK_A, traits::spmatrix_size1 (A),
                            traits::spmatrix_index1_storage (A),
                            traits::spmatrix_index2_storage (A),
                            traits::spmatrix_value_storage (A),
                            traits::vector_storage (X),
                            traits::vector_storage (B),
                            Numeric.ptr, Control, Info); 
    }

    template <typename MatrA, typename VecX, typename VecB>
    inline
    int umf_solve (traits::coordinate_t, 
                   MatrA const& A, VecX& X, VecB const& B, 
                   double const* Control = 0, double* Info = 0) 
    {
      int n_row = traits::spmatrix_size1 (A); 
      int n_col = traits::spmatrix_size2 (A); 
      int nnz = traits::spmatrix_num_nonzeros (A); 

      typedef typename traits::sparse_matrix_traits<MatrA>::value_type val_t; 

      int const* Ti = traits::spmatrix_index2_storage (A);
      int const* Tj = traits::spmatrix_index1_storage (A); 
      traits::detail::array<int> Ap (n_col+1); 
      if (!Ap.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<int> Ai (nnz); 
      if (!Ai.valid()) return UMFPACK_ERROR_out_of_memory;

      int status = detail::triplet_to_col (n_row, n_col, nnz, 
                                           Ti, Tj, static_cast<val_t*> (0),
                                           Ap.storage(), Ai.storage(), 
                                           static_cast<val_t*> (0), 0); 
      if (status != UMFPACK_OK) return status; 

      symbolic_type<typename traits::sparse_matrix_traits<MatrA>::value_type>
        Symbolic; 
      numeric_type<typename traits::sparse_matrix_traits<MatrA>::value_type>
        Numeric; 

      status = detail::symbolic (n_row, n_col, 
                                 Ap.storage(), Ai.storage(),
                                 traits::spmatrix_value_storage (A),
                                 &Symbolic.ptr, Control, Info); 
      if (status != UMFPACK_OK) return status; 

      status = detail::numeric (n_row, n_col, 
                                Ap.storage(), Ai.storage(),
                                traits::spmatrix_value_storage (A),
                                Symbolic.ptr, &Numeric.ptr, Control, Info); 
      if (status != UMFPACK_OK) return status; 

      return detail::solve (UMFPACK_A, n_row, Ap.storage(), Ai.storage(),
                            traits::spmatrix_value_storage (A),
                            traits::vector_storage (X),
                            traits::vector_storage (B),
                            Numeric.ptr, Control, Info); 
    }

  } // detail 

  template <typename MatrA, typename VecX, typename VecB>
  inline
  int umf_solve (MatrA const& A, VecX& X, VecB const& B, 
                 double const* Control = 0, double* Info = 0) 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
    BOOST_STATIC_ASSERT((boost::is_same<
      typename traits::sparse_matrix_traits<MatrA>::matrix_structure, 
      traits::general_t
    >::value)); 
    BOOST_STATIC_ASSERT((boost::is_same<
      typename traits::sparse_matrix_traits<MatrA>::ordering_type,
      traits::column_major_t
    >::value)); 
    BOOST_STATIC_ASSERT(traits::sparse_matrix_traits<MatrA>::index_base == 0);
#endif 

    typedef 
      typename traits::sparse_matrix_traits<MatrA>::storage_format storage_f; 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
    BOOST_STATIC_ASSERT(
      (boost::is_same<storage_f, traits::compressed_t>::value
       || 
       boost::is_same<storage_f, traits::coordinate_t>::value
       )); 
#endif 

    assert (traits::spmatrix_size1 (A) == traits::spmatrix_size1 (A)); 
    assert (traits::spmatrix_size2 (A) == traits::vector_size (X)); 
    assert (traits::spmatrix_size2 (A) == traits::vector_size (B)); 

    return detail::umf_solve (storage_f(), A, X, B, Control, Info); 
  }

  template <typename MatrA, typename VecX, typename VecB>
  inline
  int umf_solve (MatrA const& A, VecX& X, VecB const& B, 
                 control_type<
                   typename traits::sparse_matrix_traits<MatrA>::value_type
                 > const& Control, 
                 info_type<
                   typename traits::sparse_matrix_traits<MatrA>::value_type
                 >& Info) 
  {
    return umf_solve (A, X, B, Control.ptr, Info.ptr); 
  }    

  template <typename MatrA, typename VecX, typename VecB>
  inline
  int umf_solve (MatrA const& A, VecX& X, VecB const& B, 
                 control_type<
                   typename traits::sparse_matrix_traits<MatrA>::value_type
                 > const& Control)
  {
    return umf_solve (A, X, B, Control.ptr); 
  }    


  ///////////////////////
  // matrix manipulations
  ///////////////////////


  // scale 
  
  template <typename VecX, typename VecB> 
  inline 
  int scale (VecX& X, VecB const& B, 
             numeric_type<
               typename traits::vector_traits<VecB>::value_type
             > const& Numeric) 
  {
    return detail::scale (traits::vector_size (B),
                          traits::vector_storage (X),
                          traits::vector_storage (B),
                          Numeric.ptr);
  }


  ////////////
  // reporting
  ////////////


  // report status

  template <typename T>
  inline
  void report_status (control_type<T> const& Control, int status) {
    detail::report_status (T(), 0, Control.ptr, status); 
  }

#if 0
  template <typename T>
  inline
  void report_status (int printing_level, int status) {
    control_type<T> Control; 
    Control[UMFPACK_PRL] = printing_level; 
    detail::report_status (T(), 0, Control.ptr, status); 
  }
  template <typename T>
  inline
  void report_status (int status) {
    control_type<T> Control; 
    detail::report_status (T(), 0, Control.ptr, status); 
  }
#endif 
  

  // report control

  template <typename T>
  inline
  void report_control (control_type<T> const& Control) {
    detail::report_control (T(), 0, Control.ptr); 
  }
  

  // report info 

  template <typename T>
  inline
  void report_info (control_type<T> const& Control, info_type<T> const& Info) {
    detail::report_info (T(), 0, Control.ptr, Info.ptr); 
  }

#if 0
  template <typename T>
  inline
  void report_info (int printing_level, info_type<T> const& Info) {
    control_type<T> Control; 
    Control[UMFPACK_PRL] = printing_level; 
    detail::report_info (T(), 0, Control.ptr, Info.ptr); 
  }
  template <typename T>
  inline
  void report_info (info_type<T> const& Info) {
    control_type<T> Control; 
    detail::report_info (T(), 0, Control.ptr, Info.ptr); 
  }
#endif 


  // report matrix (compressed column and coordinate) 

  namespace detail {

    template <typename MatrA>
    inline
    int report_matrix (traits::compressed_t, MatrA const& A, 
                       double const* Control)
    {
      return detail::report_matrix (traits::spmatrix_size1 (A),
                                    traits::spmatrix_size2 (A),
                                    traits::spmatrix_index1_storage (A),
                                    traits::spmatrix_index2_storage (A),
                                    traits::spmatrix_value_storage (A),
                                    1, Control); 
    }
    
    template <typename MatrA>
    inline
    int report_matrix (traits::coordinate_t, MatrA const& A, 
                       double const* Control)
    {
      return detail::report_triplet (traits::spmatrix_size1 (A),
                                     traits::spmatrix_size2 (A),
                                     traits::spmatrix_num_nonzeros (A), 
                                     traits::spmatrix_index1_storage (A),
                                     traits::spmatrix_index2_storage (A),
                                     traits::spmatrix_value_storage (A),
                                     Control); 
    }
    
  } // detail 

  template <typename MatrA>
  inline
  int report_matrix (MatrA const& A, 
                     control_type<
                       typename traits::sparse_matrix_traits<MatrA>::value_type
                     > const& Control) 
  {
#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
    BOOST_STATIC_ASSERT((boost::is_same<
      typename traits::sparse_matrix_traits<MatrA>::matrix_structure, 
      traits::general_t
    >::value)); 
    BOOST_STATIC_ASSERT((boost::is_same<
      typename traits::sparse_matrix_traits<MatrA>::ordering_type,
      traits::column_major_t
    >::value)); 
    BOOST_STATIC_ASSERT(traits::sparse_matrix_traits<MatrA>::index_base == 0);
#endif 

    typedef 
      typename traits::sparse_matrix_traits<MatrA>::storage_format storage_f; 

#ifndef BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 
    BOOST_STATIC_ASSERT(
      (boost::is_same<storage_f, traits::compressed_t>::value
       || 
       boost::is_same<storage_f, traits::coordinate_t>::value
       )); 
#endif 

    return detail::report_matrix (storage_f(), A, Control.ptr); 
  }


  // report vector 

  template <typename VecX>
  inline
  int report_vector (VecX const& X, 
                     control_type<
                       typename traits::vector_traits<VecX>::value_type
                     > const& Control) 
  {
    return detail::report_vector (traits::vector_size (X), 
                                  traits::vector_storage (X), 
                                  Control.ptr);
  }


  // report numeric 

  template <typename T> 
  inline
  int report_numeric (numeric_type<T> const& Numeric, 
                      control_type<T> const& Control)
  {
    return detail::report_numeric (T(), 0, Numeric.ptr, Control.ptr); 
  }


  // report symbolic 

  template <typename T> 
  inline
  int report_symbolic (symbolic_type<T> const& Symbolic, 
                       control_type<T> const& Control)
  {
    return detail::report_symbolic (T(), 0, Symbolic.ptr, Control.ptr); 
  }


  // report permutation vector 

  template <typename VecP, typename T> 
  inline
  int report_permutation (VecP const& Perm, control_type<T> const& Control) {
    return detail::report_perm (T(), 0, 
                                traits::vector_storage (Perm),
                                Control.ptr); 
  }


}}}} 

#endif // BOOST_NUMERIC_BINDINGS_UMFPACK_HPP
