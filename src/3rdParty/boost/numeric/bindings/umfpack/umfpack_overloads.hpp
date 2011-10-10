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


#ifndef BOOST_NUMERIC_BINDINGS_UMFPACK_OVERLOADS_HPP
#define BOOST_NUMERIC_BINDINGS_UMFPACK_OVERLOADS_HPP

#include <boost/numeric/bindings/umfpack/umfpack_inc.hpp>
#include <boost/numeric/bindings/traits/type.hpp>
#include <boost/numeric/bindings/traits/detail/array.hpp>
#include <boost/numeric/bindings/traits/detail/utils.hpp>

namespace boost { namespace numeric { namespace bindings { 

  namespace umfpack { namespace detail {

    ////////////////////////////
    // UMFPACK primary routines:
    ////////////////////////////

    // symbolic 

    inline
    int symbolic (int n_row, int n_col, 
                  int const* Ap, int const* Ai, double const* Ax, 
                  void **Symbolic, double const* Control, double* Info) 
    {
      return umfpack_di_symbolic (n_row, n_col, Ap, Ai, Ax, 
                                  Symbolic, Control, Info); 
    }

    inline
    int symbolic (int n_row, int n_col,
                  int const* Ap, int const* Ai, traits::complex_d const* Ax, 
                  void **Symbolic, double const* Control, double* Info)
    {
      int nnz = Ap[n_col];
      traits::detail::array<double> Axr (nnz); 
      if (!Axr.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<double> Axi (nnz); 
      if (!Axi.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::disentangle (Ax, Ax+nnz, 
                                   Axr.storage(), Axi.storage()); 
      return umfpack_zi_symbolic (n_row, n_col, Ap, Ai, 
                                  Axr.storage(), Axi.storage(), 
                                  Symbolic, Control, Info); 
    }


    // numeric 

    inline
    int numeric (int, int, 
                 int const* Ap, int const* Ai, double const* Ax, 
                 void *Symbolic, void **Numeric, 
                 double const* Control, double* Info) 
    {
      return umfpack_di_numeric (Ap, Ai, Ax, Symbolic, Numeric, Control, Info);
    }

    inline
    int numeric (int, int n_col, 
                 int const* Ap, int const* Ai, traits::complex_d const* Ax, 
                 void *Symbolic, void **Numeric,
                 double const* Control, double* Info)
    {
      int nnz = Ap[n_col];
      traits::detail::array<double> Axr (nnz); 
      if (!Axr.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<double> Axi (nnz); 
      if (!Axi.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::disentangle (Ax, Ax+nnz, 
                                   Axr.storage(), Axi.storage()); 
      return umfpack_zi_numeric (Ap, Ai, Axr.storage(), Axi.storage(), 
                                 Symbolic, Numeric, Control, Info); 
    }


    // solve 

    inline
    int solve (int sys, int, 
               int const* Ap, int const* Ai, double const* Ax,
               double* X, double const* B, void *Numeric,
               double const* Control, double* Info)
    {
      return umfpack_di_solve (sys, Ap, Ai, Ax, X, B, Numeric, Control, Info);
    }

    inline
    int solve (int sys, int n, 
               int const* Ap, int const* Ai, traits::complex_d const* Ax, 
               traits::complex_d* X, traits::complex_d const* B, 
               void *Numeric, double const* Control, double* Info)
    {
      int nnz = Ap[n];
      traits::detail::array<double> Axr (nnz); 
      if (!Axr.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<double> Axi (nnz); 
      if (!Axi.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::disentangle (Ax, Ax+nnz, 
                                   Axr.storage(), Axi.storage()); 
      traits::detail::array<double> Br (n); 
      if (!Br.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<double> Bi (n); 
      if (!Bi.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::disentangle (B, B+n, 
                                   Br.storage(), Bi.storage()); 
      traits::detail::array<double> Xr (n); 
      if (!Xr.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<double> Xi (n); 
      if (!Xi.valid()) return UMFPACK_ERROR_out_of_memory;

      int status = umfpack_zi_solve (sys, Ap, Ai, 
                                     Axr.storage(), Axi.storage(), 
                                     Xr.storage(), Xi.storage(), 
                                     Br.storage(), Bi.storage(), 
                                     Numeric, Control, Info);
      if (status != UMFPACK_OK) return status; 
      traits::detail::interlace (Xr.storage(), Xr.storage() + n,
                                 Xi.storage(), X); 
      return status; 
    }


    // free_symbolic 

    inline
    void free_symbolic (double, int, void **Symbolic) {
      umfpack_di_free_symbolic (Symbolic); 
    }
    inline
    void free_symbolic (traits::complex_d const&, int, void **Symbolic) {
      umfpack_zi_free_symbolic (Symbolic); 
    }


    // free_numeric

    inline
    void free_numeric (double, int, void **Numeric) {
      umfpack_di_free_numeric (Numeric); 
    }
    inline
    void free_numeric (traits::complex_d const&, int, void **Numeric) {
      umfpack_zi_free_numeric (Numeric); 
    }


    ////////////////////////////////
    // UMFPACK alternative routines:
    ////////////////////////////////

    // default control 

    inline
    void defaults (double, int, double* Control) {
      umfpack_di_defaults (Control); 
    }
    inline
    void defaults (traits::complex_d const&, int, double* Control) {
      umfpack_zi_defaults (Control); 
    }


    // symbolic with specified column preordering

    inline
    int qsymbolic (int n_row, int n_col, 
                   int const* Ap, int const* Ai, double const* Ax, 
                   int const* Qinit, 
                   void **Symbolic, double const* Control, double* Info) 
    {
      return umfpack_di_qsymbolic (n_row, n_col, Ap, Ai, Ax, Qinit, 
                                  Symbolic, Control, Info); 
    }

    inline
    int qsymbolic (int n_row, int n_col,
                   int const* Ap, int const* Ai, traits::complex_d const* Ax, 
                   int const* Qinit, 
                   void **Symbolic, double const* Control, double* Info)
    {
      int nnz = Ap[n_col];
      traits::detail::array<double> Axr (nnz); 
      if (!Axr.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<double> Axi (nnz); 
      if (!Axi.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::disentangle (Ax, Ax+nnz, 
                                   Axr.storage(), Axi.storage()); 
      return umfpack_zi_qsymbolic (n_row, n_col, Ap, Ai, 
                                   Axr.storage(), Axi.storage(), 
                                   Qinit, Symbolic, Control, Info); 
    }


    ///////////////////////////////////////
    // UMFPACK matrix manipulation routines
    ///////////////////////////////////////

    // triplet (coordinate) to compressed column 

    inline
    int triplet_to_col (int n_row, int n_col, int nz,
                        int const* Ti, int const* Tj, double const* Tx, 
                        int* Ap, int* Ai, double* Ax, int* Map) 
    {
      return umfpack_di_triplet_to_col (n_row, n_col, nz, 
                                        Ti, Tj, Tx,
                                        Ap, Ai, Ax, Map);
    }

    inline 
    int triplet_to_col (int n_row, int n_col, int nz,
                        int const* Ti, int const* Tj, 
                        traits::complex_d const* Tx, 
                        int* Ap, int* Ai, traits::complex_d* Ax, 
                        int* Map)
    {
      assert (Tx == 0 && Ax == 0 || Tx != 0 && Ax != 0); 
      double *Txr = 0, *Txi = 0;
      if (Tx != 0) {
        traits::detail::array<double> ATxr (nz); 
        if (!ATxr.valid()) return UMFPACK_ERROR_out_of_memory;
        traits::detail::array<double> ATxi (nz); 
        if (!ATxi.valid()) return UMFPACK_ERROR_out_of_memory;
        Txr = ATxr.storage();
        Txi = ATxi.storage(); 
        traits::detail::disentangle (Tx, Tx+nz, Txr, Txi); 
      }
      double *Axr = 0, *Axi = 0;
      if (Ax != 0) {
        traits::detail::array<double> AAxr (nz); 
        if (!AAxr.valid()) return UMFPACK_ERROR_out_of_memory;
        traits::detail::array<double> AAxi (nz); 
        if (!AAxi.valid()) return UMFPACK_ERROR_out_of_memory;
        Axr = AAxr.storage();
        Axi = AAxi.storage(); 
      } 
      int status; 
      status = umfpack_zi_triplet_to_col (n_row, n_col, nz, 
                                          Ti, Tj, Txr, Txi,
                                          Ap, Ai, Axr, Axi, Map);
      if (Ax != 0) {
        if (status != UMFPACK_OK) return status; 
        traits::detail::interlace (Axr, Axr + nz, Axi, Ax); 
      }
      return status; 
    }


    // scale

    inline
    int scale (int, double* X, double const* B, void* Numeric) {
      return umfpack_di_scale (X, B, Numeric); 
    } 

    inline
    int scale (int n, traits::complex_d* X, 
               traits::complex_d const* B, void* Numeric) 
    {
      traits::detail::array<double> Br (n); 
      if (!Br.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<double> Bi (n); 
      if (!Bi.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::disentangle (B, B+n, 
                                   Br.storage(), Bi.storage()); 
      traits::detail::array<double> Xr (n); 
      if (!Xr.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<double> Xi (n); 
      if (!Xi.valid()) return UMFPACK_ERROR_out_of_memory;

      int status = umfpack_zi_scale (Xr.storage(), Xi.storage(), 
                                     Br.storage(), Bi.storage(), 
                                     Numeric); 
      if (status != UMFPACK_OK) return status; 
      traits::detail::interlace (Xr.storage(), Xr.storage() + n,
                                 Xi.storage(), X); 
      return status; 
    } 


    //////////////////////////////
    // UMFPACK reporting routines:
    //////////////////////////////

    // report status

    inline
    void report_status (double, int, double const* Control, int status) {
      umfpack_di_report_status (Control, status); 
    }
    inline
    void report_status (traits::complex_d const&, int, 
                        double const* Control, int status) 
    {
      umfpack_zi_report_status (Control, status); 
    }


    // report control

    inline
    void report_control (double, int, double const* Control) {
      umfpack_di_report_control (Control); 
    }
    inline
    void 
    report_control (traits::complex_d const&, int, double const* Control) {
      umfpack_zi_report_control (Control); 
    }


    // report info

    inline
    void report_info (double, int, double const* Control, double const* Info) {
      umfpack_di_report_info (Control, Info); 
    }
    inline
    void report_info (traits::complex_d const&, int, 
                      double const* Control, double const* Info) 
    {
      umfpack_zi_report_info (Control, Info); 
    }


    // report matrix 

    inline 
    int report_matrix (int n_row, int n_col, 
                       int const* Ap, int const* Ai, double const* Ax,
                       int col_form, double const* Control) 
    {
      return umfpack_di_report_matrix (n_row, n_col, Ap, Ai, Ax, 
                                       col_form, Control); 
    }

    inline 
    int report_matrix (int n_row, int n_col, 
                       int const* Ap, int const* Ai, 
                       traits::complex_d const* Ax, 
                       int col_form, double const* Control) 
    {
      int nnz = Ap[n_col];
      traits::detail::array<double> Axr (nnz); 
      if (!Axr.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<double> Axi (nnz); 
      if (!Axi.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::disentangle (Ax, Ax+nnz, 
                                   Axr.storage(), Axi.storage()); 
      return umfpack_zi_report_matrix (n_row, n_col, Ap, Ai, 
                                       Axr.storage(), Axi.storage(), 
                                       col_form, Control); 
    }


    // report triplet (coordinate)

    int report_triplet (int n_row, int n_col, int nz,
                        int const* Ti, int const* Tj, double const* Tx,
                        double const* Control) 
    {
      return umfpack_di_report_triplet (n_row, n_col, nz, Ti, Tj, Tx, Control);
    }

    int report_triplet (int n_row, int n_col, int nz,
                        int const* Ti, int const* Tj, 
                        traits::complex_d const* Tx, 
                        double const* Control) 
    {
      traits::detail::array<double> Txr (nz); 
      if (!Txr.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<double> Txi (nz); 
      if (!Txi.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::disentangle (Tx, Tx+nz, 
                                   Txr.storage(), Txi.storage()); 
      return umfpack_zi_report_triplet (n_row, n_col, nz, Ti, Tj, 
                                        Txr.storage(), Txi.storage(), 
                                        Control);
    }


    // report vector 

    int report_vector (int n, double const* X, double const* Control) {
      return umfpack_di_report_vector (n, X, Control);
    }

    int report_vector (int n, traits::complex_d const* X, 
                       double const* Control) 
    {
#if 0
      // see UMFPACK v 4.1 User Guide
      traits::detail::array<double> Xr (n); 
      if (!Xr.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::array<double> Xi (n); 
      if (!Xi.valid()) return UMFPACK_ERROR_out_of_memory;
      traits::detail::disentangle (X, X+n, 
                                   Xr.storage(), Xi.storage()); 
      return umfpack_zi_report_vector (n, Xr.storage(), Xi.storage(), Control);
#endif 
      return umfpack_zi_report_vector (n, 
                                       reinterpret_cast<double const*> (X), 
                                       reinterpret_cast<double const*> (0), 
                                       Control);
    }


    // report numeric 

    inline
    int report_numeric (double, int, void* Numeric, double const* Control) {
      return umfpack_di_report_numeric (Numeric, Control); 
    }
    inline
    int report_numeric (traits::complex_d const&, int, 
                        void* Numeric, double const* Control) 
    {
      return umfpack_zi_report_numeric (Numeric, Control); 
    }


    // report symbolic 

    inline
    int report_symbolic (double, int, void* Symbolic, double const* Control) {
      return umfpack_di_report_symbolic (Symbolic, Control); 
    }
    inline
    int report_symbolic (traits::complex_d const&, int, 
                         void* Symbolic, double const* Control) 
    {
      return umfpack_zi_report_symbolic (Symbolic, Control); 
    }


    // report permutation vector

    inline
    int report_perm (double, int, int np, 
                     int const* Perm, double const* Control) {
      return umfpack_di_report_perm (np, Perm, Control); 
    }
    inline
    int report_perm (traits::complex_d const&, int, int np, 
                     int const* Perm, double const* Control) {
      return umfpack_zi_report_perm (np, Perm, Control); 
    }

  }}

}}}

#endif // BOOST_NUMERIC_BINDINGS_UMFPACK_OVERLOADS_HPP
