#ifndef CC_HASH_H
#define CC_HASH_H

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

/* This interface is OBSOLETE -- please do not use it in your
   application code, as it will be removed from Coin 3 and onwards. */

#include <stdlib.h>
#include <Inventor/C/basic.h>

#if defined(COIN_INTERNAL) && !defined(COIN_ALLOW_CC_HASH)
#error prefer cc_dict over cc_hash for internal code
#endif /* COIN_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  typedef uintptr_t cc_hash_key;
  typedef struct cc_hash cc_hash;
  typedef cc_hash_key cc_hash_func(const cc_hash_key key);
  typedef void cc_hash_apply_func(cc_hash_key key, void * val, void * closure);

  COIN_DLL_API cc_hash * cc_hash_construct(unsigned int size, float loadfactor);
  COIN_DLL_API void cc_hash_destruct(cc_hash * ht);
  COIN_DLL_API void cc_hash_clear(cc_hash * ht);

  COIN_DLL_API SbBool cc_hash_put(cc_hash * ht, cc_hash_key key, void * val);
  COIN_DLL_API SbBool cc_hash_get(cc_hash * ht, cc_hash_key key, void ** val);
  COIN_DLL_API SbBool cc_hash_remove(cc_hash * ht, cc_hash_key key);
  COIN_DLL_API void cc_hash_apply(cc_hash * ht, cc_hash_apply_func * func, void * closure);

  COIN_DLL_API unsigned int cc_hash_get_num_elements(cc_hash * ht);

  COIN_DLL_API void cc_hash_set_hash_func(cc_hash * ht, cc_hash_func * func);
  COIN_DLL_API void cc_hash_print_stat(cc_hash * ht);

/* ********************************************************************** */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* ! CC_HASH_H */
