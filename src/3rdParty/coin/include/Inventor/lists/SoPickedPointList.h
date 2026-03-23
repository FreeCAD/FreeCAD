#ifndef COIN_SOPICKEDPOINTLIST_H
#define COIN_SOPICKEDPOINTLIST_H

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

class SoPickedPoint;

class COIN_DLL_API SoPickedPointList : public SbPList {
public:
  SoPickedPointList(void) : SbPList() { }
  SoPickedPointList(const int sizehint) : SbPList(sizehint) { }
  SoPickedPointList(const SoPickedPointList & l);
  ~SoPickedPointList() { this->truncate(0); }
  
  void append(SoPickedPoint * pp) { 
    SbPList::append(static_cast<void *>(pp));
  }
  void insert(SoPickedPoint * pp, const int insertbefore) { 
    SbPList::insert(static_cast<void *>(pp), insertbefore);
  }
  SoPickedPoint * operator[](const int idx) const {
    return static_cast<SoPickedPoint *>(SbPList::operator[](idx));
  } 

  void truncate(const int start, const int fit = 0);
  void set(const int idx, SoPickedPoint * pp);

};

#endif // !COIN_SOPICKEDPOINTLIST_H
