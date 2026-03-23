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

#include "glue/bzip2.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#else /* No config.h? Hmm. Assume the zlib library is available for linking. */
#define BZ2GLUE_ASSUME_BZIP2 1
#endif /* !HAVE_CONFIG_H */

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#ifdef HAVE_BZIP2 /* In case we're _not_ doing runtime linking. */
#define BZGLUE_ASSUME_BZIP2 1
#include <bzlib.h>
#endif /* HAVE_BZIP2 */

#include <Inventor/C/basic.h>
#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/C/glue/dl.h>

#include "threads/threadsutilp.h"
#include "tidbitsp.h"

typedef const char * (*cc_bzglue_BZ2_bzlibVersion_t)(void);

typedef void * (*cc_bzglue_BZ2_bzReadOpen_t)(int * bzerror,
                                             FILE * f,
                                             int verbosity,
                                             int bzsmall,
                                             void * unused,
                                             int nunused);
typedef void (*cc_bzglue_BZ2_bzReadClose_t)(int * bzerror, 
                                            void * bzfile); 
typedef int (*cc_bzglue_BZ2_bzRead_t)(int * bzerror, 
                                      void * bzfile, 
                                      void * buf, 
                                      int len);
typedef void * (*cc_bzglue_BZ2_bzWriteOpen_t)(int * bzerror,      
                                              FILE * fp, 
                                              int blocksize100k, 
                                              int verbosity, 
                                              int workfactor); 
typedef void (*cc_bzglue_BZ2_bzWriteClose_t)(int * bzerror, 
                                             void * bzfile, 
                                             int abandon, 
                                             unsigned int * nbytesin, 
                                             unsigned int * nbytesout);
typedef void (*cc_bzglue_BZ2_bzWrite_t)(int * bzerror, 
                                        void * bzfile, 
                                        void * buf, 
                                        int len);

typedef struct {
  int available;
  cc_bzglue_BZ2_bzlibVersion_t BZ2_bzlibVersion;
  cc_bzglue_BZ2_bzReadOpen_t BZ2_bzReadOpen;
  cc_bzglue_BZ2_bzReadClose_t BZ2_bzReadClose; 
  cc_bzglue_BZ2_bzRead_t BZ2_bzRead;
  cc_bzglue_BZ2_bzWriteOpen_t BZ2_bzWriteOpen;
  cc_bzglue_BZ2_bzWriteClose_t BZ2_bzWriteClose;
  cc_bzglue_BZ2_bzWrite_t BZ2_bzWrite;
} cc_bzglue_t;


static cc_bzglue_t * bzlib_instance = NULL;
static cc_libhandle bzlib_libhandle = NULL;
static int bzlib_failed_to_load = 0;

/* Cleans up at exit. */
static void
bzglue_cleanup(void)
{
#ifdef LIBBZIP2_RUNTIME_LINKING
  if (bzlib_libhandle) { 
    cc_dl_close(bzlib_libhandle); 
    bzlib_libhandle = NULL;
  }
#endif /* LIBBZIP2_RUNTIME_LINKING */
  assert(bzlib_instance);
  free(bzlib_instance);
  bzlib_instance = NULL;
  bzlib_failed_to_load = 0;
}

static const cc_bzglue_t *
bzglue_init(void)
{
  CC_SYNC_BEGIN(bzglue_init);

  if (!bzlib_instance && !bzlib_failed_to_load) {
    /* First invocation, do initializations. */
    cc_bzglue_t * bi = (cc_bzglue_t *)malloc(sizeof(cc_bzglue_t));
    (void)coin_atexit((coin_atexit_f *)bzglue_cleanup, CC_ATEXIT_DYNLIBS);

    /* The common case is that libbz2 is either available from the
       linking process or we're successfully going to link it in. */
    bi->available = 1;

#ifdef LIBBZIP2_RUNTIME_LINKING
    {
      int idx;
      /* FIXME: should we get the system shared library name from an
         Autoconf check? 20000930 mortene. */
      const char * possiblelibnames[] = {
        NULL, /* is set below */ 
        "bz2", "libbz2", "libbz2.so",
        "libbz2.dylib", 
        NULL
      };

      possiblelibnames[0] = coin_getenv("COIN_BZIP2_LIBNAME");
      idx = possiblelibnames[0] ? 0 : 1;
      while (!bzlib_libhandle && possiblelibnames[idx]) {
        bzlib_libhandle = cc_dl_open(possiblelibnames[idx]);
        idx++;
      }

      if (!bzlib_libhandle) {
        bi->available = 0;
        bzlib_failed_to_load = 1;
      }
    }
    /* Define BZGLUE_REGISTER_FUNC macro. Casting the type is
       necessary for this file to be compatible with C++ compilers. */
#define BZGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    do { \
      bi->_funcname_ = (_funcsig_)cc_dl_sym(bzlib_libhandle, SO__QUOTE(_funcname_)); \
      if (bi->_funcname_ == NULL) bi->available = 0; \
    } while (0)

#elif defined(BZGLUE_ASSUME_BZIP2) /* !LIBBZIP2_RUNTIME_LINKING */

    /* Define BZGLUE_REGISTER_FUNC macro. */
#define BZGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    bi->_funcname_ = (_funcsig_)_funcname_

#else /* !BZGLUE_ASSUME_BZIP2 */
    bi->available = 0;
    /* Define BZGLUE_REGISTER_FUNC macro. */
#define BZGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    bi->_funcname_ = NULL

#endif /* !BZGLUE_ASSUME_BZIP2 */

    if (bi->available) {
      BZGLUE_REGISTER_FUNC(cc_bzglue_BZ2_bzlibVersion_t, BZ2_bzlibVersion);
    }

    if (!bi->available || !bi->BZ2_bzlibVersion) {
      if (!bi->available) {
        cc_debugerror_post("libbzip2 glue",
                           "Unable to load bzip2 DLL/shared object.");
        
      }
      else {
        /* something is seriously wrong */
        cc_debugerror_post("libbzip2 glue",
                           "Loaded bzip2 DLL ok, but couldn't resolve symbol "
                           "BZ2_bzlibVersion().");
      }
      bi->available = 0;
      bzlib_failed_to_load = 1;
      bzlib_instance = bi;
    }
    else {
      int major, minor, patch;
      if (!coin_parse_versionstring(bi->BZ2_bzlibVersion(), &major, &minor, &patch) ||
          (major < 1)) {
        cc_debugerror_post("bzip2 glue",
                           "Loaded bzip2 DLL ok, but version >= 1.0.0 is needed.");        
        bi->available = 0;
        bzlib_failed_to_load = 1;
        bzlib_instance = bi;
      }
      else {
        BZGLUE_REGISTER_FUNC(cc_bzglue_BZ2_bzReadOpen_t, BZ2_bzReadOpen);
        BZGLUE_REGISTER_FUNC(cc_bzglue_BZ2_bzReadClose_t, BZ2_bzReadClose); 
        BZGLUE_REGISTER_FUNC(cc_bzglue_BZ2_bzRead_t, BZ2_bzRead);
        BZGLUE_REGISTER_FUNC(cc_bzglue_BZ2_bzWriteOpen_t, BZ2_bzWriteOpen);
        BZGLUE_REGISTER_FUNC(cc_bzglue_BZ2_bzWriteClose_t, BZ2_bzWriteClose);
        BZGLUE_REGISTER_FUNC(cc_bzglue_BZ2_bzWrite_t, BZ2_bzWrite);
        
        /* Do this late, so we can detect recursive calls to this function. */
        bzlib_instance = bi;
      }
    }
  }
  CC_SYNC_END(bzglue_init);
  return bzlib_instance;
}

int 
cc_bzglue_available(void)
{
  bzglue_init();
  return bzlib_instance && bzlib_instance->available;
}

const char * 
cc_bzglue_BZ2_bzlibVersion(void)
{
  bzglue_init();
  return bzlib_instance->BZ2_bzlibVersion();
}

void * 
cc_bzglue_BZ2_bzReadOpen(int * bzerror,
                         FILE * f,
                         int verbosity,
                         int bzsmall,
                         void * unused,
                         int nunused)
{
  bzglue_init();
  return bzlib_instance->BZ2_bzReadOpen(bzerror,
                                        f,
                                        verbosity,
                                        bzsmall,
                                        unused,
                                        nunused);
}

void 
cc_bzglue_BZ2_bzReadClose(int * bzerror, 
                          void * bzfile)
{
  bzglue_init();
  bzlib_instance->BZ2_bzReadClose(bzerror, bzfile);
}

int 
cc_bzglue_BZ2_bzRead(int * bzerror, 
                     void * bzfile, 
                     void * buf, 
                     int len)
{
  bzglue_init();
  return bzlib_instance->BZ2_bzRead(bzerror, bzfile, buf, len);
}

void *
cc_bzglue_BZ2_bzWriteOpen(int * bzerror,      
                          FILE * fp, 
                          int blocksize100k, 
                          int verbosity, 
                          int workfactor)
{
  bzglue_init();
  return bzlib_instance->BZ2_bzWriteOpen(bzerror, fp, blocksize100k,
                                         verbosity, workfactor);
}


void 
cc_bzglue_BZ2_bzWriteClose(int * bzerror, 
                           void * bzfile, 
                           int abandon, 
                           unsigned int * nbytesin, 
                           unsigned int * nbytesout)
{
  bzglue_init();
  bzlib_instance->BZ2_bzWriteClose(bzerror, bzfile,
                                   abandon, nbytesin, nbytesout);
}

void 
cc_bzglue_BZ2_bzWrite(int * bzerror, 
                      void * bzfile, 
                      void * buf, 
                      int len)
{
  bzglue_init();
  bzlib_instance->BZ2_bzWrite(bzerror, bzfile, buf, len);
}
