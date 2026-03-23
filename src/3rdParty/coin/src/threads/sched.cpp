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
  \struct cc_sched common.h Inventor/C/threads/common.h
  \ingroup coin_threads
  \brief The structure for the thread scheduler.
*/

/*!
  \typedef struct cc_sched cc_sched
  \ingroup coin_threads
  \brief The type definition for the thread scheduler structure.
*/

#include <Inventor/C/threads/sched.h>

#include <cassert>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

/* FIXME: This class has changed somewhat since Coin-2. Tag this so
   we can find it when writing Coin-2 -> Coin-3 API change documentation.
   kintel 20061124. */

/* FIXME: Evaluate if some of the new functionality can be ported back 
   to Coin-2. kintel 20061124. */

#if (!defined HAVE_THREADS) && (!defined DOXYGEN_SKIP_THIS)
/* The DOXYGEN_SKIP_THIS define has been added to ensure that doxygen
   ignores this section and finds the documentation under HAVE_THREADS
   even though HAVE_THREADS will not have been defined.

   20140918 Roy Walmsley */

/* FIXME: instead of disallowing the use of these completely when
   thread support is not available (that's why there are asserts
   within them), could we perhaps implement in such a manner that they
   still work, but within only the calling thread?

   20051202 mortene. */

cc_sched * cc_sched_construct(int numthreads) { assert(FALSE); return NULL; }
void cc_sched_destruct(cc_sched * sched) { assert(FALSE); }
void cc_sched_set_num_threads(cc_sched * sched, int num) { assert(FALSE); }
int cc_sched_get_num_threads(cc_sched * sched) { assert(FALSE); return 0; }
uint32_t cc_sched_schedule(cc_sched * sched, 
                           cc_sched_f * workfunc, void * closure,
                           float priority) { assert(FALSE); }
void cc_sched_wait_all(cc_sched * sched) { assert(FALSE); }
SbBool cc_sched_unschedule(cc_sched * sched, 
                           uint32_t schedid) { assert(FALSE); }
void cc_sched_set_num_allowed(cc_sched * sched, 
                              int num)  { assert(FALSE); }
void cc_sched_change_priority(cc_sched * sched, 
                              uint32_t schedid, 
                              float priority)  { assert(FALSE); }

#else /* HAVE_THREADS && DOXYGEN_SKIP_THIS*/

#include <cstdlib>

#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/threads/mutex.h>
#include <Inventor/C/threads/thread.h>
#include <Inventor/C/threads/wpool.h>

#include "threads/schedp.h"

/* ********************************************************************** */

/*!
  \typedef void cc_sched_f(void * closure)

  The type definition for the work function called when a thread becomes available.
*/

/* ********************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* private methods */

static void sched_worker_entry_point(void * userdata);

typedef struct {
  cc_sched_f * workfunc;
  void * closure;
  float priority;
  uint32_t schedid;
} sched_item;

static int
sched_item_compare(void * o1, void * o2)
{
  sched_item * i1 = (sched_item *) o1;
  sched_item * i2 = (sched_item *) o2;
  
  float diff = i1->priority - i2->priority;
  if (diff > 0) return 1;
  else if (diff < 0) return -1;
  return 0;
}

/* assumes mutex is locked */
static SbBool
sched_try_trigger(cc_sched * sched)
{
  if (cc_wpool_try_begin(sched->pool, 1)) {
    cc_wpool_start_worker(sched->pool, sched_worker_entry_point, sched);
    cc_wpool_end(sched->pool);
    return TRUE;
  }
  return FALSE;
}

void
sched_worker_entry_point(void * userdata)
{
  sched_item * item;
  cc_sched * sched = (cc_sched *)userdata;

  cc_mutex_lock(sched->mutex);
  while (!cc_heap_empty(sched->itemheap) && 
         (sched->numallowed != 0 || sched->iswaitingall)) {
    item = (sched_item *)cc_heap_extract_top(sched->itemheap);
    cc_dict_remove(sched->schedid_dict, item->schedid);
    cc_mutex_unlock(sched->mutex);
    item->workfunc(item->closure);
    cc_mutex_lock(sched->mutex);
    cc_memalloc_deallocate(sched->itemalloc, item);
    if (sched->numallowed > 0) sched->numallowed--;
  }
  cc_mutex_unlock(sched->mutex);
}

/* ********************************************************************** */
/* public api */


/*!
  Construct a scheduler that uses \a numthreads threads.
*/
cc_sched *
cc_sched_construct(int numthreads)
{
  cc_sched * sched = (cc_sched *) malloc(sizeof(cc_sched));
  assert(sched);
  sched->pool = cc_wpool_construct(numthreads);
  sched->mutex = cc_mutex_construct();
 
  sched->itemheap = cc_heap_construct(64, sched_item_compare, TRUE);
  sched->itemalloc = cc_memalloc_construct(sizeof(sched_item));
  sched->schedid_dict = cc_dict_construct(64, 0.75f);
  sched->schedid_counter = 1;
  sched->iswaitingall = FALSE;
  sched->numallowed = -1; /* Unlimited */

  return sched;
}

/*!
  Destruct the scheduler.

  This method will block until all currently executing jobs have finished.
  Any remaining scheduled jobs will be canceled.

  Note that this differs from Coin-2. To emulate Coin-2 behavior, call
  cc_sched_wait_all() before calling this method.
*/
void
cc_sched_destruct(cc_sched * sched)
{
  cc_sched_set_num_allowed(sched, 0); // Exit inner scheduler loop faster
  cc_wpool_wait_all(sched->pool); // Make sure all worker threads are finished

  cc_dict_destruct(sched->schedid_dict);
  cc_heap_destruct(sched->itemheap);
  cc_memalloc_destruct(sched->itemalloc);
  cc_mutex_destruct(sched->mutex);
  cc_wpool_destruct(sched->pool);
  free(sched);
}

/*!
  Set/change the number of threads used by the scheduler.
*/
void
cc_sched_set_num_threads(cc_sched * sched, int num)
{
  cc_sched_wait_all(sched);
  cc_wpool_set_num_workers(sched->pool, num);
}

/*!
  Returns the number of threads used by the scheduler.
*/
int
cc_sched_get_num_threads(cc_sched * sched)
{
  return cc_wpool_get_num_workers(sched->pool);
}

/*! 
  Schedule a new job. A thread calls \a workfunc with the \a closure
  argument when a thread becomes available. larger \a priority values will
  be scheduled first.

  Returns a schedid that can be used to unschedule the job. schedid is
  guaranteed to be != 0.

  Note that jobs are automatically unscheduled when triggered, just before
  calling the work function.
*/
uint32_t
cc_sched_schedule(cc_sched * sched,
                  cc_sched_f * workfunc, void * closure,
                  float priority)
{
  sched_item * item;

  cc_mutex_lock(sched->mutex);
  item = (sched_item *)cc_memalloc_allocate(sched->itemalloc);
  
  item->workfunc = workfunc;
  item->closure = closure;
  item->priority = priority;
  item->schedid = sched->schedid_counter++;
  // avoid schedid == 0
  if (item->schedid == 0) {
    item->schedid = sched->schedid_counter++;
  }
  cc_heap_add(sched->itemheap, item);
  cc_dict_put(sched->schedid_dict, item->schedid, item);
  if (cc_dict_get_num_elements(sched->schedid_dict) == 1) {
    sched_try_trigger(sched);
  }

  cc_mutex_unlock(sched->mutex);

  return item->schedid;
}

/*!
  Attempt to unschedule a job. \a schedid must be an id returned
  from cc_sched_schedule().

  Note that jobs are automatically unscheduled when triggered, just before
  calling the work function.

  Returns TRUE if job was successfully removed, FALSE if job wasn't found
  in the internal dict.
*/
SbBool
cc_sched_unschedule(cc_sched * sched, uint32_t schedid)
{
  SbBool didremove = FALSE;
  void * item = NULL;
  cc_mutex_lock(sched->mutex);

  if (cc_dict_get(sched->schedid_dict, schedid, &item)) {
    cc_heap_remove(sched->itemheap, item);
    cc_dict_remove(sched->schedid_dict, schedid);
    cc_memalloc_deallocate(sched->itemalloc, item);
    didremove = TRUE;
  }
  cc_mutex_unlock(sched->mutex);
  return didremove;
}

/*!
  Returns the number of remaining scheduled jobs, not counting jobs that
  are currently being executed (i.e. are in their workfuncs).
*/
int 
cc_sched_get_num_remaining(cc_sched * sched)
{
  int num;
  cc_mutex_lock(sched->mutex);
  num = (int)cc_heap_elements(sched->itemheap);
  cc_mutex_unlock(sched->mutex);
  return num;
}

/*!
  Wait for all scheduled jobs to finish.
*/
void
cc_sched_wait_all(cc_sched * sched)
{
  cc_mutex_lock(sched->mutex);
  sched->iswaitingall = TRUE;
  /* Make sure all workers are doing something */
  while (!cc_heap_empty(sched->itemheap) && sched_try_trigger(sched)) { }

  cc_mutex_unlock(sched->mutex);
  cc_wpool_wait_all(sched->pool);

  cc_mutex_lock(sched->mutex);
  sched->iswaitingall = FALSE;
  cc_mutex_unlock(sched->mutex);
}

/*!
  Sets the number of allowed jobs per "batch". The scheduler will execute 
  at max the \e num jobs. To continue executing jobs, call this function again.
  This is typically done to limit the number of I/O or CPU intensive jobs to
  a few per frame to avoid starving the main thread.

  If \e num is -1 (the default), the number of jobs per batch is not limited.
*/
void 
cc_sched_set_num_allowed(cc_sched * sched, int num)
{
  cc_mutex_lock(sched->mutex);
  sched->numallowed = num;
  sched_try_trigger(sched);
  cc_mutex_unlock(sched->mutex);
}

/*!
  Changes the priority of the given scheduled item.
*/
void 
cc_sched_change_priority(cc_sched * sched, 
                         uint32_t schedid, float priority)
{
  void * item;
  cc_mutex_lock(sched->mutex);
  
  if (cc_dict_get(sched->schedid_dict, schedid, &item)) {
    cc_heap_remove(sched->itemheap, item);
    ((sched_item *)item)->priority = priority;
    cc_heap_add(sched->itemheap, item);
  }

  cc_mutex_unlock(sched->mutex);
}

/* ********************************************************************** */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* HAVE_THREADS */
