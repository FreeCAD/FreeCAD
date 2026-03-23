#ifndef COIN_DEFS_H
#define COIN_DEFS_H

/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

/*
  This file contains definitions which should _only_ be used during
  library build. It is not installed for use by the application
  programmer.
*/

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* !COIN_INTERNAL */

#ifdef HAVE_CONFIG_H
#include <config.h> /* for HAVE_* defines */
#endif /* HAVE_CONFIG_H */

#include <Inventor/C/basic.h> /* For COMPILE_ONLY_BEFORE */

#ifdef __FILE__
#define COIN_STUB_FILE __FILE__
#else
#define COIN_STUB_FILE ((const char *)0L)
#endif

#ifdef __LINE__
#define COIN_STUB_LINE __LINE__
#else
#define COIN_STUB_LINE 0
#endif

#ifdef __cplusplus
#ifdef HAVE_CPP_COMPILER_FUNCTION_NAME_VAR
#define COIN_STUB_FUNC HAVE_CPP_COMPILER_FUNCTION_NAME_VAR
#define COIN_STUB_FUNC_STRING  COIN_STUB_FUNC
#else
#define COIN_STUB_FUNC ((const char *)0L)
#define COIN_STUB_FUNC_STRING  "<>"
#endif
#else /* !__cplusplus */
#ifdef HAVE_C_COMPILER_FUNCTION_NAME_VAR
#define COIN_STUB_FUNC HAVE_C_COMPILER_FUNCTION_NAME_VAR
#else
#define COIN_STUB_FUNC ((const char *)0L)
#endif
#endif /* !__cplusplus */


/*
  COIN_STUB(): this is the method which prints out stub
  information. Used where there is functionality missing.

  COIN_OBSOLETED: this is the method which prints out obsoleted
  information. Used where there is an obsoleted or unsupported
  function. Typically a function that we feel should have been private
  in Open Inventor.
*/

#if COIN_DEBUG

#include <Inventor/errors/SoDebugError.h>

#define COIN_STUB() \
  do { \
    SbString s; \
    s.sprintf("%s:%u:%s", \
              COIN_STUB_FILE ? COIN_STUB_FILE : "<>", \
              COIN_STUB_LINE, \
              COIN_STUB_FUNC_STRING); \
    SoDebugError::postWarning(s.getString(), \
                              "STUB: functionality not yet completed"); \
  } while (0)

#define COIN_STUB_ONCE() \
  do { \
    static int first = 1; \
    if (first) { \
      SbString s; \
      s.sprintf("%s:%u:%s", \
                COIN_STUB_FILE ? COIN_STUB_FILE : "<>", \
                COIN_STUB_LINE, \
                COIN_STUB_FUNC_STRING); \
      SoDebugError::postWarning(s.getString(), \
                                "STUB: functionality not yet completed"); \
      first = 0; \
    } \
  } while (0)

#define COIN_OBSOLETED() \
  do { \
    SbString s; \
    s.sprintf("%s:%u:%s", \
              COIN_STUB_FILE ? COIN_STUB_FILE : "<>", \
              COIN_STUB_LINE, \
              COIN_STUB_FUNC_STRING); \
    SoDebugError::post(s.getString(), \
                       "OBSOLETED: functionality not supported (any more)"); \
  } while (0)

#else /* !COIN_DEBUG */

#define COIN_STUB()       do { } while (0)
#define COIN_OBSOLETED()  do { } while (0)
#define COIN_STUB_ONCE()  do { } while (0)

#endif /* !COIN_DEBUG */

#ifdef __GNUC__
#define COIN_UNUSED_ARG(x) x __attribute__((__unused__))
#else
#define COIN_UNUSED_ARG(x) x
#endif


/* COIN_CT_ASSERT() - a macro for doing compile-time asserting */
#define COIN_CT_ASSERT(expr) \
  do { switch ( 0 ) { case 0: case (expr): break; } } while ( 0 )

#define COMPILE_ONLY_BEFORE(MAJOR,MINOR,MICRO,REASON)                            \
COIN_CT_ASSERT( (COIN_MAJOR_VERSION < MAJOR) || (COIN_MAJOR_VERSION == MAJOR && ((COIN_MINOR_VERSION < MINOR) || ( COIN_MINOR_VERSION == MINOR && (COIN_MICRO_VERSION < MICRO )))))

#define COIN_CONCAT( X, Y ) COIN_CONCAT_INTERNAL( X, Y )
#define COIN_CONCAT_INTERNAL( X, Y ) COIN_CONCAT_INTERNAL2(X,Y)
#define COIN_CONCAT_INTERNAL2( X, Y ) X##Y

#define COMPILE_ONLY_BEFORE_NOFUNCTION(MAJOR,MINOR,MICRO,REASON)              \
static void inline COIN_CONCAT(compile_only_before_nofunction,__LINE__) () { \
  COMPILE_ONLY_BEFORE(MAJOR,MINOR,MICRO,REASON);                              \
}

/*
  Grep for POTENTIAL_ROTTING_DOCUMENTATION and update the version
  number when you have checked all occurrences of this. Some places
  this marker is used in a pure comment, so do not content yourself
  with checking every place that fails.
*/
#define POTENTIAL_ROTTING_DOCUMENTATION COMPILE_ONLY_BEFORE_NOFUNCTION(4,0,1,"This piece of documentation may potentially be outdated in the future.")

#ifdef _MSC_VER
#define COIN_MSVC _MSC_VER
#endif /* _MSC_VER */

#define COIN_MSVC_6_0_VERSION 1200
#define COIN_MSVC_7_0_VERSION 1300
#define COIN_MSVC_7_1_VERSION 1310
#define COIN_MSVC_8_0_VERSION 1400
#define COIN_MSVC_9_0_VERSION 1500

/* see SbTime.cpp for example usage */
#define COIN_WORKAROUND(def, test) ((def) != 0 && ((def) test))

#if COIN_WORKAROUND(_MSC_VER, <= COIN_MSVC_6_0_VERSION)
#define COIN_WORKAROUND_NO_USING_STD_FUNCS
#endif

#ifdef HAVE___BUILTIN_EXPECT
/* for branch-prediction hint optimization */
#ifndef likely
#define likely(cond)      (__builtin_expect(!!(cond), 1))
#endif /* !likely */
#ifndef unlikely
#define unlikely(cond)    (__builtin_expect(!!(cond), 0))
#endif /* !unlikely */
#else /* !HAVE___BUILTIN_EXPECT */
#ifndef likely
#define likely(cond)      (cond)
#endif /* !likely */
#ifndef unlikely
#define unlikely(cond)    (cond)
#endif /* !unlikely */
#endif /* !HAVE___BUILTIN_EXPECT */

#ifdef COIN_DEBUG_CHECK_THREAD

#include <thread>

inline std::thread::id& coin_get_thread()
{
  static std::thread::id tid;
  return tid;
}

#define COIN_INIT_CHECK_THREAD() \
  do { \
    coin_get_thread() = std::this_thread::get_id(); \
  } while (0)

#define COIN_CHECK_THREAD() \
  do { \
    if (coin_get_thread() != std::this_thread::get_id()) { \
      SbString s; \
      s.sprintf("%s:%u:%s", \
                COIN_STUB_FILE ? COIN_STUB_FILE : "<>", \
                COIN_STUB_LINE, \
                COIN_STUB_FUNC_STRING); \
      SoDebugError::post(s.getString(), \
                         "Inventor access from wrong thread."); \
    } \
  } while (0)

#else /* COIN_DEBUG_CHECK_THREAD */
#define COIN_INIT_CHECK_THREAD() do { } while (0)
#define COIN_CHECK_THREAD()      do { } while (0)
#endif /* COIN_DEBUG_CHECK_THREAD */

#endif /* !COIN_DEFS_H */
