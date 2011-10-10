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


/***********************************************************************/
/*         UMFPACK Copyright, License and Availability                 */
/***********************************************************************/
/*
 *
 * UMFPACK Version 4.1 (Apr. 30, 2003),  Copyright (c) 2003 by Timothy A.
 * Davis.  All Rights Reserved.
 *
 * UMFPACK License:
 *
 *   Your use or distribution of UMFPACK or any modified version of
 *   UMFPACK implies that you agree to this License.
 *
 *   THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY
 *   EXPRESSED OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 *   Permission is hereby granted to use or copy this program, provided
 *   that the Copyright, this License, and the Availability of the original
 *   version is retained on all copies.  User documentation of any code that
 *   uses UMFPACK or any modified version of UMFPACK code must cite the
 *   Copyright, this License, the Availability note, and "Used by permission."
 *   Permission to modify the code and to distribute modified code is granted,
 *   provided the Copyright, this License, and the Availability note are
 *   retained, and a notice that the code was modified is included.  This
 *   software was developed with support from the National Science Foundation,
 *   and is provided to you free of charge.
 *
 * Availability:
 *
 *   http://www.cise.ufl.edu/research/sparse/umfpack
 *
 */


/* Used by permission. */ 


#ifndef BOOST_NUMERIC_BINDINGS_UMFPACK_INC_H
#define BOOST_NUMERIC_BINDINGS_UMFPACK_INC_H

extern "C" {
#if defined(FC_OS_WIN64) || defined(FC_OS_WIN32)
# include <umfpack/umfpack.h>
#else
# include <suitesparse/umfpack.h>
#endif
}

#endif 
