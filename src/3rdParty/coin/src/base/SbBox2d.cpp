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
  \class SbBox2d SbBox2d.h Inventor/SbBox2d.h
  \brief The SbBox2d class is a 2 dimensional box with double precision
  corner coordinates.

  \ingroup coin_base

  This box class is used by many other classes in Coin for data
  exchange and storage. It provides two box corners with double
  precision coordinates, which is among other things useful for
  representing screen or canvas dimensions in normalized coordinates.

  This class is a Coin extension.

  \sa SbBox2s, SbBox2f, SbBox3s, SbBox3f, SbBox3d, SbXfBox3f.

  \since Coin 2.0
  \since TGS Inventor 2.6
*/

#include <Inventor/SbBox2d.h>

#include <limits>

#include <Inventor/SbBox2f.h>
#include <Inventor/SbBox2s.h>
#include <Inventor/SbBox2i32.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  \fn SbBox2d::SbBox2d(void)

  The default constructor makes an empty box.
*/

/*!
  \fn SbBox2d::SbBox2d(double xmin, double ymin, double xmax, double ymax)

  Constructs a box with the given corners.

  \a xmin should be less than \a xmax and \a ymin should be less than
  \a ymax if you want to make a valid box.
*/

/*!
  \fn SbBox2d::SbBox2d(const SbVec2d & min, const SbVec2d & max)

  Constructs a box with the given lower left and upper right corners.

  The coordinates of \a min should be less than the coordinates of
  \a max if you want to make a valid box.
*/

/*!
  \fn SbBox2d::SbBox2d(const SbBox2f & box)

  Constructs an SbBox2d instance from the value in an SbBox2f instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox2d::SbBox2d(const SbBox2s & box)

  Constructs an SbBox2d instance from the value in an SbBox2s instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox2d::SbBox2d(const SbBox2i32 & box)

  Constructs an SbBox2d instance from the value in an SbBox2i32 instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox2d & SbBox2d::setBounds(double xmin, double ymin, double xmax, double ymax)

  Reset the boundaries of the box.

  \a xmin should be less than \a xmax and \a ymin should be less than
  \a ymax if you want to make a valid box.

  Returns reference to self.

  \sa getBounds().
*/

/*!
  \fn SbBox2d & SbBox2d::setBounds(const SbVec2d & min, const SbVec2d & max)

  Reset the boundaries of the box with the given corners.

  The coordinates of \a min should be less than the coordinates of
  \a max if you want to make a valid box.

  Returns reference to self.

  \sa getBounds().
*/

/*!
  Reset the boundaries of the box with the given \a box boundaries.

  Returns reference to self.

  \sa getBounds()
*/
SbBox2d &
SbBox2d::setBounds(const SbBox2f & box)
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
  Reset the boundaries of the box with the given \a box boundaries.

  Returns reference to self.

  \sa getBounds()
*/
SbBox2d &
SbBox2d::setBounds(const SbBox2s & box)
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
  Reset the boundaries of the box with the given \a box boundaries.

  Returns reference to self.

  \sa getBounds()
*/
SbBox2d &
SbBox2d::setBounds(const SbBox2i32 & box)
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
SbBox2d::makeEmpty(void)
{
  minpt.setValue(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
  maxpt.setValue(-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
}

/*!
  \fn SbBool SbBox2d::isEmpty(void) const

  Check if this has been marked as an empty box.

  \sa makeEmpty().
*/

/*!
  \fn SbBool SbBox2d::hasArea(void) const

  Check if the box has been correctly specified and by that virtue
  has "positive" area, i.e. all coordinates of its upper right corner
  (the maximum point) are greater than the corresponding coordinates 
  of its lower left corner (the minimum point).
*/

/*!
  \fn const SbVec2d & SbBox2d::getMin(void) const

  Returns the lower left corner of the box.

  \sa getOrigin(), getMax().
*/

/*!
  \fn SbVec2d & SbBox2d::getMin(void)

  Returns the lower left corner of the box.

  \sa getOrigin(), getMax().
*/

/*!
  \fn const SbVec2d & SbBox2d::getMax(void) const

  Returns the upper right corner of the box.

  \sa getMin().
*/

/*!
  \fn SbVec2d & SbBox2d::getMax(void)

  Returns the upper right corner of the box.

  \sa getMin().
*/

/*!
  \fn SbVec2d SbBox2d::getCenter(void) const

  Returns the center point of the box.
*/

/*!
  Extend the boundaries of the box by the given point, i.e. make the
  box fit around the \a point if it isn't already situated within it.
*/
void
SbBox2d::extendBy(const SbVec2d & point)
{
  // The explicit cast to double is done to humour the HPUX aCC
  // compiler, which will otherwise say ``Template deduction failed to
  // find a match for the call to 'SbMin'''. mortene.
  this->minpt.setValue(SbMin(static_cast<double>(point[0]), static_cast<double>(this->minpt[0])),
                       SbMin(static_cast<double>(point[1]), static_cast<double>(this->minpt[1])));
  this->maxpt.setValue(SbMax(static_cast<double>(point[0]), static_cast<double>(this->maxpt[0])),
                       SbMax(static_cast<double>(point[1]), static_cast<double>(this->maxpt[1])));
}

/*!
  Extend the boundaries of the box by the given \a box parameter. This
  is equal to calling the above method twice with the corner points.
*/
void
SbBox2d::extendBy(const SbBox2d & box)
{
  if (box.isEmpty()) { return; }

  this->extendBy(box.getMin());
  this->extendBy(box.getMax());
}

/*!
  Check if \a point lies within the boundaries of this box.
*/
SbBool
SbBox2d::intersect(const SbVec2d & point) const
{
  if ((point[0] >= this->minpt[0]) && (point[0] <= this->maxpt[0]) &&
      (point[1] >= this->minpt[1]) && (point[1] <= this->maxpt[1])) return TRUE;
  return FALSE;
}

/*!
  Check if \a box lies entirely or partially within the boundaries
  of this box.
*/
SbBool
SbBox2d::intersect(const SbBox2d & box) const
{
  if ((box.getMax()[0] < this->getMin()[0]) ||
      (box.getMax()[1] < this->getMin()[1]) ||
      (box.getMin()[0] > this->getMax()[0]) ||
      (box.getMin()[1] > this->getMax()[1])) return FALSE;
  return TRUE;
}

/*!
  Check if a a line from \a a to \a b intersects the box, and return the
  coordinates of the union line in \a ia and \a ib.

  This function is a Coin extension.
*/
SbBool
SbBox2d::findIntersection(const SbVec2d & a, const SbVec2d & b, SbVec2d & ia, SbVec2d & ib) const
{
  // FIXME: this function should be tested thoroughly...

  // we place point a and b in their respective blocks, and handle cases accordingly
  //
  //   block-ids     intersection
  //   6 | 7 | 8      candidates
  //  ---+---+---       +-8-+
  //   3 | 4 | 5        2   4
  //  ---+---+---       +-1-+
  //   0 | 1 | 2
  static int candidates[9] = { 1|2, 1, 1|4, 2, 0, 4, 2|8, 8, 4|8 };

  int blocka = 0, blockb = 0;
  if ( a[0] < this->minpt[0] ) blocka += 0;
  else if ( a[0] <= this->maxpt[0] ) blocka += 1;
  else blocka += 2;
  if ( a[1] < this->minpt[1] ) blocka += 0;
  else if ( a[1] <= this->maxpt[1] ) blocka += 3;
  else blocka += 6;
  if ( b[0] < this->minpt[0] ) blockb += 0;
  else if ( b[0] <= this->maxpt[0] ) blockb += 1;
  else blockb += 2;
  if ( b[1] < this->minpt[1] ) blockb += 0;
  else if ( b[1] <= this->maxpt[1] ) blockb += 3;
  else blockb += 6;
  int enterwalls = candidates[blocka];
  int leavewalls = candidates[blockb];
  // both a and b can be outside box in the same way
  if ( (enterwalls & leavewalls) != 0 ) return FALSE;

  SbBool foundia = FALSE;
  if ( blocka == 4 ) {
    ia = a;
    foundia = TRUE;
  }
  if ( !foundia && (enterwalls & 1) ) {
    do {
      if ( blockb == 0 || blockb == 1 || blockb == 2 ) break;
      SbVec2d vec = b - a;
      double t = (this->minpt[1] - a[1]) / vec[1];
      if ( t < 0.0 || t > 1.0 ) break;
      ia = a + vec * t;
      if ( ia[0] < this->minpt[0] || ia[0] > this->maxpt[0] ) break;
      foundia = TRUE;
    } while ( FALSE );
  }
  if ( !foundia && (enterwalls & 2) ) {
    do {
      if ( blockb == 0 || blockb == 3 || blockb == 6 ) break;
      SbVec2d vec = b - a;
      double t = (this->minpt[0] - a[0]) / vec[0];
      if ( t < 0.0 || t > 1.0 ) break;
      ia = a + vec * t;
      if ( ia[1] < this->minpt[1] || ia[1] > this->maxpt[1] ) break;
      foundia = TRUE;
    } while ( FALSE );
  }
  if ( !foundia && (enterwalls & 4) ) {
    do {
      if ( blockb == 2 || blockb == 5 || blockb == 8 ) break;
      SbVec2d vec = b - a;
      double t = (this->maxpt[0] - a[0]) / vec[0];
      if ( t < 0.0 || t > 1.0 ) break;
      ia = a + vec * t;
      if ( ia[1] < this->minpt[1] || ia[1] > this->maxpt[1] ) break;
      foundia = TRUE;
    } while ( FALSE );
  }
  if ( !foundia && (enterwalls & 8) ) {
    do {
      if ( blockb == 6 || blockb == 7 || blockb == 8 ) break;
      SbVec2d vec = b - a;
      double t = (this->maxpt[1] - a[1]) / vec[1];
      if ( t < 0.0 || t > 1.0 ) break;
      ia = a + vec * t;
      if ( ia[0] < this->minpt[0] || ia[0] > this->maxpt[0] ) break;
      foundia = TRUE;
    } while ( FALSE );
  }
  if ( !foundia ) return FALSE;

  SbBool foundib = FALSE;
  if ( blockb == 4 ) {
    ib = b;
    foundib = TRUE;
  }
  if ( !foundib && (leavewalls & 1) ) {
    do {
      if ( blocka == 0 || blocka == 1 || blocka == 2 ) break;
      SbVec2d vec = a - b;
      double t = (this->minpt[1] - b[1]) / vec[1];
      if ( t < 0.0 || t > 1.0 ) break;
      ib = b + vec * t;
      if ( ib[0] < this->minpt[0] || ib[0] > this->maxpt[0] ) break;
      foundib = TRUE;
    } while ( FALSE );
  }
  if ( !foundib && (leavewalls & 2) ) {
    do {
      if ( blocka == 0 || blocka == 3 || blocka == 6 ) break;
      SbVec2d vec = a - b;
      double t = (this->minpt[0] - b[0]) / vec[0];
      if ( t < 0.0 || t > 1.0 ) break;
      ib = b + vec * t;
      if ( ib[1] < this->minpt[1] || ib[1] > this->maxpt[1] ) break;
      foundib = TRUE;
    } while ( FALSE );
  }
  if ( !foundib && (leavewalls & 4) ) {
    do {
      if ( blocka == 2 || blocka == 5 || blocka == 8 ) break;
      SbVec2d vec = a - b;
      double t = (this->maxpt[0] - b[0]) / vec[0];
      if ( t < 0.0 || t > 1.0 ) break;
      ib = b + vec * t;
      if ( ib[1] < this->minpt[1] || ib[1] > this->maxpt[1] ) break;
      foundib = TRUE;
    } while ( FALSE );
  }
  if ( !foundib && (leavewalls & 8) ) {
    do {
      if ( blocka == 6 || blocka == 7 || blocka == 8 ) break;
      SbVec2d vec = a - b;
      double t = (this->maxpt[1] - b[1]) / vec[1];
      if ( t < 0.0 || t > 1.0 ) break;
      ib = b + vec * t;
      if ( ib[0] < this->minpt[0] || ib[0] > this->maxpt[0] ) break;
      foundib = TRUE;
    } while ( FALSE );
  }
  if ( !foundib ) return FALSE;

  return TRUE;
} // findIntersection()

/*!
  Return the point on the box closest to the given point \a p.
  If the given point equals the center, the center point on
  the positive X-side is returned.
 */
SbVec2d
SbBox2d::getClosestPoint(const SbVec2d & point) const
{
  if (isEmpty()) return point;

  double halfwidth = (this->maxpt[0] - this->minpt[0]) / 2.0;
  double halfheight = (this->maxpt[1] - this->minpt[1]) / 2.0;

  SbVec2d center = this->getCenter();
  if (point == center)
    return SbVec2d(this->maxpt[0], halfheight);

  SbVec2d vec = point - center;

  SbVec2d absvec;
  absvec[0] = halfwidth > 0.0 ? fabs(vec[0] / halfwidth) : fabs(vec[0]);
  absvec[1] = halfheight > 0.0 ? fabs(vec[1] / halfheight) : fabs(vec[1]);

  SbVec2d closest;

  // Clamp to be on box hull.
  closest[0] = SbMin(absvec[0], 1.0);
  closest[1] = SbMin(absvec[1], 1.0);

  // Move point to be on the nearest side of the unit box ((-1 -1), (1 1)).
  if (absvec[0] > absvec[1]) // x-axis
    closest[0] = 1.0;
  else if (absvec[1] > absvec[0]) // y-axis
    closest[1] = 1.0;
  else if (absvec[0] == absvec[1]) // corner
    closest = SbVec2d(1.0, 1.0);

  closest[0] *= (vec[0] < 0.0) ? -halfwidth : halfwidth;
  closest[1] *= (vec[1] < 0.0) ? -halfheight : halfheight;

  closest += center;

  return closest;
}

/*!
  \fn void SbBox2d::getBounds(double & xmin, double & ymin, double & xmax, double & ymax) const

  Returns the box boundaries.

  \sa setBounds(), getMin(), getMax().
*/

/*!
  \fn void SbBox2d::getBounds(SbVec2d & min, SbVec2d & max) const

  Returns the box min and max corner points.

  \sa setBounds(), getMin(), getMax().
*/

/*!
  \fn void SbBox2d::getOrigin(double & originX, double & originY) const

  Returns the coordinates of the box origin (i.e. the lower left corner).

  \sa getMin().
*/

/*!
  \fn void SbBox2d::getSize(double & sizeX, double & sizeY) const

  Returns width and height of box.
*/

/*!
  \fn SbVec2d SbBox2d::getSize(void) const

  Returns width and height of box as a 2D vector.

  \since Coin 3.0
*/

/*!
  \fn double SbBox2d::getAspectRatio(void) const

  Returns aspect ratio of box, which is defined as box width divided on
  box height.
*/

/*!
  \fn int operator == (const SbBox2d & b1, const SbBox2d & b2)
  \relates SbBox2d

  Check \a b1 and \a b2 for equality.
*/

/*!
  \fn int operator != (const SbBox2d & b1, const SbBox2d & b2)
  \relates SbBox2d

  Check \a b1 and \a b2 for inequality.
*/

#ifdef COIN_TEST_SUITE
BOOST_AUTO_TEST_CASE(checkSize) {
  SbVec2d min(1,2);
  SbVec2d max(3,4);

  SbVec2d diff = max - min;

  SbBox2d box(min, max);

  BOOST_CHECK_MESSAGE(box.getSize() == diff,
                      "Box has incorrect size");
}
BOOST_AUTO_TEST_CASE(checkGetClosestPoint) {
  SbVec2d point(1524, 13794);
  SbVec2d min(1557, 3308);
  SbVec2d max(3113, 30157);

  SbBox2d box(min, max);
  SbVec2d expected(1557, 13794);

  BOOST_CHECK_MESSAGE(box.getClosestPoint(point) == expected,
                      "Closest point does not fit");

  SbVec2d sizes = box.getSize();
  SbVec2d expectedCenterQuery(max[0], sizes[1] / 2.0);

  BOOST_CHECK_MESSAGE(box.getClosestPoint(box.getCenter()) == expectedCenterQuery,
                      "Closest point for center query does not fit");
}
#endif //COIN_TEST_SUITE
