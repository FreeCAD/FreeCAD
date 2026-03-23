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
  \class SoPointSet SoPointSet.h Inventor/nodes/SoPointSet.h
  \brief The SoPointSet class is used to display a set of 3D points.

  \ingroup coin_nodes

  This node either uses the coordinates currently on the state
  (typically set up by a leading SoCoordinate3 node in the scene graph)
  or from a SoVertexProperty node attached to this node to render a
  set of 3D points.

  The SoPointSet::numPoints field specifies the number of points in
  the coordinate set which should be rendered (or otherwise handled by
  traversal actions).

  Here's a simple usage example of SoPointSet in a scene graph:

  \verbatim
  #Inventor V2.1 ascii

  Separator {
     Material {
        diffuseColor [
         1 0 0, 0 1 0, 0 0 1, 1 1 0, 1 0 1, 1 1 1, 1 0.8 0.6, 0.6 0.8 1
        ]
     }
     MaterialBinding { value PER_PART }

     Coordinate3 {
        point [
         -1 1 0, -1 -1 0, 1 -1 0, 1 1 0, 0 2 -1, -2 0 -1, 0 -2 -1, 2 0 -1
        ]
     }

     DrawStyle { pointSize 3 }

     PointSet { }
  }
  \endverbatim

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    PointSet {
        vertexProperty NULL
        startIndex 0
        numPoints -1
    }
  \endcode

*/

#include <Inventor/nodes/SoPointSet.h>

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

/*!
  \var SoSFInt32 SoPointSet::numPoints

  Used to specify number of points in the point set.  Coordinates for
  the points will be taken from the state stack's set of 3D
  coordinates, typically set up by a leading SoCoordinate3 node.

  If this field is equal to -1 (the default value) \e all coordinates
  currently on the state will be rendered or otherwise handled by
  traversal actions.

  SoPointSet inherits the field SoNonIndexedShape::startIndex, which
  specifies the start index for points from the current state set of
  coordinates. Please note that this field has been obsoleted, but is
  still provided for compatibility.
*/

SO_NODE_SOURCE(SoPointSet);

/*!
  Constructor.
*/
SoPointSet::SoPointSet()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoPointSet);

  SO_NODE_ADD_FIELD(numPoints, (-1));
}

/*!
  Destructor.
*/
SoPointSet::~SoPointSet()
{
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoPointSet::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoPointSet, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

// doc from parent
void
SoPointSet::computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center)
{
  inherited::computeCoordBBox(action, this->numPoints.getValue(), box, center);
}

// Internal method which translates the current material binding
// found on the state to a material binding for this node.
// PER_PART, PER_FACE, PER_VERTEX and their indexed counterparts
// are translated to PER_VERTEX binding. OVERALL means overall
// binding for point set also, of course. The default material
// binding is OVERALL.
SoPointSet::Binding
SoPointSet::findMaterialBinding(SoState * const state) const
{
  Binding binding = OVERALL;
  if (SoMaterialBindingElement::get(state) !=
      SoMaterialBindingElement::OVERALL) binding = PER_VERTEX;
  return binding;
}

//  Internal method which translates the current normal binding
//  found on the state to a normal binding for this node.
//  PER_PART, PER_FACE, PER_VERTEX and their indexed counterparts
//  are translated to PER_VERTEX binding. OVERALL means overall
//  binding for point set also, of course. The default normal
//  binding is PER_VERTEX.
SoPointSet::Binding
SoPointSet::findNormalBinding(SoState * const state) const

{
  Binding binding = PER_VERTEX;

  if (SoNormalBindingElement::get(state) ==
      SoNormalBindingElement::OVERALL) binding = OVERALL;
  return binding;
}

// doc from parent
void
SoPointSet::GLRender(SoGLRenderAction * action)
{
  int32_t numpts = this->numPoints.getValue();
  if (numpts == 0) return;
  if (!this->shouldGLRender(action)) return;

  SoState * state = action->getState();

  SbBool didpush = FALSE;
  if (this->vertexProperty.getValue()) {
    state->push();
    didpush = TRUE;
    this->vertexProperty.getValue()->GLRender(action);
  }

  const SoCoordinateElement * tmp;
  const SbVec3f * normals;

  SoTextureCoordinateBundle tb(action, TRUE, FALSE);
  SbBool doTextures = tb.needCoordinates();
  SoMaterialBundle mb(action);
  
  SbBool needNormals = !mb.isColorOnly() || tb.isFunction();

  SoVertexShape::getVertexData(state, tmp, normals,
                               needNormals);

  if (normals == NULL && needNormals) {
    needNormals = FALSE;
    if (!didpush) {
      state->push();
      didpush = TRUE;
    }
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
  }

  const SoGLCoordinateElement * coords = (SoGLCoordinateElement *)tmp;

  Binding mbind = this->findMaterialBinding(action->getState());

  Binding nbind = OVERALL;
  if (needNormals) nbind = this->findNormalBinding(action->getState());

  if (nbind == OVERALL && needNormals) {
    if (normals) glNormal3fv((const GLfloat *)normals);
    else glNormal3f(0.0f, 0.0f, 1.0f);
  }

  mb.sendFirst(); // make sure we have the correct material

  int32_t idx = this->startIndex.getValue();
  if (numpts < 0) numpts = coords->getNum() - idx;
  
  const cc_glglue * glue = sogl_glue_instance(state);
  const uint32_t contextid = action->getCacheContext();

  SbBool dova = 
    SoVBO::shouldRenderAsVertexArrays(state, contextid, numpts) && 
    SoGLDriverDatabase::isSupported(glue, SO_GL_VERTEX_ARRAY);
  
  if (dova && (mbind == PER_VERTEX)) {
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
                                        coords,
                                        (needNormals && (nbind != OVERALL)) ? normals : NULL,
                                        doTextures,
                                        mbind == PER_VERTEX);
    didrenderasvbo = vbo;
    cc_glglue_glDrawArrays(glue, GL_POINTS, idx, numpts);
    this->finishVertexArray(action, vbo,
                            (needNormals && (nbind != OVERALL)),
                            doTextures,
                            mbind == PER_VERTEX);
  }
  else {
    sogl_render_pointset(coords,
                         nbind != OVERALL ? normals : NULL,
                         mbind != OVERALL ? &mb : NULL,
                         doTextures ? &tb : NULL,
                         numpts, idx);
  }
  if (didpush) 
    state->pop();

  // send approx number of points for autocache handling. Divide
  // by three so that three points is the same as one triangle.
  sogl_autocache_update(state, numpts/3, didrenderasvbo);

}

// Documented in superclass.
SbBool
SoPointSet::generateDefaultNormals(SoState *, SoNormalCache * nc)
{
  // Overridden to clear normal cache, as it's not possible to
  // generate a normal for a point.
  nc->set(0, NULL);
  return TRUE;
}

// Documented in superclass.
SbBool
SoPointSet::generateDefaultNormals(SoState * COIN_UNUSED_ARG(state), SoNormalBundle * COIN_UNUSED_ARG(bundle))
{
  // Overridden to avoid (faulty) compiler warnings with some version
  // of g++.
  return FALSE;
}

// doc from parent
void
SoPointSet::getBoundingBox(SoGetBoundingBoxAction *action)
{
  inherited::getBoundingBox(action);
}

// doc from parent
void
SoPointSet::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  if (!this->shouldPrimitiveCount(action)) return;
  int num = this->numPoints.getValue();
  if (num < 0) {
    SoNode *vpnode = this->vertexProperty.getValue();
    SoVertexProperty *vp = 
      (vpnode && vpnode->isOfType(SoVertexProperty::getClassTypeId())) ?
      (SoVertexProperty *)vpnode : NULL;
    if (vp && vp->vertex.getNum()) {
      num = vp->vertex.getNum() - this->startIndex.getValue();
    }
    else {
      const SoCoordinateElement *coordelem =
        SoCoordinateElement::getInstance(action->getState());
      num = coordelem->getNum() - this->startIndex.getValue();
    }
  }
  action->addNumPoints(num);
}

// doc from parent
void
SoPointSet::generatePrimitives(SoAction *action)
{
  int32_t numpts = this->numPoints.getValue();
  if (numpts == 0) return;

  SoState * state = action->getState();

  if (this->vertexProperty.getValue()) {
    state->push();
    this->vertexProperty.getValue()->doAction(action);
  }

  const SoCoordinateElement *coords;
  const SbVec3f * normals;
  SbBool doTextures;
  SbBool needNormals = TRUE;

  SoVertexShape::getVertexData(action->getState(), coords, normals,
                               needNormals);

  if (normals == NULL) needNormals = FALSE;

  SoTextureCoordinateBundle tb(action, FALSE, FALSE);
  doTextures = tb.needCoordinates();

  Binding mbind = this->findMaterialBinding(action->getState());
  Binding nbind = this->findNormalBinding(action->getState());

  if (!needNormals) nbind = OVERALL;

  SoPrimitiveVertex vertex;
  SoPointDetail pointDetail;
  vertex.setDetail(&pointDetail);

  SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
  const SbVec3f * currnormal = &dummynormal;
  if (normals) currnormal = normals;
  if (nbind == OVERALL && needNormals)
    vertex.setNormal(*currnormal);

  int32_t idx = this->startIndex.getValue();
  if (numpts < 0) numpts = coords->getNum() - idx;

  int matnr = 0;
  int texnr = 0;
  int normnr = 0;

  this->beginShape(action, SoShape::POINTS);
  for (int i = 0; i < numpts; i++) {
    if (nbind == PER_VERTEX) {
      pointDetail.setNormalIndex(normnr);
      currnormal = &normals[normnr++];
      vertex.setNormal(*currnormal);
    }
    if (mbind == PER_VERTEX) {
      pointDetail.setMaterialIndex(matnr);
      vertex.setMaterialIndex(matnr++);
    }
    if (doTextures) {
      if (tb.isFunction()) {
        vertex.setTextureCoords(tb.get(coords->get3(idx), *currnormal));
      }
      else {
        pointDetail.setTextureCoordIndex(texnr);
        vertex.setTextureCoords(tb.get(texnr++));
      }
    }
    pointDetail.setCoordinateIndex(idx);
    vertex.setPoint(coords->get3(idx++));
    this->shapeVertex(&vertex);
  }
  this->endShape();

  if (this->vertexProperty.getValue())
    state->pop();
}
