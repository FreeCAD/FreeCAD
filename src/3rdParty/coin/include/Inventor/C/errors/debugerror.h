#ifndef CC_DEBUGERROR_H
#define CC_DEBUGERROR_H

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

#include <Inventor/C/basic.h>
#include <Inventor/C/errors/error.h>

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ********************************************************************** */

typedef enum CC_DEBUGERROR_SEVERITY {
  CC_DEBUGERROR_ERROR,
  CC_DEBUGERROR_WARNING,
  CC_DEBUGERROR_INFO
} CC_DEBUGERROR_SEVERITY;

typedef struct cc_debugerror {
  cc_error super; /* make struct is-A cc_error */

  CC_DEBUGERROR_SEVERITY severity;
} cc_debugerror;

typedef void cc_debugerror_cb(const cc_debugerror * err, void * data);

/* ********************************************************************** */

/* FIXME: missing stuff from SoDebugError: type-system,
   COIN_DEBUG_BREAK handling, ... 20020524 mortene. */

/* ********************************************************************** */

COIN_DLL_API void cc_debugerror_post(const char * source, const char * format, ...);
COIN_DLL_API void cc_debugerror_postwarning(const char * source, const char * format, ...);
COIN_DLL_API void cc_debugerror_postinfo(const char * source, const char * format, ...);


COIN_DLL_API void cc_debugerror_init(cc_debugerror * me);
COIN_DLL_API void cc_debugerror_clean(cc_debugerror * me);

COIN_DLL_API CC_DEBUGERROR_SEVERITY cc_debugerror_get_severity(const cc_debugerror * me);

COIN_DLL_API void cc_debugerror_set_handler_callback(cc_debugerror_cb * function, void * data);
COIN_DLL_API cc_debugerror_cb * cc_debugerror_get_handler_callback(void);
COIN_DLL_API void * cc_debugerror_get_handler_data(void);

COIN_DLL_API cc_debugerror_cb * cc_debugerror_get_handler(void ** data);

/* ********************************************************************** */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* ! CC_DEBUGERROR_H */
