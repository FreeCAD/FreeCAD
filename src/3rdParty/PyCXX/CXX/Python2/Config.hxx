//-----------------------------------------------------------------------------
//
// Copyright (c) 1998 - 2007, The Regents of the University of California
// Produced at the Lawrence Livermore National Laboratory
// All rights reserved.
//
// This file is part of PyCXX. For details,see http://cxx.sourceforge.net/. The
// full copyright notice is contained in the file COPYRIGHT located at the root
// of the PyCXX distribution.
//
// Redistribution  and  use  in  source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
//  - Redistributions of  source code must  retain the above  copyright notice,
//    this list of conditions and the disclaimer below.
//  - Redistributions in binary form must reproduce the above copyright notice,
//    this  list of  conditions  and  the  disclaimer (as noted below)  in  the
//    documentation and/or materials provided with the distribution.
//  - Neither the name of the UC/LLNL nor  the names of its contributors may be
//    used to  endorse or  promote products derived from  this software without
//    specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR  PURPOSE
// ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  REGENTS  OF  THE  UNIVERSITY OF
// CALIFORNIA, THE U.S.  DEPARTMENT  OF  ENERGY OR CONTRIBUTORS BE  LIABLE  FOR
// ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
// SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
// CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
// LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
// OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
//-----------------------------------------------------------------------------

#ifndef __PyCXX_config_hh__
#define __PyCXX_config_hh__

//
// Microsoft VC++ 6.0 has no traits
//
#if defined( _MSC_VER )

#  define STANDARD_LIBRARY_HAS_ITERATOR_TRAITS 1

#elif defined( __GNUC__ )
#  if __GNUC__ >= 3
#    define STANDARD_LIBRARY_HAS_ITERATOR_TRAITS 1
#  else
#    define STANDARD_LIBRARY_HAS_ITERATOR_TRAITS 0
#endif

//
//	Assume all other compilers do
//
#else

// Macros to deal with deficiencies in compilers
#  define STANDARD_LIBRARY_HAS_ITERATOR_TRAITS 1
#endif

#if STANDARD_LIBRARY_HAS_ITERATOR_TRAITS
#  define random_access_iterator_parent(itemtype) std::iterator<std::random_access_iterator_tag,itemtype,int>
#else
#  define random_access_iterator_parent(itemtype) std::random_access_iterator<itemtype, int>
#endif

//
//	Which C++ standard is in use?
//
#if defined( _MSC_VER )
#  if _MSC_VER <= 1200
// MSVC++ 6.0
#    define PYCXX_ISO_CPP_LIB 0
#    define STR_STREAM <strstream>
#    define TEMPLATE_TYPENAME class
#  else
#    define PYCXX_ISO_CPP_LIB 1
#    define STR_STREAM <sstream>
#    define TEMPLATE_TYPENAME typename
#  endif
#elif defined( __GNUC__ )
#  if __GNUC__ >= 3
#    define PYCXX_ISO_CPP_LIB 1
#    define STR_STREAM <sstream>
#    define TEMPLATE_TYPENAME typename
#  else
#    define PYCXX_ISO_CPP_LIB 0
#    define STR_STREAM <strstream>
#    define TEMPLATE_TYPENAME class
#  endif
#endif

#if PYCXX_ISO_CPP_LIB
#    define STR_STREAM <sstream>
#    define OSTRSTREAM ostringstream
#    define EXPLICIT_TYPENAME typename
#    define EXPLICIT_CLASS class
#    define TEMPLATE_TYPENAME typename
#else
#    define STR_STREAM <strstream>
#    define OSTRSTREAM ostrstream
#    define EXPLICIT_TYPENAME
#    define EXPLICIT_CLASS
#    define TEMPLATE_TYPENAME class
#endif

// before 2.5 Py_ssize_t was missing
#ifndef PY_MAJOR_VERSION
#error not defined PY_MAJOR_VERSION
#endif
#if PY_MAJOR_VERSION < 2 || (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION < 5)
typedef int Py_ssize_t;
#endif

// export macro
#if defined( _MSC_VER )
#  pragma warning( disable : 4251 )
#endif
#if defined( _MSC_VER ) || defined( __MINGW32__ )
#  ifdef PYCXX_DLL
#    define PYCXX_EXPORT  __declspec(dllexport)
#  else
#    define PYCXX_EXPORT  __declspec(dllimport)
#  endif
#else
#    define PYCXX_EXPORT
#endif

#endif //  __PyCXX_config_hh__
