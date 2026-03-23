#ifndef COIN_SBOCTTREE_H
#define COIN_SBOCTTREE_H

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

#include <Inventor/SbVec3f.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/SbBox3f.h>
#include <cstdio>

class SbSphere;
class SbOctTreeNode;
class SbPlane;

// *************************************************************************

typedef struct
{
  SbBool (*ptinsidefunc)(void * const item, const SbVec3f & pt);
  SbBool (*insideboxfunc)(void * const item, const SbBox3f & box);
  SbBool (*insidespherefunc)(void * const item, const SbSphere & sphere);
  SbBool (*insideplanesfunc)(void * const item,
                             const SbPlane * const planes,
                             const int numplanes);
} SbOctTreeFuncs;

// *************************************************************************

class COIN_DLL_API SbOctTree {
public:
  SbOctTree(const SbBox3f & bbox,
            const SbOctTreeFuncs & itemfuncs,
            const int maxitemspernode = 64);
  ~SbOctTree();

  void addItem(void * const item);
  void removeItem(void * const item);
  void findItems(const SbVec3f & pos,
                 SbList <void*> & destarray,
                 const SbBool removeduplicates = TRUE) const;
  void findItems(const SbBox3f & box,
                 SbList <void*> & destarray,
                 const SbBool removeduplicates = TRUE) const;
  void findItems(const SbSphere & sphere,
                 SbList <void*> & destarray,
                 const SbBool removeduplicates = TRUE) const;
  void findItems(const SbPlane * const planes,
                 const int numplanes,
                 SbList <void*> & destarray,
                 const SbBool removeduplicates= TRUE) const;

  const SbBox3f & getBoundingBox(void) const;
  void clear(void);
  void debugTree(FILE * fp);

private:
  SbOctTreeNode * topnode;
  SbOctTreeFuncs itemfuncs;
  int maxitemspernode;
};

// *************************************************************************

#endif // !COIN_SBOCTTREE_H
