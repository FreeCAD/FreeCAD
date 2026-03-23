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
  \class SoCamera SoCamera.h Inventor/nodes/SoCamera.h
  \brief The SoCamera class is the abstract base class for camera definition nodes.

  \ingroup coin_nodes

  To be able to view a scene, one needs to have a camera in the scene
  graph. A camera node will set up the projection and viewing matrices
  for rendering of the geometry in the scene.

  This node just defines the abstract interface by collecting common
  fields that all camera type nodes needs. Use the non-abstract camera
  node subclasses within a scene graph. The ones that are default part
  of the Coin library are SoPerspectiveCamera and
  SoOrthographicCamera, which uses the two different projections given
  by their name.

  Note that the viewer components of the GUI glue libraries of Coin
  (SoXt, SoQt, SoWin, etc) will automatically insert a camera into a
  scene graph if none has been defined.

  It is possible to have more than one camera in a scene graph. One
  common trick is for instance to use a second camera to display
  static geometry or overlay geometry (e.g. for head-up displays
  ("HUD")), as shown by this example code:

  \code
  #include <Inventor/Qt/SoQt.h>
  #include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
  #include <Inventor/nodes/SoNodes.h>

  int
  main(int argc, char ** argv)
  {
    QWidget * mainwin = SoQt::init(argv[0]);

    SoSeparator * root = new SoSeparator;
    root->ref();

    // Adds a camera and a red cone. The first camera found in the
    // scene graph by the SoQtExaminerViewer will be picked up and
    // initialized automatically.

    root->addChild(new SoPerspectiveCamera);
    SoMaterial * material = new SoMaterial;
    material->diffuseColor.setValue(1.0, 0.0, 0.0);
    root->addChild(material);
    root->addChild(new SoCone);


    // Set up a second camera for the remaining geometry. This camera
    // will not be picked up and influenced by the viewer, so the
    // geometry will be kept static.

    SoPerspectiveCamera * pcam = new SoPerspectiveCamera;
    pcam->position = SbVec3f(0, 0, 5);
    pcam->nearDistance = 0.1;
    pcam->farDistance = 10;
    root->addChild(pcam);

    // Adds a green cone to demonstrate static geometry.

    SoMaterial * greenmaterial = new SoMaterial;
    greenmaterial->diffuseColor.setValue(0, 1.0, 0.0);
    root->addChild(greenmaterial);
    root->addChild(new SoCone);


    SoQtExaminerViewer * viewer = new SoQtExaminerViewer(mainwin);
    viewer->setSceneGraph(root);
    viewer->show();

    SoQt::show(mainwin);
    SoQt::mainLoop();

    delete viewer;
    root->unref();
    return 0;
  }
  \endcode

  NB: The support for multiple cameras in Coin is limited, and problems with multiple cameras will be considered fixed on a case by case basis.
*/

#include <Inventor/nodes/SoCamera.h>

#include <cfloat> // for FLT_EPSILON

#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/elements/SoFocalDistanceElement.h>
#include <Inventor/elements/SoGLProjectionMatrixElement.h>
#include <Inventor/elements/SoGLViewingMatrixElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/elements/SoGLLineWidthElement.h>
#include <Inventor/elements/SoGLShapeHintsElement.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/elements/SoGLRenderPassElement.h>
#include <Inventor/elements/SoListenerPositionElement.h>
#include <Inventor/elements/SoListenerOrientationElement.h>
#include <Inventor/elements/SoListenerDopplerElement.h>
#include <Inventor/elements/SoListenerGainElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/SbColor4f.h>
#include <Inventor/C/glue/gl.h>
#include <Inventor/C/tidbits.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "elements/GL/SoResetMatrixElement.h"
#include "nodes/SoSubNodeP.h"

/*!
  \enum SoCamera::ViewportMapping

  Enumerates the available possibilities for how the render frame
  should map the viewport.
*/

/*!
  \var SoSFEnum SoCamera::viewportMapping

  Set up how the render frame should map the viewport. The default is
  SoCamera::ADJUST_CAMERA.
*/
/*!
  \var SoSFVec3f SoCamera::position

  Camera position. Defaults to <0,0,1>.
*/
/*!
  \var SoSFRotation SoCamera::orientation

  Camera orientation specified as a rotation value from the default
  orientation where the camera is pointing along the negative Z-axis,
  with "up" along the positive Y-axis.

  E.g., to rotate the camera to point along the X-axis:

  \code
  mycamera->orientation.setValue(SbRotation(SbVec3f(0, 1, 0), M_PI / 2.0f));
  \endcode

  For queries, e.g. to get the current "up" and "look at" vectors of
  the camera:

  \code
  SbRotation camrot = mycamera->orientation.getValue();

  SbVec3f upvec(0, 1, 0); // init to default up vector
  camrot.multVec(upvec, upvec);

  SbVec3f lookat(0, 0, -1); // init to default view direction vector
  camrot.multVec(lookat, lookat);
  \endcode
*/
/*!
  \var SoSFFloat SoCamera::aspectRatio

  Aspect ratio for the camera (i.e. width / height). Defaults to 1.0.
*/
/*!
  \var SoSFFloat SoCamera::nearDistance

  Distance from camera position to the near clipping plane in the
  camera's view volume.

  Default value is 1.0.  Value must be larger than 0.0, or it will not
  be possible to construct a valid viewing volume (for perspective
  rendering, at least).

  If you use one of the viewer components from the So[Xt|Qt|Win|Gtk] GUI
  libraries provided Kongsberg Oil & Gas Technologies, they will automatically
  update this value for the scene camera according to the scene bounding box.
  Ditto for the far clipping plane.

  \sa SoCamera::farDistance
*/
/*!
  \var SoSFFloat SoCamera::farDistance

  Distance from camera position to the far clipping plane in the
  camera's view volume.

  Default value is 10.0.  Must be larger than the
  SoCamera::nearDistance value, or it will not be possible to
  construct a valid viewing volume.

  Note that the range [nearDistance, farDistance] decides the dynamic
  range of the z-buffer in the underlying polygon-rendering
  rasterizer.  What this means is that if the near and far clipping
  planes of the camera are wide apart, the possibility of visual
  artifacts will increase. The artifacts will manifest themselves in
  the form of flickering of primitives close in depth.

  It is therefore a good idea to keep the near and far clipping planes
  of your camera(s) as closely fitted around the geometry of the
  scene graph as possible.

  \sa SoCamera::nearDistance, SoPolygonOffset
*/
/*!
  \var SoSFFloat SoCamera::focalDistance

  Distance from camera position to center of scene.
*/


/*!
  \fn void SoCamera::scaleHeight(float scalefactor)

  Sets a \a scalefactor for the height of the camera viewport. What
  "viewport height" means exactly in this context depends on the
  camera model. See documentation in subclasses.
*/

/*!
  \fn SbViewVolume SoCamera::getViewVolume(float useaspectratio = 0.0f) const

  Returns total view volume covered by the camera under the current
  settings.

  This view volume is not adjusted to account for viewport mapping.
  If you want the same view volume as the one used during rendering,
  you should use getViewVolume(SbViewportRegion & vp, const SbMatrix & mm),
  or do something like this:

  \verbatim

  SbViewVolume vv;
  float aspectratio = myviewport.getViewportAspectRatio();

  switch (camera->viewportMapping.getValue()) {
  case SoCamera::CROP_VIEWPORT_FILL_FRAME:
  case SoCamera::CROP_VIEWPORT_LINE_FRAME:
  case SoCamera::CROP_VIEWPORT_NO_FRAME:
    vv = camera->getViewVolume(0.0f);
    break;
  case SoCamera::ADJUST_CAMERA:
    vv = camera->getViewVolume(aspectratio);
    if (aspectratio < 1.0f) vv.scale(1.0f / aspectratio);
    break;
  case SoCamera::LEAVE_ALONE:
    vv = camera->getViewVolume(0.0f);
    break;
  default:
    assert(0 && "unknown viewport mapping");
    break;
  }

  \endverbatim

  Also, for the CROPPED viewport mappings, the viewport might be
  changed if the viewport aspect ratio is not equal to the camera
  aspect ratio. See the SoCamera::getView() source-code
  (private method) to see how this is done.
*/

/*!
  \fn void SoCamera::viewBoundingBox(const SbBox3f & box, float aspect, float slack)

  Convenience method for setting up the camera definition to cover the
  given bounding \a box with the given \a aspect ratio. Multiplies the
  exact dimensions with a \a slack factor to have some space between
  the rendered model and the borders of the rendering area.

  If you define your own camera node class, be aware that this method
  should \e not set the orientation field of the camera, only the
  position, focal distance and near and far clipping planes.
*/

/*!
  \enum SoCamera::StereoMode
  Enumerates the possible stereo modes.
*/

/*!
  \var SoCamera::MONOSCOPIC
  No stereo.
*/

/*!
  \var SoCamera::LEFT_VIEW
  Left view.
*/

/*!
  \var SoCamera::RIGHT_VIEW
  Right view.
*/

SO_NODE_ABSTRACT_SOURCE(SoCamera);

/*!
  Constructor.
*/
SoCamera::SoCamera()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoCamera);

  SO_NODE_ADD_FIELD(viewportMapping, (ADJUST_CAMERA));
  SO_NODE_ADD_FIELD(position, (0.0f, 0.0f, 1.0f));
  SO_NODE_ADD_FIELD(orientation, (SbRotation::identity()));
  SO_NODE_ADD_FIELD(nearDistance, (1.0f));
  SO_NODE_ADD_FIELD(farDistance, (10.0f));
  SO_NODE_ADD_FIELD(aspectRatio, (1.0f));
  SO_NODE_ADD_FIELD(focalDistance, (5.0f));

  SO_NODE_DEFINE_ENUM_VALUE(ViewportMapping, CROP_VIEWPORT_FILL_FRAME);
  SO_NODE_DEFINE_ENUM_VALUE(ViewportMapping, CROP_VIEWPORT_LINE_FRAME);
  SO_NODE_DEFINE_ENUM_VALUE(ViewportMapping, CROP_VIEWPORT_NO_FRAME);
  SO_NODE_DEFINE_ENUM_VALUE(ViewportMapping, ADJUST_CAMERA);
  SO_NODE_DEFINE_ENUM_VALUE(ViewportMapping, LEAVE_ALONE);

  SO_NODE_SET_SF_ENUM_TYPE(viewportMapping, ViewportMapping);

  this->stereomode = MONOSCOPIC;
  this->stereoadjustment = 0.1f;
  this->balanceadjustment = 1.0f;
}

/*!
  Destructor.
*/
SoCamera::~SoCamera()
{
}

// Doc in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoCamera::initClass(void)
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoCamera, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoGLRenderAction, SoFocalDistanceElement);
  SO_ENABLE(SoGLRenderAction, SoGLProjectionMatrixElement);
  SO_ENABLE(SoGLRenderAction, SoViewVolumeElement);
  SO_ENABLE(SoGLRenderAction, SoGLViewingMatrixElement);
  SO_ENABLE(SoGLRenderAction, SoResetMatrixElement);
  SO_ENABLE(SoGLRenderAction, SoCullElement);

  SO_ENABLE(SoGetBoundingBoxAction, SoFocalDistanceElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoProjectionMatrixElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoViewVolumeElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoViewingMatrixElement);

  SO_ENABLE(SoGetMatrixAction, SoViewVolumeElement);

  SO_ENABLE(SoRayPickAction, SoFocalDistanceElement);
  SO_ENABLE(SoRayPickAction, SoProjectionMatrixElement);
  SO_ENABLE(SoRayPickAction, SoViewVolumeElement);
  SO_ENABLE(SoRayPickAction, SoViewingMatrixElement);

  SO_ENABLE(SoCallbackAction, SoFocalDistanceElement);
  SO_ENABLE(SoCallbackAction, SoProjectionMatrixElement);
  SO_ENABLE(SoCallbackAction, SoViewVolumeElement);
  SO_ENABLE(SoCallbackAction, SoViewingMatrixElement);

  SO_ENABLE(SoGetPrimitiveCountAction, SoFocalDistanceElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoProjectionMatrixElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoViewVolumeElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoViewingMatrixElement);

  SO_ENABLE(SoAudioRenderAction, SoListenerPositionElement);
  SO_ENABLE(SoAudioRenderAction, SoListenerOrientationElement);
  SO_ENABLE(SoAudioRenderAction, SoListenerDopplerElement);
  SO_ENABLE(SoAudioRenderAction, SoListenerGainElement);
}

/*!
  Convenience method which returns the actual view volume used when
  rendering, adjusted for the current viewport mapping.

  Supply the view's viewport in \a vp. If the viewport mapping
  is one of CROP_VIEWPORT_FILL_FRAME, CROP_VIEWPORT_LINE_FRAME or
  CROP_VIEWPORT_NO_FRAME, \a resultvp will be modified to contain the
  resulting viewport.

  If you got any transformations in front of the camera, \a mm should
  contain this transformation.
  
  \since Coin 4.0
*/
SbViewVolume 
SoCamera::getViewVolume(const SbViewportRegion & vp, 
                        SbViewportRegion & resultvp, 
                        const SbMatrix & mm) const
{
  float aspectratio = resultvp.getViewportAspectRatio();
  int vpm = this->viewportMapping.getValue();  
  SbBool adjustvp = FALSE;
  resultvp = vp;
  SbViewVolume resultvv;

  switch (vpm) {
  case CROP_VIEWPORT_FILL_FRAME:
  case CROP_VIEWPORT_LINE_FRAME:
  case CROP_VIEWPORT_NO_FRAME:
    resultvv = this->getViewVolume(0.0f);
    adjustvp = TRUE;
    break;
  case ADJUST_CAMERA:
    resultvv = this->getViewVolume(aspectratio);
    if (aspectratio < 1.0f) resultvv.scale(1.0f / aspectratio);
    break;
  case LEAVE_ALONE:
    resultvv = this->getViewVolume(0.0f);
    break;
  default:
    assert(0 && "unknown viewport mapping");
    break;
  }

  if (mm != SbMatrix::identity()) {
    resultvv.transform(mm);
  }
  if (adjustvp) {
    float cameraratio = this->aspectRatio.getValue();
    if (aspectratio != cameraratio) {
      SbViewportRegion oldvp = resultvp;
      if (aspectratio < cameraratio) {
        resultvp.scaleHeight(aspectratio/cameraratio);
      }
      else {
        resultvp.scaleWidth(cameraratio/aspectratio);
      }
    }
  }
  return resultvv;
}

/*!
  Reorients the camera so that it points towards \a targetpoint.
  The positive Y-axis is used as the up vector of the camera, unless
  the new camera direction is parallel to this axis, in which case the
  positive Z-axis will be used instead.
*/
void
SoCamera::pointAt(const SbVec3f & targetpoint)
{
  SbVec3f dir = targetpoint - this->position.getValue();
  if (dir.normalize() == 0.0f) {
#if COIN_DEBUG
    SoDebugError::postInfo("SoCamera::pointAt",
                           "targetpoint == camera position.");
#endif // debug
    return;
  }

  SbVec3f up(0.0f, 1.0f, 0.0f);

  // use 0,1,0 as the up vector unless direction and up vector are parallel
  if (SbAbs(dir.dot(up)) >= (1.0f - FLT_EPSILON)) up.setValue(0.0f, 0.0f, 1.0f);
  this->lookAt(dir, up);
}

/*!
  Reorients the camera so that it points towards \a targetpoint, using
  \a upvector as the camera up vector.

  \COIN_FUNCTION_EXTENSION
*/
void
SoCamera::pointAt(const SbVec3f & targetpoint, const SbVec3f & upvector)
{
  SbVec3f dir = targetpoint - this->position.getValue();
  if (dir.normalize() == 0.0f) {
#if COIN_DEBUG
    SoDebugError::postInfo("SoCamera::pointAt",
                           "targetpoint == camera position.");
#endif // debug
    return;
  }
  this->lookAt(dir, upvector);
}

// FIXME: should collect common code from the two viewAll() methods
// below. 20010824 mortene.

/*!
  Position the camera so that all geometry of the scene from \a sceneroot
  is contained in the view volume of the camera, while keeping the
  camera orientation constant.

  Finds the bounding box of the scene and calls
  SoCamera::viewBoundingBox(). A bounding sphere will be calculated
  from the scene bounding box, so the camera will "view all" even when
  the scene is rotated, in any way.

  The \a slack argument gives a multiplication factor to the distance
  the camera is supposed to move out from the \a sceneroot mid-point.

  A value less than 1.0 for the \a slack argument will therefore cause
  the camera to come closer to the scene, a value of 1.0 will position
  the camera as exactly outside the scene bounding sphere, and a value
  larger than 1.0 will give "extra slack" versus the scene bounding
  sphere.
*/
void
SoCamera::viewAll(SoNode * const sceneroot, const SbViewportRegion & vpregion,
                  const float slack)
{
  SoGetBoundingBoxAction action(vpregion);
  action.apply(sceneroot);
  SbBox3f box = action.getBoundingBox();
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoCamera::viewAll",
                         "bbox: <%f %f %f>, <%f %f %f>\n",
                         box.getMin()[0], box.getMin()[1], box.getMin()[2],
                         box.getMax()[0], box.getMax()[1], box.getMax()[2]);
  SoDebugError::postInfo("SoCamera::viewAll",
                         "viewportregion, windowsize: <%f, %f>, <%d, %d>\n",
                         vpregion.getViewportSize()[0],
                         vpregion.getViewportSize()[1],
                         vpregion.getWindowSize()[0],
                         vpregion.getWindowSize()[1] );
#endif // debug

  // Only check for "flagged" emptiness and don't use
  // SbBox3f::hasVolume(), as we *can* handle flat boxes (in all
  // dimensions).
  if (box.isEmpty()) { return; }

  // get the actual aspect ratio used while rendering
  float aspectratio = vpregion.getViewportAspectRatio();
  switch (this->viewportMapping.getValue()) {
  case CROP_VIEWPORT_FILL_FRAME:
  case CROP_VIEWPORT_LINE_FRAME:
  case CROP_VIEWPORT_NO_FRAME:
    aspectratio = 1.0f;
    break;
  default:
    break;
  }

  this->viewBoundingBox(box, aspectratio, slack);
}

/*!
  Position the camera so all geometry of the scene in \a path is
  contained in the view volume of the camera.

  Finds the bounding box of the scene and calls
  SoCamera::viewBoundingBox().
*/
void
SoCamera::viewAll(SoPath * const path, const SbViewportRegion & vpregion,
                  const float slack)
{
  SoGetBoundingBoxAction action(vpregion);
  action.apply(path);
  SbBox3f box = action.getBoundingBox();

  // Only check for "flagged" emptiness and don't use
  // SbBox3f::hasVolume(), as we *can* handle flat boxes (in all
  // dimensions).
  if (box.isEmpty()) { return; }

  // get the actual aspect ratio used while rendering
  float aspectratio = vpregion.getViewportAspectRatio();
  switch (this->viewportMapping.getValue()) {
  case CROP_VIEWPORT_FILL_FRAME:
  case CROP_VIEWPORT_LINE_FRAME:
  case CROP_VIEWPORT_NO_FRAME:
    aspectratio = 1.0f;
    break;
  default:
    break;
  }

  this->viewBoundingBox(box, aspectratio, slack);
}

/*!
  Based in the SoCamera::viewportMapping setting, convert the values
  of \a region to the viewport region we will actually render into.
*/
SbViewportRegion
SoCamera::getViewportBounds(const SbViewportRegion & region) const
{
  SbViewportRegion vp = region;
  switch (this->viewportMapping.getValue()) {
  case CROP_VIEWPORT_FILL_FRAME:
  case CROP_VIEWPORT_LINE_FRAME:
  case CROP_VIEWPORT_NO_FRAME:
    {
      float vpaspect = region.getViewportAspectRatio();
      float camaspect = this->aspectRatio.getValue();
      if (vpaspect > camaspect) {
        vp.scaleWidth(camaspect / vpaspect);
        return vp;
      }
      else if (vpaspect < camaspect) {
        vp.scaleHeight(vpaspect / camaspect);
      }
    }
    break;
  default:
    // do nothing
    break;
  }
  return vp;
}

// Doc in superclass.
void
SoCamera::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();

  SbViewportRegion vp;
  SbViewVolume vv;
  this->getView(action, vv, vp, FALSE);

  SbMatrix affine, proj;
  if (vv.getDepth() == 0.0f || vv.getWidth() == 0.0f || vv.getHeight() == 0.0f) {
    // Handle empty scenes.
    affine = proj = SbMatrix::identity();
  }
  else {
    if (this->stereomode != MONOSCOPIC) {
      SbViewVolume copyvv = vv;
      SbMatrix dummy;
      float offset = this->stereoadjustment * 0.5f;
      if (this->stereomode == LEFT_VIEW) offset = -offset;
      SbVec3f r = vv.getProjectionDirection().cross(vv.getViewUp());
      (void) r.normalize();

      // get the current camera transformation/size
      vv.getMatrices(affine, proj);
      affine = affine.inverse();
      float nearv, farv, left, right, top, bottom;
      nearv = vv.getNearDist();
      farv = nearv + vv.getDepth();
      right = vv.getWidth() * 0.5f;
      left = -right;
      top = vv.getHeight() * 0.5f;
      bottom = -top;

      // create a skewed frustum
      float focaldist = this->focalDistance.getValue() * this->balanceadjustment;
      if (focaldist < nearv) focaldist = nearv;
      left -= offset * nearv / focaldist;
      right -= offset * nearv / focaldist;
      vv.frustum(left,right,bottom,top,nearv,farv);

      // transform the skewed view volume to the same location as the original
      vv.transform(affine);
      // translate to account for left/right view
      affine.setTranslate(r * offset);
      vv.transform(affine);
      // read out the stereo view volume
      vv.getMatrices(affine, proj);
    }
    else {
      vv.getMatrices(affine, proj);
    }
    SbBool identity;
    const SbMatrix & mm = SoModelMatrixElement::get(state, identity);
    if (!identity) {
      affine.multRight(mm.inverse());
      vv.transform(SoModelMatrixElement::get(state));
    }
    SoCullElement::setViewVolume(state, vv);
  }

  SoViewVolumeElement::set(state, this, vv);
  if (action->getNumPasses() > 1) {
    SbVec3f jittervec;
    this->jitter(action->getNumPasses(), SoGLRenderPassElement::get(state),
                 vp, jittervec);
    SbMatrix m;
    m.setTranslate(jittervec);
    proj.multRight(m);
  }
  SoProjectionMatrixElement::set(state, this, proj);
  SoViewingMatrixElement::set(state, this, affine);
  SoFocalDistanceElement::set(state, this, this->focalDistance.getValue());
}

// Documented in superclass.
void
SoCamera::audioRender(SoAudioRenderAction *action)
{
  SoState * state = action->getState();

  SbBool setbylistener;
  setbylistener = SoListenerPositionElement::isSetByListener(state);
  if ((! setbylistener) &&  (! this->position.isIgnored())) {
    SbVec3f pos, worldpos;
    pos = this->position.getValue();
    SoModelMatrixElement::get(action->getState()).multVecMatrix(pos, worldpos);
    SoListenerPositionElement::set(state, this, worldpos, FALSE);
#if COIN_DEBUG && 0
  float x, y, z;
  worldpos.getValue(x, y, z);
  SoDebugError::postInfo("SoCamera::audioRender","listenerpos (%0.2f, %0.2f, %0.2f)\n", x, y, z);
#endif // debug
  } else {
#if COIN_DEBUG && 0
  SoDebugError::postInfo("SoCamera::audioRender","ignoring listenerpos\n");
#endif // debug
  }
  setbylistener = SoListenerOrientationElement::isSetByListener(state);
  if ((! setbylistener) && (! this->orientation.isIgnored())) {
    SbBool mmidentity;
    SbRotation r;
    SbMatrix m = SoModelMatrixElement::get(state, mmidentity);
    if (!mmidentity) {
      SbVec3f t;
      SbVec3f s;
      SbRotation so;
      m.getTransform(t, r, s, so);
      r *= this->orientation.getValue();
    }
    else {
      r = this->orientation.getValue();
    }
    SoListenerOrientationElement::set(state, this, r, FALSE);
  }

  // Set view volume. This is needed for LOD nodes to work properly.
  SbViewportRegion vp;
  SbViewVolume vv;
  this->getView(action, vv, vp, FALSE);

  if (! (vv.getDepth() == 0.0f || vv.getWidth() == 0.0f || vv.getHeight() == 0.0f) ) {
    SbBool identity;
    const SbMatrix & mm = SoModelMatrixElement::get(state, identity);
    if (!identity)
      vv.transform(mm);
  }
  SoViewVolumeElement::set(state, this, vv);
}

// Doc in superclass.
void
SoCamera::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoCacheElement::invalidate(action->getState());
  SoCamera::doAction(action);
}

// Doc in superclass.
void
SoCamera::getMatrix(SoGetMatrixAction * action)
{
  SbViewportRegion vp;
  SbViewVolume vv;
  this->getView(action, vv, vp, FALSE);
  vv.transform(action->getMatrix());
  SoViewVolumeElement::set(action->getState(), this, vv);
}

/*!
  Picking actions can be triggered during handle event action
  traversal, and to do picking we need to know the camera state.

  \sa SoCamera::rayPick()
 */
void
SoCamera::handleEvent(SoHandleEventAction * action)
{
  SbViewportRegion vp;
  SbViewVolume vv;
  this->getView(action, vv, vp, FALSE);
  SoViewVolumeElement::set(action->getState(), this, vv);
}

/*!
  "Jitter" the camera according to the current rendering pass (\a
  curpass), to get an antialiased rendering of the scene when doing
  multipass rendering.
*/
void
SoCamera::jitter(int numpasses, int curpass, const SbViewportRegion & vpreg,
                 SbVec3f & jitteramount) const
{
  const int vpsize[2] = { vpreg.getViewportSizePixels()[0], vpreg.getViewportSizePixels()[1] };
  coin_viewvolume_jitter(numpasses, curpass, vpsize, (float*) jitteramount.getValue());
}

// Documented in superclass. Overridden to set up the viewing and
// projection matrices.
void
SoCamera::doAction(SoAction * action)
{
  SoState * state = action->getState();

  SbViewportRegion vp;
  SbViewVolume vv;
  this->getView(action, vv, vp, FALSE);

  SbMatrix affine, proj;
  if (vv.getDepth() == 0.0f || vv.getWidth() == 0.0f || vv.getHeight() == 0.0f) {
    // Handle empty scenes.
    affine = proj = SbMatrix::identity();
  }
  else {
    vv.getMatrices(affine, proj);

    SbBool identity;
    const SbMatrix & mm = SoModelMatrixElement::get(state, identity);
    if (!identity) {
      vv.transform(mm);
      affine.multRight(mm.inverse());
    }
  }
  SoViewVolumeElement::set(state, this, vv);
  SoProjectionMatrixElement::set(state, this, proj);
  SoViewingMatrixElement::set(state, this, affine);
  SoFocalDistanceElement::set(state, this, this->focalDistance.getValue());
}

// Doc in superclass.
void
SoCamera::callback(SoCallbackAction * action)
{
  SoCamera::doAction(action);
}

// Documented in superclass.
void
SoCamera::rayPick(SoRayPickAction * action)
{
  // Overridden to calculate the coordinates of the ray within the
  // current camera settings.

  SoCamera::doAction(action);

  // We need to check for a non-empty view volume, as caused by scene
  // graphs with no geometry in them.
  SbViewVolume vv = this->getViewVolume(1.0f);
  if (vv.getDepth() != 0.0f &&
      vv.getWidth() != 0.0f &&
      vv.getHeight() != 0.0f) {
    action->computeWorldSpaceRay();
  }
}

// Documented in superclass.
void
SoCamera::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  // The number of primitives used to render a shape can change
  // according to the shape's distance to the camera, so we need to
  // override this method from the superclass to modify the traversal
  // state settings for the camera view.

  SoCamera::doAction(action);
}

//
// private method which calculates view volume, and calculates
// new viewport region if viewportMapping requires this.
// The state is updated with the new viewport, not with the
// new view volume.
//
void
SoCamera::getView(SoAction * action, SbViewVolume & resultvv, SbViewportRegion & resultvp,
                  const SbBool considermodelmatrix)
{
  SoState * state = action->getState();
  // need to test if vp element is enabled. SoGetPrimitiveCountAction
  // does not enable this element, although I think it should (to get
  // correct SCREEN_SPACE complexity handling).  pederb, 2001-10-31
  SbBool usevpelement =
    state->isElementEnabled(SoViewportRegionElement::getClassStackIndex());
  
  if (usevpelement) {
    resultvp = SoViewportRegionElement::get(state);
  }
  else {
    // just set it to some value. It's not important as the current
    // action does not support viewports.
    resultvp = SbViewportRegion(256, 256);
  }
  SbMatrix mm = considermodelmatrix ? SoModelMatrixElement::get(state) : SbMatrix::identity();
  SbViewportRegion oldvp(resultvp);
  resultvv = this->getViewVolume(oldvp, resultvp, mm);

  if (resultvp != oldvp) {
    // only draw if this is an SoGLRenderAction
    if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
      this->drawCroppedFrame((SoGLRenderAction*)action, this->viewportMapping.getValue(), oldvp, resultvp);
    }
    if (usevpelement) {
      SoViewportRegionElement::set(action->getState(), resultvp);
    }
  }
}

//
// private method that draws a cropped frame
//
void
SoCamera::drawCroppedFrame(SoGLRenderAction *action,
                           const int viewportmapping,
                           const SbViewportRegion & oldvp,
                           const SbViewportRegion & newvp)
{
  if (viewportmapping == SoCamera::CROP_VIEWPORT_NO_FRAME) return;

  if (action->handleTransparency(FALSE))
    return;

  SoState *state = action->getState();
  state->push();

  if (viewportmapping == SoCamera::CROP_VIEWPORT_LINE_FRAME) {
    SoLineWidthElement::set(state, this, 1.0f);
  }
  else { // FILL
    SoDrawStyleElement::set(state, this, SoDrawStyleElement::FILLED);
    // turn off backface culling
    SoGLShapeHintsElement::forceSend(state, TRUE, FALSE);
  }

  SbVec2s oldorigin = oldvp.getViewportOriginPixels();
  SbVec2s oldsize = oldvp.getViewportSizePixels();
  glMatrixMode(GL_PROJECTION);
  // projection matrix will be set later, so don't push
  glOrtho(oldorigin[0], oldorigin[0]+oldsize[0]-1,
          oldorigin[1], oldorigin[1]+oldsize[1]-1,
          -1, 1);

  SoGLMultiTextureEnabledElement::disableAll(state);

  glPushAttrib(GL_LIGHTING_BIT|
               GL_FOG_BIT|
               GL_DEPTH_BUFFER_BIT|
               GL_TEXTURE_BIT|
               GL_CURRENT_BIT);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glDisable(GL_LIGHTING);
  glDisable(GL_FOG);
  glDisable(GL_DEPTH_TEST);

  glColor3f(0.8f, 0.8f, 0.8f);

  SbVec2s origin = newvp.getViewportOriginPixels();
  SbVec2s size = newvp.getViewportSizePixels();
  SbVec2s orgsize = oldvp.getViewportSizePixels();

  if (size[0] < orgsize[0]) {
    short minpos = origin[0] - 1;
    short maxpos = origin[0] + size[0];
    if (viewportmapping == SoCamera::CROP_VIEWPORT_LINE_FRAME) {
      glBegin(GL_LINES);
      glVertex2s(minpos, oldorigin[1]);
      glVertex2s(minpos, oldorigin[1]+oldsize[1]);
      glVertex2s(maxpos, oldorigin[1]);
      glVertex2s(maxpos, oldorigin[1]+oldsize[1]);
      glEnd();
    }
    else {
      glBegin(GL_QUADS);
      glVertex2s(oldorigin[0], oldorigin[1]);
      glVertex2s(oldorigin[0], oldorigin[1]+oldsize[1]-1);
      glVertex2s(minpos, oldorigin[1]+oldsize[1]);
      glVertex2s(minpos, oldorigin[1]);

      glVertex2s(maxpos, oldorigin[1]);
      glVertex2s(maxpos, oldorigin[1]+oldsize[1]-1);
      glVertex2s(oldorigin[0]+oldsize[0]-1, oldorigin[1]+oldsize[1]-1);
      glVertex2s(oldorigin[0]+oldsize[0]-1, oldorigin[1]);
      glEnd();
    }
  }
  else if (size[1] < orgsize[1]) {
    short minpos = origin[1] - 1;
    short maxpos = origin[1] + size[1];
    if (viewportmapping == SoCamera::CROP_VIEWPORT_LINE_FRAME) {
      glBegin(GL_LINES);
      glVertex2s(oldorigin[0], minpos);
      glVertex2s(oldorigin[0]+oldsize[0], minpos);
      glVertex2s(oldorigin[0], maxpos);
      glVertex2s(oldorigin[0]+oldsize[0], maxpos);
      glEnd();
    }
    else {
      glBegin(GL_QUADS);
      glVertex2s(oldorigin[0], minpos);
      glVertex2s(oldorigin[0]+oldsize[0]-1, minpos);
      glVertex2s(oldorigin[0]+oldsize[0]-1, oldorigin[1]);
      glVertex2s(oldorigin[0], oldorigin[1]);

      glVertex2s(oldorigin[0], maxpos);
      glVertex2s(oldorigin[0], oldorigin[1]+oldsize[1]-1);
      glVertex2s(oldorigin[0]+oldsize[0]-1, oldorigin[1]+oldsize[1]-1);
      glVertex2s(oldorigin[1]+oldsize[0]-1, maxpos);
      glEnd();
    }
  }

  glPopMatrix();
  glPopAttrib();

  state->pop();
}

/*!
  Sets the stereo mode.
*/
void
SoCamera::setStereoMode(StereoMode mode)
{
  this->stereomode = mode;
}

/*!
  Returns the stereo mode.
*/
SoCamera::StereoMode
SoCamera::getStereoMode(void) const
{
  return this->stereomode;
}

/*!
  Sets the stereo adjustment. This is the distance between the left
  and right "eye" when doing stereo rendering.

  When doing stereo rendering, Coin will render two views, one for the
  left eye, and one for the right eye. The stereo adjustment is, a bit
  simplified, how much the camera is translated along the local X-axis
  between the left and the right view.

  The default distance is 0.1, which is chosen since it is the
  approximate distance between the human eyes.

  To create a nice looking and visible stereo effect, the application
  programmer will often have to adjust this value. If all you want to
  do is examine simple standalone 3D objects, it is possible to
  calculate a stereo offset based on the bounding box of the 3D model
  (or scale the model down to an appropriate size).

  However, if you have a large scene, where you want to fly around in
  the scene, and see stereo on different objects as you approach them,
  you can't calculate the stereo offset based on the bounding box
  of the scene, but rather use a stereo offset based on the scale of
  the individual objects/details you want to examine.

  Please note that it is important to set a sensible focal distance
  when doing stereo rendering. See setBalanceAdjustment() for
  information about how the focal distance affects the stereo
  rendering.

  \sa setBalanceAdjustment()
*/
void
SoCamera::setStereoAdjustment(float adjustment)
{
  this->stereoadjustment = adjustment;
}

/*!
  Returns the stereo adjustment.

  \sa setStereoAdjustment()
*/
float
SoCamera::getStereoAdjustment(void) const
{
  return this->stereoadjustment;
}

/*!
  Sets the stereo balance adjustment. This is a factor that enables you to
  move the zero parallax plane. Geometry in front of the zero parallax plane
  will appear to be in front of the screen.

  The balance adjustment is multiplied with the focal distance to find
  the zero parallax plane. The default value is 1.0, and the zero
  parallax plane is then at the focal point.

  \sa SoCamera::focalDistance
*/
void
SoCamera::setBalanceAdjustment(float adjustment)
{
  this->balanceadjustment = adjustment;
}

/*!
  Returns the stereo balance adjustment.

  \sa setBalanceAdjustment()
*/
float
SoCamera::getBalanceAdjustment(void) const
{
  return this->balanceadjustment;
}

// Private method that calculates a new orientation based on camera
// direction and camera up vector. Vectors must be unit length.
void
SoCamera::lookAt(const SbVec3f & dir, const SbVec3f & up)
{
  SbVec3f z = -dir;
  SbVec3f y = up;
  SbVec3f x = y.cross(z);

  // recompute y to create a valid coordinate system
  y = z.cross(x);

  // normalize x and y to create an orthonormal coord system
  if ((y.normalize() == 0.0f) ||
      (x.normalize() == 0.0f)) {
#if COIN_DEBUG
    SoDebugError::postInfo("SoCamera::lookAt",
                           "Unable to create a rotation matrix "
                           "(dir = %g %g %g, up = %g %g %g)\n",
                           dir[0], dir[1], dir[2],
                           up[0], up[1], up[2]);
#endif // debug
    return;
  }

  // create a rotation matrix
  SbMatrix rot = SbMatrix::identity();
  rot[0][0] = x[0];
  rot[0][1] = x[1];
  rot[0][2] = x[2];

  rot[1][0] = y[0];
  rot[1][1] = y[1];
  rot[1][2] = y[2];

  rot[2][0] = z[0];
  rot[2][1] = z[1];
  rot[2][2] = z[2];

  this->orientation.setValue(SbRotation(rot));
}
