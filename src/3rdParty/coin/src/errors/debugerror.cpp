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

#include <Inventor/C/errors/debugerror.h>

#include <cstdlib>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "tidbitsp.h"

static cc_debugerror_cb * dbgerr_callback =
  reinterpret_cast<cc_debugerror_cb *>(cc_error_default_handler_cb);
static void * dbgerr_callback_data = NULL;
static SbBool dbgerr_cleanup_function_set = FALSE;

/*!
  \struct cc_debugerror debugerror.h Inventor/C/errors/debugerror.h
  \typedef struct cc_debugerror cc_debugerror
  \brief The cc_debugerror type is an internal Coin structure for debug error management.

  \ingroup coin_errors

  This is a Coin extension.
*/

/*!
  \var cc_debugerror::super

  The data from the parent cc_error class.
  This has the advantage that the cc_debugerror structure can appear to be a cc_error.
*/

/*!
  \var cc_debugerror::severity

  The severity level of the debug error.
*/

/*!
  \enum CC_DEBUGERROR_SEVERITY
  \typedef enum CC_DEBUGERROR_SEVERITY CC_DEBUGERROR_SEVERITY

  Specifies the available severity levels of the debug messages.
*/

/*!
  \var CC_DEBUGERROR_SEVERITY::CC_DEBUGERROR_ERROR

  An actual error.
 */

/*!
  \var CC_DEBUGERROR_SEVERITY::CC_DEBUGERROR_WARNING

  Just a warning.
 */

/*!
  \var CC_DEBUGERROR_SEVERITY::CC_DEBUGERROR_INFO

  For information only.
 */

/*!
  \typedef void cc_debugerror_cb(const cc_debugerror * err, void * data)

  The definition for a debug error callback handler.
*/

extern "C" {

static void
debugerror_cleanup(void)
{
  dbgerr_callback = reinterpret_cast<cc_debugerror_cb *>(cc_error_default_handler_cb);
  dbgerr_callback_data = NULL;
  dbgerr_cleanup_function_set = FALSE;
}

} // extern "C"

/*!
  \relates cc_debugerror
*/

void
cc_debugerror_init(cc_debugerror * me)
{
  cc_error_init(reinterpret_cast<cc_error *>(me));
}

/*!
  \relates cc_debugerror
*/

void
cc_debugerror_clean(cc_debugerror * me)
{
  cc_error_clean(reinterpret_cast<cc_error *>(me));
}

/*!
  \relates cc_debugerror
*/

CC_DEBUGERROR_SEVERITY
cc_debugerror_get_severity(const cc_debugerror * me)
{
  return me->severity;
}

/*!
  \relates cc_debugerror
*/

void
cc_debugerror_set_handler_callback(cc_debugerror_cb * function, void * data)
{
  dbgerr_callback = function;
  dbgerr_callback_data = data;

  if (!dbgerr_cleanup_function_set) {
    coin_atexit(debugerror_cleanup, CC_ATEXIT_MSG_SUBSYSTEM);
    dbgerr_cleanup_function_set = TRUE;
  }
}

/*!
  \relates cc_debugerror
*/

cc_debugerror_cb *
cc_debugerror_get_handler_callback(void)
{
  return dbgerr_callback;
}

/*!
  \relates cc_debugerror
*/

void *
cc_debugerror_get_handler_data(void)
{
  return dbgerr_callback_data;
}

/*!
  \relates cc_debugerror
*/

cc_debugerror_cb *
cc_debugerror_get_handler(void ** data)
{
  *data = dbgerr_callback_data;
  return dbgerr_callback;
}

/*!
  \relates cc_debugerror
*/

static void
cc_debugerror_internal_post(const char * source, cc_string * msg,
                            CC_DEBUGERROR_SEVERITY sev, const char * type)
{
  cc_debugerror deberr;

  cc_debugerror_init(&deberr);

  deberr.severity = sev;
  cc_error_set_debug_string(reinterpret_cast<cc_error *>(&deberr), "Coin ");
  cc_error_append_to_debug_string(reinterpret_cast<cc_error *>(&deberr), type);
  cc_error_append_to_debug_string(reinterpret_cast<cc_error *>(&deberr), " in ");
  cc_error_append_to_debug_string(reinterpret_cast<cc_error *>(&deberr), source);
  cc_error_append_to_debug_string(reinterpret_cast<cc_error *>(&deberr), "(): ");
  cc_error_append_to_debug_string(reinterpret_cast<cc_error *>(&deberr), cc_string_get_text(msg));

  if (dbgerr_callback != reinterpret_cast<cc_debugerror_cb *>(cc_error_default_handler_cb)) {
    dbgerr_callback(&deberr, dbgerr_callback_data);
  }
  else {
    cc_error_handle(reinterpret_cast<cc_error *>(&deberr));
  }

  /* FIXME: port to C. 20020524 mortene. */
  /* check_breakpoints(source);*/

  cc_debugerror_clean(&deberr);
}

/*!
  A macro to simplify posting of debug error messages
*/

#define CC_DEBUGERROR_POST(SEVERITY, TYPE) \
  cc_string s; \
  va_list args; \
 \
  va_start(args, format); \
  cc_string_construct(&s); \
  cc_string_vsprintf(&s, format, args); \
  va_end(args); \
 \
  cc_debugerror_internal_post(source, &s, SEVERITY, TYPE); \
  cc_string_clean(&s)

/*!
  \relates cc_debugerror
*/

void
cc_debugerror_post(const char * source, const char * format, ...)
{
  CC_DEBUGERROR_POST(CC_DEBUGERROR_ERROR, "error");
}

/*!
  \relates cc_debugerror
*/

void
cc_debugerror_postwarning(const char * source, const char * format, ...)
{
  CC_DEBUGERROR_POST(CC_DEBUGERROR_WARNING, "warning");
}

/*!
  \relates cc_debugerror
*/

void
cc_debugerror_postinfo(const char * source, const char * format, ...)
{
  CC_DEBUGERROR_POST(CC_DEBUGERROR_INFO, "info");
}

#undef CC_DEBUGERROR_POST
