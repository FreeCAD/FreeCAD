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

/*
  This ADT manages thread-local memory.  When different threads access
  the memory an cc_storage object manages, they will receive different
  memory blocks back.

  For additional API documentation, see doc of the SbStorage C++
  wrapper around the cc_storage_*() functions at the bottom of this
  file.
*/

/* ********************************************************************** */

/*!
  \struct cc_storage common.h Inventor/C/threads/common.h
  \ingroup coin_threads
  \brief The structure for the thread local memory storage.
*/

/*!
  \typedef struct cc_storage cc_storage
  \ingroup coin_threads
  \brief The type definition for the thread local memory storage structure.
*/

#include <Inventor/C/threads/storage.h>
#include "coindefs.h"

#include <cstdlib>
#include <cassert>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

/* FIXME: instead of doing all the HAVE_THREADS wrapping in the code
   of this file, perhaps it would be cleaner to make the cc_thread and
   cc_mutex interfaces *working*, dummy skeletons when no thread
   abstractions are available?  20040615 mortene. */
#ifdef HAVE_THREADS
#include <Inventor/C/threads/thread.h>
#include <Inventor/C/threads/mutex.h>
#endif /* HAVE_THREADS */

#include "threads/storagep.h"

/* ********************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* private functions */

static cc_storage *
cc_storage_init(unsigned int size, void (*constructor)(void *), 
                void (*destructor)(void *)) 
{
  cc_storage * storage = (cc_storage *) malloc(sizeof(cc_storage));
  storage->size = size;
  storage->constructor = constructor;
  storage->destructor = destructor;
  storage->dict = cc_dict_construct(8, 0.75f);
#ifdef HAVE_THREADS
  storage->mutex = cc_mutex_construct();
#endif /* HAVE_THREADS */

  return storage;
}

static void
cc_storage_hash_destruct_cb(uintptr_t COIN_UNUSED_ARG(key), void * val, void * closure)
{
  cc_storage * storage = (cc_storage*) closure;
  
  if (storage->destructor) {
    storage->destructor(val);
  }
  free(val);
}

/* ********************************************************************** */
/* public api */

cc_storage *
cc_storage_construct(unsigned int size)
{
  return cc_storage_init(size, NULL, NULL);
}

cc_storage *
cc_storage_construct_etc(unsigned int size,
                         void (*constructor)(void *),
                         void (*destructor)(void *))
{
  return cc_storage_init(size, constructor, destructor);
}

/*
*/
void
cc_storage_destruct(cc_storage * storage)
{
  assert(storage != NULL);

  cc_dict_apply(storage->dict, cc_storage_hash_destruct_cb, storage);
  cc_dict_destruct(storage->dict);

#ifdef HAVE_THREADS
  cc_mutex_destruct(storage->mutex);
#endif /* HAVE_THREADS */

  free(storage);
}

/* ********************************************************************** */

/*
*/

void *
cc_storage_get(cc_storage * storage)
{
  void * val;
  unsigned long threadid = 0;

#ifdef HAVE_THREADS
  threadid = cc_thread_id();

  cc_mutex_lock(storage->mutex);
#endif /* HAVE_THREADS */

  if (!cc_dict_get(storage->dict, threadid, &val)) {
    val = malloc(storage->size);
    if (storage->constructor) {
      storage->constructor(val);
    }
    (void) cc_dict_put(storage->dict, threadid, val);
  }

#ifdef HAVE_THREADS
  cc_mutex_unlock(storage->mutex);
#endif /* HAVE_THREADS */

  return val;
}

/* struct needed for cc_dict wrapper callback */
typedef struct {
  cc_storage_apply_func * func;
  void * closure;
} cc_storage_hash_apply_data; 

/* callback from cc_dict_apply. will simply call the function specified
   in cc_storage_apply_to_appl */
static void 
storage_hash_apply(uintptr_t COIN_UNUSED_ARG(key), void * val, void * closure)
{
  cc_storage_hash_apply_data * data = 
    (cc_storage_hash_apply_data*) closure;
  data->func(val, data->closure);
}

void 
cc_storage_apply_to_all(cc_storage * storage, 
                        cc_storage_apply_func * func, 
                        void * closure)
{
  /* need to set up a struct to use cc_dict_apply */
  cc_storage_hash_apply_data mydata;
  
  /* store func and closure in struct */
  mydata.func = func;
  mydata.closure = closure;

#ifdef HAVE_THREADS
  cc_mutex_lock(storage->mutex);
  cc_dict_apply(storage->dict, storage_hash_apply, &mydata);
  cc_mutex_unlock(storage->mutex);
#else /* ! HAVE_THREADS */
  cc_dict_apply(storage->dict, storage_hash_apply, &mydata);
#endif /* ! HAVE_THREADS */

}


/* ********************************************************************** */

void 
cc_storage_thread_cleanup(unsigned long COIN_UNUSED_ARG(threadid))
{
  /* FIXME: remove and destruct all data for this thread for all storages */
}

/* ********************************************************************** */

// All the documentation below is obsolete - however it may be useful for re-writing
// So, for now, simply revert to a normal c++ comment.  walroy 20140613
/*
  \class SbStorage Inventor/threads/SbStorage.h
  \brief The SbStorage class manages thread-local memory.

  \ingroup coin_threads

  This class manages thread-local memory.  When different threads
  access the memory an SbStorage object manages, they will receive
  different memory blocks back.

  This provides a mechanism for sharing read/write static data.

  One important implementation detail: if the Coin library was
  explicitly configured to be built without multi-platform thread
  abstractions, or neither pthreads nor native Win32 thread functions
  are available, it will be assumed that the client code will all run
  in the same thread. This means that the same memory block will be
  returned for any request without considering the current thread id.
*/

/*
  \fn SbStorage::SbStorage(unsigned int size)

  Constructor.  \a size specifies the number of bytes each thread should
  have in this thread-local memory management object.
*/

/*
  \fn SbStorage::SbStorage(unsigned int size, void (*constr)(void *), void (*destr)(void *))

  Constructor.  \a size specifies the number of bytes each thread should
  have in this thread-local memory management object.  A constructor and
  a destructor functions can be given that will be called when the actual
  memory blocks are allocated and freed.
*/

/*
  \fn SbStorage::~SbStorage(void)

  The destructor.
*/

/*
  \fn void * SbStorage::get(void)

  This method returns the calling thread's thread-local memory block.
*/

/*
  \fn void SbStorage::applyToAll(SbStorageApplyFunc * func, void * closure)
  
  This method will call \a func for all thread local storage data.
  \a closure will be supplied as the second parameter to the callback.
*/

/* ********************************************************************** */

/*
  \class SbTypedStorage Inventor/threads/SbTypedStorage.h
  \brief The SbTypedStorage class manages generic thread-local memory.

  \ingroup coin_threads

  This class manages thread-local memory.  When different threads
  access the memory an SbTypedStorage object manages, they will receive
  different memory blocks back.

  This provides a mechanism for sharing read/write static data.
*/

/*
  \fn SbTypedStorage<Type>::SbTypedStorage(unsigned int size)

  Constructor.  \a size specifies the number of bytes each thread should
  have in this thread-local memory management object.
*/

/*
  \fn SbTypedStorage<Type>::SbTypedStorage(unsigned int size, void (*constr)(void *), void (*destr)(void *))

  Constructor.  \a size specifies the number of bytes each thread
  should have in this thread-local memory management object.
  Constructor and a destructor functions can be specified that will be
  called when the actual memory blocks are allocated and freed.
*/

/*
  \fn SbTypedStorage<Type>::~SbTypedStorage(void)

  The destructor.
*/

/*
  \fn Type SbTypedStorage<Type>::get(void)

  This method returns the calling thread's thread-local memory block.
*/

/* ********************************************************************** */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
