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
  \class SoNurbsCurve SoNurbsCurve.h Inventor/nodes/SoNurbsCurve.h
  \brief The SoNurbsCurve class is a node for representing smooth curves.

  \ingroup coin_nodes

  A general explanation of NURBS is beyond the scope of the Coin
  documentation. For detailed information, refer to the specialized
  literature on the topic (for example "An Introduction to NURBS: With
  Historical Perspective" by David F. Rogers). A basic overview of
  curve and surface rendering using NURBS can be found in chapter 8 of
  "The Inventor Mentor".

  Note that knot values should be specified as [0, 1, 2,..., a] rather
  than [0, 1/a, 2/a,..., 1] to avoid tessellation errors due to
  floating point precision problems. (Even if the rendered curve
  <i>looks</i> correct, such issues might surface when e.g. doing
  picking, since the tessellated representation used internally is not
  the same as the one you see rendered by OpenGL on-screen.)

  Each control point has a weight that changes the shape of its basis function.
  Weight is analogous to having magnets pulling on the curve.
  Coordinate3 sets control points to have an equal weight of 1.0 (nonrational).
  Use Coordinate4 to specify x, y, z and weight values (rational).  

  A small usage example (drawing a circle using a rational curve):

  \code
  #Inventor V2.1 ascii

  Coordinate4 {
     point [
       0.0 -5.0     0.0 1.0,
       2.5 -2.5     0.0 0.5,
       2.5 -0.66987 0.0 1.0,
       0.0  1.94013 0.0 0.5,
      -2.5 -0.66987 0.0 1.0,
      -2.5 -2.5     0.0 0.5,
       0.0 -5.0     0.0 1.0
     ]
  }

  NurbsCurve {
     numControlPoints 7
     knotVector [ 0, 0, 0, 1, 1, 2, 2, 3, 3, 3 ]
  }

  # debug: show all the control points
  BaseColor { rgb 1 1 0 }
  DrawStyle { pointSize 3 }
  PointSet { numPoints 7 }
  \endcode

  <center>
  \image html nurbscurve-circle.png "Rendering of Example Scenegraph"
  </center>

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    NurbsCurve {
        numControlPoints 0
        knotVector 0
    }
  \endcode

*/

#include <Inventor/nodes/SoNurbsCurve.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/elements/SoComplexityTypeElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoPickStyleElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/system/gl.h>

#include "glue/GLUWrapper.h"
#include "nodes/SoSubNodeP.h"
#include "rendering/SoGL.h"
#include "rendering/SoGLNurbs.h"
#include "coindefs.h" // COIN_OBSOLETED()
#include "SoNurbsP.h"

// *************************************************************************

/*!
  \var SoSFInt32 SoNurbsCurve::numControlPoints
  Number of control points to use in this NURBS curve.
*/
/*!
  \var SoMFFloat SoNurbsCurve::knotVector
  The knot vector.

  Values should be specified as [0, 1, 2,..., a] rather than [0, 1/a,
  2/a,..., 1] to avoid tessellation errors due to floating point
  precision problems.
*/

// *************************************************************************

class SoNurbsCurveP {
public:
  SoNurbsCurveP(SoNurbsCurve * m)
  {
    this->owner = m;
    this->nurbsrenderer = NULL;
    this->offscreenctx = NULL;
  }

  ~SoNurbsCurveP()
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
  SoNurbsCurve * owner;
};

#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->owner)

// *************************************************************************

SO_NODE_SOURCE(SoNurbsCurve);

/*!
  Constructor.
*/
SoNurbsCurve::SoNurbsCurve(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoNurbsCurve);

  SO_NODE_ADD_FIELD(numControlPoints, (0));
  SO_NODE_ADD_FIELD(knotVector, (0));

  PRIVATE(this) = new SoNurbsCurveP(this);
}

/*!
  Destructor.
*/
SoNurbsCurve::~SoNurbsCurve()
{
  delete PRIVATE(this);
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoNurbsCurve::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoNurbsCurve, SO_FROM_INVENTOR_1);
}

// Doc from parent class.
void
SoNurbsCurve::GLRender(SoGLRenderAction * action)
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
  }
}

/*!
  Calculates the bounding box of all control points, and sets the
  center to the average of these points.
*/
void
SoNurbsCurve::computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center)
{
  SoState * state = action->getState();
  const SoCoordinateElement * coordelem =
    SoCoordinateElement::getInstance(state);

  int numCoords = coordelem->getNum();
  int num = this->numControlPoints.getValue();

  assert(num <= numCoords);

  SbVec3f acccenter(0.0f, 0.0f, 0.0f);
  box.makeEmpty();
  if (coordelem->is3D()) {
    const SbVec3f * coords = coordelem->getArrayPtr3();
    assert(coords);
    for (int i = 0; i < num; i++) {
      box.extendBy(coords[i]);
      acccenter += coords[i];
    }
  }
  else {
    const SbVec4f * coords = coordelem->getArrayPtr4();
    assert(coords);
    for (int i = 0; i < num; i++) {
      SbVec4f tmp = coords[i];
      float mul = (tmp[3] != 0.0f) ? 1.0f / tmp[3] : 1.0f;
      SbVec3f tmp3D(tmp[0]*mul, tmp[1]*mul, tmp[2]*mul);
      box.extendBy(tmp3D);
      acccenter += tmp3D;
    }
  }
  if (num) center = acccenter / float(num);
}

// Doc from parent class.
void
SoNurbsCurve::rayPick(SoRayPickAction * action)
{
  if (!this->shouldRayPick(action)) return;

  if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
    SoShape::rayPick(action); // do normal generatePrimitives() pick
  }
  else {
    static SbBool firstpick = TRUE;
    if (firstpick) {
      firstpick = FALSE;
      SoDebugError::postWarning("SoNurbsCurve::rayPick",
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

// Doc from parent class.
void
SoNurbsCurve::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  // for now, just generate primitives to count. Very slow, of course.
  SoShape::getPrimitiveCount(action);
}

/*!
  Redefined to notify open caches that this shape contains lines.
*/
void
SoNurbsCurve::getBoundingBox(SoGetBoundingBoxAction * action)
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
SoNurbsCurve::sendPrimitive(SoAction * COIN_UNUSED_ARG(a), SoPrimitiveVertex * COIN_UNUSED_ARG(p))
{
  COIN_OBSOLETED();
}

// Doc from parent class.
void
SoNurbsCurve::generatePrimitives(SoAction * action)
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
SoNurbsCurve::createLineSegmentDetail(SoRayPickAction * COIN_UNUSED_ARG(action),
                                      const SoPrimitiveVertex * COIN_UNUSED_ARG(v1),
                                      const SoPrimitiveVertex * COIN_UNUSED_ARG(v2),
                                      SoPickedPoint * COIN_UNUSED_ARG(pp))
{
  return NULL;
}

typedef SoNurbsP<SoNurbsCurve>::coin_nurbs_cbdata coin_nc_cbdata;

void
SoNurbsCurveP::doNurbs(SoAction * action,
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
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_BEGIN_DATA, (gluNurbsCallback_cb_t)SoNurbsP<SoNurbsCurve>::tessBegin);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_TEXTURE_COORD_DATA, (gluNurbsCallback_cb_t)SoNurbsP<SoNurbsCurve>::tessTexCoord);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_NORMAL_DATA, (gluNurbsCallback_cb_t)SoNurbsP<SoNurbsCurve>::tessNormal);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_VERTEX_DATA, (gluNurbsCallback_cb_t)SoNurbsP<SoNurbsCurve>::tessVertex);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_END_DATA, (gluNurbsCallback_cb_t)SoNurbsP<SoNurbsCurve>::tessEnd);
    }
  }

  // NB, don't move this structure inside the if-statement. It needs
  // to be here so that the callbacks from sogl_render_nurbs_curve()
  // have a valid pointer to the structure.
  coin_nc_cbdata cbdata(action, PUBLIC(this),
                        !SoCoordinateElement::getInstance(action->getState())->is3D());

  if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
    // if we're just tessellating, the callbacks will be invoked:
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
                          drawaspoints);
}

#undef PRIVATE
#undef PUBLIC
