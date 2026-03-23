#ifndef COIN_SOCOMPACTPATHLIST_H
#define COIN_SOCOMPACTPATHLIST_H

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

class SoPathList;

#ifndef COIN_INTERNAL
#error this is a private header file
#endif // !COIN_INTERNAL

// SoCompactPathList is an internal class in Coin and should not be
// used by external applications. It's used only to optimize
// SoPathList traversing. Let us know if you still need to use this
// class for some reason and we might add it as a part of the public
// Coin API.

class SoCompactPathList {
public:
  SoCompactPathList(const SoPathList & list);
  ~SoCompactPathList();

  void reset(void);

  void getChildren(int & numindices, const int *& indices);
  SbBool push(int childindex);
  void pop(void);

  int getDepth(void) const;

private:
  int * lookuptable;
  SbList <int> stack;
  int lookupidx;
  int lookupsize;

  int getNumIndices(void);
  int getStartIndex(void);

  int getChildIndex(const int child);
  int createLookupTable(int curslot, int depth,
                        const SoPathList & list,
                        int firstpath, int numpaths);
};

#endif /* COIN_SOCOMPACTPATHLIST_H */
