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
  \class SoIndexedNurbsSurface SoIndexedNurbsSurface.h Inventor/nodes/SoIndexedNurbsSurface.h
  \brief The SoIndexedNurbsSurface class can be used to render NURBS surfaces.

  \ingroup coin_nodes

  It is very similar to the SoNurbsSurface class, but control points
  can be specified using indices.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    IndexedNurbsSurface {
        numUControlPoints 0
        numVControlPoints 0
        coordIndex 0
        uKnotVector 0
        vKnotVector 0
        numSControlPoints 0
        numTControlPoints 0
        textureCoordIndex -1
        sKnotVector 0
        tKnotVector 0
    }
  \endcode

*/
// FIXME: more class doc. Usage example! 20011220 mortene.

#include <Inventor/nodes/SoIndexedNurbsSurface.h>
#include "SoNurbsP.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/elements/SoPickStyleElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoComplexityTypeElement.h>
#include <Inventor/system/gl.h>

#include "coindefs.h" // COIN_OBSOLETED()
#include "glue/GLUWrapper.h"
#include "nodes/SoSubNodeP.h"
#include "rendering/SoGLNurbs.h"
#include "rendering/SoGL.h"

/*!
  \var SoSFInt32 SoIndexedNurbsSurface::numUControlPoints
  Number of control points in the U direction.
*/
/*!
  \var SoSFInt32 SoIndexedNurbsSurface::numVControlPoints
  Number of control points in the V direction.
*/
/*!
  \var SoSFInt32 SoIndexedNurbsSurface::numSControlPoints
  Number of control points in the S direction.
*/
/*!
  \var SoSFInt32 SoIndexedNurbsSurface::numTControlPoints
  Number of control points in the T direction.
*/
/*!
  \var SoMFFloat SoIndexedNurbsSurface::uKnotVector
  The Bezier knot vector for the U direction.
*/
/*!
  \var SoMFFloat SoIndexedNurbsSurface::vKnotVector
  The Bezier knot vector for the V direction.
*/
/*!
  \var SoMFFloat SoIndexedNurbsSurface::sKnotVector
  The Bezier knot vector for the S direction.
*/
/*!
  \var SoMFFloat SoIndexedNurbsSurface::tKnotVector
  The Bezier knot vector for the T direction.
*/

/*!
  \var SoMFInt32 SoIndexedNurbsSurface::coordIndex
  The coordinate control point indices.
*/
/*!
  \var SoMFInt32 SoIndexedNurbsSurface::textureCoordIndex
  The texture coordinate control point indices.
*/

// *************************************************************************

class SoIndexedNurbsSurfaceP {
public:
  SoIndexedNurbsSurfaceP(SoIndexedNurbsSurface * m)
  {
    this->owner = m;
    this->nurbsrenderer = NULL;
    this->offscreenctx = NULL;
  }

  ~SoIndexedNurbsSurfaceP()
  {
    if (this->offscreenctx) { cc_glglue_context_destruct(this->offscreenctx); }
    if (this->nurbsrenderer) {
      GLUWrapper()->gluDeleteNurbsRenderer(this->nurbsrenderer);
    }
  }

  void * offscreenctx;
  void * nurbsrenderer;

  void doNurbs(SoAction * action, const SbBool glrender);

private:
  SoIndexedNurbsSurface * owner;
};

#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->owner)

// *************************************************************************

SO_NODE_SOURCE(SoIndexedNurbsSurface);

/*!
  Constructor.
*/
SoIndexedNurbsSurface::SoIndexedNurbsSurface()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoIndexedNurbsSurface);

  SO_NODE_ADD_FIELD(numUControlPoints, (0));
  SO_NODE_ADD_FIELD(numVControlPoints, (0));
  SO_NODE_ADD_FIELD(coordIndex, (0));
  SO_NODE_ADD_FIELD(uKnotVector, (0));
  SO_NODE_ADD_FIELD(vKnotVector, (0));
  SO_NODE_ADD_FIELD(numSControlPoints, (0));
  SO_NODE_ADD_FIELD(numTControlPoints, (0));
  SO_NODE_ADD_FIELD(textureCoordIndex, (-1));
  SO_NODE_ADD_FIELD(sKnotVector, (0));
  SO_NODE_ADD_FIELD(tKnotVector, (0));

  PRIVATE(this) = new SoIndexedNurbsSurfaceP(this);
}

/*!
  Destructor.
*/
SoIndexedNurbsSurface::~SoIndexedNurbsSurface()
{
  delete PRIVATE(this);
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoIndexedNurbsSurface::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoIndexedNurbsSurface, SO_FROM_INVENTOR_1);
}

/*!
  Calculates the bounding box of all control points and sets the center
  to the average of these points.
*/
void
SoIndexedNurbsSurface::computeBBox(SoAction * action,
                                   SbBox3f & box, SbVec3f & center)
{
  SoState * state = action->getState();
  const SoCoordinateElement * coordelem =
    SoCoordinateElement::getInstance(state);

  const int num = this->coordIndex.getNum();
  const int32_t * idxptr = this->coordIndex.getValues(0);

  box.makeEmpty();
  SbVec3f acccenter(0.0f, 0.0f, 0.0f);
  SbVec3f tmp3D;

  if (coordelem->is3D()) {
    const SbVec3f * coords = coordelem->getArrayPtr3();
    assert(coords);
    for (int i = 0; i < num; i++) {
      tmp3D = coords[idxptr[i]];
      box.extendBy(tmp3D);
      acccenter += tmp3D;
    }
  }
  else {
    const SbVec4f * coords = coordelem->getArrayPtr4();
    assert(coords);
    for (int i = 0; i< num; i++) {
      coords[idxptr[i]].getReal(tmp3D);
      box.extendBy(tmp3D);
      acccenter += tmp3D;
    }
  }
  if (num) center = acccenter / float(num);
}

// Doc in superclass.
void
SoIndexedNurbsSurface::GLRender(SoGLRenderAction * action)
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
    SoGLCacheContextElement::incNumShapes(state);
  }
}

// Doc in superclass.
void
SoIndexedNurbsSurface::rayPick(SoRayPickAction * action)
{
  if (!this->shouldRayPick(action)) return;
  if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
    SoShape::rayPick(action); // do normal generatePrimitives() pick
  }
  else {
    static SbBool firstpick = TRUE;
    if (firstpick) {
      firstpick = FALSE;
      SoDebugError::postWarning("SoIndexedNurbsSurface::rayPick",
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

// Documented in superclass.
void
SoIndexedNurbsSurface::getPrimitiveCount(SoGetPrimitiveCountAction * action)
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
SoIndexedNurbsSurface::sendPrimitive(SoAction * ,  SoPrimitiveVertex *)
{
  COIN_OBSOLETED();
}

// Documented in superclass.
void
SoIndexedNurbsSurface::generatePrimitives(SoAction * action)
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
SoIndexedNurbsSurface::createTriangleDetail(SoRayPickAction * /* action */,
                                            const SoPrimitiveVertex * /*v1*/,
                                            const SoPrimitiveVertex * /*v2*/,
                                            const SoPrimitiveVertex * /*v3*/,
                                            SoPickedPoint * /* pp */)
{
  return NULL;
}

typedef SoNurbsP<SoIndexedNurbsSurface>::coin_nurbs_cbdata coin_ins_cbdata;

void
SoIndexedNurbsSurfaceP::doNurbs(SoAction * action, const SbBool glrender)
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

  if (!PUBLIC(this)->coordIndex.getNum()) return;

  if (this->nurbsrenderer == NULL) {
    this->nurbsrenderer = GLUWrapper()->gluNewNurbsRenderer();

    if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_BEGIN_DATA, (gluNurbsCallback_cb_t)SoNurbsP<SoIndexedNurbsSurface>::tessBegin);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_TEXTURE_COORD_DATA, (gluNurbsCallback_cb_t)SoNurbsP<SoIndexedNurbsSurface>::tessTexCoord);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_NORMAL_DATA,  (gluNurbsCallback_cb_t)SoNurbsP<SoIndexedNurbsSurface>::tessNormal);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_VERTEX_DATA,  (gluNurbsCallback_cb_t)SoNurbsP<SoIndexedNurbsSurface>::tessVertex);
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_END_DATA,  (gluNurbsCallback_cb_t)SoNurbsP<SoIndexedNurbsSurface>::tessEnd);
    }
  }

  // NB, don't move this structure inside the if-statement. It needs
  // to be here so that the callbacks from sogl_render_nurbs_surface()
  // have a valid pointer to the structure.
  coin_ins_cbdata cbdata(action, PUBLIC(this),
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

  SbBool texindex =
    PUBLIC(this)->textureCoordIndex.getNum() &&
    PUBLIC(this)->textureCoordIndex[0] >= 0;

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
                            glrender,
                            PUBLIC(this)->coordIndex.getNum(),
                            PUBLIC(this)->coordIndex.getValues(0),
                            texindex ? PUBLIC(this)->textureCoordIndex.getNum() : 0,
                            texindex ? PUBLIC(this)->textureCoordIndex.getValues(0) : NULL);
}


#undef PRIVATE
#undef PUBLIC
