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

/*!
  \class SoLightPath SoLightPath.h Inventor/misc/SoLightPath.h
  \brief The SoLightPath class is a light version of SoPath.

  \ingroup coin_general

  SoLightPath can be used if you only need a temporary path, and don't
  want the overhead that comes with an SoPath (ref, unref, auditing etc).

  It is your responsibility to make sure the path is valid before
  using it.
*/

#include <Inventor/misc/SoLightPath.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/misc/SoTempPath.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  A constructor. Supply the head node and the approximate length of
  the path.
*/
SoLightPath::SoLightPath(SoNode *headnodeptr, const int approxlength)
  : headnode(NULL),
    indices(approxlength)
{
  this->setHead(headnodeptr);
}

/*!
  A constructor. Supply the approximate length of the path.
*/
SoLightPath::SoLightPath(const int approxlength)
  : headnode(NULL),
    indices(approxlength)
{
}

/*!
  Destructor.
*/
SoLightPath::~SoLightPath()
{
  if (this->headnode) this->headnode->unref();
}

/*!
  Sets head of path. Truncates path to length 1.
*/
void
SoLightPath::setHead(SoNode * const node)
{
  if (this->headnode) this->headnode->unref();
  this->headnode = node;
  assert(node);
  this->headnode->ref();
  this->indices.truncate(0);
  this->indices.append(-1);
}

/*!
  Appends a \a childindex to the path.
*/
void
SoLightPath::append(const int childindex)
{
  this->indices.append(childindex);
}

/*!
  Same as append().
*/
void
SoLightPath::push(const int childindex)
{
  this->indices.append(childindex);
}

/*!
  Pops off the last child.
*/
void
SoLightPath::pop(void)
{
#if COIN_DEBUG && 1 // debug
  if (this->indices.getLength() <= 1) {
    SoDebugError::postInfo("SoLightPath::pop",
                           "You shouldn't pop off the head node.");
  }
#endif // debug

  this->truncate(this->indices.getLength()-1);
}

/*!
  Sets the tail of the path.
*/
void
SoLightPath::setTail(const int childindex)
{
  this->indices[this->indices.getLength()-1] = childindex;
}

/*!
  Returns the tail node of the path. Uses getNode().
*/
SoNode *
SoLightPath::getTail(void) const
{
  return this->getNode(this->indices.getLength()-1);
}

/*!
  Returns the head node.
*/
SoNode *
SoLightPath::getHead(void) const
{
  return this->headnode;
}

/*!
  Returns the index'th node in path.
*/
SoNode *
SoLightPath::getNode(const int index) const
{
#if COIN_DEBUG && 1 // debug
  if (index < 0 || index >= this->indices.getLength()) {
    SoDebugError::postInfo("SoLightPath::getNode",
                           "index %d out of bounds", index);
  }
#endif // debug

  SoNode *node = this->headnode;
  for (int i = 1; i < index; i++) {
    int childidx = this->indices[i];
    SoChildList *children = node->getChildren();
    node = NULL;
    if (children == NULL || childidx < 0 || childidx >= children->getLength()) break;
    node = (*children)[childidx];
  }
  return node;
}

/*!
  Returns the child index of the index'th node in the path.
*/
int
SoLightPath::getIndex(const int index) const
{
#if COIN_DEBUG && 1 // debug
  if (index < 0 || index >= this->indices.getLength()) {
    SoDebugError::postInfo("SoLightPath::getNode",
                           "index %d out of bounds", index);
  }
#endif // debug
  return this->indices[index];
}

/*!
  Returns the length of the path.
*/
int
SoLightPath::getFullLength() const
{
  return this->indices.getLength();
}

/*!
  Truncates the path from \a startindex.
*/
void
SoLightPath::truncate(const int startindex)
{
  this->indices.truncate(startindex);
}

/*!
  Updates \a path to be the same path as this path.
*/
void
SoLightPath::makeTempPath(SoTempPath *path) const
{
  const int n = this->indices.getLength();
  SoNode *node = this->headnode;
  path->setHead(node);
  for (int i = 1; i < n; i++) {
    int childidx = this->indices[i];
    // check validity of path, return if invalid
    SoChildList *children = node->getChildren();
    if (children == NULL || childidx < 0 || childidx >= children->getLength()) break;
    node = (*children)[childidx];
    path->append(childidx); // childidx should be ok
  }
}
