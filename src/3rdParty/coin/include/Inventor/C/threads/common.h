#ifndef CC_THREADCOMMON_H
#define CC_THREADCOMMON_H

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

#include <Inventor/C/basic.h>  /* COIN_DLL_API */

/* ********************************************************************** */

/* Implementation note: it is important that this header file can be
   included even when Coin was built with no threads support.

   (This simplifies client code, as we get away with far less #ifdef
   HAVE_THREADS wrapping.) */

/* ********************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ********************************************************************** */

  typedef struct cc_sched cc_sched;
  typedef struct cc_wpool cc_wpool;
  typedef struct cc_worker cc_worker;
  typedef struct cc_thread cc_thread;
  typedef struct cc_mutex cc_mutex;
  typedef struct cc_rwmutex cc_rwmutex;
  typedef struct cc_condvar cc_condvar;
  typedef struct cc_storage cc_storage;
  typedef struct cc_fifo cc_fifo;
  typedef struct cc_barrier cc_barrier;
  typedef struct cc_recmutex cc_recmutex;

  /* used by rwmutex - read_precedence is default */
  enum cc_precedence {
    CC_READ_PRECEDENCE,
    CC_WRITE_PRECEDENCE
  };

  enum cc_threads_implementation {
    CC_NO_THREADS = -1,
    CC_PTHREAD    = 0,
    CC_W32THREAD
  };

  enum cc_retval {
    CC_ERROR = 0,
    CC_OK = 1,
    CC_TIMEOUT,
    CC_BUSY
  };

  typedef enum cc_precedence cc_precedence;
  typedef enum cc_threads_implementation cc_threads_implementation;
  typedef enum cc_retval cc_retval;

  /* ********************************************************************** */

  COIN_DLL_API int cc_thread_implementation(void);

  /* ********************************************************************** */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* ! CC_THREADCOMMON_H */
