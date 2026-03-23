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
  \struct cc_thread common.h Inventor/C/threads/common.h
  \ingroup coin_threads
  \brief The structure for a thread.
*/

/*!
  \typedef struct cc_thread cc_thread
  \ingroup coin_threads
  \brief The type definition for the thread structure.
*/

/*!
  \enum cc_retval {
    CC_ERROR = 0,
    CC_OK = 1,
    CC_TIMEOUT,
    CC_BUSY
  }
  \ingroup coin_threads
  \brief The enumerator for return values of thread related functions.
*/

/*!
  \typedef enum cc_retval cc_retval
  \ingroup coin_threads
  \brief The type definition for the return value enumerator.
*/

#include <Inventor/C/threads/thread.h>

#include <cstdlib>
#include <cassert>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <Inventor/C/errors/debugerror.h>

#include "threads/threadp.h"
#include "threads/mutexp.h"
#include "threads/recmutexp.h"
#include "threads/syncp.h"
#include "tidbitsp.h"


/* ********************************************************************** */

/*
 FIXME:
 - copy struct malloc/free/init/clean setup scheme from cc_string
 - use static table of cc_thread structures?
 - use cc_storage to reference self-structure for cc_thread_get_self()?
*/

/* ********************************************************************** */

#ifdef USE_PTHREAD
#include "threads/thread_pthread.icc"
#endif /* USE_PTHREAD */

#ifdef USE_W32THREAD
#include "threads/thread_win32.icc"
#endif /* USE_W32THREAD */

/*
*/

cc_thread *
cc_thread_construct(cc_thread_f * func, void * closure)
{
  cc_thread * thread;
  int ok;

  thread = (cc_thread*) malloc(sizeof(cc_thread));
  assert(thread != NULL);
  thread->func = func;
  thread->closure = closure;

  ok = internal_init(thread);
  if (ok) return thread;
  assert(0 && "unable to create thread");
  free(thread);
  return NULL;
}

/* ********************************************************************** */

/*
*/

void
cc_thread_destruct(cc_thread * thread)
{
  int ok;
  assert(thread != NULL);
  ok = internal_clean(thread);
  assert(ok == CC_OK);
  free(thread);
}

/* ********************************************************************** */

/*
*/

int
cc_thread_join(cc_thread * thread,
               void ** retvalptr)
{
  int ok;
  assert(thread != NULL);

  ok = internal_join(thread, retvalptr);
  assert(ok == CC_OK);
  return ok;
}

/* ********************************************************************** */

void
cc_sleep(float seconds)
{
#ifndef _WIN32
  /* FIXME: 20011107, thammer: create a configure macro to detect
   * which sleep function is available */
  sleep(floor(seconds));
#else
  Sleep((int)(seconds*1000.0));
#endif
};

#ifdef USE_PTHREAD
unsigned long 
cc_thread_id(void)
{
  return (unsigned long) pthread_self();
}
#endif /* USE_PTHREAD */

#ifdef USE_W32THREAD

static DWORD win32_threadid_idx;

unsigned long 
cc_thread_id(void)
{
  static unsigned long currentidx = 1;
  LPVOID val = TlsGetValue(win32_threadid_idx);
  if (val == 0) { /* not set yet */
    cc_mutex_global_lock();
    val = (LPVOID) (uintptr_t)currentidx++;
    cc_mutex_global_unlock();
    if (!TlsSetValue(win32_threadid_idx, (LPVOID)val)) {
      assert(0 && "unexpected failure");
    }
  }
  return (unsigned long) (intptr_t) (val);
}

static void 
win32_threadid_idx_cleanup(void)
{
  TlsFree(win32_threadid_idx);
}

#endif /* USE_WIN32THREAD */


void
cc_thread_init(void)
{
  cc_mutex_init();
  cc_sync_init();
#ifdef USE_W32THREAD
  /* needed to quickly generate a thread-id for each thread */
  win32_threadid_idx = TlsAlloc();
  assert(win32_threadid_idx != TLS_OUT_OF_INDEXES); 
  /* cleanup priority for the thread sub-system in Coin is set so it
     is done very late at exit */
  /* FIXME: not sure if this really needs the "- 2", but I added it
     to keep the same order wrt the other thread-related cleanup
     functions, since before I changed hard-coded numbers for
     enumerated values for coin_atexit() invocations. 20060301 mortene. */
  coin_atexit(win32_threadid_idx_cleanup, CC_ATEXIT_THREADING_SUBSYSTEM_VERYLOWPRIORITY);
#endif /* USE_WIN32THREAD */ 
  cc_recmutex_init();
}

/* ********************************************************************** */

/* maybe use static table of thread structures, reference counted, to be
   able to implement something like this, if needed */
/* cc_thread * cc_thread_get_self(void); */

/* ********************************************************************** */

/*
 * We don't really want to expose internal id types, which would mean we
 * must include threads-implementation-specific headers in the header files.
 * It's therefore better to implement the missing/needed functionality for
 * the cc_thread type, so id peeking won't be necessary.
 */

/* <id> cc_thread_get_id(cc_thread * thread); */
/* <id> cc_thread_get_current_id(void); */

/* ********************************************************************** */

/*!
  \page coin_multithreading_support Multithreading Support in Coin

  The support in Coin for using multiple threads in application
  programs and the Coin library itself, consists of two main features:

  <ul>

  <li>
  Coin provides platform independent thread handling abstraction
  classes. These are classes that the application programmer can
  freely use in her application code to start new threads, control
  their execution, work with mutexes and do other tasks related to
  handling multiple threads.

  The classes in question are SbThread, SbMutex, SbStorage, SbBarrier,
  SbCondVar, SbFifo, SbThreadAutoLock, SbRWMutex, and
  SbTypedStorage. See their respective documentation for the detailed
  information.

  The classes fully hides the system specific implementation, which is
  either done on top of native Win32 (if on Microsoft Windows), or
  over POSIX threads (on UNIX and UNIX-like systems).
  </li>

  <li>
  The other aspect of our multi-threading support is that Coin can be
  specially configured so that rendering traversals of the scene graph
  are done in a thread-safe manner. This means e.g. that it is
  possible to have Coin render the scene in parallel on multiple CPUs
  for multiple rendering pipes, to better take advantage of such
  high-end systems (like CAVE environments, for instance).

  Thread-safe render traversals are \e off by default, because there
  is a small overhead involved which would make rendering (very)
  slightly slower on single-threaded invocations.

  To get a Coin library built with thread-safe rendering, one must
  actively reconfigure Coin and build a special, local version. For
  configure-based builds (UNIX and UNIX-like systems, or with Cygwin
  on Microsoft Windows) this is done with the option
  "--enable-threadsafe" to Autoconf configure. To change the
  configuration and rebuild with Visual Studio, you will need to
  change the preprocessor directive COIN_THREADSAFE to defined in the
  file src/setup.h located in the same folder as you found your
  solution file.</li>

  </ul>

  There are some restrictions and other issues which it is important
  to be aware of:
  
  <ul>

  <li> We do not yet provide any support for binding the
  multi-threaded rendering support into the SoQt / SoWin / etc GUI
  bindings, and neither do we provide bindings against any specific
  library that handles multi-pipe rendering. This means the
  application programmer will have to possess some expertise, and put
  in some effort, to be able to utilize multi-pipe rendering with
  Coin. </li>

  <li> Rendering traversals are currently the only operation which we
  publicly support to be thread-safe. There are other aspects of Coin
  that we know are thread-safe, like most other action traversals
  beside just rendering, but we make no guarantees in this
  regard. </li>

  <li> Be careful about using a separate thread for changing Coin
  structures versus what is used for the applications GUI event
  thread.

  We are aware of at least issues with Qt3 (and thereby SoQt), where
  you should not modify the scene graph in any way in a thread
  separate from the main Qt thread. This because it will trigger
  operations where Qt3 is not thread-safe. For Qt4, we have not been
  aware of such problems.</li>

  </ul>

  \since Coin 2.0
*/

/* ********************************************************************** */

// All the documentation below is obsolete - however it may be useful for re-writing
// So, for now, simply revert to a normal c++ comment.  walroy 20140613

/*
  \class SbThread Inventor/threads/SbThread.h
  \brief A class for managing threads.

  \ingroup coin_threads

  This class provides a portable framework around the tasks of
  instantiating, starting, stopping and joining threads.

  It wraps the underlying native thread-handling toolkit in a
  transparent manner, to make multiplatform threads programming
  straightforward for the application programmer.
*/

/*
  \fn static SbThread * SbThread::create(void *(*func)(void *), void * closure)

  This function creates a new thread, or returns NULL on failure.
*/

/*
  \fn static void SbThread::destroy(SbThread * thread)

  This function destroys a thread.
*/

/*
  \fn static int SbThread::join(SbThread * thread, void ** retval)

  This function waits on the death of the given thread, returning the thread's
  return value at the location pointed to by \c retval.
*/

/*
  \fn int SbThread::join(void ** retval)

  This function waits on the death of the given thread, returning the thread's
  return value at the location pointed to by \c retval.
*/

/*
  \fn SbThread::SbThread(cc_thread * thread)

  Protected constructor handling the internal thread ADT.

  \sa SbThread::create
*/

/*
  \fn SbThread::~SbThread(void)

  Destructor.

  \sa SbThread::destroy
*/

/* ********************************************************************** */

