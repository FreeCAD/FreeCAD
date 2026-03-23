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

#include <Inventor/C/base/list.h>

#include <cassert>
#include <cstddef>
#include <cstdlib>

#include "coindefs.h"

#define CC_LIST_DEFAULT_SIZE 4

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::malloc;
using std::free;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

/* ********************************************************************** */

/* FIXME: consider making this struct public to enable users to have
   lists on the stack */
struct cc_list {
  int itembuffersize;
  int numitems;
  void ** itembuffer;
  void * builtinbuffer[CC_LIST_DEFAULT_SIZE];
};

/* ********************************************************************** */

static void 
list_grow(cc_list * list) 
{
  int i, n;
  void ** newbuffer;
  list->itembuffersize <<= 1;

  newbuffer = static_cast<void**>(malloc(list->itembuffersize*sizeof(void*)));
  
  n = list->numitems;
  for (i = 0; i < n; i++) newbuffer[i] = list->itembuffer[i];
  if (list->itembuffer != list->builtinbuffer) {
    free(list->itembuffer);
  }
  list->itembuffer = newbuffer;
}

/* ********************************************************************** */

cc_list *
cc_list_construct(void)
{
  return cc_list_construct_sized(CC_LIST_DEFAULT_SIZE);
}

cc_list *
cc_list_construct_sized(int size)
{
  cc_list * list = static_cast<cc_list*>(malloc(sizeof(cc_list)));
  assert(list);
  if (size > CC_LIST_DEFAULT_SIZE) {
    list->itembuffer = static_cast<void**>(malloc(sizeof(void*)*size));
    assert(list->itembuffer);
    list->itembuffersize = size;
  }
  else {
    list->itembuffer = list->builtinbuffer;
    list->itembuffersize = CC_LIST_DEFAULT_SIZE;
  }
  list->numitems = 0;
  return list;
}

cc_list * 
cc_list_clone(cc_list * list)
{
  int i;
  cc_list * cloned = cc_list_construct_sized(list->numitems);

  for (i = 0; i < list->numitems; i++) {
    cloned->itembuffer[i] = list->itembuffer[i];
  }
  cloned->numitems = list->numitems;
  return cloned;
}

void
cc_list_destruct(cc_list * list)
{
  if (list->itembuffer != list->builtinbuffer) {
    free(list->itembuffer);
  }
  free(list);
}

void
cc_list_append(cc_list * list, void * item)
{
  if (list->numitems == list->itembuffersize) {
    list_grow(list);
  }
  list->itembuffer[list->numitems++] = item;
}

int
cc_list_find(cc_list * list, void * item)
{
  int i, n = list->numitems;
  for (i = 0; i < n; i++) {
    if (list->itembuffer[i] == item) return i;
  }
  return -1;
}

void
cc_list_insert(cc_list * list, void * item, int insertbefore)
{
  int i;
#ifdef COIN_EXTRA_DEBUG
  assert(insertbefore >= 0 && insertbefore <= list->numitems);
#endif /* COIN_EXTRA_DEBUG */
  if (list->numitems == list->itembuffersize) {
    list_grow(list);
  }  
  for (i = list->numitems; i > insertbefore; i--) {
    list->itembuffer[i] = list->itembuffer[i-1];
  }
  list->itembuffer[insertbefore] = item;
  list->numitems++;
}

void
cc_list_remove(cc_list * list, int index)
{
  int i;
#ifdef COIN_EXTRA_DEBUG
  assert(index >= 0 && index < list->numitems);
#endif /* COIN_EXTRA_DEBUG */
  list->numitems--;
  for (i = index; i < list->numitems; i++) {
    list->itembuffer[i] = list->itembuffer[i + 1];
  }
}

void
cc_list_remove_item(cc_list * list, void * item)
{
  int idx = cc_list_find(list, item);
#ifdef COIN_EXTRA_DEBUG
  assert(idx != -1);
#endif /* COIN_EXTRA_DEBUG */
  cc_list_remove(list, idx);
}

void
cc_list_remove_fast(cc_list * list, int index)
{
#ifdef COIN_EXTRA_DEBUG
  assert(index >= 0 && index < list->numitems);
#endif /* COIN_EXTRA_DEBUG */
  list->itembuffer[index] = list->itembuffer[--list->numitems];
}


void
cc_list_fit(cc_list * list)
{
  int i;
  int items = list->numitems;
  
  if (items < list->itembuffersize) {
    void ** newitembuffer = list->builtinbuffer;
    if (items > CC_LIST_DEFAULT_SIZE) newitembuffer = static_cast<void**>(malloc(sizeof(void*)*items));
    
    if (newitembuffer != list->itembuffer) {
      for (i = 0; i < items; i++) {
        newitembuffer[i] = list->itembuffer[i];
      }
    }
    
    if (list->itembuffer != list->builtinbuffer) {
      free(list->itembuffer);
    }
    list->itembuffer = newitembuffer;
    list->itembuffersize = items > CC_LIST_DEFAULT_SIZE ? items : CC_LIST_DEFAULT_SIZE;
  }
}

void
cc_list_truncate(cc_list * list, int length)
{
#ifdef COIN_EXTRA_DEBUG
  assert(length <= list->numitems);
#endif /* COIN_EXTRA_DEBUG */
  list->numitems = length;
}

void
cc_list_truncate_fit(cc_list * list, int length)
{
#ifdef COIN_EXTRA_DEBUG
  assert(length <= list->numitems);
#endif /* COIN_EXTRA_DEBUG */
  list->numitems = length;
  cc_list_fit(list);
}

int
cc_list_get_length(cc_list * list)
{
  return list->numitems;
}

void **
cc_list_get_array(cc_list * list)
{
  return list->itembuffer;
}

void * 
cc_list_get(cc_list * list, int itempos)
{
#ifdef COIN_EXTRA_DEBUG
  assert(itempos < list->numitems);
#endif /* COIN_EXTRA_DEBUG */
  return list->itembuffer[itempos];
}

void
cc_list_push(cc_list * list, void * item)
{
  cc_list_append(list, item);
}

void *
cc_list_pop(cc_list * list)
{
#ifdef COIN_EXTRA_DEBUG
  assert(list->numitems > 0);
#endif /* COIN_EXTRA_DEBUG */
  return list->itembuffer[--list->numitems];
}

#undef CC_LIST_DEFAULT_SIZE

/* ********************************************************************** */
