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

#include "shapenodes/soshape_primdata.h"

#include <cstring>

#include <Inventor/SbTesselator.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoShapeHintsElement.h>

#include "base/SbGLUTessellator.h"

// *************************************************************************

soshape_primdata::soshape_primdata(void)
{
  this->counter = 0;
  this->action = NULL;
  this->shape = NULL;
  this->faceCounter = 0;
  this->arraySize = 4;
  this->vertsArray = new SoPrimitiveVertex[this->arraySize];
  this->pointDetails = new SoPointDetail[this->arraySize];
  this->faceDetail = NULL;
  this->lineDetail = NULL;
  this->matPerFace = FALSE;
  this->normPerFace = FALSE;

  this->tess = NULL;
  this->glutess = NULL;

  if (SbGLUTessellator::preferred()) {
    this->glutess = new SbGLUTessellator(soshape_primdata::tess_callback, this);
  }
  else {
    this->tess = new SbTesselator(soshape_primdata::tess_callback, this);
  }
}

soshape_primdata::~soshape_primdata()
{
  delete[] this->vertsArray;
  delete[] this->pointDetails;
  delete this->tess;
  delete this->glutess;
}

// *************************************************************************

void
soshape_primdata::beginShape(SoShape * shapeptr, SoAction * actionptr,
                               SoShape::TriangleShape shapetypearg,
                               SoDetail * detail)
{
  this->shape = shapeptr;
  this->action = actionptr;
  this->shapetype = shapetypearg;
  this->faceDetail = (detail && detail->isOfType(SoFaceDetail::getClassTypeId())) ? (SoFaceDetail*)detail : NULL;
  this->lineDetail = (detail && detail->isOfType(SoLineDetail::getClassTypeId())) ? (SoLineDetail*)detail : NULL;
  this->counter = 0;

  SoState * state = action->getState();

  SoMaterialBindingElement::Binding mbind = SoMaterialBindingElement::get(state);
  SoNormalBindingElement::Binding nbind = SoNormalBindingElement::get(state);

  // need to test for PER_FACE bindings since special rules need to followed 
  // to get the correct per vertex material and normal indices in these cases
  // (basically the same rules as when sending geometry to OpenGL)
  this->matPerFace = 
    (mbind == SoMaterialBindingElement::PER_FACE) ||
    (mbind == SoMaterialBindingElement::PER_FACE_INDEXED);
  this->normPerFace = 
    (nbind == SoNormalBindingElement::PER_FACE) ||
    (nbind == SoNormalBindingElement::PER_FACE_INDEXED);
    
}

void
soshape_primdata::endShape(void)
{
  if (this->shapetype == SoShape::POLYGON) {
    this->handleFaceDetail(this->counter);

    if (SoShapeHintsElement::getFaceType(action->getState()) ==
        SoShapeHintsElement::CONVEX) {
      for (int i = 1; i < this->counter-1; i++) {
        this->shape->invokeTriangleCallbacks(this->action,
                                             &vertsArray[0],
                                             &vertsArray[i],
                                             &vertsArray[i+1]);
      }
    }
    else {
      if (SbGLUTessellator::preferred()) {
        this->glutess->beginPolygon();
        for (int i = 0; i < counter; i++) {
          this->glutess->addVertex(vertsArray[i].getPoint(), &vertsArray[i]);
        }
        this->glutess->endPolygon();
      }
      else {
        // FIXME: the keepVertices==TRUE setting may not be necessary,
        // according to pederb. (The flag causes us to get callbacks
        // even on empty polygons -- probably not useful in Coin, as
        // it was for Rational Reducer.)  20060216 mortene.
        this->tess->beginPolygon(TRUE);
        for (int i = 0; i < counter; i++) {
          this->tess->addVertex(vertsArray[i].getPoint(), &vertsArray[i]);
        }
        this->tess->endPolygon();
      }
    }
  }
}

void
soshape_primdata::shapeVertex(const SoPrimitiveVertex * const v)
{
  switch (shapetype) {
  case SoShape::TRIANGLE_STRIP:
    if (this->counter >= 3) {
      if (this->counter & 1) this->copyVertex(2, 0);
      else this->copyVertex(2, 1);
    }
    this->setVertex(SbMin(this->counter, 2), v);
    this->counter++;
    if (this->counter >= 3) {
      this->handleFaceDetail(3);
      this->shape->invokeTriangleCallbacks(this->action,
                                           &vertsArray[0],
                                           &vertsArray[1],
                                           &vertsArray[2]);
    }
    break;
  case SoShape::TRIANGLE_FAN:
    if (this->counter == 3) {
      this->copyVertex(2, 1);
      this->setVertex(2, v);
    }
    else {
      this->setVertex(this->counter++, v);
    }
    if (this->counter == 3) {
      this->handleFaceDetail(3);
      this->shape->invokeTriangleCallbacks(this->action,
                                           &vertsArray[0],
                                           &vertsArray[1],
                                           &vertsArray[2]);
    }
    break;
  case SoShape::TRIANGLES:
    this->setVertex(counter++, v);
    if (this->counter == 3) {
      this->handleFaceDetail(3);
      this->shape->invokeTriangleCallbacks(this->action,
                                           &vertsArray[0],
                                           &vertsArray[1],
                                           &vertsArray[2]);
      this->counter = 0;
    }
    break;
  case SoShape::POLYGON:
    if (this->counter >= this->arraySize) {
      this->arraySize <<= 1;
      SoPrimitiveVertex * newArray = new SoPrimitiveVertex[this->arraySize];
      memcpy(newArray, this->vertsArray,
             sizeof(SoPrimitiveVertex)* this->counter);
      delete [] this->vertsArray;
      this->vertsArray = newArray;

      SoPointDetail * newparray = new SoPointDetail[this->arraySize];
      memcpy(newparray, this->pointDetails,
             sizeof(SoPointDetail)* this->counter);
      delete [] this->pointDetails;
      this->pointDetails = newparray;

      if (this->faceDetail) {
        for (int i = 0; i < this->counter; i++) {
          this->vertsArray[i].setDetail(&this->pointDetails[i]);
        }
      }
    }
    this->setVertex(this->counter++, v);
    break;
  case SoShape::QUADS:
    this->setVertex(this->counter++, v);
    if (this->counter == 4) {
      this->handleFaceDetail(4);
      this->shape->invokeTriangleCallbacks(this->action,
                                           &vertsArray[0],
                                           &vertsArray[1],
                                           &vertsArray[2]);
      this->shape->invokeTriangleCallbacks(this->action,
                                           &vertsArray[0],
                                           &vertsArray[2],
                                           &vertsArray[3]);
      this->counter = 0;
    }
    break;
  case SoShape::QUAD_STRIP:
    this->setVertex(this->counter++, v);
    if (counter == 4) {
      if (this->matPerFace) this->copyMaterialIndex(3);
      if (this->normPerFace) this->copyNormalIndex(3);
      // can't use handleFaceDetail(), because of the vertex
      // order.
      if (this->faceDetail) {
        this->faceDetail->setNumPoints(4);
        this->faceDetail->setPoint(0, &this->pointDetails[0]);
        this->vertsArray[0].setDetail(this->faceDetail);
        this->faceDetail->setPoint(1, &this->pointDetails[1]);
        this->vertsArray[1].setDetail(this->faceDetail);
        this->faceDetail->setPoint(2, &this->pointDetails[3]);
        this->vertsArray[2].setDetail(this->faceDetail);
        this->faceDetail->setPoint(3, &this->pointDetails[2]);
        this->vertsArray[3].setDetail(this->faceDetail);
      }
      this->shape->invokeTriangleCallbacks(this->action,
                                           &vertsArray[0],
                                           &vertsArray[1],
                                           &vertsArray[3]);
      this->shape->invokeTriangleCallbacks(this->action,
                                           &vertsArray[0],
                                           &vertsArray[3],
                                           &vertsArray[2]);
      this->copyVertex(2, 0);
      this->copyVertex(3, 1);
      this->counter = 2;
    }
    break;
  case SoShape::POINTS:
    this->shape->invokePointCallbacks(this->action, v);
    break;
  case SoShape::LINES:
    this->setVertex(this->counter++, v);
    if (this->counter == 2) {
      this->handleLineDetail();
      this->shape->invokeLineSegmentCallbacks(this->action,
                                              &vertsArray[0],
                                              &vertsArray[1]);
      this->counter = 0;
    }
    break;
  case SoShape::LINE_STRIP:
    this->setVertex(this->counter++, v);
    if (this->counter == 2) {
      this->handleLineDetail();
      this->shape->invokeLineSegmentCallbacks(this->action,
                                              &vertsArray[0],
                                              &vertsArray[1]);
      this->copyVertex(1, 0);
      this->counter = 1;
    }
    break;
  default:
    assert(0 && "Unknown shape type");
  }
}

void
soshape_primdata::copyVertex(const int src, const int dest)
{
  this->vertsArray[dest] = this->vertsArray[src];
  if (this->faceDetail) {
    this->pointDetails[dest] = this->pointDetails[src];
    this->vertsArray[dest].setDetail(&this->pointDetails[dest]);
  }
}

void
soshape_primdata::setVertex(const int idx, const SoPrimitiveVertex * const v)
{
  this->vertsArray[idx] = *v;
  if (this->faceDetail || this->lineDetail) {
    SoPointDetail * pd = (SoPointDetail *)v->getDetail();
    assert(pd);
    this->pointDetails[idx] = * pd;
    this->vertsArray[idx].setDetail(&this->pointDetails[idx]);
    }
}

void
soshape_primdata::handleFaceDetail(const int numv)
{
  // if PER_FACE binding, copy indices from the last vertex we
  // received to the other vertices
  if (this->matPerFace) this->copyMaterialIndex(numv-1);
  if (this->normPerFace) this->copyNormalIndex(numv-1);

  if (this->faceDetail) {
    this->faceDetail->setNumPoints(numv);
    for (int i = 0; i < numv; i++) {
      this->faceDetail->setPoint(i, &this->pointDetails[i]);
        this->vertsArray[i].setDetail(this->faceDetail);
    }
  }
}

void
soshape_primdata::handleLineDetail(void)
{
  if (this->lineDetail) {
    this->lineDetail->setPoint0(&this->pointDetails[0]);
    this->lineDetail->setPoint1(&this->pointDetails[1]);
    this->vertsArray[0].setDetail(this->lineDetail);
    this->vertsArray[1].setDetail(this->lineDetail);
  }
}

int 
soshape_primdata::getPointDetailIndex(const SoPrimitiveVertex * v) const
{
  const ptrdiff_t d = v - this->vertsArray;
  return (int)d;
}

SoDetail *
soshape_primdata::createPickDetail(void)
{
  switch (this->shapetype) {
  case SoShape::TRIANGLE_STRIP:
  case SoShape::TRIANGLE_FAN:
  case SoShape::TRIANGLES:
    {
      SoFaceDetail * detail = (SoFaceDetail *)this->faceDetail->copy();
      detail->setNumPoints(3);
      detail->setPoint(0, &this->pointDetails[0]);
      detail->setPoint(1, &this->pointDetails[1]);
      detail->setPoint(2, &this->pointDetails[2]);
      return detail;
    }
  case SoShape::POLYGON:
    {
      SoFaceDetail * detail = (SoFaceDetail *)this->faceDetail->copy();
      detail->setNumPoints(this->counter);
      for (int i = 0; i < this->counter; i++) {
        detail->setPoint(i, &this->pointDetails[i]);
      }
      return detail;
    }
  case SoShape::QUADS:
  case SoShape::QUAD_STRIP:
    {
      SoFaceDetail * detail = (SoFaceDetail *)this->faceDetail->copy();
      detail->setNumPoints(4);
      detail->setPoint(0, &this->pointDetails[0]);
      detail->setPoint(1, &this->pointDetails[1]);
      detail->setPoint(2, &this->pointDetails[2]);
      detail->setPoint(3, &this->pointDetails[3]);
      return detail;
    }
  case SoShape::POINTS:
    {
      assert(0 && "should not get here");
      return NULL;
    }
  case SoShape::LINES:
  case SoShape::LINE_STRIP:
    {
      SoLineDetail * detail = (SoLineDetail *)this->lineDetail->copy();
      detail->setPoint0(&this->pointDetails[0]);
      detail->setPoint1(&this->pointDetails[1]);
      return detail;
    }
  default:
    assert(0 && "unknown shape type");
    return NULL;
  }
}

void
soshape_primdata::tess_callback(void * v0, void * v1, void * v2, void * data)
{
  soshape_primdata * thisp = (soshape_primdata *) data;
  thisp->shape->invokeTriangleCallbacks(thisp->action,
                                        (SoPrimitiveVertex *)v0,
                                        (SoPrimitiveVertex *)v1,
                                        (SoPrimitiveVertex *)v2);
}

void 
soshape_primdata::copyMaterialIndex(const int lastvertex)
{
  int i;
  int matidx = this->vertsArray[lastvertex].getMaterialIndex();
  for (i = 0; i < lastvertex; i++) {
    this->vertsArray[i].setMaterialIndex(matidx);
    this->pointDetails[i].setMaterialIndex(matidx);
  } 
}

void 
soshape_primdata::copyNormalIndex(const int lastvertex)
{
  int i;
  int normidx = this->pointDetails[lastvertex].getNormalIndex();
  for (i = 0; i < lastvertex; i++) {
    this->pointDetails[i].setNormalIndex(normidx);
  } 
}
