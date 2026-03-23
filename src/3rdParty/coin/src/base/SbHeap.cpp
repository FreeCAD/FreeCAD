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
  \class SbHeap SbHeap.h Inventor/SbHeap.h
  \brief The SbHeap class is a generic heap class.

  \ingroup coin_base

  FIXME: write doc

  Note: SbHeap is an extension versus the Open Inventor API.
*/

/*!
  \struct SbHeapFuncs SbHeap.h Inventor/SbHeap.h
  \brief The SbHeapFuncs struct is used to specify functions on heap elements.
*/

/*!
  \var SbHeapFuncs::eval_func

  The \e eval_func member is a pointer to a function that should
  return a weight-value for a heap element. Smaller elements are
  extracted first from the heap.  */
/*!
  \var SbHeapFuncs::get_index_func

  \e get_index_func is a pointer to a function which should return the
  element's heap index. If you want to remove an element from the heap
  (other than the first element), or change the weight for a heap
  element, you must supply the two index functions. Each element must
  then store its heap index in its own data structures.  */
/*!
  \var SbHeapFuncs::set_index_func

  \e set_index_func is used to set this index value, and will be
  called whenever the element is moved in the heap.  */

#include <Inventor/SbHeap.h>
#include <cstring>
#include <cstdio>
#include <cassert>

/*!
  Constructor. \a hFuncs specifies the functions for modifying
  and returning information about the heap object, \a initsize
  specifies the initial number of allocated elements. This array
  will automatically grow when necessary, but if you know
  approximately how many elements the heap will contain, you
  should supply this to avoid some reallocs.
*/
SbHeap::SbHeap(const SbHeapFuncs &hFuncs, const int initsize)
  : heap(initsize)
{
  this->funcs = hFuncs;
  assert(funcs.eval_func);
  this->heap.append(NULL);
}

/*!
  Destructor.
*/
SbHeap::~SbHeap(void)
{
}

/*!
  Removes all the elements from the heap.
*/
void
SbHeap::emptyHeap(void)
{
  this->heap.truncate(0);
  this->heap.append(NULL);
}

/*!
  Traverses each heap elements, and calls \a func for each element.
*/
SbBool
SbHeap::traverseHeap(SbBool (*func)(void *, void *), void *userdata) const
{
  SbBool ok = TRUE;
  int hsize = this->heap.getLength() - 1;

  for(int i = 1; (i <= hsize) && ok; i++)
    if (this->heap[i]) ok = func(this->heap[i], userdata);

  return ok;
}

/*!
  Adds an element to the heap. Returns the element's heap position.
*/
int
SbHeap::add(void *obj)
{
  return this->heapInsert(obj);
}

/*!
  Removes an element from the heap.
*/
void
SbHeap::remove(const int idx)
{
  int hsize = this->heap.getLength()-1;
  if (idx == hsize) {
    this->heap.truncate(hsize);
    return;
  }
  this->heap[idx] = this->heap[hsize];
  if (this->funcs.set_index_func)
    this->funcs.set_index_func(this->heap[idx], idx);
  this->heap.truncate(hsize);

  this->newWeight(this->heap[idx]);
}

/*!
  \overload
*/
void
SbHeap::remove(void *obj)
{
  if (this->funcs.get_index_func) {
    this->remove(this->funcs.get_index_func(obj));
  }
  else { // slow!!!
    this->remove(this->heap.find(obj));
  }
}

/*!
  Returns and removes the first element in the heap, or \a NULL
  if heap is empty.
*/
void *
SbHeap::extractMin(void)
{
  return this->heapExtractMin();
}

/*!
  Returns the first element in the heap, or \e NULL if heap is empty.
*/
void *
SbHeap::getMin(void)
{
  if (this->heap.getLength() > 1)
    return heap[1];
  else
    return NULL;
}

/*!
  Returns the heap element at index \a idx in the heap.
*/
void *
SbHeap::operator[](const int idx)
{
  assert(idx > 0 && idx < heap.getLength());
  return heap[idx];
}

/*!
  Fixes heap if necessary when the element at \a hpos has changed weight.
  If you know the element's heap position you can supply it in \a hpos.
*/
void
SbHeap::newWeight(void *obj, int hpos)
{
  int hsize = this->heap.getLength()-1;
  if (hpos < 0) {
    if (this->funcs.get_index_func)
      hpos = this->funcs.get_index_func(obj);
    else
      hpos = this->heap.find(obj);
  }
  int i = hpos;

  float (*eval)(void*) = this->funcs.eval_func;
  void (*setindex)(void*,int) = this->funcs.set_index_func;

  if (i > 1 && eval(obj) < eval(heap[i/2])) {
    while (i > 1 && eval(this->heap[i/2]) > eval(obj)) {
      this->heap[i] = this->heap[i/2];
      if (setindex) setindex(this->heap[i], i);
      i >>= 1;
    }
    this->heap[i]=obj;
    if (setindex) setindex(obj, i);
  }
  if ((i<<1) > hsize) return;
  if (eval(obj) > eval(this->heap[i*2])) this->heapify(i);
  else {
    if ((i<<1)+1 > hsize) return;
    if (eval(obj) > eval(this->heap[i*2+1])) this->heapify(i);
  }
}

/*!
  Builds heap out of randomly ordered data-structure.
*/
SbBool
SbHeap::buildHeap(SbBool (*progresscb)(float percentage, void *data),
                  void *data)
{
  SbBool ok = TRUE;
  int hsize = this->heap.getLength()-1;

  int nrelems = hsize >> 1;

  for (int i = nrelems; (i >= 1) && ok; i--) {
    this->heapify(i);
    if(progresscb && ((i & 31) == 0))
      ok = progresscb(static_cast<float>(nrelems - i)/static_cast<float>(nrelems), data);
  }
  return ok;
}

/*!
  Returns the number of elements in the heap.
*/
int
SbHeap::size(void) const
{
  return this->heap.getLength() - 1;
}

//
// PRIVATE
//

//
// Maintain the heap-structure. Both children must be heaps
//
void
SbHeap::heapify(const int idx)
{
  int l, r, smallest;
  void *tmp;
  int hsize = this->heap.getLength()-1;

  float (*eval)(void*) = this->funcs.eval_func;
  void (*setindex)(void*,int) = this->funcs.set_index_func;

  l = 2*idx;
  r = l+1;
  if (l <= hsize && eval(heap[l]) < eval(heap[idx])) smallest = l;
  else smallest = idx;
  if (r <= hsize && eval(heap[r]) < eval(heap[smallest])) smallest = r;
  if (smallest != idx) {
    tmp = this->heap[idx];
    this->heap[idx] = this->heap[smallest];
    this->heap[smallest] = tmp;
    if (setindex) {
      setindex(this->heap[idx], idx);
      setindex(this->heap[smallest], smallest);
    }
    this->heapify(smallest);
  }
}

//
// Returns the smallest object, and removes it from the heap
//
void *
SbHeap::heapExtractMin(void)
{
  int hsize = this->heap.getLength()-1;
  void *min;
  if (hsize < 1) return NULL;
  min = this->heap[1];
  this->heap[1] = this->heap[hsize];
  if (this->funcs.set_index_func)
    this->funcs.set_index_func(this->heap[1], 1);
  this->heap.truncate(hsize);
  this->heapify(1);

  return min;
}

int
SbHeap::heapInsert(void *obj)
{
  int i;
  int hsize = this->heap.getLength();
  i = hsize;
  void (*setindex)(void*,int) = this->funcs.set_index_func;
  float (*eval)(void*) = this->funcs.eval_func;

  this->heap.append(NULL); // will be overwritten later

  while (i > 1 && eval(this->heap[i>>1]) > eval(obj)) {
    this->heap[i] = this->heap[i>>1];
    if (setindex) setindex(this->heap[i], i);
    i >>= 1;
  }
  this->heap[i] = obj;
  if (setindex) setindex(obj, i);
  return i;
}
