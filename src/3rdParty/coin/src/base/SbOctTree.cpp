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
  \class SbOctTree SbOctTree.h Inventor/SbOctTree.h
  \brief The SbOctTree class defines a generic octree for fast geometry searches.

  \ingroup coin_base

  \COIN_CLASS_EXTENSION
*/

// *************************************************************************

#include <cassert>
#include <cfloat>
#include <cstddef>

#include <Inventor/SbOctTree.h>
#include <Inventor/SbSphere.h>
#include <Inventor/SbPlane.h>
#include <Inventor/errors/SoDebugError.h>

// *************************************************************************

/*!
  \struct SbOctTreeFuncs SbOctTree.h Inventor/SbOctTree.h

  The SbOctTreeFuncs struct is used to specify callback functions for
  working with items in an SbOctTree.

  The only function pointer that \e must be set up is \c
  insideboxfunc. The other functions must be set if you intend to use
  the corresponding find methods in SbOctTree.
*/

/*!
  \var SbOctTreeFuncs::ptinsidefunc
  Should return whether a point is inside item.
*/
/*!
  \var SbOctTreeFuncs::insideboxfunc
  Should return whether item is either fully or partially inside a box.
 */
/*!
  \var SbOctTreeFuncs::insidespherefunc
  Should return whether item is either fully or partially inside a
  sphere.
 */
/*!
  \var SbOctTreeFuncs::insideplanesfunc
  Should return whether item is either fully or partially inside a set of
  planes.
*/

// *************************************************************************

//
// got to have unique intersection funcs, therefore the standard
// Inventor intersection functions won't do. E.g. SbBox3f::pointInside()
// will return TRUE for all eight child-boxes if the center point of the
// parent box is tested, which is correct, but not really usable for an
// octree.
//

static SbBool
intersect_box_sphere(const SbBox3f & box,
                     const SbSphere & sphere)
{
  const SbVec3f &C = sphere.getCenter();
  const SbVec3f &Bmin = box.getMin();
  const SbVec3f &Bmax = box.getMax();
  float dmin = 0;
  for (int i = 0; i < 3; i++) {
    if (C[i] < Bmin[i]) dmin += SbSqr(C[i] - Bmin[i]);
    else if (C[i] > Bmax[i]) dmin += SbSqr(C[i] - Bmax[i]);
  }
  return (dmin <= SbSqr(sphere.getRadius()));
}

static SbBool
intersect_box_box(const SbBox3f & box1, const SbBox3f & box2)
{
  return ! (box1.getMin()[0] >= box2.getMax()[0] ||
            box1.getMax()[0] < box2.getMin()[0] ||
            box1.getMin()[1] >= box2.getMax()[1] ||
            box1.getMax()[1] < box2.getMin()[1] ||
            box1.getMin()[2] >= box2.getMax()[2] ||
            box1.getMax()[2] < box2.getMin()[2]);
}

static SbBool
point_inside_box(const SbVec3f & pt, const SbBox3f & box)
{
  return ! (pt[0] < box.getMin()[0] ||
            pt[0] >= box.getMax()[0] ||
            pt[1] < box.getMin()[1] ||
            pt[1] >= box.getMax()[1] ||
            pt[2] < box.getMin()[2] ||
            pt[2] >= box.getMax()[2]);
}

static SbBool
box_inside_planes(const SbBox3f & box, const SbPlane * const planes,
                  const int numplanes)
{
  // Uses box "radius" for speed.
  // FIXME: consider just checking all 8 points of the box. pederb, 20000811
  SbVec3f size = (box.getMax() - box.getMin()) * 0.5f;
  float radius = static_cast<float>(sqrt(size[0]*size[0] + size[1]*size[1] + size[2]*size[2]));

  SbVec3f center = (box.getMin() + box.getMax()) * 0.5f;

  for (int i = 0; i < numplanes; i++) {
    if (planes[i].getDistance(center) < -radius) return FALSE;
  }
  return TRUE;
}

// *************************************************************************

class SbOctTreeNode
{
public:
  SbOctTreeNode(const SbBox3f & b);
  ~SbOctTreeNode();

  void addItem(void * const item,
               const SbOctTreeFuncs & itemfuncs,
               const int maxitems);
  void removeItem(void * const item,
                  const SbOctTreeFuncs & itemfuncs);
  void findItems(const SbVec3f &pos,
                 SbList <void*> &destarray,
                 const SbOctTreeFuncs &itemfuncs,
                 const SbBool removeduplicates) const;
  void findItems(const SbBox3f &box,
                 SbList <void*> &destarray,
                 const SbOctTreeFuncs &itemfuncs,
                 const SbBool removeduplicates) const;
  void findItems(const SbSphere &sphere,
                 SbList <void*> &destarray,
                 const SbOctTreeFuncs &itemfuncs,
                 const SbBool removeduplicates) const;
  void findItems(const SbPlane * const planes,
                 const int numPlanes,
                 SbList <void*> &destarray,
                 const SbOctTreeFuncs &itemfuncs,
                 const SbBool removeduplicates) const;

  const SbBox3f & getBBox(void) const { return this->nodesize; }

  void debugTree(FILE *fp, const int indent) const;

private:
  SbBool isLeaf(void) const { return this->children[0] == NULL; }
  SbBool isGroup(void) const { return ! this->isLeaf(); }

  unsigned int totalNumberOfItems(void) const;

  static void split3Way(const SbBox3f & box, SbBox3f * destarray);
  SbBool splitNode(const SbOctTreeFuncs & funcs);

  SbOctTreeNode * children[8];
  SbList <void*> items;
  SbBox3f nodesize;
};

// Returns all items of the node, including all items in child nodes
// if we're not a leaf node.
unsigned int
SbOctTreeNode::totalNumberOfItems(void) const
{
  unsigned int nr = this->items.getLength();

  if (this->isGroup()) {
    for (int i = 0; i < 8; i++) {
      nr += this->children[i]->totalNumberOfItems();
    }
  }
  return nr;
}

void 
SbOctTreeNode::debugTree(FILE *fp, const int indent) const
{
  (void)fprintf(fp, "%02d", indent - 1);

  int i;
  for (i = 0; i < indent; i++) { (void)fprintf(fp, "  "); }

  const SbVec3f & vmin = this->nodesize.getMin();
  const SbVec3f & vmax = this->nodesize.getMax();

  (void)fprintf(fp, "%s, %u items, ",
                this->isLeaf() ? "Leaf" : "Group", this->totalNumberOfItems());
  (void)fprintf(fp, "box==<%.2f, %.2f, %.2f>-<%.2f, %.2f, %.2f>",
                vmin[0], vmin[1], vmin[2], vmax[0], vmax[1], vmax[2]);
  (void)fprintf(fp, "\n");

  if (this->isGroup()) {
    for (i = 0; i < 8; i++) {
      this->children[i]->debugTree(fp, indent+1);
    }
  }
}

static void
add_to_array(SbList<void *> & array, void * ptr)
{
  // FIXME: this is rather awful, resulting in n^2 algorithm time.
  // Should change to using the array as a sorted set. 20050512 mortene.
  if (array.find(ptr) == -1) { array.append(ptr); }
}

SbOctTreeNode::SbOctTreeNode(const SbBox3f & b)
{
  for (int i = 0; i < 8; i++) {
    this->children[i] = NULL;
  }
  this->nodesize = b;
}

SbOctTreeNode::~SbOctTreeNode()
{
  if (this->isGroup()) {
    for (int i = 0; i < 8; i++) delete children[i];
  }
}

void
SbOctTreeNode::addItem(void * const item,
                       const SbOctTreeFuncs & itemfuncs,
                       const int maxitems)
{
  if (this->isGroup()) { // node has been split
    for (int i = 0; i < 8; i++) {
      if (itemfuncs.insideboxfunc(item, this->children[i]->nodesize)) {
        this->children[i]->addItem(item, itemfuncs, maxitems);
      }
    }
  }
  else if (this->items.getLength() >= maxitems) {
    // avoid trying a split too often by using a modulo
    if ((this->items.getLength() % (maxitems+1) == maxitems) &&
        this->splitNode(itemfuncs)) {
      this->addItem(item, itemfuncs, maxitems);
    }
    else {
      this->items.append(item);
    }
  }
  else {
    this->items.append(item);
  }
}

void
SbOctTreeNode::removeItem(void * const item, const SbOctTreeFuncs & itemfuncs)
{
  if (children[0]) {
    for (int i = 0; i < 8; i++) {
      if (itemfuncs.insideboxfunc(item, this->children[i]->nodesize)) {
        this->children[i]->removeItem(item, itemfuncs);
      }
    }
  }
  else {
    int n = this->items.getLength();
    for (int i = 0; i < n; i++) {
      if (this->items[i] == item) {
        this->items.removeFast(i);
        n--;
      }
    }
  }
}

void
SbOctTreeNode::findItems(const SbVec3f & pos,
                         SbList <void*> & destarray,
                         const SbOctTreeFuncs & itemfuncs,
                         const SbBool removeduplicates) const
{
  if (this->isGroup()) {
    for (int i = 0; i < 8; i++) {
      if (point_inside_box(pos, this->children[i]->nodesize)) {
        this->children[i]->findItems(pos, destarray,
                                     itemfuncs, removeduplicates);
      }
    }
  }
  else {
    int n = this->items.getLength();
    for (int i = 0; i < n; i++) {
      void *item = this->items[i];
      if (itemfuncs.ptinsidefunc(item, pos)) {
        if (removeduplicates)
          add_to_array(destarray, item);
        else
          destarray.append(item);
      }
    }
  }
}

void
SbOctTreeNode::findItems(const SbBox3f & box,
                         SbList <void*> & destarray,
                         const SbOctTreeFuncs & itemfuncs,
                         const SbBool removeduplicates) const
{
  if (this->isGroup()) {
    for (int i = 0; i < 8; i++) {
      if (intersect_box_box(box, this->children[i]->nodesize))
        this->children[i]->findItems(box, destarray,
                                     itemfuncs, removeduplicates);
    }
  }
  else {
    int n = this->items.getLength();
    for (int i = 0; i < n; i++) {
      void *item = this->items[i];
      if (itemfuncs.insideboxfunc(item, box)) {
        if (removeduplicates)
          add_to_array(destarray, item);
        else
          destarray.append(item);
      }
    }
  }
}

void
SbOctTreeNode::findItems(const SbSphere & sphere,
                         SbList <void*> & destarray,
                         const SbOctTreeFuncs & itemfuncs,
                         const SbBool removeduplicates) const
{
  if (this->isGroup()) {
    for (int i = 0; i < 8; i++) {
      if (intersect_box_sphere(this->children[i]->nodesize, sphere))
        this->children[i]->findItems(sphere, destarray,
                                     itemfuncs, removeduplicates);
    }
  }
  else {
    int n = this->items.getLength();
    for (int i = 0; i < n; i++) {
      void * item = this->items[i];
      if (itemfuncs.insidespherefunc(item, sphere)) {
        if (removeduplicates)
          add_to_array(destarray, item);
        else
          destarray.append(item);
      }
    }
  }
}

void
SbOctTreeNode::findItems(const SbPlane * const planes,
                         const int numplanes,
                         SbList <void*> & destarray,
                         const SbOctTreeFuncs & itemfuncs,
                         const SbBool removeduplicates) const
{
  if (this->isGroup()) {
    for (int i = 0; i < 8; i++) {
      if (box_inside_planes(this->children[i]->nodesize, planes, numplanes)) {
        this->children[i]->findItems(planes, numplanes,
                                     destarray, itemfuncs, removeduplicates);
      }
    }
  }
  else {
    int n = this->items.getLength();
    for (int i = 0; i < n; i++) {
      void *item = this->items[i];
      if (itemfuncs.insideplanesfunc(item, planes, numplanes)) {
        if (removeduplicates)
          add_to_array(destarray, item);
        else
          destarray.append(item);
      }
    }
  }
}

void
SbOctTreeNode::split3Way(const SbBox3f & box, SbBox3f * dest)
{
  SbVec3f mid = (box.getMin() + box.getMax()) * 0.5f;

  for (int i = 0; i < 8; i++) {
    dest[i].setBounds((i & 4) ? box.getMin()[0] : mid[0],
                      (i & 2) ? box.getMin()[1] : mid[1],
                      (i & 1) ? box.getMin()[2] : mid[2],
                      (i & 4) ? mid[0] : box.getMax()[0],
                      (i & 2) ? mid[1] : box.getMax()[1],
                      (i & 1) ? mid[2] : box.getMax()[2]);
  }
}

SbBool
SbOctTreeNode::splitNode(const SbOctTreeFuncs & itemfuncs)
{
  SbBox3f childbox[8];
  SbOctTreeNode::split3Way(this->nodesize, childbox);
  int i;
  for (i = 0; i < 8; i++) {
    this->children[i] = new SbOctTreeNode(childbox[i]);
  }

  const int n = this->items.getLength();
  for (i = 0; i < n; i++) {
    void *item = this->items[i];
    for (int j = 0; j < 8; j++) {
      if (itemfuncs.insideboxfunc(item, childbox[j])) {
        this->children[j]->items.append(item);
      }
    }
  }

  // Check to see if one or more of the new nodes contains *all* items
  // from the parent node (i.e. this node). If so, the split won't
  // gain us any in processing time (it will likely be hurtful), so
  // decide against splitting.

  for (i = 0; i < 8; i++) {
    if (this->children[i]->items.getLength() == n) {
      for (int j = 0; j < 8; j++) {
        delete this->children[j];
        this->children[j] = NULL;
      }
      return FALSE;
    }
  }

  // Box was indeed split, we're now a group node, so truncate our
  // list of items and carry on with new tree structure.

  this->items.truncate(0, TRUE);
  return TRUE;
}

// *************************************************************************

/*!
  Constructor.
*/
SbOctTree::SbOctTree(const SbBox3f & bbox,
                     const SbOctTreeFuncs & itemfuncs,
                     const int maxitems)
  : topnode(new SbOctTreeNode(bbox)),
    itemfuncs(itemfuncs),
    maxitemspernode(maxitems)
{
}

/*!
  Destructor.
*/
SbOctTree::~SbOctTree()
{
  delete this->topnode;
}

/*!
  Restores this octree to an empty octree. The bounding
  box will still be the same though.
*/
void
SbOctTree::clear(void)
{
  SbBox3f bbox = this->topnode->getBBox();
  delete this->topnode;
  this->topnode = new SbOctTreeNode(bbox);
}

/*!
  Adds an item to this octree.
*/
void
SbOctTree::addItem(void * const item)
{
  // Note that the assert() below can hit due to floating point
  // inaccuracies.
  //
  // When that happens, an easy and fairly decent fix is usually to
  // add a bit of slack on the caller side to the input bbox argument
  // to SbOctTree::SbOctTree().

  // FIXME: a better solution would be to not force an static bbox
  // upon the SbOctTree through its constructor, but let it
  // dynamically expand / re-structure itself as items are added.
  //
  // An easy, but a bit inefficient, way to do that would be to simply
  // store a copy of all items in the octree structure, destruct it,
  // restore a new top-level node, and then re-add all items to let a
  // new octree structure build itself.
  //
  // 20050512 mortene.
#if COIN_DEBUG && 0 // debug
  const SbBox3f & b = this->topnode->getBBox();
  if (!this->itemfuncs.insideboxfunc(item, b)) {
    const SbVec3f & bmin = b.getMin();
    const SbVec3f & bmax = b.getMax();
    SoDebugError::post("SbOctTree::addItem",
                       "tree bbox==<%f, %f, %f>, <%f, %f, %f>",
                       bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]);
  }
#endif // debug
  assert(this->itemfuncs.insideboxfunc(item, this->topnode->getBBox()) &&
         "bbox of item outside the octtree top-level bbox");

  this->topnode->addItem(item, this->itemfuncs, this->maxitemspernode);
}

/*!
  Removes the item from the octree. The octree will not be
  modified/simplified even when all items are removed.
*/
void
SbOctTree::removeItem(void * const item)
{
  this->topnode->removeItem(item, this->itemfuncs);
}

/*!
  Finds all items which contains the point \a pos. Items are
  returned in \a destarray.

  If \a removeduplicates is TRUE (the default), \a destarray will not
  contain duplicate items. This is not an optimized process, so if
  you're looking for speed you should set this to FALSE and do
  your own post processing of the array of returned items.

  \DANGEROUS_ALLOC_RETURN
*/
void
SbOctTree::findItems(const SbVec3f & pos,
                     SbList <void*> & destarray,
                     const SbBool removeduplicates) const
{
  // FIXME: passing in an SbList is dangerous under Microsoft Windows, as
  // allocation and deallocation can then happen on different
  // C-library's heaps. The other findItems() functions below have the
  // same problem. 20050512 mortene.

  // FIXME: should be straightforward to drop the removeduplicates
  // argument -- it's a hack. We just need to optimize the
  // add_to_array() function above to keep a _sorted_ array.
  //
  // This also goes for the other findItems() functions below, of
  // course.
  //
  // 20050512 mortene.

  assert(this->itemfuncs.ptinsidefunc);
  topnode->findItems(pos, destarray, this->itemfuncs, removeduplicates);
}

/*!
  Finds all items inside \a box. Items are returned in \a destarray.

  If \a removeduplicates is TRUE (the default), \a destarray will not
  contain duplicate items. This is not an optimized process, so if
  you're looking for speed you should set this to FALSE and do
  your own post processing of the array of returned items.

  \DANGEROUS_ALLOC_RETURN
*/
void
SbOctTree::findItems(const SbBox3f & box, SbList <void*> & destarray,
                     const SbBool removeduplicates) const
{
  assert(this->itemfuncs.insideboxfunc);
  this->topnode->findItems(box, destarray, this->itemfuncs, removeduplicates);
}

/*!
  Finds all items inside \a sphere. Items are returned in \a destarray.

  If \a removeduplicates is TRUE (the default), \a destarray will not
  contain duplicate items. This is not an optimized process, so if
  you're looking for speed you should set this to FALSE and do
  your own post processing of the array of returned items.

  \DANGEROUS_ALLOC_RETURN
*/
void
SbOctTree::findItems(const SbSphere & sphere,
                     SbList <void*> & destarray,
                     const SbBool removeduplicates) const
{
  assert(this->itemfuncs.insidespherefunc);
  this->topnode->findItems(sphere, destarray, this->itemfuncs, removeduplicates);
}

/*!
  Finds all items inside \a planes. The method
  SbPlane::isInHalfSpace() should be used, and only items which are
  (partially) inside \e all planes are returned. Items are returned in \a
  destarray.

  If \a removeduplicates is TRUE (the default), \a destarray will not
  contain duplicate items. This is not an optimized process, so if
  you're looking for speed you should set this to FALSE and do
  your own post processing of the array of returned items.

  \DANGEROUS_ALLOC_RETURN
*/
void
SbOctTree::findItems(const SbPlane * const planes,
                     const int numplanes,
                     SbList <void*> & destarray,
                     const SbBool removeduplicates) const
{
  assert(this->itemfuncs.insideplanesfunc);
  this->topnode->findItems(planes, numplanes, destarray, this->itemfuncs,
                           removeduplicates);
}

/*!
  Returns a bounding box enclosing all the elements in the tree.
  This is just the same bounding box which was supplied to the
  constructor.
*/
const SbBox3f &
SbOctTree::getBoundingBox(void) const
{
  return this->topnode->getBBox();
}

void 
SbOctTree::debugTree(FILE * fp)
{
  fprintf(fp, "Oct Tree:\n");
  if (this->topnode) { this->topnode->debugTree(fp, 1); }
}
