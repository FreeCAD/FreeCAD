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
  \class SoIndexedLineSet SoIndexedLineSet.h Inventor/nodes/SoIndexedLineSet.h
  \brief The SoIndexedLineSet class is used to render and otherwise represent indexed lines.

  \ingroup coin_nodes

  The indexed counterpart of SoLineSet. Lines can be specified using
  indices for coordinates, normals, materials and texture coordinates.

  If no normals are supplied on the stack (or in the vertexProperty
  field), the lines will be rendered with lighting disabled.

  For more information about line sets, see documentation in
  SoLineSet.  For more information about indexed shapes, see
  documentation in SoIndexedShape and SoIndexedFaceSet.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    IndexedLineSet {
        vertexProperty NULL
        coordIndex 0
        materialIndex -1
        normalIndex -1
        textureCoordIndex -1
    }
  \endcode

*/

#include <Inventor/nodes/SoIndexedLineSet.h>

#include <cassert>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/system/gl.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoGLShapeHintsElement.h>
#include <Inventor/elements/SoTextureCoordinateBindingElement.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/misc/SoGLDriverDatabase.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "nodes/SoSubNodeP.h"
#include "rendering/SoGL.h"
#include "rendering/SoVertexArrayIndexer.h"
#include "rendering/SoVBO.h"


SO_NODE_SOURCE(SoIndexedLineSet);


#define PRIVATE(obj) obj->pimpl
#define LOCK_VAINDEXER(obj) SoBase::staticDataLock()
#define UNLOCK_VAINDEXER(obj) SoBase::staticDataUnlock()

class SoIndexedLineSetP {
public:
  SoVertexArrayIndexer * vaindexer;
};

/*!
  Constructor.
*/
SoIndexedLineSet::SoIndexedLineSet()
{
  PRIVATE(this) = new SoIndexedLineSetP;
  PRIVATE(this)->vaindexer = NULL;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoIndexedLineSet);
}

/*!
  Destructor.
*/
SoIndexedLineSet::~SoIndexedLineSet()
{
  delete PRIVATE(this)->vaindexer;
  delete PRIVATE(this);
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoIndexedLineSet::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoIndexedLineSet, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

//
// translates current material binding into the internal Binding enum
//
SoIndexedLineSet::Binding
SoIndexedLineSet::findMaterialBinding(SoState * state)
{
  Binding binding = OVERALL;
  SoMaterialBindingElement::Binding matbind =
    (SoMaterialBindingElement::Binding) SoMaterialBindingElement::get(state);

  switch (matbind) {
  case SoMaterialBindingElement::OVERALL:
    binding = OVERALL;
    break;
  case SoMaterialBindingElement::PER_VERTEX:
    binding = PER_VERTEX;
    break;
  case SoMaterialBindingElement::PER_VERTEX_INDEXED:
    binding = PER_VERTEX_INDEXED;
    break;
  case SoMaterialBindingElement::PER_PART:
    binding = PER_SEGMENT;
    break;
  case SoMaterialBindingElement::PER_PART_INDEXED:
    binding = PER_SEGMENT_INDEXED;
    break;
  case SoMaterialBindingElement::PER_FACE:
    binding = PER_LINE;
    break;
  case SoMaterialBindingElement::PER_FACE_INDEXED:
    binding = PER_LINE_INDEXED;
    break;
  default:
#if COIN_DEBUG
    SoDebugError::postWarning("SoIndexedLineSet::findMaterialBinding",
                              "unknown material binding setting");
#endif // COIN_DEBUG
    break;
  }
  return binding;
}


//
// translates current normal binding into the internal Binding enum
//
SoIndexedLineSet::Binding
SoIndexedLineSet::findNormalBinding(SoState* state)
{
  Binding binding = PER_VERTEX_INDEXED;
  SoNormalBindingElement::Binding normbind =
    (SoNormalBindingElement::Binding) SoNormalBindingElement::get(state);

  switch (normbind) {
  case SoNormalBindingElement::OVERALL:
    binding = OVERALL;
    break;
  case SoNormalBindingElement::PER_VERTEX:
    binding = PER_VERTEX;
    break;
  case SoNormalBindingElement::PER_VERTEX_INDEXED:
    binding = PER_VERTEX_INDEXED;
    break;
  case SoNormalBindingElement::PER_PART:
    binding = PER_SEGMENT;
    break;
  case SoNormalBindingElement::PER_PART_INDEXED:
    binding = PER_SEGMENT_INDEXED;
    break;
  case SoNormalBindingElement::PER_FACE:
    binding = PER_LINE;
    break;
  case SoNormalBindingElement::PER_FACE_INDEXED:
    binding = PER_LINE_INDEXED;
    break;
  default:
#if COIN_DEBUG
    SoDebugError::postWarning("SoIndexedLineSet::findNormalBinding",
                              "unknown normal binding setting");
#endif // COIN_DEBUG
    break;
  }
  return binding;
}


// doc from parent
void
SoIndexedLineSet::GLRender(SoGLRenderAction * action)
{
  if (this->coordIndex.getNum() < 2) return;
  SoState * state = action->getState();

  SbBool hasvp = FALSE;

  if (this->vertexProperty.getValue()) {
    hasvp = TRUE;
    state->push();
    this->vertexProperty.getValue()->GLRender(action);
  }

  if (!this->shouldGLRender(action)) {
    if (hasvp)
      state->pop();
    return;
  }

  Binding mbind = this->findMaterialBinding(state);
  Binding nbind = this->findNormalBinding(state);

  const SoCoordinateElement * coords;
  const SbVec3f * normals;
  const int32_t * cindices;
  int numindices;
  const int32_t * nindices;
  const int32_t * tindices;
  const int32_t * mindices;
  SbBool doTextures;
  SbBool normalCacheUsed;

  SoMaterialBundle mb(action);

  SoTextureCoordinateBundle tb(action, TRUE, FALSE);
  doTextures = tb.needCoordinates();
  SbBool sendNormals = !mb.isColorOnly() || tb.isFunction();

  this->getVertexData(state, coords, normals, cindices,
                      nindices, tindices, mindices, numindices,
                      sendNormals, normalCacheUsed);

  if (sendNormals && normals == NULL) {
    if (!hasvp) {
      state->push();
      hasvp = TRUE;
    }
    sendNormals = FALSE;
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
  }

  if (this->getNodeType() == SoNode::VRML1) {
    // For VRML1, PER_VERTEX means per vertex in shape, not PER_VERTEX
    // on the state.
    if (mbind == PER_VERTEX) {
      mbind = PER_VERTEX_INDEXED;
      mindices = cindices;
    }
    if (nbind == PER_VERTEX) {
      nbind = PER_VERTEX_INDEXED;
      nindices = cindices;
    }
  }

  if (doTextures) {
    if (SoTextureCoordinateBindingElement::get(state) ==
        SoTextureCoordinateBindingElement::PER_VERTEX) {
      tindices = NULL; // just in case
    }
    else if (tindices == NULL) {
      tindices = cindices;
    }
  }

  mb.sendFirst(); // make sure we have the correct material

  if (!sendNormals) nbind = OVERALL;
  else if (nbind == OVERALL) {
    glNormal3fv((const GLfloat *)normals);
  }

  SbBool drawPoints =
    SoDrawStyleElement::get(state) == SoDrawStyleElement::POINTS;

  const uint32_t contextid = action->getCacheContext();
  SoGLLazyElement * lelem = NULL;

  SbBool dova =
    !drawPoints &&
    SoVBO::shouldRenderAsVertexArrays(state, contextid, numindices) &&
    ((nbind == OVERALL) || ((nbind == PER_VERTEX_INDEXED) && ((nindices == cindices) || (nindices == NULL)))) &&
    (!doTextures || (tindices == cindices)) &&
    ((mbind == OVERALL) || ((mbind == PER_VERTEX_INDEXED) && ((mindices == cindices) || (mindices == NULL)))) &&
    SoGLDriverDatabase::isSupported(sogl_glue_instance(state), SO_GL_VERTEX_ARRAY);

  const SoGLVBOElement * vboelem = SoGLVBOElement::getInstance(state);
  SoVBO * colorvbo = NULL;

  SbBool didrenderasvbo = FALSE;
  if (dova && (mbind != OVERALL)) {
    dova = FALSE;
    if ((mbind == PER_VERTEX_INDEXED) && ((mindices == cindices) || (mindices == NULL))) {
      lelem = (SoGLLazyElement*) SoLazyElement::getInstance(state);
      colorvbo = vboelem->getColorVBO();
      if (colorvbo) dova = TRUE;
      else {
        // we might be able to do VA-rendering, but need to check the
        // diffuse color type first.
        if (!lelem->isPacked() && lelem->getNumTransparencies() <= 1) {
          dova = TRUE;
        }
      }
    }
  }
  if (dova) {
    SbBool dovbo = this->startVertexArray(action,
                                          coords,
                                          nbind != OVERALL ? normals : NULL,
                                          doTextures,
                                          mbind != OVERALL);
    didrenderasvbo = dovbo;

    LOCK_VAINDEXER(this);
    if (PRIVATE(this)->vaindexer == NULL) {
      SoVertexArrayIndexer * indexer = new SoVertexArrayIndexer;
      int i = 0;
      while (i < numindices) {
        int cnt = 0;
        while (i + cnt < numindices && cindices[i+cnt] >= 0) cnt++;
        if (cnt >= 2) {
          for (int j = 1; j < cnt;j++) {
            indexer->addLine(cindices[i+j-1],
                             cindices[i+j]);
          }
        }
        i += cnt + 1;
      }
      indexer->close();
      if (indexer->getNumVertices()) {
        PRIVATE(this)->vaindexer = indexer;
      }
      else {
        delete indexer;
      }
#if 0
      fprintf(stderr,"XXX: ILS create VertexArrayIndexer: %d\n", indexer->getNumVertices());
#endif
    }

    if (PRIVATE(this)->vaindexer) {
      PRIVATE(this)->vaindexer->render(state, dovbo, contextid);
    }
    UNLOCK_VAINDEXER(this);

    this->finishVertexArray(action,
                            dovbo,
                            nbind != OVERALL,
                            doTextures,
                            mbind != OVERALL);
  }
  else {
    sogl_render_lineset((SoGLCoordinateElement*)coords,
                        cindices,
                        numindices,
                        normals,
                        nindices,
                        &mb,
                        mindices,
                        &tb,
                        tindices,
                        (int)nbind,
                        (int)mbind,
                        doTextures ? 1 : 0,
                        drawPoints ? 1 : 0);
  }

  if (hasvp) {
    state->pop();
  }
  // send approx number of lines for autocache handling
  sogl_autocache_update(state, this->coordIndex.getNum() / 2, didrenderasvbo);
}

// Documented in superclass.
SbBool
SoIndexedLineSet::generateDefaultNormals(SoState *, SoNormalBundle *)
{
  return FALSE;
}

// Documented in superclass.
SbBool
SoIndexedLineSet::generateDefaultNormals(SoState * COIN_UNUSED_ARG(state), SoNormalCache * nc)
{
  // not possible to generate normals for IndexedLineSet
  nc->set(0, NULL);
  return TRUE;
}

// doc from parent
void
SoIndexedLineSet::getBoundingBox(SoGetBoundingBoxAction * action)
{
  inherited::getBoundingBox(action);
  // notify open (if any) bbox caches about lines in this shape
  SoBoundingBoxCache::setHasLinesOrPoints(action->getState());
}

// doc from parent
void
SoIndexedLineSet::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  int n = this->coordIndex.getNum();
  if (n < 2) return;

  if (action->canApproximateCount()) {
    action->addNumLines(n/3);
  }
  else {
    const int32_t * ptr = coordIndex.getValues(0);
    const int32_t * endptr = ptr + n;
    int cnt = 0;
    int add = 0;
    while (ptr < endptr) {
      if (*ptr++ >= 0) cnt++;
      else {
        add += cnt-1;
        cnt = 0;
      }
    }
    // in case index array wasn't terminated by a -1
    if (cnt >= 2) add += cnt-1;
    action->addNumLines(add);
  }
}

// doc from parent
void
SoIndexedLineSet::generatePrimitives(SoAction *action)
{
  if (this->coordIndex.getNum() < 2) return;

  SoState * state = action->getState();

  if (this->vertexProperty.getValue()) {
    state->push();
    this->vertexProperty.getValue()->doAction(action);
  }

  Binding mbind = this->findMaterialBinding(state);
  Binding nbind = this->findNormalBinding(state);

  const SoCoordinateElement * coords;
  const SbVec3f * normals;
  const int32_t * cindices;
  int numindices;
  const int32_t * normindices;
  const int32_t * texindices;
  const int32_t * matindices;
  SbBool doTextures;
  SbBool sendNormals = TRUE;
  SbBool normalCacheUsed;

  getVertexData(state, coords, normals, cindices,
                normindices, texindices, matindices, numindices,
                sendNormals, normalCacheUsed);

  if (normals == NULL) {
    sendNormals = FALSE;
    nbind = OVERALL;
  }

  if (this->getNodeType() == SoNode::VRML1) {
    // For VRML1, PER_VERTEX means per vertex in shape, not PER_VERTEX
    // on the state.
    if (mbind == PER_VERTEX) {
      mbind = PER_VERTEX_INDEXED;
      matindices = cindices;
    }
    if (nbind == PER_VERTEX) {
      nbind = PER_VERTEX_INDEXED;
      normindices = cindices;
    }
  }

  SoTextureCoordinateBundle tb(action, FALSE, FALSE);
  doTextures = tb.needCoordinates();

  if (doTextures) {
    if (SoTextureCoordinateBindingElement::get(state) ==
        SoTextureCoordinateBindingElement::PER_VERTEX) {
      texindices = NULL; // just in case
    }
    else if (texindices == NULL) {
      texindices = cindices;
    }
  }

  if (mbind == PER_VERTEX_INDEXED && matindices == NULL) {
    matindices = cindices;
  }
  if (nbind == PER_VERTEX_INDEXED && normindices == NULL) {
    normindices = cindices;
  }
  if (mbind == PER_VERTEX || mbind == PER_LINE || mbind == PER_SEGMENT) {
    matindices = NULL;
  }
  if (nbind == PER_VERTEX || nbind == PER_LINE || nbind == PER_SEGMENT) {
    normindices = NULL;
  }

  if (nbind == OVERALL) normindices = NULL;
  if (mbind == OVERALL) matindices = NULL;

  int matnr = 0;
  int normnr = 0;
  int texidx = 0;
  int32_t i;
  const int32_t *end = cindices + numindices;

  SoPrimitiveVertex vertex;
  SoPointDetail pointDetail;
  SoLineDetail lineDetail;

  vertex.setDetail(&pointDetail);

  SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
  const SbVec3f *currnormal = &dummynormal;
  if (normals) currnormal = normals;

  if (nbind == OVERALL) {
    vertex.setNormal(*currnormal);
  }

  if (mbind == PER_SEGMENT || mbind == PER_SEGMENT_INDEXED ||
      nbind == PER_SEGMENT || nbind == PER_SEGMENT_INDEXED) {
    int previ;
    SbBool matPerPolyline = mbind == PER_LINE || mbind == PER_LINE_INDEXED;
    SbBool normPerPolyline = nbind == PER_LINE || nbind == PER_LINE_INDEXED;

    this->beginShape(action, SoShape::LINES, &lineDetail);

    while (cindices + 1 < end) { // need at least two vertices
      previ = *cindices++;

      if (matPerPolyline || mbind >= PER_VERTEX) {
        if (matindices) vertex.setMaterialIndex(*matindices++);
        else vertex.setMaterialIndex(matnr++);
        pointDetail.setMaterialIndex(vertex.getMaterialIndex());
      }
      if (normPerPolyline || nbind >= PER_VERTEX) {
        if (normindices) {
          pointDetail.setNormalIndex(*normindices);
          currnormal = &normals[*normindices++];
        }
        else {
          pointDetail.setNormalIndex(normnr);
          currnormal = &normals[normnr++];
        }
        vertex.setNormal(*currnormal);
      }
      if (doTextures) {
        if (tb.isFunction()) {
          vertex.setTextureCoords(tb.get(coords->get3(previ), *currnormal));
        }
        else {
          pointDetail.setTextureCoordIndex(texindices?*texindices:texidx);
          vertex.setTextureCoords(tb.get(texindices?*texindices++:texidx++));
        }
      }

      while (cindices < end && (i = *cindices++) >= 0) {
        if (mbind == PER_SEGMENT || mbind == PER_SEGMENT_INDEXED) {
          if (matindices) vertex.setMaterialIndex(*matindices++);
          else vertex.setMaterialIndex(matnr++);
          pointDetail.setMaterialIndex(vertex.getMaterialIndex());
        }
        if (nbind == PER_SEGMENT || nbind == PER_SEGMENT_INDEXED) {
          if (normindices) {
            pointDetail.setNormalIndex(*normindices);
            currnormal = &normals[*normindices++];
          }
          else {
            pointDetail.setNormalIndex(normnr);
            currnormal = &normals[normnr++];
          }
          vertex.setNormal(*currnormal);
        }
        pointDetail.setCoordinateIndex(previ);
        vertex.setPoint(coords->get3(previ));
        this->shapeVertex(&vertex);

        if (mbind >= PER_VERTEX) {
          if (matindices) vertex.setMaterialIndex(*matindices++);
          else vertex.setMaterialIndex(matnr++);
          pointDetail.setMaterialIndex(vertex.getMaterialIndex());
        }
        if (nbind >= PER_VERTEX) {
          if (normindices) {
            pointDetail.setNormalIndex(*normindices);
            currnormal = &normals[*normindices++];
          }
          else {
            pointDetail.setNormalIndex(normnr);
            currnormal = &normals[normnr++];
          }
          vertex.setNormal(*currnormal);
        }
        if (doTextures) {
          if (tb.isFunction()) {
            vertex.setTextureCoords(tb.get(coords->get3(i), *currnormal));
          }
          else {
            pointDetail.setTextureCoordIndex(texindices?*texindices:texidx);
            vertex.setTextureCoords(tb.get(texindices?*texindices++:texidx++));
          }
        }
        pointDetail.setCoordinateIndex(i);
        vertex.setPoint(coords->get3(i));
        this->shapeVertex(&vertex);
        lineDetail.incPartIndex();
        previ = i;
      }
      lineDetail.incLineIndex();
      if (mbind == PER_VERTEX_INDEXED) matindices++;
      if (nbind == PER_VERTEX_INDEXED) normindices++;
      if (doTextures && texindices) texindices++;
    }
    this->endShape();
    return;
  }

  while (cindices + 1 < end) { // need at least two vertices
    this->beginShape(action, LINE_STRIP, &lineDetail);
    i = *cindices++;
    assert(i >= 0);
    if (matindices) {
      pointDetail.setMaterialIndex(*matindices);
      vertex.setMaterialIndex(*matindices++);
    }
    else if (mbind != OVERALL) {
      pointDetail.setMaterialIndex(matnr);
      vertex.setMaterialIndex(matnr++);
    }
    if (normindices) {
      pointDetail.setNormalIndex(*normindices);
      currnormal = &normals[*normindices++];
    }
    else if (nbind != OVERALL) {
      pointDetail.setNormalIndex(normnr);
      currnormal = &normals[normnr++];
    }
    vertex.setNormal(*currnormal);
    if (doTextures) {
      if (tb.isFunction()) {
        vertex.setTextureCoords(tb.get(coords->get3(i), *currnormal));
      }
      else {
        pointDetail.setTextureCoordIndex(texindices?*texindices:texidx);
        vertex.setTextureCoords(tb.get(texindices?*texindices++:texidx++));
      }
    }
    pointDetail.setCoordinateIndex(i);
    vertex.setPoint(coords->get3(i));
    this->shapeVertex(&vertex);

    i = *cindices++;
    assert(i >= 0);
    if (mbind >= PER_VERTEX) {
      if (matindices) vertex.setMaterialIndex(*matindices++);
      else vertex.setMaterialIndex(matnr++);
      pointDetail.setMaterialIndex(vertex.getMaterialIndex());
    }
    if (nbind >= PER_VERTEX) {
      if (normindices) {
        pointDetail.setNormalIndex(*normindices);
        currnormal = &normals[*normindices++];
      }
      else {
        pointDetail.setNormalIndex(normnr);
        currnormal = &normals[normnr++];
      }
      vertex.setNormal(*currnormal);
    }
    if (doTextures) {
      if (tb.isFunction()) {
        vertex.setTextureCoords(tb.get(coords->get3(i), *currnormal));
      }
      else {
        pointDetail.setTextureCoordIndex(texindices?*texindices:texidx);
        vertex.setTextureCoords(tb.get(texindices?*texindices++:texidx++));
      }
    }
    pointDetail.setCoordinateIndex(i);
    vertex.setPoint(coords->get3(i));
    this->shapeVertex(&vertex);
    lineDetail.incPartIndex();

    while (cindices < end && (i = *cindices++) >= 0) {
      assert(cindices <= end);
      if (mbind >= PER_VERTEX) {
        if (matindices) vertex.setMaterialIndex(*matindices++);
        else vertex.setMaterialIndex(matnr++);
        pointDetail.setMaterialIndex(vertex.getMaterialIndex());
      }
      if (nbind >= PER_VERTEX) {
        if (normindices) {
          pointDetail.setNormalIndex(*normindices);
          currnormal = &normals[*normindices++];
        }
        else {
          pointDetail.setNormalIndex(normnr);
          currnormal = &normals[normnr++];
        }
        vertex.setNormal(*currnormal);
      }
      if (doTextures) {
        if (tb.isFunction()) {
          vertex.setTextureCoords(tb.get(coords->get3(i), *currnormal));
        }
        else {
          pointDetail.setTextureCoordIndex(texindices?*texindices:texidx);
          vertex.setTextureCoords(tb.get(texindices?*texindices++:texidx++));
        }
      }
      pointDetail.setCoordinateIndex(i);
      vertex.setPoint(coords->get3(i));
      this->shapeVertex(&vertex);
      lineDetail.incPartIndex();
    }
    this->endShape(); // end of line strip
    if (mbind == PER_VERTEX_INDEXED) matindices++;
    if (nbind == PER_VERTEX_INDEXED) normindices++;
    if (doTextures && texindices) texindices++;
    lineDetail.incLineIndex();
  }

  if (this->vertexProperty.getValue()) {
    state->pop();
  }
}

void
SoIndexedLineSet::notify(SoNotList * list)
{
  SoField *f = list->getLastField();
  if (f == &this->coordIndex) {
    LOCK_VAINDEXER(this);
    delete PRIVATE(this)->vaindexer;
    PRIVATE(this)->vaindexer = NULL;
    UNLOCK_VAINDEXER(this);
  }
  inherited::notify(list);
}


#undef LOCK_VAINDEXER
#undef PRIVATE
#undef UNLOCK_VAINDEXER
