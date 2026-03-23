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
  \class SoLineDetail SoLineDetail.h Inventor/details/SoLineDetail.h
  \brief The SoLineDetail class is for storing detailed 3D line information.

  \ingroup coin_details

  Instances of this class are used among other things for storing
  information about lines after pick operations, and for storing
  information returned to tessellation callbacks.

  \sa SoRayPickAction, SoPickedPoint, SoCallbackAction
*/

#include <Inventor/details/SoLineDetail.h>
#include <Inventor/SbName.h>

SO_DETAIL_SOURCE(SoLineDetail);


/*!
  Default constructor sets up an empty, non-valid detail
  specification.
*/
SoLineDetail::SoLineDetail(void)
  : lineindex(0), partindex(0)
{
}

/*!
  Destructor.
 */
SoLineDetail::~SoLineDetail()
{
}

/*!
  \copybrief SoDetail::initClass(void)
*/
void
SoLineDetail::initClass(void)
{
  SO_DETAIL_INIT_CLASS(SoLineDetail, SoDetail);
}

// Doc in superclass.
SoDetail *
SoLineDetail::copy(void) const
{
  SoLineDetail *copy = new SoLineDetail();
  *copy = *this;
  return copy;

}

/*!
  Returns SoPointDetail describing the line start point.
 */
const SoPointDetail *
SoLineDetail::getPoint0(void) const
{
  return &this->points[0];
}

/*!
  Returns SoPointDetail describing the line end point.
 */
const SoPointDetail *
SoLineDetail::getPoint1(void) const
{
  return &this->points[1];
}

/*!
  Returns the index of this line within the lineset node it is part
  of.
 */
int
SoLineDetail::getLineIndex(void) const
{
  return this->lineindex;
}

/*!
  Returns the index of this line within the complex shape node it is
  part of.
 */
int
SoLineDetail::getPartIndex(void) const
{
  return this->partindex;
}

/*!
  Stores information about line's start point. Used internally from
  library client code setting up a SoLineDetail instance.

  \sa getPoint0()
 */
void
SoLineDetail::setPoint0(const SoPointDetail * const detail)
{
  this->points[0] = *detail;
}

/*!
  Stores information about line's end point. Used internally from
  library client code setting up a SoLineDetail instance.

  \sa getPoint1()
 */
void
SoLineDetail::setPoint1(const SoPointDetail * const detail)
{
  this->points[1] = *detail;
}

/*!
  Used internally from library client code setting up a SoLineDetail
  instance.

  \sa getLineIndex()
 */
void
SoLineDetail::setLineIndex(const int idx)
{
  this->lineindex = idx;
}

/*!
  Used internally from library client code setting up a SoLineDetail
  instance.

  \sa getPartIndex()
 */
void
SoLineDetail::setPartIndex(const int idx)
{
  this->partindex = idx;
}

/*!
  Convenience method for library client code when setting up a
  SoLineDetail instance to use the line index as a counter.
*/
void
SoLineDetail::incLineIndex(void)
{
  this->lineindex++;
}

/*!
  Convenience method for library client code when setting up a
  SoLineDetail instance to use the part index as a counter.
*/
void
SoLineDetail::incPartIndex(void)
{
  this->partindex++;
}
