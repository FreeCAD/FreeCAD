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

#include <Inventor/C/base/memalloc.h>

#include <cstdlib>
#include <cstddef>
#include <cassert>
#include <cstdio>

#include "coindefs.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::malloc;
using std::free;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

/* ********************************************************************** */

/*!
  \struct cc_memalloc memalloc.h Inventor/C/base/memalloc.h

  The allocator structure for memory.
*/

/*!
  \typedef struct cc_memalloc cc_memalloc

  A type definition for the memory allocator structure.
*/

/*!
  \typedef int cc_memalloc_strategy_cb(const int numunits_allocated)

  The type definition for the memory allocator strategy callback function.
*/

/* internal struct used to store a linked list of free'ed items */
struct cc_memalloc_free {
  struct cc_memalloc_free * next;
};

typedef struct cc_memalloc_free cc_memalloc_free;

/* internal struct used to organize a block of allocated memory */
struct cc_memalloc_memnode {
  struct cc_memalloc_memnode * next;
  unsigned char * block;
  unsigned int currpos;
  unsigned int size;
};

typedef struct cc_memalloc_memnode cc_memalloc_memnode;

/* allocator struct */
struct cc_memalloc {

  cc_memalloc_free * free;
  cc_memalloc_memnode * memnode;

  unsigned int chunksize;

  unsigned int num_allocated_units;
  cc_memalloc_strategy_cb * strategy;
};

/*
 * allocate 'numbytes' bytes from 'memnode'. Returns NULL if
 * the memory node is full.
 */
static void *
node_alloc(struct cc_memalloc_memnode * memnode, const int numbytes)
{
  unsigned char * ret = NULL;
  if (memnode->currpos + numbytes <= memnode->size) {
    ret = memnode->block + memnode->currpos;
    memnode->currpos += numbytes;
  }
  return ret;
}

/*
 * creates a new memory node for the allocator. Sets the next
 * pointer to the current memnode in allocator.
 */
static struct cc_memalloc_memnode *
create_memnode(cc_memalloc * allocator)
{
  unsigned int numbytes;
  int chunkmultiplier;
  cc_memalloc_memnode * node =
    (cc_memalloc_memnode*) malloc(sizeof(cc_memalloc_memnode));

  chunkmultiplier = allocator->strategy(allocator->num_allocated_units);
  assert(chunkmultiplier >= 1 && "strategy callback returned erroneous value");
  numbytes = allocator->chunksize * chunkmultiplier;
  
  node->next = allocator->memnode;
  node->block = (unsigned char*) malloc(numbytes);
  node->currpos = 0;
  node->size = numbytes;

  return node;
}

/*
 * Allocate memory from the allocator's memnode. If the memnode is
 * full, a new memnode is created for the allocator.
*/
static void *
alloc_from_memnode(cc_memalloc * allocator)
{
  void * ret = NULL;

  if (allocator->memnode) ret = node_alloc(allocator->memnode, allocator->chunksize);
  if (ret == NULL) {
    allocator->memnode = create_memnode(allocator);
    ret = node_alloc(allocator->memnode, allocator->chunksize);
    /* FIXME: I've seen this assert() hit, but I couldn't easily
       reproduce it. (It hit for a system that was running a viewer
       spin overnight.) I've inserted additional assert() calls to try
       to catch the problem closer to the source. 20031008 mortene. */
    assert(ret);
  }
  return ret;
}

/*!
  Construct a memory allocator. Each allocated unit will be \a unitsize
  bytes.
*/
cc_memalloc *
cc_memalloc_construct(const unsigned int unitsize)
{
  cc_memalloc * allocator = (cc_memalloc*)
    malloc(sizeof(cc_memalloc));

  allocator->chunksize = unitsize;
  if (unitsize < sizeof(cc_memalloc_free)) {
    allocator->chunksize = sizeof(cc_memalloc_free);
  }
  allocator->free = NULL;
  allocator->memnode = NULL;
  allocator->num_allocated_units = 0;

  cc_memalloc_set_strategy(allocator, NULL); /* will insert default handler */

  return allocator;
}

/*!
  Destruct \a allocator, freeing all memory used.
*/
void
cc_memalloc_destruct(cc_memalloc * allocator)
{
  cc_memalloc_clear(allocator);
  free(allocator);
}

/*!
  Allocate a memory unit from \a allocator.
*/
void *
cc_memalloc_allocate(cc_memalloc * allocator)
{
  allocator->num_allocated_units++;
  if (allocator->free) {
    void * storage = allocator->free;
    allocator->free = allocator->free->next;
    return storage;
  }
  return alloc_from_memnode(allocator);
}

/*!
  Deallocate a memory unit. \a ptr must have been allocated using
  cc_memalloc_allocate(), of course.
*/
void
cc_memalloc_deallocate(cc_memalloc * allocator, void * ptr)
{
  cc_memalloc_free * newfree = (cc_memalloc_free*) ptr;
  allocator->num_allocated_units--;
  newfree->next = allocator->free;
  allocator->free = newfree;
}

/*!
  Free all memory allocated by \a allocator.
*/
void
cc_memalloc_clear(cc_memalloc * allocator)
{
  cc_memalloc_memnode * tmp;
  cc_memalloc_memnode * node = allocator->memnode;
  while (node) {
    tmp = node->next;
    free(node->block);
    free(node);
    node = tmp;
  }
  allocator->free = NULL;
  allocator->memnode = NULL;
}

extern "C" {

/* default strategy cb */
static int
default_strategy(const int numunits_allocated)
{
  if (numunits_allocated < 64) return 64;
  return numunits_allocated;
}

} // extern "C"

/*!
  Sets the allocator strategy callback. \c cb should be a function that
  returns the number of units to allocated in a block, based on the
  number of units currently allocated.

  The default strategy is to just return the number of units allocated
  (which will successively double the internal memory chunk sizes),
  unless the number of units allocated is less than 64, then 64 is
  returned.
*/
void
cc_memalloc_set_strategy(cc_memalloc * allocator, cc_memalloc_strategy_cb * cb)
{
  if (cb == NULL) allocator->strategy = default_strategy;
  else allocator->strategy = cb;
}
