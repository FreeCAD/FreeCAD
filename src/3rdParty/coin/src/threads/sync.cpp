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

#include <Inventor/C/threads/sync.h>
#include "coindefs.h"

#include <cstddef>

#include <Inventor/C/threads/mutex.h>

#include "tidbitsp.h"
#include "base/dict.h"
#include "threads/syncp.h"
#include "threads/mutexp.h"

/* ********************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static cc_dict * sync_hash_table = NULL;

static void
sync_hash_cb(uintptr_t COIN_UNUSED_ARG(key), void * val, void * COIN_UNUSED_ARG(closure))
{
  cc_mutex_destruct((cc_mutex*) val);
}

static void
sync_cleanup(void)
{
  cc_dict_apply(sync_hash_table, sync_hash_cb, NULL);
  cc_dict_destruct(sync_hash_table);
  sync_hash_table = NULL;
}

/*
  Initialize synchronizer. Should only be called once, from SoDB::init()
*/
void
cc_sync_init(void)
{
  if (sync_hash_table == NULL) {
    /* the priority is set so to make this callback trigger late,
       after normal cleanup function which might still use a cc_sync
       instance */
    coin_atexit((coin_atexit_f*) sync_cleanup, CC_ATEXIT_THREADING_SUBSYSTEM);
    sync_hash_table = cc_dict_construct(256, 0.75f);    
  }
}

/*!

  Begin synchronizing \a id. After returning from this call, other
  threads trying to synchronize \a id will be blocked until the first
  thread calls cc_sync_end() with the key returned from this function.

*/
void *
cc_sync_begin(void * id)
{
  void * mutex;

  cc_mutex_global_lock();
  if (sync_hash_table == NULL) {
    cc_sync_init();
  }
  if (!cc_dict_get(sync_hash_table, (uintptr_t)id, &mutex)) {
    mutex = cc_mutex_construct();
    (void) cc_dict_put(sync_hash_table, (uintptr_t)id, mutex);
  }

  cc_mutex_global_unlock();
  cc_mutex_lock((cc_mutex*) mutex);
  
  return mutex;
}

/*!
  End synchronize. \a key should be the key returned from cc_sync_begin().
*/
void
cc_sync_end(void * key)
{
  cc_mutex_unlock((cc_mutex*) key);
}

/*!
  Free sync item. This can be called from the destructor of objects
  that use a sync item so that the sync-mutex is freed when the object
  is destructed.

  \since Coin 2.3
*/
void 
cc_sync_free(void * id)
{
  void * mutex;

  cc_mutex_global_lock();
  if (sync_hash_table == NULL) {
    cc_sync_init();
  }
  if (cc_dict_get(sync_hash_table, (uintptr_t)id, &mutex)) {
    cc_mutex_destruct((cc_mutex*) mutex);
    cc_dict_remove(sync_hash_table, (uintptr_t)id);
  }
  cc_mutex_global_unlock();
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
