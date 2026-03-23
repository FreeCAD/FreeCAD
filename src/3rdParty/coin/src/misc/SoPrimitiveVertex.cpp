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
  \class SoPrimitiveVertex SoPrimitiveVertex.h Inventor/SoPrimitiveVertex.h
  \brief The SoPrimitiveVertex class represents a single vertex of a generated primitive.

  \ingroup coin_general

  Instances of SoPrimitiveVertex are constructed when generating
  primitive data, primarily during an SoCallbackAction traversal.
  Depending on the context the vertex could represent a single 3D
  point, one of the two vertices in a line or one of the three
  vertices in a triangle.
*/

#include <Inventor/SoPrimitiveVertex.h>
#include <cstdlib>

/*!
  Default constructor, sets up a "void" instance.
*/
SoPrimitiveVertex::SoPrimitiveVertex(void)
  : point(0.0f, 0.0f, 0.0f),
    normal(0.0f, 0.0f, 1.0f),
    textureCoords(0.0f, 0.0f, 0.0f, 1.0f),
    materialIndex(0),
    detail(NULL),
    packedColor(0)
{
}

/*!
  Copy operator.

  When \a pv is copied into this instance, a \e shallow copy is
  made. I.e., only the reference to the detail instance is copied (if
  any), not the detail itself.
*/

SoPrimitiveVertex &
SoPrimitiveVertex::operator = (const SoPrimitiveVertex & pv)
{
  this->point = pv.point;
  this->normal = pv.normal;
  this->textureCoords = pv.textureCoords;
  this->materialIndex = pv.materialIndex;
  this->detail = pv.detail;
  this->packedColor = pv.packedColor;
  return *this;
}

/*!
  Copy constructor. Does a shallow copy.

  \sa SoPrimitiveVertex::operator=()
*/

SoPrimitiveVertex::SoPrimitiveVertex(const SoPrimitiveVertex & pv)
{
  *this = pv;
}

/*!
  Destructor. The detail instance is owned by client code and will not
  be destructed here.
*/

SoPrimitiveVertex::~SoPrimitiveVertex()
{
}

/*!
  \fn const SbVec3f & SoPrimitiveVertex::getPoint(void) const

  Returns vertex coordinates, positioned in object space.
*/

/*!
  \fn const SbVec3f & SoPrimitiveVertex::getNormal(void) const

  Returns normal vector, oriented in object space.
*/

/*!
  \fn const SbVec4f & SoPrimitiveVertex::getTextureCoords(void) const

  Returns texture coordinates for vertex, specified in object space.
*/

/*!
  \fn int SoPrimitiveVertex::getMaterialIndex(void) const

  Returns index of the vertex into the currently active material, if
  any.
*/

/*!
  \fn uint32_t SoPrimitiveVertex::getPackedColor(void) const

  Returns the RGBA packed color for the given vertex.

  \since Coin 3.0
*/

/*!
  \fn const SoDetail * SoPrimitiveVertex::getDetail(void) const

  Returns pointer to detail instance with more information about the
  vertex. A detail instance may not be available, and if so \c NULL is
  returned.
*/

/*!
  \fn void SoPrimitiveVertex::setPoint(const SbVec3f & pt)

  Used internally from library client code setting up an
  SoPrimitiveVertex instance.
*/

/*!
  \fn void SoPrimitiveVertex::setPoint(float x, float y, float z)

  Used internally from library client code setting up an
  SoPrimitiveVertex instance.
*/

/*!
  \fn void SoPrimitiveVertex::setNormal(const SbVec3f & n)

  Used internally from library client code setting up an
  SoPrimitiveVertex instance.
*/

/*!
  \fn void SoPrimitiveVertex::setNormal(float nx, float ny, float nz)

  Used internally from library client code setting up an
  SoPrimitiveVertex instance.
*/

/*!
  \fn void SoPrimitiveVertex::setTextureCoords(const SbVec2f & texcoords)

  Used internally from library client code setting up an
  SoPrimitiveVertex instance.

  Convenience function. Will fill in 0 and 1 in the last two texture
  coordinates in the internal SbVec4f texture coordinate instance.
*/

/*!
  \fn void SoPrimitiveVertex::setTextureCoords(float tx, float ty)

  Used internally from library client code setting up an
  SoPrimitiveVertex instance.

  Convenience function. Will fill in 0 and 1 in the last two texture
  coordinates in the internal SbVec4f texture coordinate instance.
*/

/*!
  \fn void SoPrimitiveVertex::setTextureCoords(const SbVec3f & texcoords)

  Convenience function. Will fill in 1 in the last coordinate.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/

/*!
  \fn void SoPrimitiveVertex::setTextureCoords(float tx, float ty, float tz)

  Convenience function. Will fill in 1 in the last coordinate.

  \COIN_FUNCTION_EXTENSION
  \since Coin 2.5
*/

/*!
  \fn void SoPrimitiveVertex::setTextureCoords(const SbVec4f & texcoords)

  Used internally from library client code setting up an
  SoPrimitiveVertex instance.
*/

/*!
  \fn void SoPrimitiveVertex::setTextureCoords(float tx, float ty, float tz, float tw)

  Used internally from library client code setting up an
  SoPrimitiveVertex instance.

  \COIN_FUNCTION_EXTENSION
  \since Coin 2.5
*/

/*!
  \fn void SoPrimitiveVertex::setMaterialIndex(int index)

  Used internally from library client code setting up an
  SoPrimitiveVertex instance.
*/

/*!
  \fn void SoPrimitiveVertex::setPackedColor(uint32_t rgba)

  Used internally from library client code setting up an
  SoPrimitiveVertex instance.
*/

/*!
  \fn void SoPrimitiveVertex::setDetail(SoDetail * detail)

  Used internally from library client code setting up an
  SoPrimitiveVertex instance.

  Note that it is the client's responsibility to do the deallocation of
  the detail instance after the SoPrimitiveVertex instance has gone
  out of scope.
*/
