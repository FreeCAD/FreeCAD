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
  \class SoRayPickAction SoRayPickAction.h Inventor/actions/SoRayPickAction.h
  \brief The SoRayPickAction class does ray intersection with scene graphs.

  \ingroup coin_actions

  For interaction with the scene graph geometry, it is necessary to be
  able to do intersection testing for rays. This functionality is
  provided by the SoRayPickAction class.

  SoRayPickAction can be used to pass arbitrary rays through the scene
  for intersection detections, by using the setRay() method.

  Because a very common operation is to check for intersections along
  the ray from the mouse cursor upon mouse clicks, it also contains
  convenience methods for setting up a ray from the near plane to the
  far plane from the 2D mouse cursor coordinates. See the setPoint()
  and setNormalizedPoint() methods. A simple usage example for this
  case is presented below.

  Note that one common mistake when using a ray pick action to
  intersect from a point under the mouse cursor after a mouse click is
  that one tries to apply it to a scene graph that does not contain a
  camera \e explicitly set up by the application programmer. Without a
  camera as part of the traversal, the ray pick action does not know
  which view volume to send the ray through.

  In this regard, be aware that the getSceneGraph() call in the
  So*-libraries' viewer classes will return the root of the
  user-supplied scene graph, not the "real" internal scene graph root
  used by the viewer (which should always contain a camera node). So
  ray picks done from the application code will fail when doing this:

  \code
  // initializing viewer scene graph
  SoSeparator * root = new SoSeparator;
  root->ref();

  SoEventCallback * ecb = new SoEventCallback;
  ecb->addEventCallback(SoMouseButtonEvent::getClassTypeId(), event_cb, viewer);
  root->addChild(ecb);

  root->addChild(new SoCone);

  viewer->setSceneGraph( root );
  // -- [snip] -------------------------

  // attempting ray pick in the event_cb() callback method
  SoRayPickAction rp( viewer->getViewportRegion() );
  rp.setPoint(mouseevent->getPosition());
  rp.apply(viewer->getSceneGraph());
  // BUG: results will not be what you expected, as no camera was
  // part of the "user's scene graph"
  \endcode

  While this is the correct way to do it:

  \code
  // initializing viewer scene graph
  SoSeparator * root = new SoSeparator;
  root->ref();

  // Need to set up our own camera in the "user scene graph", or else
  // the ray pick action will fail because the camera is hidden in the
  // viewer-specific root of the scene graph.
  SoPerspectiveCamera * pcam = new SoPerspectiveCamera;
  root->addChild(pcam);

  SoEventCallback * ecb = new SoEventCallback;
  ecb->addEventCallback(SoMouseButtonEvent::getClassTypeId(), event_cb, viewer);
  root->addChild(ecb);

  root->addChild(new SoCone);

  viewer->setSceneGraph( root );
  pcam->viewAll( root, viewer->getViewportRegion() );
  // -- [snip] -------------------------

  // attempting ray pick in the event_cb() callback method
  SoRayPickAction rp( viewer->getViewportRegion() );
  rp.setPoint(mouseevent->getPosition());
  rp.apply(viewer->getSceneGraph());
  \endcode

  Or if you do want the convenience of having the viewer set up a
  camera for you implicitly, you can get hold of the root-node of the
  "complete" scene graph by simply calling:

  \code
  SoNode * realroot = viewer->getSceneManager()->getSceneGraph();
  \endcode
*/
// FIXME: in the class doc, also mention how one can use
// SoRayPickAction from within an SoHandleEventAction callback with
// the getNodeAppliedTo() method etc.  Include a usage example code
// snippet. 20010920 mortene.

#include <Inventor/actions/SoRayPickAction.h>

#include <cfloat>

#include <Inventor/SbLine.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/elements/SoClipPlaneElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoTextureOverrideElement.h>
#include <Inventor/elements/SoPickRayElement.h>
#include <Inventor/elements/SoPickStyleElement.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/lists/SoEnabledElementsList.h>
#include <Inventor/lists/SoPickedPointList.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoLOD.h>
#include <Inventor/nodes/SoLevelOfDetail.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/SbVec3d.h>
#include <Inventor/SbVec2d.h>
#include <Inventor/SbDPLine.h>
#include <Inventor/SbDPPlane.h>
#include <Inventor/SbDPMatrix.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "actions/SoSubActionP.h"



// *************************************************************************

// The private data for the SoRayPickAction.

class SoRayPickActionP {
public:
  SoRayPickActionP(void) : owner(NULL) { }

  // Hidden private methods.

  SbBool isBetweenPlanesWS(const SbVec3d & intersection,
                           const SoClipPlaneElement * planes) const;
  void cleanupPickedPoints(void);
  void setFlag(const unsigned int flag);
  void clearFlag(const unsigned int flag);
  SbBool isFlagSet(const unsigned int flag) const;
  void calcObjectSpaceData(SoState * ownerstate);
  void calcMatrices(SoState * ownerstate);
  void setPickStyleFlags(SoState * ownerstate);

  // Hidden private variables.

  SbViewVolume osvolume;
  SbViewVolume wsvolume;
  SbLine osline_sp;

  // use double precision types to increase picking precision
  SbDPLine osline;
  SbDPPlane nearplane;
  SbVec2s vppoint;
  SbVec2f normvppoint;
  SbVec3d raystart;
  SbVec3d raydirection;
  double rayradiusstart;
  double rayradiusdelta;
  double raynear;
  double rayfar;
  float radiusinpixels;

  SbDPLine wsline;
  SbDPMatrix obj2world;
  SbDPMatrix world2obj;
  SbDPMatrix extramatrix;

  SoPickedPointList pickedpointlist;
  SbList <double> ppdistance;

  unsigned int flags;
  SbBool objectspacevalid; // FIXME: why not a flag?

  enum {
    WS_RAY_SET =         0x0001, // ray set by setRay()
    WS_RAY_COMPUTED =    0x0002, // ray computed in computeWorldSpaceRay()
    PICK_ALL =           0x0004, // return all picked objects, or just closest
    NORM_POINT =         0x0008, // is normalized vppoint calculated
    CLIP_NEAR =          0x0010, // clip ray at near plane?
    CLIP_FAR =           0x0020, // clip ray at far plane?
    EXTRA_MATRIX =       0x0040, // is extra matrix supplied in setObjectSpace()
    PPLIST_IS_SORTED =   0x0080, // did we sort pickedpointslist ?
    OSVOLUME_DIRTY =     0x0100, // did we calculate osvolume?
    PUSH_PICK_TO_FRONT = 0x0200, // should pick go in front?
    CULL_BACKFACES =     0x0400  // should backface picks be ignored?
  };

  SoRayPickAction * owner;
};

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

SO_ACTION_SOURCE(SoRayPickAction);


/*!
  \copydetails SoAction::initClass(void)
*/
void
SoRayPickAction::initClass(void)
{
  SO_ACTION_INTERNAL_INIT_CLASS(SoRayPickAction, SoPickAction);

  SO_ENABLE(SoRayPickAction, SoPickRayElement);
  SO_ENABLE(SoRayPickAction, SoViewportRegionElement);
  SO_ENABLE(SoRayPickAction, SoOverrideElement);
  SO_ENABLE(SoRayPickAction, SoTextureOverrideElement);
}


/*!
  Constructor.

  Some node types need a \a viewportregion to know exactly how they
  are positioned within the scene. For an in-depth explanation of why
  the \a viewportregion argument is needed, see the documentation of
  SoGetBoundingBox::SoGetBoundingBox(const SbViewportRegion &).
*/
SoRayPickAction::SoRayPickAction(const SbViewportRegion & viewportregion)
  : inherited(viewportregion)
{
  PRIVATE(this)->owner = this;
  PRIVATE(this)->radiusinpixels = 5.0f;
  PRIVATE(this)->flags = 0;
  PRIVATE(this)->objectspacevalid = TRUE;

  SO_ACTION_CONSTRUCTOR(SoRayPickAction);
}

/*!
  Destructor, free temporary resources used by action.
*/
SoRayPickAction::~SoRayPickAction(void)
{
  PRIVATE(this)->cleanupPickedPoints();
}

/*!
  Sets the viewport-space point. This point is calculated into a line
  from the near clipping plane to the far clipping plane, and the
  intersection ray follows the line.

  This is a convenient way to detect object intersection below the
  cursor.
*/
void
SoRayPickAction::setPoint(const SbVec2s & viewportpoint)
{
  PRIVATE(this)->vppoint = viewportpoint;
  PRIVATE(this)->clearFlag(SoRayPickActionP::NORM_POINT |
                           SoRayPickActionP::WS_RAY_SET |
                           SoRayPickActionP::WS_RAY_COMPUTED);
  PRIVATE(this)->setFlag(SoRayPickActionP::CLIP_NEAR |
                         SoRayPickActionP::CLIP_FAR);
}

/*!
  Sets the viewport-space point which the ray is sent through.
  The coordinate is normalized, ranging from (0, 0) to (1, 1).

  \sa setPoint()
*/
void
SoRayPickAction::setNormalizedPoint(const SbVec2f & normpoint)
{
  PRIVATE(this)->normvppoint = normpoint;
  PRIVATE(this)->clearFlag(SoRayPickActionP::WS_RAY_SET |
                           SoRayPickActionP::WS_RAY_COMPUTED);
  PRIVATE(this)->setFlag(SoRayPickActionP::NORM_POINT |
                         SoRayPickActionP::CLIP_NEAR |
                         SoRayPickActionP::CLIP_FAR);
}

/*!
  Sets the radius of the picking ray, in screen pixels.  Default value
  is 5.0.

  The radius of the intersection ray will only influence the pick
  operation's behavior versus lines and points, and has no effect on
  picking of shapes / polygons.
*/
void
SoRayPickAction::setRadius(const float radiusinpixels)
{
  PRIVATE(this)->radiusinpixels = radiusinpixels;
}


/*!
  Gets the radius of the picking ray, in screen pixels.
*/
float
SoRayPickAction::getRadius(void) const
{
  return PRIVATE(this)->radiusinpixels;
}

/*!
  Sets the intersection ray in world space coordinates.

  Use this method if you want to send any ray through the scene to
  detect intersections, independently of mouse cursor position upon
  clicks and scene graph camera settings.
*/
void
SoRayPickAction::setRay(const SbVec3f & start, const SbVec3f & direction,
                        float neardistance, float fardistance)
{
#if COIN_DEBUG
  if (direction == SbVec3f(0.0f, 0.0f, 0.0f)) {
    SoDebugError::postWarning("SoRayPickAction::setRay",
                              "Ray has no direction");

  }
#endif // COIN_DEBUG
  if (neardistance >= 0.0f) PRIVATE(this)->setFlag(SoRayPickActionP::CLIP_NEAR);
  else {
    PRIVATE(this)->clearFlag(SoRayPickActionP::CLIP_NEAR);
    neardistance = 1.0f;
    // make sure neardistance is smaller than fardistance
    if (fardistance > 0.0f && neardistance >= fardistance) {
      neardistance = fardistance * 0.01f;
    }
  }

  if (fardistance >= 0.0f) PRIVATE(this)->setFlag(SoRayPickActionP::CLIP_FAR);
  else {
    PRIVATE(this)->clearFlag(SoRayPickActionP::CLIP_FAR);
    // just set to some value bigger than neardistance.
    fardistance = neardistance + 10.0f;
  }

  // set these to some values. They will be set to better values
  // in computeWorldSpaceRay() (when we know the view volume).
  PRIVATE(this)->rayradiusstart = 0.01;
  PRIVATE(this)->rayradiusdelta = 0.0;

  PRIVATE(this)->raystart.setValue(start);
  PRIVATE(this)->raydirection.setValue(direction);
  (void) PRIVATE(this)->raydirection.normalize();
  PRIVATE(this)->raynear = neardistance;
  PRIVATE(this)->rayfar = fardistance;
  PRIVATE(this)->wsline = SbDPLine(PRIVATE(this)->raystart,
                                   PRIVATE(this)->raystart + PRIVATE(this)->raydirection);

  // D = shortest distance from origin to plane
  const double D = PRIVATE(this)->raydirection.dot(PRIVATE(this)->raystart);
  PRIVATE(this)->nearplane = SbDPPlane(PRIVATE(this)->raydirection, D + PRIVATE(this)->raynear);

  PRIVATE(this)->setFlag(SoRayPickActionP::WS_RAY_SET);

  // We use a real cone for picking, but keep pick view volume in sync to be
  // compatible with OIV
  PRIVATE(this)->wsvolume.perspective(0.0, 1.0, neardistance, fardistance);
  PRIVATE(this)->wsvolume.translateCamera(start);
  PRIVATE(this)->wsvolume.rotateCamera(SbRotation(SbVec3f(0.0f, 0.0f, -1.0f), direction));
  PRIVATE(this)->setFlag(SoRayPickActionP::OSVOLUME_DIRTY);
}

/*!
  Lets you decide whether or not all the objects the ray intersects
  with should be picked. If not, only the intersection point of the
  object closest to the camera will be picked.

  Default value of the "pick all" flag is \c FALSE.
*/
void
SoRayPickAction::setPickAll(const SbBool flag)
{
  if (flag) PRIVATE(this)->setFlag(SoRayPickActionP::PICK_ALL);
  else PRIVATE(this)->clearFlag(SoRayPickActionP::PICK_ALL);
}

/*!
  Returns whether only the closest object or all the objects the ray
  intersects with is picked.

  \sa setPickAll()
*/
SbBool
SoRayPickAction::isPickAll(void) const
{
  return PRIVATE(this)->isFlagSet(SoRayPickActionP::PICK_ALL);
}

/*!
  Returns a list of the picked points.
*/
const SoPickedPointList &
SoRayPickAction::getPickedPointList(void) const
{
  int n = PRIVATE(this)->pickedpointlist.getLength();
  if (!PRIVATE(this)->isFlagSet(SoRayPickActionP::PPLIST_IS_SORTED) && n > 1) {
    SoPickedPoint ** pparray = reinterpret_cast<SoPickedPoint **>(PRIVATE(this)->pickedpointlist.getArrayPtr());
    double * darray = const_cast<double*>(PRIVATE(this)->ppdistance.getArrayPtr());

    int i, j, distance;
    SoPickedPoint * pptmp;
    double dtmp;

    // shell sort algorithm (O(nlog(n))
    for (distance = 1; distance <= n/9; distance = 3*distance + 1) ;
    for (; distance > 0; distance /= 3) {
      for (i = distance; i < n; i++) {
        dtmp = darray[i];
        pptmp = pparray[i];
        j = i;
        while (j >= distance && darray[j-distance] > dtmp) {
          darray[j] = darray[j-distance];
          pparray[j] = pparray[j-distance];
          j -= distance;
        }
        darray[j] = dtmp;
        pparray[j] = pptmp;
      }
    }
    SoRayPickActionP * thisp =
      const_cast<SoRayPickActionP *>(&PRIVATE(this).get());
    thisp->setFlag(SoRayPickActionP::PPLIST_IS_SORTED);
  }

  return PRIVATE(this)->pickedpointlist;
}

/*!
  Returns the picked point with \a index in the list of picked points.

  Returns \c NULL if less than \a index + 1 points where picked during
  the last ray pick action.
*/
SoPickedPoint *
SoRayPickAction::getPickedPoint(const int index) const
{
  assert(index >= 0);
  if (index < PRIVATE(this)->pickedpointlist.getLength()) {
    return this->getPickedPointList()[index];
  }
  return NULL;
}

/*!
  \COININTERNAL
 */
void
SoRayPickAction::computeWorldSpaceRay(void)
{
  if (PRIVATE(this)->isFlagSet(SoRayPickActionP::WS_RAY_SET)) {
    // set the ray radius to some very small value, since
    // the user set the ray manually using setRay().
    //
    // FIXME: Wouldn't it be a nice new feature to be able to
    // set the radius of the ray in setRay()? pederb, 2001-01-05
    const SbViewVolume & vv = SoViewVolumeElement::get(this->state);
    PRIVATE(this)->rayradiusstart = SbMin(vv.getWidth(), vv.getHeight()) * FLT_EPSILON;
    PRIVATE(this)->rayradiusdelta = 0.0f;
  }
  else {
    const SbViewVolume & vv = SoViewVolumeElement::get(this->state);
    const SbViewportRegion & vp = SoViewportRegionElement::get(this->state);

    if (!PRIVATE(this)->isFlagSet(SoRayPickActionP::NORM_POINT)) {
      SbVec2s pt = PRIVATE(this)->vppoint - vp.getViewportOriginPixels();
      SbVec2s size = vp.getViewportSizePixels();
      PRIVATE(this)->normvppoint.setValue(float(pt[0]) / float(size[0]),
                                 float(pt[1]) / float(size[1]));
    }

#if COIN_DEBUG
    if (vv.getDepth() == 0.0f || vv.getWidth() == 0.0f || vv.getHeight() == 0.0f) {
      SoDebugError::postWarning("SoRayPickAction::computeWorldSpaceRay",
                                "invalid frustum: <%f, %f, %f>",
                                vv.getWidth(), vv.getHeight(), vv.getDepth());
      return;
    }
#endif // COIN_DEBUG

    SbDPLine templine;
    SbVec2d tmppt;
    tmppt.setValue(PRIVATE(this)->normvppoint);
    vv.getDPViewVolume().projectPointToLine(tmppt, templine);
    PRIVATE(this)->raystart = templine.getPosition();
    PRIVATE(this)->raydirection = templine.getDirection();

    PRIVATE(this)->raynear = 0.0;
    PRIVATE(this)->rayfar = vv.getDPViewVolume().getDepth();

    SbVec2s vpsize = vp.getViewportSizePixels();
    PRIVATE(this)->rayradiusstart = (double(vv.getHeight()) / double(vpsize[1]))*
      double(PRIVATE(this)->radiusinpixels);
    PRIVATE(this)->rayradiusdelta = 0.0;
    if (vv.getProjectionType() == SbViewVolume::PERSPECTIVE) {
      SbVec3d dir(0.0f, vv.getHeight()*0.5f, vv.getNearDist());
      // no need to test here, we know vv isn't empty
      (void) dir.normalize();
      SbVec3d upperfar = dir * (vv.getNearDist()+vv.getDepth()) /
        dir.dot(SbVec3d(0.0f, 0.0f, 1.0f));

      double farheight = double(upperfar[1])*2.0;
      double farsize = (farheight / double(vpsize[1])) * double(PRIVATE(this)->radiusinpixels);
      PRIVATE(this)->rayradiusdelta = (farsize - PRIVATE(this)->rayradiusstart) / double(vv.getDepth());
    }
    PRIVATE(this)->wsline = SbDPLine(PRIVATE(this)->raystart,
                                     PRIVATE(this)->raystart + PRIVATE(this)->raydirection);

    PRIVATE(this)->nearplane = SbDPPlane(vv.getDPViewVolume().getProjectionDirection(),
					 PRIVATE(this)->raystart);
    PRIVATE(this)->setFlag(SoRayPickActionP::WS_RAY_COMPUTED);

    // we pick on a real cone, but keep pick view volume in sync to be
    // compatible with OIV.
    double normradius = double(PRIVATE(this)->radiusinpixels) /
      double(SbMin(vp.getViewportSizePixels()[0], vp.getViewportSizePixels()[1]));

    PRIVATE(this)->wsvolume = vv.narrow(float(PRIVATE(this)->normvppoint[0] - normradius),
                                        float(PRIVATE(this)->normvppoint[1] - normradius),
                                        float(PRIVATE(this)->normvppoint[0] + normradius),
                                        float(PRIVATE(this)->normvppoint[1] + normradius));
    SoPickRayElement::set(state, PRIVATE(this)->wsvolume);
    PRIVATE(this)->setFlag(SoRayPickActionP::OSVOLUME_DIRTY);
  }
}

/*!
  \COININTERNAL
 */
SbBool
SoRayPickAction::hasWorldSpaceRay(void) const
{
  return PRIVATE(this)->isFlagSet(SoRayPickActionP::WS_RAY_SET|SoRayPickActionP::WS_RAY_COMPUTED);
}

/*!
  \COININTERNAL
 */
void
SoRayPickAction::setObjectSpace(void)
{
  PRIVATE(this)->clearFlag(SoRayPickActionP::EXTRA_MATRIX);
  PRIVATE(this)->calcObjectSpaceData(this->state);
  PRIVATE(this)->setPickStyleFlags(this->state);
}

/*!
  \COININTERNAL
 */
void
SoRayPickAction::setObjectSpace(const SbMatrix & matrix)
{
  PRIVATE(this)->setFlag(SoRayPickActionP::EXTRA_MATRIX);
  PRIVATE(this)->extramatrix = SbDPMatrix(matrix);
  PRIVATE(this)->calcObjectSpaceData(this->state);
  PRIVATE(this)->setPickStyleFlags(this->state);
}

/*!
  \COININTERNAL
 */
SbBool
SoRayPickAction::intersect(const SbVec3f & v0_in,
                           const SbVec3f & v1_in,
                           const SbVec3f & v2_in,
                           SbVec3f & intersection, SbVec3f & barycentric,
                           SbBool & front) const
{
  // Calculating intersections when we have a degenerate transform
  // makes no sense. We could do the intersection calculations in
  // world space, but it is impossible to calculate the object space
  // intersection point, so we just return FALSE.
  if (!PRIVATE(this)->objectspacevalid) return FALSE;

  SbVec3d v0,v1,v2;
  v0.setValue(v0_in);
  v1.setValue(v1_in);
  v2.setValue(v2_in);

  const SbVec3d & orig = PRIVATE(this)->osline.getPosition();
  const SbVec3d & dir = PRIVATE(this)->osline.getDirection();

  SbVec3d edge1 = v1 - v0;
  SbVec3d edge2 = v2 - v0;

  SbVec3d pvec = dir.cross(edge2);

  // if determinant is near zero, ray lies in plane of triangle
  double det = edge1.dot(pvec);
  if (fabs(det) < DBL_EPSILON) return FALSE;

  // does ray hit front or back of triangle
  if (det > 0.0) front = TRUE;
  else front = FALSE;

  // create some more intuitive barycentric coordinate names
  double u, v, w;
  double inv_det = 1.0 / det;

  // calculate distance from v0 to ray origin
  SbVec3d tvec = orig - v0;

  // calculate U parameter and test bounds
  u = tvec.dot(pvec) * inv_det;
  if (u < 0.0 || u > 1.0)
    return FALSE;

  // prepare to test V parameter
  SbVec3d qvec = tvec.cross(edge1);

  // calculate V parameter and test bounds
  v = dir.dot(qvec) * inv_det;
  if (v < 0.0 || u + v > 1.0)
    return FALSE;

  // third barycentric coordinate
  w = 1.0 - u - v;

  // calculate t and intersection point
  double t = edge2.dot(qvec) * inv_det;

  SbVec3d itmp = orig + t * dir;
  intersection.setValue(itmp);

  // set the barycentric coordinates before returning
  barycentric[0] = static_cast<float>(w);
  barycentric[1] = static_cast<float>(u);
  barycentric[2] = static_cast<float>(v);

  return TRUE;
}

/*!
  \COININTERNAL
 */
SbBool
SoRayPickAction::intersect(const SbVec3f & v0_in, const SbVec3f & v1_in,
                           SbVec3f & intersection) const
{
  // Calculating intersections when we have a degenerate transform
  // makes no sense. We could do the intersection calculations in
  // world space, but it is impossible to calculate the object space
  // intersection point, so we just return FALSE.
  if (!PRIVATE(this)->objectspacevalid) return FALSE;

  SbVec3d v0, v1;
  v0.setValue(v0_in);
  v1.setValue(v1_in);

  // test if we have a valid line, and do point intersection testing
  // if we don't
  if (v0 == v1) {
    intersection = v0_in;
    // this might return TRUE or FALSE. We already set the
    // intersection point.
    return this->intersect(v0_in);
  }

  SbDPLine line(v0, v1);
  SbVec3d op0, op1; // object space
  SbVec3d p0, p1; // world space

  if (!PRIVATE(this)->osline.getClosestPoints(line, op0, op1)) return FALSE;

  // clamp op1 between v0 and v1
  if ((op1-v0).dot(line.getDirection()) < 0.0) op1 = v0;
  else if ((v1-op1).dot(line.getDirection()) < 0.0) op1 = v1;

  PRIVATE(this)->obj2world.multVecMatrix(op0, p0);
  PRIVATE(this)->obj2world.multVecMatrix(op1, p1);

  // distance between points
  double distance = (p1-p0).length();

  double raypos = PRIVATE(this)->nearplane.getDistance(p0);

  double radius = static_cast<float>((PRIVATE(this)->rayradiusstart +
                           PRIVATE(this)->rayradiusdelta * raypos));

  if (radius >= distance) {
    intersection.setValue(op1);
    return TRUE;
  }
  return FALSE;
}

/*!
  \COININTERNAL
 */
SbBool
SoRayPickAction::intersect(const SbVec3f & point_in) const
{
  // Calculating intersections when we have a degenerate transform
  // makes no sense. We could do the intersection calculations in
  // world space, but it is impossible to calculate the object space
  // intersection point, so we just return FALSE.
  if (!PRIVATE(this)->objectspacevalid) return FALSE;

  SbVec3d point;
  point.setValue(point_in);

  SbVec3d wpoint;
  PRIVATE(this)->obj2world.multVecMatrix(point, wpoint);
  SbVec3d ptonline = PRIVATE(this)->wsline.getClosestPoint(wpoint);

  // distance between points
  double distance = (wpoint-ptonline).length();

  double raypos = PRIVATE(this)->nearplane.getDistance(ptonline);

  double radius = static_cast<double>((PRIVATE(this)->rayradiusstart +
                            PRIVATE(this)->rayradiusdelta * raypos));

  return (radius >= distance);
}

// calculates the square distance (smallest possible) from a 2D point
// to a 2D rectangle
static double
dist_to_quad(const double xmin, const double ymin,
             const double xmax, const double ymax,
             const double x, const double y,
             double & cx, double & cy)
{
  if (x < xmin) {
    if (y < ymin) {
      cx = xmin;
      cy = ymin;
      return (x-xmin)*(x-xmin) + (y-ymin)*(y-ymin);
    }
    else if (y > ymax) {
      cx = xmin;
      cy = ymax;
      return (x-xmin)*(x-xmin) + (y-ymax)*(y-ymax);
    }
    else {
      cx = xmin;
      cy = y;
      return (x-xmin)*(x-xmin);
    }
  }
  else if (x > xmax) {
    if (y < ymin) {
      cx = xmax;
      cy = ymin;
      return (x-xmax)*(x-xmax) + (y-ymin) * (y-ymin);
    }
    else if (y > ymax) {
      cx = xmax;
      cy = ymax;
      return (x-xmax)*(x-xmax) + (y-ymax)*(y-ymax);
    }
    else {
      cx = xmax;
      cy = y;
      return (x-xmax)*(x-xmax);
    }
  }
  else {
    if (y < ymin) {
      cx = x;
      cy = ymin;
      return (y-ymin)*(y-ymin);
    }
    else if (y > ymax) {
      cx = x;
      cy = ymax;
      return (y-ymax)*(y-ymax);
    }
    else {
      // inside rectangle
      cx = x;
      cy = y;
      return -1.0;
    }
  }
}

/*!
  \COININTERNAL
*/
SbBool
SoRayPickAction::intersect(const SbBox3f & box, SbVec3f & intersection,
                           const SbBool usefullviewvolume)
{
  // Calculating intersections when we have a degenerate transform
  // makes no sense. We could do the intersection calculations in
  // world space, but it is impossible to calculate the object space
  // intersection point, so we just return FALSE.
  if (!PRIVATE(this)->objectspacevalid) return FALSE;

  const SbDPLine & line = PRIVATE(this)->osline;
  SbVec3d bounds[2];
  bounds[0].setValue(box.getMin());
  bounds[1].setValue(box.getMax());

  SbVec3d ptonray, ptonbox;
  double sqrmindist = DBL_MAX;

  SbBool conepick = usefullviewvolume && !PRIVATE(this)->isFlagSet(SoRayPickActionP::WS_RAY_SET);

  int i;

  if (PRIVATE(this)->isFlagSet(SoRayPickActionP::CLIP_NEAR|SoRayPickActionP::CLIP_FAR)) {
    // check if all points are in front of the near or behind the far
    // clipping plane
    int numnear = 0;
    int numfar = 0;

    for (i = 0; i < 8; i++) {
      SbVec3d bp(i&1 ? bounds[0][0] : bounds[1][0],
                 i&2 ? bounds[0][1] : bounds[1][1],
                 i&4 ? bounds[0][2] : bounds[1][2]);
      PRIVATE(this)->obj2world.multVecMatrix(bp, bp);
      double dist = PRIVATE(this)->nearplane.getDistance(bp);
      if (PRIVATE(this)->isFlagSet(SoRayPickActionP::CLIP_NEAR)) {
        if (dist < 0.0) numnear++;
      }
      if (PRIVATE(this)->isFlagSet(SoRayPickActionP::CLIP_FAR)) {
        if (dist > (PRIVATE(this)->rayfar - PRIVATE(this)->raynear)) numfar++;
      }
      if ((numnear < i) && (numfar < i)) break;
    }
    if (numnear == 8 || numfar == 8) return FALSE;
  }

  for (int j = 0; j < 2; j++) {
    for (i = 0; i < 3; i++) {
      SbVec3d norm(0.0f, 0.0f, 0.0f);
      norm[i] = 1.0f;
      SbVec3d isect;

      SbDPPlane plane(norm, bounds[j][i]);
      if (plane.intersect(line, isect)) {
        int i1 = (i+1) % 3;
        int i2 = (i+2) % 3;
        double x, y;

        double d = dist_to_quad(bounds[0][i1], bounds[0][i2],
                                bounds[1][i1], bounds[1][i2],
                                isect[i1], isect[i2],
                                x, y);
        if (d <= 0.0f) {
          // center of ray hit box directly
          intersection.setValue(isect);
          return TRUE;
        }
        else if (d < sqrmindist) {
          sqrmindist = d;
          ptonray = ptonbox = isect;
          ptonbox[i1] = x;
          ptonbox[i2] = y;
        }
      }
    }
  }
  if (sqrmindist != DBL_MAX && conepick) {
    // transform ptonray and ptonbox to world space to test on ray cone
    SbVec3d wptonray, wptonbox;
    PRIVATE(this)->obj2world.multVecMatrix(ptonbox, wptonbox);
    PRIVATE(this)->obj2world.multVecMatrix(ptonray, wptonray);

    double raypos = PRIVATE(this)->nearplane.getDistance(wptonray);
    double distance = (wptonray-wptonbox).length();

    // find ray radius at wptonray
    double radius = static_cast<float>((PRIVATE(this)->rayradiusstart +
                             PRIVATE(this)->rayradiusdelta * raypos));

    // test for cone intersection
    if (radius >= distance) {
      intersection.setValue(ptonbox); // set intersection to the point on box closest to ray
      return TRUE;
    }
  }
  return FALSE;
}


/*!
  \COININTERNAL
 */
SbBool
SoRayPickAction::intersect(const SbBox3f & box, const SbBool usefullviewvolume)
{
  SbVec3f dummy;
  return this->intersect(box, dummy, usefullviewvolume);
}

/*!
  \COININTERNAL
 */
const SbViewVolume &
SoRayPickAction::getViewVolume(void)
{
  if (PRIVATE(this)->objectspacevalid &&
      PRIVATE(this)->isFlagSet(SoRayPickActionP::OSVOLUME_DIRTY)) {
    // we pick on a real cone, but calculate pick view volume
    // to be compatible with OIV.
    PRIVATE(this)->osvolume = SoPickRayElement::get(this->getState());
    if (PRIVATE(this)->isFlagSet(SoRayPickActionP::EXTRA_MATRIX)) {
      SbDPMatrix m = PRIVATE(this)->world2obj * PRIVATE(this)->extramatrix;
      SbMatrix tmp(
                 static_cast<float>(m[0][0]), static_cast<float>(m[0][1]),
                 static_cast<float>(m[0][2]), static_cast<float>(m[0][3]),

                   static_cast<float>(m[1][0]), static_cast<float>(m[1][1]),
                 static_cast<float>(m[1][2]), static_cast<float>(m[1][3]),

                   static_cast<float>(m[2][0]), static_cast<float>(m[2][1]),
                 static_cast<float>(m[2][2]), static_cast<float>(m[2][3]),

                   static_cast<float>(m[3][0]), static_cast<float>(m[3][1]),
                 static_cast<float>(m[3][2]), static_cast<float>(m[3][3])
                 );

      PRIVATE(this)->osvolume.transform(tmp);
    }
    else {
      const SbDPMatrix & m = PRIVATE(this)->world2obj;
      SbMatrix tmp(
                 static_cast<float>(m[0][0]), static_cast<float>(m[0][1]),
                 static_cast<float>(m[0][2]), static_cast<float>(m[0][3]),

                   static_cast<float>(m[1][0]), static_cast<float>(m[1][1]),
                 static_cast<float>(m[1][2]), static_cast<float>(m[1][3]),

                   static_cast<float>(m[2][0]), static_cast<float>(m[2][1]),
                 static_cast<float>(m[2][2]), static_cast<float>(m[2][3]),

                   static_cast<float>(m[3][0]), static_cast<float>(m[3][1]),
                 static_cast<float>(m[3][2]), static_cast<float>(m[3][3])
                 );


      PRIVATE(this)->osvolume.transform(tmp);
    }
    PRIVATE(this)->clearFlag(SoRayPickActionP::OSVOLUME_DIRTY);
  }
  return PRIVATE(this)->osvolume;
}

/*!
  \COININTERNAL
 */
const SbLine &
SoRayPickAction::getLine(void)
{
  return PRIVATE(this)->osline_sp;
}

/*!
  \COININTERNAL
 */
SbBool
SoRayPickAction::isBetweenPlanes(const SbVec3f & intersection_in) const
{
  SbVec3d intersection;
  intersection.setValue(intersection_in);
  SbVec3d worldpoint;
  PRIVATE(this)->obj2world.multVecMatrix(intersection, worldpoint);
  return PRIVATE(this)->isBetweenPlanesWS(worldpoint,
                                          SoClipPlaneElement::getInstance(this->state));
}

/*!
  \COININTERNAL
*/
SoPickedPoint *
SoRayPickAction::addIntersection(const SbVec3f & objectspacepoint_in, SbBool frontpick)
{
  if (PRIVATE(this)->isFlagSet(SoRayPickActionP::CULL_BACKFACES) && !frontpick)
    return NULL;

  SbVec3d objectspacepoint;
  objectspacepoint.setValue(objectspacepoint_in);
  SbVec3d worldpoint;
  PRIVATE(this)->obj2world.multVecMatrix(objectspacepoint, worldpoint);
  double dist = PRIVATE(this)->isFlagSet(SoRayPickActionP::PUSH_PICK_TO_FRONT) ?
    0.0 : PRIVATE(this)->nearplane.getDistance(worldpoint);

  if (!PRIVATE(this)->isFlagSet(SoRayPickActionP::PICK_ALL) && PRIVATE(this)->pickedpointlist.getLength()) {
    // got to test if new candidate is closer than old one
    if (dist >= PRIVATE(this)->ppdistance[0]) return NULL; // farther
    // remove old point
    PRIVATE(this)->pickedpointlist.truncate(0);
    PRIVATE(this)->ppdistance.truncate(0);
  }

  // create the new picked point
  SoPickedPoint * pp = new SoPickedPoint(this->getCurPath(),
                                         this->state, objectspacepoint_in);
  PRIVATE(this)->pickedpointlist.append(pp);
  PRIVATE(this)->ppdistance.append(dist);
  PRIVATE(this)->clearFlag(SoRayPickActionP::PPLIST_IS_SORTED);
  return pp;
}

/*!
  Truncates the internal picked points list.

  \since Coin 2.2
*/
void
SoRayPickAction::reset(void)
{
  PRIVATE(this)->cleanupPickedPoints();
}

// Documented in superclass.
void
SoRayPickAction::beginTraversal(SoNode * node)
{
  PRIVATE(this)->cleanupPickedPoints();
  this->getState()->push();
  SoViewportRegionElement::set(this->getState(), this->vpRegion);

  if (PRIVATE(this)->isFlagSet(SoRayPickActionP::WS_RAY_SET)) {
    SoPickRayElement::set(state, PRIVATE(this)->wsvolume);
  }
  inherited::beginTraversal(node);
  this->getState()->pop();
}


//////// Hidden private methods for //////////////////////////////////////
//////// SoRayPickActionP (pimpl) ////////////////////////////////////////

SbBool
SoRayPickActionP::isBetweenPlanesWS(const SbVec3d & intersection,
                                    const SoClipPlaneElement * planes) const
{
  SbVec3f isect_f;
  isect_f.setValue(intersection);
  double dist = this->nearplane.getDistance(intersection);
  if (this->isFlagSet(CLIP_NEAR)) {
    if (dist < 0) return FALSE;
  }
  if (this->isFlagSet(CLIP_FAR)) {
    if (dist > (this->rayfar - this->raynear)) return FALSE;
  }
  int n =  planes->getNum();
  for (int i = 0; i < n; i++) {
    if (!planes->get(i).isInHalfSpace(isect_f)) return FALSE;
  }
  return TRUE;
}

void
SoRayPickActionP::cleanupPickedPoints(void)
{
  this->pickedpointlist.truncate(0); // this will delete all SoPickedPoint instances in the list
  this->ppdistance.truncate(0);
  this->clearFlag(PPLIST_IS_SORTED);
}

void
SoRayPickActionP::setFlag(const unsigned int flag)
{
  this->flags |= flag;
}

void
SoRayPickActionP::clearFlag(const unsigned int flag)
{
  this->flags &= ~flag;
}

SbBool
SoRayPickActionP::isFlagSet(const unsigned int flag) const
{
  return (this->flags & flag) != 0;
}

void
SoRayPickActionP::calcObjectSpaceData(SoState * ownerstate)
{
  this->calcMatrices(ownerstate);

  SbVec3d start, dir;

  if (this->objectspacevalid) {
    this->world2obj.multVecMatrix(this->raystart, start);
    this->world2obj.multDirMatrix(this->raydirection, dir);
    this->osline = SbDPLine(start, start + dir);

    SbVec3f tmp1, tmp2;
    tmp1.setValue(start);

    // scale direction with depth to avoid that line gets no direction
    // when we convert it to single precision below.
    dir *= this->rayfar;
    tmp2.setValue(dir);

    this->osline_sp = SbLine(tmp1, tmp1 + tmp2);
  }
}

void
SoRayPickActionP::calcMatrices(SoState * state)
{
  const SbMatrix & tmp = SoModelMatrixElement::get(state);
  this->obj2world = SbDPMatrix(tmp);
  if (this->isFlagSet(EXTRA_MATRIX)) {
    this->obj2world.multLeft(this->extramatrix);
  }
  this->world2obj = this->obj2world.inverse();
  // FIXME: find a safe way to test if we were able to properly calculate the inverse matrix
  this->objectspacevalid = TRUE;
}

void
SoRayPickActionP::setPickStyleFlags(SoState * state)
{
  assert(state->isElementEnabled(SoPickStyleElement::getClassStackIndex()));
  assert(state->isElementEnabled(SoShapeHintsElement::getClassStackIndex()));

  SoPickStyleElement::Style style = SoPickStyleElement::get(state);
  SoShapeHintsElement::ShapeType type = SoShapeHintsElement::getShapeType(state);
  SoShapeHintsElement::VertexOrdering ordering = SoShapeHintsElement::getVertexOrdering(state);

  // assert(style != SoPickStyleElement::UNPICKABLE); // could we get here?

  if (style == SoPickStyleElement::SHAPE_FRONTFACES &&
      type == SoShapeHintsElement::SOLID &&
      (ordering == SoShapeHintsElement::COUNTERCLOCKWISE ||
       ordering == SoShapeHintsElement::CLOCKWISE)) {
    this->setFlag(CULL_BACKFACES);
  } else {
    this->clearFlag(CULL_BACKFACES);
  }

  if ((style == SoPickStyleElement::SHAPE_ON_TOP) ||
      (style == SoPickStyleElement::BOUNDING_BOX_ON_TOP)) {
    this->setFlag(PUSH_PICK_TO_FRONT);
  } else {
    this->clearFlag(PUSH_PICK_TO_FRONT);
  }
}

#undef PRIVATE
