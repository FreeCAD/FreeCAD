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

/*!
  \struct cc_error error.h Inventor/C/errors/error.h
  \typedef struct cc_error cc_error
  \brief The cc_error type is an internal Coin structure for error management.

  \ingroup coin_errors

  This is a Coin extension.
*/

/*!
  \var cc_error::debugstring

  The error message.
*/

/*!
  \typedef void cc_error_cb(const cc_error * err, void * data)

  The definition for an error callback handler.
*/

#include <Inventor/C/errors/error.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <cstdio>
#include <cassert>
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* STDERR_FILENO */
#endif /* HAVE_UNISTD_H */

#ifdef COIN_THREADSAFE
#include <Inventor/C/threads/mutex.h>
#include "threads/mutexp.h"
#endif /* COIN_THREADSAFE */

#include "coindefs.h"
#include "tidbitsp.h"

/* ********************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef COIN_THREADSAFE
static cc_mutex * cc_error_mutex = NULL;
static void cc_error_mutex_cleanup(void) {
  if (cc_error_mutex) {
    cc_mutex_destruct(cc_error_mutex);
    cc_error_mutex = NULL;
  }
}
#endif /* COIN_THREADSAFE */

/* FIXME: should be hidden from public API, and only visible to
   subclasses. 20020526 mortene. */
/*!
  \relates cc_error
*/

void
cc_error_default_handler_cb(const cc_error * err, void * COIN_UNUSED_ARG(data))
{
  /* It is not possible to "pass" C library data from the application
     to a MSWin .DLL, so this is necessary to get hold of the stderr
     FILE*. Just using fprintf(stderr, ...) or fprintf(stdout, ...)
     directly will result in a crash when Coin has been compiled as a
     .DLL. */
  FILE * coin_stderr = coin_get_stderr();

  if (coin_stderr) {
    const cc_string * str = cc_error_get_debug_string(err);
    (void)fprintf(coin_stderr, "%s\n", cc_string_get_text(str));
    (void)fflush(coin_stderr);
  }
}

static cc_error_cb * cc_error_callback = cc_error_default_handler_cb;
static void * cc_error_callback_data = NULL;
static SbBool cc_error_cleanup_function_set = FALSE;

static void
cc_error_cleanup(void)
{
  cc_error_callback = cc_error_default_handler_cb;
  cc_error_callback_data = NULL;
  cc_error_cleanup_function_set = FALSE;
}

/*!
  \relates cc_error
*/

void
cc_error_init(cc_error * me)
{
  cc_string_construct(&(me->debugstring));
}

/*!
  \relates cc_error
*/

void
cc_error_clean(cc_error * me)
{
  cc_string_clean(&(me->debugstring));
}

/*!
  \relates cc_error
*/

void
cc_error_copy(const cc_error * src, cc_error * dst)
{
  cc_string_set_string(&dst->debugstring, &src->debugstring);
}

/*!
  \relates cc_error
*/

void
cc_error_set_debug_string(cc_error * me, const char * str)
{
  cc_string_set_text(&(me->debugstring), str);
}

/*!
  \relates cc_error
*/

void
cc_error_append_to_debug_string(cc_error * me, const char * str)
{
  cc_string_append_text(&(me->debugstring), str);
}

/*!
  \relates cc_error
*/

void
cc_error_handle(cc_error * me)
{
  void * arg = NULL;

  cc_error_cb * function = cc_error_get_handler(&arg);
  assert(function != NULL);

#ifdef COIN_THREADSAFE
  if (!cc_error_mutex) {
    /* extra locking to avoid that two threads create the mutex */
    /* FIXME: this is not smart, as it means that if the first call to
       e.g. SoDebugError::post*() will hang if it happens within a
       region where the global lock is already taken. 20050708 mortene.*/
    cc_mutex_global_lock();
    if (cc_error_mutex == NULL) {
      cc_error_mutex = cc_mutex_construct();
      coin_atexit(cc_error_mutex_cleanup, CC_ATEXIT_MSG_SUBSYSTEM);
    }
    cc_mutex_global_unlock();
  }
  cc_mutex_lock(cc_error_mutex);
#endif /* COIN_THREADSAFE */

  (*function)(me, arg);

#ifdef COIN_THREADSAFE
  cc_mutex_unlock(cc_error_mutex);
#endif /* COIN_THREADSAFE */
}

/*!
  \relates cc_error
*/

void
cc_error_set_handler_callback(cc_error_cb * func, void * data)
{
  cc_error_callback = func;
  cc_error_callback_data = data;

  if (!cc_error_cleanup_function_set) {
    coin_atexit((coin_atexit_f*) cc_error_cleanup, CC_ATEXIT_MSG_SUBSYSTEM);
    cc_error_cleanup_function_set = TRUE;
  }
}

/*!
  \relates cc_error
*/

cc_error_cb *
cc_error_get_handler_callback(void)
{
  return cc_error_callback;
}

/*!
  \relates cc_error
*/

void *
cc_error_get_handler_data(void)
{
  return cc_error_callback_data;
}

/*!
  \relates cc_error
*/

cc_error_cb *
cc_error_get_handler(void ** data)
{
  *data = cc_error_callback_data;
  return cc_error_callback;
}

/*!
  \relates cc_error
*/

const cc_string *
cc_error_get_debug_string(const cc_error * me)
{
  return &(me->debugstring);
}

/*!
  \relates cc_error
*/

void
cc_error_post_arglist(const char * format, va_list args)
{
  cc_string s;
  cc_error err;

  cc_string_construct(&s);

  cc_string_vsprintf(&s, format, args);

  cc_error_init(&err);
  cc_error_set_debug_string(&err, cc_string_get_text(&s));
  cc_error_handle(&err);
  cc_error_clean(&err);

  cc_string_clean(&s);
}

/*!
  \relates cc_error
*/

void
cc_error_post(const char * format, ...)
{
  va_list argptr;
  va_start(argptr, format);
  cc_error_post_arglist(format, argptr);
  va_end(argptr);
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
