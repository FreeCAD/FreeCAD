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

/* FIXME: should provide dummy implementations of the (few) internal
   public cc_mutex_*() calls, so one can include the header files
   mutex.h and SbMutex.h without #ifdef checks, and also declare
   e.g. SbMutex instances when thread-support is missing.

   This would clean up source code everywhere we're using mutex'es.

   20050516 mortene.
*/

/*!
  \struct cc_mutex common.h Inventor/C/threads/common.h
  \ingroup coin_threads
  \brief The structure for a mutex.
*/

/*!
  \typedef struct cc_mutex cc_mutex
  \ingroup coin_threads
  \brief The type definition for the mutex structure.
*/

#include <Inventor/C/threads/mutex.h>

#include <cstdlib>
#include <cassert>
#include <cstddef>
#include <cerrno>
#include <cfloat>

#include <Inventor/C/base/time.h>
#include <Inventor/C/errors/debugerror.h>

#include "threads/mutexp.h"
#include "tidbitsp.h"

#ifdef USE_PTHREAD
#include "mutex_pthread.icc"
#endif /* USE_PTHREAD */

#ifdef USE_W32THREAD
/* we test if Win32 TryEnterCriticalSection exists, and use Win32
   critical section if it does, and Win32 mutex if it doesn't */
typedef BOOL (WINAPI * cc_mutex_TryEnterCriticalSection_func)(LPCRITICAL_SECTION);
static cc_mutex_TryEnterCriticalSection_func cc_mutex_TryEnterCriticalSection = NULL; 
#include "mutex_win32mutex.icc" 
#include "mutex_win32cs.icc" 
#endif /* USE_W32THREAD */

/**************************************************************************/

static double maxmutexlocktime = DBL_MAX;
static double reportmutexlocktiming = DBL_MAX;

/**************************************************************************/

/*
  \internal
*/

void
cc_mutex_struct_init(cc_mutex * mutex_struct)
{
  int ok;
#ifdef USE_W32THREAD
  if (cc_mutex_TryEnterCriticalSection)
    ok = win32_cs_struct_init(mutex_struct);
  else 
    ok = win32_mutex_struct_init(mutex_struct);
#else /* USE_W32THREAD */
  ok = internal_mutex_struct_init(mutex_struct);
#endif /* ! USE_W32THREAD */
  assert(ok);
}

/*
  \internal
*/

void
cc_mutex_struct_clean(cc_mutex * mutex_struct)
{
  int ok;
  assert(mutex_struct);
#ifdef USE_W32THREAD
  if (cc_mutex_TryEnterCriticalSection)
    ok = win32_cs_struct_clean(mutex_struct);
  else 
    ok = win32_mutex_struct_clean(mutex_struct);
#else /* USE_W32THREAD */  
  ok = internal_mutex_struct_clean(mutex_struct);
#endif /* ! USE_W32THREAD */
  assert(ok == CC_OK);
}

/**************************************************************************/

/* debugging. for instance useful for checking that there's not
   excessive mutex construction. */

/* don't hide 'static' these to hide them in file-scope, as they are
   used from rwmutex.cpp and recmutex.cpp as well. */
unsigned int cc_debug_mtxcount = 0;
const char * COIN_DEBUG_MUTEX_COUNT = "COIN_DEBUG_MUTEX_COUNT";

/**************************************************************************/

/* Return value of COIN_DEBUG_MUTEX_COUNT environment variable. */
static int coin_debug_mutex_count(void)
{
  static int d = -1;
  if (d == -1) {
    const char* val = coin_getenv("COIN_DEBUG_MUTEX_COUNT");
    d = val ? atoi(val) : 0;
  }
  return d;
}

/*! Constructs a mutex. */
cc_mutex *
cc_mutex_construct(void)
{
  cc_mutex * mutex;
  mutex = (cc_mutex *) malloc(sizeof(cc_mutex));
  assert(mutex != NULL);
  cc_mutex_struct_init(mutex);

  /* debugging */
  if (coin_debug_mutex_count() > 0) {
    cc_debug_mtxcount += 1;
    (void)fprintf(stderr, "DEBUG: live mutexes +1 => %u (mutex++)\n",
                  cc_debug_mtxcount);
  }

  return mutex;
}

/*! Destroys the \a mutex specified. */
void
cc_mutex_destruct(cc_mutex * mutex)
{
  /* debugging */
  if (coin_debug_mutex_count() > 0) {
    assert((cc_debug_mtxcount > 0) && "skewed mutex construct/destruct pairing");
    cc_debug_mtxcount -= 1;
    (void)fprintf(stderr, "DEBUG: live mutexes -1 => %u (mutex--)\n",
                  cc_debug_mtxcount);
  }

  assert(mutex != NULL);
  cc_mutex_struct_clean(mutex);
  free(mutex);
}

/**************************************************************************/

/*! Locks the \a mutex specified. */
void
cc_mutex_lock(cc_mutex * mutex)
{
  int ok;
  SbBool timeit;
  cc_time start = 0.0;

  assert(mutex != NULL);

  timeit = (maxmutexlocktime != DBL_MAX) || (reportmutexlocktiming != DBL_MAX);
  if (timeit) { start = cc_time_gettimeofday(); }

#ifdef USE_W32THREAD
  ok = cc_mutex_TryEnterCriticalSection ? win32_cs_lock(mutex) : win32_mutex_lock(mutex);
#else /* USE_W32THREAD */  
  ok = internal_mutex_lock(mutex);
#endif /* USE_W32THREAD */

  assert(ok == CC_OK);

  /* This is here as an optional debugging aid, when having problems
     related to locks that are held too long. (Typically resulting in
     unresponsive user interaction / lags.)  */
  if (timeit) {
    const cc_time spent = cc_time_gettimeofday() - start;

    if (spent >= reportmutexlocktiming) {
      /* Can't use cc_debugerror_postinfo() here, because we get a
         recursive call to this function, and a non-terminating lock /
         hang. */
      (void)fprintf(stdout, "DEBUG cc_mutex_lock(): mutex %p spent %f secs in lock\n",
                    mutex, spent);
    }

    assert(spent <= maxmutexlocktime);
  }
}

/*! Tests the specified \a mutex to see it is already locked. */

int
cc_mutex_try_lock(cc_mutex * mutex)
{
  int ok;
  assert(mutex != NULL);
#ifdef USE_W32THREAD
  if (cc_mutex_TryEnterCriticalSection)
    ok = win32_cs_try_lock(mutex);
  else 
    ok = win32_mutex_try_lock(mutex);
#else /* USE_W32THREAD */  
  ok = internal_mutex_try_lock(mutex);
#endif /* ! USE_W32THREAD */
  assert(ok == CC_OK || ok == CC_BUSY);
  return ok;
}

/*! Unlocks the specified \a mutex.*/

void
cc_mutex_unlock(cc_mutex * mutex)
{
  int ok;
  assert(mutex != NULL);
#ifdef USE_W32THREAD
  if (cc_mutex_TryEnterCriticalSection)
    ok = win32_cs_unlock(mutex);
  else 
    ok = win32_mutex_unlock(mutex);
#else /* USE_W32THREAD */  
  ok = internal_mutex_unlock(mutex);
#endif /* USE_W32THREAD */

  assert(ok == CC_OK);
}

static cc_mutex * cc_global_mutex = NULL;

static void
cc_mutex_cleanup(void)
{
  cc_mutex_destruct(cc_global_mutex);
  cc_global_mutex = NULL;
}

void
cc_mutex_init(void)
{
  const char * env = coin_getenv("COIN_DEBUG_MUTEXLOCK_MAXTIME");

#ifdef USE_W32THREAD /* TryEnterCriticalSection test. */

  HINSTANCE h = GetModuleHandle("kernel32.dll");
  /* If we can't get a handle to kernel32.dll, something is seriously
     wrong, and we should investigate. <mortene> */
  assert(h && "GetModuleHandle('kernel32.dll') failed!");

  /* This function is unsupported in Win95/98/Me and NT <=3.51, but we
     still want to use it if it is available, since it can provide
     major speed-ups for certain aspects of Win32 mutex handling. */
  cc_mutex_TryEnterCriticalSection = (cc_mutex_TryEnterCriticalSection_func)
    GetProcAddress(h, "TryEnterCriticalSection");
  
#endif /* USE_W32THREAD */

  if (cc_global_mutex == NULL) {
    cc_global_mutex = cc_mutex_construct();
    /* atexit priority makes this callback trigger after other cleanup
       functions. */
    /* FIXME: not sure if this really needs the "- 1", but I added it
       to keep the same order wrt the other thread-related cleanup
       functions, since before I changed hard-coded numbers for
       enumerated values for coin_atexit() invocations. 20060301 mortene. */
    coin_atexit((coin_atexit_f*) cc_mutex_cleanup, CC_ATEXIT_THREADING_SUBSYSTEM_LOWPRIORITY);
  }

  if (env) { maxmutexlocktime = atof(env); }

  env = coin_getenv("COIN_DEBUG_MUTEXLOCK_TIMING");
  if (env) { reportmutexlocktiming = atof(env); }
}

void 
cc_mutex_global_lock(void)
{
  /* Do this test in case a mutex is needed before cc_mutex_init() is
     called (called from SoDB::init()). This is safe, since the
     application should not be multithreaded before SoDB::init() is
     called */
  if (cc_global_mutex == NULL) cc_mutex_init();
  
  (void) cc_mutex_lock(cc_global_mutex);
}

void 
cc_mutex_global_unlock(void)
{
  (void) cc_mutex_unlock(cc_global_mutex);
}

