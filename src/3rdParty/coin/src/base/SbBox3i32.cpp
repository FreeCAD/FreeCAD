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
  \class SbBox3i32 SbBox3i32.h Inventor/SbBox3i32.h
  \brief The SbBox3i32 class is a 3 dimensional box with 32-bit
  integer coordinates.

  \ingroup coin_base

  This box class is used by other classes in Coin for data
  exchange. It provides storage for two box corners with 32-bit integer
  coordinates, which is among other things useful for representing
  screen or canvas areas in absolute window coordinates.

  \sa SbBox2f, SbBox2d, SbBox3s, SbBox3f, SbBox3d, SbXfBox3f.
*/

#include <Inventor/SbBox3i32.h>

#include <limits>

#include <Inventor/SbBox3s.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SbBox3d.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/errors/SoDebugError.h>

// *************************************************************************

/*!
  \fn SbBox3i32::SbBox3i32(void)
  The default constructor makes an empty box.
*/

/*!
  \fn SbBox3i32::SbBox3i32(int32_t xmin, int32_t ymin, int32_t zmin, int32_t xmax, int32_t ymax, int32_t zmax)

  Constructs a box with the given corners.

  \a xmin should be less than \a xmax, \a ymin should be less than
  \a ymax and \a zmin should be less than \a zmax if you want to make
  a valid box.
*/

/*!
  \fn SbBox3i32::SbBox3i32(const SbVec3i32 & minpoint, const SbVec3i32 & maxpoint)

  Constructs a box with the given corners.

  The coordinates of \a minpoint should be less than the coordinates of
  \a maxpoint if you want to make a valid box.
*/

/*!
  \fn SbBox3i32::SbBox3i32(const SbBox3s & box)

  Constructs an SbBox3i32 instance from the value in an SbBox3s instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox3i32::SbBox3i32(const SbBox3f & box)

  Constructs an SbBox3i32 instance from the value in an SbBox3f instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox3i32::SbBox3i32(const SbBox3d & box)

  Constructs an SbBox3i32 instance from the value in an SbBox3d instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox3i32 & SbBox3i32::setBounds(int32_t xmin, int32_t ymin, int32_t zmin, int32_t xmax, int32_t ymax, int32_t zmax)

  Reset the boundaries of the box.

  \a xmin should be less than \a xmax, \a ymin should be less than
  \a ymax and \a zmin should be less than \a zmax if you want to make
  a valid box.

  Returns reference to self.

  \sa getBounds().
*/

/*!
  \fn SbBox3i32 & SbBox3i32::setBounds(const SbVec3i32 & minpoint, const SbVec3i32 & maxpoint)

  Reset the boundaries of the box with the given corners.

  The coordinates of \a minpoint should be less than the coordinates of
  \a maxpoint if you want to make a valid box.

  Returns reference to self.

  \sa getBounds().
*/

/*!
  Reset the boundaries to the boundaries of the given \a box.

  Returns reference to self.

  \sa getBounds()
*/

SbBox3i32 &
SbBox3i32::setBounds(const SbBox3s & box)
{
  if (box.isEmpty()) {
    makeEmpty();
  } else {
    minpt.setValue(box.getMin());
    maxpt.setValue(box.getMax());
  }
  return *this;
}

/*!
  Reset the boundaries to the boundaries of the given \a box.

  Returns reference to self.

  \sa getBounds()
*/

SbBox3i32 &
SbBox3i32::setBounds(const SbBox3f & box)
{
  if (box.isEmpty()) {
    makeEmpty();
  } else {
    minpt.setValue(box.getMin());
    maxpt.setValue(box.getMax());
  }
  return *this;
}

/*!
  Reset the boundaries to the boundaries of the given \a box.

  Returns reference to self.

  \sa getBounds()
*/

SbBox3i32 &
SbBox3i32::setBounds(const SbBox3d & box)
{
  if (box.isEmpty()) {
    makeEmpty();
  } else {
    minpt.setValue(box.getMin());
    maxpt.setValue(box.getMax());
  }
  return *this;
}

/*!
  \fn const SbVec3i32 & SbBox3i32::getMin(void) const

  Returns the minimum point. This should usually be the lower left corner
  point of the box.

  \sa getOrigin(), getMax().
*/

/*!
  \fn const SbVec3i32 & SbBox3i32::getMax(void) const

  Returns the maximum point. This should usually be the upper right corner
  point of the box.

  \sa getMin().
*/

/*!
  \fn SbVec3i32 & SbBox3i32::getMin(void)

  Returns a modifiable reference to the minimum point.
*/

/*!
  \fn SbVec3i32 & SbBox3i32::getMax(void)

  Returns a modifiable reference to the maximum point.
*/

/*!
  \fn SbVec3i32 SbBox3i32::getCenter(void) const

  Returns the center point of the box.
*/

/*!
  Extend the boundaries of the box by the given point, i.e. make the
  point fit inside the box if it isn't already within it.
 */
void
SbBox3i32::extendBy(const SbVec3i32 & point)
{
  if (isEmpty()) {
    minpt = maxpt = point;
  } else {
    this->minpt.setValue(SbMin(static_cast<int32_t>(point[0]), static_cast<int32_t>(this->minpt[0])),
                      SbMin(static_cast<int32_t>(point[1]), static_cast<int32_t>(this->minpt[1])),
                      SbMin(static_cast<int32_t>(point[2]), static_cast<int32_t>(this->minpt[2])));
    this->maxpt.setValue(SbMax(static_cast<int32_t>(point[0]), static_cast<int32_t>(this->maxpt[0])),
                      SbMax(static_cast<int32_t>(point[1]), static_cast<int32_t>(this->maxpt[1])),
                      SbMax(static_cast<int32_t>(point[2]), static_cast<int32_t>(this->maxpt[2])));
  }
}

/*!
  Extend the boundaries of the box by the given \a box parameter. This
  is equal to calling extendBy() twice with the corner points.
 */
void
SbBox3i32::extendBy(const SbBox3i32 & box)
{
  if (!box.isEmpty()) {
    extendBy(box.getMin());
    extendBy(box.getMax());
  }
}

/*!
  Extend the boundaries of the box by the given point, i.e. make the
  point fit inside the box if it isn't already within it.
 */
void
SbBox3i32::extendBy(const SbVec3f & point)
{
  SbVec3i32 ptmax(point), ptmin(point);
  if (float(ptmax[0]) < point[0]) ++ptmax[0];
  if (float(ptmax[1]) < point[1]) ++ptmax[1];
  if (float(ptmax[2]) < point[2]) ++ptmax[2];
  extendBy(ptmin);
  extendBy(ptmax);
}

/*!
  Transform the box by the matrix, and change its boundaries to contain
  the transformed box.

  Doesn't touch illegal/empty boxes.
*/
void
SbBox3i32::transform(const SbMatrix & matrix)
{
#if COIN_DEBUG
  if (this->isEmpty()) {
    SoDebugError::postWarning("SbBox3i32::transform",
                              "The box is not valid!");
    return;
  }
#endif // COIN_DEBUG

  SbVec3f dst;
  SbVec3f points[2] = {SbVec3f(this->minpt), SbVec3f(this->maxpt)};
  SbVec3f corner;
  SbBox3i32 newbox;

  //transform all the corners and include them into the new box.
  for (int i = 0; i < 8; ++i) {
    //Find all corners the "binary" way :-)
    corner.setValue(points[(i&4)>>2][0], points[(i&2)>>1][1], points[i&1][2]);
    matrix.multVecMatrix(corner, dst);
    newbox.extendBy(dst);
  }
  this->setBounds(newbox.minpt, newbox.maxpt);
}

/*!
  \fn void SbBox3i32::getBounds(int32_t & xmin, int32_t & ymin, int32_t & zmin, int32_t & xmax, int32_t & ymax, int32_t & zmax) const

  Returns the box boundaries.

  \sa setBounds().
*/

/*!
  \fn void SbBox3i32::getBounds(SbVec3i32 & minpoint, SbVec3i32 & maxpoint) const

  Returns the box corner points.

  \sa setBounds().
*/

/*!
  \fn void SbBox3i32::getOrigin(int32_t & originX, int32_t & originY, int32_t & originZ) const

  Returns the coordinates of the box origin (i.e. the lower left corner).

  \sa getMin().
*/

/*!
  \fn void SbBox3i32::getSize(int32_t & sizeX, int32_t & sizeY, int32_t & sizeZ) const

  Returns width, height and depth of box.
*/

/*!
  \fn SbVec3i32 SbBox3i32::getSize(void) const

  Returns width, height and depth of box as a 3D vector.

  \since Coin 3.0
*/

/*!
  Marks this as an empty box.

  \sa isEmpty().
*/
void
SbBox3i32::makeEmpty(void)
{
  minpt.setValue(std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max());
  maxpt.setValue(-std::numeric_limits<int32_t>::max(), -std::numeric_limits<int32_t>::max(), -std::numeric_limits<int32_t>::max());
}

/*!
  Check if the given point lies within the boundaries of this box.
 */
SbBool
SbBox3i32::intersect(const SbVec3i32 & point) const
{
  return !(point[0] < minpt[0] || point[0] > maxpt[0] ||
           point[1] < minpt[1] || point[1] > maxpt[1] ||
           point[2] < minpt[2] || point[2] > maxpt[2]);
}

/*!
  Check if \a box lies entirely or partially within the boundaries
  of this box.
 */
SbBool
SbBox3i32::intersect(const SbBox3i32 & box) const
{
  return !((box.maxpt[0] < minpt[0]) || (box.minpt[0] > maxpt[0]) ||
           (box.maxpt[1] < minpt[1]) || (box.minpt[1] > maxpt[1]) ||
           (box.maxpt[2] < minpt[2]) || (box.minpt[2] > maxpt[2]));
}

/*!
  Check if the given point lies within the boundaries of this box.
 */
SbBool
SbBox3i32::intersect(const SbVec3f & point) const
{
  SbVec3i32 ptmax(point), ptmin(point);
  if (float(ptmax[0]) < point[0]) ++ptmax[0];
  if (float(ptmax[1]) < point[1]) ++ptmax[1];
  if (float(ptmax[2]) < point[2]) ++ptmax[2];
  return !((ptmin[0] < minpt[0]) || (ptmax[0] > maxpt[0]) ||
           (ptmin[1] < minpt[1]) || (ptmax[1] > maxpt[1]) ||
           (ptmin[2] < minpt[2]) || (ptmax[2] > maxpt[2]));
}

/*!
  Check if the box is outside the view volume defined by the \a mvp
  matrix. Sets \a cullbits according to which planes we're inside or
  outside. Bit 0 (0x1) is cleared when box is completely inside left
  and right clipping planes. Bit 1 (0x2) is cleared when box is inside
  top and bottom clipping planes. Bit 2 (0x4) is cleared when box is
  inside near and far clipping planes.

  Returns \c TRUE if box is completely outside one of the clipping
  planes. \c FALSE otherwise.
*/
SbBool
SbBox3i32::outside(const SbMatrix & mvp, int & cullbits) const
{
  // FIXME: this function is untested (code written by
  // pederb). 20000615 mortene.

  int i;
  SbVec3f tmp;
  SbVec3f clip[8];
  for (i = 0; i < 8; ++i) {
    tmp[0] = i & 4 ? float(minpt[0]) : float(maxpt[0]);
    tmp[1] = i & 2 ? float(minpt[1]) : float(maxpt[1]);
    tmp[2] = i & 1 ? float(minpt[2]) : float(maxpt[2]);
    mvp.multVecMatrix(tmp, clip[i]);
  }
  for (int j = 0; j < 3; j++) {
    if (cullbits & (1<<j)) {
      int inside = 0;
      int outsideneg = 0;
      int outsidepos = 0;
      for (i = 0; i < 8; i++) {
        float val = clip[i][j];
        if (val < -1.0f) outsideneg++;
        else if (val > 1.0f) outsidepos++;
        else inside++;
      }
      if (outsidepos == 8) return TRUE;
      if (outsideneg == 8) return TRUE;
      if (inside == 8) cullbits ^= (1<<j);
    }
  }
  return FALSE;
}

/*!
  Return the point on the box closest to the given \a point.
  If the given point equals the center, the center point of
  the positive Z face is returned.
*/
SbVec3f
SbBox3i32::getClosestPoint(const SbVec3f & point) const
{
  if (isEmpty()) return point;

  float halfwidth = float(this->maxpt[0] - this->minpt[0]) / 2.0f;
  float halfheight = float(this->maxpt[1] - this->minpt[1]) / 2.0f;
  float halfdepth = float(this->maxpt[2] - this->minpt[2]) / 2.0f;

  SbVec3f center = this->getCenter();
  if (point == center)
    return SbVec3f(halfwidth, halfheight, float(this->maxpt[2]));

  SbVec3f vec = point - center;

  SbVec3f absvec;
  absvec[0] = float(halfwidth > 0.0f ? fabs(vec[0] / halfwidth) : fabs(vec[0]));
  absvec[1] = float(halfheight > 0.0f ? fabs(vec[1] / halfheight) : fabs(vec[1]));
  absvec[2] = float(halfdepth > 0.0f ? fabs(vec[2] / halfdepth) : fabs(vec[2]));

  SbVec3f closest;

  // Clamp to be on box hull.
  closest[0] = SbMin(absvec[0], 1.0f);
  closest[1] = SbMin(absvec[1], 1.0f);
  closest[2] = SbMin(absvec[2], 1.0f);

  // Move point to be on the nearest plane of the unit box ((-1 -1 -1), (1 1 1)).
  if ((absvec[0] > absvec[1]) && (absvec[0] > absvec[2])) // yz-plane
    closest[0] = 1.0f;
  else if ((absvec[1] > absvec[0]) && (absvec[1] > absvec[2])) // xz-plane
    closest[1] = 1.0f;
  else if ((absvec[2] > absvec[0]) && (absvec[2] > absvec[1])) // xy-plane
    closest[2] = 1.0f;
  else if ((absvec[0] == absvec[1]) && (absvec[0] == absvec[2])) // corner
    closest = SbVec3f(1.0f, 1.0f, 1.0f);
  else if (absvec[0] == absvec[1]) { // edge parallel to z-axis
    closest[0] = 1.0f;
    closest[1] = 1.0f;
  }
  else if (absvec[0] == absvec[2]) { // edge parallel to y-axis
    closest[0] = 1.0f;
    closest[2] = 1.0f;
  }
  else if (absvec[1] == absvec[2]) { // edge parallel to x-axis
    closest[1] = 1.0f;
    closest[2] = 1.0f;
  }

  closest[0] *= (vec[0] < 0.0f) ? -halfwidth : halfwidth;
  closest[1] *= (vec[1] < 0.0f) ? -halfheight : halfheight;
  closest[2] *= (vec[2] < 0.0f) ? -halfdepth : halfdepth;

  closest += center;

  return closest;
}

/*!
  \fn SbBool SbBox3i32::isEmpty(void) const

  Check if this has been marked as an empty box.

  \sa makeEmpty().
*/

/*!
  \fn SbBool SbBox3i32::hasVolume(void) const

  Check if the box has been correctly specified and by that virtue
  has "positive" volume, i.e. all coordinates of its upper right corner
  (the maximum point) are greater than the corresponding coordinates 
  of its lower left corner (the minimum point).
*/

/*!
  \fn float SbBox3i32::getVolume(void) const

  Returns the volume of the box.
*/

/*!
  \fn int operator == (const SbBox3i32 & b1, const SbBox3i32 & b2)
  \relates SbBox3i32

  Check \a b1 and \a b2 for equality.
*/

/*!
  \fn int operator != (const SbBox3i32 & b1, const SbBox3i32 & b2)
  \relates SbBox3i32

  Check \a b1 and \a b2 for inequality.
*/

/*!
  Find the span of the box in the given direction (i.e. how much room in
  the given direction the box needs). The distance is returned as the minimum
  and maximum distance from origo to the closest and furthest plane defined
  by the direction vector and each of the box' corners. The difference
  between these values gives the span.
*/
void
SbBox3i32::getSpan(const SbVec3f & dir, float & dmin, float & dmax) const
{
  float dist, mindist = std::numeric_limits<float>::max(), maxdist = -std::numeric_limits<float>::max();
  SbVec3f points[2] = { SbVec3f(this->minpt), SbVec3f(this->maxpt) };
  SbVec3f corner;
  SbVec3f normdir(dir);
  if (normdir.normalize() == 0.0f) {
#if COIN_DEBUG
    SoDebugError::postWarning("SbBox3f::getSpan",
                              "The direction is a null vector.");
#endif // COIN_DEBUG

    dmin = dmax = 0.0f;
    return;
  }

  for (int i = 0; i < 8; ++i) {
    //Find all corners the "binary" way :-)
    corner.setValue(points[(i&4)>>2][0], points[(i&2)>>1][1], points[i&1][2]);

    // a = dir
    // b = corner-[0, 0, 0]
    // v = dir/|dir|
    //
    // The dot product formula (1) gives the formula we use (2).
    //
    // (1)  a * b = |a|*|b| * cos(angle(a, b))
    //
    // (2)  distance = v * b
    //
    // This dot product (2) gives the distance from origo to the plane
    // defined by normdir and corner.
    dist=normdir.dot(corner);
    if (dist < mindist) mindist = dist;
    if (dist > maxdist) maxdist = dist;
  }
  dmin = mindist;
  dmax = maxdist;
}


#ifdef COIN_TEST_SUITE
BOOST_AUTO_TEST_CASE(checkSize) {
  SbVec3i32 min(1,2,3);
  SbVec3i32 max(3,4,5);

  SbVec3i32 diff = max - min;

  SbBox3i32 box(min, max);

  BOOST_CHECK_MESSAGE(box.getSize() == diff,
                      "Box has incorrect size");
}
BOOST_AUTO_TEST_CASE(checkGetClosestPoint) {
  SbVec3f point(1524 , 13794 , 851);
  SbVec3i32 min(1557, 3308, 850);
  SbVec3i32 max(3113, 30157, 1886);

  SbBox3i32 box(min, max);
  SbVec3f expected(1557, 13794, 851);

  BOOST_CHECK_MESSAGE(box.getClosestPoint(point) == expected,
                      "Closest point does not fit");

  SbVec3i32 sizes = box.getSize();
  SbVec3f expectedCenterQuery(sizes[0]/2.0f, sizes[1]/2.0f, (float)max[2]);

  BOOST_CHECK_MESSAGE(box.getClosestPoint(box.getCenter()) == expectedCenterQuery,
                      "Closest point for center query does not fit");
}
#endif //COIN_TEST_SUITE
