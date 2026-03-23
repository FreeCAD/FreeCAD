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

#ifdef HAVE_CONFIG_H
#include "config.h"
#else /* No config.h? Hmm. Assume the zlib library is available for linking. */
#define ZLIBGLUE_ASSUME_ZLIB 1
#endif /* !HAVE_CONFIG_H */

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#ifdef HAVE_ZLIB /* In case we're _not_ doing runtime linking. */
#define ZLIBGLUE_ASSUME_ZLIB 1
#include <zlib.h>
#endif /* ZLIBGLUE_ASSUME_ZLIB */

#include <Inventor/C/basic.h>
#include <Inventor/C/glue/dl.h>
#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/tidbits.h>

#include "tidbitsp.h"
#include "threads/threadsutilp.h"
#include "glue/zlib.h"
#include "io/gzmemio.h"

/* workarounds for hacks in the zlib header file. inflateInit2
   and deflateInit2 are not functions but defines. The real
   function names are inflateInit2_ and deflateInit2_ *sigh* */

#ifdef deflateInit
#undef deflateInit
#endif /* deflateInit */

#ifdef deflateInit2
#undef deflateInit2
#endif /* deflateInit2 */

#ifdef inflateInit
#undef inflateInit
#endif /* inflateInit */

#ifdef inflateInit2
#undef inflateInit2
#endif /* inflateInit */

#define deflateInit2 deflateInit2_
#define inflateInit2 inflateInit2_

typedef const char * (*cc_zlibglue_zlibVersion_t)(void); 
typedef int (*cc_zlibglue_deflateInit2_t)(void * stream,
                                        int level,
                                        int method,
                                        int windowbits,
                                        int memlevel,
                                        int strategy);

typedef int (*cc_zlibglue_inflateInit2_t)(void * stream,
                                          int windowbits,
                                          const char * version,
                                          int stream_size);

typedef int (*cc_zlibglue_deflateEnd_t)(void * stream);
typedef int (*cc_zlibglue_inflateEnd_t)(void * stream);
typedef int (*cc_zlibglue_inflate_t)(void * stream, int flush);
typedef int (*cc_zlibglue_inflateReset_t)(void * stream);
typedef int (*cc_zlibglue_deflateParams_t)(void * stream, int level, int strategy);
typedef int (*cc_zlibglue_deflate_t)(void * stream, int flush);

typedef void * (*cc_zlibglue_gzopen_t)(const char * path, const char * mode);
typedef void * (*cc_zlibglue_gzdopen_t)(int fd, const char * mode);
typedef int (*cc_zlibglue_gzsetparams_t)(void * fp, int level, int strategy);
typedef int (*cc_zlibglue_gzread_t)(void * fp, void * buf, unsigned int len);
typedef int (*cc_zlibglue_gzwrite_t)(void * fp, const void * buf, unsigned int len);
typedef off_t (*cc_zlibglue_gzseek_t)(void * fp, off_t offset, int whence);
typedef int (*cc_zlibglue_gzrewind_t)(void * fp);
typedef off_t (*cc_zlibglue_gztell_t)(void * fp);
typedef int (*cc_zlibglue_gzeof_t)(void * fp);
typedef int (*cc_zlibglue_gzclose_t)(void * fp);
typedef int (*cc_zlibglue_crc32_t)(unsigned long crc, const char * buf, unsigned int len);

typedef struct {
  int available;
  cc_zlibglue_zlibVersion_t zlibVersion;
  cc_zlibglue_deflateInit2_t deflateInit2;
  cc_zlibglue_inflateInit2_t inflateInit2;
  cc_zlibglue_deflateEnd_t deflateEnd;
  cc_zlibglue_inflateEnd_t inflateEnd;
  cc_zlibglue_inflate_t inflate;
  cc_zlibglue_inflateReset_t inflateReset;
  cc_zlibglue_deflateParams_t deflateParams;
  cc_zlibglue_deflate_t deflate;
  cc_zlibglue_gzopen_t gzopen;
  cc_zlibglue_gzdopen_t gzdopen;
  cc_zlibglue_gzsetparams_t gzsetparams;
  cc_zlibglue_gzread_t gzread;
  cc_zlibglue_gzwrite_t gzwrite;
  cc_zlibglue_gzseek_t gzseek;
  cc_zlibglue_gzrewind_t gzrewind;
  cc_zlibglue_gztell_t gztell;
  cc_zlibglue_gzeof_t gzeof;
  cc_zlibglue_gzclose_t gzclose;
  cc_zlibglue_crc32_t crc32;
} cc_zlibglue_t;

static cc_zlibglue_t * zlib_instance = NULL;
static cc_libhandle zlib_libhandle = NULL;
static int zlib_failed_to_load = 0;

/* Cleans up at exit. */
static void
zlibglue_cleanup(void)
{
#ifdef ZLIB_RUNTIME_LINKING
  if (zlib_libhandle) {
    cc_dl_close(zlib_libhandle);
    zlib_libhandle = NULL;
  }
#endif /* ZLIB_RUNTIME_LINKING */
  assert(zlib_instance);
  free(zlib_instance);
  zlib_instance = NULL;
  zlib_failed_to_load = 0;
}

static const cc_zlibglue_t *
zlibglue_init(void)
{
  CC_SYNC_BEGIN(zlibglue_init);
    
  if (!zlib_instance && !zlib_failed_to_load) {
    /* First invocation, do initializations. */
    cc_zlibglue_t * zi = (cc_zlibglue_t *)malloc(sizeof(cc_zlibglue_t));
    (void)coin_atexit((coin_atexit_f *)zlibglue_cleanup, CC_ATEXIT_DYNLIBS);

    /* The common case is that zlib is either available from the
       linking process or we're successfully going to link it in. */
    zi->available = 1;

#ifdef ZLIB_RUNTIME_LINKING
    {
      int idx;
      /* FIXME: should we get the system shared library name from an
         Autoconf check? 20000930 mortene. */
      const char * possiblelibnames[] = {
        NULL, /* is set below */
        "zlib1", "zlib", "libz", "libz.so",
        "libz.dylib", 
        NULL
      };

      possiblelibnames[0] = coin_getenv("COIN_ZLIB_LIBNAME");
      idx = possiblelibnames[0] ? 0 : 1;

      while (!zlib_libhandle && possiblelibnames[idx]) {
        zlib_libhandle = cc_dl_open(possiblelibnames[idx]);
        idx++;
      }

      if (!zlib_libhandle) {
        zi->available = 0;
        zlib_failed_to_load = 1;
      }
    }
    /* Define ZLIBGLUE_REGISTER_FUNC macro. Casting the type is
       necessary for this file to be compatible with C++ compilers. */
#define ZLIBGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    do { \
      zi->_funcname_ = (_funcsig_)cc_dl_sym(zlib_libhandle, SO__QUOTE(_funcname_)); \
      if (zi->_funcname_ == NULL) zi->available = 0; \
    } while (0)

#elif defined(ZLIBGLUE_ASSUME_ZLIB) /* !ZLIB_RUNTIME_LINKING */

    /* Define ZLIBGLUE_REGISTER_FUNC macro. */
#define ZLIBGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    zi->_funcname_ = (_funcsig_)_funcname_

#else /* !ZLIBGLUE_ASSUME_ZLIB */
    zi->available = 0;
    /* Define ZLIBGLUE_REGISTER_FUNC macro. */
#define ZLIBGLUE_REGISTER_FUNC(_funcsig_, _funcname_) \
    zi->_funcname_ = NULL

#endif /* !ZLIBGLUE_ASSUME_ZLIB */

    if (zi->available) {
      ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_zlibVersion_t, zlibVersion);
    }
    
    if (!zi->available || !zi->zlibVersion) {
      if (!zi->available) {
        cc_debugerror_post("zlib glue",
                           "Unable to load zlib DLL/shared object.");
        
      }
      else {
        /* something is seriously wrong */
        cc_debugerror_post("zlib glue",
                           "Loaded zlib DLL ok, but couldn't resolve symbol "
                           "zlibVersion().");
      }
      zi->available = 0;
      zlib_failed_to_load = 1;

      zlib_instance = zi;
    }
    else {
      int major, minor, patch;;
      if (!coin_parse_versionstring(zi->zlibVersion(), &major, &minor, &patch) ||
          (major < 1) ||
          (major == 1 && minor == 0 && patch < 2)) {
        cc_debugerror_post("zlib glue",
                           "Loaded zlib DLL ok, but version >= 1.0.2 is needed.");        
        zi->available = 0;
        zlib_failed_to_load = 1;
        zlib_instance = zi;
      }
      else {
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_deflateInit2_t, deflateInit2);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_inflateInit2_t, inflateInit2);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_deflateEnd_t, deflateEnd);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_inflateEnd_t, inflateEnd);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_inflate_t, inflate);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_inflateReset_t, inflateReset);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_deflateParams_t, deflateParams);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_deflate_t, deflate);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_gzopen_t, gzopen);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_gzdopen_t, gzdopen);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_gzsetparams_t, gzsetparams);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_gzread_t, gzread);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_gzwrite_t, gzwrite);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_gzseek_t, gzseek);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_gzrewind_t, gzrewind);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_gztell_t, gztell);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_gzeof_t, gzeof);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_gzclose_t, gzclose);
        ZLIBGLUE_REGISTER_FUNC(cc_zlibglue_crc32_t, crc32);
        
        /* Do this late, so we can detect recursive calls to this function. */
        zlib_instance = zi;
      }
    }
  }
  CC_SYNC_END(zlibglue_init);
  return zlib_instance;
}


int 
cc_zlibglue_available(void)
{
  zlibglue_init();
  return zlib_instance && zlib_instance->available;
}

int 
cc_zlibglue_deflateInit2(void * stream,
                         int level,
                         int method,
                         int windowbits,
                         int memlevel,
                         int strategy)
{
  zlibglue_init();
  return zlib_instance->deflateInit2(stream,
                                     level,
                                     method,
                                     windowbits,
                                     memlevel,
                                     strategy);
}

int 
cc_zlibglue_inflateInit2(void * stream,
                         int windowbits)
{
  zlibglue_init();
  return zlib_instance->inflateInit2(stream,
                                     windowbits,
                                     zlib_instance->zlibVersion(),
                                     cc_gzm_sizeof_z_stream());
}

int 
cc_zlibglue_deflateEnd(void * stream)
{
  zlibglue_init();
  return zlib_instance->deflateEnd(stream);
}

int 
cc_zlibglue_inflateEnd(void * stream)
{
  zlibglue_init();
  return zlib_instance->inflateEnd(stream);
}

int 
cc_zlibglue_inflate(void * stream, int flush)
{
  zlibglue_init();
  return zlib_instance->inflate(stream, flush);
}

int 
cc_zlibglue_inflateReset(void * stream)
{
  zlibglue_init();
  return zlib_instance->inflateReset(stream);
}

int 
cc_zlibglue_deflateParams(void * stream, int level, int strategy)
{
  zlibglue_init();
  return zlib_instance->deflateParams(stream, level, strategy);
}

int 
cc_zlibglue_deflate(void * stream, int flush)
{
  zlibglue_init();
  return zlib_instance->deflate(stream, flush);
}

void * 
cc_zlibglue_gzopen(const char * path, const char * mode)
{
  zlibglue_init();
  return zlib_instance->gzopen(path, mode);
}

void * 
cc_zlibglue_gzdopen(int fd, const char * mode)
{
  zlibglue_init();
  return zlib_instance->gzdopen(fd, mode);
}

int 
cc_zlibglue_gzsetparams(void * fp, int level, int strategy)
{
  zlibglue_init();
  return zlib_instance->gzsetparams(fp, level, strategy);
}

int 
cc_zlibglue_gzread(void * fp, void * buf, unsigned int len)
{
  zlibglue_init();
  return zlib_instance->gzread(fp, buf, len);
}

int 
cc_zlibglue_gzwrite(void * fp, const void * buf, unsigned int len)
{
  zlibglue_init();
  return zlib_instance->gzwrite(fp, buf, len);
}

off_t 
cc_zlibglue_gzseek(void * fp, off_t offset, int whence)
{
  zlibglue_init();
  return zlib_instance->gzseek(fp, offset, whence);
}

int 
cc_zlibglue_gzrewind(void * fp)
{
  zlibglue_init();
  return zlib_instance->gzrewind(fp);
}

off_t 
cc_zlibglue_gztell(void * fp)
{
  zlibglue_init();
  return zlib_instance->gztell(fp);
}

int 
cc_zlibglue_gzeof(void * fp)
{
  zlibglue_init();
  return zlib_instance->gzeof(fp);
}

int 
cc_zlibglue_gzclose(void * fp)
{
  zlibglue_init();
  return zlib_instance->gzclose(fp);
}

int 
cc_zlibglue_crc32(unsigned long crc, const char * buf, unsigned int len)
{
  zlibglue_init();
  return zlib_instance->crc32(crc, buf, len);
}

#undef deflateInit2
#undef inflateInit2
