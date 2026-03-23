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

/*!
  \struct cc_fifo common.h Inventor/C/threads/common.h
  \ingroup coin_threads
  \brief The structure for a first-in, first-out queue.
*/

/*!
  \typedef struct cc_fifo cc_fifo
  \ingroup coin_threads
  \brief The type definition for the first-in, first-out queue structure.
*/

#include <Inventor/C/threads/fifo.h>

#include <cstdlib>
#include <cassert>

#include <Inventor/C/threads/mutex.h>
#include <Inventor/C/threads/condvar.h>
#include <Inventor/C/errors/debugerror.h>

#include "threads/fifop.h"
#include "threads/mutexp.h"
#include "threads/condvarp.h"

/*
  The cc_fifo is a C datatype for managing a pointer first-in,
  first-out queue.
*/

/* ********************************************************************** */

static cc_fifo_item * cc_fifo_item_new(void);
static void cc_fifo_item_delete(cc_fifo_item * item);

static cc_fifo_item * i_get_free_item(cc_fifo * fifo);
static void i_append(cc_fifo * fifo, cc_fifo_item * item);
static cc_fifo_item * i_unlink_head(cc_fifo * fifo);

/* ********************************************************************** */

void
cc_fifo_struct_init(cc_fifo * fifo)
{
  assert(fifo != NULL);
  cc_mutex_struct_init(&fifo->access);
  fifo->head = NULL;
  fifo->tail = NULL;
  fifo->free = NULL;
  fifo->elements = 0;
  cc_condvar_struct_init(&fifo->sleep);
}

void
cc_fifo_struct_clean(cc_fifo * fifo)
{
  cc_fifo_item * item, * next;
  assert(fifo != NULL);
  cc_mutex_struct_clean(&fifo->access);
  /* free fifo list */
  item = fifo->head;
  while ( item != NULL ) {
    next = item->next;
    cc_fifo_item_delete(item);
    item = next;
  }
  /* free free list */
  item = fifo->free;
  while ( item != NULL ) {
    next = item->next;
    cc_fifo_item_delete(item);
    item = next;
  }
  cc_condvar_struct_clean(&fifo->sleep);
}

/* ********************************************************************** */

/*! Constructs a new first-in, first-out queue. */
cc_fifo *
cc_fifo_new(void)
{
  cc_fifo * fifo;
  fifo = (cc_fifo*) malloc(sizeof(cc_fifo));
  cc_fifo_struct_init(fifo);
  return fifo;
}

/*! Destroys the \a fifo first-in, first-out queue. */
void
cc_fifo_delete(cc_fifo * fifo)
{
  cc_fifo_struct_clean(fifo);
  free(fifo);
}

/* ********************************************************************** */

/*! Appends the \a ptr of type \a type to the end of the \a fifo
    first-in, first-out queue. */
void
cc_fifo_assign(cc_fifo * fifo, void * ptr, uint32_t type)
{
  cc_fifo_item * item;
  assert(fifo != NULL);
  cc_mutex_lock(&fifo->access);
  item = i_get_free_item(fifo);
  item->item = ptr;
  item->type = type;
  i_append(fifo, item);
  cc_condvar_wake_one(&fifo->sleep);
  cc_mutex_unlock(&fifo->access);
}

/*! Retrieves the first item from the \a fifo
    first-in, first-out queue. */
void
cc_fifo_retrieve(cc_fifo * fifo, void ** ptr, uint32_t * type)
{
  cc_fifo_item * item;
  assert(fifo != NULL && ptr != NULL);
  cc_mutex_lock(&fifo->access);
  while ( TRUE ) {
    if ( fifo->elements == 0 ) {
      cc_condvar_wait(&fifo->sleep, &fifo->access);
    } else {
      item = i_unlink_head(fifo);
      *ptr = item->item;
      if ( type != NULL ) *type = item->type;
      item->next = fifo->free;
      fifo->free = item;
      cc_mutex_unlock(&fifo->access);
      cc_condvar_wake_one(&fifo->sleep);
      return;
    }
  }
}

/*! Checks the \a fifo first-in, first-out queue to see if an item can be
    retrieved. If so the properties of the first available item are returned. */
SbBool
cc_fifo_try_retrieve(cc_fifo * fifo, void ** ptr, uint32_t * type)
{
  cc_fifo_item * item;
  assert(fifo != NULL && ptr != NULL);
  /* FIXME: consider cc_mutex_try_lock()? to escape even a failed lock */
  if ( ! cc_mutex_try_lock(&fifo->access) ) {
    return FALSE;
  }
  if ( fifo->elements == 0 ) {
    cc_mutex_unlock(&fifo->access);
    return FALSE;
  }
  item = i_unlink_head(fifo);
  *ptr = item->item;
  if ( type != NULL ) *type = item->type;
  cc_fifo_item_delete(item);
  cc_mutex_unlock(&fifo->access);
  cc_condvar_wake_one(&fifo->sleep);
  return TRUE;
}

/* ********************************************************************** */

/*! Returns the number of elements in the \a fifo first-in, first-out queue. */
unsigned int
cc_fifo_size(cc_fifo * fifo)
{
  assert(fifo != NULL);
  return fifo->elements;
}

/* ********************************************************************** */

cc_fifo_item *
cc_fifo_item_new(void) /* static */
{
  cc_fifo_item * item;
  item = (cc_fifo_item*) malloc(sizeof(cc_fifo_item));
  assert(item != NULL);
  item->next = NULL;
  item->item = NULL;
  item->type = 0;
  return item;
}

void
cc_fifo_item_delete(cc_fifo_item * item) /* static */
{
  assert(item != NULL);
  free(item);
}

/* ********************************************************************** */

/*! Locks the \a fifo first-in, first-out queue. */
void
cc_fifo_lock(cc_fifo * fifo)
{
  assert(fifo != NULL);
  cc_mutex_lock(&fifo->access);
}

SbBool
cc_fifo_try_lock(cc_fifo * fifo)
{
  assert(fifo != NULL);
  return cc_mutex_try_lock(&fifo->access);
}

/*! Unlocks the \a fifo first-in, first-out queue. */
void
cc_fifo_unlock(cc_fifo * fifo)
{
  assert(fifo != NULL);
  cc_mutex_unlock(&fifo->access);
}

/* ********************************************************************** */

/*! Returns the properties of the first item in the \a fifo first-in, first-out queue. */
SbBool
cc_fifo_peek(cc_fifo * fifo, void ** item, uint32_t * type)
{
  assert(fifo != NULL);
  if ( fifo->head == NULL ) return FALSE;
  *item = fifo->head->item;
  if ( type != NULL ) *type = fifo->head->type;
  return TRUE;
}

/*! Checks if the \a fifo first-in, first-out queue contains the \a itemptr item. */
SbBool
cc_fifo_contains(cc_fifo * fifo, void * itemptr)
{
  cc_fifo_item * item;
  assert(fifo != NULL);
  item = fifo->head;
  while ( item != NULL ) {
    if ( item->item == itemptr ) return TRUE;
    item = item->next;
  }
  return FALSE;
}

/*! Removes from the \a fifo first-in, first-out queue the \a itemptr item if present. */
SbBool
cc_fifo_reclaim(cc_fifo * fifo, void * itemptr)
{
  cc_fifo_item * item, * prev;
  assert(fifo != NULL);
  item = fifo->head;
  prev = NULL;
  while ( item != NULL ) {
    if ( item->item == itemptr ) {
      if ( prev == NULL ) fifo->head = item->next;
      else prev->next = item->next;
      if ( fifo->tail == item ) fifo->tail = prev;
      /* and reset/store the container */
      item->item = NULL;
      item->type = 0;
      item->next = fifo->free;
      fifo->free = item;
      return TRUE;
    }
    prev = item;
    item = item->next;
  }
  return FALSE;
}

/* ********************************************************************** */

/*
  get item from free list or construct a new one
*/

cc_fifo_item *
i_get_free_item(cc_fifo * fifo) /* static */
{
  cc_fifo_item * item;
  if ( fifo->free != NULL ) {
    item = fifo->free;
    fifo->free = item->next;
    item->next = NULL;
  } else {
    item = cc_fifo_item_new();
  }
  return item;
}

/*
  append item to fifo.  make sure both ::head and ::tail are correct
  after.
*/

void
i_append(cc_fifo * fifo, cc_fifo_item * item) /* static */
{
  if ( fifo->tail == NULL ) {
    fifo->head = item;
    fifo->tail = item;
  } else {
    fifo->tail->next = item;
    fifo->tail = item;
  }
  fifo->elements += 1;
}

/*
  unlink first item from fifo.  make sure both ::head and ::tail are
  correct after.
*/

cc_fifo_item *
i_unlink_head(cc_fifo * fifo) /* static */
{
  cc_fifo_item * item;
  item = fifo->head;
  fifo->head = item->next;
  if ( fifo->head == NULL )
    fifo->tail = NULL;
  fifo->elements -= 1;
  return item;
}
