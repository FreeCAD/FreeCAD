#ifndef COIN_SONODEKITPATH_H
#define COIN_SONODEKITPATH_H

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

#include <Inventor/SoPath.h>

class SoNode;
class SoBaseKit;
class SoSearchAction;

class COIN_DLL_API SoNodeKitPath : public SoPath {
  typedef SoPath inherited;

public:
  int getLength(void) const;
  SoNode * getTail(void) const;
  SoNode * getNode(const int idx) const;
  SoNode * getNodeFromTail(const int idx) const;
  void truncate(const int length);
  void pop(void);
  void append(SoBaseKit * childKit);
  void append(const SoNodeKitPath * fromPath);
  SbBool containsNode(SoBaseKit * node) const;
  int findFork(const SoNodeKitPath * path) const;

  friend COIN_DLL_API int operator==(const SoNodeKitPath & p1, const SoNodeKitPath & p2);

protected:
  SoNodeKitPath(const int approxLength);
  virtual ~SoNodeKitPath();

private:

  static void clean(void);
  static SoSearchAction *searchAction;
  SoSearchAction *getSearchAction(void);

  // these methods should not be used on an SoNodeKitPath
  void append(const int childIndex);
  void append(SoNode *childNode);
  void append(const SoPath *fromPath);
  void push(const int childIndex);
  int getIndex(const int i) const;
  int getIndexFromTail(const int i) const;
  void insertIndex(SoNode *parent,const int newIndex);
  void removeIndex(SoNode *parent,const int oldIndex);
  void replaceIndex(SoNode *parent,const int index,SoNode *newChild);
};

COIN_DLL_API int operator==(const SoNodeKitPath & p1, const SoNodeKitPath & p2);

#endif // !COIN_SONODEKITPATH_H
