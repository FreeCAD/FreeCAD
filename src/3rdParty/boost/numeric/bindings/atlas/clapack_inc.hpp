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

//////////////////////////////////////////////////////////////////////////
//
// ATLAS (Automatically Tuned Linear Algebra Software)
// 
// ''At present, it provides C and Fortran77 interfaces to a portably 
// efficient BLAS implementation, as well as a few routines from LAPACK.''
//
// see: http://math-atlas.sourceforge.net/
//
//////////////////////////////////////////////////////////////////////////

#ifndef BOOST_NUMERIC_BINDINGS_CLAPACK_INC_H
#define BOOST_NUMERIC_BINDINGS_CLAPACK_INC_H

extern "C" {
/* see footnote [2] in libs/numeric/bindings/lapack/doc/index.html */ 
/* #include <atlas/atlas_enum.h> */
#include <atlas/clapack.h> 
}

#endif 
