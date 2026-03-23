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
  \class SoFaceDetail SoFaceDetail.h Inventor/details/SoFaceDetail.h
  \brief The SoFaceDetail class is for storing detailed polygon information.

  \ingroup coin_details

  Instances of this class are used among other things for storing
  information about polygons after pick operations, and for storing
  information returned to tessellation callbacks.

  Note that a SoFaceDetail instance consists of a set of SoPointDetail
  instances, one for each vertex of the polygon it represents.

  \sa SoRayPickAction, SoPickedPoint, SoCallbackAction
*/

#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/SbName.h>
#include <cstddef>

SO_DETAIL_SOURCE(SoFaceDetail);

/*!
  Default constructor sets up an empty, non-valid detail
  specification.
 */
SoFaceDetail::SoFaceDetail(void)
  : pointsarray(NULL),
    numallocated(0),
    numpoints(0),
    faceindex(0),
    partindex(0)
{
}

/*!
  Destructor, free internal resources used for storing the polygon
  vertices.
 */
SoFaceDetail::~SoFaceDetail()
{
  delete [] this->pointsarray;
}

/*!
  \copybrief SoDetail::initClass(void)
*/
void
SoFaceDetail::initClass(void)
{
  SO_DETAIL_INIT_CLASS(SoFaceDetail, SoDetail);
}

// doc in super
SoDetail *
SoFaceDetail::copy(void) const
{
  SoFaceDetail *copy = new SoFaceDetail();
  if (this->numpoints) {
    copy->setNumPoints(this->numpoints);
    for (int i = 0; i < this->numpoints; i++) {
      copy->setPoint(i, this->getPoint(i));
    }
  }
  copy->faceindex = this->faceindex;
  copy->partindex = this->partindex;
  return copy;
}

/*!
  Number of vertices making up the polygon.
 */
int
SoFaceDetail::getNumPoints(void) const
{
  return this->numpoints;
}

/*!
  Returns a pointer into the array of vertices, starting at the \a
  idx'th vertex of the polygon.

  The array will contain (SoFaceDetail::getNumPoints() - \a idx)
  elements.
 */
const SoPointDetail *
SoFaceDetail::getPoint(const int idx) const
{
  assert(idx >= 0 && idx < this->numpoints);
  return &this->pointsarray[idx];
}

/*!
  Returns the full array of vertices details for the polygon. The array
  will contain SoFaceDetail::getNumPoints() elements.
 */
SoPointDetail *
SoFaceDetail::getPoints(void)
{
  return this->pointsarray;
}

/*!
  Returns the index of this polygon within the faceset node it is part
  of.
 */
int
SoFaceDetail::getFaceIndex(void) const
{
  return this->faceindex;
}

/*!
  If this SoFaceDetail represents a triangle tessellated from a complex
  shape, this method returns the index of the part of the complex
  shape it was tessellated from.
 */
int
SoFaceDetail::getPartIndex(void) const
{
  return this->partindex;
}

/*!
  Used internally from library client code setting up a SoFaceDetail
  instance.

  \sa getNumPoints()
 */
void
SoFaceDetail::setNumPoints(const int num)
{
  if (num > this->numallocated) {
    this->numallocated = num;
    delete [] this->pointsarray;
    this->pointsarray = new SoPointDetail[this->numallocated];
  }
  this->numpoints = num;
}

/*!
  Used internally from library client code setting up a SoFaceDetail
  instance.

  \sa getPoint(), getPoints()
 */
void
SoFaceDetail::setPoint(const int idx, const SoPointDetail * const detail)
{
  assert(idx >= 0 && idx <= this->numpoints);
  this->pointsarray[idx] = *detail;
}

/*!
  Used internally from library client code setting up a SoFaceDetail
  instance.

  \sa getFaceIndex()
 */
void
SoFaceDetail::setFaceIndex(const int idx)
{
  this->faceindex = idx;
}

/*!
  Used internally from library client code setting up a SoFaceDetail
  instance.

  \sa getPartIndex()
 */
void
SoFaceDetail::setPartIndex(const int idx)
{
  this->partindex = idx;
}

/*!
  Used internally from library client code setting up a SoFaceDetail
  instance.

  This function is specific for Coin, and is not present in SGI/TGS
  Open Inventor.
*/
void
SoFaceDetail::incFaceIndex(void)
{
  this->faceindex++;
}

/*!
  Used internally from library client code setting up a SoFaceDetail
  instance.

  This function is specific for Coin, and is not present in SGI/TGS
  Open Inventor.
*/
void
SoFaceDetail::incPartIndex(void)
{
  this->partindex++;
}
