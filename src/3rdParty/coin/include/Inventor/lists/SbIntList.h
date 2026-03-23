#ifndef COIN_SBINTLIST_H
#define COIN_SBINTLIST_H

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

#include <Inventor/lists/SbPList.h>

// *************************************************************************

class  COIN_DLL_API SbIntList : public SbPList {
public:
  SbIntList(void) : SbPList () { }
  SbIntList(const int sizehint) : SbPList(sizehint) { }
  
  void append(const int item) {
    this->SbPList::append(reinterpret_cast<void*>(static_cast<uintptr_t>(item)));
  }
  int find(const int item) {
    return this->SbPList::find(reinterpret_cast<void *>(static_cast<uintptr_t>(item)));
  }
  void insert(const int item, const int addbefore) {
    this->SbPList::insert(reinterpret_cast<void *>(static_cast<uintptr_t>(item)), addbefore);
  }
  int & operator[](const int idx) const {
    return reinterpret_cast<int&>(((*static_cast<const SbPList *>(this))[idx]));
  }
};

// *************************************************************************

// Internally, we should use the SbList template class, since it
// provides a better interface than SbIntList, and since the interface
// of SbIntList may have issues with casting type safety.
//
// This class is provided for two reasons: 1) to be compatible with
// SGI/TGS Inventor, 2) there are a few places in the public API where
// it is used (these are all our own doing, not from the original SGI
// Inventor, and could be taken out for any new major release).

#if defined(COIN_INTERNAL) && !defined(COIN_ALLOW_SBINTLIST)
#error prefer SbList over SbIntList for internal code
#endif // COIN_INTERNAL

// *************************************************************************

#endif // !COIN_SBINTLIST_H
