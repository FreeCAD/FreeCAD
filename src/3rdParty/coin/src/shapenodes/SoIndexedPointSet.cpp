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
  \class SoIndexedPointSet SoIndexedPointSet.h Inventor/nodes/SoIndexedPointSet.h
  \brief The SoIndexedPointSet class is used to display a set of 3D points.

  \ingroup coin_nodes

  This node either uses the coordinates currently on the state
  (typically set up by a leading SoCoordinate3 node in the scene graph)
  or from a SoVertexProperty node attached to this node to render a
  set of 3D points.

  Here's a simple usage example of SoIndexedPointSet in a scene graph:

  \verbatim
  #Inventor V2.1 ascii

  Separator {
     Material {
        diffuseColor [
         1 0 0, 0 1 0, 0 0 1, 1 1 0, 1 0 1, 1 1 1, 1 0.8 0.6, 0.6 0.8 1
        ]
     }
     MaterialBinding { value PER_VERTEX_INDEXED }

     Normal {
        vector [
         0 0 1, 1 0 0
        ]
     }
     NormalBinding { value PER_VERTEX_INDEXED }

     Coordinate3 {
        point [
         -1 1 0, -1 -1 0, 1 -1 0, 1 1 0, 0 2 -1, -2 0 -1, 0 -2 -1, 2 0 -1
        ]
     }

     DrawStyle { pointSize 5 }

     IndexedPointSet {
        coordIndex [0, 1, 2, 3, 4, 5, 6, 7]
        normalIndex [0, 1, 0, 1, 0, 1, 0, 1]
     }
  }

  \endverbatim

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
  IndexedPointSet {
    vertexProperty        NULL
    coordIndex        0
    materialIndex        -1
    normalIndex        -1
    textureCoordIndex        -1
  }
  \endcode

  \since TGS Inventor 6.0, Coin 3.1
*/

#include <Inventor/nodes/SoIndexedPointSet.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/misc/SoState.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/system/gl.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoTextureCoordinateBindingElement.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/misc/SoGLDriverDatabase.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "nodes/SoSubNodeP.h"
#include "rendering/SoGL.h"
#include "rendering/SoVBO.h"
#include "rendering/SoVertexArrayIndexer.h"

#define LOCK_VAINDEXER(obj) SoBase::staticDataLock()
#define UNLOCK_VAINDEXER(obj) SoBase::staticDataUnlock()

SO_NODE_SOURCE(SoIndexedPointSet);

/*!
  Constructor.
*/
SoIndexedPointSet::SoIndexedPointSet()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoIndexedPointSet);
  this->vaindexer = NULL;
}

/*!
  Destructor.
*/
SoIndexedPointSet::~SoIndexedPointSet()
{
  delete this->vaindexer;
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoIndexedPointSet::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoIndexedPointSet, SO_FROM_INVENTOR_6_0);
}

/*
  Convenience function for fetching the material binding.
  Converts PER_PART* and PER_FACE* to PER_VERTEX*
  Defaults to OVERALL.
*/
SoIndexedPointSet::Binding
SoIndexedPointSet::findMaterialBinding(SoState * const state) const
{
  Binding binding;
  switch (SoMaterialBindingElement::get(state)){
    case SoMaterialBindingElement::PER_PART:
    case SoMaterialBindingElement::PER_FACE:
    case SoMaterialBindingElement::PER_VERTEX:
      binding = PER_VERTEX;
      break;
    case SoMaterialBindingElement::PER_PART_INDEXED:
    case SoMaterialBindingElement::PER_FACE_INDEXED:
    case SoMaterialBindingElement::PER_VERTEX_INDEXED:
      binding = PER_VERTEX_INDEXED;
      break;
    default:
      binding = OVERALL;
  }
  return binding;
}

/*
  Convenience function for fetching the normal binding.
  Converts PER_PART* and PER_FACE* to PER_VERTEX*
  Defaults to OVERALL.
*/
SoIndexedPointSet::Binding
SoIndexedPointSet::findNormalBinding(SoState * const state) const

{
  Binding binding;
  switch (SoNormalBindingElement::get(state)){
    case SoNormalBindingElement::PER_PART:
    case SoNormalBindingElement::PER_FACE:
    case SoNormalBindingElement::PER_VERTEX:
      binding = PER_VERTEX;
      break;
    case SoNormalBindingElement::PER_PART_INDEXED:
    case SoNormalBindingElement::PER_FACE_INDEXED:
    case SoNormalBindingElement::PER_VERTEX_INDEXED:
      binding = PER_VERTEX_INDEXED;
      break;
    default:
      binding = OVERALL;
  }
  return binding;
}

/*
  Convenience function for fetching the texture coordinate binding.
*/
SoIndexedPointSet::Binding
SoIndexedPointSet::findTextureBinding(SoState * const state) const

{
  Binding binding;
  switch (SoTextureCoordinateBindingElement::get(state)){
    case SoTextureCoordinateBindingElement::PER_VERTEX:
      binding = PER_VERTEX;
      break;
    default:
      binding = PER_VERTEX_INDEXED;
  }
  return binding;
}

// doc from parent
void
SoIndexedPointSet::GLRender(SoGLRenderAction * action)
{
  int32_t numpts = this->coordIndex.getNum();
  if (numpts == 0) return;

  SoState * state = action->getState();

  SbBool didpush = FALSE;
  if (this->vertexProperty.getValue()) {
    state->push();
    didpush = TRUE;
    this->vertexProperty.getValue()->GLRender(action);
  }

  if (!this->shouldGLRender(action)){
    if (didpush) state->pop();
    return;
  }

  SoMaterialBundle mb(action);
  SoTextureCoordinateBundle tb(action, TRUE, FALSE);

  const SbVec3f * normals;
  int numindices;
  const int32_t * cindices;
  const int32_t * nindices;
  const int32_t * tindices;
  const int32_t * mindices;
  SbBool doTextures;
  SbBool needNormals;
  const SoCoordinateElement * coords;
  Binding nbind, tbind, mbind;

  SbBool normalCacheUsed;

  doTextures = tb.needCoordinates();
  needNormals = !mb.isColorOnly() || tb.isFunction();

  this->getVertexData(state, coords, normals, cindices,
                      nindices, tindices, mindices, numindices,
                      needNormals, normalCacheUsed);

  if (numindices == 0){
    if (didpush) state->pop();
    return;
  }

  if (normals == NULL && needNormals) {
    needNormals = FALSE;
    if (!didpush) {
      state->push();
      didpush = TRUE;
    }
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
  }

  nbind = OVERALL;
  if (needNormals){
    nbind = this->findNormalBinding(state);
  }
  //if we don't have explicit normal indices, use coord indices:
  if (nbind == PER_VERTEX_INDEXED && nindices == NULL) nindices = cindices;

  mbind = this->findMaterialBinding(state);
  //if we don't have explicit material indices, use coord indices:
  if (mbind == PER_VERTEX_INDEXED && mindices == NULL) mindices = cindices;

  tbind = OVERALL;
  if (doTextures) {
    tbind = this->findTextureBinding(state);
    if (tb.isFunction() && !tb.needIndices()) {
      tbind = OVERALL;
      tindices = NULL;//don't send texture coords
    }
    else if (tbind == PER_VERTEX){
      tindices = NULL;//texture coords are sent per vertex
    }
    else {//tbind == PER_VERTEX_INDEXED
      //if we don't have explicit texture coord indices, use coord indices:
      if (tindices == NULL) tindices = cindices;
    }
  }

  const SoGLCoordinateElement * glcoords = dynamic_cast<const SoGLCoordinateElement *>(coords);
  assert(glcoords && "could not cast to SoGLCoordinateElement");

  if (nbind == OVERALL && needNormals) {
    glNormal3fv((const GLfloat *)normals);
  }

  mb.sendFirst(); // always do this, even if mbind != OVERALL

  const cc_glglue * glue = sogl_glue_instance(state);
  const uint32_t contextid = action->getCacheContext();

  SbBool dova =
    SoVBO::shouldRenderAsVertexArrays(state, contextid, numindices) &&
    SoGLDriverDatabase::isSupported(glue, SO_GL_VERTEX_ARRAY) &&
    ((nbind == OVERALL) || ((nbind == PER_VERTEX_INDEXED) && (nindices == cindices))) &&
    ((tbind == OVERALL) || ((tbind == PER_VERTEX_INDEXED) && (tindices == cindices))) &&
    ((mbind == OVERALL) || ((mbind == PER_VERTEX_INDEXED) && (mindices == cindices)));

  if (dova && (mbind == PER_VERTEX_INDEXED)) {
    const SoGLVBOElement * vboelem = SoGLVBOElement::getInstance(state);
    if (vboelem->getColorVBO() == NULL) {
      dova = FALSE;
      // we might be able to do VA-rendering, but need to check the
      // diffuse color type first.
      SoGLLazyElement * lelem = (SoGLLazyElement*) SoLazyElement::getInstance(state);
      if (!lelem->isPacked() && lelem->getNumTransparencies() <= 1) {
        dova = TRUE;
      }
    }
  }
  SbBool didrenderasvbo = FALSE;
  if (dova) {
    SbBool vbo = this->startVertexArray(action,
                                        glcoords,
                                        (needNormals && (nbind == PER_VERTEX_INDEXED)) ? normals : NULL,
                                        doTextures,
                                        mbind == PER_VERTEX_INDEXED);
    didrenderasvbo = vbo;
    LOCK_VAINDEXER(this);
    if (this->vaindexer == NULL) {
      SoVertexArrayIndexer * indexer = new SoVertexArrayIndexer;
      for (int i = 0; i < numindices; i++) {
        int32_t idx = this->coordIndex[i];
        indexer->addPoint(idx);
      }
      indexer->close();
      if (indexer->getNumVertices()) {
        this->vaindexer = indexer;
      }
      else {
        delete indexer;
      }
    }

    if (this->vaindexer) {
      this->vaindexer->render(state, vbo, contextid);
    }
    UNLOCK_VAINDEXER(this);
    this->finishVertexArray(action, vbo,
                            (needNormals && (nbind == PER_VERTEX_INDEXED)),
                            doTextures,
                            mbind == PER_VERTEX_INDEXED);
  }
  else {//no vertex array rendering
    glBegin(GL_POINTS);
    SbVec3f currnormal = normals ? normals[0] : SbVec3f(0, 0, 1);
    for (int i = 0; i < numindices; i++) {
      int32_t idx = cindices[i];
      if (idx < 0) continue;

      if (mbind == PER_VERTEX_INDEXED) mb.send(mindices[i], TRUE);
      else if (mbind == PER_VERTEX) mb.send(i, TRUE);

      if (nbind == PER_VERTEX_INDEXED) currnormal = normals[nindices[i]];
      else if (nbind == PER_VERTEX) currnormal = normals[i];

      if (needNormals && nbind != OVERALL){
        const GLfloat * ptr = reinterpret_cast<const GLfloat*>(&currnormal);
        glNormal3fv(ptr);
      }

      if (doTextures){
        if (tbind == PER_VERTEX_INDEXED){
          tb.send(tindices[i], glcoords->get3(idx), currnormal);
        }
        else if (tbind == PER_VERTEX){
          tb.send(i, glcoords->get3(idx), currnormal);
        }
      }

      glcoords->send(idx);
    }
    glEnd();
  }
  if (didpush)
    state->pop();

  // send approx number of points for autocache handling. Divide
  // by three so that three points is the same as one triangle.
  sogl_autocache_update(state, numindices/3, didrenderasvbo);
}

// Documented in superclass.
SbBool
SoIndexedPointSet::generateDefaultNormals(SoState *, SoNormalCache * nc)
{
  // Overridden to clear normal cache, as it's not possible to
  // generate a normal for a point.
  nc->set(0, NULL);
  return TRUE;
}

// Documented in superclass.
SbBool
SoIndexedPointSet::generateDefaultNormals(SoState * COIN_UNUSED_ARG(state), SoNormalBundle * COIN_UNUSED_ARG(bundle))
{
  // Overridden to avoid (faulty) compiler warnings with some version
  // of g++.
  return FALSE;
}

// doc from parent
void
SoIndexedPointSet::getBoundingBox(SoGetBoundingBoxAction *action)
{
  inherited::getBoundingBox(action);
}

// doc from parent
void
SoIndexedPointSet::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  if (!this->shouldPrimitiveCount(action)) return;
  int num = this->coordIndex.getNum();
  action->addNumPoints(num);
}

// doc from parent
void
SoIndexedPointSet::generatePrimitives(SoAction *action)
{
  int32_t numpts = this->coordIndex.getNum();
  if (numpts == 0) return;

  SoState * state = action->getState();

  if (this->vertexProperty.getValue()) {
    state->push();
    this->vertexProperty.getValue()->doAction(action);
  }

  SoTextureCoordinateBundle tb(action, FALSE, FALSE);

  /*
    FIXME: the following code is almost identical to that in glRender.
    Consider refactoring. wiesener 20090317
  */
  const SbVec3f * normals;
  int numindices;
  const int32_t * cindices;
  const int32_t * nindices;
  const int32_t * tindices;
  const int32_t * mindices;
  SbBool doTextures;
  SbBool needNormals;
  const SoCoordinateElement * coords;
  Binding nbind, tbind, mbind;

  SbBool normalCacheUsed;

  doTextures = tb.needCoordinates();
  needNormals = TRUE;

  this->getVertexData(state, coords, normals, cindices,
                      nindices, tindices, mindices, numindices,
                      needNormals, normalCacheUsed);

  if (numindices == 0){
    if (this->vertexProperty.getValue()) state->pop();
    return;
  }

  if (normals == NULL) {
    needNormals = FALSE;
  }

  nbind = OVERALL;
  if (needNormals){
    nbind = this->findNormalBinding(state);
  }
  //if we don't have explicit normal indices, use coord indices:
  if (nbind == PER_VERTEX_INDEXED && nindices == NULL) nindices = cindices;

  mbind = this->findMaterialBinding(state);
  //if we don't have explicit material indices, use coord indices:
  if (mbind == PER_VERTEX_INDEXED && mindices == NULL) mindices = cindices;

  tbind = OVERALL;
  if (doTextures) {
    tbind = this->findTextureBinding(state);
    if (tb.isFunction() && !tb.needIndices()) {
      tbind = OVERALL;
      tindices = NULL;//don't send texture coords
    }
    else if (tbind == PER_VERTEX){
      tindices = NULL;//texture coords are sent per vertex
    }
    else {//tbind == PER_VERTEX_INDEXED
      //if we don't have explicit texture coord indices, use coord indices:
      if (tindices == NULL) tindices = cindices;
    }
  }

  SoPrimitiveVertex vertex;
  SoPointDetail pointDetail;
  vertex.setDetail(&pointDetail);

  this->beginShape(action, SoShape::POINTS);
  SbVec3f currnormal = normals ? normals[0] : SbVec3f(0, 0, 1);
  for (int i = 0; i < numindices; i++) {
    int32_t idx = cindices[i];
    if (idx < 0) continue;

    if (mbind == PER_VERTEX_INDEXED){
      pointDetail.setMaterialIndex(mindices[i]);
      vertex.setMaterialIndex(mindices[i]);
    }
    else if (mbind == PER_VERTEX){
      pointDetail.setMaterialIndex(i);
      vertex.setMaterialIndex(i);
    }

    int32_t nindex = i;
    if (nbind == PER_VERTEX_INDEXED) nindex = nindices[i];
    if (nbind != OVERALL) currnormal = normals[nindex];
    if (needNormals){
      pointDetail.setNormalIndex(nindex);
      vertex.setNormal(currnormal);
    }

    if (doTextures) {
      if (tb.isFunction()) {
        vertex.setTextureCoords(tb.get(coords->get3(idx), currnormal));
      }
      else {
        int32_t tindex = i;
        if (tbind == PER_VERTEX_INDEXED) tindex = tindices[i];
        pointDetail.setTextureCoordIndex(tindex);
        vertex.setTextureCoords(tb.get(tindex));
      }
    }
    pointDetail.setCoordinateIndex(idx);
    vertex.setPoint(coords->get3(idx));
    this->shapeVertex(&vertex);
  }
  this->endShape();

  if (this->vertexProperty.getValue())
    state->pop();
}

// Documented in superclass.
void
SoIndexedPointSet::notify(SoNotList * list)
{
  SoField * f = list->getLastField();
  if (f == &this->coordIndex) {
    LOCK_VAINDEXER(this);
    delete this->vaindexer;
    this->vaindexer = NULL;
    UNLOCK_VAINDEXER(this);
  }
  inherited::notify(list);
}

#undef LOCK_VAINDEXER
#undef UNLOCK_VAINDEXER
