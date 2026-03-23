#ifndef COIN_LISTS_SBPLIST_H
#define COIN_LISTS_SBPLIST_H

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
#include <cassert>
#include <cstddef> // NULL definition

class COIN_DLL_API SbPList {
  enum { DEFAULTSIZE = 4 };

public:
  SbPList(const int sizehint = DEFAULTSIZE);
  SbPList(const SbPList & l);
  ~SbPList();

  void copy(const SbPList & l);
  SbPList & operator=(const SbPList & l);
  void fit(void);

  void append(void * item);
  int find(const void * item) const;
  void insert(void * item, const int insertbefore);
  void removeItem(void * item);
  void remove(const int index);
  void removeFast(const int index);
  int getLength(void) const;
  void truncate(const int length, const int fit = 0);

  void ** getArrayPtr(const int start = 0) const;
  void *& operator[](const int index) const;

  int operator==(const SbPList & l) const;
  int operator!=(const SbPList & l) const;  
  void * get(const int index) const;
  void set(const int index, void * item);
  
protected:

  void expand(const int size);
  int getArraySize(void) const;

private:
  void expandlist(const int size) const; 
  void grow(const int size = -1);

  int itembuffersize;
  int numitems;
  void ** itembuffer;
  void * builtinbuffer[DEFAULTSIZE];
};

/* inlined methods ********************************************************/

inline void 
SbPList::append(void * item) 
{
  if (this->numitems == this->itembuffersize) this->grow();
  this->itembuffer[this->numitems++] = item;
}

inline void 
SbPList::removeFast(const int index) 
{
#ifdef COIN_EXTRA_DEBUG
  assert(index >= 0 && index < this->numitems);
#endif // COIN_EXTRA_DEBUG
  this->itembuffer[index] = this->itembuffer[--this->numitems];
}

inline int 
SbPList::getLength(void) const 
{
  return this->numitems;
}

inline void 
SbPList::truncate(const int length, const int dofit) 
{
#ifdef COIN_EXTRA_DEBUG
  assert(length <= this->numitems);
#endif // COIN_EXTRA_DEBUG
  this->numitems = length;
  if (dofit) this->fit();
}

inline void ** 
SbPList::getArrayPtr(const int start) const 
{
#ifdef COIN_EXTRA_DEBUG
  assert(start >= 0 && start < this->numitems);
#endif // COIN_EXTRA_DEBUG
  return &this->itembuffer[start];
}

inline void *& 
SbPList::operator[](const int index) const 
{
#ifdef COIN_EXTRA_DEBUG
  assert(index >= 0);
#endif // COIN_EXTRA_DEBUG
  if (index >= this->getLength()) this->expandlist(index + 1);
  return this->itembuffer[index];
}

inline int 
SbPList::operator!=(const SbPList & l) const 
{
  return !(*this == l);
}

inline void * 
SbPList::get(const int index) const 
{ 
  return this->itembuffer[index]; 
}

inline void 
SbPList::set(const int index, void * item) 
{ 
  this->itembuffer[index] = item; 
}

inline void 
SbPList::expand(const int size) 
{
  this->grow(size);
  this->numitems = size;
}

inline int 
SbPList::getArraySize(void) const 
{
  return this->itembuffersize;
}


#endif // !COIN_LISTS_SBPLIST_H
