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
  \class SoPointDetail SoPointDetail.h Inventor/details/SoPointDetail.h
  \brief The SoPointDetail class is for storing detailed information for a single 3D point.

  \ingroup coin_details

  Instances of this class are used among other things for storing
  information about the vertices of lines and polygons after pick
  operations, and for storing information returned to tessellation
  callbacks.

  It contains indices into the vertex coordinate sets, along with
  indices into material, texture and normal coordinates for the point.
*/

#include <Inventor/details/SoPointDetail.h>
#include <Inventor/SbName.h>

SO_DETAIL_SOURCE(SoPointDetail);

/*!
  Sets up an empty detail instance (all indices are equal to 0).
 */
SoPointDetail::SoPointDetail(void)
  : coordindex(0), matindex(0), normindex(0), texcoordindex(0)
{
}

/*!
  Destructor.
 */
SoPointDetail::~SoPointDetail()
{
}

/*!
  \copybrief SoDetail::initClass(void)
*/
void
SoPointDetail::initClass(void)
{
  SO_DETAIL_INIT_CLASS(SoPointDetail, SoDetail);
}

// Doc in superclass.
SoDetail *
SoPointDetail::copy(void) const
{
  SoPointDetail * copy = new SoPointDetail;
  *copy = *this;
  return copy;
}

/*!
  Returns index into coordinate set for the point's 3D coordinates.
 */
int
SoPointDetail::getCoordinateIndex(void) const
{
  return this->coordindex;
}

/*!
  Returns point's index into set of materials.
 */
int
SoPointDetail::getMaterialIndex(void) const
{
  return this->matindex;
}

/*!
  Returns point's index into set of normals.
 */
int
SoPointDetail::getNormalIndex(void) const
{
  return this->normindex;
}

/*!
  Returns point's index into set of texture coordinates.
 */
int
SoPointDetail::getTextureCoordIndex(void) const
{
  return this->texcoordindex;
}

/*!
  Used by client code for initializing the point detail instance.
 */
void
SoPointDetail::setCoordinateIndex(const int idx)
{
  this->coordindex = idx;
}

/*!
  Used by client code for initializing the point detail instance.
 */
void
SoPointDetail::setMaterialIndex(const int idx)
{
  this->matindex = idx;
}

/*!
  Used by client code for initializing the point detail instance.
 */
void
SoPointDetail::setNormalIndex(const int idx)
{
  this->normindex = idx;
}

/*!
  Used by client code for initializing the point detail instance.
 */
void
SoPointDetail::setTextureCoordIndex(const int idx)
{
  this->texcoordindex = idx;
}
