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
  \class SoPickedPoint SoPickedPoint.h Inventor/SoPickedPoint.h
  \brief The SoPickedPoint class is used for specifying picked points.

  \ingroup coin_general

  It holds miscellaneous information about the picked point, such
  as position, normal, texture coordinate and material index in the
  current material. It might also hold detail information (an SoDetail
  subclass) for every node in the picked path.

  \sa SoRayPickAction
*/

#include <Inventor/SoPickedPoint.h>

#include <cassert>

#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoMultiTextureMatrixElement.h>
#include <Inventor/details/SoDetail.h>

#include "tidbitsp.h"


//
// this is not thread-safe, but creating a new matrix action for
// each picked point is not very efficient.
//
static SoGetMatrixAction *matrixAction = NULL;

//
//  Will be called at the end of program to free static memory
//  used by this class.
//
static
void clean_class()
{
  delete matrixAction;
  matrixAction = NULL;
}

/*!
  Copy constructor.
*/
SoPickedPoint::SoPickedPoint(const SoPickedPoint &pp)
{
  this->path = pp.path;
  this->path->ref();
  this->state = pp.state;
  this->point = pp.point;
  this->objPoint = pp.objPoint;
  this->normal = pp.normal;
  this->objNormal = pp.objNormal;
  this->texCoords = pp.texCoords;
  this->objTexCoords = pp.objTexCoords;
  this->materialIndex = pp.materialIndex;
  this->onGeometry = pp.onGeometry;
  this->viewport = pp.viewport;

  int n = pp.detailList.getLength();
  for (int i = 0; i < n; i++) {
    if (pp.detailList[i])
      this->detailList.append(pp.detailList[i]->copy());
    else
      this->detailList.append(NULL);
  }
}

/*!
  Constructor. Uses the state to convert between world and object
  space for the data.
*/
SoPickedPoint::SoPickedPoint(const SoPath * const pathptr, SoState * const stateptr,
                             const SbVec3f &objSpacePoint)
{
  this->path = pathptr->copy();
  this->path->ref();
  this->state = stateptr;
  this->objPoint = objSpacePoint;
  SoModelMatrixElement::get(state).multVecMatrix(objSpacePoint, this->point);
  this->objNormal = this->normal = SbVec3f(0,0,1);
  this->objTexCoords = this->texCoords = SbVec4f(0,0,0,1);
  this->materialIndex = 0;
  this->onGeometry = TRUE;
  this->viewport = SoViewportRegionElement::get(stateptr);

  int pathlen = ((SoFullPath*)this->path)->getLength();
  for (int i = 0; i < pathlen; i++) {
    this->detailList.append(NULL);
  }
}

/*!
  Destructor.
 */
SoPickedPoint::~SoPickedPoint()
{
  assert(this->path);
  this->path->unref();

  // SoDetailList will delete all SoDetail instances, so we don't have
  // to do that here
}

/*!
  Returns a copy of this picked point.

  \DANGEROUS_ALLOC_RETURN
*/
SoPickedPoint *
SoPickedPoint::copy() const
{
  return new SoPickedPoint(*this);
}

/*!
  Returns the world space point.
*/
const SbVec3f &
SoPickedPoint::getPoint() const
{
  return this->point;
}

/*!
  Returns the world space normal.
*/
const SbVec3f &
SoPickedPoint::getNormal() const
{
  return this->normal;
}

/*!
  Returns the image space texture coordinates.
*/
const SbVec4f &
SoPickedPoint::getTextureCoords() const
{
  return this->texCoords;
}

/*!
  Returns the material index.
*/
int
SoPickedPoint::getMaterialIndex() const
{
  return this->materialIndex;
}

/*!
  Returns the path to the picked object.
*/
SoPath *
SoPickedPoint::getPath() const
{
  return (SoPath*)this->path;
}

/*!
  Returns TRUE if this picked point is on the actual geometry of the
  picked object, or FALSE if not (it might for instance be on the
  bounding box if picking was done on bounding boxes).
*/
SbBool
SoPickedPoint::isOnGeometry() const
{
  return this->onGeometry;
}

/*!
  Returns detail for \a node. If \a node equals NULL, the detail
  for the picked object is returned.
*/
const SoDetail *
SoPickedPoint::getDetail(const SoNode * const node) const
{
  int idx = node ? this->path->findNode(node) :
    ((SoFullPath*)this->path)->getLength() - 1;
  return idx >= 0 ? this->detailList[idx] : NULL;
}

/*!
  Returns the matrix which converts from object (specified by \a node)
  to world space. If \a node equals NULL, the object space of the
  picked object will used.
*/
const SbMatrix &
SoPickedPoint::getObjectToWorld(const SoNode * const node) const
{
  this->applyMatrixAction(node);
  return getMatrixAction()->getMatrix();
}

/*!
  Returns the matrix which converts from world to object (specified
  by \a node) space. If \a node equals NULL, the object space of the
  picked object will used.
*/
const SbMatrix &
SoPickedPoint::getWorldToObject(const SoNode * const node) const
{
  this->applyMatrixAction(node);
  return getMatrixAction()->getInverse();
}

/*!
  Returns the matrix which converts from object (specified by \a node)
  to image space. If \a node equals NULL, the object space of the
  picked object will used.
*/
const SbMatrix &
SoPickedPoint::getObjectToImage(const SoNode * const node) const
{
  this->applyMatrixAction(node);
  return getMatrixAction()->getTextureMatrix();
}

/*!
  Returns the matrix which converts from image to object (specified
  by \a node) space. If \a node equals NULL, the object space of the
  picked object will used.
*/
const SbMatrix &
SoPickedPoint::getImageToObject(const SoNode * const node) const
{
  this->applyMatrixAction(node);
  return getMatrixAction()->getTextureInverse();
}

/*!
  Returns the object space point, in the object space specified by \a
  node. If \a node equals \c NULL, the object space of the node where
  the point was actually picked will be used (this is what one would
  usually be interested in).

  \a node can be any node in the scene graph.
*/
SbVec3f
SoPickedPoint::getObjectPoint(const SoNode * const node) const
{
  if (node && node != ((SoFullPath*)this->path)->getTail()) {
    SbVec3f ret;
    this->getWorldToObject(node).multVecMatrix(this->point, ret);
    return ret;
  }
  return this->objPoint;
}

/*!
  Returns the object space (specified by \a node) normal. If
  \a node equals NULL, the picked point object space will
  be used.
*/
SbVec3f
SoPickedPoint::getObjectNormal(const SoNode * const node) const
{
  if (node && node != ((SoFullPath*)this->path)->getTail()) {
    SbVec3f ret;
    this->getWorldToObject(node).multDirMatrix(this->normal, ret);
    return ret;
  }
  return this->objNormal;
}

/*!
  Returns the object space (specified by \a node) texture coordinates.
  If \a node equals NULL, the picked point object space will be used.
*/
SbVec4f
SoPickedPoint::getObjectTextureCoords(const SoNode * const node) const
{
  if (node && node != ((SoFullPath*)this->path)->getTail()) {
    SbVec4f ret;
    this->getImageToObject(node).multVecMatrix(this->texCoords, ret);
    return ret;
  }
  return this->objTexCoords;
}

/*!
  Sets the picked point objects space normal vector.
*/
void
SoPickedPoint::setObjectNormal(const SbVec3f &normalref)
{
  this->objNormal = normalref;
  SoModelMatrixElement::get(this->state).multDirMatrix(normalref, this->normal);
}

/*!
  Sets the picked point object space texture coordinates.
*/
void
SoPickedPoint::setObjectTextureCoords(const SbVec4f &texCoordsref)
{
  this->objTexCoords = texCoordsref;
  SoMultiTextureMatrixElement::get(this->state, 0).multVecMatrix(texCoordsref, this->texCoords);
}

/*!
  Sets the material index.
*/
void
SoPickedPoint::setMaterialIndex(const int index)
{
  this->materialIndex = index;
}

/*!
  Sets the detail for \a node. \a node must be in the picked
  path, of course. Set to NULL if you want to remove a detail
  for a node.
*/
void
SoPickedPoint::setDetail(SoDetail *detail, SoNode *node)
{
  int idx = this->path->findNode(node);
  if (idx >= 0) {
    // do not delete the old detail here. SoDetailList will handle it.
    this->detailList.set(idx, detail);
  }
}

//
// applies a matrix action to the path. Stops at node if != NULL
//
void
SoPickedPoint::applyMatrixAction(const SoNode * const node) const
{
  if (node) {
    SoFullPath *fullpath = (SoFullPath*) this->path;
    int idx = fullpath->findNode(node);
    assert(idx >= 0);
    SoTempPath subpath(idx+1);
    subpath.ref(); // Avoid an internal Coin warning for SoAction::apply().
    for (int i = 0; i <= idx; i++) {
      subpath.append(fullpath->getNode(i));
    }
    this->getMatrixAction()->apply(&subpath);
  }
  else {
    this->getMatrixAction()->apply(this->path);
  }
}

//
// creates or returns a matrix action.
//
SoGetMatrixAction *
SoPickedPoint::getMatrixAction() const
{
  if (matrixAction == NULL) {
    matrixAction = new SoGetMatrixAction(this->viewport);
    coin_atexit((coin_atexit_f *)clean_class, CC_ATEXIT_NORMAL);
  }
  else {
    matrixAction->setViewportRegion(this->viewport);
  }
  return matrixAction;
}
