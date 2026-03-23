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

#ifndef COIN_TIDBITSP_H
#define COIN_TIDBITSP_H

#ifndef COIN_INTERNAL
#error this is a private header file
#endif

#include <stdio.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/C/basic.h>
#include <Inventor/C/base/string.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if 0 /* to get proper auto-indentation in emacs */
}
#endif /* emacs indentation */

/* ********************************************************************** */

void coin_init_tidbits(void);

/* ********************************************************************** */

FILE * coin_get_stdin(void);
FILE * coin_get_stdout(void);
FILE * coin_get_stderr(void);

/* ********************************************************************** */

#define coin_atexit(func, priority) \
        coin_atexit_func(SO__QUOTE(func), func, priority)

void coin_atexit_cleanup(void);

SbBool coin_is_exiting(void);

/* this enum contains all values that are available for use for the
   third argument to coin_atexit_func() -- no other values than these
   should be used! */
enum coin_atexit_priorities {
  /* Absolute priorities goes first */

  /* cleanups of client code should be done before any cleanup code
     within Coin happens, so this is (2^31 - 1): */
  CC_ATEXIT_EXTERNAL = 2147483647,

  /* cleanup code with no particular dependencies should use this
     code: */
  CC_ATEXIT_NORMAL = 0,

  /* dynamically loaded libraries should be the last to go, as other
     code in Coin will be dependent on functionality in these in its
     own cleanup code: */
  CC_ATEXIT_DYNLIBS = -2147483647,

  /* Relative priorities */

  /* The realTime field should be cleaned up before normal cleanups
     are called, since the global field list will be cleaned up there.
  */
  CC_ATEXIT_REALTIME_FIELD = CC_ATEXIT_NORMAL + 10,

  /*
    We need to clean up default parts for draggers before tracking SoBase instances
   */
  CC_ATEXIT_DRAGGERDEFAULTS = CC_ATEXIT_NORMAL + 2,

  /*
    SoBase instance tracking should happen before normal cleanups
  */
  CC_ATEXIT_TRACK_SOBASE_INSTANCES = CC_ATEXIT_NORMAL + 1,

  /* Just after NORMAL */
  CC_ATEXIT_NORMAL_LOWPRIORITY = CC_ATEXIT_NORMAL - 1,

  /* 
     Used to clean up static data for nodes/elements/nodekits ++
     Must be done before the typesystem cleanup 
  */
  CC_ATEXIT_STATIC_DATA = CC_ATEXIT_NORMAL - 10,

  /*
     Cleanups for static SoDB data (sensor manager, converters++)
   */
  CC_ATEXIT_SODB = CC_ATEXIT_NORMAL - 20,
  /*
    SoBase (the base class) cleanup.
   */
  CC_ATEXIT_SOBASE = CC_ATEXIT_NORMAL - 30,

  /*
    Typesystem cleanup.
  */
  CC_ATEXIT_SOTYPE  = CC_ATEXIT_NORMAL - 40,

  /* later, in case e.g. some nodes' cleanup depends on the font
     subsystem still being up'n'running: */
  CC_ATEXIT_FONT_SUBSYSTEM = CC_ATEXIT_NORMAL - 100,

  /* Just before FONT_SUBSYSTEM */
  CC_ATEXIT_FONT_SUBSYSTEM_HIGHPRIORITY = CC_ATEXIT_FONT_SUBSYSTEM + 1,

  /* Just later than FONT_SUBSYSTEM */
  CC_ATEXIT_FONT_SUBSYSTEM_LOWPRIORITY = CC_ATEXIT_FONT_SUBSYSTEM - 1,

  /* still later, so cleanup code can use e.g. SoDebugError to report
     problems, output debugging info, etc: */
  CC_ATEXIT_MSG_SUBSYSTEM = CC_ATEXIT_NORMAL - 200,
  /*
    Clean up the SbName dictionary. Should be done as late as
    possible, since SbName is used a lot in other modules.
  */
  CC_ATEXIT_SBNAME = CC_ATEXIT_NORMAL - 500,

  /* needs to happen late, since CC_ATEXIT_NORMAL cleanup routines
     will for instance often want to dealloc mutexes: */
  CC_ATEXIT_THREADING_SUBSYSTEM = CC_ATEXIT_NORMAL - 1000,

  /* FIXME: Not sure if this is needed, refer comment by mortene where
     enum is used in mutex.cpp - 20080711 BFG */
  CC_ATEXIT_THREADING_SUBSYSTEM_LOWPRIORITY = CC_ATEXIT_THREADING_SUBSYSTEM - 1,

  /* FIXME: Not sure if this is needed, refer comment by mortene where
     enum is used in thread.cpp - 20080711 BFG */
  CC_ATEXIT_THREADING_SUBSYSTEM_VERYLOWPRIORITY = CC_ATEXIT_THREADING_SUBSYSTEM - 2,

  /* Needs to happen late, but before we cleanup our dynamic
     libraries */
  CC_ATEXIT_ENVIRONMENT = CC_ATEXIT_DYNLIBS + 10


};

void coin_atexit_func(const char * name, coin_atexit_f * fp, coin_atexit_priorities priority);

/* ********************************************************************** */

/*
  We're using these to ensure portable import and export even when the
  application sets a locale with different thousands separator and
  decimal point than the default "C" locale.

  Use these functions to wrap locale-aware functions where
  necessary:

  \code
  cc_string storedlocale;
  SbBool changed = coin_locale_set_portable(&storedlocale);

  // [code with locale-aware functions]

  if (changed) { coin_locale_reset(&storedlocale); }
  \endcode

  Possibly locale-aware functions includes at least atof(), atoi(),
  atol(), strtol(), strtoul(), strtod(), strtof(), strtold(), and all
  the *printf() functions.
*/

SbBool coin_locale_set_portable(cc_string * storeold);
void coin_locale_reset(cc_string * storedold);

/*
  Portable atof() function, which will not cause any trouble due to
  underlying locale's decimal point setting.
*/
double coin_atof(const char * ptr);

/* ********************************************************************** */

/*
  Functions to output ascii85 encoded data. Used for instance for PostScript
  image rendering.
*/
void coin_output_ascii85(FILE * fp,
                         const unsigned char val,
                         unsigned char * tuple,
                         unsigned char * linebuf,
                         int * tuplecnt, int * linecnt,
                         const int rowlen,
                         const SbBool flush);

void coin_flush_ascii85(FILE * fp,
                        unsigned char * tuple,
                        unsigned char * linebuf,
                        int * tuplecnt, int * linecnt,
                        const int rowlen);

/* ********************************************************************** */

/*
  Parse version string of type <major>.<minor>.<patch>. <minor> or
  <patch> might not be in the string. It's possible to supply NULL for
  minor and/or patch if you're not interested in minor and/or patch.
*/
SbBool coin_parse_versionstring(const char * versionstr,
                                int * major,
                                int * minor,
                                int * patch);

/* ********************************************************************** */

SbBool coin_getcwd(cc_string * str);

/* ********************************************************************** */

int coin_isinf(double value);
int coin_isnan(double value);
int coin_finite(double value);

/* ********************************************************************** */

unsigned long coin_geq_prime_number(unsigned long num);

/* ********************************************************************** */

enum CoinOSType {
  COIN_UNIX,
  COIN_OS_X,
  COIN_MSWINDOWS
};

int coin_runtime_os(void);

#define COIN_MAC_FRAMEWORK_IDENTIFIER_CSTRING ("org.coin3d.Coin.framework")

/* ********************************************************************** */

int coin_debug_extra(void);
int coin_debug_normalize(void);
int coin_debug_caching_level(void);

/* ********************************************************************** */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

/* ********************************************************************** */

#endif /* !COIN_TIDBITS_H */
