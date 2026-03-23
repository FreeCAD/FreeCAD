#ifndef CC_LIST_H
#define CC_LIST_H

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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ********************************************************************** */

typedef struct cc_list cc_list;

COIN_DLL_API cc_list * cc_list_construct(void);
COIN_DLL_API cc_list * cc_list_construct_sized(int size);
COIN_DLL_API cc_list * cc_list_clone(cc_list * list);
COIN_DLL_API void cc_list_destruct(cc_list * list);

COIN_DLL_API void cc_list_append(cc_list * list, void * item);
COIN_DLL_API int cc_list_find(cc_list * list, void * item);
COIN_DLL_API void cc_list_insert(cc_list * list, void * item, int pos);
COIN_DLL_API void cc_list_remove(cc_list * list, int pos);
COIN_DLL_API void cc_list_remove_item(cc_list * list, void * item);
COIN_DLL_API void cc_list_remove_fast(cc_list * list, int pos);
COIN_DLL_API void cc_list_fit(cc_list * list);
COIN_DLL_API void cc_list_truncate(cc_list * list, int length);
COIN_DLL_API void cc_list_truncate_fit(cc_list * list, int length);

COIN_DLL_API int cc_list_get_length(cc_list * list);
COIN_DLL_API void ** cc_list_get_array(cc_list * list);
COIN_DLL_API void * cc_list_get(cc_list * list, int itempos);

COIN_DLL_API void cc_list_push(cc_list * list, void * item);
COIN_DLL_API void * cc_list_pop(cc_list * list);

/* ********************************************************************** */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* ! CC_LIST_H */
