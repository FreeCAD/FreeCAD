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

#include "glue/openal_wrapper.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#else /* No config.h? Hmm. Assume the openal library is not available for linking. */
#define OPENALWRAPPER_ASSUME_OPENAL 0
#endif /* !HAVE_CONFIG_H */

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <Inventor/C/basic.h>
#include <Inventor/C/glue/dl.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/C/errors/debugerror.h>

#include "threads/threadsutilp.h"
#include "tidbitsp.h"

#ifdef HAVE_OPENAL /* In case we're _not_ doing runtime linking. */
#define OPENALWRAPPER_ASSUME_OPENAL 1
#endif /* HAVE_OPENAL */

#if OPENALWRAPPER_ASSUME_OPENAL
#if defined HAVE_AL_AL_H
#include <AL/al.h>
#include <AL/alc.h>
#elif defined HAVE_OPENAL_AL_H
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif
#endif /* OPENALWRAPPER_ASSUME_OPENAL */

static openal_wrapper_t * openal_instance = NULL;
static cc_libhandle openal_libhandle = NULL;
static int openal_failed_to_load = 0;
static int openal_is_initializing = 0;

/* Cleans up at exit. */
static void
openal_wrapper_cleanup(void)
{
#ifdef OPENAL_RUNTIME_LINKING
  if (openal_libhandle) {
    cc_dl_close(openal_libhandle);
    openal_libhandle = NULL;
  }
#endif /* OPENAL_RUNTIME_LINKING */

  assert(openal_instance);
  free(openal_instance);
  openal_instance = NULL;
  openal_failed_to_load = 0;
  openal_is_initializing = 0;
}

/* Implemented by using the singleton pattern. */
const openal_wrapper_t *
openal_wrapper(void)
{
  CC_SYNC_BEGIN(openal_wrapper);

  if (!openal_instance && !openal_failed_to_load) {

    /* First invocation, do initializations. */
    openal_wrapper_t * oal = (openal_wrapper_t *)malloc(sizeof(openal_wrapper_t));
    /* FIXME: Verify that atexit works when Coin is used in an ActiveX
       dll also. 2003-01-27 thammer. */
    coin_atexit((coin_atexit_f *)openal_wrapper_cleanup, CC_ATEXIT_DYNLIBS);

    /* Detect recursive calls. */
    {
      assert(openal_is_initializing == 0);
      openal_is_initializing = 1;
    }

    /* The common case is that simage is either available from the
       linking process or we're successfully going to link it in. */
    oal->available = 1;

#ifdef OPENAL_RUNTIME_LINKING
    {
      int idx;
      const char * possiblelibnames[] = {
        NULL, /* is set below */
        "openal", "openal32", "libopenal.so", "libopenal.dylib",
        NULL
      };

      possiblelibnames[0] = coin_getenv("COIN_OPENAL_LIBNAME");
      idx = possiblelibnames[0] ? 0 : 1;

      while (!openal_libhandle && possiblelibnames[idx]) {
        openal_libhandle = cc_dl_open(possiblelibnames[idx]);
        idx++;
      }

      if (!openal_libhandle) {
        oal->available = 0;
        openal_failed_to_load = 1;
      }
      oal->runtime = 1;
    }
    /* Define OPENALWRAPPER_REGISTER_FUNC macro. Casting the type is
       necessary for this file to be compatible with C++ compilers. */
#define OPENALWRAPPER_REGISTER_FUNC(_funcname_, _funcsig_) \
    oal->_funcname_ = (_funcsig_)cc_dl_sym(openal_libhandle, SO__QUOTE(_funcname_))

#elif defined(OPENALWRAPPER_ASSUME_OPENAL) /* !OPENAL_RUNTIME_LINKING */

    oal->runtime = 0;
#define OPENALWRAPPER_REGISTER_FUNC(_funcname_, _funcsig_) \
    oal->_funcname_ = (_funcsig_)_funcname_

#else /* !OPENALWRAPPER_ASSUME_OPENAL */
    oal->available = 0;
    oal->runtime = 0;
#define OPENALWRAPPER_REGISTER_FUNC(_funcname_, _funcsig_) \
    oal->_funcname_ = NULL

#endif /* !OPENALWRAPPER_ASSUME_OPENAL */

    OPENALWRAPPER_REGISTER_FUNC(alGetString, alGetString_t);

    if (oal->available && !oal->alGetString) {
      /* something is seriously wrong */
      cc_debugerror_post("openal_wrapper",
                         "Loaded OpenAL DLL ok, but couldn't resolve symbol "
                         "alGetString().");

      oal->available = 0;
      openal_failed_to_load = 1;
    }
    else {
      OPENALWRAPPER_REGISTER_FUNC(alGetString, alGetString_t);
      OPENALWRAPPER_REGISTER_FUNC(alGetError, alGetError_t);
      OPENALWRAPPER_REGISTER_FUNC(alListenerfv, alListenerfv_t);
      OPENALWRAPPER_REGISTER_FUNC(alListenerf, alListenerf_t);
      OPENALWRAPPER_REGISTER_FUNC(alDistanceModel, alDistanceModel_t);

      OPENALWRAPPER_REGISTER_FUNC(alcCreateContext, alcCreateContext_t);
      OPENALWRAPPER_REGISTER_FUNC(alcMakeContextCurrent, alcMakeContextCurrent_t);
      OPENALWRAPPER_REGISTER_FUNC(alcProcessContext, alcProcessContext_t);
      OPENALWRAPPER_REGISTER_FUNC(alcSuspendContext, alcSuspendContext_t);
      OPENALWRAPPER_REGISTER_FUNC(alcDestroyContext, alcDestroyContext_t);
      OPENALWRAPPER_REGISTER_FUNC(alcOpenDevice, alcOpenDevice_t);
      OPENALWRAPPER_REGISTER_FUNC(alcCloseDevice, alcCloseDevice_t);

      OPENALWRAPPER_REGISTER_FUNC(alGenSources, alGenSources_t);
      OPENALWRAPPER_REGISTER_FUNC(alDeleteSources, alDeleteSources_t);
      OPENALWRAPPER_REGISTER_FUNC(alSourcePlay, alSourcePlay_t);
      OPENALWRAPPER_REGISTER_FUNC(alSourceStop, alSourceStop_t);
      OPENALWRAPPER_REGISTER_FUNC(alSourceRewind, alSourceRewind_t);
      OPENALWRAPPER_REGISTER_FUNC(alSourcefv, alSourcefv_t);
      OPENALWRAPPER_REGISTER_FUNC(alSourcef, alSourcef_t);
      OPENALWRAPPER_REGISTER_FUNC(alSourcei, alSourcei_t);
      OPENALWRAPPER_REGISTER_FUNC(alGetSourcei, alGetSourcei_t);
      OPENALWRAPPER_REGISTER_FUNC(alSourceQueueBuffers, alSourceQueueBuffers_t);
      OPENALWRAPPER_REGISTER_FUNC(alSourceUnqueueBuffers, alSourceUnqueueBuffers_t);
      OPENALWRAPPER_REGISTER_FUNC(alBufferData, alBufferData_t);
      OPENALWRAPPER_REGISTER_FUNC(alGenBuffers, alGenBuffers_t);
      OPENALWRAPPER_REGISTER_FUNC(alDeleteBuffers, alDeleteBuffers_t);
    }

    if (oal->available) { /* debugging */
      const unsigned char * str;
      const char * debug = coin_getenv("COIN_DEBUG_AUDIO");
      if (debug && atoi(debug)) {
        cc_debugerror_postinfo("openal_wrapper", "OpenAL using %s linking.", 
                               oal->runtime ? "runtime" : "build-time");
        
        str = oal->alGetString(AL_VENDOR);
        cc_debugerror_postinfo("openal_wrapper", "AL_VENDOR=='%s'", str);
        str = oal->alGetString(AL_VERSION);
        cc_debugerror_postinfo("openal_wrapper", "AL_VERSION=='%s'", str);
        str = oal->alGetString(AL_RENDERER);
        cc_debugerror_postinfo("openal_wrapper", "AL_RENDERER=='%s'", str);
        str = oal->alGetString(AL_EXTENSIONS);
        cc_debugerror_postinfo("openal_wrapper", "AL_EXTENSIONS=='%s'", str);
      }
    }

    /* Do this late, so we can detect recursive calls to this function. */
    openal_instance = oal;
  }
  CC_SYNC_END(openal_wrapper);
  return openal_instance;
}
