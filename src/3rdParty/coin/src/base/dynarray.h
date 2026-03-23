#ifndef COIN_DYNARRAY_H
#define COIN_DYNARRAY_H

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

/* This is the interface for a C ADT of a dynamic array. */

/*************************************************************************/

#ifndef COIN_INTERNAL
#error Only for internal use.
#endif /* COIN_INTERNAL */

/*************************************************************************/

#include <Inventor/C/basic.h>

/*************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct cc_dynarray cc_dynarray;

  cc_dynarray * cc_dynarray_new(void);
  cc_dynarray * cc_dynarray_duplicate(const cc_dynarray * src);
  void cc_dynarray_destruct(cc_dynarray * arr);

  unsigned int cc_dynarray_length(const cc_dynarray * arr);

  void cc_dynarray_append(cc_dynarray * arr, void * item);
  void cc_dynarray_insert(cc_dynarray * arr, void * item, unsigned int idx);

  int cc_dynarray_find(const cc_dynarray * arr, void * item);

  void * cc_dynarray_get(const cc_dynarray * arr, unsigned int idx);
  void cc_dynarray_set(cc_dynarray * arr, unsigned int idx, void * item);

  void cc_dynarray_remove(cc_dynarray * arr, void * item);
  void cc_dynarray_remove_idx(cc_dynarray * arr, unsigned int idx);
  void cc_dynarray_removefast(cc_dynarray * arr, unsigned int idx);

  void cc_dynarray_truncate(cc_dynarray * arr, unsigned int len);
  void cc_dynarray_fit(cc_dynarray * arr);

  void ** cc_dynarray_get_arrayptr(const cc_dynarray * arr);

  SbBool cc_dynarray_eq(const cc_dynarray * arr1, const cc_dynarray * arr2);
  
#ifdef __cplusplus
}
#endif

#endif /* !COIN_DYNARRAY_H */
