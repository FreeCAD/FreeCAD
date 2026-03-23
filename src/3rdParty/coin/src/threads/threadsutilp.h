#ifndef CC_THREADUTILP_H
#define CC_THREADUTILP_H

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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* ! COIN_INTERNAL */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_THREADS

#include <Inventor/C/threads/mutex.h>
#include <Inventor/C/threads/sync.h>

#include "threads/mutexp.h"

#define CC_MUTEX_CONSTRUCT(_mymutex_) \
  do { \
    cc_mutex_global_lock(); \
    if (_mymutex_ == NULL) { \
      _mymutex_ = static_cast<void*>(cc_mutex_construct()); \
    } \
    cc_mutex_global_unlock(); \
  } while (0)

#define CC_MUTEX_DESTRUCT(_mymutex_) \
  cc_mutex_destruct(static_cast<cc_mutex*>(_mymutex_));     \
  _mymutex_ = NULL

#define CC_MUTEX_LOCK(_mymutex_) \
  cc_mutex_lock(static_cast<cc_mutex *>(_mymutex_))

#define CC_MUTEX_UNLOCK(_mymutex_) \
  cc_mutex_unlock(static_cast<cc_mutex *>(_mymutex_))

#define CC_SYNC_BEGIN(_myid_) \
  void * coin_mydummysyncptr = cc_sync_begin((void*) _myid_)

#define CC_SYNC_END(_myid_) \
  cc_sync_end(coin_mydummysyncptr)

#define CC_GLOBAL_LOCK cc_mutex_global_lock()
#define CC_GLOBAL_UNLOCK cc_mutex_global_unlock()

#else /* ! HAVE_THREADS */

#define CC_MUTEX_CONSTRUCT(_mymutex_)  do { } while (0)
#define CC_MUTEX_DESTRUCT(_mymutex_)  do { } while (0)
#define CC_MUTEX_LOCK(_mymutex_)  do { } while (0)
#define CC_MUTEX_UNLOCK(_mymutex_)  do { } while (0)
#define CC_SYNC_BEGIN(_myid_)  do { } while (0)
#define CC_SYNC_END(_myid_)  do { } while (0)
#define CC_GLOBAL_LOCK  do { } while (0)
#define CC_GLOBAL_UNLOCK  do { } while (0)

#endif /* ! HAVE_THREADS */

#endif /* CC_THREADUTILP_H */
