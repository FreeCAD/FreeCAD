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
  \class SoExtSelection SoExtSelection.h Inventor/nodes/SoExtSelection.h
  \brief The SoExtSelection class can be used for extended selection functionality.

  \ingroup coin_nodes

  This class enables you to select geometry by specifying a lasso (a
  polygon) or a rectangle on screen. When objects are selected, you'll
  receive the same callbacks as for the SoSelection node.

  The application programmer interface of this class is somewhat
  complex, due to its non-trivial functionality. To see an \e
  extensive usage example of the SoExtSelection node, we advise you to
  go look at the "extselection" example application in the "nodes/"
  directory of the SoGuiExamples Mercurial repository. Further information and
  links for downloading and building this module should be available at <a
  href="https://github.com/coin3d/soguiexamples">github.com/coin3d/soguiexamples</a>.

  This node class is an extension versus the original SGI Inventor
  v2.1 API. It is based on the API of VSG (was TGS) Inventor's SoExtSelection,
  and we aim to be fully compatible with this node to enable users to
  switch between using Coin and VSG Inventor.  Please contact us if
  you find discrepancies between Coin's SoExtSelection and VSG's
  SoExtSelection node.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    ExtSelection {
        renderCaching AUTO
        boundingBoxCaching AUTO
        renderCulling AUTO
        pickCulling AUTO
        policy SHIFT
        lassoType NOLASSO
        lassoPolicy FULL_BBOX
        lassoMode ALL_SHAPES
    }
  \endcode

  \since TGS Inventor 2.5
  \since Coin 1.0
*/

// *************************************************************************

#include <Inventor/nodes/SoExtSelection.h>

#include <cfloat>
#include <cmath>
#include <climits>
#include <cstring> // memset()

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/C/glue/gl.h>
#include <Inventor/C/tidbits.h> // coin_getenv()
#include <Inventor/SbBox2s.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbTesselator.h>
#include <Inventor/SbTime.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SoOffscreenRenderer.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoPointSizeElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/events/SoEvent.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/lists/SoCallbackList.h>
#include <Inventor/lists/SoPathList.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoVertexShape.h>
#include <Inventor/sensors/SoTimerSensor.h>
#include <Inventor/misc/SoGLDriverDatabase.h>

#include "nodes/SoSubNodeP.h"
#include "coindefs.h" // COIN_OBSOLETED()

// *************************************************************************

/*!
  \enum SoExtSelection::LassoType
  Enum for type of lasso selection.
*/
/*!
  \var SoExtSelection::LassoType SoExtSelection::NOLASSO
  Makes this node behave like a normal SoSelection node.
*/
/*!
  \var SoExtSelection::LassoType SoExtSelection::LASSO

  Select objects using a lasso. Selections can be aborted by the
  end-user by hitting the \c END key on the keyboard.
*/
/*!
  \var SoExtSelection::LassoType SoExtSelection::RECTANGLE

  Select objects using a rectangle. Selections can be aborted by the
  end-user by hitting the \c END key on the keyboard.
*/

/*!
  \enum SoExtSelection::LassoPolicy
  Enum for specifying how objects are selected.
*/

/*!
  \var SoExtSelection::LassoPolicy SoExtSelection::FULL_BBOX
  The entire bounding box must be inside the lasso/rectangle.
*/
/*!
  \var SoExtSelection::LassoPolicy SoExtSelection::PART_BBOX
  Some part of the bounding box must intersect the lasso/rectangle.
*/
/*!
  \var SoExtSelection::LassoPolicy SoExtSelection::FULL
  All primitives must be completely inside the lasso/rectangle.
*/
/*!
  \var SoExtSelection::LassoPolicy SoExtSelection::PART
  Some primitive must intersect the lasso/rectangle.
*/

/*!
  \enum SoExtSelection::LassoMode
  Enum for specifying selection mode.
*/
/*!
  \var SoExtSelection::LassoMode SoExtSelection::ALL_SHAPES
  All primitives inside the lasso/rectangle will be selected.
*/
/*!
  \var SoExtSelection::LassoMode SoExtSelection::VISIBLE_SHAPES
  All \e visible primitives inside the lasso/rectangle will be selected.
*/


/*!
  \var SoSFEnum SoExtSelection::lassoType

  Field for lasso type. Default value is SoExtSelection::NOLASSO.

  Selections with type SoExtSelection::RECTANGLE or
  SoExtSelection::LASSO can be aborted by the end-user by hitting the
  \c END key on the keyboard.
*/
/*!
  \var SoSFEnum SoExtSelection::lassoPolicy
  Field for lasso policy. Default value is FULL_BBOX.
*/

// FIXME: I wonder if this was a later addition to the API, possibly
// both for Coin and TGS Inventor. Investigate. If so, this means it
// must get special handling when exporting .iv-files, with regard to
// what header we can put on the output. See also item #003 in the
// Coin/docs/todo.txt file. 20030529 mortene.
/*!
  \var SoSFEnum SoExtSelection::lassoMode

  Field for lasso mode. Default value is ALL_SHAPES.

  Set this field to VISIBLE_SHAPES to make only the primitives visible
  from the current viewpoint be selected.
*/

// *************************************************************************

/*
  FIXME, KNOWN BUGS:
  ==================

  * Sometimes my GTK-lib (i think) gets corrupt (most likely because
    of a bug in Gimp 1.2).  This causes the 'glGetInteger' to fail
    causing incorrect 'maximumcolorcounter' value. This often leads to
    infinite loop and crash when in VISIBLE_SHAPE mode.  Other
    symptoms are 'GL_OUT_OF_MEMORY' error, Mozilla suddenly dies
    etc. A restart of X11 is needed. I am using the Mesa
    drivers. 2002-08-02 handegar.

    UPDATE 2004-10-27 mortene: I found a grave overflow error in the
    calculation of maximumcolorcounter from the values returned from
    glGetInteger() -- could the problem mentioned above actually be
    due to this? The explanation given above doesn't seem very
    likely...

  * Offcreenrendering comes out wrong if offscreenrenderer decides to
    render to multiple subscreens (due to size limitations). the
    'offscreencolorcounter' is increased for each pass on each
    subscreen which leads to inconsistency on the second
    traversalpass. 2002-08-12 handegar.
*/


// Debug define. Used to force lower color resolution for testing
// purposes. The number of available colors in the offscreen buffer
// for visible shape testing will be 2^COLORBITS.
//
//#define COLORBITS 3

// *************************************************************************

#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->master)

class SoExtSelectionP {
public:
  SoExtSelectionP(SoExtSelection * masterptr)
    : runningselection(masterptr)
  {
    PUBLIC(this) = masterptr;
  }

  SbColor lassocolor;
  float lassowidth;
  SbBool lassopatternanimate;
  unsigned short lassopattern;

  SbVec2s requestedsize;

  SoCallbackAction * cbaction;

  SbViewportRegion curvp;

  static SbBool debug(void);

  void handleEventRectangle(SoHandleEventAction * action);
  void handleEventLasso(SoHandleEventAction * action);

  void addTriangleToOffscreenBuffer(SoCallbackAction * action,
                                    const SoPrimitiveVertex * v1,
                                    const SoPrimitiveVertex * v2,
                                    const SoPrimitiveVertex * v3,
                                    SbBool renderAsBlack);

  void addLineToOffscreenBuffer(SoCallbackAction * action,
                                const SoPrimitiveVertex * v1,
                                const SoPrimitiveVertex * v2,
                                SbBool renderAsBlack);

  void addPointToOffscreenBuffer(SoCallbackAction * action,
                                 const SoPrimitiveVertex * v1,
                                 SbBool renderAsBlack);

  int scanOffscreenBuffer(SoNode * root);
  void addVisitedPath(const SoPath *path);

  SbBool checkOffscreenRendererCapabilities();

  static void offscreenRenderCallback(void * userdata, SoAction * action);

  static void offscreenRenderLassoCallback(void * userdata, SoAction * action);

  static void timercallback(void * data, SoSensor * sensor);

  static SoCallbackAction::Response preShapeCallback(void * data,
                                                     SoCallbackAction * action,
                                                     const SoNode * node);

  static SoCallbackAction::Response postShapeCallback(void * data,
                                                      SoCallbackAction * action,
                                                      const SoNode * node);

  static SoCallbackAction::Response cameraCB(void * data,
                                             SoCallbackAction * action,
                                             const SoNode * node);

  SoCallbackAction::Response testShape(SoCallbackAction * action, const SoShape * shape);

  SoCallbackAction::Response testBBox(SoCallbackAction * action,
                                      const SbMatrix & projmatrix,
                                      const SoShape * shape,
                                      const SbBox2s & lassorect,
                                      const SbBool full);

  SoCallbackAction::Response testPrimitives(SoCallbackAction * action,
                                            const SbMatrix & projmatrix,
                                            const SoShape * shape,
                                            const SbBox2s & lassorect,
                                            const SbBool full);

  static void offscreenLassoTesselatorCallback(void * v0, void * v1, void * v2, void * userdata);

  static void triangleCB(void * userData,
                         SoCallbackAction * action,
                         const SoPrimitiveVertex * v1,
                         const SoPrimitiveVertex * v2,
                         const SoPrimitiveVertex * v3);

  static void lineSegmentCB(void * userData,
                            SoCallbackAction * action,
                            const SoPrimitiveVertex * v1,
                            const SoPrimitiveVertex * v2);

  static void pointCB(void * userData,
                      SoCallbackAction * action,
                      const SoPrimitiveVertex * v);

  void selectAndReset(SoHandleEventAction * action);
  void performSelection(SoHandleEventAction * action);

  void validateViewportBBox(SbBox2s & bbox, 
                            const SbVec2s & vpsize);

  // This keeps track of the state of a rectangle or lasso selection
  // operation.
  struct SelectionState {
    enum Mode { NONE, RECTANGLE, LASSO };

    Mode mode;
    SbList<SbVec2s> coords;
    SoTimerSensor * updatetimer;

    SelectionState(SoExtSelection * t)
    {
      this->updatetimer = new SoTimerSensor(&SoExtSelectionP::timercallback, t);
      this->updatetimer->setInterval(SbTime(0.1));//Aim for 10 FPS
      //Not setting basetime here, as drift in the timer doesn't matter for the animation
      this->reset();
    }

    ~SelectionState()
    {
      delete this->updatetimer;
    }

    void start(Mode m, const SbVec2s & mousecoords)
    {
      assert(this->coords.getLength() == 0);
      this->coords.append(mousecoords);
      this->coords.append(mousecoords);
      this->mode = m;
      if (!this->updatetimer->isScheduled()) { this->updatetimer->schedule(); }
    }

    void reset(void)
    {
      if (this->updatetimer->isScheduled()) { this->updatetimer->unschedule(); }
      this->mode = SelectionState::NONE;
      this->coords.truncate(0);
    }
  } runningselection;

  // Note: Microsoft Visual C++ 6.0 needs to have a type definition
  // and an explicit variable declaration, just using
  //     struct { ... } structname;
  // won't do.
  typedef struct
  {
    SbMatrix projmatrix;
    SbBool fulltest;
    SbBox2s lassorect;
    SbBool hit;
    SbVec2s vporg;
    SbVec2s vpsize;
    SbBool abort;
    SbBool allhit;
    SbBool onlyrect;
    SbBool allshapes;
    SbBool hasgeometry;
  } primcbdata_t;
  primcbdata_t primcbdata;

  void doSelect(const SoPath * path);
  void selectPaths(void);
  SoLassoSelectionFilterCB * filterCB;
  void * filterCBData;

  SoExtSelectionTriangleCB * triangleFilterCB;
  void * triangleFilterCBData;
  SoExtSelectionLineSegmentCB * lineFilterCB;
  void * lineFilterCBData;
  SoExtSelectionPointCB * pointFilterCB;
  void * pointFilterCBData;

  SbBool callfiltercbonlyifselectable;
  SbBool wasshiftdown;

  SoHandleEventAction *offscreenaction;
  SbViewVolume offscreenviewvolume;
  int offscreencolorcounter;
  int offscreencolorcounterpasses;
  unsigned int offscreenskipcounter;
  SbBool offscreencolorcounteroverflow;
  SoOffscreenRenderer * renderer;
  SoOffscreenRenderer * lassorenderer;

  SbBool lassostencilisdrawed;
  SbBool applyonlyonselectedtriangles;

  unsigned int maximumcolorcounter;

  SbBool has3DTextures;

  unsigned char *visibletrianglesbitarray;

  SoNode *offscreenheadnode;
  unsigned int drawcallbackcounter;
  unsigned int drawcounter;
  SoPathList *visitedshapepaths;
  SbBool somefacesvisible;
  SoPathList dummypathlist;

private:
  SoExtSelection * master;
};

// *************************************************************************

SbBool
SoExtSelectionP::debug(void)
{
  static int dbg = -1;
  if (dbg == -1) {
    const char * env = coin_getenv("COIN_DEBUG_SOEXTSELECTION");
    dbg = env && (atoi(env) > 0);
  }
  return dbg ? TRUE : FALSE;
}

// *************************************************************************

//
// Faster line segment intersection by Franklin Antonio, from Graphics
// Gems III.
//

static SbBool
line_line_intersect(const SbVec2s &p00, const SbVec2s & p01,
                    const SbVec2s &p10, const SbVec2s & p11)
{
  /* The SAME_SIGNS macro assumes arithmetic where the exclusive-or    */
  /* operation will work on sign bits.  This works for twos-complement,*/
  /* and most other machine arithmetic.                                */
#define SAME_SIGNS( a, b ) \
        (((int) ((unsigned int) a ^ (unsigned int) b)) >= 0 )

#define DONT_INTERSECT FALSE
#define COLINEAR TRUE
#define DO_INTERSECT TRUE

  int x1 = p00[0];
  int y1 = p00[1];
  int x2 = p01[0];
  int y2 = p01[1];

  int x3 = p10[0];
  int y3 = p10[1];
  int x4 = p11[0];
  int y4 = p11[1];

  int Ax,Bx,Cx,Ay,By,Cy,d,e,f;
  int x1lo,x1hi,y1lo,y1hi;

  Ax = x2-x1;
  Bx = x3-x4;

  if(Ax < 0) {                                          /* X bound box test*/
    x1lo = x2; x1hi = x1;
  }
  else {
    x1hi = x2; x1lo = x1;
  }
  if(Bx > 0) {
    if(x1hi < x4 || x3 < x1lo) return DONT_INTERSECT;
  }
  else {
    if(x1hi < x3 || x4 < x1lo) return DONT_INTERSECT;
  }

  Ay = y2-y1;
  By = y3-y4;

  if (Ay < 0) {                                         /* Y bound box test*/
    y1lo = y2; y1hi = y1;
  }
  else {
    y1hi = y2; y1lo = y1;
  }
  if (By > 0) {
    if (y1hi < y4 || y3 < y1lo) return DONT_INTERSECT;
  }
  else {
    if (y1hi < y3 || y4 < y1lo) return DONT_INTERSECT;
  }

  Cx = x1-x3;
  Cy = y1-y3;
  d = By*Cx - Bx*Cy;                                    /* alpha numerator*/
  f = Ay*Bx - Ax*By;                                    /* both denominator*/

  if (f > 0) {                                          /* alpha tests*/
    if (d < 0 || d > f) return DONT_INTERSECT;
  }
  else {
    if (d > 0 || d < f) return DONT_INTERSECT;
  }

  e = Ax*Cy - Ay*Cx;                                    /* beta numerator*/
  if (f > 0) {                                          /* beta tests*/
    if (e < 0 || e > f) return DONT_INTERSECT;
  }
  else {
    if (e > 0 || e < f) return DONT_INTERSECT;
  }

  /*compute intersection coordinates*/
  if (f == 0) return COLINEAR;
#if 0 // we don't need the intersection point, disabled
  int num, offset;
  int x, y;
  num = d*Ax;                                           /* numerator */
  offset = SAME_SIGNS(num,f) ? f/2 : -f/2;              /* round direction*/
  x = x1 + (num+offset) / f;                            /* intersection x */

  num = d*Ay;
  offset = SAME_SIGNS(num,f) ? f/2 : -f/2;
  y = y1 + (num+offset) / f;                            /* intersection y */
#endif // disabled code

  return DO_INTERSECT;
#undef COLINEAR
#undef DONT_INTERSECT
#undef DO_INTERSECT
#undef SAME_SIGNS
}

// The following code is by Randolph Franklin,
// it returns 1 for interior points and 0 for exterior points.
// http://astronomy.swin.edu.au/pbourke/geometry/insidepoly/

static SbBool
point_in_poly(const SbList <SbVec2s> & coords, const SbVec2s & point)
{
  int i, j;
  SbBool c = FALSE;
  int npol = coords.getLength();
  float x = (float) point[0];
  float y = (float) point[1];
  SbVec2f pi, pj;

  for (i = 0, j = npol-1; i < npol; j = i++) {

    pi[0] = (float) coords[i][0];
    pi[1] = (float) coords[i][1];
    pj[0] = (float) coords[j][0];
    pj[1] = (float) coords[j][1];

    if ((((pi[1] <= y) && (y < pj[1])) ||
         ((pj[1] <= y) && (y < pi[1]))) &&
        (x < (pj[0] - pi[0]) * (y - pi[1]) / (pj[1] - pi[1]) + pi[0]))
      c = !c;
  }
  return c;
}

// do a bounding box rejection test before calling this method. It's not fast,
// but testing will usually (always) be done on polygon vs triangle in
// which case it should be pretty fast.
static SbBool
poly_poly_intersect(const SbList <SbVec2s> & poly1,
                    const SbList <SbVec2s> & poly2)
{
  int i;
  int n1 = poly1.getLength();
  int n2 = poly2.getLength();

  if (n1 < n2) {
    for (i = 0; i < n1; i++) {
      if (point_in_poly(poly2, poly1[i])) return TRUE;
    }
    for (i = 0; i < n2; i++) {
      if (point_in_poly(poly1, poly2[i])) return TRUE;
    }
  }
  else {
    for (i = 0; i < n2; i++) {
      if (point_in_poly(poly1, poly2[i])) return TRUE;
    }
    for (i = 0; i < n1; i++) {
      if (point_in_poly(poly2, poly1[i])) return TRUE;
    }
  }
  // warning O(n^2)
  SbVec2s prev1 = poly1[n1-1];
  for (i = 0; i < n1; i++) {
    SbVec2s prev2 = poly2[n2-1];
    for (int j = 0; j < n2; j++) {
      if (line_line_intersect(prev1, poly1[i], prev2, poly2[j])) return TRUE;
      prev2 = poly2[j];
    }
    prev1 = poly1[i];
  }
  return FALSE;
}

static SbBool
poly_line_intersect(const SbList <SbVec2s> & poly,
                    const SbVec2s & p0,
                    const SbVec2s & p1,
                    const SbBool checkcontained = TRUE)
{
  if (checkcontained && point_in_poly(poly, p0)) return TRUE;
  if (checkcontained && point_in_poly(poly, p1)) return TRUE;

  int n = poly.getLength();
  SbVec2s prev = poly[n-1];
  for (int i = 0; i < n; i++) {
    if (line_line_intersect(prev, poly[i], p0, p1)) return TRUE;
    prev = poly[i];
  }
  return FALSE;
}

// do a bounding box rejection test before calling this method
static SbBool
poly_tri_intersect(const SbList <SbVec2s> & poly,
                   const SbVec2s & v0,
                   const SbVec2s & v1,
                   const SbVec2s & v2)
{
  SbList <SbVec2s> poly2;
  poly2.append(v0);
  poly2.append(v1);
  poly2.append(v2);
  return poly_poly_intersect(poly, poly2);
}

// only used by polyprojboxintersect()
static SbBool
test_quad_intersect(const SbList <SbVec2s> & poly,
                    const SbVec2s & p0,
                    const SbVec2s & p1,
                    const SbVec2s & p2,
                    const SbVec2s & p3)
{
  // test if front facing:
  SbVec2s v0 = p1-p0;
  SbVec2s v1 = p3-p0;
  int crossz = v0[0]*v1[1] - v0[1]*v1[0];
  if (crossz > 0) {
    SbList <SbVec2s> poly2;
    poly2.append(p0);
    poly2.append(p1);
    poly2.append(p2);
    poly2.append(p3);
    return poly_poly_intersect(poly, poly2);
  }
  return FALSE;
}

// do a bounding box rejection test before calling this method
static SbBool
poly_projbox_intersect(const SbList <SbVec2s> & poly,
                       const SbVec2s * projpts)
{
  // test all size quads in the box
  if (test_quad_intersect(poly, projpts[0], projpts[1],
                          projpts[3], projpts[2])) return TRUE;
  if (test_quad_intersect(poly, projpts[1], projpts[5],
                          projpts[7], projpts[3])) return TRUE;
  if (test_quad_intersect(poly, projpts[2], projpts[3],
                          projpts[7], projpts[6])) return TRUE;
  if (test_quad_intersect(poly, projpts[4], projpts[0],
                          projpts[2], projpts[6])) return TRUE;
  if (test_quad_intersect(poly, projpts[4], projpts[5],
                          projpts[1], projpts[0])) return TRUE;
  if (test_quad_intersect(poly, projpts[6], projpts[7],
                          projpts[5], projpts[4])) return TRUE;

  return FALSE;
}

// *************************************************************************

SO_NODE_SOURCE(SoExtSelection);

// *************************************************************************

/*!
  Constructor.
*/
SoExtSelection::SoExtSelection(void)
{
  PRIVATE(this) = new SoExtSelectionP(this);

  SO_NODE_INTERNAL_CONSTRUCTOR(SoExtSelection);

  SO_NODE_ADD_FIELD(lassoType, (SoExtSelection::NOLASSO));
  SO_NODE_ADD_FIELD(lassoPolicy, (SoExtSelection::FULL_BBOX));
  SO_NODE_ADD_FIELD(lassoMode, (SoExtSelection::ALL_SHAPES));

  SO_NODE_DEFINE_ENUM_VALUE(LassoType, NOLASSO);
  SO_NODE_DEFINE_ENUM_VALUE(LassoType, LASSO);
  SO_NODE_DEFINE_ENUM_VALUE(LassoType, RECTANGLE);
  SO_NODE_SET_SF_ENUM_TYPE(lassoType, LassoType);

  SO_NODE_DEFINE_ENUM_VALUE(LassoPolicy, FULL_BBOX);
  SO_NODE_DEFINE_ENUM_VALUE(LassoPolicy, PART_BBOX);
  SO_NODE_DEFINE_ENUM_VALUE(LassoPolicy, FULL);
  SO_NODE_DEFINE_ENUM_VALUE(LassoPolicy, PART);
  SO_NODE_SET_SF_ENUM_TYPE(lassoPolicy, LassoPolicy);

  SO_NODE_DEFINE_ENUM_VALUE(LassoMode, VISIBLE_SHAPES);
  SO_NODE_DEFINE_ENUM_VALUE(LassoMode, ALL_SHAPES);
  SO_NODE_SET_SF_ENUM_TYPE(lassoMode, LassoMode);

  PRIVATE(this)->cbaction = new SoCallbackAction();

  PRIVATE(this)->cbaction->addPreCallback(SoShape::getClassTypeId(),
                                          SoExtSelectionP::preShapeCallback,
                                          (void *) this);

  PRIVATE(this)->cbaction->addPostCallback(SoShape::getClassTypeId(),
                                           SoExtSelectionP::postShapeCallback,
                                           (void *) this);

  PRIVATE(this)->cbaction->addTriangleCallback(SoShape::getClassTypeId(),
                                               SoExtSelectionP::triangleCB,
                                               (void*) this);

  PRIVATE(this)->cbaction->addLineSegmentCallback(SoShape::getClassTypeId(),
                                                  SoExtSelectionP::lineSegmentCB,
                                                  (void*) this);

  PRIVATE(this)->cbaction->addPointCallback(SoShape::getClassTypeId(),
                                            SoExtSelectionP::pointCB,
                                            (void*) this);

  PRIVATE(this)->cbaction->addPostCallback(SoCamera::getClassTypeId(),
                                           SoExtSelectionP::cameraCB,
                                           (void *) this);


  // some init (just to be sure?)
  PRIVATE(this)->lassocolor = SbColor(1.0f, 1.0f, 1.0f);
  PRIVATE(this)->lassowidth = 1.0f;
  PRIVATE(this)->lassopatternanimate = TRUE;
  PRIVATE(this)->lassopattern = 0xf0f0;

  PRIVATE(this)->filterCB = NULL;
  PRIVATE(this)->triangleFilterCB = NULL;
  PRIVATE(this)->lineFilterCB = NULL;
  PRIVATE(this)->pointFilterCB = NULL;

  PRIVATE(this)->drawcallbackcounter=0;
  PRIVATE(this)->drawcounter=0;
  PRIVATE(this)->visitedshapepaths = new SoPathList();
  PRIVATE(this)->somefacesvisible = FALSE;

  PRIVATE(this)->renderer = NULL;
  PRIVATE(this)->lassorenderer = NULL;

}

/*!
  Destructor.
*/
SoExtSelection::~SoExtSelection()
{
  delete PRIVATE(this)->renderer;
  delete PRIVATE(this)->lassorenderer;
  delete PRIVATE(this)->cbaction;
  delete PRIVATE(this)->visitedshapepaths;
  delete PRIVATE(this);
}

// *************************************************************************

// doc in superclass
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoExtSelection::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoExtSelection, SO_FROM_INVENTOR_1);
}

// *************************************************************************

/*!
  Specifies whether the overlay planes should be used to render the
  lasso.

  This method has been obsoleted in Coin, as most graphics cards comes
  without support for overlay rendering. A better strategy is to just
  "overlay" the lasso graphics on top of the scene after everything
  else has been rendered -- and this is the strategy we apply in Coin.
*/
void
SoExtSelection::useOverlay(SbBool COIN_UNUSED_ARG(overlay))
{
  COIN_OBSOLETED();
}

/*!
  Returns whether overlay planes are used to draw the lasso.

  \sa useOverlay().
*/
SbBool
SoExtSelection::isUsingOverlay(void)
{
  COIN_OBSOLETED();
  return FALSE;
}

/*!
  Returns the scene graph for overlay rendering. Will always return
  NULL in Coin, as this method has been obsoleted.

  (It is probably used in TGS Inventor from the SoXt / SoWin
  libraries' So[Xt|Win]RenderArea class to fetch the overlay graph to
  draw, and as such should be treated as an internal method.)
*/
SoSeparator *
SoExtSelection::getOverlaySceneGraph(void)
{
  COIN_OBSOLETED();
  return NULL;
}

// *************************************************************************

/*!
  Obsoleted in Coin, use SoExtSelection::setLassoColor() instead.
*/
void
SoExtSelection::setOverlayLassoColorIndex(const int COIN_UNUSED_ARG(index))
{
  COIN_OBSOLETED();
}

/*!
  Obsoleted in Coin, use SoExtSelection::getLassoColor() instead.

  \sa setOverlayLassoColorIndex().
*/
int
SoExtSelection::getOverlayLassoColorIndex(void)
{
  COIN_OBSOLETED();
  return 0;
}

// *************************************************************************

/*!
  Sets the lasso/rectangle line color. Default value is (1.0, 1.0,
  1.0).
*/
void
SoExtSelection::setLassoColor(const SbColor & color)
{
  PRIVATE(this)->lassocolor = color;
}

/*!
  Returns the lasso color.
*/
const SbColor &
SoExtSelection::getLassoColor(void)
{
  return PRIVATE(this)->lassocolor;
}

// *************************************************************************

/*!
  Sets the lasso line width. Default value is 1.0.
*/
void
SoExtSelection::setLassoWidth(const float width)
{
  PRIVATE(this)->lassowidth = width;
}

/*!
  Returns the lasso line width.
*/
float
SoExtSelection::getLassoWidth(void)
{
  return PRIVATE(this)->lassowidth;
}

// *************************************************************************

/*!
  Sets the lasso line pattern. Default value is 0xf0f0.
*/
void
SoExtSelection::setOverlayLassoPattern(const unsigned short pattern)
{
  PRIVATE(this)->lassopattern = pattern;
}

/*!
  Returns the lasso line pattern.
*/
unsigned short
SoExtSelection::getOverlayLassoPattern(void)
{
  return PRIVATE(this)->lassopattern;
}

// *************************************************************************

/*!
  Sets whether the lasso should be animated by scrolling
  the line pattern.
*/
void
SoExtSelection::animateOverlayLasso(const SbBool animate)
{
  PRIVATE(this)->lassopatternanimate = animate;
}

/*!
  Returns whether the lasso is set to animate or not.
*/
SbBool
SoExtSelection::isOverlayLassoAnimated(void)
{
  return PRIVATE(this)->lassopatternanimate;
}

// *************************************************************************

void
SoExtSelectionP::handleEventRectangle(SoHandleEventAction * action)
{
  // Make sure the new coord is inside the viewport
  const SbViewportRegion & vpr = action->getViewportRegion();
  const SbVec2s vprsize = vpr.getWindowSize();
  const SoEvent * event = action->getEvent();

  SbVec2s mousecoords = event->getPosition();

  // FIXME: shouldn't this normalization also be done for
  // lasso-select?  or perhaps *not* for rectangle-select? 20050427 mortene.
  mousecoords[0] = mousecoords[0] < vprsize[0] ? mousecoords[0] : vprsize[0];
  mousecoords[1] = mousecoords[1] < vprsize[1] ? mousecoords[1] : vprsize[1];
  // FIXME: this doesn't look entirely correct -- shouldn't
  // SbViewportRegion::getOrigin() be the lower bound? 20050104 mortene.
  mousecoords[0] = mousecoords[0] < 0 ? 0 : mousecoords[0];
  mousecoords[1] = mousecoords[1] < 0 ? 0 : mousecoords[1];

  // mouse click
  if (SO_MOUSE_PRESS_EVENT(event, BUTTON1)) {
    if (this->runningselection.mode == SelectionState::NONE) { // be robust vs two incoming press events without an intervening release
      this->runningselection.start(SelectionState::RECTANGLE, mousecoords);
      action->setHandled();
    }
  }

  if (this->runningselection.mode == SelectionState::RECTANGLE) {
    assert(this->runningselection.coords.getLength() == 2);
    this->runningselection.coords[1] = mousecoords;
    
    if (SO_MOUSE_RELEASE_EVENT(event, BUTTON1)) {
      this->selectAndReset(action);
      action->setHandled();
    }
    // mouse move
    else if ((event->isOfType(SoLocation2Event::getClassTypeId()))) {
      action->setHandled();
      PUBLIC(this)->touch();
    }
  }
}

void
SoExtSelectionP::handleEventLasso(SoHandleEventAction * action)
{
  const SoEvent * event = action->getEvent();
  SbVec2s mousecoords = event->getPosition();

  // mouse click
  if (SO_MOUSE_PRESS_EVENT(event, BUTTON1)) {
    if (this->runningselection.mode == SelectionState::NONE) {
      this->runningselection.start(SelectionState::LASSO, mousecoords);
      action->setHandled();
    }
    else {
      assert(this->runningselection.mode == SelectionState::LASSO);
      const int nrcoords = this->runningselection.coords.getLength();
      assert(nrcoords > 1);
      const SbVec2s vsprev = mousecoords - this->runningselection.coords[nrcoords - 2];
      if ((SbAbs(vsprev[0]) + SbAbs(vsprev[1])) <= 2) {
        // clicked twice on same coord (double click) -> end selection
        this->selectAndReset(action);
        action->setHandled();
      }
      else {
        this->runningselection.coords[nrcoords - 1] = mousecoords;
        this->runningselection.coords.append(mousecoords);
        action->setHandled();
        PUBLIC(this)->touch();
      }
    }
  }

  if (this->runningselection.mode == SelectionState::NONE) { return; }
  assert(this->runningselection.mode == SelectionState::LASSO);
  
  const int nrcoords = this->runningselection.coords.getLength();
  assert(nrcoords > 0);

  // mouse move
  if ((event->isOfType(SoLocation2Event::getClassTypeId()))) {
    this->runningselection.coords[nrcoords - 1] = mousecoords;
    PUBLIC(this)->touch();
    action->setHandled();
  }
  // end selection with right-click
  else if (SO_MOUSE_PRESS_EVENT(event, BUTTON2)) {
    this->runningselection.coords[nrcoords - 1] = mousecoords;
    this->selectAndReset(action);
    action->setHandled();
  }
}

// Documented in superclass.
void
SoExtSelection::handleEvent(SoHandleEventAction * action)
{
  const SoEvent * e = action->getEvent();
  PRIVATE(this)->wasshiftdown = e->wasShiftDown();

  // Behave as SoSelection when lassoType==NOLASSO.
  if ((PRIVATE(this)->runningselection.mode == SoExtSelectionP::SelectionState::NONE) &&
      (this->lassoType.getValue() == SoExtSelection::NOLASSO)) {
    inherited::handleEvent(action);
    return;
  }

  // Let children have a chance at grabbing events, if they want them.
  SoSeparator::handleEvent(action);
  if (action->isHandled()) { return; }

  // An option for the end-user to abort a selection.
  if (e->getTypeId() == SoKeyboardEvent::getClassTypeId()) {
    SoKeyboardEvent * k = (SoKeyboardEvent *)e;
    // note: will match on either up or down presses, or even not any
    // up/down state set. this is what we want, to be robust against
    // e.g. someone setting up their own SoKeyboardEvent and manually
    // sending it into the scene graph.  -mortene.
    if (k->getKey() == SoKeyboardEvent::END) {
      if (PRIVATE(this)->runningselection.mode != SoExtSelectionP::SelectionState::NONE) {
        PRIVATE(this)->runningselection.reset();
        this->touch();
        action->setHandled();
        return;
      }
    }
  }

  // The lassoType field could have changed as a result of a callback
  // from the above SoSeparator::handleEvent() call, so we need to
  // check for this before proceeding.
  if (this->lassoType.getValue() == SoExtSelection::NOLASSO) { return; }
  
  switch (PRIVATE(this)->runningselection.mode) {
    // No selection mode has been activated yet, so decide from the
    // lassoType field value where to go:
  case SoExtSelectionP::SelectionState::NONE:
    switch (this->lassoType.getValue()) {
    case SoExtSelection::RECTANGLE: PRIVATE(this)->handleEventRectangle(action); break;
    case SoExtSelection::LASSO: PRIVATE(this)->handleEventLasso(action); break;
    default: assert(FALSE); break;
    }
    break;
    
    // A selection mode is already "in action", so continue with that:
  case SoExtSelectionP::SelectionState::RECTANGLE: PRIVATE(this)->handleEventRectangle(action); break;
  case SoExtSelectionP::SelectionState::LASSO: PRIVATE(this)->handleEventLasso(action); break;
  default: assert(FALSE); break;
  }
}

// *************************************************************************

// internal method for drawing lasso
void
SoExtSelection::draw(SoGLRenderAction *action)
{
  const cc_glglue * glw = cc_glglue_instance(action->getCacheContext());
  pimpl->has3DTextures = SoGLDriverDatabase::isSupported(glw, SO_GL_3D_TEXTURES);

  SbViewportRegion vp = SoViewportRegionElement::get(action->getState());
  SbVec2s vpo = vp.getViewportOriginPixels();
  SbVec2s vps = vp.getViewportSizePixels();

  // matrices
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(vpo[0], vpo[0]+vps[0]-1,
          vpo[1], vpo[0]+vps[1]-1,
          -1, 1);


  // Because Mesa 3.4.2 can't properly push & pop GL_CURRENT_BIT, we have to
  // save the current color for later.
  GLfloat currentColor[4];
  glGetFloatv(GL_CURRENT_COLOR,currentColor);

  // attributes
  glPushAttrib(GL_LIGHTING_BIT|
               GL_FOG_BIT|
               GL_DEPTH_BUFFER_BIT|
               GL_TEXTURE_BIT|
               GL_LINE_BIT|
               GL_CURRENT_BIT);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);

  if(pimpl->has3DTextures) glDisable(GL_TEXTURE_3D);
  glDisable(GL_FOG);
  glDisable(GL_DEPTH_TEST);

  // line color & width
  glColor3f(PRIVATE(this)->lassocolor[0],PRIVATE(this)->lassocolor[1],PRIVATE(this)->lassocolor[2]);
  glLineWidth(PRIVATE(this)->lassowidth);

  // stipple pattern
  glEnable(GL_LINE_STIPPLE);
  glLineStipple(1, PRIVATE(this)->lassopattern);

  // --- RECTANGLE ---

  if(PRIVATE(this)->runningselection.mode == SoExtSelectionP::SelectionState::RECTANGLE) {
    assert(PRIVATE(this)->runningselection.coords.getLength() >= 2);
    SbVec2s c1 = PRIVATE(this)->runningselection.coords[0];
    SbVec2s c2 = PRIVATE(this)->runningselection.coords[1];
    glBegin(GL_LINE_LOOP);
    glVertex2s(c1[0], c1[1]);
    glVertex2s(c2[0], c1[1]);
    glVertex2s(c2[0], c2[1]);
    glVertex2s(c1[0], c2[1]);
    glEnd();
  }

  // --- LASSO ---

  else if(PRIVATE(this)->runningselection.mode == SoExtSelectionP::SelectionState::LASSO) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < PRIVATE(this)->runningselection.coords.getLength(); i++) {
      SbVec2s temp = PRIVATE(this)->runningselection.coords[i];
      glVertex2s(temp[0],temp[1]);
    }
    glEnd();
  }

  // finish - restore state
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glPopAttrib();

  // Due to a Mesa 3.4.2 bug
  glColor3fv(currentColor);
}

// Documented in superclass.
void
SoExtSelection::GLRenderBelowPath(SoGLRenderAction * action)
{
  // Overridden to render lasso.

  inherited::GLRenderBelowPath(action);
  SoState * state = action->getState();
  state->push();

  if (action->isRenderingDelayedPaths()) {

    SbViewportRegion vp = SoViewportRegionElement::get(state);
    SbVec2s vpo = vp.getViewportOriginPixels();
    SbVec2s vps = vp.getViewportSizePixels();
    this->draw(action);
  }
  // render this path after all other (delayed)
  else {
    if (PRIVATE(this)->runningselection.mode != SoExtSelectionP::SelectionState::NONE) {
      action->addDelayedPath(action->getCurPath()->copy());
    }
  }
  state->pop();
}

/*!
  Simulate lasso selection programmatically.

  This function is currently just stubbed.
*/

void
SoExtSelection::select(SoNode * COIN_UNUSED_ARG(root), int COIN_UNUSED_ARG(numcoords), SbVec3f * COIN_UNUSED_ARG(lasso), const SbViewportRegion & COIN_UNUSED_ARG(vp), SbBool COIN_UNUSED_ARG(shiftpolicy))
{
  // FIXME: Implement this for TGS compatibility...
  COIN_STUB_ONCE();
}

/*!
  Simulate lasso selection programmatically.

  This function is currently just stubbed.
*/

void
SoExtSelection::select(SoNode * COIN_UNUSED_ARG(root), int COIN_UNUSED_ARG(numcoords), SbVec2f * COIN_UNUSED_ARG(lasso), const SbViewportRegion & COIN_UNUSED_ARG(vp), SbBool COIN_UNUSED_ARG(shiftpolicy))
{
  // FIXME: Implement this for TGS compatibility...
  COIN_STUB_ONCE();
}

/*!
  Returns lasso coordinates in device coordinates.

  This function is currently just stubbed.
*/
const SbVec2s *
SoExtSelection::getLassoCoordsDC (int &COIN_UNUSED_ARG(numCoords))
{
  // FIXME: Implement this for TGS compatibility...
  COIN_STUB_ONCE();
  return NULL;
}

/*!
  Returns lasso coordinates in world coordinates.

  This function is currently just stubbed.
*/
const SbVec3f *
SoExtSelection::getLassoCoordsWC (int &COIN_UNUSED_ARG(numCoords))
{
  // FIXME: Implement this for TGS compatibility...
  COIN_STUB_ONCE();
  return NULL;
}

/*!
  Returns a path list containing selected objects.

  This function is currently just stubbed.
*/
const SoPathList &
SoExtSelection::getSelectionPathList () const
{
  // FIXME: Implement this for TGS compatibility...
  COIN_STUB_ONCE();
  return PRIVATE(this)->dummypathlist;
}

/*!

  The lasso selection filter callback is called when a node is about
  to be selected, and enables the application programmer to return a
  new path to be used when selecting. The new returned path should
  not be ref'd. SoExtSelection will ref() and unref() it.

  To cancel the selection, return NULL from the callback.

  if \a callonlyifselectable is TRUE, the callback will only be
  invoked when the path to the new node pass through the
  SoExtSelection node.

  This method is specific to Coin, and is not part of TGS OIV.
*/

void
SoExtSelection::setLassoFilterCallback(SoLassoSelectionFilterCB * f, void * userdata,
                                       const SbBool callonlyifselectable)
{
  PRIVATE(this)->filterCB = f;
  PRIVATE(this)->filterCBData = userdata;
  PRIVATE(this)->callfiltercbonlyifselectable = callonlyifselectable;
}

/*!
  Sets the callback that will be called for every triangle inside the
  lasso/rectangle when selecting.

  The callback should return \c FALSE if it wants to continue being
  invoked. When the callback returns \c TRUE, the object/shape is
  selected, and no more callbacks will be invoked for the object.

  \sa setLineSegmentFilterCallback, setPointFilterCallback
*/
void
SoExtSelection::setTriangleFilterCallback(SoExtSelectionTriangleCB * func,
                                          void * userdata)
{
  PRIVATE(this)->triangleFilterCB = func;
  PRIVATE(this)->triangleFilterCBData = userdata;
}

/*!
  Sets the callback that will be called for every line segment inside
  the lasso/rectangle when selecting.

  The callback should return \c FALSE if it wants to continue being
  invoked. When the callback returns \c TRUE, the object/shape is
  selected, and no more callbacks will be invoked for the object.

  \sa setTriangleFilterCallback, setPointFilterCallback
*/
void
SoExtSelection::setLineSegmentFilterCallback(SoExtSelectionLineSegmentCB * func,
                                             void * userdata)
{
  PRIVATE(this)->lineFilterCB = func;
  PRIVATE(this)->lineFilterCBData = userdata;
}

/*!
  Sets the callback that will be called for every point inside the
  lasso/rectangle when selecting.

  The callback should return \c FALSE if it wants to continue being
  invoked. When the user returns \c TRUE, the object/shape is
  selected, and no more callbacks will be invoked for the object.

  \sa setLineSegmentFilterCallback, setTriangleFilterCallback
*/
void
SoExtSelection::setPointFilterCallback(SoExtSelectionPointCB * func,
                                       void * userdata)
{
  PRIVATE(this)->pointFilterCB = func;
  PRIVATE(this)->pointFilterCBData = userdata;
}

/*!
  Returns whether the \c SHIFT key was pressed during the latest user
  interaction. This is useful if you want to respect the shift policy
  while selecting primitives.

  This method is specific to Coin, and is not part of TGS OIV.
*/
SbBool
SoExtSelection::wasShiftDown(void) const
{
  return PRIVATE(this)->wasshiftdown;
}

// timer callback for rendering lasso animation.
void
SoExtSelectionP::timercallback(void * data, SoSensor * COIN_UNUSED_ARG(sensor))
{
  SoExtSelection * ext = (SoExtSelection *)data;
  if (ext == NULL) return;
  if (ext->isOverlayLassoAnimated()) {
    int pat = ext->getOverlayLassoPattern();
    int pat2 = pat << 1;
    if ((pat & 0x8000) != 0) pat2 |= 1;
    ext->setOverlayLassoPattern(pat2 & 0xffff);
    ext->touch();
  }
}

// callback that is called for a shape before processing primitives.
SoCallbackAction::Response
SoExtSelectionP::preShapeCallback(void *data, SoCallbackAction *action, const SoNode *node)
{
  SoExtSelection * ext = (SoExtSelection *)data;
  assert(node->isOfType(SoShape::getClassTypeId()));

  PRIVATE(ext)->somefacesvisible = FALSE;

  if (SoPickStyleElement::get(action->getState()) == 
      SoPickStyleElement::UNPICKABLE) {
    return SoCallbackAction::PRUNE;
  }

  return PRIVATE(ext)->testShape(action, (const SoShape*) node);
}


// callback that is called for a shape after all primitives have been processed
SoCallbackAction::Response
SoExtSelectionP::postShapeCallback(void * data, SoCallbackAction * action, const SoNode * COIN_UNUSED_ARG(node))
{
  SoExtSelection * ext = (SoExtSelection*)data;

  SbBool hit = FALSE;
  switch (ext->lassoPolicy.getValue()) {
  case SoExtSelection::FULL:
    hit = (PRIVATE(ext)->primcbdata.allhit && 
           PRIVATE(ext)->primcbdata.hasgeometry);
    PRIVATE(ext)->primcbdata.allhit = FALSE;
    break;
  case SoExtSelection::PART:
    hit = PRIVATE(ext)->primcbdata.hit;
    PRIVATE(ext)->primcbdata.hit = FALSE;
    break;
  default:
    break;
  }

  if(hit){

    if(!PRIVATE(ext)->primcbdata.allshapes){ //VISIBLE_SHAPES
      if((ext->lassoPolicy.getValue() == SoExtSelection::FULL) &&
         (!PRIVATE(ext)->somefacesvisible))
        return SoCallbackAction::CONTINUE;
    }

    // Add visited shape to PathList of visited shapes
    const SoPath * curpath = action->getCurPath();
    PRIVATE(ext)->addVisitedPath(curpath);


  }

  return SoCallbackAction::CONTINUE;
}

// SoCamera callback for callback action. Will set view volume planes
// in SoCullElement to optimize picking.
SoCallbackAction::Response
SoExtSelectionP::cameraCB(void * data,
                          SoCallbackAction * action,
                          const SoNode * COIN_UNUSED_ARG(node))
{
  SoExtSelection * thisp = (SoExtSelection*) data;

  SoState * state = action->getState();
  SbViewVolume vv = SoViewVolumeElement::get(state);
  const SbViewportRegion & vp = SoViewportRegionElement::get(state);

  // Save viewvolume for later use.
  thisp->pimpl->offscreenviewvolume = vv;

  SbBox2s rectbbox;
  for (int i = 0; i < PRIVATE(thisp)->runningselection.coords.getLength(); i++) {
    rectbbox.extendBy(PRIVATE(thisp)->runningselection.coords[i]);
  }
  PRIVATE(thisp)->validateViewportBBox(rectbbox, vp.getViewportSizePixels());

  SbVec2s org = vp.getViewportOriginPixels();
  SbVec2s siz = vp.getViewportSizePixels();
  float left = float(rectbbox.getMin()[0] - org[0]) / float(siz[0]);
  float bottom = float(rectbbox.getMin()[1] - org[1]) / float(siz[1]);

  float right = float(rectbbox.getMax()[0] - org[0]) / float(siz[0]);
  float top = float(rectbbox.getMax()[1] - org[1]) / float(siz[1]);

  if (vv.getDepth() > 0.0f && vv.getWidth() > 0.0f && vv.getHeight() > 0.0f &&
      (right - left) > 0.0f && (top - bottom) > 0.0f) {
    vv = vv.narrow(left, bottom, right, top);
    SoCullElement::setViewVolume(state, vv);
  }
  return SoCallbackAction::CONTINUE;
}

//
SoCallbackAction::Response
SoExtSelectionP::testShape(SoCallbackAction * action, const SoShape * shape)
{
  int i;
  SoState * state = action->getState();

  SbBox2s rectbbox;
  for (i = 0; i < this->runningselection.coords.getLength(); i++) {
    rectbbox.extendBy(this->runningselection.coords[i]);
  }

  SbMatrix projmatrix;
  projmatrix = (SoModelMatrixElement::get(state) *
                SoViewingMatrixElement::get(state) *
                SoProjectionMatrixElement::get(state));

  SbBool full = FALSE;
  switch (PUBLIC(this)->lassoPolicy.getValue()) {
  case SoExtSelection::FULL_BBOX: /* fall through intended */
    full = TRUE;
  case SoExtSelection::PART_BBOX:
    return testBBox(action, projmatrix, shape, rectbbox, full);
  case SoExtSelection::FULL: /* fall through intended */
    full = TRUE;
  case SoExtSelection::PART:
    return testPrimitives(action, projmatrix, shape, rectbbox, full);
  default:
    assert(0 && "unknown lasso policy");
    break;
  }
  return SoCallbackAction::CONTINUE;
}

// project a point to screen
static SbVec2s
project_pt(const SbMatrix & projmatrix, const SbVec3f & v,
           const SbVec2s & vporg, const SbVec2s & vpsize)
{
  SbVec3f normpt;
  projmatrix.multVecMatrix(v, normpt);
  normpt[0] += 1.0f;
  normpt[1] += 1.0f;
  normpt[0] *= 0.5f;
  normpt[1] *= 0.5f;

  normpt[0] *= (float) vpsize[0];
  normpt[1] *= (float) vpsize[1];
  normpt[0] += (float) vporg[0];
  normpt[1] += (float) vporg[1];

  return SbVec2s((short) SbClamp(normpt[0], -32768.0f, 32767.0f),
                 (short) SbClamp(normpt[1], -32768.0f, 32767.0f));
}

// test for intersection between bounding box and lasso/rectangle
SoCallbackAction::Response
SoExtSelectionP::testBBox(SoCallbackAction * action,
                          const SbMatrix & projmatrix,
                          const SoShape * shape,
                          const SbBox2s & lassorect,
                          const SbBool full)
{
  SbBox3f bbox;
  SbVec3f center;
  const SoBoundingBoxCache * bboxcache = shape->getBoundingBoxCache();
  if (bboxcache && bboxcache->isValid(action->getState())) {
    bbox = bboxcache->getProjectedBox();
    if (bboxcache->isCenterSet()) center = bboxcache->getCenter();
    else center = bbox.getCenter();
  }
  else {
    ((SoShape *)shape)->computeBBox(action, bbox, center);
  }
  SbVec3f mincorner = bbox.getMin();
  SbVec3f maxcorner = bbox.getMax();

  SbBox2s shapebbox;

  SbVec2s vppt;
  SbVec3f normpt;
  SbVec2s vpo = this->curvp.getViewportOriginPixels();
  SbVec2s vps = this->curvp.getViewportSizePixels();

  SbVec2s projpts[8];

  for (int i = 0; i < 8; i++) {
    SbVec3f corner(i & 1 ? maxcorner[0] : mincorner[0],
                   i & 2 ? maxcorner[1] : mincorner[1],
                   i & 4 ? maxcorner[2] : mincorner[2]);
    vppt = project_pt(projmatrix, corner, vpo, vps);
    projpts[i] = vppt;
    shapebbox.extendBy(vppt);
  }
  if (lassorect.intersect(shapebbox)) { // quick reject
    int i;
    int hit = 0;
    switch (this->runningselection.mode) {
    case SelectionState::LASSO:
      if (full) {
        for (i = 0; i < 8; i++) {
          if (!point_in_poly(this->runningselection.coords, projpts[i])) break;
        }
        if (i == 8) hit = TRUE;
      }
      else {
        hit = poly_projbox_intersect(this->runningselection.coords, projpts);
      }
      break;
    case SelectionState::RECTANGLE:
      if (full) {
        for (i = 0; i < 8; i++) {
          if (!lassorect.intersect(projpts[i])) break;
        }
        if (i == 8) hit = TRUE;
      }
      else {
        for (i = 0; i < 8; i++) {
          if (lassorect.intersect(projpts[i])) { hit = TRUE; break; }
        }
      }
      break;
    default:
      break;
    }

    if(hit)
      this->doSelect(action->getCurPath());

  }
  return SoCallbackAction::PRUNE; // we don't need do callbacks for primitives

}

// initialize some variables needed before receiving primitive callbacks.
SoCallbackAction::Response
SoExtSelectionP::testPrimitives(SoCallbackAction * action,
                                const SbMatrix & projmatrix,
                                const SoShape * /* shape */,
                                const SbBox2s & lassorect,
                                const SbBool full)
{
  // FIXME: consider quick reject based on bounding box for now we
  // just initialize some variables, and trust that the user has a
  // sensible scene graph so that shapes are culled in the separators.
  // ????-??-?? pederb.
  //
  // FIXME: I believe what pederb mention above would be a _very_
  // important optimization -- especially when using VISIBLE_SHAPES
  // with a small rectangle/polygon selection in a huge scene.
  // 20050309 mortene.

  this->primcbdata.fulltest = full;
  this->primcbdata.projmatrix = projmatrix;
  this->primcbdata.lassorect = lassorect;
  this->primcbdata.hit = FALSE;
  this->primcbdata.allhit = TRUE;
  this->primcbdata.vporg = SoViewportRegionElement::get(action->getState()).getViewportOriginPixels();
  this->primcbdata.vpsize = SoViewportRegionElement::get(action->getState()).getViewportSizePixels();
  this->primcbdata.abort = FALSE;
  this->primcbdata.onlyrect = (this->runningselection.mode == SelectionState::LASSO);
  this->primcbdata.hasgeometry = FALSE;
  // signal to callback action that we want to generate primitives for
  // this shape
  return SoCallbackAction::CONTINUE;
}



// triangle callback from SoCallbackAction
//
// FIXME: much code of the triangleCB, lineSegmentCB and pointCB
// functions is common. Try to contract this code by sharing. 20041028 mortene.
void
SoExtSelectionP::triangleCB(void * userData,
                            SoCallbackAction * action,
                            const SoPrimitiveVertex * v1,
                            const SoPrimitiveVertex * v2,
                            const SoPrimitiveVertex * v3)
{
  // FIXME: there's a potential here for what could amount to a huge
  // optimization possibility: instead of waiting for this callback to
  // be triggered for each and every triangle, one could use the
  // preShapeCallback() we set up to detect the most common straight
  // collections of primitive types -- such as SoIndexedFaceSet,
  // SoFaceSet, SoQuadMesh, etc -- and then *loop* through the
  // polygons in those shapes nodes and render them there (that's the
  // optimization part).
  //
  // If implementing this scheme, one would have to turn off the
  // SoCallbackAction triangle-callback hook, or otherwise ignore it
  // (that would probably be less optimal, though), until the next
  // shape is traversed (to avoid duplicated rendering).
  //
  // This strategy should of course also work for non-polygon
  // primitives, such as in SoLineSet, SoPointSet etc.
  //
  // I haven't done any profiling, but it seems like this is likely to
  // have a huge impact on models with *lots* of polygon and/or line
  // and/or point primitives collected in relatively few shape nodes,
  // as one should get rid of much overhead.
  //
  // 20020807 mortene.

  SoExtSelectionP * thisp = ((SoExtSelection*)userData)->pimpl;

  thisp->primcbdata.hasgeometry = TRUE;
  thisp->drawcallbackcounter++;

  if (!thisp->applyonlyonselectedtriangles) {
    thisp->addTriangleToOffscreenBuffer(action, v1, v2, v3, TRUE);
  }

  // Shall we skip a certain amount of triangles before we start processing?
  if (!thisp->primcbdata.allshapes){
    // FIXME: what does this value actually represent? (And what's up
    // with the "-1"?) Please explain. 20041028 mortene.
    const double v = double(thisp->maximumcolorcounter) * thisp->offscreencolorcounterpasses - 1;
    if (thisp->offscreenskipcounter < v) {
      ++thisp->offscreenskipcounter;
      return;
    }
  }

  // Increase draw counter. Used below.
  thisp->drawcounter++;

  if(thisp->primcbdata.abort){
    return;
  }


  if(!thisp->triangleFilterCB && thisp->primcbdata.fulltest &&
     !thisp->primcbdata.allhit) {
    thisp->primcbdata.abort = TRUE;
    return;
  }
  if(!thisp->triangleFilterCB && !thisp->primcbdata.fulltest &&
     thisp->primcbdata.hit) {
    thisp->primcbdata.abort = TRUE;
    return;
  }


  SbVec2s p0 = project_pt(thisp->primcbdata.projmatrix, v1->getPoint(),
                          thisp->primcbdata.vporg, thisp->primcbdata.vpsize);
  SbVec2s p1 = project_pt(thisp->primcbdata.projmatrix, v2->getPoint(),
                          thisp->primcbdata.vporg, thisp->primcbdata.vpsize);
  SbVec2s p2 = project_pt(thisp->primcbdata.projmatrix, v3->getPoint(),
                          thisp->primcbdata.vporg, thisp->primcbdata.vpsize);


  if(thisp->primcbdata.fulltest) { // entire triangle must be inside lasso

    if(thisp->runningselection.mode == SelectionState::RECTANGLE){ // Rectangle check only
      if (!thisp->primcbdata.lassorect.intersect(p0) || (!point_in_poly(thisp->runningselection.coords, p0))) {
        thisp->primcbdata.allhit = FALSE;
        return;
      }
      if (!thisp->primcbdata.lassorect.intersect(p1) || (!point_in_poly(thisp->runningselection.coords, p1))) {
        thisp->primcbdata.allhit = FALSE;
        return;
      }
      if (!thisp->primcbdata.lassorect.intersect(p2) || (!point_in_poly(thisp->runningselection.coords, p2))) {
        thisp->primcbdata.allhit = FALSE;
        return;
      }
    }

    if(poly_line_intersect(thisp->runningselection.coords, p0, p1, FALSE) || !point_in_poly(thisp->runningselection.coords, p0)) {
      thisp->primcbdata.allhit = FALSE;
      return;
    }
    if(poly_line_intersect(thisp->runningselection.coords, p1, p2, FALSE) || !point_in_poly(thisp->runningselection.coords, p1)) {
      thisp->primcbdata.allhit = FALSE;
      return;
    }
    if(poly_line_intersect(thisp->runningselection.coords, p2, p0, FALSE) || !point_in_poly(thisp->runningselection.coords, p2)) {
      thisp->primcbdata.allhit = FALSE;
      return;
    }


  } else { // some part of the triangle must be inside lasso
    if (!poly_tri_intersect(thisp->runningselection.coords, p0, p1, p2)) {
      thisp->primcbdata.allhit = FALSE;
      return;
    }
  }



  if(!thisp->applyonlyonselectedtriangles){ // --- First pass

    if(!thisp->primcbdata.allshapes){ // LassoMode==VISIBLE_SHAPES
      if(thisp->drawcounter > thisp->maximumcolorcounter)
        thisp->offscreencolorcounteroverflow = TRUE;

      thisp->addTriangleToOffscreenBuffer(action, v1, v2, v3, thisp->offscreencolorcounteroverflow);

    } else if (thisp->triangleFilterCB) {

      // Present accepted triangle to 'user' through a callback.
      if(thisp->triangleFilterCB(thisp->triangleFilterCBData,
                                  action, v1, v2, v3)) {
        // Select shape.
        thisp->primcbdata.hit = TRUE;
        thisp->primcbdata.allhit = TRUE;
        thisp->primcbdata.abort = TRUE;
      }

    } else {
      thisp->primcbdata.hit = TRUE;
    }

  } else {  // --- Second pass. Feeding visible tris to client.


    if(thisp->drawcounter > thisp->maximumcolorcounter){
      thisp->offscreencolorcounteroverflow = TRUE;
      return;
    }

    int flag = 0x1 << (thisp->offscreencolorcounter & 0x07);
    int index = thisp->offscreencolorcounter >> 3;

    if (thisp->visibletrianglesbitarray[index] & flag){
      thisp->somefacesvisible = TRUE;
      if (thisp->triangleFilterCB &&
          thisp->triangleFilterCB(thisp->triangleFilterCBData, action, v1, v2, v3)){
        thisp->primcbdata.hit = TRUE;
        thisp->primcbdata.allhit = TRUE;
      }
    }
    ++thisp->offscreencolorcounter;
  }
}



void
SoExtSelectionP::addTriangleToOffscreenBuffer(SoCallbackAction * action,
                                              const SoPrimitiveVertex * v1,
                                              const SoPrimitiveVertex * v2,
                                              const SoPrimitiveVertex * v3,
                                              SbBool renderAsBlack)
{
  // FIXME: there is a likely major optimization that can be done when
  // rendering: use the NVidia occlusion culling extension, if
  // available. That most likely needs to be done on a shape basis (or
  // if possible: per separator) for it to have a positive effect,
  // though. 20030824 mortene.

  assert(!this->applyonlyonselectedtriangles);

  if(primcbdata.allshapes)
    return;

  SoState * state = action->getState();
  SbMatrix proj, affine;
  const SbMatrix & mm = SoModelMatrixElement::get(state);
  offscreenviewvolume.getMatrices(affine, proj);
  affine.multLeft(mm);

  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf((float *)proj);
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf((float *)affine);

  glDepthFunc(GL_LEQUAL);

  // Check vertex ordrering
  SoShapeHintsElement::VertexOrdering vertexorder;
  SoShapeHintsElement::ShapeType shapetype;
  SoShapeHintsElement::FaceType facetype; //Unused.
  SoShapeHintsElement::get(state, vertexorder, shapetype, facetype);


  if(shapetype == SoShapeHintsElement::SOLID){
    if(vertexorder == SoShapeHintsElement::CLOCKWISE){
      glFrontFace(GL_CW);
      glEnable(GL_CULL_FACE);
    } else if(vertexorder == SoShapeHintsElement::COUNTERCLOCKWISE){
      glFrontFace(GL_CCW);
      glEnable(GL_CULL_FACE);
    } else {
      glDisable(GL_CULL_FACE);
    }
  } else {
    glDisable(GL_CULL_FACE);
  }

  glBegin(GL_TRIANGLES);

  if(!renderAsBlack){
    glColor3ub((unsigned char) (this->offscreencolorcounter>>(8+8)),
               (unsigned char) (this->offscreencolorcounter>>(8)),
               (unsigned char) (this->offscreencolorcounter));
    ++offscreencolorcounter;
  } else {
    glColor3f(0,0,0);
  }

  glVertex3fv(v1->getPoint().getValue());
  glVertex3fv(v2->getPoint().getValue());
  glVertex3fv(v3->getPoint().getValue());
  glEnd();

}


// line segment callback from SoCallbackAction
//
// FIXME: much code of the triangleCB, lineSegmentCB and pointCB
// functions is common. Try to contract this code by sharing. 20041028 mortene.
void
SoExtSelectionP::lineSegmentCB(void *userData,
                               SoCallbackAction * action,
                               const SoPrimitiveVertex * v1,
                               const SoPrimitiveVertex * v2)
{
  SoExtSelectionP * thisp = ((SoExtSelection*)userData)->pimpl;

  thisp->primcbdata.hasgeometry = TRUE;
  thisp->drawcallbackcounter++;

  if (!thisp->applyonlyonselectedtriangles) {
    thisp->addLineToOffscreenBuffer(action, v1, v2, TRUE);
  }

  // Shall we skip a certain amount of lines before we start processing?
  if(!thisp->primcbdata.allshapes){
    // FIXME: what does this value actually represent? (And what's up
    // with the "-1"?) Please explain. 20041028 mortene.
    const double v = double(thisp->maximumcolorcounter) * thisp->offscreencolorcounterpasses - 1;
    if (thisp->offscreenskipcounter < v) {
      ++thisp->offscreenskipcounter;
      return;
    }
  }

  // Increase draw counter. Used below.
  thisp->drawcounter++;

  if (thisp->primcbdata.abort) return;

  if (!thisp->lineFilterCB && thisp->primcbdata.fulltest &&
      !thisp->primcbdata.allhit) {
    thisp->primcbdata.abort = TRUE;
    return;
  }
  if (!thisp->lineFilterCB && !thisp->primcbdata.fulltest &&
      thisp->primcbdata.hit) {
    thisp->primcbdata.abort = TRUE;
    return;
  }

  SbVec2s p0 = project_pt(thisp->primcbdata.projmatrix, v1->getPoint(),
                          thisp->primcbdata.vporg, thisp->primcbdata.vpsize);

  SbVec2s p1 = project_pt(thisp->primcbdata.projmatrix, v2->getPoint(),
                          thisp->primcbdata.vporg, thisp->primcbdata.vpsize);

  if (thisp->primcbdata.fulltest) {


    if (thisp->runningselection.mode == SelectionState::RECTANGLE){ // Rectangle check only
      if (!thisp->primcbdata.lassorect.intersect(p0) || (!point_in_poly(thisp->runningselection.coords, p0))) {
        thisp->primcbdata.allhit = FALSE;
        return;
      }

      if (!thisp->primcbdata.lassorect.intersect(p1) || (!point_in_poly(thisp->runningselection.coords, p1))) {
        thisp->primcbdata.allhit = FALSE;
        return;
      }
    }

    if (poly_line_intersect(thisp->runningselection.coords, p0, p1, FALSE) || !point_in_poly(thisp->runningselection.coords, p0)) {
      thisp->primcbdata.allhit = FALSE;
      return;
    }
  }
  else {
    if (!poly_line_intersect(thisp->runningselection.coords, p0, p1, TRUE)) {
      thisp->primcbdata.allhit = FALSE;
      return;
    }
  }


  if(!thisp->applyonlyonselectedtriangles){ // --- First pass

    // line segment is hit/surrounded by the lasso
    if(!thisp->primcbdata.allshapes){

      if(thisp->drawcounter > thisp->maximumcolorcounter)
        thisp->offscreencolorcounteroverflow = TRUE;

      thisp->addLineToOffscreenBuffer(action, v1, v2, thisp->offscreencolorcounteroverflow);

    } else if (thisp->lineFilterCB) {

      if (thisp->lineFilterCB(thisp->lineFilterCBData,
                              action, v1, v2)) {
        // select shape
        thisp->primcbdata.hit = TRUE;
        thisp->primcbdata.allhit = TRUE;
        thisp->primcbdata.abort = TRUE;
      }
    }
    else thisp->primcbdata.hit = TRUE;

  } else { // ---- Second pass

    if(thisp->drawcounter > thisp->maximumcolorcounter){
      thisp->offscreencolorcounteroverflow = TRUE;
      return;
    }

    int flag = 0x1 << (thisp->offscreencolorcounter & 0x07);
    int index = thisp->offscreencolorcounter >> 3;

    if (thisp->visibletrianglesbitarray[index] & flag) {
      if (thisp->lineFilterCB &&
          thisp->lineFilterCB(thisp->lineFilterCBData, action, v1, v2)) {
        thisp->primcbdata.hit = TRUE;
        thisp->primcbdata.allhit = TRUE;
      }
    }
    ++thisp->offscreencolorcounter;
  }
}

void
SoExtSelectionP::addLineToOffscreenBuffer(SoCallbackAction * action,
                                          const SoPrimitiveVertex * v1,
                                          const SoPrimitiveVertex * v2,
                                          SbBool renderAsBlack)
{
  assert(!this->applyonlyonselectedtriangles);

  if(primcbdata.allshapes)
    return;

  SoState * state = action->getState();
  SbMatrix proj, affine;
  const SbMatrix & mm = SoModelMatrixElement::get(state);
  offscreenviewvolume.getMatrices(affine, proj);
  affine.multLeft(mm);

  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf((float *)proj);
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf((float *)affine);

  glDepthFunc(GL_LEQUAL);


  glBegin(GL_LINES);

  if(!renderAsBlack){
    glColor3ub((unsigned char) (this->offscreencolorcounter>>(8+8)),
               (unsigned char) (this->offscreencolorcounter>>(8)),
               (unsigned char) (this->offscreencolorcounter));
    ++offscreencolorcounter;
  } else {
    glColor3f(0,0,0);
  }

  glVertex3fv(v1->getPoint().getValue());
  glVertex3fv(v2->getPoint().getValue());
  glEnd();

}



// point data callback from SoCallbackAction
//
// FIXME: much code of the triangleCB, lineSegmentCB and pointCB
// functions is common. Try to contract this code by sharing. 20041028 mortene.
void
SoExtSelectionP::pointCB(void *userData,
                         SoCallbackAction *action,
                         const SoPrimitiveVertex * v)
{
  SoExtSelectionP * thisp = ((SoExtSelection*)userData)->pimpl;
 
  thisp->primcbdata.hasgeometry = TRUE;
  thisp->drawcallbackcounter++;

  if (!thisp->applyonlyonselectedtriangles) {
    thisp->addPointToOffscreenBuffer(action, v, TRUE);
  }
  // Shall we skip a certain amount of lines before we start processing?
  if(!thisp->primcbdata.allshapes){
    // FIXME: what does this value actually represent? (And what's up
    // with the "-1"?) Please explain. 20041028 mortene.
    const double val = double(thisp->maximumcolorcounter) * thisp->offscreencolorcounterpasses - 1;
    if (thisp->offscreenskipcounter < val) {
      ++thisp->offscreenskipcounter;
      return;
    }
  }

  // Increase draw counter. (Used below).
  thisp->drawcounter++;


  if(thisp->primcbdata.abort){
    return;
  }

  if(!thisp->pointFilterCB && thisp->primcbdata.fulltest &&
      !thisp->primcbdata.allhit) {
    thisp->primcbdata.abort = TRUE;
    return;
  }
  if(!thisp->pointFilterCB && !thisp->primcbdata.fulltest &&
      thisp->primcbdata.hit) {
    thisp->primcbdata.abort = TRUE;
    return;
  }


  SbVec2s p = project_pt(thisp->primcbdata.projmatrix, v->getPoint(),
                         thisp->primcbdata.vporg, thisp->primcbdata.vpsize);

  if (thisp->runningselection.mode == SelectionState::RECTANGLE){ // Rectangle check only

    SbBool onlyrect = thisp->primcbdata.onlyrect;
    if (!thisp->primcbdata.lassorect.intersect(p) || (onlyrect || !point_in_poly(thisp->runningselection.coords, p))) {
      thisp->primcbdata.allhit = FALSE;
      return;
    }

  } else if(!point_in_poly(thisp->runningselection.coords, p)) {
    thisp->primcbdata.allhit = FALSE;
    return;
  }


  if(!thisp->applyonlyonselectedtriangles){ // ---- First pass

    if(!thisp->primcbdata.allshapes){

      // Draw to offscreen
      if(thisp->drawcounter > thisp->maximumcolorcounter)
        thisp->offscreencolorcounteroverflow = TRUE;

      thisp->addPointToOffscreenBuffer(action, v, thisp->offscreencolorcounteroverflow);

    } else if (thisp->pointFilterCB) {

      if (thisp->pointFilterCB(thisp->pointFilterCBData, action, v)) {
        // select shape
        thisp->primcbdata.hit = TRUE;
        thisp->primcbdata.allhit = TRUE;
        thisp->primcbdata.abort = TRUE;
      }
    }
    else thisp->primcbdata.hit = TRUE;

  } else { // ---- Second pass

    if(thisp->drawcounter > thisp->maximumcolorcounter){
      thisp->offscreencolorcounteroverflow = TRUE;
      return;
    }

    int flag = 0x1 << (thisp->offscreencolorcounter & 0x07);
    int index = thisp->offscreencolorcounter >> 3;

    if (thisp->visibletrianglesbitarray[index] & flag) {
      if (thisp->pointFilterCB &&
          thisp->pointFilterCB(thisp->pointFilterCBData, action, v)) {
        thisp->primcbdata.hit = TRUE;
        thisp->primcbdata.allhit = TRUE;
      }
    }
    ++thisp->offscreencolorcounter;
  }
}

void
SoExtSelectionP::addPointToOffscreenBuffer(SoCallbackAction * action,
                                           const SoPrimitiveVertex * v1,
                                           SbBool renderAsBlack)
{
  assert(!this->applyonlyonselectedtriangles);

  if(primcbdata.allshapes)
    return;

  SoState * state = action->getState();
  SbMatrix proj, affine;
  const SbMatrix & mm = SoModelMatrixElement::get(state);
  offscreenviewvolume.getMatrices(affine, proj);
  affine.multLeft(mm);

  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf((float *)proj);
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf((float *)affine);

  glDepthFunc(GL_LEQUAL);
  glPointSize(SoPointSizeElement::get(action->getState()));


  glBegin(GL_POINTS);

  if(!renderAsBlack){
    glColor3ub((unsigned char) (this->offscreencolorcounter>>(8+8)),
               (unsigned char) (this->offscreencolorcounter>>(8)),
               (unsigned char) (this->offscreencolorcounter));
    ++offscreencolorcounter;
  } else {
    glColor3f(0,0,0);
  }

  glVertex3fv(v1->getPoint().getValue());
  glEnd();

}


// invoke selection policy on a shape
void
SoExtSelectionP::doSelect(const SoPath * path)
{
  SoPath * newpath = (SoPath*) path;

  if (this->filterCB && (!this->callfiltercbonlyifselectable ||
                         path->findNode(PUBLIC(this)) >= 0)) {
    newpath = this->filterCB(this->filterCBData, path);
  }

  if (newpath == NULL) { return; }

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoExtSelectionP::doSelect",
                         "selected path with %s at tail",
                         newpath->getTail()->getTypeId().getName().getString());
#endif // debug

  if (newpath != path) { newpath->ref(); }
  PUBLIC(this)->invokeSelectionPolicy(newpath, TRUE);
  if (newpath != path) { newpath->unref(); }
}


void
SoExtSelectionP::addVisitedPath(const SoPath *path)
{

  // FIXME: This is a linear search. Storing paths should have be
  // done using something more sophisticated for better
  // performance. (handegar)

  if(this->visitedshapepaths->findPath(*path) < 0)
    this->visitedshapepaths->append(path->copy());

}

// Call a doSelect for all paths in pathlist
void
SoExtSelectionP::selectPaths(void)
{
  int length = this->visitedshapepaths->getLength();

  for(int i=0;i<length;++i) {
    this->doSelect((*(this->visitedshapepaths))[i]);
  }

  this->visitedshapepaths->truncate(0);
}

void
SoExtSelectionP::offscreenLassoTesselatorCallback(void * v0, void * v1, void * v2, void * userdata)
{
  SoExtSelectionP * pimpl = (SoExtSelectionP *) userdata;

  // Set flag indicating that a callback was executed.
  pimpl->lassostencilisdrawed = TRUE;

  SbVec3f *vec0 = (SbVec3f *) v0;
  SbVec3f *vec1 = (SbVec3f *) v1;
  SbVec3f *vec2 = (SbVec3f *) v2;

  glBegin(GL_TRIANGLES);
   glColor3f(1,1,1);
   glVertex2f(vec0->getValue()[0],vec0->getValue()[1]);
   glVertex2f(vec1->getValue()[0],vec1->getValue()[1]);
   glVertex2f(vec2->getValue()[0],vec2->getValue()[1]);
  glEnd();

}

void
SoExtSelectionP::offscreenRenderLassoCallback(void * userdata, SoAction * action)
{
  if (!action->isOfType(SoGLRenderAction::getClassTypeId())) return;

  SoExtSelectionP * pimpl = (SoExtSelectionP *) userdata;

  // Setup optimal screen-aspect according to lasso-size
  SoHandleEventAction * eventAction = pimpl->offscreenaction;
  const SbViewportRegion & vp = SoViewportRegionElement::get(eventAction->getState());

  SbVec2s vpo = vp.getViewportOriginPixels();
  SbVec2s vps = vp.getViewportSizePixels();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glOrtho(vpo[0], vpo[0]+vps[0]-1,
          vpo[1], vpo[0]+vps[1]-1,
          -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  
  // This flag will be set to TRUE if the tesselatorcallbacks was executed.
  pimpl->lassostencilisdrawed = FALSE;

  // Render all tris to offscreen buffer via a tessellator.
  SbTesselator tessellator(pimpl->offscreenLassoTesselatorCallback,pimpl);
  tessellator.beginPolygon();

  int i;
  SbList <SbVec3f> tmplist;
  for(i = 0; i < pimpl->runningselection.coords.getLength(); i++){
    tmplist.append(SbVec3f((float) pimpl->runningselection.coords[i][0],(float) pimpl->runningselection.coords[i][1],0));
  }
  const SbVec3f * tmparray = tmplist.getArrayPtr();
  for(i = 0; i < pimpl->runningselection.coords.getLength(); i++)
    tessellator.addVertex(tmparray[i],(void*)&tmparray[i]);
  tessellator.endPolygon();

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

void
SoExtSelectionP::offscreenRenderCallback(void * userdata, SoAction * action)
{
  if (!action->isOfType(SoGLRenderAction::getClassTypeId())) return;

  SoExtSelectionP * pimpl = (SoExtSelectionP *) userdata;

  /*
    FIXME: A nice feature could be an option to 'zoom' in on the
    selected area to increase pixel detail in the offscreen
    buffer. This could increase the hitrate to the offscreen-scanner
    when searching for really small visible entities. Note that you
    must also deform the stencil polygon accordingly (which can be
    abit tricky i believe). (handegar)
  */

  /*
    UPDATE: another idea here would be to allocate a larger offscreen
    buffer than the onscreen buffer to increase the precision.

    The scale-value could be controlled by an environment variable
    COIN_EXTSELECTION_SCALE_PRECISION, or something like that.

    The downside of this is that the buffer scanning takes quite a lot
    of time already.

    20020802 mortene.
  */

  // Because Mesa 3.4.2 can't properly push & pop GL_CURRENT_BIT, we have to
  // save the current color for later.
  GLfloat currentColor[4];
  glGetFloatv(GL_CURRENT_COLOR,currentColor);

  glPushAttrib(GL_LIGHTING_BIT|
               GL_FOG_BIT|
               GL_DEPTH_BUFFER_BIT|
               GL_TEXTURE_BIT|
               GL_LINE_BIT|
               GL_CURRENT_BIT);

  // Setup GL-state for offscreen context
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  if(pimpl->has3DTextures)
    glDisable(GL_TEXTURE_3D);
  glDisable(GL_FOG);
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);

  // --- Render all tris to offscreen buffer.
  pimpl->cbaction->apply(pimpl->offscreenheadnode);
  PUBLIC(pimpl)->touch();


  // Restore all OpenGL States
  glPopAttrib();

  // Due to a Mesa 3.4.2 bug
  glColor3fv(currentColor);
}

SbBool
SoExtSelectionP::checkOffscreenRendererCapabilities()
{
  GLboolean rgbmode;
  glGetBooleanv(GL_RGBA_MODE, &rgbmode);
  if (!rgbmode) {
    SoDebugError::post("SoExtSelectionP::checkOffscreenRendererCapabilities",
                       "Couldn't get an RGBA OpenGL context -- cannot "
                       "proceed with VISIBLE_SHAPES selection. Check your "
                       "system for driver errors.");
    return FALSE;
  }

  // Check color-channel bitresolution
  GLint red, green, blue;
  glGetIntegerv(GL_RED_BITS, &red);
  glGetIntegerv(GL_GREEN_BITS, &green);
  glGetIntegerv(GL_BLUE_BITS, &blue);

  // Calculate maximum colorcounter from RGB bit depth.
#ifndef COLORBITS
  const double maxcols = pow(2.0, double(red + green + blue));
#else // COLORBITS debug
  // This is debug/testing stuff, forcing lower colorresolution. VERY
  // useful for testing.
  const double maxcols = pow(2.0, double(COLORBITS));
#endif

  // We do not want to use too many bits, as we later need to allocate
  // an array which is (maximumcolorcounter / 8) bytes large, using 1
  // bit for each primitive in the scene to detect whether visible or
  // not. So we set a limit at ~4 million, which should be enough to
  // do even really heavy scenes in a single pass, while this will
  // allocate a buffer of not more than ~0.5 MB.
  const unsigned int threshold = (unsigned int)(1 << 22);

  this->maximumcolorcounter =
    (maxcols > threshold) ? threshold : (unsigned int)maxcols;

  if (SoExtSelectionP::debug()) {
    SoDebugError::postInfo("SoExtSelectionP::checkOffscreenRendererCapabilities",
                           "GL_{color}_BITS==[%d, %d, %d] "
                           "maximumcolorcounter==%u",
                           red, green, blue,
                           this->maximumcolorcounter);
  }

  if (this->maximumcolorcounter < 2) {
    SoDebugError::post("SoExtSelectionP::checkOffscreenRendererCapabilities",
                       "Couldn't get an RGBA OpenGL context with at least "
                       "two colors -- can't proceed with VISIBLE_SHAPE "
                       "selection (check your system for errors).");
    return FALSE;
  }

  return TRUE;
}


SbBool
SoExtSelectionP::scanOffscreenBuffer(SoNode * COIN_UNUSED_ARG(sceneRoot))
{

  const SbViewportRegion vpr = renderer->getViewportRegion();

  int offscreenSizeX = vpr.getViewportSizePixels()[0];
  int offscreenSizeY = vpr.getViewportSizePixels()[1];

  unsigned char * rgbBuffer = renderer->getBuffer();
  unsigned char * stencilBuffer = lassorenderer->getBuffer();
  int pixelValue = 0;
  int index = 0;
  int flag = 0;
  SbBool hitflag = FALSE;

  // Clear entire table.
  (void)memset(this->visibletrianglesbitarray, 0,
               (this->maximumcolorcounter + 7) / 8);

  SbBox2s rectbbox;
  for (int k = 0; k < this->runningselection.coords.getLength(); k++) {
    rectbbox.extendBy(this->runningselection.coords[k]);
  }
  this->validateViewportBBox(rectbbox, SbVec2s(offscreenSizeX, offscreenSizeY));

  // We must also deform lassocoords if offscreen size is changed
  const int minx = (int) (rectbbox.getMin()[0] * ((float) offscreenSizeX/requestedsize[0]));
  const int maxx = (int) (rectbbox.getMax()[0] * ((float) offscreenSizeX/requestedsize[0]));
  const int miny = (int) (rectbbox.getMin()[1] * ((float) offscreenSizeY/requestedsize[1]));
  const int maxy = (int) (rectbbox.getMax()[1] * ((float) offscreenSizeY/requestedsize[1]));


  for(int j=miny; j < maxy; ++j){
    for(int i=minx*3; i < maxx*3; i+=3){

      // If needed, we consult the stencil buffer before fetching pixelvalue
      if(this->lassostencilisdrawed){
        if(stencilBuffer[j*offscreenSizeX*3 + i] == 0)
          continue;
      }

      pixelValue = (rgbBuffer[j*offscreenSizeX*3 + i]<< (8+8));
      pixelValue += (rgbBuffer[j*offscreenSizeX*3 + i + 1]<< (8));
      pixelValue += (rgbBuffer[j*offscreenSizeX*3 + i + 2]);

      if(pixelValue != 0){
        flag = 0x1 << (pixelValue & 0x07);
        index = pixelValue >> 3;
        visibletrianglesbitarray[index] |= flag;
        hitflag = TRUE;
      }

    }
  }

  return(hitflag);
}

void
SoExtSelectionP::selectAndReset(SoHandleEventAction * action)
{
  this->performSelection(action);
  this->runningselection.reset();
}

// start a selecting for the current lasso/rectangle
void
SoExtSelectionP::performSelection(SoHandleEventAction * action)
{
  assert(this->runningselection.mode != SelectionState::NONE);

  if (SoExtSelectionP::debug()) {
    for (int i = 0; i < this->runningselection.coords.getLength(); i++) {
      const SbVec2s & c = this->runningselection.coords[i];
      SoDebugError::postInfo("SoExtSelectionP::performSelection",
                             "coord[%d]==<%d, %d>", i, c[0], c[1]);
    }
  }

  // convert the rectangle to a polygon
  if (this->runningselection.mode == SelectionState::RECTANGLE) {
    assert(this->runningselection.coords.getLength() == 2);

    const SbVec2s p0 = this->runningselection.coords[0];
    const SbVec2s p1 = this->runningselection.coords[1];
    this->runningselection.coords[1] = SbVec2s(p1[0], p0[1]);
    this->runningselection.coords.append(p1);
    this->runningselection.coords.append(SbVec2s(p0[0], p1[1]));
  }

  //Send signal to client that tris are coming up,
  PUBLIC(this)->startCBList->invokeCallbacks(PUBLIC(this));

  this->curvp = SoViewportRegionElement::get(action->getState());
  this->cbaction->setViewportRegion(this->curvp);

  switch (PUBLIC(this)->policy.getValue()) {
  case SoSelection::SINGLE:
    PUBLIC(this)->deselectAll();
    break;
  case SoSelection::SHIFT:
    if (!this->wasshiftdown) PUBLIC(this)->deselectAll();
    break;
  default:
    break;
  }

  if (PUBLIC(this)->lassoMode.getValue() == SoExtSelection::ALL_SHAPES) {
    this->offscreencolorcounter = 1;
    this->offscreenskipcounter = 0;
    this->applyonlyonselectedtriangles = FALSE;
    this->offscreencolorcounteroverflow = FALSE;
    this->drawcallbackcounter = 0;
    this->drawcounter = 0;

    // Execute 'search' for triangles
    primcbdata.allshapes = TRUE;
    this->cbaction->apply(action->getCurPath()->getHead());

  }
  else {

    //
    // -- Search for visible triangles inside lasso
    //
    primcbdata.allshapes = FALSE;


    this->offscreenaction = action;
    this->offscreenheadnode = action->getCurPath()->getHead();

    // Check OpenGL capabilities
    SbBool setupok = this->checkOffscreenRendererCapabilities();
    // Ai, ai. OpenGL context cannot be used with VISIBLE_SHAPE
    // selection.  We'll spit out informative error messages within
    // checkOffscreenRendererCapabilities().
    if (!setupok) {
      // start/finish should be paired up
      PUBLIC(this)->finishCBList->invokeCallbacks(PUBLIC(this));
      return;
    }

    this->visibletrianglesbitarray =
      new unsigned char[(this->maximumcolorcounter + 7) / 8];

    // --- Do this procedure several times if colorcounter overflows
    this->offscreencolorcounterpasses = 0;


    // Create offscreen renderer
    /*
      FIXME: Due to the implementation of this node, offscreen
      rendering using multiple subscreens will lead to erroneous
      results. Therefore we automatically reduce the size of the
      offscreen to fit within the maximum offscreen limitations.
      (20020812 handegar)

      UPDATE 20041028 mortene: this is a rather nasty problem, as we
      have no guarantee that SoOffscreenRenderer uses the same
      criteria as below for deciding whether to use tiled rendering or
      not. (In fact, it doesn't.) This should /really/ be improved
      upon, if possible.
    */
    unsigned int maxsize[2];
    cc_glglue_context_max_dimensions(&maxsize[0], &maxsize[1]);

    this->requestedsize = action->getViewportRegion().getViewportSizePixels();

    SbViewportRegion vp = action->getViewportRegion();
    if((unsigned int) requestedsize[0] > maxsize[0] || (unsigned int) requestedsize[1] > maxsize[1]){

      double maxv = (float) SbMax(requestedsize[0],requestedsize[1]);
      double minv = (float) SbMin(maxsize[0],maxsize[1]);
      double scale = minv/maxv;

      SbVec2s newsize;
      newsize[0] = (int) (requestedsize[0] * scale);
      newsize[1] = (int) (requestedsize[1] * scale);

      vp = SbViewportRegion(newsize[0],newsize[1]);
    }
    // only (re)allocate the renderers if the viewport has changed
    if (this->renderer == NULL || this->renderer->getViewportRegion() != vp) {
      delete this->renderer;
      this->renderer = new SoOffscreenRenderer(vp);
    }
    if (this->lassorenderer == NULL || this->lassorenderer->getViewportRegion() != vp) {
      delete this->lassorenderer;
      this->lassorenderer = new SoOffscreenRenderer(vp);
    }

    SoCallback * cbnode = new SoCallback;
    cbnode->ref();
    cbnode->setCallback(offscreenRenderCallback, this);

    do { // --- Render and processing loop.

      this->offscreencolorcounter = 1;
      this->offscreenskipcounter = 0;
      this->applyonlyonselectedtriangles = FALSE;
      this->offscreencolorcounteroverflow = FALSE;
      this->drawcallbackcounter = 0;
      this->drawcounter = 0;

      assert(this->runningselection.mode != SelectionState::NONE);

      // Render a offscreen stencil buffer of lasso-shape.  Only draw
      // stencil buffer on the first pass
      if (this->offscreencolorcounterpasses == 0){
        SoCallback * cbnode2 = new SoCallback;
        cbnode2->ref();
        cbnode2->setCallback(offscreenRenderLassoCallback, this);
        this->lassorenderer->render(cbnode2);
        cbnode2->unref();
      }

      // First render pass to offscreen buffer.
      this->renderer->render(cbnode);

      // Debugging: if the envvar is set, the contents of the
      // offscreen buffer are stored to disk for investigation.
      static SbBool chkenv = FALSE;
      static const char * dumpfilename = NULL;
      if (chkenv == FALSE) {
        dumpfilename = coin_getenv("COIN_EXTSELECTION_SAVE_OFFSCREENBUFFER");
        chkenv = TRUE;
      }
      if (dumpfilename) { this->renderer->writeToRGB(dumpfilename); }

      // Scan buffer marking visible colors in the
      // 'visibletrianglesbitarray' array.
      if (this->scanOffscreenBuffer(action->getCurPath()->getHead()) != 0) {

        // Render once more, but only selected triangles which are forwarded
        // to client code through 'triangleFilterCB'.

        this->offscreencolorcounteroverflow = FALSE;
        this->offscreencolorcounter = 1;
        this->offscreenskipcounter = 0;
        this->drawcallbackcounter = 0;
        this->drawcounter = 0;

        this->applyonlyonselectedtriangles = TRUE;
        this->cbaction->apply(action->getCurPath()->getHead());
        PUBLIC(this)->touch();

      } else {

        this->offscreencolorcounteroverflow = FALSE;
        this->offscreencolorcounter = 1;
        this->offscreenskipcounter = 0;
      }
      ++this->offscreencolorcounterpasses;

    } while (this->offscreencolorcounterpasses * double(this->maximumcolorcounter) < this->drawcallbackcounter);

    // Release allocated stuff
    cbnode->unref();
    delete [] this->visibletrianglesbitarray;
  }

  this->selectPaths(); // Execute a 'doSelect' on all stored paths.

  // Send signal to client that we are finished searching for tris
  PUBLIC(this)->finishCBList->invokeCallbacks(PUBLIC(this));
  PUBLIC(this)->touch();
}

//
// avoid an empty viewport bounding box (support for a single click and 
// a 1-pixel-size rectangles/lassos).
//
void 
SoExtSelectionP::validateViewportBBox(SbBox2s & bbox, 
                                     const SbVec2s & vpsize)
{
  const SbVec2s bmin = bbox.getMin();
  const SbVec2s bmax = bbox.getMax();

  if (bmin[0] == bmax[0]) {
    int add = (bmin[0] < (vpsize[0] - 1)) ? 1 : -1;
    bbox.extendBy(SbVec2s(bmin[0] + add, bmin[1]));
  }

  if (bmin[1] == bmax[1]) {
    int add = (bmin[1] < (vpsize[1] - 1)) ? 1 : -1;
    bbox.extendBy(SbVec2s(bmin[0], bmin[1] + add));
  }
}

#undef PRIVATE
#undef PUBLIC
