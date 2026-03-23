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
  \class SbBox3d SbBox3d.h Inventor/SbBox3d.h
  \brief The SbBox3d class is an abstraction for an axis aligned 3 dimensional box.

  \ingroup coin_base

  This box abstraction class is used by other entities in the Coin
  library for data exchange and storage. It provides a representation
  of the defining corners of a box in 3D space, with the sides aligned
  with the 3 principal axes.

  \sa SbBox2s, SbBox2f, SbBox2d, SbBox3s, SbBox3f, SbXfBox3f.
*/

// *************************************************************************

#include <Inventor/SbBox3d.h>

#include <limits>

#include <Inventor/SbBox3f.h>
#include <Inventor/SbBox3s.h>
#include <Inventor/SbBox3i32.h>
#include <Inventor/SbDPMatrix.h>
#include <Inventor/errors/SoDebugError.h>

// *************************************************************************

/*!
  \fn SbBox3d::SbBox3d(void)

  The default constructor makes an empty box.
*/

/*!
  \fn SbBox3d::SbBox3d(double minx, double miny, double minz, double maxx, double maxy, double maxz)
  Constructs a box with the given corners.

  \a minx should be less than \a maxx, \a miny should be less than
  \a maxy and \a minz should be less than \a maxz if you want to make
  a valid box.
*/

/*!
  \fn SbBox3d::SbBox3d(const SbVec3d & minval, const SbVec3d & maxval)

  Constructs a box with the given corners.

  The coordinates of \a min should be less than the coordinates of \a
  max if you want to make a valid box.
*/

/*!
  \fn SbBox3d::SbBox3d(const SbBox3f & box)

  Constructs an SbBox3d instance from the value in an SbBox3f instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox3d::SbBox3d(const SbBox3s & box)

  Constructs an SbBox3d instance from the value in an SbBox3s instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox3d::SbBox3d(const SbBox3i32 & box)

  Constructs an SbBox3d instance from the value in an SbBox3i32 instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox3d & SbBox3d::setBounds(double xmin, double ymin, double zmin, double xmax, double ymax, double zmax)

  Reset the boundaries of the box.

  \a minx should be less than \a maxx, \a miny should be less than
  \a maxy and \a minz should be less than \a maxz if you want to make
  a valid box.

  Returns reference to self.

  \sa getBounds().
*/

/*!
  \fn SbBox3d & SbBox3d::setBounds(const SbVec3d & minval, const SbVec3d & maxval)

  Reset the boundaries of the box with the given corners.

  The coordinates of \a min should be less than the coordinates of
  \a max if you want to make a valid box.

  Returns reference to self.

  \sa getBounds().
*/

/*!
  Reset the boundaries to the boundaries of the given \a box.

  Returns reference to self.

  \sa setBounds()
*/

SbBox3d &
SbBox3d::setBounds(const SbBox3f & box)
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

  \sa setBounds()
*/

SbBox3d &
SbBox3d::setBounds(const SbBox3s & box)
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

  \sa setBounds()
*/

SbBox3d &
SbBox3d::setBounds(const SbBox3i32 & box)
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
  \fn const SbVec3d & SbBox3d::getMin(void) const

  Returns the minimum point. This should usually be the lower left
  corner point of the box.

  \sa getOrigin(), getMax().
*/

/*!
  \fn const SbVec3d & SbBox3d::getMax(void) const

  Returns the maximum point. This should usually be the upper right
  corner point of the box.

  \sa getMin().
*/

/*!
  \fn SbVec3d & SbBox3d::getMin(void)

  Returns a modifiable reference the minimum point.
*/

/*!
  \fn SbVec3d & SbBox3d::getMax(void)

  Returns a modifiable reference the maximum point.
*/

/*!
  \fn SbVec3d SbBox3d::getCenter(void) const
  Returns the center point of the box.
*/

/*!
  Extend the boundaries of the box by the given point, i.e. make the
  point fit inside the box if it isn't already so.
*/
void
SbBox3d::extendBy(const SbVec3d & point)
{
  if (this->isEmpty()) {
    this->setBounds(point, point);
  }
  else {
    // The explicit casts are done to humour the HPUX aCC compiler,
    // which will otherwise say ``Template deduction failed to find a
    // match for the call to 'SbMin'''. mortene.
  this->minpt.setValue(SbMin(static_cast<double>(point[0]), static_cast<double>(this->minpt[0])),
                       SbMin(static_cast<double>(point[1]), static_cast<double>(this->minpt[1])),
                       SbMin(static_cast<double>(point[2]), static_cast<double>(this->minpt[2])));
  this->maxpt.setValue(SbMax(static_cast<double>(point[0]), static_cast<double>(this->maxpt[0])),
                       SbMax(static_cast<double>(point[1]), static_cast<double>(this->maxpt[1])),
                       SbMax(static_cast<double>(point[2]), static_cast<double>(this->maxpt[2])));
  }
}

/*!
  Extend the boundaries of the box by the given \a box parameter. This
  is equal to calling extendBy() twice with the corner points.
 */
void
SbBox3d::extendBy(const SbBox3d & box)
{
  if (box.isEmpty()) { return; }

  if (this->isEmpty()) {
    *this = box;
  }
  else {
    this->extendBy(box.minpt);
    this->extendBy(box.maxpt);
  }
}

/*!
  Check if the given point lies within the boundaries of this box.
 */
SbBool
SbBox3d::intersect(const SbVec3d & point) const
{
  return !(point[0] < this->minpt[0] ||
           point[0] > this->maxpt[0] ||
           point[1] < this->minpt[1] ||
           point[1] > this->maxpt[1] ||
           point[2] < this->minpt[2] ||
           point[2] > this->maxpt[2]);
}

/*!
  Check if the given \a box lies entirely or partially within the boundaries
  of this box.
 */
SbBool
SbBox3d::intersect(const SbBox3d & box) const
{
  if ((box.maxpt[0] < this->minpt[0]) ||
     (box.maxpt[1] < this->minpt[1]) ||
     (box.maxpt[2] < this->minpt[2]) ||
     (box.minpt[0] > this->maxpt[0]) ||
     (box.minpt[1] > this->maxpt[1]) ||
     (box.minpt[2] > this->maxpt[2])) return FALSE;
  return TRUE;
}

/*!
  \fn void SbBox3d::getBounds(double & minx, double & miny, double & minz, double & maxx, double & maxy, double & maxz) const

  Returns the box boundaries.

  \sa setBounds().
*/

/*!
  \fn void SbBox3d::getBounds(SbVec3d & minobj, SbVec3d & maxobj) const

  Returns the box corner points.

  \sa setBounds().
*/

/*!
  \fn void SbBox3d::getOrigin(double & originX, double & originY, double & originZ) const

  Returns the coordinates of the box origin (i.e. the lower left corner).

  \sa getMin().
*/

/*!
  \fn void SbBox3d::getSize(double & sizeX, double & sizeY, double & sizeZ) const

  Returns width, height and depth of box.
*/

/*!
  \fn SbVec3d SbBox3d::getSize(void) const

  Returns width, height and depth of box as a 3D vector.

  \since Coin 3.0
*/

/*!
  Marks this as an empty box.

  \sa isEmpty().
*/
void
SbBox3d::makeEmpty(void)
{
  this->minpt.setValue(std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
  this->maxpt.setValue(-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
}

/*!
  \fn SbBool SbBox3d::isEmpty(void) const
  Check if this has been marked as an empty box.

  \sa makeEmpty().
*/

/*!
  \fn SbBool SbBox3d::hasVolume(void) const

  Check if the box has been correctly specified and by that virtue
  has "positive" volume, i.e. all coordinates of its upper right corner
  (the maximum point) are greater than the corresponding coordinates 
  of its lower left corner (the minimum point).
*/

/*!
  \fn double SbBox3d::getVolume(void) const

  Returns the volume of the box.
*/

/*!
  Find the span of the box in the given direction (i.e. how much room in
  the given direction the box needs). The distance is returned as the minimum
  and maximum distance from origo to the closest and furthest plane defined
  by the direction vector and each of the box' corners. The difference
  between these values gives the span.
*/
void
SbBox3d::getSpan(const SbVec3d & dir, double & dmin, double & dmax) const
{
  double dist, mindist = std::numeric_limits<double>::max(), maxdist = -std::numeric_limits<double>::max();
  SbVec3d points[2]={this->minpt, this->maxpt};
  SbVec3d corner;
  SbVec3d normdir(dir);
  if (normdir.normalize() == 0.0) {
#if COIN_DEBUG
    SoDebugError::postWarning("SbBox3d::getSpan",
                              "The direction is a null vector.");
#endif // COIN_DEBUG

    dmin = dmax = 0.0;
    return;
  }

  for (int i=0;i<8;i++) {
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

/*!
  Transform the box by the matrix, and change its boundaries to contain
  the transformed box.

  Doesn't touch illegal/empty boxes.
 */
void
SbBox3d::transform(const SbDPMatrix & matrix)
{
#if COIN_DEBUG
  if (this->isEmpty()) {
    SoDebugError::postWarning("SbBox3d::transform",
                              "The box is not valid!");
    return;
  }
#endif // COIN_DEBUG

  SbVec3d dst;
  SbVec3d points[2]={this->minpt, this->maxpt};
  SbVec3d corner;
  SbBox3d newbox;

  //transform all the corners and include them into the new box.
  for (int i=0;i<8;i++) {
    //Find all corners the "binary" way :-)
    corner.setValue(points[(i&4)>>2][0], points[(i&2)>>1][1], points[i&1][2]);
    matrix.multVecMatrix(corner, dst);
    newbox.extendBy(dst);
  }
  this->setBounds(newbox.minpt, newbox.maxpt);
}

/*!
  Dump the state of this object to the \a fp file stream. Only works in
  debug version of library, method does nothing in an optimized build.
 */
void
SbBox3d::print(FILE * fp) const
{
#if COIN_DEBUG
  SbVec3d minv, maxv;
  this->getBounds(minv, maxv);
  minv.print(fp);
  fprintf(fp, " ");
  maxv.print(fp);
#endif // COIN_DEBUG
}

/*!
  \fn int operator == (const SbBox3d & b1, const SbBox3d & b2)
  \relates SbBox3d

  Check \a b1 and \a b2 for equality.
*/

/*!
  \fn int operator != (const SbBox3d & b1, const SbBox3d & b2)
  \relates SbBox3d

  Check \a b1 and \a b2 for inequality.
*/

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
SbBox3d::outside(const SbDPMatrix & mvp, int & cullbits) const
{
  // FIXME: this function is untested (code written by
  // pederb). 20000615 mortene.

  int i;
  SbVec3d tmp;
  SbVec3d clip[8];
  for (i = 0; i < 8; i++) {
    tmp[0] = i & 4 ? this->minpt[0] : this->maxpt[0];
    tmp[1] = i & 2 ? this->minpt[1] : this->maxpt[1];
    tmp[2] = i & 1 ? this->minpt[2] : this->maxpt[2];
    mvp.multVecMatrix(tmp, clip[i]);
  }
  for (int j = 0; j < 3; j++) {
    if (cullbits & (1<<j)) {
      int inside = 0;
      int outsideneg = 0;
      int outsidepos = 0;
      for (i = 0; i < 8; i++) {
        double val = clip[i][j];
        if (val < -1.0) outsideneg++;
        else if (val > 1.0) outsidepos++;
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
SbVec3d
SbBox3d::getClosestPoint(const SbVec3d & point) const
{
    if (isEmpty()) return point;

    double halfwidth = (this->maxpt[0] - this->minpt[0]) / 2.0;
    double halfheight = (this->maxpt[1] - this->minpt[1]) / 2.0;
    double halfdepth = (this->maxpt[2] - this->minpt[2]) / 2.0;

    SbVec3d center = this->getCenter();
    if (point == center)
        return SbVec3d(halfwidth, halfheight, this->maxpt[2]);

    SbVec3d vec = point - center;

    SbVec3d absvec;
    absvec[0] = halfwidth > 0.0 ? fabs(vec[0] / halfwidth) : fabs(vec[0]);
    absvec[1] = halfheight > 0.0 ? fabs(vec[1] / halfheight) : fabs(vec[1]);
    absvec[2] = halfdepth > 0.0 ? fabs(vec[2] / halfdepth) : fabs(vec[2]);

    SbVec3d closest;

    // Clamp to be on box hull.
    closest[0] = SbMin(absvec[0], 1.0);
    closest[1] = SbMin(absvec[1], 1.0);
    closest[2] = SbMin(absvec[2], 1.0);

    // Move point to be on the nearest plane of the unit box ((-1 -1 -1), (1 1 1)).
    if ((absvec[0] > absvec[1]) && (absvec[0] > absvec[2])) // yz-plane
        closest[0] = 1.0;
    else if ((absvec[1] > absvec[0]) && (absvec[1] > absvec[2])) // xz-plane
        closest[1] = 1.0;
    else if ((absvec[2] > absvec[0]) && (absvec[2] > absvec[1])) // xy-plane
        closest[2] = 1.0;
    else if ((absvec[0] == absvec[1]) && (absvec[0] == absvec[2])) // corner
        closest = SbVec3d(1.0, 1.0, 1.0);
    else if (absvec[0] == absvec[1]) { // edge parallel to z-axis
        closest[0] = 1.0;
        closest[1] = 1.0;
    }
    else if (absvec[0] == absvec[2]) { // edge parallel to y-axis
        closest[0] = 1.0;
        closest[2] = 1.0;
    }
    else if (absvec[1] == absvec[2]) { // edge parallel to x-axis
        closest[1] = 1.0;
        closest[2] = 1.0;
    }

    closest[0] *= (vec[0] < 0.0) ? -halfwidth : halfwidth;
    closest[1] *= (vec[1] < 0.0) ? -halfheight : halfheight;
    closest[2] *= (vec[2] < 0.0) ? -halfdepth : halfdepth;

    closest += center;

    return closest;
}

#ifdef COIN_TEST_SUITE
BOOST_AUTO_TEST_CASE(checkGetClosestPoint) {
    SbVec3d point(1524, 13794, 851);
    SbVec3d min(1557, 3308, 850);
    SbVec3d max(3113, 30157, 1886);

    SbBox3d box(min, max);
    SbVec3d expected(1557, 13794, 851);

    BOOST_CHECK_MESSAGE(box.getClosestPoint(point) == expected,
        "Closest point does not fit");

    SbVec3d sizes = box.getSize();
    SbVec3d expectedCenterQuery(sizes[0] / 2.0, sizes[1] / 2.0, max[2]);

    BOOST_CHECK_MESSAGE(box.getClosestPoint(box.getCenter()) == expectedCenterQuery,
        "Closest point for center query does not fit");
}
#endif //COIN_TEST_SUITE
