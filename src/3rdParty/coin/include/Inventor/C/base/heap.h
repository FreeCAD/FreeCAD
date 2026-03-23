#ifndef CC_HEAP_H
#define CC_HEAP_H

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

#include <Inventor/C/basic.h>
#include <Inventor/SbString.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  typedef int cc_heap_compare_cb(void * o1, void * o2);
  typedef void cc_heap_print_cb(void * o, SbString& str);

  typedef struct cc_heap cc_heap;

  COIN_DLL_API cc_heap * cc_heap_construct(unsigned int size,
                                           cc_heap_compare_cb * comparecb,
                                           SbBool support_remove);
  
  COIN_DLL_API void cc_heap_destruct(cc_heap * h);
  COIN_DLL_API void cc_heap_clear(cc_heap * h);
  
  COIN_DLL_API void cc_heap_add(cc_heap * h, void * o);
  COIN_DLL_API void * cc_heap_get_top(cc_heap * h);
  COIN_DLL_API void * cc_heap_extract_top(cc_heap * h);
  COIN_DLL_API int cc_heap_remove(cc_heap * h, void * o);
  COIN_DLL_API int cc_heap_update(cc_heap * h, void * o);
  COIN_DLL_API unsigned int cc_heap_elements(cc_heap * h);
  COIN_DLL_API SbBool cc_heap_empty(cc_heap * h);
  COIN_DLL_API void cc_heap_print(cc_heap * h, cc_heap_print_cb * printcb, SbString& str, SbBool printLeveled = FALSE);

/* ********************************************************************** */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* ! CC_HEAP_H */
