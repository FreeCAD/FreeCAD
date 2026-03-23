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

#include "base/dict.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <Inventor/C/tidbits.h>
#include <Inventor/C/errors/debugerror.h>

#include "tidbitsp.h"
#include "base/dictp.h"
#include "coindefs.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::free;
using std::malloc;
using std::calloc;
using std::memset;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

/* ********************************************************************** */
/* private functions */

extern "C" {

static uintptr_t
dict_default_hashfunc(const uintptr_t key)
{
  return key;
}

} // extern "C"

static unsigned int
dict_get_index(cc_dict * ht, uintptr_t key)
{
  assert(ht != NULL);
  key = ht->hashfunc(key);
  return (unsigned int) (key % ht->size);
}

static void
dict_resize(cc_dict * ht, unsigned int newsize)
{
  cc_dict_entry ** oldbuckets = ht->buckets;
  unsigned int oldsize = ht->size, i;

  /* Never shrink the table */
  if (ht->size >= newsize)
    return;

  ht->size = newsize;
  ht->elements = 0;
  ht->threshold = (unsigned int) (newsize * ht->loadfactor);
  ht->buckets = (cc_dict_entry **) calloc(newsize, sizeof(cc_dict_entry*));

  /* Transfer all mappings */
  for (i = 0; i < oldsize; i++) {
    cc_dict_entry * he = oldbuckets[i];
    while (he) {
      cc_dict_put(ht, he->key, he->val);
      he = he->next;
    }
  }
  free(oldbuckets);
}

/* ********************************************************************** */
/* public api */

/*!
  Construct a hash table.

  \a size is the initial bucket size. The caller need not attempt to
  find a good (prime number) value for this argument to ensure good
  hashing. That will be taken care of internally.

  \a loadfactor is the percentage the table should be filled before
  resizing, and should be a number from 0 to 1. It is of course
  possible to specify a number bigger than 1, but then there will be
  greater chance of having many elements on the same bucket (linear
  search for an element). If you supply a number <= 0 for loadfactor,
  the default value 0.75 will be used.
*/
cc_dict *
cc_dict_construct(unsigned int size, float loadfactor)
{
  unsigned int s;
  cc_dict * ht = (cc_dict *) malloc(sizeof(cc_dict));
  
  s = (unsigned int) coin_geq_prime_number(size);
  if (loadfactor <= 0.0f) loadfactor = 0.75f;
  
  ht->size = s;
  ht->elements = 0;
  ht->threshold = (unsigned int) (s * loadfactor);
  ht->loadfactor = loadfactor;
  ht->buckets = (cc_dict_entry **) calloc(s, sizeof(cc_dict_entry*));
  ht->hashfunc = dict_default_hashfunc;
  /* we use a memory allocator to avoid an operating system malloc
     every time a new entry is needed */
  ht->memalloc = cc_memalloc_construct(sizeof(cc_dict_entry));
  return ht;
}

/*!
  Destruct the hash table \a ht.
*/
void
cc_dict_destruct(cc_dict * ht)
{
  cc_dict_clear(ht);
  cc_memalloc_destruct(ht->memalloc);
  free(ht->buckets);
  free(ht);
}

/*!
  Clear/remove all elements in the hash table \a ht.
*/
void
cc_dict_clear(cc_dict * ht)
{
  // cc_memalloc_clear() will free memory used by internal
  // structures. To avoid continuous memory allocation/deallocation
  // that could be bad for performance (cc_dict is used in
  // SoSensorManager) we manually free all entries from cc_memalloc
  // instead.
#if 0 // disabled
  cc_memalloc_clear(ht->memalloc); /* free all memory used by all entries */
#else // new version that will not trigger any malloc()/free() calls
  unsigned int i;
  cc_dict_entry * entry;
  cc_dict_entry * next;
  for (i = 0; i < ht->size; i++) {
    entry = ht->buckets[i];
    while (entry) {
      next = entry->next;
      cc_memalloc_deallocate(ht->memalloc, entry);
      entry = next;
    }
  }
#endif // new version

  // all memory has been freed. Just clear buckets
  memset(ht->buckets, 0, ht->size * sizeof(cc_dict_entry*));
  ht->elements = 0;
}

/*!

  Insert a new element in the hash table \a ht. \a key is the key used
  to identify the element, while \a val is the element value. If \a
  key is already used by another element, the element value will be
  overwritten, and \e FALSE is returned. Otherwise a new element is
  created and \e TRUE is returned.

 */
SbBool
cc_dict_put(cc_dict * ht, uintptr_t key, void * val)
{
  unsigned int i = dict_get_index(ht, key);
  cc_dict_entry * he = ht->buckets[i];

  while (he) {
    if (he->key == key) {
      /* Replace the old value */
      he->val = val;
      return FALSE;
    }
    he = he->next;
  }

  /* Key not already in the hash table; insert a new
   * entry as the first element in the bucket
   */
  he = (cc_dict_entry *) cc_memalloc_allocate(ht->memalloc);
  he->key = key;
  he->val = val;
  he->next = ht->buckets[i];
  ht->buckets[i] = he;
  
  if (ht->elements++ >= ht->threshold) {
    dict_resize(ht, (unsigned int) coin_geq_prime_number(ht->size + 1));
  }
  return TRUE;
}

/*!

  Find the element with key value \a key. If found, the value is written to
  \a val, and TRUE is returned. Otherwise FALSE is returned and \a val
  is not changed.

*/
SbBool
cc_dict_get(cc_dict * ht, uintptr_t key, void ** val)
{
  cc_dict_entry * he;
  unsigned int i = dict_get_index(ht, key);
  he = ht->buckets[i];
  while (he) {
    if (he->key == key) {
      *val = he->val;
      return TRUE;
    }
    he = he->next;
  }
  return FALSE;
}

/*!
  Attempt to remove the element with key value \a key. Returns
  TRUE if found, FALSE otherwise.
*/
SbBool
cc_dict_remove(cc_dict * ht, uintptr_t key)
{
  cc_dict_entry * he, *next, * prev;
  unsigned int i = dict_get_index(ht, key);

  he = ht->buckets[i];
  prev = NULL;
  while (he) {
    next = he->next;
    if (he->key == key) {
      ht->elements--;
      if (prev == NULL) {
        ht->buckets[i] = next;
      }
      else {
        prev->next = next;
      }
      cc_memalloc_deallocate(ht->memalloc, he);
      return TRUE;
    }
    prev = he;
    he = next;
  }
  return FALSE;
}

/*!
  Return the number of elements in the hash table.
*/
unsigned int
cc_dict_get_num_elements(cc_dict * ht)
{
  return ht->elements;
}

/*!
  Set the hash func that is used to map key values into
  a bucket index.
*/
void
cc_dict_set_hash_func(cc_dict * ht, cc_dict_hash_func * func)
{
  ht->hashfunc = func;
}

/*!
  Call \a func for for each element in the hash table.
*/
void
cc_dict_apply(cc_dict * ht, cc_dict_apply_func * func, void * closure)
{
  unsigned int i;
  cc_dict_entry * elem;
  for (i = 0; i < ht->size; i++) {
    elem = ht->buckets[i];
    while (elem) {
      func(elem->key, elem->val, closure);
      elem = elem->next;
    }
  }
}

/*!
  For debugging only. Prints information about hash with
  cc_debugerror.
*/
void
cc_dict_print_stat(cc_dict * ht)
{
  unsigned int i, used_buckets = 0, max_chain_l = 0;
  for (i = 0; i < ht->size; i++) {
    if (ht->buckets[i]) {
      unsigned int chain_l = 0;
      cc_dict_entry * he = ht->buckets[i];
      used_buckets++;
      while (he) {
        chain_l++;
        he = he->next;
      }
      if (chain_l > max_chain_l) max_chain_l = chain_l;
    }
  }
  cc_debugerror_postinfo("cc_dict_print_stat",
                         "Used buckets %u of %u (%u elements), "
                         "avg chain length: %.2f, max chain length: %u\n",
                         used_buckets, ht->size, ht->elements,
                         (float)ht->elements / used_buckets, max_chain_l);
}
