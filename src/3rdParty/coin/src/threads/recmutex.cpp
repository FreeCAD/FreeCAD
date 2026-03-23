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
  \struct cc_recmutex common.h Inventor/C/threads/common.h
  \ingroup coin_threads
  \brief The structure for the recursive mutex.
*/

/*!
  \typedef struct cc_recmutex cc_recmutex
  \ingroup coin_threads
  \brief The type definition for the recursive mutex structure.
*/

#include <Inventor/C/threads/recmutex.h>

#include <cstdlib>
#include <cassert>

#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/threads/mutex.h>
#include <Inventor/C/threads/thread.h>
#include <Inventor/C/threads/condvar.h>

#include "tidbitsp.h"
#include "threads/recmutexp.h"

/* ********************************************************************** */

/* debugging. for instance useful for checking that there's not
   excessive mutex construction. */

/* these are declared in mutex.cpp */
extern unsigned int cc_debug_mtxcount;
extern const char * COIN_DEBUG_MUTEX_COUNT;

/* ********************************************************************** */

/*!
  \internal
*/
void
cc_recmutex_struct_init(cc_recmutex * recmutex)
{
  cc_mutex_struct_init(&recmutex->mutex);
  cc_condvar_struct_init(&recmutex->condvar);
  
  recmutex->threadid = 0;
  recmutex->level = 0;
  recmutex->waiters = 0;
}

/*!
  \internal
*/
void
cc_recmutex_struct_clean(cc_recmutex * recmutex)
{
  cc_mutex_struct_clean(&recmutex->mutex);
  cc_condvar_struct_clean(&recmutex->condvar);
}

/*! Constructs a recursive mutex. */

cc_recmutex *
cc_recmutex_construct(void)
{
  cc_recmutex * recmutex;
  recmutex = (cc_recmutex *) malloc(sizeof(cc_recmutex));
  assert(recmutex != NULL);
  cc_recmutex_struct_init(recmutex);

  { /* debugging */
    const char * env = coin_getenv(COIN_DEBUG_MUTEX_COUNT);
    if (env && (atoi(env) > 0)) {
      cc_debug_mtxcount += 1;
      (void)fprintf(stderr, "DEBUG: live mutexes +1 => %u (recmutex++)\n",
                    cc_debug_mtxcount);
    }
  }

  return recmutex;
}


/*! Destroys the recursive mutex \a recmutex. */

void
cc_recmutex_destruct(cc_recmutex * recmutex)
{
  { /* debugging */
    const char * env = coin_getenv(COIN_DEBUG_MUTEX_COUNT);
    if (env && (atoi(env) > 0)) {
      assert((cc_debug_mtxcount > 0) && "skewed mutex construct/destruct pairing");
      cc_debug_mtxcount -= 1;
      (void)fprintf(stderr, "DEBUG: live mutexes -1 => %u (recmutex--)\n",
                    cc_debug_mtxcount);
    }
  }

  assert(recmutex != NULL);
  cc_recmutex_struct_clean(recmutex);
  free(recmutex);
}

/*
  Internal function used by cc_recmutex_lock() and cc_recmutex_try_lock().
*/
static int recmutex_lock_internal(cc_recmutex * recmutex, int wait)
{
  int level = -1; /* return -1 for recmutex_try_lock() if we couldn't get the mutex */
  unsigned long id = cc_thread_id();
  
  assert(recmutex != NULL);
  cc_mutex_lock(&recmutex->mutex);
  if (recmutex->level == 0) {
    recmutex->level++;
    recmutex->threadid = id;
    level = recmutex->level;
  }
  else if (id == recmutex->threadid) {
    recmutex->level++;
    level = recmutex->level;
  }
  else if (wait) {
    recmutex->waiters++;
    /* wait in loop, since some thread might snatch the mutex before 
       us when we receive a signal */
    do {
      cc_condvar_wait(&recmutex->condvar, &recmutex->mutex);
    } while (recmutex->level > 0);
    
    assert(recmutex->level == 0);
    recmutex->waiters--;
    recmutex->threadid = id;
    recmutex->level++;
    level = recmutex->level;
  }
  cc_mutex_unlock(&recmutex->mutex);
  return level;  
}

/*! Locks the recursive mutex \a recmutex. Returns the nesting level. */

int
cc_recmutex_lock(cc_recmutex * recmutex)
{
  return recmutex_lock_internal(recmutex, TRUE);
}

/*! Attempts to lock the recursive mutex \a recmutex. Returns TRUE if
    thread got the lock or already had the lock. */
int 
cc_recmutex_try_lock(cc_recmutex * recmutex)
{
  return recmutex_lock_internal(recmutex, FALSE) >= 0;
}

/*! Unlocks the recursive mutex \a recmutex. Returns the nesting level
    after unlocking. */

int
cc_recmutex_unlock(cc_recmutex * recmutex)
{
  int level;
  assert(recmutex != NULL);
  assert(recmutex->threadid == cc_thread_id());
  assert(recmutex->level > 0);
  cc_mutex_lock(&recmutex->mutex);
  recmutex->level--;
  if (recmutex->level == 0 && recmutex->waiters) {
    cc_condvar_wake_one(&recmutex->condvar);
  }
  level = recmutex->level;
  cc_mutex_unlock(&recmutex->mutex);
  return level;
}

/*
  internal functions
*/

static cc_recmutex * recmutex_field_lock;
static cc_recmutex * recmutex_notify_lock;

static void
recmutex_cleanup(void)
{
  cc_recmutex_destruct(recmutex_field_lock);
  cc_recmutex_destruct(recmutex_notify_lock);
}

void 
cc_recmutex_init(void)
{
  recmutex_field_lock = cc_recmutex_construct();
  recmutex_notify_lock = cc_recmutex_construct();
  /* atexit priority makes this callback trigger after normal cleanup
     functions which might still use a recmutex instance */
  coin_atexit((coin_atexit_f*) recmutex_cleanup, CC_ATEXIT_THREADING_SUBSYSTEM);
}

int 
cc_recmutex_internal_field_lock(void)
{
  return cc_recmutex_lock(recmutex_field_lock);
}

int 
cc_recmutex_internal_field_unlock(void)
{
  return cc_recmutex_unlock(recmutex_field_lock);
}

int 
cc_recmutex_internal_notify_lock(void)
{
  return cc_recmutex_lock(recmutex_notify_lock);
}

int 
cc_recmutex_internal_notify_unlock(void)
{
  return cc_recmutex_unlock(recmutex_notify_lock);
}
