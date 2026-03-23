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

#include <Inventor/SbBSPTree.h>
#include <Inventor/SbSphere.h>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cfloat>

#include "coindefs.h"

// define this to do a sorted split (slower, but more efficient?)
//#define BSP_SORTED_SPLIT



/*!
  \class SbBSPTree SbBSPTree.h Inventor/SbBSPTree.h
  \brief The SbBSPTree class provides a binary space partitioning container.

  \ingroup coin_base

  This class can be used to organize searches for 3D points or normals
  in a set in O(log(n)) time.

  Note: SbBSPTree is an extension to the original Open Inventor API.
*/


class coin_bspnode
{
public:
  coin_bspnode(SbList <SbVec3f> *array);
  ~coin_bspnode();

  int addPoint(const SbVec3f &pt, const int maxpts);
  int findPoint(const SbVec3f &pt) const;
  void findPoints(const SbSphere &sphere, SbList <int> &array);
  void findPoints(const SbSphere &sphere, SbIntList & array);
  int removePoint(const SbVec3f &pt);
  void updateIndex(const SbVec3f & pt, int previdx, int newidx);

private:
  void sort();
  void split();
  int leftOf(const SbVec3f &pt) const;

  enum {
    // do not change these values!
    DIM_YZ = 0,
    DIM_XZ = 1,
    DIM_XY = 2,
    DIM_NONE
  };

  coin_bspnode *left;
  coin_bspnode *right;
  int dimension;   // which dimension?
  double position;  // position in dimension (double to avoid floating point precision problems)
  SbList <int> indices;
  SbList <SbVec3f> *pointsArray;
};

coin_bspnode::coin_bspnode(SbList <SbVec3f> *ptsarray)
  : indices(4)
{
  this->left = this->right = NULL;
  this->pointsArray = ptsarray;
  this->dimension = DIM_NONE;
}

coin_bspnode::~coin_bspnode()
{
  delete left;
  delete right;
}

inline int
coin_bspnode::leftOf(const SbVec3f &pt) const
{
  return double(pt[this->dimension]) < this->position;
}

int
coin_bspnode::addPoint(const SbVec3f &pt, const int maxpts)
{
  if (this->left) { // node has been split
    if (this->leftOf(pt)) return this->left->addPoint(pt, maxpts);
    else return this->right->addPoint(pt, maxpts);
  }
  else if (this->indices.getLength() >= maxpts) {
    split();
    return this->addPoint(pt, maxpts);
  }
  else {
    int n = this->indices.getLength();
    int i;
    SbVec3f tmp;
    for (i = 0; i < n; i++) {
      tmp = (*pointsArray)[this->indices[i]];
      if (pt == tmp) break;
    }
    if (i == n) {
      int idx = this->pointsArray->getLength();
      this->pointsArray->append(pt);
      this->indices.append(idx);
      return idx;
    }
    return this->indices[i];
  }
}

int
coin_bspnode::findPoint(const SbVec3f &pt) const
{
  if (this->left) {
    if (this->leftOf(pt)) return this->left->findPoint(pt);
    else return this->right->findPoint(pt);
  }
  else {
    int i, n = this->indices.getLength();
    for (i = 0; i < n; i++) {
      SbVec3f arrpt = (*pointsArray)[this->indices[i]];
      if (pt == arrpt) return this->indices[i];
    }
  }
  return -1;
}

void
coin_bspnode::findPoints(const SbSphere &sphere, SbList <int> &array)
{
  if (this->left) {
    SbVec3f min, max;
    min = max = sphere.getCenter();
    min[this->dimension] -= sphere.getRadius();
    max[this->dimension] += sphere.getRadius();

    if (this->leftOf(min)) this->left->findPoints(sphere, array);
    if (!this->leftOf(max)) this->right->findPoints(sphere, array);
  }
  else {
    int i, n = this->indices.getLength();
    for (i = 0; i < n; i++) {
      SbVec3f pt = (*pointsArray)[this->indices[i]];
      if (sphere.pointInside(pt)) array.append(this->indices[i]);
    }
  }
}

void
coin_bspnode::findPoints(const SbSphere &sphere, SbIntList & array)
{
  if (this->left) {
    SbVec3f min, max;
    min = max = sphere.getCenter();
    min[this->dimension] -= sphere.getRadius();
    max[this->dimension] += sphere.getRadius();

    if (this->leftOf(min)) this->left->findPoints(sphere, array);
    if (!this->leftOf(max)) this->right->findPoints(sphere, array);
  }
  else {
    int i, n = this->indices.getLength();
    for (i = 0; i < n; i++) {
      SbVec3f pt = (*pointsArray)[this->indices[i]];
      if (sphere.pointInside(pt)) array.append(this->indices[i]);
    }
  }
}

/*
  Used to update index after a point is removed.
*/
void
coin_bspnode::updateIndex(const SbVec3f & pt, int previdx, int newidx)
{
  if (this->left) {
    if (this->leftOf(pt)) this->left->updateIndex(pt, previdx, newidx);
    else this->right->updateIndex(pt, previdx, newidx);
  }
  else {
    int i, n = this->indices.getLength();
    for (i = 0; i < n; i++) {
      if (this->indices[i] == previdx) {
        this->indices[i] = newidx;
        return;
      }
    }
  }
}

int
coin_bspnode::removePoint(const SbVec3f &pt)
{
  if (this->left) {
    if (this->leftOf(pt)) return this->left->removePoint(pt);
    else return this->right->removePoint(pt);
  }
  else {
    int i, n = this->indices.getLength();
    for (i = 0; i < n; i++) {
      SbVec3f arrpt = (*pointsArray)[this->indices[i]];
      if (pt == arrpt) {
        int idx = this->indices[i];
        // just remove point from index array here. The invoker will
        // remove the point from the pointsArray
        this->indices.removeFast(i);
        return idx;
      }
    }
  }
  return -1;
}

void
coin_bspnode::split()
{
  assert(this->left == NULL && this->right == NULL);
  this->left = new coin_bspnode(this->pointsArray);
  this->right = new coin_bspnode(this->pointsArray);

  SbBox3f box;
  int i, n = this->indices.getLength();
  for (i = 0; i < n; i++) {
    box.extendBy((*pointsArray)[this->indices[i]]);
  }
  SbVec3f diag = box.getMax() - box.getMin();
  int dim;
  double pos;

  if (diag[0] > diag[1]) {
    if (diag[0] > diag[2]) dim = DIM_YZ;
    else dim = DIM_XY;
  }
  else {
    if (diag[1] > diag[2]) dim = DIM_XZ;
    else dim = DIM_XY;
  }

  this->dimension = dim; // set the dimension

  float mid = (box.getMin()[dim] + box.getMax()[dim]) / 2.0f;
#ifdef BSP_SORTED_SPLIT
  this->sort(); // sort vertices on ascending dimension values

  int splitidx = n / 2;
  pos = ((*pointsArray)[this->indices[splitidx-1]][dim]+
    (*pointsArray)[this->indices[splitidx]][dim]) / 2.0f;

  // got to check and adjust for special cases
  if (pos == box.getMin()[dim] || pos == box.getMax()[dim]) {
    pos = (pos + mid) / 2.0f;
  }

#else
  pos = (double(box.getMin()[this->dimension])+double(box.getMax()[this->dimension])) / 2.0;
#endif // BSP_SORTED_SPLIT

  this->position = pos;

  for (i = 0; i < n; i++) {
    int idx = this->indices[i];
    if (this->leftOf((*pointsArray)[idx]))
      this->left->indices.append(idx);
    else
      this->right->indices.append(idx);
  }

//   fprintf(stderr,"bsp split: %.3f %.3f %.3f, %.3f %.3f %.3f "
//        "==> (%d %d) %d %.3f\n",
//        box.min[0], box.min[1], box.min[2],
//        box.max[0], box.max[1], box.max[2],
//        this->left->indices.getLength(), this->right->indices.getLength(),
//        this->dimension, this->position);

//   for (i = 0; i < n; i++) {
//     SbVec3f p;
//     this->pointsArray->getElem(this->indices[i], p);
//     fprintf(stderr, "pt %d: %.3f %.3f %.3f\n", i, p[0], p[1], p[2]);
//   }


#if COIN_DEBUG
  if (!this->left->indices.getLength() ||
      !this->right->indices.getLength()) {
    fprintf(stderr,"Left:\n");
    n = this->indices.getLength();
    const SbVec3f * pts = this->pointsArray->getArrayPtr();
    for (i = 0; i < n; i++) {
      SbVec3f vec = pts[this->indices[i]];
      fprintf(stderr,"pt: %f %f %f\n",
              vec[0], vec[1], vec[2]);
    }
    fprintf(stderr,"pos: %f\n",
            pos);
    fprintf(stderr,"mid: %f\n",
            mid);
    fprintf(stderr,"dim: %d\n", dim);
    fprintf(stderr,"diag: %f %f %f\n",
            diag[0], diag[1], diag[2]);

#ifdef BSP_SORTED_SPLIT
    fprintf(stderr,"splitidx: %d\n", splitidx);
#endif
  }

#endif
  assert(this->left->indices.getLength() && this->right->indices.getLength());


  // will never be used anymore
  this->indices.truncate(0, TRUE);
}

//
// an implementation of the shellsort algorithm
//
void
coin_bspnode::sort()
{
  int num = this->indices.getLength();
  int dim = this->dimension;
  const SbVec3f * points = this->pointsArray->getArrayPtr();
  int i, j, distance;
  int idx;
  for (distance = 1; distance <= num/9; distance = 3*distance + 1) ;
  for (; distance > 0; distance /= 3) {
    for (i = distance; i < num; i++) {
      idx = this->indices[i];
      j = i;
      while (j >= distance &&
             points[this->indices[j-distance]][dim] > points[idx][dim]) {
        this->indices[j] = this->indices[j-distance];
        j -= distance;
      }
      this->indices[j] = idx;
    }
  }
}

/*!
  Constructor with \a maxnodepts specifying the maximum number of
  points in a node before it must be split, and \a initsize
  is the number of initially allocated points in the growable
  points array. If you know approximately the number of points
  which will be added to the tree, it will help the performance
  if you supply this in \a initsize.
 */
SbBSPTree::SbBSPTree(const int maxnodepts, const int initsize)
  : pointsArray(initsize),
    userdataArray(initsize)
{
  this->topnode = new coin_bspnode(&this->pointsArray);
  this->maxnodepoints = maxnodepts;
}

/*!
  Destructor. Frees used memory.
*/
SbBSPTree::~SbBSPTree()
{
  delete this->topnode;
}

/*!
  Returns the number of points in the BSP tree.
*/
int
SbBSPTree::numPoints() const
{
  return this->pointsArray.getLength();
}

/*!
  Returns the point at index \a idx.
  \sa SbBSPTree::numPoints()
*/
SbVec3f
SbBSPTree::getPoint(const int idx) const
{
  assert(idx < this->pointsArray.getLength());
  return this->pointsArray[idx];
}

/*!
  \overload
*/
void
SbBSPTree::getPoint(const int idx, SbVec3f &pt) const
{
  assert(idx < this->pointsArray.getLength());
  pt = this->pointsArray[idx];
}

/*!
  Returns the user data for the point at index \a idx.
  \sa SbBSPTree::addPoint()
  \sa SbBSPTree::numPoints()
*/
void *
SbBSPTree::getUserData(const int idx) const
{
  assert(idx < this->userdataArray.getLength());
  return this->userdataArray[idx];
}

/*!
  Sets the user data for the point at index \a idx to \a data.
  \sa SbBSPTree::addPoint()
  \sa SbBSPTree::numPoints()
*/
void
SbBSPTree::setUserData(const int idx, void * const data)
{
  assert(idx < this->userdataArray.getLength());
  this->userdataArray[idx] = data;
}

/*!
  Adds a new point \a pt to the BSP tree, and returns the index to
  the new point. The user data for that point will be set to \a data.

  If the point already exists in the BSP tree, the index to the
  old point will be returned. The user data for that point will
  not be changed.

  \sa SbBSPTree::findPoint()
*/
int
SbBSPTree::addPoint(const SbVec3f &pt, void * const data)
{
  this->boundingBox.extendBy(pt);
  int ret = this->topnode->addPoint(pt, this->maxnodepoints);
  if (ret == this->userdataArray.getLength()) {
    this->userdataArray.append(data);
  }
  return ret;
}

/*!
  Removes the point with coordinates \a pt, and returns the index
  to the removed point. -1 is returned if no point with those
  coordinates could be found.
*/
int
SbBSPTree::removePoint(const SbVec3f &pt)
{
  int idx = this->topnode->removePoint(pt);
  if (idx >= 0) {
    // SbList::removeFast() will move the last item onto the removed item
    // to avoid copying/moving all the data. We need to notify the node that
    // has that last point that the index has changed.
    int lastidx = this->pointsArray.getLength() - 1;
    if (lastidx != idx) {
      // update index
      this->topnode->updateIndex(this->pointsArray[lastidx], lastidx, idx);
    }
    // actually remove the point (copy lastidx onto idx, decrement size)
    this->pointsArray.removeFast(idx);
    this->userdataArray.removeFast(idx);
  }
  return idx;
}

/*!
  Removes the point at index \a idx.
  \sa SbBSPTree::numPoints()
*/
void
SbBSPTree::removePoint(const int idx)
{
  assert(idx < this->pointsArray.getLength());
  this->removePoint(this->pointsArray[idx]);
}

/*!
  Will search the tree, and return the index to the point
  with coordinates matching \a pos. If no such point can be
  found, -1 is returned.
*/
int
SbBSPTree::findPoint(const SbVec3f &pos) const
{
  return topnode->findPoint(pos);
}

/*!
  Will empty all points from the BSP tree.
*/
void
SbBSPTree::clear(const int COIN_UNUSED_ARG(initsize))
{
  delete this->topnode;
  this->topnode = NULL;
  this->pointsArray.truncate(0, TRUE);
  this->userdataArray.truncate(0, TRUE);
  this->topnode = new coin_bspnode(&this->pointsArray);
  this->boundingBox.makeEmpty();
}

/*!
  Will return the bounding box of all points in the BSP tree.
*/
const SbBox3f &
SbBSPTree::getBBox() const
{
  return this->boundingBox;
}

/*!
  \overload
*/
int
SbBSPTree::findClosest(const SbVec3f &pos) const
{
  int n = this->pointsArray.getLength();
  if (n < 32) { // points are very scattered when few are inserted
    SbVec3f tmp;
    int smallidx = -1;
    float smalldist = FLT_MAX;
    for (int i = 0; i < n; i++) {
      tmp = this->pointsArray[i];
      float dist = (tmp-pos).sqrLength();
      if (dist < smalldist) {
        smalldist = dist;
        smallidx = i;
      }
    }
    return smallidx;
  }
  SbVec3f center =
    (this->boundingBox.getMin() +
     this->boundingBox.getMax()) * 0.5f;
  center -= pos;

  float siz = center.length() * 2 +
    (this->boundingBox.getMax()-this->boundingBox.getMin()).length();

  float currsize = siz / 65536.0f;  // max 16 iterations (too much?).

  SbSphere sphere(pos, currsize);
  SbList <int> tmparray; // use only one array to avoid reallocs
  int idx = -1;

  // double size of sphere until a vertex is found
  while (currsize < siz) {
    sphere.setRadius(currsize);
    tmparray.truncate(0);
    idx = this->findClosest(sphere, tmparray);
    if (idx >= 0) return idx;
    currsize *= 2;
  }
  assert(0);
  return -1; // this should not happen!
}

/*!
  Returns a pointer to the array of points inserted into the BPS tree.
*/
const SbVec3f *
SbBSPTree::getPointsArrayPtr(void) const
{
  return this->pointsArray.getArrayPtr();
}

/*!
  Will return indices to all points inside \a sphere.

  \since Coin 2.3
*/
void
SbBSPTree::findPoints(const SbSphere & sphere, SbIntList & array) const
{
  this->topnode->findPoints(sphere, array);
}

/*!
  Will return the index to the point closest to the center of \a
  sphere. Indices to all points inside the sphere is returned in
  \a arr. If no points can be found inside the sphere, -1 is
  returned.

  \since Coin 2.3
*/
int
SbBSPTree::findClosest(const SbSphere & sphere, SbIntList & arr) const
{
  this->findPoints(sphere, arr);
  SbVec3f pos = sphere.getCenter();
  int n = arr.getLength();
  int closeidx = -1;
  float closedist = FLT_MAX;
  for (int i = 0; i < n; i++) {
    int idx = arr[i];
    float tmp = (pos-this->pointsArray[idx]).sqrLength();
    if (tmp < closedist) {
      closeidx = idx;
      closedist = tmp;
    }
  }
  return closeidx;
}


/*!
  WARNING: Please don't use this function. It can cause hard to find
  bugs on the Windows platform if your application is linked against a
  different CRT than your Coin DLL.

  Use int findClosest(const SbSphere &sphere, SbIntList & arr)
  instead.
*/
int
SbBSPTree::findClosest(const SbSphere &sphere,
                       SbList <int> &arr) const
{
  this->findPoints(sphere, arr);
  SbVec3f pos = sphere.getCenter();
  int n = arr.getLength();
  int closeidx = -1;
  float closedist = FLT_MAX;
  for (int i = 0; i < n; i++) {
    int idx = arr[i];
    float tmp = (pos-this->pointsArray[idx]).sqrLength();
    if (tmp < closedist) {
      closeidx = idx;
      closedist = tmp;
    }
  }
  return closeidx;
}

/*!
  WARNING: Please don't use this function. It can cause hard to find
  bugs on the Windows platform if your application is linked against a
  different CRT than your Coin DLL.

  Use void findPoints(const SbSphere &sphere, SbIntList & array)
  instead.
*/
void
SbBSPTree::findPoints(const SbSphere &sphere,
                      SbList <int> &array) const
{
  this->topnode->findPoints(sphere, array);
}

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SbBSPTree bsp;
  SbVec3f p0(0.0f, 0.0f, 0.0f);
  SbVec3f p1(1.0f, 0.0f, 0.0f);
  SbVec3f p2(2.0f, 0.0f, 0.0f);
  void * userdata0 = reinterpret_cast<void*> (&p0); 
  void * userdata1 = reinterpret_cast<void*> (&p1); 
  void * userdata2 = reinterpret_cast<void*> (&p2); 

  BOOST_CHECK_MESSAGE(bsp.addPoint(p0, userdata0) == 0, "unexpected index");
  BOOST_CHECK_MESSAGE(bsp.addPoint(p1, userdata1) == 1, "unexpected index");
  BOOST_CHECK_MESSAGE(bsp.addPoint(p2, userdata2) == 2, "unexpected index");
  BOOST_CHECK_MESSAGE(bsp.addPoint(p2, userdata2) == 2, "unexpected index");
  BOOST_CHECK_MESSAGE(bsp.numPoints() == 3, "wrong number of points in the tree");

  BOOST_CHECK_MESSAGE(bsp.findPoint(p0) == 0, "wrong index");
  BOOST_CHECK_MESSAGE(bsp.getUserData(0) == userdata0, "wrong userdata");
  BOOST_CHECK_MESSAGE(bsp.findPoint(p1) == 1, "wrong index");
  BOOST_CHECK_MESSAGE(bsp.getUserData(1) == userdata1, "wrong userdata");
  BOOST_CHECK_MESSAGE(bsp.findPoint(p2) == 2, "wrong index");
  BOOST_CHECK_MESSAGE(bsp.getUserData(2) == userdata2, "wrong userdata");

  BOOST_CHECK_MESSAGE(bsp.numPoints() == 3, "wrong number of points in the tree");
  BOOST_CHECK_MESSAGE(bsp.getPointsArrayPtr()[0] == p0, "wrong point at index 0");
  BOOST_CHECK_MESSAGE(bsp.getPointsArrayPtr()[1] == p1, "wrong point at index 1");
  BOOST_CHECK_MESSAGE(bsp.getPointsArrayPtr()[2] == p2, "wrong point at index 2");

  BOOST_CHECK_MESSAGE(bsp.removePoint(p1) == 1, "unable to remove point");
  BOOST_CHECK_MESSAGE(bsp.numPoints() == 2, "wrong number of points after removePoint().");
  BOOST_CHECK_MESSAGE(bsp.getPointsArrayPtr()[0] == p0, "wrong point at index 0");
  BOOST_CHECK_MESSAGE(bsp.getUserData(0) == userdata0, "wrong userdata");
  BOOST_CHECK_MESSAGE(bsp.getPointsArrayPtr()[1] == p2, "wrong point at index 1");
  BOOST_CHECK_MESSAGE(bsp.getUserData(1) == userdata2, "wrong userdata");

  BOOST_CHECK_MESSAGE(bsp.removePoint(p0) >= 0, "unable to remove point");
  BOOST_CHECK_MESSAGE(bsp.removePoint(p2) >= 0, "unable to remove point");
  BOOST_CHECK_MESSAGE(bsp.numPoints() == 0, "wrong number of points after removing all points.");

}

#endif // COIN_TEST_SUITE
