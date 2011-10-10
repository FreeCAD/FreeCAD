//
//  Copyright (c) 2002-2003
//  Toon Knapen, Kresimir Fresl, Joerg Walter
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_NUMERIC_BINDINGS_CONFIG_HPP
#define BOOST_NUMERIC_BINDINGS_CONFIG_HPP

#include <boost/config.hpp> 

// Microsoft Visual C++
#if defined (BOOST_MSVC)
// .. version 6.0 & 7.0
#  if BOOST_MSVC <= 1300
#    define BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS
#  endif

#elif defined(__ICC)

#elif defined(__IBMCPP__)

#elif defined(__GNUC__)

#elif defined(__COMO__)

#elif defined(__KCC)

#elif defined(__sgi)

#else
#error bindings do not recognise compiler
#endif
 

#if defined (BOOST_NUMERIC_BINDINGS_POOR_MANS_TRAITS)

// structure checks require proper traits
#  define BOOST_NUMERIC_BINDINGS_NO_STRUCTURE_CHECK 

// type checks require proper traits 
#  define BOOST_NUMERIC_BINDINGS_NO_TYPE_CHECK 

#endif 

#endif

