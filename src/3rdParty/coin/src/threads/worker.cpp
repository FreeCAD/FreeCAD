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
  \struct cc_worker common.h Inventor/C/threads/common.h
  \ingroup coin_threads
  \brief The structure for a worker thread.
*/

/*!
  \typedef struct cc_worker cc_worker
  \ingroup coin_threads
  \brief The type definition for the worker thread structure.
*/

#include <Inventor/C/threads/worker.h>

#include <cstdlib>
#include <cassert>

#include <Inventor/C/threads/thread.h>
#include <Inventor/C/threads/mutex.h>
#include <Inventor/C/threads/condvar.h>
#include <Inventor/C/errors/debugerror.h>

#include "threads/workerp.h"
#include "threads/mutexp.h"

/* ********************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * this is the thread's main loop.
 */
static void 
worker_thread_loop(cc_worker * worker)
{
  cc_mutex_lock(worker->mutex);
  cc_mutex_lock(worker->beginmutex);
  /* signal master that we've entered the scheduling loop */
  cc_condvar_wake_one(worker->begincond);
  cc_mutex_unlock(worker->beginmutex);

  while (!worker->shutdown) {
    /* wait for job */
    cc_condvar_wait(worker->cond, worker->mutex);
    /* check if there are more jobs */
    if (!worker->shutdown) {
      /* do the scheduled job */
      worker->workfunc(worker->workclosure);
      /* call the idle cb */
      if (worker->idlecb) {
        worker->idlecb(worker, worker->idleclosure);
      }
    }
  }

  worker->workfunc = NULL;
  /* remember to unlock mutex after we break out of the loop */
  cc_mutex_unlock(worker->mutex);
}

/*
 * entry point for thread. It just jumps to the main loop
 */
static void * 
worker_thread_entry(void * data)
{
  cc_worker * worker = (cc_worker*) data;
  worker_thread_loop(worker);
  return NULL;
}

/*
 * called to start thread. Assumes worker->mutex is locked by caller */
static void 
worker_start_thread(cc_worker * worker)
{
  if (!worker->threadisrunning) {
    cc_mutex_lock(worker->beginmutex);
    /* Unlock worker mutex before starting thread. The new thread will use
       mutex and beginmutex  to synchronize */
    cc_mutex_unlock(worker->mutex);
    worker->thread = cc_thread_construct(worker_thread_entry, worker);
    
    /* Wait for thread to get to the main loop. The new thread will
     have worker->mutex locked when this signal arrives */
    cc_condvar_wait(worker->begincond, worker->beginmutex);
    
    /* lock thread again so that we know the new thread is waiting for
       a signal before we return */
    cc_mutex_lock(worker->mutex);
    worker->threadisrunning = 1;
    cc_mutex_unlock(worker->beginmutex);
  }
}

/*
 * join caller thread with this thread.
 */
static void 
worker_stop_thread(cc_worker * worker)
{
  if (worker->threadisrunning) {
    cc_mutex_lock(worker->mutex);
    worker->threadisrunning = 0;
    
    worker->shutdown = TRUE;  /* signal thread to exit loop */
    /* in case thread is waiting for cond... */
    cc_condvar_wake_one(worker->cond);
    cc_mutex_unlock(worker->mutex);
    /* wait for thread to finish */
    cc_thread_join(worker->thread, NULL);
    cc_thread_destruct(worker->thread);
    worker->thread = NULL;
    worker->shutdown = FALSE; /* reset signal */
  }
}
  

cc_worker * 
cc_worker_construct(void)
{
  cc_worker * worker = (cc_worker*) malloc(sizeof(cc_worker));
  assert(worker);

  worker->mutex = cc_mutex_construct();
  worker->cond = cc_condvar_construct();
  worker->begincond = cc_condvar_construct();
  worker->beginmutex = cc_mutex_construct();
  worker->thread = NULL; /* delay creating thread */
  worker->threadisrunning = FALSE;
  worker->shutdown = FALSE;
  worker->workfunc = NULL;
  worker->workclosure = NULL;
  worker->idlecb = NULL;
  worker->idleclosure = NULL;
  return worker;
}

/*!
  Will wait for the current task to be finished before destructing the worker.
*/
void 
cc_worker_destruct(cc_worker * worker)
{
  if (worker->threadisrunning) {
    worker_stop_thread(worker);
  }
  assert(worker->thread == NULL);
  cc_mutex_destruct(worker->mutex);
  cc_condvar_destruct(worker->cond);
  cc_condvar_destruct(worker->begincond);
  cc_mutex_destruct(worker->beginmutex);
  free(worker);
}

/*!
  Start the worker by either waking is from a condvar_wait() or
  creating a new thread if the thread is not running.
*/
SbBool 
cc_worker_start(cc_worker * worker, cc_worker_f * workfunc, void * closure)
{
  assert(workfunc);

  cc_mutex_lock(worker->mutex);  
  worker->workfunc = workfunc;
  worker->workclosure = closure;

  if (!worker->threadisrunning) {
    worker_start_thread(worker);
  }
  
  /* We now know that thread is waiting for a signal */
  cc_condvar_wake_one(worker->cond);
  cc_mutex_unlock(worker->mutex);
  return TRUE;
}

SbBool 
cc_worker_is_busy(cc_worker * worker)
{
  SbBool busy = TRUE;
  
  if (cc_mutex_try_lock(worker->mutex)) {
    busy = FALSE;
    cc_mutex_unlock(worker->mutex);
  }
  return busy;
}

void 
cc_worker_wait(cc_worker * worker)
{
  /* synchronize by stopping thread. A new thread must be created
   * if more work needs to be done, but this is ok, I guess.
   */
  worker_stop_thread(worker);
}

void 
cc_worker_set_idle_callback(cc_worker * worker, cc_worker_idle_f * cb, 
                            void * closure)
{
  cc_mutex_lock(worker->mutex);
  worker->idlecb = cb;
  worker->idleclosure = closure;
  cc_mutex_unlock(worker->mutex);
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
