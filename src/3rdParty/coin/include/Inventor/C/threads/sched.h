#ifndef CC_SCHED_H
#define CC_SCHED_H

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
#include <Inventor/C/threads/common.h>  /* cc_sched */

/* ********************************************************************** */

/* Implementation note: it is important that this header file can be
   included even when Coin was built with no threads support.

   (This simplifies client code, as we get away with far less #ifdef
   HAVE_THREADS wrapping.) */

/* ********************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void cc_sched_f(void * closure);

/* ********************************************************************** */

  COIN_DLL_API cc_sched * cc_sched_construct(int numthreads);
  COIN_DLL_API void cc_sched_destruct(cc_sched * sched);
  COIN_DLL_API void cc_sched_set_num_threads(cc_sched * sched, int num);
  COIN_DLL_API int cc_sched_get_num_threads(cc_sched * sched);

  COIN_DLL_API uint32_t cc_sched_schedule(cc_sched * sched, 
                                          cc_sched_f * workfunc, 
                                          void * closure,
                                          float priority);
  COIN_DLL_API SbBool cc_sched_unschedule(cc_sched * sched, 
                                          uint32_t schedid); 
  COIN_DLL_API void cc_sched_wait_all(cc_sched * sched);
  COIN_DLL_API int cc_sched_get_num_remaining(cc_sched * sched);
  COIN_DLL_API void cc_sched_set_num_allowed(cc_sched * sched, int num);
  COIN_DLL_API void cc_sched_change_priority(cc_sched * sched, 
                                             uint32_t schedid, 
                                             float priority);

/* ********************************************************************** */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* ! CC_SCHED_H */
