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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97

/*!
  \class SoVRMLIndexedLineSet SoVRMLIndexedLineSet.h Inventor/VRMLnodes/SoVRMLIndexedLineSet.h
  \brief The SoVRMLIndexedLineSet class is used to represent a generic 3D line shape.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  IndexedLineSet {
    eventIn       MFInt32 set_colorIndex
    eventIn       MFInt32 set_coordIndex
    exposedField  SFNode  color             NULL
    exposedField  SFNode  coord             NULL
    field         MFInt32 colorIndex        []     # [-1, inf)
    field         SFBool  colorPerVertex    TRUE
    field         MFInt32 coordIndex        []     # [-1, inf)
  }
  \endverbatim

  The IndexedLineSet node represents a 3D geometry formed by
  constructing polylines from 3D vertices specified in the coord
  field. IndexedLineSet uses the indices in its coordIndex field to
  specify the polylines by connecting vertices from the coord
  field. An index of "-1" indicates that the current polyline has
  ended and the next one begins. The last polyline may be (but does
  not have to be) followed by a "-1".  IndexedLineSet is specified in
  the local coordinate system and is affected by the transformations
  of its ancestors.  

  The coord field specifies the 3D vertices of the line set and
  contains a Coordinate node.  Lines are not lit, are not
  texture-mapped, and do not participate in collision detection. The
  width of lines is implementation dependent and each line segment is
  solid (i.e., not dashed).  If the color field is not NULL, it shall
  contain a Color node.  The colours are applied to the line(s) as
  follows: 
  
  - If colorPerVertex is FALSE:

    - If the colorIndex field is not empty, one colour is used for
      each polyline of the IndexedLineSet. There shall be at least as
      many indices in the colorIndex field as there are polylines in the
      IndexedLineSet.  If the greatest index in the colorIndex field is
      N, there shall be N+1 colours in the Color node. The colorIndex
      field shall not contain any negative entries.
   
    - If the colorIndex field is empty, the colours from the Color
      node are applied to each polyline of the IndexedLineSet in
      order. There shall be at least as many colours in the Color node
      as there are polylines.  

  - If colorPerVertex is TRUE:

    - If the colorIndex field is not empty, colours are applied to
      each vertex of the IndexedLineSet in exactly the same manner that
      the coordIndex field is used to supply coordinates for each vertex
      from the Coordinate node. The colorIndex field shall contain at
      least as many indices as the coordIndex field and shall contain
      end-of-polyline markers (-1) in exactly the same places as the
      coordIndex field.  If the greatest index in the colorIndex field
      is N, there shall be N+1 colours in the Color node.
   
    - If the colorIndex field is empty, the coordIndex field is used
      to choose colours from the Color node. If the greatest index in
      the coordIndex field is N, there shall be N+1 colours in the Color
      node.  

  If the color field is NULL and there is a Material defined for the
  Appearance affecting this IndexedLineSet, the emissiveColor of the
  Material shall be used to draw the lines. Details on lighting
  equations as they affect IndexedLineSet nodes are described in 4.14,
  Lighting model
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.14>).  

*/

#include <Inventor/VRMLnodes/SoVRMLIndexedLineSet.h>

#include <cassert>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoGLDriverDatabase.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/system/gl.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoTextureCoordinateBindingElement.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/SbColor4f.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "nodes/SoSubNodeP.h"
#include "rendering/SoGL.h"
#include "glue/glp.h"
#include "rendering/SoVertexArrayIndexer.h"
#include "rendering/SoVBO.h"

class SoVRMLIndexedLineSetP {
 public:
  SoVRMLIndexedLineSetP() : vaindexer(NULL) { }
  ~SoVRMLIndexedLineSetP() { delete this->vaindexer; }

  enum Binding {
    // Needs to be these specific values to match the rendering code
    // in SoGL.cpp.  FIXME: bad dependency. It looks like the same
    // dependency also exists many other places in the Coin
    // sources. Should grep around and fix this. 20020805 mortene.
    OVERALL = 0,
    PER_LINE = 3,
    PER_LINE_INDEXED = 4,
    PER_VERTEX = 5 ,
    PER_VERTEX_INDEXED = 6
  };

  static Binding findMaterialBinding(SoVRMLIndexedLineSet * node, SoState * state);
  SoVertexArrayIndexer * vaindexer;
};

#define PRIVATE(obj) obj->pimpl
#define LOCK_VAINDEXER(obj) SoBase::staticDataLock()
#define UNLOCK_VAINDEXER(obj) SoBase::staticDataUnlock()


SO_NODE_SOURCE(SoVRMLIndexedLineSet);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLIndexedLineSet::initClass(void) // static
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLIndexedLineSet, SO_VRML97_NODE_TYPE);
}

SoVRMLIndexedLineSet::SoVRMLIndexedLineSet(void)
{
  PRIVATE(this) = new SoVRMLIndexedLineSetP;
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLIndexedLineSet);
}

SoVRMLIndexedLineSet::~SoVRMLIndexedLineSet()
{
  delete PRIVATE(this);
}

SoVRMLIndexedLineSetP::Binding
SoVRMLIndexedLineSetP::findMaterialBinding(SoVRMLIndexedLineSet * node,
                                           SoState * state)
{
  Binding binding = OVERALL;
  if (SoOverrideElement::getMaterialBindingOverride(state)) {
    switch (SoMaterialBindingElement::get(state)) {
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
    case SoMaterialBindingElement::PER_FACE:
      binding = PER_LINE;
      break;
    case SoMaterialBindingElement::PER_FACE_INDEXED:
    case SoMaterialBindingElement::PER_PART_INDEXED:
      binding = PER_LINE_INDEXED;
      break;
    default:
#if COIN_DEBUG
      SoDebugError::postWarning("SoVRMLIndexedLineSetP::findMaterialBinding",
                                "unknown material binding setting");
#endif // COIN_DEBUG
      break;
    }
  }
  else {
    if (node->color.getValue()) {
      if (node->colorPerVertex.getValue()) {
        binding = PER_VERTEX_INDEXED;
        if (!node->colorIndex.getNum()) binding = PER_VERTEX;
      }
      else {
        binding = PER_LINE;
        if (node->colorIndex.getNum()) binding = PER_LINE_INDEXED;
      }
    }
  }
  return binding;
}

void
SoVRMLIndexedLineSet::GLRender(SoGLRenderAction * action)
{
  if (this->coordIndex.getNum() < 2) return;
  
  SoState * state = action->getState();
  state->push();

  SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
  SoMultiTextureEnabledElement::disableAll(state);
  
  SoVRMLVertexLine::GLRender(action);

  if (!this->shouldGLRender(action)) {
    state->pop();
    return;
  }

  // If the coordIndex field is invalid by not including the
  // terminating -1, fix the field by adding it.
  //
  // (FIXME: this is a bit naughty, as we change a field without
  // warning from within the library code. Should really see if we
  // could find a better solution -- which also goes for the other
  // nodes using coordinate index fields, of course. 20010104
  // mortene.)

  const SoCoordinateElement * coords;
  const int32_t * cindices;
  int32_t numindices;
  const int32_t * mindices;

  coords = SoCoordinateElement::getInstance(state);
  cindices = this->coordIndex.getValues(0);
  numindices = this->coordIndex.getNum();
  mindices = this->colorIndex.getNum() ? this->colorIndex.getValues(0) : NULL;

  SoVRMLIndexedLineSetP::Binding mbind =
    SoVRMLIndexedLineSetP::findMaterialBinding(this, state);
  if (mbind == SoVRMLIndexedLineSetP::PER_VERTEX) {
    mbind = SoVRMLIndexedLineSetP::PER_VERTEX_INDEXED;
    mindices = cindices;
  }

  SbBool drawPoints =
    SoDrawStyleElement::get(state) == SoDrawStyleElement::POINTS;

  // place it here so that it will stay in stack scope
  uint32_t packedcolor;

  if (this->color.getValue() == NULL) {
    // FIXME: the vrml97 spec states that the emissiveColor should be
    // used when no color node is found, but this doesn't work for
    // some Hydro models, since diffuseColor is used to set the color
    // of lines. It might be a bug in the VRML2 exporter, but for now
    // I just add emissive and diffuse to support this. pederb,
    // 2001-11-21
    SbColor col = SoLazyElement::getEmissive(state);
    const SbColor & col2 = SoLazyElement::getDiffuse(state, 0);
    
    col += col2;
    
    SbColor4f c(SbClamp(col[0], 0.0f, 1.0f), 
                SbClamp(col[1], 0.0f, 1.0f),
                SbClamp(col[2], 0.0f, 1.0f),
                1.0f);
    packedcolor = col.getPackedValue();
    SoLazyElement::setPacked(state, this, 1, &packedcolor);
  }

  SoMaterialBundle mb(action);
  mb.sendFirst(); // make sure we have the correct material
 
  SoGLLazyElement * lelem = NULL;
  const uint32_t contextid = action->getCacheContext();

  SbBool dova = 
    !drawPoints &&
    SoVBO::shouldRenderAsVertexArrays(state, contextid, numindices) &&
    SoGLDriverDatabase::isSupported(sogl_glue_instance(state), SO_GL_VERTEX_ARRAY);
  
  const SoGLVBOElement * vboelem = SoGLVBOElement::getInstance(state);
  SoVBO * colorvbo = NULL;
  
  if (dova && (mbind != SoVRMLIndexedLineSetP::OVERALL)) {
    dova = FALSE;
    if ((mbind == SoVRMLIndexedLineSetP::PER_VERTEX_INDEXED) && 
        ((mindices == cindices) || (mindices == NULL))) {
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
  SbBool didrenderasvbo = FALSE;
  if (dova) {
    SbBool dovbo = this->startVertexArray(action,
                                          coords,
                                          FALSE,
                                          FALSE,
                                          mbind != SoVRMLIndexedLineSetP::OVERALL);
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
                            FALSE,
                            FALSE,
                            mbind != SoVRMLIndexedLineSetP::OVERALL);
  }
  else {
    
    sogl_render_lineset((SoGLCoordinateElement*)coords,
                        cindices,
                        numindices,
                        NULL,
                        NULL,
                        &mb,
                        mindices,
                        NULL, 0,
                        0,
                        (int)mbind,
                        0,
                        drawPoints ? 1 : 0);
  }
  // send approx number of triangles for autocache handling
  sogl_autocache_update(state, this->coordIndex.getNum() / 2, didrenderasvbo);
  state->pop();
}

void
SoVRMLIndexedLineSet::getPrimitiveCount(SoGetPrimitiveCountAction * action)
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

void
SoVRMLIndexedLineSet::getBoundingBox(SoGetBoundingBoxAction * action)
{
  inherited::getBoundingBox(action);
  // notify open (if any) bbox caches about lines in this shape
  SoBoundingBoxCache::setHasLinesOrPoints(action->getState());
}

void
SoVRMLIndexedLineSet::generatePrimitives(SoAction * action)
{
  if (this->coordIndex.getNum() < 2) return;

  SoState * state = action->getState();
  state->push();

  SoVRMLVertexLine::doAction(action);

  const SoCoordinateElement * coords;
  int32_t numindices;
  const int32_t * cindices;
  const int32_t * matindices;

  coords = SoCoordinateElement::getInstance(state);
  cindices = this->coordIndex.getValues(0);
  numindices = this->coordIndex.getNum();
  matindices = this->colorIndex.getNum() ? this->colorIndex.getValues(0) : NULL;

  SoVRMLIndexedLineSetP::Binding mbind =
    SoVRMLIndexedLineSetP::findMaterialBinding(this, state);
  if (mbind == SoVRMLIndexedLineSetP::PER_VERTEX) {
    mbind = SoVRMLIndexedLineSetP::PER_VERTEX_INDEXED;
    matindices = cindices;
  }

  if (mbind == SoVRMLIndexedLineSetP::PER_LINE || mbind == SoVRMLIndexedLineSetP::OVERALL) {
    matindices = NULL;
  }

  int matnr = 0;
  int32_t i;
  const int32_t *end = cindices + numindices;

  SoPrimitiveVertex vertex;
  SoPointDetail pointDetail;
  SoLineDetail lineDetail;
  vertex.setDetail(&pointDetail);

  while (cindices + 1 < end) {
    this->beginShape(action, LINE_STRIP, &lineDetail);
    i = *cindices++;
    assert(i >= 0);
    if (matindices) {
      pointDetail.setMaterialIndex(*matindices);
      vertex.setMaterialIndex(*matindices++);
    }
    else if (mbind != SoVRMLIndexedLineSetP::OVERALL) {
      pointDetail.setMaterialIndex(matnr);
      vertex.setMaterialIndex(matnr++);
    }
    pointDetail.setCoordinateIndex(i);
    vertex.setPoint(coords->get3(i));
    this->shapeVertex(&vertex);

    i = *cindices++;
    assert(i >= 0);
    if (mbind >= SoVRMLIndexedLineSetP::PER_VERTEX) {
      if (matindices) vertex.setMaterialIndex(*matindices++);
      else vertex.setMaterialIndex(matnr++);
      pointDetail.setMaterialIndex(vertex.getMaterialIndex());
    }
    pointDetail.setCoordinateIndex(i);
    vertex.setPoint(coords->get3(i));
    this->shapeVertex(&vertex);
    lineDetail.incPartIndex();

    i = cindices < end ? *cindices++ : -1;
    while (i >= 0) {
      if (mbind >= SoVRMLIndexedLineSetP::PER_VERTEX) {
        if (matindices) vertex.setMaterialIndex(*matindices++);
        else vertex.setMaterialIndex(matnr++);
        pointDetail.setMaterialIndex(vertex.getMaterialIndex());
      }
      pointDetail.setCoordinateIndex(i);
      vertex.setPoint(coords->get3(i));
      this->shapeVertex(&vertex);
      lineDetail.incPartIndex();
      i = cindices < end ? *cindices++ : -1;
    }
    this->endShape(); // end of line strip
    if (mbind == SoVRMLIndexedLineSetP::PER_VERTEX_INDEXED) matindices++;
    lineDetail.incLineIndex();
  }
  state->pop();
}

void 
SoVRMLIndexedLineSet::notify(SoNotList * list)
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
#endif // HAVE_VRML97
