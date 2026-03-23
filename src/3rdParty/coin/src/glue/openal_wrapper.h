#ifndef COIN_OPENALWRAPPER_H
#define COIN_OPENALWRAPPER_H

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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif /* HAVE_WINDOWS_H */

/* Note: Under Win32, we need to make sure we use the correct calling
   method by using the OPENALWRAPPER_APIENTRY define for the function
   signature types (or else we'll get weird stack errors). The calling
   convention must match the calling convention used in the
   OpenAL32.dll we link against. Both the LGPL version of OpenAL in
   CVS (www.openal.org), Creative Lab's (binary) SDK, and nVidia's
   AudioSDK, uses this calling method, so I think it is quite safe to
   assume that all OpenAL32.dlls use this method.  On other platforms,
   just define OPENALWRAPPER_APIENTRY empty. Stack errors (under Win32)
   can be detected by specifying the /GZ linker option. 
   2003-03-19 thammer. */

#ifdef _WIN32
  #define OPENALWRAPPER_APIENTRY __cdecl
#else
  #define OPENALWRAPPER_APIENTRY
#endif /* _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Typedefinitions of function signatures for openal calls we use. We
   need these for casting from the void-pointer return of dlsym().*/

  typedef const unsigned char * (OPENALWRAPPER_APIENTRY *alGetString_t)(int param);
  typedef int (OPENALWRAPPER_APIENTRY *alGetError_t)(void);

  typedef void (OPENALWRAPPER_APIENTRY *alListenerfv_t)(int pname, float *param);
  typedef void (OPENALWRAPPER_APIENTRY *alListenerf_t)(int pname, float param);

  typedef void (OPENALWRAPPER_APIENTRY *alDistanceModel_t)(int distanceModel);

  typedef void (OPENALWRAPPER_APIENTRY *alGenSources_t)(int n, unsigned int *sources);
  typedef void (OPENALWRAPPER_APIENTRY *alDeleteSources_t)(int n, unsigned int *sources);
  typedef void (OPENALWRAPPER_APIENTRY *alSourcePlay_t)(unsigned int source);
  typedef void (OPENALWRAPPER_APIENTRY *alSourceStop_t)(unsigned int source);
  typedef void (OPENALWRAPPER_APIENTRY *alSourceRewind_t)(unsigned int source);
  typedef void (OPENALWRAPPER_APIENTRY *alSourcefv_t)(unsigned int source, int param, 
                                        float *values);
  typedef void (OPENALWRAPPER_APIENTRY *alSourcef_t)(unsigned int source, int param, 
                                       float value);
  typedef void (OPENALWRAPPER_APIENTRY *alSourcei_t)(unsigned int source, int param, 
                                       int value);
  typedef void (OPENALWRAPPER_APIENTRY *alGetSourcei_t)(unsigned int source, int param, 
                                          int *value);
  typedef void (OPENALWRAPPER_APIENTRY *alSourceQueueBuffers_t)(unsigned int source, 
                                                  unsigned int n, 
                                                  unsigned int *buffers);
  typedef void (OPENALWRAPPER_APIENTRY *alSourceUnqueueBuffers_t)(unsigned int source, 
                                                  unsigned int n, 
                                                  unsigned int *buffers);

  typedef void (OPENALWRAPPER_APIENTRY *alBufferData_t)(unsigned int buffer, int format,
                                       void *data, unsigned int size,
                                       unsigned int freq);
  typedef void (OPENALWRAPPER_APIENTRY *alGenBuffers_t)(int n, unsigned int *buffers);
  typedef void (OPENALWRAPPER_APIENTRY *alDeleteBuffers_t)(int n, unsigned int *buffers);

  typedef void * (OPENALWRAPPER_APIENTRY *alcCreateContext_t)(void *device, int *attrlist);
  typedef int (OPENALWRAPPER_APIENTRY *alcMakeContextCurrent_t)(void *context);
  typedef void (OPENALWRAPPER_APIENTRY *alcProcessContext_t)(void *context);
  typedef void (OPENALWRAPPER_APIENTRY *alcSuspendContext_t)(void *context);
  typedef void (OPENALWRAPPER_APIENTRY *alcDestroyContext_t)(void *context);

  typedef void * (OPENALWRAPPER_APIENTRY *alcOpenDevice_t)(unsigned char *deviceName);
  typedef void (OPENALWRAPPER_APIENTRY *alcCloseDevice_t)(void *device);

#if !OPENALWRAPPER_ASSUME_OPENAL
  enum {
    AL_NO_ERROR=0x0000,
    AL_NONE=0x0000,
    AL_PITCH=0x1003,
    AL_POSITION=0x1004,
    AL_VELOCITY=0x1006,
    AL_LOOPING=0x1007,
    AL_GAIN=0x100a,
    AL_MIN_GAIN=0x100d,
    AL_MAX_GAIN=0x100e,
    AL_ORIENTATION=0x100f,
    AL_BUFFER=0x1009,
    AL_SOURCE_STATE=0x1010,
    AL_INITIAL=0x1011,
    AL_PLAYING=0x1012,
    AL_PAUSED=0x1013,
    AL_STOPPED=0x1014,
    AL_BUFFERS_QUEUED=0x1015,
    AL_BUFFERS_PROCESSED=0x1016,
    AL_ROLLOFF_FACTOR=0x1021,
    AL_FORMAT_MONO8=0x1100,
    AL_FORMAT_MONO16=0x1101,
    AL_FORMAT_STEREO8=0x1102,
    AL_FORMAT_STEREO16=0x1103,
    AL_INVALID_NAME=0xa001,
    AL_INVALID_ENUM=0xa002,
    AL_INVALID_VALUE=0xa003,
    AL_INVALID_OPERATION=0xa004,
    AL_OUT_OF_MEMORY=0xa005,
    AL_VENDOR=0xb001,
    AL_VERSION=0xb002,
    AL_RENDERER=0xb003,
    AL_EXTENSIONS=0xb004,
    AL_INVERSE_DISTANCE_CLAMPED=0xd002
  };
#endif /* !OPENALWRAPPER_ASSUME_OPENAL */

  typedef struct {
    int available;
    int runtime;

    alGetString_t alGetString;
    alGetError_t alGetError;
    alListenerfv_t alListenerfv;
    alListenerf_t alListenerf;
    alDistanceModel_t alDistanceModel;
    alGenSources_t alGenSources;
    alDeleteSources_t alDeleteSources;
    alSourcePlay_t alSourcePlay;
    alSourceStop_t alSourceStop;
    alSourceRewind_t alSourceRewind;
    alSourcefv_t alSourcefv;
    alSourcef_t alSourcef;
    alSourcei_t alSourcei;
    alGetSourcei_t alGetSourcei;
    alSourceQueueBuffers_t alSourceQueueBuffers;
    alSourceUnqueueBuffers_t alSourceUnqueueBuffers;
    alBufferData_t alBufferData;
    alGenBuffers_t alGenBuffers;
    alDeleteBuffers_t alDeleteBuffers;

    alcCreateContext_t alcCreateContext;
    alcMakeContextCurrent_t alcMakeContextCurrent;
    alcProcessContext_t alcProcessContext;
    alcSuspendContext_t alcSuspendContext;
    alcDestroyContext_t alcDestroyContext;
    alcOpenDevice_t alcOpenDevice;
    alcCloseDevice_t alcCloseDevice;

  } openal_wrapper_t;

  const openal_wrapper_t * openal_wrapper(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COIN_OPENALWRAPPER_H */
