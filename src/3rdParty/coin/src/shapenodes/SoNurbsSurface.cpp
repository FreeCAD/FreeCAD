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
  \class SoNurbsSurface SoNurbsSurface.h Inventor/nodes/SoNurbsSurface.h
  \brief The SoNurbsSurface class is used to render smooth surfaces.

  \ingroup coin_nodes

  A general explanation of NURBS is beyond the scope of the Coin
  documentation. For detailed information, refer to the specialized
  literature on the topic (for example "An Introduction to NURBS: With
  Historical Perspective" by David F. Rogers). A basic overview of
  curve and surface rendering using NURBS can be found in chapter 8 of
  "The Inventor Mentor".

  Note that knot values should be specified as [0, 1, 2,..., a] rather
  than [0, 1/a, 2/a,..., 1] to avoid tessellation errors due to
  floating point precision problems. (Even if the rendered surface
  <i>looks</i> correct, such issues might surface when e.g. doing
  picking, since the tessellated representation used internally is not
  the same as the one you see rendered by OpenGL on-screen.)

  Each control point has a weight that changes the shape of its basis function.
  Weight is analogous to having magnets pulling on the curve.
  Coordinate3 sets control points to have an equal weight of 1.0 (nonrational).
  Use Coordinate4 to specify x, y, z and weight values (rational).

  A basic usage example:

  \code
  #Inventor V2.1 ascii

  ShapeHints {
    vertexOrdering COUNTERCLOCKWISE
  }

  Coordinate3 {
     point [
       -3 -3 -3, -3 -1 -3, -3  1 -3, -3  3 -3,
       -1 -3 -3, -1 -1  3, -1  1  3, -1  3 -3,
        1 -3 -3,  1 -1  3,  1  1  3,  1  3 -3,
        3 -3 -3,  3 -1 -3,  3  1 -3,  3  3 -3
      ]
  }

  NurbsSurface {
     numUControlPoints 4
     numVControlPoints 4
     uKnotVector [ 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 ]
     vKnotVector [ 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 ]
  }
  \endcode

  <center>
  \image html nurbssurface.png "Rendering of Example Scenegraph"
  </center>

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    NurbsSurface {
        numUControlPoints 0
        numVControlPoints 0
        numSControlPoints 0
        numTControlPoints 0
        uKnotVector 0
        vKnotVector 0
        sKnotVector 0
        tKnotVector 0
    }
  \endcode

*/

#include <Inventor/nodes/SoNurbsSurface.h>
#include "SoNurbsP.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoPickStyleElement.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoComplexityTypeElement.h>
#include <Inventor/system/gl.h>

#include "glue/GLUWrapper.h"
#include "nodes/SoSubNodeP.h"
#include "rendering/SoGLNurbs.h"
#include "rendering/SoGL.h"
#include "coindefs.h" // COIN_OBSOLETED()

/*!
  \var SoSFInt32 SoNurbsSurface::numUControlPoints
  Number of control points in the U direction.
*/
/*!
  \var SoSFInt32 SoNurbsSurface::numVControlPoints
  Number of control points in the V direction.
*/
/*!
  \var SoSFInt32 SoNurbsSurface::numSControlPoints
  Number of control points in the S direction.
*/
/*!
  \var SoSFInt32 SoNurbsSurface::numTControlPoints
  Number of control points in the T direction.
*/
/*!
  \var SoMFFloat SoNurbsSurface::uKnotVector
  The Bezier knot vector for the U direction.
*/
/*!
  \var SoMFFloat SoNurbsSurface::vKnotVector
  The Bezier knot vector for the V direction.
*/
/*!
  \var SoMFFloat SoNurbsSurface::sKnotVector
  The Bezier knot vector for the S direction.
*/
/*!
  \var SoMFFloat SoNurbsSurface::tKnotVector
  The Bezier knot vector for the T direction.
*/

// *************************************************************************

class SoNurbsSurfaceP {
public:
  SoNurbsSurfaceP(SoNurbsSurface * m)
  {
    this->owner = m;
    this->nurbsrenderer = NULL;
    this->offscreenctx = NULL;
  }

  ~SoNurbsSurfaceP()
  {
    if (this->nurbsrenderer) {
      GLUWrapper()->gluDeleteNurbsRenderer(this->nurbsrenderer);
    }
    if (this->offscreenctx) { cc_glglue_context_destruct(this->offscreenctx); }
  }

  void * offscreenctx;
  void * nurbsrenderer;

  void doNurbs(SoAction * action, const SbBool glrender);

private:
  SoNurbsSurface * owner;
};

#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->owner)

// *************************************************************************

SO_NODE_SOURCE(SoNurbsSurface);

/*!
  Constructor.
*/
SoNurbsSurface::SoNurbsSurface()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoNurbsSurface);

  SO_NODE_ADD_FIELD(numUControlPoints, (0));
  SO_NODE_ADD_FIELD(numVControlPoints, (0));
  SO_NODE_ADD_FIELD(numSControlPoints, (0));
  SO_NODE_ADD_FIELD(numTControlPoints, (0));
  SO_NODE_ADD_FIELD(uKnotVector, (0));
  SO_NODE_ADD_FIELD(vKnotVector, (0));
  SO_NODE_ADD_FIELD(sKnotVector, (0));
  SO_NODE_ADD_FIELD(tKnotVector, (0));

  PRIVATE(this) = new SoNurbsSurfaceP(this);
}

/*!
  Destructor.
*/
SoNurbsSurface::~SoNurbsSurface()
{
  delete PRIVATE(this);
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoNurbsSurface::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoNurbsSurface, SO_FROM_INVENTOR_1);
}

// Documented in superclass.
void
SoNurbsSurface::GLRender(SoGLRenderAction * action)
{
  if (!this->shouldGLRender(action)) return;

  // initialize current material
  SoMaterialBundle mb(action);
  mb.sendFirst();

  SbBool calcnormals = sogl_calculate_nurbs_normals();

  if (!calcnormals) {
    glEnable(GL_AUTO_NORMAL);
  }
  PRIVATE(this)->doNurbs(action, TRUE);
  if (!calcnormals) {
    glDisable(GL_AUTO_NORMAL);
  }

  SoState * state = action->getState();
  if (SoComplexityTypeElement::get(state) == SoComplexityTypeElement::OBJECT_SPACE) {
    SoGLCacheContextElement::shouldAutoCache(state,
                                             SoGLCacheContextElement::DO_AUTO_CACHE);
  }
}

/*!
  Calculates the bounding box of all control points, and sets the center to
  the average of these points.
*/
void
SoNurbsSurface::computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center)
{
  SoState * state = action->getState();
  const SoCoordinateElement * coordelem =
    SoCoordinateElement::getInstance(state);

  int num =
    this->numUControlPoints.getValue() * this->numVControlPoints.getValue();
  assert(num <= coordelem->getNum());

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
    for (int i = 0; i< num; i++) {
      SbVec4f tmp = coords[i];
      if (tmp[3] != 0.0f) {
        float mul = 1.0f / tmp[3];
        tmp[0] *= mul;
        tmp[1] *= mul;
        tmp[2] *= mul;
      }
      acccenter += SbVec3f(tmp[0], tmp[1], tmp[2]);
      box.extendBy(SbVec3f(tmp[0], tmp[1], tmp[2]));
    }
  }
  if (num > 0) center = acccenter / float(num);
}

// Documented in superclass.
void
SoNurbsSurface::rayPick(SoRayPickAction * action)
{
  if (!this->shouldRayPick(action)) return;

  if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
    SoShape::rayPick(action); // do normal generatePrimitives() pick
  }
  else {
    static SbBool firstpick = TRUE;
    if (firstpick) {
      firstpick = FALSE;
      SoDebugError::postWarning("SoNurbsSurface::rayPick",
                                "Proper NURBS picking requires\n"
                                "GLU version 1.3. Picking is done on bounding box.");
    }
    SoState * state = action->getState();
    state->push();
    SoPickStyleElement::set(state, this, SoPickStyleElement::BOUNDING_BOX);
    (void)this->shouldRayPick(action); // this will cause a pick on bbox
    state->pop();
  }
}

// Doc in superclass.
void
SoNurbsSurface::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  // for now, just generate primitives to count. Very slow, of course.
  SoShape::getPrimitiveCount(action);
}

/*!
  This method is part of the original SGI Inventor API, but not
  implemented in Coin, as it looks like a method that should probably
  have been private in Open Inventor.
*/
void
SoNurbsSurface::sendPrimitive(SoAction *,  SoPrimitiveVertex *)
{
  COIN_OBSOLETED();
}

// Documented in superclass.
void
SoNurbsSurface::generatePrimitives(SoAction * action)
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
      PRIVATE(this)->doNurbs(action, FALSE);
      cc_glglue_context_reinstate_previous(PRIVATE(this)->offscreenctx);
    }
  }
}

// Documented in superclass.
SoDetail *
SoNurbsSurface::createTriangleDetail(SoRayPickAction * /* action */,
                                     const SoPrimitiveVertex * /*v1*/,
                                     const SoPrimitiveVertex * /*v2*/,
                                     const SoPrimitiveVertex * /*v3*/,
                                     SoPickedPoint * /* pp */)
{
  return NULL;
}

typedef SoNurbsP<SoNurbsSurface>::coin_nurbs_cbdata coin_ns_cbdata;

//
// render or generate the NURBS surface
//
void
SoNurbsSurfaceP::doNurbs(SoAction * action, const SbBool glrender)
{
  if (GLUWrapper()->available == 0 || !GLUWrapper()->gluNewNurbsRenderer) {
#if COIN_DEBUG
    static int first = 1;
    if (first) {
      SoDebugError::postInfo("SoIndexedNurbsCurveP::doNurbs",
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
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_BEGIN_DATA, (gluNurbsCallback_cb_t)SoNurbsP<SoNurbsSurface>::tessBegin);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_TEXTURE_COORD_DATA, (gluNurbsCallback_cb_t)SoNurbsP<SoNurbsSurface>::tessTexCoord);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_NORMAL_DATA,  (gluNurbsCallback_cb_t)SoNurbsP<SoNurbsSurface>::tessNormal);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_VERTEX_DATA,  (gluNurbsCallback_cb_t)SoNurbsP<SoNurbsSurface>::tessVertex);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_END_DATA,  (gluNurbsCallback_cb_t)SoNurbsP<SoNurbsSurface>::tessEnd);
    }
  }

  // NB, don't move this structure inside the if-statement. It needs
  // to be here so that the callbacks from sogl_render_nurbs_surface()
  // have a valid pointer to the structure.
  coin_ns_cbdata cbdata(action, PUBLIC(this),
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

  int displaymode = (int) GLU_FILL;
  if (glrender) {
    switch (SoDrawStyleElement::get(action->getState())) {
    case SoDrawStyleElement::LINES:
      displaymode = (int) GLU_OUTLINE_POLYGON;
      break;
    case SoDrawStyleElement::POINTS:
      // not possible to draw NURBS as points using GLU...
      displaymode = (int) GLU_OUTLINE_PATCH;
      break;
    default:
      break;
    }
  }
  GLUWrapper()->gluNurbsProperty(this->nurbsrenderer, (GLenum) GLU_DISPLAY_MODE, (GLfloat) displaymode);

  sogl_render_nurbs_surface(action, PUBLIC(this), this->nurbsrenderer,
                            PUBLIC(this)->numUControlPoints.getValue(),
                            PUBLIC(this)->numVControlPoints.getValue(),
                            PUBLIC(this)->uKnotVector.getValues(0),
                            PUBLIC(this)->vKnotVector.getValues(0),
                            PUBLIC(this)->uKnotVector.getNum(),
                            PUBLIC(this)->vKnotVector.getNum(),
                            PUBLIC(this)->numSControlPoints.getValue(),
                            PUBLIC(this)->numTControlPoints.getValue(),
                            PUBLIC(this)->sKnotVector.getValues(0),
                            PUBLIC(this)->tKnotVector.getValues(0),
                            PUBLIC(this)->sKnotVector.getNum(),
                            PUBLIC(this)->tKnotVector.getNum(),
                            glrender);
}


#undef PRIVATE
#undef PUBLIC
