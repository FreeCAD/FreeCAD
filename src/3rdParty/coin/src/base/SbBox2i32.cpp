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
  \class SbBox2i32 SbBox2i32.h Inventor/SbBox2i32.h
  \brief The SbBox2i32 class is a 2 dimensional box with 32-bit
  integer coordinates.

  \ingroup coin_base

  This box class is used by other classes in Coin for data
  exchange. It provides storage for two box corners with 32-bit integer
  coordinates, which is among other things useful for representing
  screen or canvas areas in absolute window coordinates.

  \sa SbBox2f, SbBox2d, SbBox3s, SbBox3f, SbBox3d, SbXfBox3f.
*/

// *************************************************************************

#include <Inventor/SbBox2i32.h>

#include <limits>
#include <cassert>

#include <Inventor/SbBox2s.h>
#include <Inventor/SbBox2f.h>
#include <Inventor/SbBox2d.h>
#include <Inventor/errors/SoDebugError.h>

// *************************************************************************

/*!
  \fn SbBox2i32::SbBox2i32(void)

  The default constructor makes an empty box.
*/

/*!
  \fn SbBox2i32::SbBox2i32(int32_t xmin, int32_t ymin, int32_t xmax, int32_t ymax)

  Constructs a box with the given corner coordinates.

  \a xmin should be less than \a xmax and \a ymin should be less than
  \a ymax if you want to make a valid box.
*/

/*!
  \fn SbBox2i32::SbBox2i32(const SbVec2i32 & minpt, const SbVec2i32 & maxpt)

  Constructs a box with the given corners.

  The coordinates of \a min should be less than the coordinates of
  \a max if you want to make a valid box.
*/

/*!
  \fn SbBox2i32::SbBox2i32(const SbBox2s & box)

  Constructs an SbBox2i32 instance from the value in an SbBox2s instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox2i32::SbBox2i32(const SbBox2f & box)

  Constructs an SbBox2i32 instance from the value in an SbBox2f instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox2i32::SbBox2i32(const SbBox2d & box)

  Constructs an SbBox2i32 instance from the value in an SbBox2d instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox2i32 &  SbBox2i32::setBounds(int32_t xmin, int32_t ymin, int32_t xmax, int32_t ymax)

  Reset the boundaries of the box.

  \a xmin should be less than \a xmax and \a ymin should be less than
  \a ymax if you want to make a valid box.

  Returns reference to self.

  \sa getBounds().
*/

/*!
  \fn SbBox2i32 & SbBox2i32::setBounds(const SbVec2i32 & boxmin, const SbVec2i32 & boxmax)

  Reset the boundaries of the box with the given corners.

  The coordinates of \a min should be less than the coordinates of
  \a max if you want to make a valid box.

  Returns reference to self.

  \sa getBounds().
*/

/*!
  Reset the boundaries to the boundaries of the given \a box.

  Returns reference to self.

  \sa getBounds()
*/

SbBox2i32 &
SbBox2i32::setBounds(const SbBox2s & box)
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

SbBox2i32 &
SbBox2i32::setBounds(const SbBox2f & box)
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

SbBox2i32 &
SbBox2i32::setBounds(const SbBox2d & box)
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
SbBox2i32::makeEmpty(void)
{
  minpt.setValue(std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max());
  maxpt.setValue(-std::numeric_limits<int32_t>::max(), -std::numeric_limits<int32_t>::max());
}

/*!
  \fn const SbVec2i32 & SbBox2i32::getMin(void) const

  Returns the minimum point. This should usually be the lower left corner
  point of the box.

  \sa getOrigin(), getMax().
*/

/*!
  \fn const SbVec2i32 & SbBox2i32::getMax(void) const

  Returns the maximum point. This should usually be the upper right corner
  point of the box.

  \sa getMin().
*/

/*!
  \fn SbVec2i32 & SbBox2i32::getMin(void)

  Returns a modifiable reference to the minimum point.
*/

/*!
  \fn SbVec2i32 & SbBox2i32::getMax(void)

  Returns a modifiable reference to the maximum point.
*/

/*!
  \fn SbVec2i32 SbBox2i32::getCenter(void) const

  Returns the center point of the box.
*/

/*!
  Extend the boundaries of the box by the given point, i.e. make the
  point fit inside the box if it isn't already within it.
*/
void
SbBox2i32::extendBy(const SbVec2i32 & point)
{
  this->minpt.setValue(SbMin(point[0], this->minpt[0]),
                       SbMin(point[1], this->minpt[1]));
  this->maxpt.setValue(SbMax(point[0], this->maxpt[0]),
                       SbMax(point[1], this->maxpt[1]));
}

/*!
  Extend the boundaries of the box by the given \a box parameter. This
  is equal to calling extendBy() twice with the corner points.
*/
void
SbBox2i32::extendBy(const SbBox2i32 & box)
{
  if (box.isEmpty()) { return; }

  this->extendBy(box.getMin());
  this->extendBy(box.getMax());
}

/*!
  Check if the given point lies within the boundaries of this box.
*/
SbBool
SbBox2i32::intersect(const SbVec2i32 & point) const
{
  if((point[0] >= this->minpt[0]) && (point[0] <= this->maxpt[0]) &&
     (point[1] >= this->minpt[1]) && (point[1] <= this->maxpt[1])) return TRUE;
  return FALSE;
}

/*!
  Check if \a box lies entirely or partially within the boundaries
  of this box.
*/
SbBool
SbBox2i32::intersect(const SbBox2i32 & box) const
{
  if((box.getMax()[0] < this->getMin()[0]) ||
     (box.getMax()[1] < this->getMin()[1]) ||
     (box.getMin()[0] > this->getMax()[0]) ||
     (box.getMin()[1] > this->getMax()[1])) return FALSE;
  return TRUE;
}

/*!
  \fn void SbBox2i32::getBounds(int32_t & xmin, int32_t & ymin, int32_t & xmax, int32_t & ymax) const

  Returns the box boundary coordinates.

  \sa setBounds(), getMin(), getMax().
*/

/*!
  \fn void SbBox2i32::getBounds(SbVec2i32 & boxmin, SbVec2i32 & boxmax) const

  Returns the box corner points.

  \sa setBounds(), getMin(), getMax().
*/

/*!
  \fn void SbBox2i32::getOrigin(int32_t & originX, int32_t & originY) const

  Returns the coordinates of the box origin (i.e. the lower left corner).

  \sa getMin().
*/

/*!
  \fn void SbBox2i32::getSize(int32_t & sizeX, int32_t & sizeY) const

  Returns width and height of box.
*/

/*!
  \fn SbVec2i32 SbBox2i32::getSize(void) const

  Returns width and height of box as a 2D vector.

  \since Coin 3.0
*/

/*!
  \fn float SbBox2i32::getAspectRatio(void) const

  Returns aspect ratio of box, which is defined as box width divided by
  box height.
*/

/*!
  \fn SbBool SbBox2i32::isEmpty(void) const

  Check if this has been marked as an empty box.

  \sa makeEmpty().
*/

/*!
  \fn SbBool SbBox2i32::hasArea(void) const

  Check if the box has been correctly specified and by that virtue
  has "positive" area, i.e. all coordinates of its upper right corner
  (the maximum point) are greater than the corresponding coordinates 
  of its lower left corner (the minimum point).
*/

/*!
  \fn int operator == (const SbBox2i32 & b1, const SbBox2i32 & b2)
  \relates SbBox2i32

  Check \a b1 and \a b2 for equality.
*/

/*!
  \fn int operator != (const SbBox2i32 & b1, const SbBox2i32 & b2)
  \relates SbBox2i32

  Check \a b1 and \a b2 for inequality.
*/

#ifdef COIN_TEST_SUITE
BOOST_AUTO_TEST_CASE(checkSize) {
  SbVec2i32 min(1,2);
  SbVec2i32 max(3,4);

  SbVec2i32 diff = max - min;

  
  SbBox2i32 box(min, max);

  BOOST_CHECK_MESSAGE(box.getSize() == diff,
                      "Box has incorrect size");

}
#endif //COIN_TEST_SUITE
