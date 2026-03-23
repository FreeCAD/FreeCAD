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
  \class SoIndexedNurbsCurve SoIndexedNurbsCurve.h Inventor/nodes/SoIndexedNurbsCurve.h
  \brief The SoIndexedNurbsCurve class is a node for representing smooth curves.

  \ingroup coin_nodes

  Explaining NURBS is beyond the scope of this documentation. If you
  are unfamiliar with the principles of representing smooth curves and
  surfaces when doing 3D visualization, we recommend finding a good
  book on the subject.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    IndexedNurbsCurve {
        numControlPoints 0
        coordIndex 0
        knotVector 0
    }
  \endcode

*/
// FIXME: Usage example! Also give a reference to a decent
// book on NURBS. 20011220 mortene.

#include <Inventor/nodes/SoIndexedNurbsCurve.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoPickStyleElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoComplexityTypeElement.h>
#include <Inventor/system/gl.h>

#include "coindefs.h" // COIN_OBSOLETED()
#include "glue/GLUWrapper.h"
#include "nodes/SoSubNodeP.h"
#include "rendering/SoGLNurbs.h"
#include "rendering/SoGL.h"
#include "SoNurbsP.h"

/*!
  \var SoSFInt32 SoIndexedNurbsCurve::numControlPoints
  Number of control points for this curve.
*/
/*!
  \var SoMFInt32 SoIndexedNurbsCurve::coordIndex
  The control point indices. Supply at least numControlPoint indices.
*/
/*!
  \var SoMFFloat SoIndexedNurbsCurve::knotVector
  The knot vector.
*/


// *************************************************************************

class SoIndexedNurbsCurveP {
public:
  SoIndexedNurbsCurveP(SoIndexedNurbsCurve * m)
  {
    this->owner = m;
    this->nurbsrenderer = NULL;
    this->offscreenctx = NULL;
  }

  ~SoIndexedNurbsCurveP()
  {
    if (this->offscreenctx) { cc_glglue_context_destruct(this->offscreenctx); }
    if (this->nurbsrenderer) {
      GLUWrapper()->gluDeleteNurbsRenderer(this->nurbsrenderer);
    }
  }

  void * offscreenctx;
  void * nurbsrenderer;

  void doNurbs(SoAction * action, const SbBool glrender, const SbBool drawaspoints);

private:
  SoIndexedNurbsCurve * owner;
};

#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->owner)

// *************************************************************************

SO_NODE_SOURCE(SoIndexedNurbsCurve);

/*!
  Constructor.
*/
SoIndexedNurbsCurve::SoIndexedNurbsCurve()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoIndexedNurbsCurve);

  SO_NODE_ADD_FIELD(numControlPoints, (0));
  SO_NODE_ADD_FIELD(coordIndex, (0));
  SO_NODE_ADD_FIELD(knotVector, (0));

  PRIVATE(this) = new SoIndexedNurbsCurveP(this);
}

/*!
  Destructor.
*/
SoIndexedNurbsCurve::~SoIndexedNurbsCurve()
{
  delete PRIVATE(this);
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoIndexedNurbsCurve::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoIndexedNurbsCurve, SO_FROM_INVENTOR_1);
}

/*!
  Calculates the bounding box of all control points, and sets the
  center to the average of these points.
*/
void
SoIndexedNurbsCurve::computeBBox(SoAction * action,
                                 SbBox3f & box, SbVec3f & center)
{
  SoState * state = action->getState();
  const SoCoordinateElement * coordelem =
    SoCoordinateElement::getInstance(state);

  int num = this->numControlPoints.getValue();

  SbVec3f acccenter(0.0f, 0.0f, 0.0f);
  box.makeEmpty();
  SbVec3f tmp3D;
  if (coordelem->is3D()) {
    const SbVec3f * coords = coordelem->getArrayPtr3();
    assert(coords);
    for (int i = 0; i < num; i++) {
      tmp3D = coords[this->coordIndex[i]];
      box.extendBy(tmp3D);
      acccenter += tmp3D;
    }
  }
  else {
    const SbVec4f * coords = coordelem->getArrayPtr4();
    assert(coords);
    for (int i = 0; i < num; i++) {
      SbVec4f tmp = coords[this->coordIndex[i]];
      float mul = (tmp[3] != 0.0f) ? 1.0f / tmp[3] : 1.0f;
      tmp3D.setValue(tmp[0]*mul, tmp[1]*mul, tmp[2]*mul);
      box.extendBy(tmp3D);
      acccenter += tmp3D;
    }
  }
  if (num) center = acccenter / float(num);
}

// doc from parent
void
SoIndexedNurbsCurve::GLRender(SoGLRenderAction * action)
{
  if (!this->shouldGLRender(action)) return;

  SoState * state = action->getState();
  state->push();

  // disable lighting
  SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);

  // initialize current material
  SoMaterialBundle mb(action);
  mb.sendFirst();

  // disable texturing
  SoGLMultiTextureEnabledElement::disableAll(state);
  
  // Create lazy element for GL_AUTO_NORMAL ?
  glEnable(GL_AUTO_NORMAL);
  PRIVATE(this)->doNurbs(action, TRUE, SoDrawStyleElement::get(action->getState()) == SoDrawStyleElement::POINTS);
  glDisable(GL_AUTO_NORMAL);

  state->pop();
  if (SoComplexityTypeElement::get(state) == SoComplexityTypeElement::OBJECT_SPACE) {
    SoGLCacheContextElement::shouldAutoCache(state,
                                             SoGLCacheContextElement::DO_AUTO_CACHE);
    SoGLCacheContextElement::incNumShapes(state);
  }
}

// doc from parent
void
SoIndexedNurbsCurve::rayPick(SoRayPickAction * action)
{
  if (!this->shouldRayPick(action)) return;
  if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
    SoShape::rayPick(action); // do normal generatePrimitives() pick
  }
  else {
    static SbBool firstpick = TRUE;
    if (firstpick) {
      firstpick = FALSE;
      SoDebugError::postWarning("SoIndexedNurbsCurve::rayPick",
                                "Proper NURBS picking requires\n"
                                "GLU version 1.3. Picking will be done on bounding box.");
    }
    SoState * state = action->getState();
    state->push();
    SoPickStyleElement::set(state, this, SoPickStyleElement::BOUNDING_BOX);
    (void)this->shouldRayPick(action); // this will cause a pick on bbox
    state->pop();
  }
}

// doc from parent
void
SoIndexedNurbsCurve::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  // for now, just generate primitives to count. Very slow, of course.
  SoShape::getPrimitiveCount(action);
}

/*!
  Redefined to notify open caches that this shape contains lines.
*/
void
SoIndexedNurbsCurve::getBoundingBox(SoGetBoundingBoxAction * action)
{
  inherited::getBoundingBox(action);
  SoBoundingBoxCache::setHasLinesOrPoints(action->getState());
}

/*!
  This method is part of the original SGI Inventor API, but not
  implemented in Coin, as it looks like a method that should probably
  have been private in Open Inventor.
*/
void
SoIndexedNurbsCurve::sendPrimitive(SoAction * ,  SoPrimitiveVertex *)
{
  COIN_OBSOLETED();
}

// doc from parent
void
SoIndexedNurbsCurve::generatePrimitives(SoAction * action)
{
  if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {

    // We've found that the SGI GLU NURBS renderer makes some OpenGL
    // calls even when in tessellate mode. So we need to set up an
    // offscreen context to be guaranteed to have a valid GL context
    // before making the GLU calls.

    if (PRIVATE(this)->offscreenctx == NULL) {
      PRIVATE(this)->offscreenctx = cc_glglue_context_create_offscreen(32, 32);
    }

    if (PRIVATE(this)->offscreenctx &&
        cc_glglue_context_make_current(PRIVATE(this)->offscreenctx)) {
      PRIVATE(this)->doNurbs(action, FALSE, FALSE);
      cc_glglue_context_reinstate_previous(PRIVATE(this)->offscreenctx);
    }
  }
}

// Documented in superclass.
SoDetail *
SoIndexedNurbsCurve::createLineSegmentDetail(SoRayPickAction * /* action */,
                                             const SoPrimitiveVertex * /* v1 */,
                                             const SoPrimitiveVertex * /* v2 */,
                                             SoPickedPoint * /* pp */)
{
  return NULL;
}

typedef SoNurbsP<SoIndexedNurbsCurve>::coin_nurbs_cbdata coin_inc_cbdata;

void
SoIndexedNurbsCurveP::doNurbs(SoAction * action,
                              const SbBool glrender, const SbBool drawaspoints)
{
  if (GLUWrapper()->available == 0 || !GLUWrapper()->gluNewNurbsRenderer) {
#if COIN_DEBUG
    static int first = 1;
    if (first) {
      SoDebugError::postInfo("SoIndexedNurbsCurve::doNurbs",
                             "Looks like your GLU library doesn't have NURBS "
                             "functionality");
      first = 0;
    }
#endif // COIN_DEBUG
    return;
  }

  if (this->nurbsrenderer == NULL) {
    this->nurbsrenderer = GLUWrapper()->gluNewNurbsRenderer();

    if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_BEGIN_DATA, (gluNurbsCallback_cb_t)SoNurbsP<SoIndexedNurbsCurve>::tessBegin);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_TEXTURE_COORD_DATA, (gluNurbsCallback_cb_t)SoNurbsP<SoIndexedNurbsCurve>::tessTexCoord);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_NORMAL_DATA, (gluNurbsCallback_cb_t)SoNurbsP<SoIndexedNurbsCurve>::tessNormal);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_VERTEX_DATA, (gluNurbsCallback_cb_t)SoNurbsP<SoIndexedNurbsCurve>::tessVertex);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_END_DATA, (gluNurbsCallback_cb_t)SoNurbsP<SoIndexedNurbsCurve>::tessEnd);
    }
  }

  // NB, don't move this structure inside the if-statement. It needs
  // to be here so that the callbacks from sogl_render_nurbs_curve()
  // have a valid pointer to the structure.
  coin_inc_cbdata cbdata(action, PUBLIC(this),
                         !SoCoordinateElement::getInstance(action->getState())->is3D());

  if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
    if (!glrender) {
      GLUWrapper()->gluNurbsCallbackData(this->nurbsrenderer, &cbdata);
      cbdata.vertex.setNormal(SbVec3f(0.0f, 0.0f, 1.0f));
      cbdata.vertex.setMaterialIndex(0);
      cbdata.vertex.setTextureCoords(SbVec4f(0.0f, 0.0f, 0.0f, 1.0f));
      cbdata.vertex.setPoint(SbVec3f(0.0f, 0.0f, 0.0f));
      cbdata.vertex.setDetail(NULL);
    }
  }

  sogl_render_nurbs_curve(action, PUBLIC(this), this->nurbsrenderer,
                          PUBLIC(this)->numControlPoints.getValue(),
                          PUBLIC(this)->knotVector.getValues(0),
                          PUBLIC(this)->knotVector.getNum(),
                          glrender,
                          drawaspoints,
                          PUBLIC(this)->coordIndex.getNum(),
                          PUBLIC(this)->coordIndex.getValues(0));
}

#undef PRIVATE
#undef PUBLIC
