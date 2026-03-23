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
  \struct cc_rwmutex common.h Inventor/C/threads/common.h
  \ingroup coin_threads
  \brief The structure for a read-write mutex.
*/

/*!
  \typedef struct cc_rwmutex cc_rwmutex
  \ingroup coin_threads
  \brief The type definition for the read-write mutex structure.
*/

/*!
  \enum cc_precedence {CC_READ_PRECEDENCE, CC_WRITE_PRECEDENCE}
  \ingroup coin_threads
  \brief The precedence values for the read-write mutex.
  \details
  CC_READ_PRECEDENCE is the default.
*/

/*!
  \typedef enum cc_precedence cc_precedence
  \ingroup coin_threads
  \brief The type definition of the cc_precedence enumerator.
*/

#include <Inventor/C/threads/rwmutex.h>

#include <cstdlib>
#include <cassert>

#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/threads/mutex.h>
#include <Inventor/C/threads/condvar.h>

#include "threads/rwmutexp.h"
#include "tidbitsp.h"

/* ********************************************************************** */

/* debugging. for instance useful for checking that there's not
   excessive mutex construction. */

/* these are declared in mutex.cpp */
extern unsigned int cc_debug_mtxcount;
extern const char * COIN_DEBUG_MUTEX_COUNT;

/**************************************************************************/

/*!
  \internal
*/
void
cc_rwmutex_struct_init(cc_rwmutex * rwmutex)
{
  cc_mutex_struct_init(&rwmutex->mutex);
  cc_condvar_struct_init(&rwmutex->read);
  cc_condvar_struct_init(&rwmutex->write);

  rwmutex->readers = 0;
  rwmutex->readwaiters = 0;
  rwmutex->writers = 0;
  rwmutex->writewaiters = 0;
  rwmutex->policy = CC_READ_PRECEDENCE;
}

/*!
  \internal
*/
void
cc_rwmutex_struct_clean(cc_rwmutex * rwmutex)
{
  cc_mutex_struct_clean(&rwmutex->mutex);
  cc_condvar_struct_clean(&rwmutex->read);
  cc_condvar_struct_clean(&rwmutex->write);
}

/*!
  Construct a read-write mutex, with read precedence.
*/

cc_rwmutex *
cc_rwmutex_construct(void)
{
  cc_rwmutex * rwmutex;
  rwmutex = (cc_rwmutex *) malloc(sizeof(cc_rwmutex));
  assert(rwmutex != NULL);
  cc_rwmutex_struct_init(rwmutex);

  { /* debugging */
    const char * env = coin_getenv(COIN_DEBUG_MUTEX_COUNT);
    if (env && (atoi(env) > 0)) {
      cc_debug_mtxcount += 1;
      (void)fprintf(stderr, "DEBUG: live mutexes +1 => %u (rwmutex++)\n",
                    cc_debug_mtxcount);
    }
  }

  return rwmutex;
}

/*!
  Construct a read-write mutex, with read precedence or write precedence.
*/

cc_rwmutex *
cc_rwmutex_construct_etc(enum cc_precedence policy)
{
  cc_rwmutex * rwmutex;
  assert((policy == CC_READ_PRECEDENCE) || (policy == CC_WRITE_PRECEDENCE));
  rwmutex = cc_rwmutex_construct();
  assert(rwmutex != NULL);
  rwmutex->policy = policy;
  return rwmutex;
}

/*!
  Destruct a read-write mutex.
*/

void
cc_rwmutex_destruct(cc_rwmutex * rwmutex)
{
  { /* debugging */
    const char * env = coin_getenv(COIN_DEBUG_MUTEX_COUNT);
    if (env && (atoi(env) > 0)) {
      assert((cc_debug_mtxcount > 0) && "skewed mutex construct/destruct pairing");
      cc_debug_mtxcount -= 1;
      (void)fprintf(stderr, "DEBUG: live mutexes -1 => %u (rwmutex--)\n",
                    cc_debug_mtxcount);
    }
  }

  assert(rwmutex != NULL);
  cc_rwmutex_struct_clean(rwmutex);
  free(rwmutex);
}

/* ********************************************************************** */

/*! Locks the specified \a rwmutex for writing. */
int
cc_rwmutex_write_lock(cc_rwmutex * rwmutex)
{
  (void) cc_mutex_lock(&rwmutex->mutex);
  if (rwmutex->readers == 0 &&
      rwmutex->writers == 0 &&
      rwmutex->readwaiters == 0 &&
      rwmutex->writewaiters == 0) {
    rwmutex->writers++;
    (void) cc_mutex_unlock(&rwmutex->mutex);
    return CC_OK;
  }
  rwmutex->writewaiters++;
  
  /* loop in case some other thread acquires the lock while we wait
     for the signal */
  do {
    (void) cc_condvar_wait(&rwmutex->write, &rwmutex->mutex);
  } while (rwmutex->readers != 0 || rwmutex->writers != 0);  
  rwmutex->writers++;
  rwmutex->writewaiters--;
  assert(rwmutex->writewaiters >= 0);
  (void) cc_mutex_unlock(&rwmutex->mutex);
  return CC_OK;
} /* cc_rwmutex_write_lock() */

/*! Check whether the given \a rwmutex* is available for write locking. */

int
cc_rwmutex_write_try_lock(cc_rwmutex * rwmutex)
{
  (void) cc_mutex_lock(&rwmutex->mutex);
  if (rwmutex->readers == 0 &&
      rwmutex->writers == 0 &&
      rwmutex->readwaiters == 0 &&
      rwmutex->writewaiters == 0) {
    rwmutex->writers++;
    (void) cc_mutex_unlock(&rwmutex->mutex);
    return CC_OK;
  }
  (void) cc_mutex_unlock(&rwmutex->mutex);
  return CC_BUSY;
} /* cc_rwmutex_write_try_lock() */

/*! Unlock the write lock on the given \a rwmutex. */

int
cc_rwmutex_write_unlock(cc_rwmutex * rwmutex)
{
  int rwait;
  int wwait;
  (void) cc_mutex_lock(&rwmutex->mutex);
  rwmutex->writers--;
  assert(rwmutex->writers >= 0);
  rwait = rwmutex->readwaiters;
  wwait = rwmutex->writewaiters;

  if (rwmutex->policy == CC_READ_PRECEDENCE) {
    if (rwait) cc_condvar_wake_all(&rwmutex->read);
    else cc_condvar_wake_one(&rwmutex->write);
  } 
  else {
    if (wwait) cc_condvar_wake_one(&rwmutex->write);
    else cc_condvar_wake_all(&rwmutex->read);
  }
  (void) cc_mutex_unlock(&rwmutex->mutex);
  return CC_OK;
}

/*! Locks the specified \a rwmutex for reading. */
int
cc_rwmutex_read_lock(cc_rwmutex * rwmutex)
{
  assert(rwmutex != NULL);
  (void) cc_mutex_lock(&rwmutex->mutex);
  if (rwmutex->writers == 0) {
    rwmutex->readers++;
    (void) cc_mutex_unlock(&rwmutex->mutex);
    return CC_OK;
  }
  rwmutex->readwaiters++;

  /* loop in case some other thread acquires the lock while we wait
     for the signal */
  do {
    (void) cc_condvar_wait(&rwmutex->read, &rwmutex->mutex);
  } while (rwmutex->writers != 0);

  rwmutex->readers++;
  rwmutex->readwaiters--;
  assert(rwmutex->readwaiters >= 0);
  (void) cc_mutex_unlock(&rwmutex->mutex);
  return CC_OK;
}

/*! Check whether the given \a rwmutex* is available for read locking. */

int
cc_rwmutex_read_try_lock(cc_rwmutex * rwmutex)
{
  assert(rwmutex != NULL);

  (void) cc_mutex_lock(&rwmutex->mutex);
  if (rwmutex->writers == 0 &&
      rwmutex->writewaiters == 0) {
    rwmutex->readers++;
    (void) cc_mutex_unlock(&rwmutex->mutex);
    return CC_OK;
  }
  (void) cc_mutex_unlock(&rwmutex->mutex);
  return CC_BUSY;
}

/*! Unlock the read lock on the given \a rwmutex. */

int
cc_rwmutex_read_unlock(cc_rwmutex * rwmutex)
{
  int rwait;
  int wwait;
  int readers;
  (void) cc_mutex_lock(&rwmutex->mutex);
  rwmutex->readers--;
  readers = rwmutex->readers;
  assert(readers >= 0);
  rwait = rwmutex->readwaiters;
  wwait = rwmutex->writewaiters;

  if (rwmutex->policy == CC_READ_PRECEDENCE || readers) {
    if (rwait) cc_condvar_wake_all(&rwmutex->read);
    else if (!readers) cc_condvar_wake_one(&rwmutex->write);
  }
  else {
    if (wwait) cc_condvar_wake_one(&rwmutex->write);
    else cc_condvar_wake_all(&rwmutex->read);
  }
  (void) cc_mutex_unlock(&rwmutex->mutex);
  return CC_OK;
} /* cc_rwmutex_read_unlock() */

