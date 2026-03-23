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
 *  Environment variable controls available:
 *
 *   - COIN_DEBUG_SIMAGE: set to 1 to get information about success or
 *     failure of loading the simage library.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#else /* No config.h? Hmm. Assume the simage library is available for linking. */
#define SIMAGEWRAPPER_ASSUME_SIMAGE
#endif /* !HAVE_CONFIG_H */

#include "coindefs.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#ifdef HAVE_LIBSIMAGE /* In case we're _not_ doing runtime linking. */
#define SIMAGEWRAPPER_ASSUME_SIMAGE
#include <simage.h>
#endif /* HAVE_LIBSIMAGE */

#include <Inventor/C/basic.h>
#include <Inventor/C/glue/dl.h>
#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/tidbits.h>

#include "glue/simage_wrapper.h"
#include "threads/threadsutilp.h"
#include "tidbitsp.h"

/* ********************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static simage_wrapper_t * simage_instance = NULL;
static cc_libhandle simage_libhandle = NULL;
static int simage_failed_to_load = 0;
static int simage_is_initializing = 0;


/* Return value of COIN_DEBUG_SIMAGE environment variable. */
static int
cc_simage_debugging(void)
{
  static int d = -1;
  if (d == -1) {
    const char * val = coin_getenv("COIN_DEBUG_SIMAGE");
    d = val ? atoi(val) : 0;
  }
  return (d > 0) ? 1 : 0;
}

/* Cleans up at exit. */
static void
simage_wrapper_cleanup(void)
{
#ifdef SIMAGE_RUNTIME_LINKING
  if (simage_libhandle) {
    cc_dl_close(simage_libhandle);
    simage_libhandle = NULL;
  }
#endif /* SIMAGE_RUNTIME_LINKING */

  assert(simage_instance);
  free(simage_instance);
  simage_instance = NULL;
  simage_failed_to_load = 0;
  simage_is_initializing = 0;
}

/* backup-functions. More robust when simage is an old version, or not
   available */

static int
simage_wrapper_versionMatchesAtLeast(int major, int minor, int micro)
{
  assert(simage_instance);
  if (simage_instance->available == 0) return 0;
  if (simage_instance->version.major < major) return 0;
  else if (simage_instance->version.major > major) return 1;
  if (simage_instance->version.minor < minor) return 0;
  else if (simage_instance->version.minor > minor) return 1;
  if (simage_instance->version.micro < micro) return 0;
  return 1;
}

static int
simage_wrapper_get_num_savers(void)
{
  return 0;
}

static void *
simage_wrapper_get_saver_handle(int COIN_UNUSED_ARG(jada))
{
  return NULL;
}

static int
simage_wrapper_check_save_supported(const char * COIN_UNUSED_ARG(jada))
{
  return 0;
}

static int
simage_wrapper_save_image(const char * COIN_UNUSED_ARG(jada),
                          const unsigned char * COIN_UNUSED_ARG(jada2),
                          int COIN_UNUSED_ARG(jada3), int COIN_UNUSED_ARG(jada4), int COIN_UNUSED_ARG(jada5),
                          const char * COIN_UNUSED_ARG(jada6))
{
  return 0;
}

static const char *
simage_wrapper_get_saver_extensions(void * COIN_UNUSED_ARG(handle))
{
  return "";
}

static const char *
simage_wrapper_get_saver_fullname(void * COIN_UNUSED_ARG(handle))
{
  return NULL;
}

static const char *
simage_wrapper_get_saver_description(void * COIN_UNUSED_ARG(handle))
{
  return NULL;
}

static unsigned char *
simage_wrapper_resize3d(unsigned char * COIN_UNUSED_ARG(imagedata),
                        int COIN_UNUSED_ARG(width), int COIN_UNUSED_ARG(height),
                        int COIN_UNUSED_ARG(numcomponents),
                        int COIN_UNUSED_ARG(layers),
                        int COIN_UNUSED_ARG(newwidth),
                        int COIN_UNUSED_ARG(newheight),
                        int COIN_UNUSED_ARG(newlayers))
{
  return NULL;
}

static s_params *
simage_wrapper_s_params_create(void)
{
  return NULL;
}

static void
simage_wrapper_s_params_destroy(s_params * COIN_UNUSED_ARG(params))
{
  return;
}

static void
simage_wrapper_s_params_set(s_params * COIN_UNUSED_ARG(params), ...)
{
  return;
}

static int
simage_wrapper_s_params_get(s_params * COIN_UNUSED_ARG(params), ...)
{
  return 0;
}

static s_stream *
simage_wrapper_s_stream_open(const char * COIN_UNUSED_ARG(filename),
              s_params * COIN_UNUSED_ARG(params) /* | NULL */)
{
  return NULL;
}

static s_stream *
simage_wrapper_s_stream_create(const char * COIN_UNUSED_ARG(filename),
                s_params * COIN_UNUSED_ARG(params) /* | NULL */)
{
  return NULL;
}

static void *
simage_wrapper_s_stream_get_buffer(s_stream * COIN_UNUSED_ARG(stream),
                    void * COIN_UNUSED_ARG(prealloc) /* | NULL */,
                    int * COIN_UNUSED_ARG(size) /* | NULL */,
                    s_params * COIN_UNUSED_ARG(params) /* | NULL */)
{
  return NULL;
}

static int
simage_wrapper_s_stream_put_buffer(s_stream * COIN_UNUSED_ARG(stream), void * COIN_UNUSED_ARG(buffer),
                    int COIN_UNUSED_ARG(size), s_params * COIN_UNUSED_ARG(params) /* | NULL */)
{
  return 0;
}

static void
simage_wrapper_s_stream_close(s_stream * COIN_UNUSED_ARG(stream))
{
  return;
}

static void
simage_wrapper_s_stream_destroy(s_stream * COIN_UNUSED_ARG(stream))
{
  return;
}

static s_params *
simage_wrapper_s_stream_params(s_stream * COIN_UNUSED_ARG(stream))
{
  return NULL;
}


/* Implemented by using the singleton pattern. */
const simage_wrapper_t *
simage_wrapper(void)
{
  CC_SYNC_BEGIN(simage_wrapper);

  /* FIXME: we're not thread-safe, due to the "get_last_error" design
     of simage. Should keep a single entry-lock here to work around
     this. 20020628 mortene. */

  if (!simage_instance && !simage_failed_to_load) {
    /* First invocation, do initializations. */
    simage_wrapper_t * si = (simage_wrapper_t *)malloc(sizeof(simage_wrapper_t));
    (void)coin_atexit((coin_atexit_f *)simage_wrapper_cleanup, CC_ATEXIT_DYNLIBS);

    /* Detect recursive calls. */
    assert(simage_is_initializing == 0);
    simage_is_initializing = 1;

    si->versionMatchesAtLeast = simage_wrapper_versionMatchesAtLeast;

    /* The common case is that simage is either available from the
       linking process or we're successfully going to link it in. */
    si->available = 1;

#ifdef SIMAGE_RUNTIME_LINKING
    {
      int idx;
      const char * simage_dll_name = "simage1";

#ifdef COIN_SYSTEM_LIBRARY_NAME
      {
        /* check for 'd' suffix usage in coinX.dll coinXd.dll */
        const char * suffix = strstr(COIN_SYSTEM_LIBRARY_NAME, "d.");
        if (suffix && (strlen(suffix) != strlen(COIN_SYSTEM_LIBRARY_NAME))) {
          simage_dll_name = "simage1d";
        }
      }
#endif

      /* FIXME: should we get the system shared library name from an
         Autoconf check? 20000930 mortene. */
      const char * possiblelibnames[] = {
        NULL, /* is set below */
        "simage", "libsimage", "libsimage.so",
        /* Mach dynamic library name */
        "libsimage.dylib",
        /* Microsoft Windows DLL names for the simage library */
        simage_dll_name,
        NULL
      };

      possiblelibnames[0] = coin_getenv("COIN_SIMAGE_LIBNAME");
      idx = possiblelibnames[0] ? 0 : 1;

      while (!simage_libhandle && possiblelibnames[idx]) {
        simage_libhandle = cc_dl_open(possiblelibnames[idx]);
        idx++;
      }

      if (cc_simage_debugging()) {
        if (!simage_libhandle) {
          cc_debugerror_post("simage_wrapper",
                             "failed to load simage library.");
        } else {
          cc_debugerror_postinfo("simage_wrapper",
                                 "loaded library %s",
                                 possiblelibnames[idx-1]);
        }
      }

      if (!simage_libhandle) {
        si->available = 0;
        simage_failed_to_load = 1;
      }
    }
    /* Define SIMAGEWRAPPER_REGISTER_FUNC macro. Casting the type is
       necessary for this file to be compatible with C++ compilers. */
#define SIMAGEWRAPPER_REGISTER_FUNC(_funcname_, _funcsig_) \
    si->_funcname_ = (_funcsig_)cc_dl_sym(simage_libhandle, SO__QUOTE(_funcname_))

#elif defined(SIMAGEWRAPPER_ASSUME_SIMAGE) /* !SIMAGE_RUNTIME_LINKING */

    /* Define SIMAGEWRAPPER_REGISTER_FUNC macro. */
#define SIMAGEWRAPPER_REGISTER_FUNC(_funcname_, _funcsig_) \
    si->_funcname_ = (_funcsig_)_funcname_

#else /* !SIMAGEWRAPPER_ASSUME_SIMAGE */
    si->available = 0;
    /* Define SIMAGEWRAPPER_REGISTER_FUNC macro. */
#define SIMAGEWRAPPER_REGISTER_FUNC(_funcname_, _funcsig_) \
    si->_funcname_ = NULL

#endif /* !SIMAGEWRAPPER_ASSUME_SIMAGE */

    SIMAGEWRAPPER_REGISTER_FUNC(simage_version, simage_version_t);

    if (si->available && !si->simage_version) {
      /* something is seriously wrong */
      cc_debugerror_post("simage_wrapper",
                         "Loaded simage DLL ok, but couldn't resolve symbol "
                         "simage_version().");
      si->available = 0;
      simage_failed_to_load = 1;

      simage_instance = si;
    }
    else {
      /* get version */
      if (si->available) {
        si->simage_version(&si->version.major,
                           &si->version.minor,
                           &si->version.micro);
        if (cc_simage_debugging()) {
          cc_debugerror_postinfo ("simage_wrapper",
                                  "simage version: %d.%d.%d",
                                  si->version.major,
                                  si->version.minor,
                                  si->version.micro);
        }
      }

      SIMAGEWRAPPER_REGISTER_FUNC(simage_check_supported, simage_check_supported_t);
      SIMAGEWRAPPER_REGISTER_FUNC(simage_read_image, simage_read_image_t);
      SIMAGEWRAPPER_REGISTER_FUNC(simage_get_last_error, simage_get_last_error_t);
      SIMAGEWRAPPER_REGISTER_FUNC(simage_resize, simage_resize_t);
      SIMAGEWRAPPER_REGISTER_FUNC(simage_free_image, simage_free_image_t);
      SIMAGEWRAPPER_REGISTER_FUNC(simage_next_power_of_two, simage_next_power_of_two_t);

      /* Do this late, so we can detect recursive calls to this function. */
      simage_instance = si;

      if (simage_wrapper_versionMatchesAtLeast(1,1,0)) {
#if !defined(HAVE_LIBSIMAGE) || defined(SIMAGE_VERSION_1_1)
        SIMAGEWRAPPER_REGISTER_FUNC(simage_get_num_savers, simage_get_num_savers_t);
        SIMAGEWRAPPER_REGISTER_FUNC(simage_get_saver_handle, simage_get_saver_handle_t);
        SIMAGEWRAPPER_REGISTER_FUNC(simage_check_save_supported, simage_check_save_supported_t);
        SIMAGEWRAPPER_REGISTER_FUNC(simage_save_image, simage_save_image_t);
        SIMAGEWRAPPER_REGISTER_FUNC(simage_get_saver_extensions, simage_get_saver_extensions_t);
        SIMAGEWRAPPER_REGISTER_FUNC(simage_get_saver_fullname, simage_get_saver_fullname_t);
        SIMAGEWRAPPER_REGISTER_FUNC(simage_get_saver_description, simage_get_saver_description_t);
#endif /* !HAVE_LIBSIMAGE || SIMAGE_VERSION_1_1 */
      }
      else {
        si->simage_get_saver_handle = simage_wrapper_get_saver_handle;
        si->simage_get_num_savers = simage_wrapper_get_num_savers;
        si->simage_check_save_supported = simage_wrapper_check_save_supported;
        si->simage_save_image = simage_wrapper_save_image;
        si->simage_get_saver_extensions = simage_wrapper_get_saver_extensions;
        si->simage_get_saver_fullname = simage_wrapper_get_saver_fullname;
        si->simage_get_saver_description = simage_wrapper_get_saver_description;
      }

      if (simage_wrapper_versionMatchesAtLeast(1,3,0)) {
#if !defined(HAVE_LIBSIMAGE) || defined(SIMAGE_VERSION_1_3)
        SIMAGEWRAPPER_REGISTER_FUNC(simage_resize3d, simage_resize3d_t);
#endif
      }
      else si->simage_resize3d = NULL;

      if (simage_wrapper_versionMatchesAtLeast(1,4,0)) {
#if !defined(HAVE_LIBSIMAGE) || defined(SIMAGE_VERSION_1_4)
        SIMAGEWRAPPER_REGISTER_FUNC(simage_resize3d, simage_resize3d_t);

        SIMAGEWRAPPER_REGISTER_FUNC(s_params_create, s_params_create_t);
        SIMAGEWRAPPER_REGISTER_FUNC(s_params_destroy, s_params_destroy_t);
        SIMAGEWRAPPER_REGISTER_FUNC(s_params_set, s_params_set_t);
        SIMAGEWRAPPER_REGISTER_FUNC(s_params_get, s_params_get_t);
        SIMAGEWRAPPER_REGISTER_FUNC(s_stream_open, s_stream_open_t);
        SIMAGEWRAPPER_REGISTER_FUNC(s_stream_get_buffer, s_stream_get_buffer_t);
        SIMAGEWRAPPER_REGISTER_FUNC(s_stream_close, s_stream_close_t);
        SIMAGEWRAPPER_REGISTER_FUNC(s_stream_destroy, s_stream_destroy_t);
        SIMAGEWRAPPER_REGISTER_FUNC(s_stream_params, s_stream_params_t);
#endif
      }
      else {
#if 0
        /* 20021018 thammer. I might want to use these later instead
           of setting all of them to NULL */
        si->s_params_set = simage_wrapper_s_params_set;
        si->s_params_get = simage_wrapper_s_params_get;
        si->s_stream_open = simage_wrapper_s_stream_open;
        si->s_stream_get_buffer = simage_wrapper_s_stream_get_buffer;
        si->s_stream_close = simage_wrapper_s_stream_close;
        si->s_stream_destroy = simage_wrapper_s_stream_destroy;
        si->s_stream_params = simage_wrapper_s_stream_params;
#endif
        si->s_params_create = NULL;
        si->s_params_destroy = NULL;
        si->s_params_set = NULL;
        si->s_params_get = NULL;
        si->s_stream_open = NULL;
        si->s_stream_get_buffer = NULL;
        si->s_stream_close = NULL;
        si->s_stream_destroy = NULL;
        si->s_stream_params = NULL;
      }
    }
    simage_is_initializing = 0;
  }
  CC_SYNC_END(simage_wrapper);
  return simage_instance;
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
