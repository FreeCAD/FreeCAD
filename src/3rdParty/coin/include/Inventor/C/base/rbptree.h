#ifndef CC_RBPTREE_H
#define CC_RBPTREE_H

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

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct cc_rbptree_node cc_rbptree_node;

typedef struct cc_rbptree {
  cc_rbptree_node * root;
  
  /* store two items inline to avoid allocating memory for small tree */
  void * inlinepointer[2];
  void * inlinedata[2];
  uint32_t counter;
} cc_rbptree;

COIN_DLL_API void cc_rbptree_init(cc_rbptree * t);
COIN_DLL_API void cc_rbptree_clean(cc_rbptree * t);

COIN_DLL_API void cc_rbptree_insert(cc_rbptree * t, void * p, void * data);
COIN_DLL_API SbBool cc_rbptree_remove(cc_rbptree * t, void * p);
COIN_DLL_API uint32_t cc_rbptree_size(const cc_rbptree * t);

/* traverse all elements */
typedef void cc_rbptree_traversecb(void * p, void * data, void * closure);
COIN_DLL_API void cc_rbptree_traverse(const cc_rbptree * t, cc_rbptree_traversecb * func, void * closure);

/* only for debugging */
COIN_DLL_API void cc_rbptree_debug(const cc_rbptree * t);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* ! CC_RBPTREE_H */
