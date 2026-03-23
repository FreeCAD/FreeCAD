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

#include <Inventor/C/base/heap.h>

#include <cassert>
#include <cstdlib>
#include <cstdio>

#include "base/dict.h"
#include "base/heapp.h"
#include "coindefs.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::realloc;
using std::malloc;
using std::free;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

/* ********************************************************************** */

/*!
  \typedef int cc_heap_compare_cb(void * o1, void * o2)

  A type definition for heap compare callback function.
  The heap compare function yields true if the first argument of the call
  appears before the second in the strict weak ordering relation 
  induced by this type, and false otherwise, i.e. cc_heap_compare_cb(x, x) == false
*/

/*! 
  \typedef struct cc_heap cc_heap

  A type definition for the cc_heap structure
*/

/* ********************************************************************** */
/* private functions */

#define HEAP_PARENT(i) (((i) - 1) / 2)
#define HEAP_LEFT(i) ((i) * 2 + 1)
#define HEAP_RIGHT(i) ((i) * 2 + 2)

static void
heap_resize(cc_heap * h, unsigned int newsize)
{
  /* Never shrink the heap */
  if (h->size >= newsize)
    return;

  h->array = static_cast<void **>(realloc(h->array, newsize * sizeof(void *)));
  assert(h->array);
  h->size = newsize;
}

static void
heap_swap(void ** left, void ** right) {
  void * tmp = *left;
  *left = *right;
  *right = tmp;
}

static void
heap_heapify_down(cc_heap * h, uintptr_t i)
{
  uintptr_t largest = i;

  while (1) {
    i = largest;
    uintptr_t left = HEAP_LEFT(i);
    uintptr_t right = HEAP_RIGHT(i);

    /* Check which node is larger of i and its two children; if any
     * of them is larger swap it with i and proceed down on the child
     */
    if (left < h->elements && h->compare(h->array[left], h->array[largest]) > 0)
      largest = left;

    if (right < h->elements && h->compare(h->array[right], h->array[largest]) > 0)
      largest = right;

    if (largest == i)
      break;

    heap_swap(&h->array[i], &h->array[largest]);

    if (h->support_remove) {
      cc_dict_put(h->hash, reinterpret_cast<uintptr_t>(h->array[i]), reinterpret_cast<void *>(i));
      cc_dict_put(h->hash, reinterpret_cast<uintptr_t>(h->array[largest]), reinterpret_cast<void *>(largest));
    }
  }
}

static void
heap_heapify_up(cc_heap * h, uintptr_t i)
{
  if (i == 0)
    return;

  /* If o is greater than its parent, swap them and proceed up on the parent */
  while (i > 0 && h->compare(h->array[i], h->array[HEAP_PARENT(i)]) > 0) {
    uintptr_t parent = HEAP_PARENT(i);
    heap_swap(&h->array[i], &h->array[parent]);

    if (h->support_remove) {
      cc_dict_put(h->hash, reinterpret_cast<uintptr_t>(h->array[i]), reinterpret_cast<void*>(i));
      cc_dict_put(h->hash, reinterpret_cast<uintptr_t>(h->array[parent]), reinterpret_cast<void*>(parent));
    }

    i = parent;
  }
}

/* ********************************************************************** */
/* public api */

/*!

  Construct a heap. \a size is the initial array size.

  For a minimum heap \a comparecb should return 1 if the first element
  is less than the second, zero if they are equal or the first element
  is greater than the second.
  For a maximum heap \a comparecb should return 1 if the first element
  is greater than the second, zero if they are equal or the first element
  is less than the second.

  \a support_remove specifies if the heap should support removal of
  elements (other than the top element) after they are added; this
  requires use of a hash table to be efficient, but as a slight runtime
  overhead will be incurred for the add and extract_top functions the
  support can be disabled if you don't need it.

*/

cc_heap *
cc_heap_construct(unsigned int size,
                  cc_heap_compare_cb * comparecb,
                  SbBool support_remove)
{
  cc_heap * h = static_cast<cc_heap *>(malloc(sizeof(cc_heap)));
  assert(h);

  h->size = size;
  h->elements = 0;
  h->array = static_cast<void **>(malloc(size * sizeof(void *)));
  assert(h->array);
  h->compare = comparecb;
  h->support_remove = support_remove;
  h->hash = NULL;
  if (support_remove) {
    h->hash = cc_dict_construct(size, 0.0f);
  }
  return h;
}

/*!
  Destruct the heap \a h.
*/
void
cc_heap_destruct(cc_heap * h)
{
  cc_heap_clear(h);
  free(h->array);
  if (h->hash) cc_dict_destruct(h->hash);
  free(h);
}

/*!
  Clear/remove all elements in the heap \a h.
*/
void cc_heap_clear(cc_heap * h)
{
  h->elements = 0;
  if (h->hash) cc_dict_clear(h->hash);
}

/*!
  Add the element \a o to the heap \a h.
*/
void
cc_heap_add(cc_heap * h, void * o)
{
  /* Resize the heap if it is full or the threshold is exceeded */
  if (h->elements == h->size) {
    heap_resize(h, h->size * 2);
  }

  uintptr_t i = h->elements++;
  h->array[i] = o;
  if (h->support_remove) {
    cc_dict_put(h->hash, reinterpret_cast<uintptr_t>(h->array[i]), reinterpret_cast<void*>(i));
  }

  heap_heapify_up(h, i);
}

/*!
  Returns the top element from the heap \a h. If the heap is empty,
  NULL is returned.
*/
void *
cc_heap_get_top(cc_heap * h)
{
  if (h->elements == 0) return NULL;
  return h->array[0];
}

/*!
  Returns and removes the top element from the heap \a h. If the
  heap is empty, NULL is returned.
*/
void *
cc_heap_extract_top(cc_heap * h)
{
  if (h->elements == 0) return NULL;

  void * top = h->array[0];
  h->array[0] = h->array[--h->elements];

  if (h->support_remove) {
    cc_dict_put(h->hash, reinterpret_cast<uintptr_t>(h->array[0]), reinterpret_cast<void *>(0));
    cc_dict_remove(h->hash, reinterpret_cast<uintptr_t>(top));
  }

  heap_heapify_down(h, 0);

  return top;
}

/*!
  Remove \a o from the heap \a h; if present TRUE is returned,
  otherwise FALSE.  Please note that the heap must have been created
  with support_remove.
*/
int
cc_heap_remove(cc_heap * h, void * o)
{
  if (!h->support_remove) return FALSE;

  void * tmp;
  if (!cc_dict_get(h->hash, reinterpret_cast<uintptr_t>(o), &tmp))
    return FALSE;

  uintptr_t i = reinterpret_cast<uintptr_t>(tmp);
  assert(i < h->elements);
  assert(h->array[i] == o);

  h->array[i] = h->array[--h->elements];
  cc_dict_put(h->hash, reinterpret_cast<uintptr_t>(h->array[i]), reinterpret_cast<void *>(i));
  heap_heapify_down(h, i);

  cc_dict_remove(h->hash, reinterpret_cast<uintptr_t>(o));

  return TRUE;
}

/*!
  Updates the heap \a h for new value of existent key \a o; if key is present TRUE is returned,
  otherwise FALSE.
*/
int
cc_heap_update(cc_heap * h, void * o)
{
  void * tmp;
  if (!cc_dict_get(h->hash, reinterpret_cast<uintptr_t>(o), &tmp))
    return FALSE;

  uintptr_t i = reinterpret_cast<uintptr_t>(tmp);
  assert(i < h->elements);
  assert(h->array[i] == o);

  if (i > 0 && h->compare(h->array[i], h->array[HEAP_PARENT(i)]) > 0) {
    heap_heapify_up(h, i);
  }
  else {
    heap_heapify_down(h, i);
  }

  return TRUE;
}

/*!
  Returns the number of elements in the heap \a h.
*/
unsigned int
cc_heap_elements(cc_heap * h)
{
  return h->elements;
}

/*!
  Returns TRUE of the heap \a h is empty, otherwise FALSE.
*/
SbBool
cc_heap_empty(cc_heap * h)
{
  return h->elements == 0 ? TRUE : FALSE;
}

/*!
  Print heap \a h using a specified callback \a printcb.
*/
void
cc_heap_print(cc_heap * h, cc_heap_print_cb * printcb, SbString& str, SbBool printLeveled/* = FALSE*/)
{
  if (!printLeveled) {
    for (unsigned int i = 0; i < h->elements; ++i) {
      printcb(h->array[i], str);
      str += ' ';
    }
  }
  else {
    unsigned int level = 0;
    unsigned int level_items = 1;
    unsigned int printed_items = 0;
    for (unsigned int i = 0; i < h->elements; ++i)
    {
      if (printed_items == 0 ) {
        SbString level_str;
        level_str.sprintf("\nlevel #%d : ", level);
        str += level_str;
      }

      printcb(h->array[i], str);
      str += ' ';
      ++printed_items;

      if (printed_items == level_items)
      {
        ++level;
        level_items *= 2; // next level has at most twice as many items
        printed_items = 0;
      }
    }
    str += '\n';
  }
}

#undef HEAP_LEFT
#undef HEAP_PARENT
#undef HEAP_RIGHT

#ifdef COIN_TEST_SUITE
#include <sstream>
class mock_up {
public:
  typedef struct wrapped_value {
    double x;
  } wrapped_value;

  static void heap_print_cb(void * v, SbString& str) {
    wrapped_value* value = reinterpret_cast<wrapped_value*>(v);
    std::ostringstream oss;
    oss << value->x;
    str += oss.str().c_str();
  }

  static double heap_evaluate_cb(void * v)
  {
    wrapped_value* value = reinterpret_cast<wrapped_value*>(v);
    return value->x;
  }

  // strict weak ordering is needed for compare callbacks
  static int min_heap_compare_cb(void * lhs, void * rhs)
  {
    return heap_evaluate_cb(lhs) < heap_evaluate_cb(rhs) ? 1 : 0;
  }

  static int max_heap_compare_cb(void * lhs, void * rhs)
  {
    return heap_evaluate_cb(lhs) > heap_evaluate_cb(rhs) ? 1 : 0;
  }
};

BOOST_AUTO_TEST_CASE(min_heap) {
  mock_up::wrapped_value val[] = {3, 2, 1, 15, 5, 4, 45};
  cc_heap* heap = cc_heap_construct(256, reinterpret_cast<cc_heap_compare_cb*>(mock_up::min_heap_compare_cb), TRUE);
  for (int i = 0, n = sizeof(val) / sizeof(val[0]); i < n; ++i)
    cc_heap_add(heap, &val[i]);
  SbString result;
  cc_heap_print(heap, reinterpret_cast<cc_heap_print_cb*>(mock_up::heap_print_cb), result, FALSE);
  cc_heap_destruct(heap);
  heap = NULL;
  SbString str("1 3 2 15 5 4 45 ");
  BOOST_CHECK_MESSAGE(str == result,
    std::string("Mismatch between ") + result.getString() + " and control string " + str.getString());
}

BOOST_AUTO_TEST_CASE(max_heap) {
  mock_up::wrapped_value val[] = {3, 2, 1, 15, 5, 4, 45};
  cc_heap* heap = cc_heap_construct(256, reinterpret_cast<cc_heap_compare_cb*>(mock_up::max_heap_compare_cb), TRUE);
  for (int i = 0, n = sizeof(val) / sizeof(val[0]); i < n; ++i)
    cc_heap_add(heap, &val[i]);
  SbString result;
  cc_heap_print(heap, reinterpret_cast<cc_heap_print_cb*>(mock_up::heap_print_cb), result, FALSE);
  cc_heap_destruct(heap);
  heap = NULL;
  SbString str("45 5 15 2 3 1 4 ");
  BOOST_CHECK_MESSAGE(str == result,
    std::string("Mismatch between ") + result.getString() + " and control string " + str.getString());
}

BOOST_AUTO_TEST_CASE(heap_add) {
  mock_up::wrapped_value val[] = {3, 2, 1, 15, 5, 4, 45};
  cc_heap* heap = cc_heap_construct(256, reinterpret_cast<cc_heap_compare_cb*>(mock_up::min_heap_compare_cb), TRUE);
  for (int i = 0, n = sizeof(val) / sizeof(val[0]); i < n; ++i)
    cc_heap_add(heap, &val[i]);
  double added_value = 12;
  cc_heap_add(heap, &added_value);
  SbString result;
  cc_heap_print(heap, reinterpret_cast<cc_heap_print_cb*>(mock_up::heap_print_cb), result, FALSE);
  cc_heap_destruct(heap);
  heap = NULL;
  SbString str("1 3 2 12 5 4 45 15 ");
  BOOST_CHECK_MESSAGE(str == result,
    std::string("Mismatch between ") + result.getString() + " and control string " + str.getString());
}

BOOST_AUTO_TEST_CASE(heap_remove) {
  mock_up::wrapped_value val[] = {3, 2, 1, 15, 5, 4, 45};
  cc_heap* heap = cc_heap_construct(256, reinterpret_cast<cc_heap_compare_cb*>(mock_up::min_heap_compare_cb), TRUE);
  for (int i = 0, n = sizeof(val) / sizeof(val[0]); i < n; ++i)
    cc_heap_add(heap, &val[i]);
  cc_heap_remove(heap, &val[3]);
  SbString result;
  cc_heap_print(heap, reinterpret_cast<cc_heap_print_cb*>(mock_up::heap_print_cb), result, FALSE);
  cc_heap_destruct(heap);
  heap = NULL;
  SbString str("1 3 2 45 5 4 ");
  BOOST_CHECK_MESSAGE(str == result,
    std::string("Mismatch between ") + result.getString() + " and control string " + str.getString());
}

BOOST_AUTO_TEST_CASE(heap_update) {
  mock_up::wrapped_value val[] = {3, 2, 1, 15, 5, 4, 45};
  cc_heap* heap = cc_heap_construct(256, reinterpret_cast<cc_heap_compare_cb*>(mock_up::min_heap_compare_cb), TRUE);
  for (int i = 0, n = sizeof(val) / sizeof(val[0]); i < n; ++i)
    cc_heap_add(heap, &val[i]);
  val[3].x = 1;
  cc_heap_update(heap, &val[3]);
  SbString result;
  cc_heap_print(heap, reinterpret_cast<cc_heap_print_cb*>(mock_up::heap_print_cb), result, FALSE);
  cc_heap_destruct(heap);
  heap = NULL;
  SbString str("1 1 2 3 5 4 45 ");
  BOOST_CHECK_MESSAGE(str == result,
    std::string("Mismatch between ") + result.getString() + " and control string " + str.getString());
}
#endif //COIN_TEST_SUITE
