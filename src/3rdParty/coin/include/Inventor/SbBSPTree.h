#ifndef COIN_SBBSPTREE_H
#define COIN_SBBSPTREE_H

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

#include <cstddef> // for NULL definition
#include <Inventor/lists/SbList.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbBox3f.h>

#ifdef COIN_INTERNAL
 #define COIN_ALLOW_SBINTLIST
 #include <Inventor/lists/SbIntList.h>
 #undef COIN_ALLOW_SBINTLIST
#else
 #include <Inventor/lists/SbIntList.h>
#endif // COIN_INTERNAL

class SbSphere;
class coin_bspnode;

// *************************************************************************

class COIN_DLL_API SbBSPTree {
public:
  SbBSPTree(const int maxnodepts = 64, const int initsize = 4);
  ~SbBSPTree();

  int numPoints() const;
  SbVec3f getPoint(const int idx) const;
  void getPoint(const int idx, SbVec3f & pt) const;
  void * getUserData(const int idx) const;
  void setUserData(const int idx, void * const data);

  int addPoint(const SbVec3f & pt, void * const userdata = NULL);
  int removePoint(const SbVec3f & pt);
  void removePoint(const int idx);
  int findPoint(const SbVec3f & pos) const;
  int findClosest(const SbVec3f & pos) const;
  void clear(const int initsize = 4);
  void findPoints(const SbSphere & sphere, SbIntList & array) const;
  int findClosest(const SbSphere & sphere, SbIntList & array) const;

  const SbBox3f & getBBox() const;
  const SbVec3f * getPointsArrayPtr() const;

  // Please stop using these two functions. They will be removed in
  // Coin 3.0. Use the SbIntList versions instead.
  void findPoints(const SbSphere & sphere, SbList <int> & array) const;
  int findClosest(const SbSphere & sphere, SbList <int> & array) const;

private:
  friend class coin_bspnode;
  SbList <SbVec3f> pointsArray;
  SbList <void *> userdataArray;
  coin_bspnode * topnode;
  int maxnodepoints;
  SbBox3f boundingBox;
};

#endif // !COIN_SBBSPTREE_H
