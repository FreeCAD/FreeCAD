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
  \struct cc_barrier common.h Inventor/C/threads/common.h
  \ingroup coin_threads
  \brief The structure for the thread barrier.
*/

/*!
  \typedef struct cc_barrier cc_barrier
  \ingroup coin_threads
  \brief The type definition for the thread barrier structure.
*/

#include <Inventor/C/threads/barrier.h>

#include <cstdlib>
#include <cstdio>
#include <cassert>

#include <Inventor/C/threads/mutex.h>
#include <Inventor/C/threads/condvar.h>

#include "threads/barrierp.h"

/* ********************************************************************** */

/*! Constructs a new thread barrier. */

cc_barrier *
cc_barrier_construct(unsigned int count)
{
  cc_barrier * barrier;
  barrier = (cc_barrier *) malloc(sizeof(cc_barrier));
  barrier->numthreads = count;
  barrier->counter = 0;
  barrier->mutex = cc_mutex_construct();
  barrier->condvar = cc_condvar_construct();
  return barrier;
}

/*! Destroys the \a barrier thread barrier. */
void
cc_barrier_destruct(cc_barrier * barrier)
{
  assert(barrier != NULL);
         
  cc_condvar_wake_all(barrier->condvar);
  cc_condvar_destruct(barrier->condvar);
  cc_mutex_destruct(barrier->mutex);
  free(barrier);
}

/*! Increments the barrier counter. If equal to the number of threads
    it then enables all the threads to proceed. */

int
cc_barrier_enter(cc_barrier * barrier)
{
  assert(barrier != NULL);
  cc_mutex_lock(barrier->mutex);
  barrier->counter++;
  if (barrier->counter == barrier->numthreads) {
    barrier->counter = 0;
    cc_condvar_wake_all(barrier->condvar);
    cc_mutex_unlock(barrier->mutex);
    return 1;
  }
  else {
    cc_condvar_wait(barrier->condvar, barrier->mutex);
    cc_mutex_unlock(barrier->mutex);
  }
  return 0;
}

