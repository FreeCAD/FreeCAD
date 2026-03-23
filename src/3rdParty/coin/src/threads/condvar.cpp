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
  \struct cc_condvar common.h Inventor/C/threads/common.h
  \ingroup coin_threads
  \brief The structure for a conditional variable.
*/

/*!
  \typedef struct cc_condvar cc_condvar
  \ingroup coin_threads
  \brief The type definition for the conditional variable structure.
*/

#include <Inventor/C/threads/condvar.h>

#include <cstdlib>
#include <cassert>
#include <cerrno>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <Inventor/C/threads/mutex.h>
#include <Inventor/C/errors/debugerror.h>

#include "threads/condvarp.h"

/* ********************************************************************** */
#ifdef USE_PTHREAD
#include "condvar_pthread.icc"
#endif /* USE_PTHREAD */

#ifdef USE_W32THREAD
#include "condvar_win32.icc"
#endif /* USE_W32THREAD */

/*
  \internal
*/

void
cc_condvar_struct_init(cc_condvar * condvar_struct)
{
  int ok;
  ok = internal_condvar_struct_init(condvar_struct);
  assert(ok == CC_OK);
}

/*
  \internal
*/
void
cc_condvar_struct_clean(cc_condvar * condvar_struct)
{
  int ok;
  assert(condvar_struct != NULL);
  ok = internal_condvar_struct_clean(condvar_struct);
  assert(ok == CC_OK);
}

/* ********************************************************************** */

/*! Constructs a conditional variable. */

cc_condvar *
cc_condvar_construct(void)
{
  cc_condvar * condvar;
  condvar = (cc_condvar *) malloc(sizeof(cc_condvar));
  assert(condvar != NULL);
  cc_condvar_struct_init(condvar);
  return condvar;
}

/*! Destroys the given conditional variable \a condvar. */

void
cc_condvar_destruct(cc_condvar * condvar)
{
  assert(condvar != NULL);
  cc_condvar_struct_clean(condvar);
  free(condvar);
}

/*! Wait indefinitely for the \a condvar conditional variable
    using the specified \a mutex lock. */

int
cc_condvar_wait(cc_condvar * condvar, cc_mutex * mutex)
{
  int ok;
  assert(condvar != NULL);
  ok = internal_condvar_wait(condvar, mutex);
  assert(ok == CC_OK);
  return ok;
}

/*! Wait for no more than the \a period for the \a condvar
    conditional variable using the specified \a mutex lock. */

int
cc_condvar_timed_wait(cc_condvar * condvar,
                      cc_mutex * mutex,
                      cc_time period)
{
  int ret;
  assert(condvar != NULL);
  ret = internal_condvar_timed_wait(condvar, mutex, period);
  assert(ret == CC_OK || ret == CC_TIMEOUT);
  return ret;
}

/*! Wake one thread waiting for the \a condvar conditional variable. */

void
cc_condvar_wake_one(cc_condvar * condvar)
{
  int ok;
  assert(condvar != NULL);
  ok = internal_condvar_wake_one(condvar);
  assert(ok == CC_OK);
}

/*! Wake all threads waiting for the \a condvar conditional variable. */

void
cc_condvar_wake_all(cc_condvar * condvar)
{
  int ok;
  assert(condvar != NULL);

  ok = internal_condvar_wake_all(condvar);
  assert(ok == CC_OK);
}

