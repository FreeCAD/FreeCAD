#ifndef COIN_SBDICT_H
#define COIN_SBDICT_H

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

// *************************************************************************

// Internally, we should use the SbHash template class, since it
// provides a better interface than SbDict, and since the interface of
// SbDict has issues on 64-bit platforms when keeping strict
// compatibility with the original SbDict of SGI Inventor.

#if defined(COIN_INTERNAL) && !defined(COIN_ALLOW_SBDICT)
#error prefer SbHash over SbDict for internal code
#endif // COIN_INTERNAL

// *************************************************************************

#include <cstddef>
#include <Inventor/SbBasic.h>

class SbPList;

// *************************************************************************

extern "C" {
typedef uintptr_t SbDictKeyType;
typedef void SbDictApplyFunc(SbDictKeyType key, void * value);
typedef void SbDictApplyDataFunc(SbDictKeyType key, void * value, void * data);
typedef SbDictKeyType SbDictHashingFunc(const SbDictKeyType key);
}

class COIN_DLL_API SbDict {
public:
  SbDict(const int entries = 251);
  SbDict(const SbDict & from);
  ~SbDict();

  SbDict & operator=(const SbDict & from);

  typedef uintptr_t Key;

  void applyToAll(SbDictApplyFunc * rtn) const;
  void applyToAll(SbDictApplyDataFunc * rtn, void * data) const;
  void clear(void);

  SbBool enter(const Key key, void * const value);
  SbBool find(const Key key, void *& value) const;
  void makePList(SbPList & keys, SbPList & values);
  SbBool remove(const Key key);

  void setHashingFunction(SbDictHashingFunc * func);

private:
  struct cc_hash * hashtable;

};

// *************************************************************************

#endif // !COIN_SBDICT_H
