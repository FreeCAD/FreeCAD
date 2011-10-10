/*
 * 
 * Copyright (c) Toon Knapen & Kresimir Fresl 2003
 *
 * Permission to copy, modify, use and distribute this software 
 * for any non-commercial or commercial purpose is granted provided 
 * that this license appear on all copies of the software source code.
 *
 * Authors assume no responsibility whatsoever for its use and makes 
 * no guarantees about its quality, correctness or reliability.
 *
 * KF acknowledges the support of the Faculty of Civil Engineering, 
 * University of Zagreb, Croatia.
 *
 */

#ifndef BOOST_NUMERIC_BINDINGS_LAPACK_LAPACK_HPP
#define BOOST_NUMERIC_BINDINGS_LAPACK_LAPACK_HPP

// linear systems

#include <boost/numeric/bindings/lapack/gesv.hpp>
#include <boost/numeric/bindings/lapack/posv.hpp>
#include <boost/numeric/bindings/lapack/ppsv.hpp>
#include <boost/numeric/bindings/lapack/sysv.hpp>
#include <boost/numeric/bindings/lapack/spsv.hpp>
#include <boost/numeric/bindings/lapack/hesv.hpp>
#include <boost/numeric/bindings/lapack/hpsv.hpp>

// eigenproblems 

#include <boost/numeric/bindings/lapack/gees.hpp>
#include <boost/numeric/bindings/lapack/trevc.hpp>
#include <boost/numeric/bindings/lapack/trexc.hpp>
#include <boost/numeric/bindings/lapack/hbev.hpp>

// SVD

#include <boost/numeric/bindings/lapack/gesvd.hpp>
#include <boost/numeric/bindings/lapack/gesdd.hpp>

// Miscellaneous

#include <boost/numeric/bindings/lapack/geqrf.hpp>
#include <boost/numeric/bindings/lapack/ormqr.hpp>



#endif // BOOST_NUMERIC_BINDINGS_LAPACK_LAPACK_HPP

