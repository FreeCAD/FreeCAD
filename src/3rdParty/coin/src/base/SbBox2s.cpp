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
  \class SbBox2s SbBox2s.h Inventor/SbBox2s.h
  \brief The SbBox2s class is a 2 dimensional box with short
  integer coordinates.

  \ingroup coin_base

  This box class is used by other classes in Coin for data
  exchange. It provides storage for two box corners with short integer
  coordinates, which is among other things useful for representing
  screen or canvas areas in absolute window coordinates.

  \sa SbBox2f, SbBox2d, SbBox3s, SbBox3f, SbBox3d, SbXfBox3f.
*/

// *************************************************************************

#include <Inventor/SbBox2s.h>

#include <limits>
#include <cassert>

#include <Inventor/SbBox2i32.h>
#include <Inventor/SbBox2f.h>
#include <Inventor/SbBox2d.h>
#include <Inventor/errors/SoDebugError.h>

// *************************************************************************

/*!
  \fn SbBox2s::SbBox2s(void)

  The default constructor makes an empty box.
*/

/*!
  \fn SbBox2s::SbBox2s(short xmin, short ymin, short xmax, short ymax)

  Constructs a box with the given corner coordinates.

  \a xmin should be less than \a xmax and \a ymin should be less than
  \a ymax if you want to make a valid box.
*/

/*!
  \fn SbBox2s::SbBox2s(const SbVec2s & boxmin, const SbVec2s & boxmax)

  Constructs a box with the given corners.

  The coordinates of \a min should be less than the coordinates of
  \a max if you want to make a valid box.
*/

/*!
  \fn SbBox2s::SbBox2s(const SbBox2i32 & box)

  Constructs an SbBox2s instance from the value in an SbBox2i32 instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox2s::SbBox2s(const SbBox2f & box)

  Constructs an SbBox2s instance from the value in an SbBox2f instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox2s::SbBox2s(const SbBox2d & box)

  Constructs an SbBox2s instance from the value in an SbBox2d instance.

  \since Coin 2.5
*/

/*!
  \fn SbBox2s & SbBox2s::setBounds(short xmin, short ymin, short xmax, short ymax)

  Reset the boundaries of the box.

  \a xmin should be less than \a xmax and \a ymin should be less than
  \a ymax if you want to make a valid box.

  Returns reference to self.

  \sa getBounds().
*/

/*!
  \fn SbBox2s & SbBox2s::setBounds(const SbVec2s & boxmin, const SbVec2s & boxmax)

  Reset the boundaries of the box with the given corners.

  The coordinates of \a min should be less than the coordinates of
  \a max if you want to make a valid box.

  Returns reference to self.

  \sa getBounds().
*/

/*!
  Reset the boundaries with the boundaries of the given \a box.

  Returns reference to self.

  \sa setBounds()
*/

SbBox2s &
SbBox2s::setBounds(const SbBox2i32 & box)
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
  Reset the boundaries with the boundaries of the given \a box.

  Returns reference to self.

  \sa setBounds()
*/

SbBox2s &
SbBox2s::setBounds(const SbBox2f & box)
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
  Reset the boundaries with the boundaries of the given \a box.

  Returns reference to self.

  \sa setBounds()
*/

SbBox2s &
SbBox2s::setBounds(const SbBox2d & box)
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
SbBox2s::makeEmpty(void)
{
  minpt.setValue(std::numeric_limits<short>::max(), std::numeric_limits<short>::max());
  maxpt.setValue(-std::numeric_limits<short>::max(), -std::numeric_limits<short>::max());
}

/*!
  \fn SbBool SbBox2s::isEmpty(void) const

  Check if this has been marked as an empty box.

  \sa makeEmpty().
*/

/*!
  \fn SbBool SbBox2s::hasArea(void) const

  Check if the box has been correctly specified and by that virtue
  has "positive" area, i.e. all coordinates of its upper right corner
  (the maximum point) are greater than the corresponding coordinates 
  of its lower left corner (the minimum point).
*/

/*!
  \fn const SbVec2s & SbBox2s::getMin(void) const

  Returns the minimum point. This should usually be the lower left corner
  point of the box.

  \sa getOrigin(), getMax().
*/

/*!
  \fn SbVec2s & SbBox2s::getMin(void)

  Returns the lower left corner of the box.

  \sa getOrigin(), getMax().
*/

/*!
  \fn const SbVec2s & SbBox2s::getMax(void) const

  Returns the maximum point. This should usually be the upper right corner
  point of the box.

  \sa getMin().
*/

/*!
  \fn SbVec2s & SbBox2s::getMax(void)

  Returns the upper right corner of the box.

  \sa getMin().
*/

/*!
  \fn SbVec2s SbBox2s::getCenter(void) const

  Returns the center point of the box.
*/

/*!
  Extend the boundaries of the box by the given point, i.e. make the
  point fit inside the box if it isn't already within it.
*/
void
SbBox2s::extendBy(const SbVec2s & point)
{
  // The explicit casts are done to humour the HPUX aCC compiler,
  // which will otherwise say ``Template deduction failed to find a
  // match for the call to 'SbMin'''. mortene.
  this->minpt.setValue(SbMin(static_cast<short>(point[0]), static_cast<short>(this->minpt[0])),
                       SbMin(static_cast<short>(point[1]), static_cast<short>(this->minpt[1])));
  this->maxpt.setValue(SbMax(static_cast<short>(point[0]), static_cast<short>(this->maxpt[0])),
                       SbMax(static_cast<short>(point[1]), static_cast<short>(this->maxpt[1])));
}

/*!
  Extend the boundaries of the box by the given \a box parameter. This
  is equal to calling extendBy() twice with the corner points.
*/
void
SbBox2s::extendBy(const SbBox2s & box)
{
  if (box.isEmpty()) { return; }

  this->extendBy(box.getMin());
  this->extendBy(box.getMax());
}

/*!
  Check if the given point lies within the boundaries of this box.
*/
SbBool
SbBox2s::intersect(const SbVec2s & point) const
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
SbBox2s::intersect(const SbBox2s & box) const
{
  if((box.getMax()[0] < this->getMin()[0]) ||
     (box.getMax()[1] < this->getMin()[1]) ||
     (box.getMin()[0] > this->getMax()[0]) ||
     (box.getMin()[1] > this->getMax()[1])) return FALSE;
  return TRUE;
}

/*!
  \fn void SbBox2s::getBounds(short & xmin, short & ymin, short & xmax, short & ymax) const

  Returns the box boundary coordinates.

  \sa setBounds(), getMin(), getMax().
*/

/*!
  \fn void SbBox2s::getBounds(SbVec2s & boxmin, SbVec2s & boxmax) const

  Returns the box corner points.

  \sa setBounds(), getMin(), getMax().
*/

/*!
  \fn void SbBox2s::getOrigin(short & originX, short & originY) const

  Returns the coordinates of the box origin (i.e. the lower left corner).

  \sa getMin().
*/

/*!
  \fn void SbBox2s::getSize(short & sizeX, short & sizeY) const

  Returns width and height of box.
*/

/*!
  \fn SbVec2s SbBox2s::getSize(void) const

  Returns width and height of box as a 2D vector.

  \since Coin 3.0
*/

/*!
  \fn float SbBox2s::getAspectRatio(void) const

  Returns aspect ratio of box, which is defined as box width divided on
  box height.
*/

/*!
  \fn int operator == (const SbBox2s & b1, const SbBox2s & b2)
  \relates SbBox2s

  Check \a b1 and \a b2 for equality.
*/

/*!
  \fn int operator != (const SbBox2s & b1, const SbBox2s & b2)
  \relates SbBox2s

  Check \a b1 and \a b2 for inequality.
*/

/*!
  \fn SbBool SbBox2s::hasArea(void) const

  Check if the box has been correctly specified and by that virtue
  has "positive" area, i.e. all coordinates of its upper right corner
  (the maximum point) are greater than the corresponding coordinates 
  of its lower left corner (the minimum point).
*/

#ifdef COIN_TEST_SUITE
BOOST_AUTO_TEST_CASE(checkSize) {
  SbVec2s min(1,2);
  SbVec2s max(3,4);

  SbVec2s diff = max - min;

  
  SbBox2s box(min, max);

  BOOST_CHECK_MESSAGE(box.getSize() == diff,
                      "Box has incorrect size");

}
#endif //COIN_TEST_SUITE
