#ifndef CC_ERROR_H
#define CC_ERROR_H

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

/* FIXME: missing stuff from SoError: type-system. 20020526 mortene. */

/* ********************************************************************** */

#include <Inventor/C/base/string.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ********************************************************************** */

typedef struct cc_error {
  cc_string debugstring;
} cc_error;

typedef void cc_error_cb(const cc_error * err, void * data);

/* ********************************************************************** */

COIN_DLL_API void cc_error_init(cc_error * me);
COIN_DLL_API void cc_error_clean(cc_error * me);
COIN_DLL_API void cc_error_copy(const cc_error * src, cc_error * dst);

  /*   const SbString & getDebugString(void) const; */
COIN_DLL_API const cc_string * cc_error_get_debug_string(const cc_error * me);

/*   static void setHandlerCallback(SoErrorCB * const func, void * const data); */

COIN_DLL_API void cc_error_set_handler_callback(cc_error_cb * func, void * data);

/*   static SoErrorCB * getHandlerCallback(void); */

COIN_DLL_API cc_error_cb * cc_error_get_handler_callback(void);

/*   static void * getHandlerData(void); */

COIN_DLL_API void * cc_error_get_handler_data(void);

/*   static void post(const char * const format, ...); */
COIN_DLL_API void cc_error_post(const char * format, ...);
COIN_DLL_API void cc_error_post_arglist(const char * format, va_list args);

  /* protected: */
  /*   void setDebugString(const char * const str); */

COIN_DLL_API void cc_error_set_debug_string(cc_error * me, const char * str);

  /* protected: */
  /*   void appendToDebugString(const char * const str); */

COIN_DLL_API void cc_error_append_to_debug_string(cc_error * me, const char * str);

  /* protected: */
  /*   void handleError(void); */

COIN_DLL_API void cc_error_handle(cc_error * me);

  /* protected: */
  /*   virtual SoErrorCB * getHandler(void * & data) const; */
COIN_DLL_API cc_error_cb * cc_error_get_handler(void ** data);

  /* protected: */
  /*   static void defaultHandlerCB(const SoError * error, void * userdata); */
COIN_DLL_API void cc_error_default_handler_cb(const cc_error * err, void * data);

/* ********************************************************************** */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !CC_ERROR_H */
