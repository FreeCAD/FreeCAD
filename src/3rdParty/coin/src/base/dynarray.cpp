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

// FIXME: this implements a C ADT for a dynamic array of void
// pointers. It's basically just a quick hack of wrapping the C ADT
// around our SbPList C++ class. We should really correct this to be
// the other way around: we should make a generic C ADT for a dynamic
// array, which is then wrapped by SbList.
//
// Don't make this class public until the C rewrite happens, and the
// API has been audited. This ADT must have been reimplemented a
// zillion times around in various projects, so check out how our API
// design fares versus others.
//
// 20030604 mortene.
//
// UPDATE 20030915 mortene: this had actually been implemented in C
// already as cc_list...

#include "base/dynarray.h"

#include <Inventor/lists/SbPList.h>

struct cc_dynarray {
  SbPList plist;
};

cc_dynarray *
cc_dynarray_new(void)
{
  return new struct cc_dynarray;
}

cc_dynarray *
cc_dynarray_duplicate(const cc_dynarray * src)
{
  cc_dynarray * p = cc_dynarray_new();
  p->plist = src->plist;
  return p;
}

void
cc_dynarray_destruct(cc_dynarray * arr)
{
  delete arr;
}

void
cc_dynarray_fit(cc_dynarray * arr)
{
  arr->plist.fit();
}

void
cc_dynarray_append(cc_dynarray * arr, void * item)
{
  arr->plist.append(item);
}

int
cc_dynarray_find(const cc_dynarray * arr, void * item)
{
  return arr->plist.find(item);
}

void
cc_dynarray_insert(cc_dynarray * arr, void * item, unsigned int idx)
{
  arr->plist.insert(item, idx);
}

void
cc_dynarray_remove(cc_dynarray * arr, void * item)
{
  arr->plist.removeItem(item);
}

void
cc_dynarray_remove_idx(cc_dynarray * arr, unsigned int idx)
{
  arr->plist.remove(idx);
}

void
cc_dynarray_removefast(cc_dynarray * arr, unsigned int idx)
{
  arr->plist.removeFast(idx);
}

unsigned int
cc_dynarray_length(const cc_dynarray * arr)
{
  return arr->plist.getLength();
}

void
cc_dynarray_truncate(cc_dynarray * arr, unsigned int len)
{
  arr->plist.truncate(len);
}

void **
cc_dynarray_get_arrayptr(const cc_dynarray * arr)
{
  return arr->plist.getArrayPtr();
}

void *
cc_dynarray_get(const cc_dynarray * arr, unsigned int idx)
{
  return arr->plist[idx];
}

SbBool
cc_dynarray_eq(const cc_dynarray * arr1, const cc_dynarray * arr2)
{
  return (arr1->plist == arr2->plist);
}

void
cc_dynarray_set(cc_dynarray * arr, unsigned int idx, void * item)
{
  arr->plist.set(idx, item);
}
