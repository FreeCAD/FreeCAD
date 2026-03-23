#ifndef COIN_SBHEAP_H
#define COIN_SBHEAP_H

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

#include <Inventor/SbBasic.h>
#include <Inventor/lists/SbList.h>
#include <cstddef>

typedef struct
{
  float (*eval_func)(void*);
  int (*get_index_func)(void*);
  void (*set_index_func)(void*, int);
} SbHeapFuncs;

class COIN_DLL_API SbHeap {
public:
  SbHeap(const SbHeapFuncs &SbHeapFuncs,
         const int initsize = 1024);
  ~SbHeap();

  void emptyHeap(void);
  int size(void) const;
  int add(void *obj);
  void remove(const int pos);
  void remove(void *obj);
  void *extractMin();
  void *getMin();
  void *operator[](const int idx);

  void newWeight(void *obj, int hpos = -1);
  SbBool buildHeap(SbBool (*progresscb)(float percentage, void *data) = NULL,
                   void *data = NULL);
  SbBool traverseHeap(SbBool (*func)(void *, void *), void *userdata) const;

private:
  SbHeapFuncs funcs;
  SbList <void*> heap;

  int heapInsert(void *obj);
  void *heapExtractMin(void);
  void heapReserve(const int newsize);
  void heapify(const int idx);
};

#endif // !COIN_SBHEAP_H
