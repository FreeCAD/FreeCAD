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
  \class SoPath SoPath.h Inventor/SoPath.h
  \brief The SoPath class is a container class for traversal path descriptions.

  \ingroup coin_general

  SoPath objects contain a list of SoNode pointers and a list of child
  indices. Indices are necessary to disambiguate situations where a
  node uses the same node as a child multiple times.

  Paths can be export and imported to/from file, like other SoBase
  derived objects. Read the documentation on the write() method to see
  how the file format is.

  It's important to know the difference between this class and
  SoFullPath when working with paths. SoPath::getTail() will return
  the first node in the path that doesn't inherit SoGroup, and
  getLength() returns the number of nodes down to this node.

  If you need the actual path length, or the actual tail node, you
  need to cast the path to SoFullPath.
*/

// *************************************************************************

/*!
  \fn void SoPath::push(const int childindex)

  This method pushes the child at \a childindex on the tail on the
  path.
*/

/*!
  \fn void SoPath::pop(void)

  This method pops the tail off the path.
*/

// *************************************************************************

#include <Inventor/SoPath.h>

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/lists/SoPathList.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/nodes/SoGroup.h>

#include "io/SoWriterefCounter.h"
#include "tidbitsp.h"
#include "coindefs.h" // COIN_STUB()

// *************************************************************************

#if COIN_DEBUG && 0 // Convenience function for dumping the SoPath during debugging.
#include <Inventor/SoFullPath.h>

static void
sopath_dump(SoPath * p)
{
  SoFullPath * path = (SoFullPath *)p;

  (void)fprintf(stderr, "(path %p, len %d)  ", path, path->getLength());
  for (int i=0; i < path->getLength(); i++) {
    SoNode * n = path->getNodeFromTail(i);
    (void)fprintf(stderr, "%p (%s), ",
                  n, n->getTypeId().getName().getString());
  }
  (void)fprintf(stderr, "\n");
}
#endif // COIN_DEBUG

// *************************************************************************

SoType SoPath::classTypeId STATIC_SOTYPE_INIT;

// *************************************************************************

/*!
  The default constructor.  \a approxlength is used to indicate in
  advance the length of the path to minimize the number of reallocations.
*/
SoPath::SoPath(const int approxlength)
  : nodes(approxlength), indices(approxlength),
    isauditing(TRUE), firsthidden(-1), firsthiddendirty(FALSE)
{
}

/*!
  A constructor. The argument is set as the \a head (and only) node of
  the path.
*/
SoPath::SoPath(SoNode * const head)
  : isauditing(TRUE), firsthidden(-1), firsthiddendirty(FALSE)
{
  this->setHead(head);
}

/*!
  Copy constructor. Not part of the original Open Inventor API.
*/
SoPath::SoPath(const SoPath & rhs)
  : inherited(), nodes(rhs.getFullLength()), indices(rhs.getFullLength())
{
  this->operator=(rhs);
}

/*!
  Assignment operator, copies the \a rhs path into this.  Not part of
  the original Open Inventor API.
*/
SoPath &
SoPath::operator=(const SoPath & rhs)
{
  this->firsthidden = rhs.firsthidden;
  this->firsthiddendirty = rhs.firsthiddendirty;
  this->isauditing = rhs.isauditing;
  this->nodes = rhs.nodes;
  this->indices = rhs.indices;

  // Add ourself as an auditor to the children lists of the path.
  if (this->isauditing) {
    for (int i = 0; i < this->getFullLength(); i++) {
      SoChildList * cl = this->nodes[i]->getChildren();
      if (cl) cl->addPathAuditor(this);
    }
  }

  if (isauditing) this->startNotify();

  return *this;
}


/*!
  The destructor is protected as destruction of SoPath instances
  happens through unref() (as for all objects derived from classes
  inheriting SoBase).
*/
SoPath::~SoPath(void)
{
  // Explicitly truncate() so the SoChildList::removePathAuditor()
  // calls are made.
  this->truncate(0);
}

/*!
  This method sets the head of the path to \a node. If the object
  already has a path set, it will be truncated.
*/
void
SoPath::setHead(SoNode * const node)
{
  this->truncate(0);
  this->firsthidden = -1;
  this->firsthiddendirty = FALSE;
  this->append(node, 0);
}

/*!
  This method returns the head of the path, or \c NULL if the path is
  empty.
*/
SoNode *
SoPath::getHead(void) const
{
  if (this->getFullLength() == 0) return NULL;
  return (SoNode *) this->nodes[0];
}

/*!
  This function appends the \a childindex child of the path's tail to
  the end of the path.  It can only be used after the head is set.
*/
void
SoPath::append(const int childindex)
{
#if COIN_DEBUG
  if (this->getFullLength() == 0) {
    SoDebugError::post("SoPath::append",
                       "SoPath was empty.\n");
    return;
  }
#endif // COIN_DEBUG

  SoChildList * children =
    this->nodes[this->getFullLength() - 1]->getChildren();
#ifdef COIN_EXTRA_DEBUG
  assert(children);
#endif // COIN_EXTRA_DEBUG

#if COIN_DEBUG
  if (childindex >= children->getLength()) {
    SoDebugError::post("SoPath::append", "childindex (%d) out of bounds",
                       childindex);
    return;
  }
#endif // debug

  SoNode * node = (*children)[childindex];
  this->append(node, childindex);
}

/*!
  This function adds \a node to the end if it is a child of the
  current tail. If path is empty, this is equivalent to calling
  setHead().
*/
void
SoPath::append(SoNode * const node)
{
  const unsigned int full_length = (unsigned int)this->getFullLength();

  if (full_length == 0) {
    this->setHead(node);
    return;
  }

  SoNode * tail = this->nodes[full_length - 1];
  const SoChildList * children = tail->getChildren();

#if COIN_DEBUG
  if (!children) {
    SoDebugError::post("SoPath::append",
                       "The current tail of the SoPath, of type %s, does "
                       "not have any children, so append()'ing new elements "
                       "is bogus!",
                       tail->getTypeId().getName().getString());
    return;
  }
#endif // COIN_DEBUG

  const int idx = children->find(node);
#if COIN_DEBUG
  if (idx < 0) {
    SoDebugError::post("SoPath::append",
                       "The current tail of the SoPath does not have the "
                       "input argument node as a child, so append()'ing "
                       "it is bogus!");
    return;
  }
#endif // COIN_DEBUG

  this->append(node, idx);
}

/*!
  This method appends the path to the end of the path if the head of
  \a frompath is either the tail of the path or a child of the path's
  tail. If the head of \a frompath is used multiple times as a child
  of the path tail, the lowest child index is used.
*/
void
SoPath::append(const SoPath * const frompath)
{
  if (!this->getFullLength()) { // append to empty path
    this->operator=(*frompath);
    return;
  }

  if (!frompath->getFullLength()) // nothing to append
    return;

  SoNode * const head = frompath->getHead();
  SoNode * const tail = this->nodes[this->getFullLength() - 1];
  if (head == tail) { // easy
    const int length = frompath->getFullLength();
    for (int i = 1; i < length; i++) {
      this->append((SoNode *)frompath->nodes[i], frompath->indices[i]);
    }

    if (!this->firsthiddendirty && this->firsthidden == -1 &&
        (frompath->firsthiddendirty || frompath->firsthidden >= 0)) {
      this->firsthiddendirty = TRUE;
    }
    return;
  }
  // head of frompath must be child

  SoChildList * tailchildren = tail->getChildren();

#if COIN_DEBUG
  if (tailchildren == NULL) {
    SoDebugError::post("SoPath::append",
                       "The tail of this SoPath has no children node ('%s').\n",
                       tail->getTypeId().getName().getString());
    return;
  }
#endif // COIN_DEBUG

  const int kids = tailchildren->getLength();
  for (int kid = 0; kid < kids; kid++) {
    SoNode * child = (*tailchildren)[kid];
    if (child == head) {
      this->append(head, kid);
      const int length = frompath->getFullLength();
      for (int i = 1; i < length; i++) {
        this->append((SoNode *)frompath->nodes[i], frompath->indices[i]);
      }
      if (!this->firsthiddendirty && this->firsthidden == -1 &&
          (frompath->firsthiddendirty || frompath->firsthidden >= 0)) {
        this->firsthiddendirty = TRUE;
      }
      return;
    }
  }

#if COIN_DEBUG
  SoDebugError::post("SoPath::append", "Could not join paths.\n");
#endif // COIN_DEBUG
  return;
}


//
// convenience method that tests if a node has hidden
// children.  It would probably be a good idea to move this method to
// SoNode.
//
static inline SbBool
has_hidden_children(const SoNode * node)
{
  assert(node);

  return
    (node->getChildren() != NULL) &&
    !node->isOfType(SoGroup::getClassTypeId());
}


// This method appends a node to the path, assuming the information is
// correct.
void
SoPath::append(SoNode * const node, const int index)
{
  if (node && !this->firsthiddendirty && this->firsthidden < 0) {
    if (has_hidden_children(node)) {
      this->firsthidden = this->getFullLength();
      this->firsthiddendirty = FALSE;
    }
  }
  this->nodes.append(node);
  this->indices.append(index);

  // Add ourself as an auditor to the node's list of children.
  if (this->isauditing) {
    SoChildList * cl = node->getChildren();
    if (cl) cl->addPathAuditor(this);
  }

  if (this->isauditing) this->startNotify();
}

/*!
  This method returns the tail node of the path. Please note that this
  method only considers group nodes without hidden children (nodes
  inheriting SoGroup) when finding the tail.

  If you want to find the real tail node (also below node kits and
  VRML nodes with hidden children), you have to use
  SoFullPath::getTail(). You don't have to create an SoFullPath
  instance to do this, just cast the SoPath instance to SoFullPath
  before getting the tail node:

  \code
  SoNode * tail = static_cast<SoFullPath*>(path)->getTail();
  \endcode
*/
SoNode *
SoPath::getTail(void) const
{
  if (this->getLength() == 0) {
#if COIN_DEBUG
    SoDebugError::postWarning("SoPath::getTail", "empty path!");
#endif // COIN_DEBUG
    return NULL;
  }

#if COIN_DEBUG && 0 // debug
  int len = this->nodes.getLength();
  (void)fprintf(stderr, "** getTail() **\n\tNODES(len=%d): ", len);
  for (int i=0; i < len; i++)
    (void)fprintf(stderr, "%p ", this->nodes[i]);
  len = this->indices.getLength();
  (void)fprintf(stderr, "\n\tIDX(len=%d): ", len);
  for (int j=0; j < len; j++)
    (void)fprintf(stderr, "%d ", this->indices[j]);
  (void)fprintf(stderr, "\n");
#endif // debug

  return this->nodes[this->getLength() - 1];
}

/*!
  This method returns the node at \a index in the path, counted from 0
  at the head.
*/
SoNode *
SoPath::getNode(const int index) const
{
#if COIN_DEBUG
  if (index < 0 || index >= this->getFullLength()) {
    SoDebugError::post("SoPath::getNode", "index %d is out of bounds.",
                       index);
    return NULL;
  }
#endif // COIN_DEBUG
  return this->nodes[index];
}

/*!
  This method returns the node at \a index in the path, counting from
  the tail.  The \a index argument should be zero or positive.
*/
SoNode *
SoPath::getNodeFromTail(const int index) const
{
#if COIN_DEBUG
  if (index < 0 || index >= this->getLength()) {
    SoDebugError::post("SoPath::getNodeFromTail",
                       "index %d is out of bounds.", index);
    return NULL;
  }
#endif // COIN_DEBUG
  return this->nodes[this->getLength() - index - 1];
}

/*!
  This method returns the child index of the node at \a index in the
  path.
*/
int
SoPath::getIndex(const int index) const
{
#if COIN_DEBUG
  if (index < 0 || index >= this->getFullLength()) {
    SoDebugError::post("SoPath::getIndex", "index %d is out of bounds.",
                       index);
    return -1;
  }
#endif // COIN_DEBUG

  return this->indices[index];
}

/*!
  This method returns the child index of the node at \a index in the
  path, counting from the tail.  The \a index argument should be zero
  or positive.
*/
int
SoPath::getIndexFromTail(const int index) const
{
#if COIN_DEBUG
  if (index < 0 || index >= this->getLength()) {
    SoDebugError::post("SoPath::getIndexFromTail",
                       "index %d is out of bounds.", index);
    return -1;
  }
#endif // COIN_DEBUG
  return this->indices[this->getLength() - index - 1];
}

/*!
  This method returns the number of nodes in the path. Only the
  "visible" nodes are counted, i.e. hidden nodes of e.g. nodekits are
  not included.

  If you need the actual path length, you need to cast your path to
  SoFullPath and use SoFullPath::getLength().
*/
int
SoPath::getLength(void) const
{
  if (this->firsthiddendirty) {
    ((SoPath*)this)->setFirstHidden();
  }
  if (this->firsthidden >= 0) {
#ifdef COIN_EXTRA_DEBUG
    assert(this->firsthidden < this->nodes.getLength());
#endif // COIN_EXTRA_DEBUG
    return this->firsthidden + 1;
  }
  return this->nodes.getLength();
}

/*!
  This method truncates the path to the given \a length (i.e. all
  nodes in the path list from index \a length and onwards will be
  removed from the path list).
*/
void
SoPath::truncate(const int length)
{
  this->truncate(length, TRUE);
}

// This method truncates the path to the given length.
void
SoPath::truncate(const int length, const SbBool donotify)
{
  assert((length >= 0) && (length <= this->getFullLength()) &&
         "invalid truncation length");

#if COIN_DEBUG
  // Don't run this alive test if the node list is not referencing.
  // SoTempPath (used by actions to minimize overhead while
  // traversing) does not reference count the node pointers, so it
  // might leave dangling node pointers in the path.  This is ok
  // though, so we should not assert.  pederb, 2002-10-01
  if (this->nodes.isReferencing()) {
    // We have inserted this check here to run through the full path
    // list and detect dangling node pointers (by using the
    // SoBase::assertAlive() debugging helper function). This should
    // help us cause an early break-down for certain types of major
    // internal consistency problems in the library, which could
    // otherwise cause silent memory corruption and mysterious crashes
    // at unrelated locations.
    //
    // mortene -- the paranoid android.
    SoFullPath * fp = (SoFullPath *)this;
    for (int l = 0; l < fp->getLength(); l++) {
      SoNode * n = fp->getNode(l);
      // FIXME: are there actually conditions where we can "legally" get
      // a NULL pointer here? Or would that be an indication of an
      // internal error? 20020928 mortene.
      if (n) { n->assertAlive(); }
    }
  }
#endif // COIN_DEBUG


  // Remove ourself as an auditor to the nodes' children lists.
  if (this->isauditing) {
    for (int i = length; i < this->getFullLength(); i++) {
      SoChildList * cl = this->nodes[i]->getChildren();
#if COIN_DEBUG && 0 // debug
      if (cl) {
        SoDebugError::postInfo("SoPath::truncate",
                               "nodes[%d]=%p childlist=%p "
                               "removePathAuditor(%p)",
                               i, this->nodes[i], cl, this);
      }
#endif // debug
      if (cl) cl->removePathAuditor(this);
    }
  }

  this->nodes.truncate(length);
  this->indices.truncate(length);

  if (!this->firsthiddendirty && length <= this->firsthidden)
    this->firsthiddendirty = TRUE;

  if (donotify && this->isauditing) this->startNotify();
}

// *************************************************************************

/*!
  This method returns the index of the last node that is the same for
  both paths, or \c -1 if the paths don't even start on the same node.
*/
int
SoPath::findFork(const SoPath * const path) const
{
  const int len = SbMin(this->getFullLength(), path->getFullLength());
  if (len == 0 || this->nodes[0] != path->nodes[0]) return -1;
  int i;

  const int * thisidxptr = this->indices.getArrayPtr();
  const int * pathidxptr = path->indices.getArrayPtr();

  for (i = 1; i < len; i++) {
    if (thisidxptr[i] != pathidxptr[i]) break;
  }
  return i - 1;
}

/*!
  Returns the index of \a node if found in the \e full path (possibly
  also in the normally hidden parts), or \c -1 otherwise.

  Note that this method is not part of the original Open Inventor API.
*/
int
SoPath::findNode(const SoNode * const node) const
{
  const int len = this->getFullLength();
  const SoNode ** nodeptr = (const SoNode**) this->nodes.getArrayPtr();
  for (int i = 0; i < len; i++) if (nodeptr[i] == node) return i;
  return -1;
}

/*!
  This method returns \c TRUE is \a node is contained somewhere in the
  \e full path (possibly also in the normally hidden parts), and \c
  FALSE otherwise.
*/
SbBool
SoPath::containsNode(const SoNode * const node) const
{
  return (this->findNode(node) == -1) ? FALSE : TRUE;
}

/*!
  This method returns \c TRUE if path is contained in the path, and \c
  FALSE otherwise.
*/
SbBool
SoPath::containsPath(const SoPath * const path) const
{
  int thislen = this->getFullLength();
  int thatlen = path->getFullLength();
  if (thatlen == 0 || thatlen > thislen) return FALSE;

  int offset = this->findNode(path->nodes[0]); // find head in this path
  if ((offset < 0) || (offset + thatlen > thislen)) return FALSE;

  const int * thisidxptr = this->indices.getArrayPtr();
  const int * pathidxptr = path->indices.getArrayPtr();

  for (int i = 1; i < thatlen; i++) {
    if (thisidxptr[offset+i] != pathidxptr[i]) return FALSE;
  }
  return TRUE;
}

/*!
  Compares contents of path \a lhs and path \a rhs, and returns \c
  TRUE if they are equal.
*/
SbBool
operator==(const SoPath & lhs, const SoPath & rhs)
{
  if (&lhs == &rhs) return TRUE;
  const int len = lhs.getFullLength();
  if (len != rhs.getFullLength()) return FALSE;
  if (len == 0) return TRUE;
  if (lhs.nodes[0] != rhs.nodes[0]) return FALSE;

  const int * lhsptr = lhs.indices.getArrayPtr();
  const int * rhsptr = rhs.indices.getArrayPtr();

  for (int i = 1; i < len; i++) {
    if (lhsptr[i] != rhsptr[i]) return FALSE;
  }
  return TRUE;
}

/*!
  Returns \c TRUE if paths \a lhs and \a rhs does not contain the same
  nodes in the same order.
*/
SbBool
operator!=(const SoPath & lhs, const SoPath & rhs)
{
  return !(lhs == rhs);
}

// *************************************************************************

/*!
  This method returns a copy of the path starting at \a
  startfromnodeindex with length \a numnodes.  If \a numnodes is 0
  (default) the path will be copied to the end.

  The returned SoPath instance will have an initial reference count of
  0. It is the caller's responsibility to ref() and unref() it
  according to use.
*/
SoPath *
SoPath::copy(const int startfromnodeindex, int numnodes) const
{
#if COIN_DEBUG
  if (startfromnodeindex < 0 ||
      startfromnodeindex >= this->getFullLength()) {
    SoDebugError::post("SoPath::copy",
                       "startfromnodeindex was out of bounds with %d.",
                       startfromnodeindex);
    return NULL;
  }
#endif // COIN_DEBUG
  if (numnodes == 0) numnodes = this->getFullLength() - startfromnodeindex;

#if COIN_DEBUG
  if (numnodes <= 0 ||
      (startfromnodeindex + numnodes) > this->getFullLength()) {
    SoDebugError::post("SoPath::copy", "numnodes has invalid value %d",
                       numnodes);
    return NULL;
  }
#endif // COIN_DEBUG

  SoPath * newpath = new SoPath(numnodes);
  // Note: it is not by oversight that we're not copying the
  // isauditing flag -- this is the way copy() is supposed to work. If
  // we change this behavior, we get problems when the ``this''
  // pointer is an SoTempPath and the newly created SoPath _is_
  // supposed to audit its path for changes.

  const int max = startfromnodeindex + numnodes;
  for (int i = startfromnodeindex; i < max; i++) {
    newpath->append(this->nodes[i], this->indices[i]);
  }
  newpath->firsthiddendirty = TRUE;
  return newpath;
}

// *************************************************************************

/*!
  This static method is for retrieving an SoPath by its \a name.  The
  last registered SoPath with the given \a name is returned, or \c
  NULL if no SoPath by \a name exists.
*/
SoPath *
SoPath::getByName(const SbName name)
{
  SoBase * b = SoBase::getNamedBase(name, SoPath::getClassTypeId());
  if (!b) return NULL;
  return (SoPath *)b;
}

/*!
  This static method is for finding all the paths with a given \a name
  and append them to the \a l list. The number of SoPath instances
  with \a name found is returned.
*/
int
SoPath::getByName(const SbName name, SoPathList & l)
{
  SoBaseList bl;
  int nr = SoBase::getNamedBases(name, bl, SoPath::getClassTypeId());
  for (int i=0; i < nr; i++) l.append((SoPath *)bl[i]);
  return nr;
}

// *************************************************************************

/*!
  This method is called when a node in the path chain has a child
  added, to update the index of its child.

  \a newindex is the index of the child which was inserted. If \a
  newindex is lower than the index value of the child node stored in
  the path, the path is updated accordingly.
*/
void
SoPath::insertIndex(SoNode * const parent, const int newindex)
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoPath::insertIndex",
                         "(%p)  parent=%p, newindex=%d",
                         this, parent, newindex);
  sopath_dump(this);
#endif // debug

  if (parent == this->nodes[this->getFullLength() - 1]) return;

  int pos = this->findNode(parent);
#ifdef COIN_EXTRA_DEBUG
  assert(pos != -1); // shouldn't be notified if parent is not in path
#endif // COIN_EXTRA_DEBUG
  pos++;

  if (newindex <= this->indices[pos]) this->indices[pos]++;
}

/*!
  This method is called when a node in the path chain has a child
  removed, to update the index of its child.

  \a oldindex was the index of the removed child. If \a oldindex is
  lower than or equal to the index value of the child node stored in
  the path, the path is updated accordingly.
*/
void
SoPath::removeIndex(SoNode * const parent, const int oldindex)
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoPath::removeIndex",
                         "(%p)  parent=%p (%s), oldindex=%d",
                         this,
                         parent,
                         parent->getTypeId().getName().getString(),
                         oldindex);
  sopath_dump(this);
#endif // debug

  if (parent == this->nodes[this->getFullLength() - 1]) return;

  int pos = this->findNode(parent);
#if COIN_DEBUG
  // shouldn't be notified if parent is not in path
  if (!(pos >= 0 && pos < this->getFullLength()-1)) {
    SoDebugError::post("SoPath::removeIndex",
                       "failure: pos==%d (len=%d), parent=%p (%s)",
                       pos, this->getFullLength(),
                       parent,
                       parent->getTypeId().getName().getString());
    return;
  }
#endif // COIN_DEBUG
  pos++;

  if (oldindex < this->indices[pos]) this->indices[pos]--;
  // if node in path is removed, we have to truncate path
  else if (oldindex == this->indices[pos]) this->truncate(pos);
}

/*!
  This method is called when a node \a newchild replaces a node in the
  path. \a index is the position of the child which has been replaced.
*/
void
SoPath::replaceIndex(SoNode * const parent, const int index,
                     SoNode * const newchild)
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoPath::replaceIndex",
                         "(%p)  parent=%p (%s), newchild=%p (%s)",
                         this,
                         parent, parent->getTypeId().getName().getString(),
                         newchild, newchild->getTypeId().getName().getString());

#endif // debug

  if (parent == this->nodes[this->getFullLength() - 1]) return;

  int pos = this->findNode(parent);
#ifdef COIN_EXTRA_DEBUG
  assert(pos != -1); // shouldn't be notified if parent is not in path
#endif // COIN_EXTRA_DEBUG
  pos++;

  if (index == this->indices[pos]) {
    // The path below the replacement will not be correct -- so let's
    // truncate the path here. Truncating will also remove us as
    // auditors from the SoChildList instances we're watching.  Don't
    // notify when truncating as the append call will take care of
    // that (we don't want to notify before the path is correct
    // anyway).
    this->truncate(pos, FALSE); // FALSE = don't notify
    // append replacement node
    this->append(newchild, index);
  }

#if COIN_DEBUG && 0 // debug
  sopath_dump(this);
#endif // debug
}

// *************************************************************************

/*!
  This method is used to write the contents of an SoPath.

  A path is written as:
  \code
    Path {
      HEAD-NODE-OF-SUBGRAPH
      NUMBER-OF-INDICES
      INDEX0
      INDEX1
      ...
    }
  \endcode


  [The rest of the documentation for this method only explains the
  reason behind a mismatch in behavior between the original Open
  Inventor and Coin. Don't read it if you're not taking a particular
  interest. (Short version: we do SoPath export in a somewhat more
  inefficient way to avoid the potential for bugs).]

  Note that unlike Open Inventor, we write the complete subgraph below
  the head node. Only writing the parts of the subgraph affecting the
  state for nodes within the path is error prone if a subgraph is
  written out as part of path \e before it is written out
  "properly". Consider writing a scene graph which looks like this (in
  memory):

  \code
    DEF top_sep Separator {
      Cone { }
      DEF a_sphere Sphere { }
      Cube { }
    }

    DEF path_switch PathSwitch {
      path Path {
        ...path from "top_sep" to "a_sphere"...
      }
    }
  \endcode

  ..if we now do:

  \code
    SoSeparator * root = new SoSeparator;
    root->addChild([ptr to path_switch]);
    root->addChild([ptr to top_sep]);
    SoWriteAction wa;
    wa.apply(root);
  \endcode

  ..we would get the scene graph exported like this:

  \code
    Separator {
      DEF path_switch PathSwitch {
        path Path {
          DEF top_sep Separator {
            DEF a_sphere Sphere {
            }
          }
          1
          0
        }
      }
      USE top_sep
    }
  \endcode

  ..and as you can see, \e both the Cone and the Cube nodes have
  vanished, as they were not important for the part per se, and not
  written as part of it.

  This is why we do full subgraph export for head nodes in paths.
*/
void
SoPath::write(SoWriteAction * action)
{
  SoOutput * out = action->getOutput();

  if (out->getStage() == SoOutput::COUNT_REFS) {
    inherited::addWriteReference(out, FALSE);
    if (!SoWriterefCounter::instance(out)->hasMultipleWriteRefs(this)) {
      SoWriteAction wa(out);
      wa.continueToApply(this->getHead());
    }
  }
  else if (out->getStage() == SoOutput::WRITE) {
    if (this->writeHeader(out, FALSE, FALSE)) return;

    SoWriteAction wa(out);
    wa.continueToApply(this->getHead());

    int nrindices = this->indices.getLength();
    if (!out->isBinary()) out->indent();
    out->write(nrindices - 1);
    if (!out->isBinary()) out->write('\n');

    for (int i=1; i < nrindices; i++) {
      if (!out->isBinary()) out->indent();
      out->write(this->indices[i]);
      if (!out->isBinary()) out->write('\n');
    }

    this->writeFooter(out);
  }
  else assert(0 && "unknown stage");
}

// *************************************************************************

/*!
  Return TRUE if the notification chain \a l will affect the path. \a
  l will affect the path either if the notification is in the path
  (the notification started on a field in a node in the path), or off
  the path (the notification started in a non-separator node that is
  left of the path's node).
*/
SbBool
SoPath::isRelevantNotification(SoNotList * const l) const
{
  int len = this->getFullLength();
  if (len == 0) return FALSE;

  const SoNotRec * rec = l->getLastRec();
  if (len == 1) return rec->getBase() == (SoBase*) this->getHead();

  int pathidx = 1;

  // start at the root node and follow the notification chain until we
  // find a node that is off the path or a notification that is not
  // from a node.
  do {
    rec = rec->getPrevious();
    if (rec) {
      // check if we might be off path
      if ((SoBase*) this->getNode(pathidx) != rec->getBase()) break;
      assert(rec->getBase()->isOfType(SoNode::getClassTypeId()));
    }
    pathidx++;
  } while (rec && (rec->getType() == SoNotRec::PARENT) && (pathidx < len));

  // check if we're off path
  if (rec && (pathidx < len) && (rec->getType() == SoNotRec::PARENT)) {
    SoBase * base = rec->getBase();
    assert(base->isOfType(SoNode::getClassTypeId()));
    SoNode * parent = this->getNode(pathidx-1);
    assert(parent->getChildren());
    int childidx = parent->getChildren()->find((SoNode*) base);
    assert(childidx >= 0);

    // check if node is to the right of the path
    if (childidx > this->getIndex(pathidx)) return FALSE;

    // check if the notification is from inside a separator node,
    // then it will not affect the path
    do {
      assert(rec->getBase()->isOfType(SoNode::getClassTypeId()));
      if (!((SoNode*)rec->getBase())->affectsState()) return FALSE;
      rec = rec->getPrevious();
    } while(rec && rec->getType() == SoNotRec::PARENT);
  }
  return TRUE;
}

/*!
  Set whether or not to audit the nodes in the path to detect changes.
*/
void
SoPath::auditPath(const SbBool flag)
{
  // Don't change value in a "running" SoPath.
  assert(this->getFullLength() == 0);

  this->isauditing = flag;
}

/*!
  \copybrief SoBase::getClassTypeId(void)
*/
SoType
SoPath::getClassTypeId(void)
{
  return SoPath::classTypeId;
}

// Documented in parent class.
SoType
SoPath::getTypeId(void) const
{
  return SoPath::classTypeId;
}

// This static method creates a new SoPath object and returns a
// pointer to it. Used by the runtime type system.
/*!
  \COININTERNAL

  This static method creates a new SoPath object and returns a
  pointer to it. Used by the runtime type system.
*/
void *
SoPath::createInstance(void)
{
  return new SoPath;
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoPath::initClass(void)
{
  assert((SoPath::classTypeId == SoType::badType()) &&
         "call SoPath::initClass only once!");

  coin_atexit((coin_atexit_f*)SoPath::cleanupClass, CC_ATEXIT_NORMAL);

  SoPath::classTypeId = SoType::createType(inherited::getClassTypeId(),
                                           SbName("Path"),
                                           &SoPath::createInstance);
}

/*!
  This static method cleans up static data of the SoPath class.
*/
void
SoPath::cleanupClass(void)
{
  SoPath::classTypeId = SoType::badType();
}


// *************************************************************************

// This virtual method is used to read an SoPath from the input
// stream. See the documentation on write() for an explanation of the
// file format.
SbBool
SoPath::readInstance(SoInput * in, unsigned short COIN_UNUSED_ARG(flags))
{
  SoBase * baseptr;
  if (!SoBase::read(in, baseptr, SoNode::getClassTypeId())) return FALSE;
  this->setHead((SoNode *)baseptr);

  int nrindices;
  if (!in->read(nrindices)) {
    SoReadError::post(in, "Couldn't read number of indices");
    return FALSE;
  }

  for (int i=0; i < nrindices; i++) {
    int index;
    if (!in->read(index)) {
      SoReadError::post(in, "Couldn't read index value");
      return FALSE;
    }

    SoChildList * tailchildren = this->getTail()->getChildren();
    if (!tailchildren || index < 0 || index >= tailchildren->getLength()) {
      SoReadError::post(in, "Invalid index value %d", index);
      return FALSE;
    }

    this->append(index);
  }

  return TRUE;
}

//
// Private method that scans the nodes and finds the first (if any)
// node with hidden children. If a node with hidden children is found,
// getLength() will return this as the length of the path. This also
// affects all tail-operations.
//
void
SoPath::setFirstHidden(void)
{
  this->firsthidden = -1;

  const int nr = this->nodes.getLength();
  for (int i = 0; i < nr; i++) {
    const SoNode * n = this->nodes[i];
    if (n == NULL) { continue; } // this is a valid case, for instance
                                 // if getLength() is called right
                                 // after SoAction::pushCurPath(void)
                                 // has been invoked

    if (has_hidden_children(n)) {
      this->firsthidden = i;
      break;
    }
  }

  this->firsthiddendirty = FALSE;
}
