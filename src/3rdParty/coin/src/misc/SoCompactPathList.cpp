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

#include "misc/SoCompactPathList.h"

#include <cassert>

#include <Inventor/SoFullPath.h>
#include <Inventor/lists/SoPathList.h>
#include <Inventor/errors/SoDebugError.h>

/*!
  \class SoCompactPathList SoCompactPathList.h Inventor/misc/SoCompactPathList.h
  \brief The SoCompactPathList class is used to optimize SoPathList traversal.

  This class creates a lookup table which is used while doing SoPathList traversal.
  For each node in the paths, it stores the following information:

  1. the number of distinct children at that level
  2. a child index to each distinct child to be used for IN_PATH traversal
  3. an index to each child's lookup table position

  1. and 2. are used for optimized IN_PATH action traversal. 3. is
  used internally to traverse this lookup table.

*/

#define FULL_PATH(list, i) ((SoFullPath *) list[i])

/*!
  Constructor.
*/
SoCompactPathList::SoCompactPathList(const SoPathList & list)
  : stack(256)
{
  assert(list.getLength());
  SoNode * head = FULL_PATH(list, 0)->getHead();
  int numnodes = 0;

  for (int i = 0; i < list.getLength(); i++) {
    assert(FULL_PATH(list, i)->getHead() == head);
    numnodes += FULL_PATH(list, i)->getLength() - 1;
  }
  // 3 entries for each node + one extra for the root. This is a
  // worst-case size, but memory usage isn't an issue for this class
  // (memory usage will be small compared to memory used by the paths)
  this->lookupsize = 3 * numnodes + 1;

  this->lookuptable = new int[this->lookupsize];
  this->createLookupTable(0, 1, list, 0, list.getLength());
  this->reset();
}

/*!
  Destructor.
*/
SoCompactPathList::~SoCompactPathList()
{
  delete[] this->lookuptable;
}

/*!
  Reset path list.
*/
void
SoCompactPathList::reset()
{
  this->stack.truncate(0);
  this->lookupidx = 0;
}

/*!
  Returns the children to be used for IN_PATH traversal.
*/
void
SoCompactPathList::getChildren(int & numindices, const int *& indices)
{
  assert(this->lookupidx >= 0);
  numindices = this->getNumIndices();
  indices = this->lookuptable + this->getStartIndex();
}

/*!
  Push \a childindex to the current path.
 */
SbBool
SoCompactPathList::push(int childindex)
{
  assert(this->lookupidx >= 0); // this function should be used only for IN_PATH traversal

  // push current node to be able to restore it in pop()
  this->stack.push(this->lookupidx);

  int i;
  const int n = this->getNumIndices();
  const int idx = this->getStartIndex();

  // check if childindex is IN_PATH or not
  for (i = 0; i < n; i++) {
    if (this->lookuptable[idx + i] == childindex) break;
  }

  if (i < n) { // IN_PATH
    // get ready to traverse the child
    this->lookupidx = this->getChildIndex(i);
    assert(this->lookupidx < this->lookupsize);
  }
  else { // OFF_PATH
    this->lookupidx = -1;
  }
  return (this->lookupidx >= 0);
}

/*!
  Pop the current node off the path.
*/
void
SoCompactPathList::pop()
{
  this->lookupidx = this->stack.pop();
}

/*!
  Precalculate tables for each node in the paths.
*/
int
SoCompactPathList::createLookupTable(int curidx, int depth,
                                     const SoPathList & list,
                                     int firstpath, int numpaths)
{
  // When we get to the tail, store a 0 for no children. The traversal
  // will switch to BELOW_PATH when this happens.
  if (depth >= FULL_PATH(list, firstpath)->getLength()) {
    this->lookuptable[curidx] = 0;
    return curidx + 1;
  }

  int i;
  // count the number of IN_PATH indices that we need to create and
  // fill in first part of the lookup table. We do this by finding the
  // number of distinct children in all the paths we're traversing.
  int numchildren = 0;
  int prevchildidx = -1;

  for (i = 0; i < numpaths; i++) {
    int childidx = FULL_PATH(list, firstpath + i)->getIndex(depth);
    if (childidx != prevchildidx) {
      // fill in the IN_PATH table indices
      this->lookuptable[curidx + 1 + numchildren] = childidx;
      numchildren++;
      prevchildidx = childidx;
    }
  }
  // store the size of this node's tables
  this->lookuptable[curidx] = numchildren;

  // Find next free position in lookup table. We need to store 1 +
  // 2*numchildren integers for this node (size + IN_PATH table and
  // lookup table position for each child)
  int nextidx = curidx + 1 + 2 * numchildren;

  // Recurse and fill in info for each child in the lookup table
  int curchild = 0;
  i = 0;

  while (i < numpaths) {
    int startpath = i + firstpath;
    int childidx = FULL_PATH(list, firstpath + i)->getIndex(depth);
    int pathcounter = 1;
    i++;
    // find all paths that go through childidx
    while ((i < numpaths) && (FULL_PATH(list, firstpath + i)->getIndex(depth) == childidx)) {
      i++;
      pathcounter++;
    }
    this->lookuptable[curidx + 1 + numchildren + curchild] = nextidx;
    nextidx = this->createLookupTable(nextidx, depth + 1, list,
                                      startpath, pathcounter);
    curchild++;
  }
  assert(curchild == numchildren);
  return nextidx;
}


#undef FULL_PATH

/*!
  Returns the number of IN_PATH children for the current node.
*/
int
SoCompactPathList::getNumIndices(void)
{
  assert(this->lookupidx >= 0);
  return this->lookuptable[this->lookupidx];
}

/*!
  Returns the position of the IN_PATH table.
*/
int
SoCompactPathList::getStartIndex(void)
{
  assert(this->lookupidx >= 0);
  return this->lookupidx + 1;
}

/*!
  Returns the lookup table position for \a child.
*/
int
SoCompactPathList::getChildIndex(const int child)
{
  const int idx = this->getStartIndex() + this->getNumIndices() + child;
  assert(idx < this->lookupsize);
  return this->lookuptable[idx];
}

/*!
  Returns the depth of the current node.
*/
int
SoCompactPathList::getDepth(void) const
{
  return 1 + this->stack.getLength();
}
