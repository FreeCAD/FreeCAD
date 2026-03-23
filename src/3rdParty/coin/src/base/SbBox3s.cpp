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
  \class SbBox3s SbBox3s.h Inventor/SbBox3s.h
  \brief The SbBox3s class is a 3 dimensional box with short
  integer coordinates.

  \ingroup coin_base

  This box class is used by other classes in Coin for data
  exchange. It provides storage for two box corners with short integer
  coordinates.

  \sa SbBox2s, SbBox2f, SbBox2d, SbBox3f, SbBox3d, SbXfBox3f.
  \since Coin 2.0
  \since TGS Inventor ?.?
*/


#include <Inventor/SbBox3s.h>

#include <limits>
#include <cassert>

#include <Inventor/SbBox3i32.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SbBox3d.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  \fn SbBox3s::SbBox3s(void)
  The default constructor makes an empty box.
*/

/*!
  \fn SbBox3s::SbBox3s(short xmin, short ymin, short zmin, short xmax, short ymax, short zmax)

  Constructs a box with the given corner coordinates.

  \a xmin should be less than \a xmax, \a ymin should be less than \a
  ymax, and \a zmin should be less than \a zmax if you want to make a
  valid box.
*/

/*!
  \fn SbBox3s::SbBox3s(const SbVec3s & minvec, const SbVec3s & maxvec)

  Constructs a box with the given corners.

  The coordinates of \a min should be less than the coordinates of
  \a max if you want to make a valid box.
*/

/*!
  \fn SbBox3s::SbBox3s(const SbBox3i32 & box)

  Constructs an SbBox3s instance from the value in an SbBox3i32 instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox3s::SbBox3s(const SbBox3f & box)

  Constructs an SbBox3s instance from the value in an SbBox3f instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox3s::SbBox3s(const SbBox3d & box)

  Constructs an SbBox3s instance from the value in an SbBox3d instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox3s & SbBox3s::setBounds(short xmin, short ymin, short zmin, short xmax, short ymax, short zmax)

  Reset the boundaries of the box.

  \a xmin should be less than \a xmax, \a ymin should be less than \a
  ymax, and \a zmin should be less than \a xmax if you want to make a
  valid box.

  Returns reference to self.

  \sa getBounds().  
*/

/*!
  \fn SbBox3s & SbBox3s::setBounds(const SbVec3s & minvec, const SbVec3s & maxvec)

  Reset the boundaries of the box with the given corners.

  The coordinates of \a minvec should be less than the coordinates of
  \a maxvec if you want to make a valid box.

  Returns reference to self.

  \sa getBounds().
*/

/*!
  Reset the boundaries to the boundaries of the given \a box.

  Returns reference to self.

  \sa getBounds().
*/

SbBox3s &
SbBox3s::setBounds(const SbBox3i32 & box)
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

  \sa getBounds().
*/

SbBox3s &
SbBox3s::setBounds(const SbBox3f & box)
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

  \sa getBounds().
*/

SbBox3s &
SbBox3s::setBounds(const SbBox3d & box)
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
  Marks this as an empty box.

  \sa isEmpty().
*/
void
SbBox3s::makeEmpty(void)
{
  this->minpt.setValue(std::numeric_limits<short>::max(), std::numeric_limits<short>::max(), std::numeric_limits<short>::max());
  this->maxpt.setValue(-std::numeric_limits<short>::max(), -std::numeric_limits<short>::max(), -std::numeric_limits<short>::max());
}

/*!
  \fn SbBool SbBox3s::isEmpty(void) const

  Check if this has been marked as an empty box.

  \sa makeEmpty().
*/

/*!
  \fn const SbVec3s & SbBox3s::getMin(void) const

  Returns the minimum point. This should usually be the lower left corner
  point of the box.

  \sa getOrigin(), getMax().
*/

/*!
  \fn const SbVec3s & SbBox3s::getMax(void) const

  Returns the maximum point. This should usually be the upper right corner
  point of the box.

  \sa getMin().
*/

/*!
  \fn SbVec3s & SbBox3s::getMin(void)

  Returns a modifiable reference to the minimum point.
*/

/*!
  \fn SbVec3s & SbBox3s::getMax(void)

  Returns a modifiable reference to the maximum point.
*/

/*!
  \fn SbVec3s SbBox3s::getCenter(void) const

  Returns the center point of the box.
*/

/*!
  Extend the boundaries of the box by the given point, i.e. make the
  point fit inside the box if it isn't already within it.
 */
void
SbBox3s::extendBy(const SbVec3s & point)
{
  // The explicit casts are done to humour the HPUX aCC compiler,
  // which will otherwise say ``Template deduction failed to find a
  // match for the call to 'SbMin'''. mortene.
  this->minpt.setValue(SbMin(static_cast<short>(point[0]), static_cast<short>(this->minpt[0])),
                       SbMin(static_cast<short>(point[1]), static_cast<short>(this->minpt[1])),
                       SbMin(static_cast<short>(point[2]), static_cast<short>(this->minpt[2])));
  this->maxpt.setValue(SbMax(static_cast<short>(point[0]), static_cast<short>(this->maxpt[0])),
                       SbMax(static_cast<short>(point[1]), static_cast<short>(this->maxpt[1])),
                       SbMax(static_cast<short>(point[2]), static_cast<short>(this->maxpt[2])));
}

/*!
  Extend the boundaries of the box by the given \a box parameter. This
  is equal to calling extendBy() twice with the corner points.
 */
void
SbBox3s::extendBy(const SbBox3s & box)
{
  if (box.isEmpty()) { return; }

  this->extendBy(box.getMin());
  this->extendBy(box.getMax());
}

/*!
  Check if the given point lies within the boundaries of this box.
 */
SbBool
SbBox3s::intersect(const SbVec3s & point) const
{
  if((point[0] >= this->minpt[0]) && (point[0] <= this->maxpt[0]) &&
     (point[1] >= this->minpt[1]) && (point[1] <= this->maxpt[1]) &&
     (point[2] >= this->minpt[2]) && (point[2] <= this->maxpt[2])) return TRUE;
  return FALSE;
}

/*!
  Check if \a box lies entirely or partially within the boundaries
  of this box.
 */
SbBool
SbBox3s::intersect(const SbBox3s & box) const
{
  if((box.getMax()[0] < this->getMin()[0]) ||
     (box.getMax()[1] < this->getMin()[1]) ||
     (box.getMax()[2] < this->getMin()[2]) ||
     (box.getMin()[0] > this->getMax()[0]) ||
     (box.getMin()[1] > this->getMax()[1]) ||
     (box.getMin()[2] > this->getMax()[2])) return FALSE;
  return TRUE;
}

/*!
  Return the point on the box closest to the given \a point.
  If the given point equals the center, the center point of
  the positive Z face is returned.
*/
SbVec3f
SbBox3s::getClosestPoint(const SbVec3f & point) const
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
  \fn void SbBox3s::getBounds(short & xmin, short & ymin, short & zmin, short & xmax, short & ymax, short & zmax) const

  Returns the box boundary coordinates.

  \sa setBounds(), getMin(), getMax().
*/

/*!
  \fn void SbBox3s::getBounds(SbVec3s & minvec, SbVec3s & maxvec) const

  Returns the box corner points.

  \sa setBounds(), getMin(), getMax().
*/

/*!
  \fn void SbBox3s::getOrigin(short & originX, short & originY, short & originZ) const

  Returns the coordinates of the box origin (i.e. the lower left corner).

  \sa getMin().
*/

/*!
  \fn void SbBox3s::getSize(short & sizeX, short & sizeY, short & sizeZ) const

  Returns width and height of box.
*/

/*!
  \fn SbVec3s SbBox3s::getSize(void) const

  Returns width, height and depth of box as a 3D vector.

  \since Coin 3.0
*/

/*!
  \fn int operator == (const SbBox3s & b1, const SbBox3s & b2)
  \relates SbBox3s

  Check \a b1 and \a b2 for equality.
*/

/*!
  \fn int operator != (const SbBox3s & b1, const SbBox3s & b2)
  \relates SbBox3s

  Check \a b1 and \a b2 for inequality.
*/

/*!
  \fn SbBool SbBox3s::hasVolume(void) const

  Check if the box has been correctly specified and by that virtue
  has "positive" volume, i.e. all coordinates of its upper right corner
  (the maximum point) are greater than the corresponding coordinates 
  of its lower left corner (the minimum point).
*/

/*!
  \fn int SbBox3s::getVolume(void) const

  Returns the volume of the box.
*/

#ifdef COIN_TEST_SUITE
BOOST_AUTO_TEST_CASE(checkSize) {
  SbVec3s min(1,2,3);
  SbVec3s max(3,4,5);

  SbVec3s diff = max - min;

  SbBox3s box(min, max);

  BOOST_CHECK_MESSAGE(box.getSize() == diff,
                      "Box has incorrect size");
}
BOOST_AUTO_TEST_CASE(checkGetClosestPoint) {
  SbVec3f point(1524 , 13794 , 851);
  SbVec3s min(1557, 3308, 850);
  SbVec3s max(3113, 30157, 1886);

  SbBox3s box(min, max);
  SbVec3f expected(1557, 13794, 851);

  BOOST_CHECK_MESSAGE(box.getClosestPoint(point) == expected,
                      "Closest point does not fit");

  SbVec3s sizes = box.getSize();
  SbVec3f expectedCenterQuery(sizes[0]/2.0f, sizes[1]/2.0f, max[2]);

  BOOST_CHECK_MESSAGE(box.getClosestPoint(box.getCenter()) == expectedCenterQuery,
                      "Closest point for center query does not fit");
}
#endif //COIN_TEST_SUITE
