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
  \class SoVRMLBackground SoVRMLBackground.h Inventor/VRMLnodes/SoVRMLBackground.h
  \brief The SoVRMLBackground class is used for specifying a viewer panorama.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  Background {
    eventIn      SFBool   set_bind
    exposedField MFFloat  groundAngle  []         # [0,pi/2]
    exposedField MFColor  groundColor  []         # [0,1]
    exposedField MFString backUrl      []
    exposedField MFString bottomUrl    []
    exposedField MFString frontUrl     []
    exposedField MFString leftUrl      []
    exposedField MFString rightUrl     []
    exposedField MFString topUrl       []
    exposedField MFFloat  skyAngle     []         # [0,pi]
    exposedField MFColor  skyColor     0 0 0      # [0,1]
    eventOut     SFBool   isBound
  }
  \endverbatim

  The Background node is used to specify a colour backdrop that
  simulates ground and sky, as well as a background texture, or
  panorama, that is placed behind all geometry in the scene and in
  front of the ground and sky. Background nodes are specified in the
  local coordinate system and are affected by the accumulated rotation
  of their ancestors as described below.  Background nodes are
  bindable nodes as described in 4.6.10, Bindable children nodes
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.10>).
  There exists a Background stack, in which the top-most Background on
  the stack is the currently active Background. To move a Background
  to the top of the stack, a TRUE value is sent to the set_bind
  eventIn.  Once active, the Background is then bound to the browsers
  view. A FALSE value sent to set_bind removes the Background from the
  stack and unbinds it from the browser's view. More detail on the
  bind stack is described in 4.6.10, Bindable children nodes
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.10>).

  The backdrop is conceptually a partial sphere (the ground) enclosed
  inside of a full sphere (the sky) in the local coordinate system
  with the viewer placed at the centre of the spheres. Both spheres
  have infinite radius and each is painted with concentric circles of
  interpolated colour perpendicular to the local Y-axis of the
  sphere. The Background node is subject to the accumulated rotations
  of its ancestors' transformations. Scaling and translation
  transformations are ignored. The sky sphere is always slightly
  farther away from the viewer than the ground partial sphere causing
  the ground to appear in front of the sky where they overlap.  The
  skyColor field specifies the colour of the sky at various angles on
  the sky sphere. The first value of the skyColor field specifies the
  colour of the sky at 0.0 radians representing the zenith (i.e.,
  straight up from the viewer). The skyAngle field specifies the
  angles from the zenith in which concentric circles of colour
  appear. The zenith of the sphere is implicitly defined to be 0.0
  radians, the natural horizon is at pi/2 radians, and the nadir
  (i.e., straight down from the viewer) is at pi radians. skyAngle is
  restricted to non-decreasing values in the range [0.0, pi]. There
  shall be one more skyColor value than there are skyAngle values. The
  first colour value is the colour at the zenith, which is not
  specified in the skyAngle field.  If the last skyAngle is less than
  pi, then the colour band between the last skyAngle and the nadir is
  clamped to the last skyColor.  The sky colour is linearly
  interpolated between the specified skyColor values.

  The groundColor field specifies the colour of the ground at the
  various angles on the ground partial sphere. The first value of the
  groundColor field specifies the colour of the ground at 0.0 radians
  representing the nadir (i.e., straight down from the user). The
  groundAngle field specifies the angles from the nadir that the
  concentric circles of colour appear. The nadir of the sphere is
  implicitly defined at 0.0 radians. groundAngle is restricted to
  non-decreasing values in the range [0.0, pi/2]. There shall be one
  more groundColor value than there are groundAngle values. The first
  colour value is for the nadir which is not specified in the
  groundAngle field. If the last groundAngle is less than pi/2, the
  region between the last groundAngle and the equator is
  non-existent. The ground colour is linearly interpolated between the
  specified groundColor values.

  The backUrl, bottomUrl, frontUrl, leftUrl, rightUrl, and topUrl
  fields specify a set of images that define a background panorama
  between the ground/sky backdrop and the scene's geometry.  The
  panorama consists of six images, each of which is mapped onto a face
  of an infinitely large cube contained within the backdrop spheres
  and centred in the local coordinate system. The images are applied
  individually to each face of the cube. On the front, back, right,
  and left faces of the cube, when viewed from the origin looking down
  the negative Z-axis with the Y-axis as the view up direction, each
  image is mapped onto the corresponding face with the same
  orientation as if the image were displayed normally in 2D (backUrl
  to back face, frontUrl to front face, leftUrl to left face, and
  rightUrl to right face). On the top face of the cube, when viewed
  from the origin looking along the +Y-axis with the +Z-axis as the
  view up direction, the topUrl image is mapped onto the face with the
  same orientation as if the image were displayed normally in 2D. On
  the bottom face of the box, when viewed from the origin along the
  negative Y-axis with the negative Z-axis as the view up direction,
  the bottomUrl image is mapped onto the face with the same
  orientation as if the image were displayed normally in 2D.

  <center>
  <img src="http://www.web3d.org/documents/specifications/14772/V2.0/Images/background.gif">
  Figure 6.1
  </center>

  Figure 6.1 illustrates the Background node backdrop and background
  textures.  Alpha values in the panorama images (i.e., two or four
  component images) specify that the panorama is semi-transparent or
  transparent in regions, allowing the groundColor and skyColor to be
  visible.  See 4.6.11, Texture maps, for a general description of
  texture maps.  Often, the bottomUrl and topUrl images will not be
  specified, to allow sky and ground to show. The other four images
  may depict surrounding mountains or other distant scenery. Browsers
  shall support the JPEG (see 2.[JPEG]) and PNG (see 2.[PNG]) image
  file formats, and in addition, may support any other image format
  (e.g., CGM) that can be rendered into a 2D image. Support for the
  GIF (see E.[GIF]) format is recommended (including transparency).
  More detail on the url fields can be found in 4.5, VRML and the
  World Wide Web
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.5>).


*/

// *************************************************************************

#include <Inventor/VRMLnodes/SoVRMLBackground.h>
#include "coindefs.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#include <Inventor/SbRotation.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SbImage.h>
#include <Inventor/VRMLnodes/SoVRMLImageTexture.h>
#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoDepthBufferElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/lists/SbStringList.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/system/gl.h>
#include <Inventor/C/tidbits.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  SoMFFloat SoVRMLBackground::groundAngle

  The ground angles where different colors should be used.
*/

/*!
  SoMFColor SoVRMLBackground::groundColor

  The color for each groundAngle.
*/

/*!
  SoMFFloat SoVRMLBackground::skyAngle

  The sky angles where different colors should be used.
*/

/*!
  SoMFColor SoVRMLBackground::skyColor

  The color for each skyAngle.
*/

/*!
  SoMFString SoVRMLBackground::backUrl

  URL for the background image.
*/

/*!
  SoMFString SoVRMLBackground::bottomUrl

  URL for the bottom image.
*/

/*!
  SoMFString SoVRMLBackground::frontUrl

  URL for the front image.
*/

/*!
  SoMFString SoVRMLBackground::leftUrl

  URL for the left image.
*/

/*!
  SoMFString SoVRMLBackground::rightUrl

  URL for the right image.
*/

/*!
  SoMFString SoVRMLBackground::topUrl

  URL for the top image.
*/

/*!
  SoSFBool SoVRMLBackground::set_bind
  An eventIn which is triggered when the node is bound.
*/

/*!
  SoSFBool SoVRMLBackground::isBound
  An eventOut that is sent after the node has been bound/unbound.
*/

// *************************************************************************

SO_NODE_SOURCE(SoVRMLBackground);

// *************************************************************************

static char background_scenery_data[] = {
  "#Inventor 2.1 ascii\n\n"
  "  BaseColor { rgb [1 1 1] }\n"
  "  Coordinate3 { point [-1 -1 -1, -1 1 -1, 1 1 -1, 1 -1 -1,   -1 -1 1, -1 1 1, 1 1 1, 1 -1 1]}\n"
  "  TextureCoordinate2 { point [0 0, 1 0, 1 1, 0 1] }\n"
  "  TextureCoordinateBinding { value PER_VERTEX_INDEXED }\n"
};

// *************************************************************************

static void background_geometrychangeCB(void * data, SoSensor * sensor);
static void background_vrmltexturechangeCB(void * data, SoSensor * sensor);
static void background_bindingchangeCB(void * data, SoSensor * sensor);

static float vrmlbackground_viewup[] = {0.0f, 1.0f, 0.0f};
static SbBool vrmlbackground_viewup_set = FALSE;

// *************************************************************************

class SoVRMLBackgroundP {

public:
  SoVRMLBackgroundP(SoVRMLBackground * masterptr) {
    this->master = masterptr;
  };

  SoVRMLBackground * master;

  SoSeparator * rootnode;
  SoPerspectiveCamera * camera;
  SoChildList * children;

  SoFieldSensor * setbindsensor;
  SoFieldSensor * isboundsensor;
  SoFieldSensor * groundanglesensor;
  SoFieldSensor * groundcolorsensor;
  SoFieldSensor * skyanglesensor;
  SoFieldSensor * skycolorsensor;

  SoFieldSensor * backurlsensor;
  SoFieldSensor * fronturlsensor;
  SoFieldSensor * lefturlsensor;
  SoFieldSensor * righturlsensor;
  SoFieldSensor * bottomurlsensor;
  SoFieldSensor * topurlsensor;

  SoVRMLImageTexture * fronttexture;
  SoVRMLImageTexture * backtexture;
  SoVRMLImageTexture * lefttexture;
  SoVRMLImageTexture * righttexture;
  SoVRMLImageTexture * toptexture;
  SoVRMLImageTexture * bottomtexture;

  SoSeparator * frontface;
  SoSeparator * backface;
  SoSeparator * bottomface;
  SoSeparator * topface;
  SoSeparator * leftface;
  SoSeparator * rightface;

  SbStringList directoryList; // used for searching for textures
  SbBool geometrybuilt;

  SoSearchAction * searchaction;
  SoGetMatrixAction * getmatrixaction;

  void buildGeometry(void);
  void modifyCubeFace(SoMFString & urls, SoSeparator * facesep, const int32_t * vindices);
  SoSeparator * createCubeFace(const SoMFString & urls, SoSeparator * sep, const int32_t * vindices);
  void buildIndexList(SoIndexedTriangleStripSet * sphere, int len, int slices, int matlength);

};

#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->master)

// *************************************************************************

// dummy callback for SoGetBoundingBoxAction. A background node has no
// boundingbox, so we just invalidate the bbox cache so stop the node
// from being culled.
static void 
background_bbfix(SoAction * action, SoNode * COIN_UNUSED_ARG(node))
{
  SoCacheElement::invalidate(action->getState());
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLBackground::initClass(void) // static
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLBackground, SO_VRML97_NODE_TYPE);
  SoGetBoundingBoxAction::addMethod(SoVRMLBackground::getClassTypeId(), 
                                    background_bbfix);

  const char * env = coin_getenv("COIN_VIEWUP");
  if (env) {
    float data[3];
    int n = sscanf(env, "%f%f%f", &data[0], &data[1], &data[2]);
    if (n == 3) {
      SbVec3f v(data[0], data[1], data[2]);
      v.normalize();
      vrmlbackground_viewup[0] = v[0];
      vrmlbackground_viewup[1] = v[1];
      vrmlbackground_viewup[2] = v[2];
      vrmlbackground_viewup_set = TRUE;
    }
  }
}

/*!
  Constructor.
*/
SoVRMLBackground::SoVRMLBackground(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLBackground);

  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(groundColor);
  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(skyColor);
  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(groundAngle);
  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(skyAngle);
  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(backUrl);
  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(bottomUrl);
  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(frontUrl);
  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(leftUrl);
  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(rightUrl);
  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(topUrl);

  SO_VRMLNODE_ADD_EVENT_IN(set_bind);
  SO_VRMLNODE_ADD_EVENT_OUT(isBound);

  PRIVATE(this) = new SoVRMLBackgroundP(this);
  PRIVATE(this)->children = new SoChildList(this);
  
  // Binding sensors 
  PRIVATE(this)->setbindsensor = new SoFieldSensor(background_bindingchangeCB, PRIVATE(this));
  PRIVATE(this)->isboundsensor = new SoFieldSensor(background_bindingchangeCB, PRIVATE(this));
 
  PRIVATE(this)->setbindsensor->attach(&this->set_bind);
  PRIVATE(this)->isboundsensor->attach(&this->isBound);

  PRIVATE(this)->setbindsensor->setPriority(5);
  PRIVATE(this)->isboundsensor->setPriority(5);

  // Geometry sensors
  PRIVATE(this)->groundanglesensor = new SoFieldSensor(background_geometrychangeCB, PRIVATE(this));
  PRIVATE(this)->groundcolorsensor = new SoFieldSensor(background_geometrychangeCB, PRIVATE(this));
  PRIVATE(this)->skyanglesensor = new SoFieldSensor(background_geometrychangeCB, PRIVATE(this));
  PRIVATE(this)->skycolorsensor = new SoFieldSensor(background_geometrychangeCB, PRIVATE(this));

  PRIVATE(this)->groundanglesensor->attach(&this->groundAngle);
  PRIVATE(this)->groundcolorsensor->attach(&this->groundColor);
  PRIVATE(this)->skyanglesensor->attach(&this->skyAngle);
  PRIVATE(this)->skycolorsensor->attach(&this->skyColor);

  PRIVATE(this)->groundanglesensor->setPriority(5); 
  PRIVATE(this)->groundcolorsensor->setPriority(5); 
  PRIVATE(this)->skyanglesensor->setPriority(5);
  PRIVATE(this)->skycolorsensor->setPriority(5);

  // URL/skybox sensors  
  PRIVATE(this)->backurlsensor = new SoFieldSensor(background_vrmltexturechangeCB, PRIVATE(this));
  PRIVATE(this)->fronturlsensor = new SoFieldSensor(background_vrmltexturechangeCB, PRIVATE(this));
  PRIVATE(this)->lefturlsensor = new SoFieldSensor(background_vrmltexturechangeCB, PRIVATE(this));
  PRIVATE(this)->righturlsensor = new SoFieldSensor(background_vrmltexturechangeCB, PRIVATE(this));
  PRIVATE(this)->bottomurlsensor = new SoFieldSensor(background_vrmltexturechangeCB, PRIVATE(this));
  PRIVATE(this)->topurlsensor = new SoFieldSensor(background_vrmltexturechangeCB, PRIVATE(this));

  PRIVATE(this)->backurlsensor->attach(&this->backUrl);
  PRIVATE(this)->fronturlsensor->attach(&this->frontUrl);
  PRIVATE(this)->lefturlsensor->attach(&this->leftUrl);
  PRIVATE(this)->righturlsensor->attach(&this->rightUrl);
  PRIVATE(this)->bottomurlsensor->attach(&this->bottomUrl);
  PRIVATE(this)->topurlsensor->attach(&this->topUrl);

  PRIVATE(this)->backurlsensor->setPriority(5);
  PRIVATE(this)->fronturlsensor->setPriority(5);
  PRIVATE(this)->lefturlsensor->setPriority(5);
  PRIVATE(this)->righturlsensor->setPriority(5);
  PRIVATE(this)->bottomurlsensor->setPriority(5);
  PRIVATE(this)->topurlsensor->setPriority(5);

  PRIVATE(this)->geometrybuilt = FALSE;  
  PRIVATE(this)->camera = NULL;
  PRIVATE(this)->rootnode = NULL;

  // actions for ancestors' rotations
  PRIVATE(this)->searchaction = new SoSearchAction;
  PRIVATE(this)->searchaction->setNode(this);
  PRIVATE(this)->getmatrixaction = new SoGetMatrixAction(SbViewportRegion());
}

/*!
  Destructor.
*/
SoVRMLBackground::~SoVRMLBackground()
{
  for (int i = 0; i < PRIVATE(this)->directoryList.getLength(); i++) {
    delete PRIVATE(this)->directoryList[i];
  }
  if (PRIVATE(this)->geometrybuilt) {
    PRIVATE(this)->rootnode->removeAllChildren();
    PRIVATE(this)->rootnode->unref();
  }
  delete PRIVATE(this)->backurlsensor;
  delete PRIVATE(this)->fronturlsensor;
  delete PRIVATE(this)->lefturlsensor;
  delete PRIVATE(this)->righturlsensor;
  delete PRIVATE(this)->bottomurlsensor;
  delete PRIVATE(this)->topurlsensor;

  delete PRIVATE(this)->groundanglesensor;
  delete PRIVATE(this)->groundcolorsensor;
  delete PRIVATE(this)->skyanglesensor;
  delete PRIVATE(this)->skycolorsensor;
  
  delete PRIVATE(this)->setbindsensor;
  delete PRIVATE(this)->isboundsensor;

  delete PRIVATE(this)->searchaction;
  delete PRIVATE(this)->getmatrixaction;
  
  delete PRIVATE(this)->children;
  delete PRIVATE(this);
}

// Doc in parent
void
SoVRMLBackground::GLRender(SoGLRenderAction * action)
{
  if (!PRIVATE(this)->geometrybuilt) { PRIVATE(this)->buildGeometry(); }

  SoState * state = action->getState();

  // push state since we're going to modify the model matrix
  state->push();

  const SbMatrix & tmp = SoViewingMatrixElement::get(state);
  SbRotation rot(tmp);

  if (vrmlbackground_viewup_set) {
    // create a rotation from the positive Y-axis to the new view up
    SbRotation r2(SbVec3f(0.0f, 1.0f, 0.0f), 
                  SbVec3f(vrmlbackground_viewup[0],
                          vrmlbackground_viewup[1],
                          vrmlbackground_viewup[2]));
    r2 *= rot;
    PRIVATE(this)->camera->orientation = r2.inverse();
  }
  else {
    // get the path from the scene graph root to this node
    switch(action->getWhatAppliedTo()) {
      case SoAction::AppliedCode::NODE:
        PRIVATE(this)->searchaction->apply(action->getNodeAppliedTo());
        break;
      case SoAction::AppliedCode::PATH:
        PRIVATE(this)->searchaction->apply(action->getPathAppliedTo());
        break;
      case SoAction::AppliedCode::PATH_LIST:
        PRIVATE(this)->searchaction->apply(*action->getPathListAppliedTo());
        break;
    }
    SoPath * path = PRIVATE(this)->searchaction->getPath();
    if (path != NULL) {
      // get the transformation matrix of this path
      const SbViewportRegion vpr = action->getViewportRegion();
      PRIVATE(this)->getmatrixaction->setViewportRegion(vpr);
      PRIVATE(this)->getmatrixaction->apply(path);
      SbMatrix transformation = PRIVATE(this)->getmatrixaction->getMatrix();
      // get the rotation part of the matrix
      // (note that a SoVRMLBackground node ignores any transformation of ancestors' except rotation)
      SbVec3f translation;
      SbRotation rotation;
      SbVec3f scalevector;
      SbRotation scaleorientation;
      transformation.getTransform(translation, rotation, scalevector, scaleorientation);
      // set camera orientation
      rot = rotation * rot;
    }
    PRIVATE(this)->camera->orientation = rot.inverse();
  }

  // rotate background camera so that it matches the current camera

  // set to identity before rendering subgraph
  SoModelMatrixElement::makeIdentity(state, this);  

  SbBool test_out, write_out;
  SoDepthBufferElement::DepthWriteFunction function_out;
  SbVec2f range_out;
  
  SoDepthBufferElement::get(state, test_out, write_out,
                            function_out, range_out);
  
  range_out[0] = 1.0f;
  range_out[1] = 1.0f;

  SoDepthBufferElement::set(state, test_out, write_out,
                            function_out, range_out);
  int numindices;
  const int * indices;
  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
    PRIVATE(this)->children->traverseInPath(action, numindices, indices);
  }
  else {
    PRIVATE(this)->children->traverse((SoAction *) action);
  }

  // pop back to the old model matrix
  state->pop();
}


void
SoVRMLBackgroundP::buildGeometry(void)
{
  double sphereradius = 1.5;
  SbList <float> angles;
  const int slices = 30; // Number of slices, i.e. vertical resolution of the spheres.
  
  this->rootnode = new SoSeparator;
  this->rootnode->ref();

  // just insert a default perspective camera that is used when
  // rendering the background
  SoPerspectiveCamera * cam = new SoPerspectiveCamera;
  cam->nearDistance = 0.1f;
  this->rootnode->addChild(cam);
  // just set camera pointer for easy lookup in GLRender()
  this->camera = cam;

  // Camera orientation will be changed each time GLRender() is
  // executed -- to match actual scene graph camera -- so we disable
  // notification from the camera to avoid non-stop continuous
  // redraws.
  (void)this->camera->enableNotify(FALSE);

  SoLightModel * lightmodel = new SoLightModel;
  lightmodel->model.setValue(SoLightModel::BASE_COLOR);
  this->rootnode->addChild(lightmodel);
  
  //
  // Sky sphere
  //

  if ((PUBLIC(this)->skyAngle.getNum() > 0) || (PUBLIC(this)->skyColor.getNum() > 0)) {

    angles.append(0);
    float angle = 0.0f;

    if (PUBLIC(this)->skyAngle.getNum() > 0) {
      for (int k=0;k<PUBLIC(this)->skyAngle.getNum();++k) { 
        if (angle > PUBLIC(this)->skyAngle[k]) {
          SoDebugError::postWarning("SoVRMLBackgroundP::buildGeometry","skyAngle array values must be non-decreasing.");
          continue;
        }
        angle = PUBLIC(this)->skyAngle[k];
        if (angle > M_PI) {
          SoDebugError::postWarning("SoVRMLBackgroundP::buildGeometry","skyAngle=%f > PI not allowed.", angle);
          angle = (float) M_PI;
        } else if (angle < 0) {
          SoDebugError::postWarning("SoVRMLBackgroundP::buildGeometry","skyAngle=%f < 0 not allowed.", angle);
          angle = 0.0f;
        } 
        angles.append(angle);
      }
      if (angle != M_PI)
        angles.append((float)M_PI);

    }
    else { // No angles specified. Creating list based on number of colors.
      int num = PUBLIC(this)->skyColor.getNum();
      if (num == 1) ++num; // Special case for one-colored sky.
      for (int i=0;i<=num;++i) 
        angles.append((float) ((M_PI/num)*i));
    }


        
    int len = angles.getLength();

    SbVec3f * skyvertexarray = new SbVec3f[len * slices]; 
    SoIndexedTriangleStripSet * sky = new SoIndexedTriangleStripSet;
    SoVertexProperty * skyproperties = new SoVertexProperty;    
    skyproperties->normalBinding.setValue(SoVertexProperty::PER_VERTEX_INDEXED);

   
    // Calculate vertices and normals
    double x, y, z;
    int counter = 0;
    int i;

    for (i=0;i<slices;++i) {
      for (int j=0;j<len;++j) {
        x = sphereradius * cos(i * ((2 * M_PI) / slices)) * sin(angles[j]);
        y = sphereradius * cos(angles[j]);
        z = sphereradius * sin(i * ((2 * M_PI) / slices)) * sin(angles[j]);
        skyvertexarray[counter++] = SbVec3f((float) x, (float) y, (float) z);
      }
    }
    for (i=0;i<len*slices;++i) {
      skyproperties->vertex.set1Value(i, skyvertexarray[i]);
      SbVec3f normal = -skyvertexarray[i];
      normal.normalize();
      skyproperties->normal.set1Value(i, normal);
    }
    delete [] skyvertexarray;
    sky->vertexProperty.setValue(skyproperties);
    

    // Setup color arrays
    if (PUBLIC(this)->skyColor.getNum() > 0) {
      for (int i=0;i<PUBLIC(this)->skyColor.getNum();++i)
        skyproperties->orderedRGBA.set1Value(i, PUBLIC(this)->skyColor[i].getPackedValue(0));
      skyproperties->materialBinding = SoMaterialBinding::PER_VERTEX_INDEXED;
    } else {
      SoDebugError::postWarning("SoVRMLBackgroundP::buildGeometry","No colors specified for the sky.");
      return;
    }


    buildIndexList(sky, len, slices, PUBLIC(this)->skyColor.getNum());

    SoShapeHints * shapehintssky = new SoShapeHints;
    shapehintssky->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    shapehintssky->shapeType = SoShapeHints::SOLID;
    shapehintssky->faceType = SoShapeHints::CONVEX;

    this->rootnode->addChild(shapehintssky);
    this->rootnode->addChild(sky);  

  }


  
  //
  // Ground sphere
  //

  if ((PUBLIC(this)->groundAngle.getNum() > 0) || (PUBLIC(this)->groundColor.getNum() > 0)) {

    sphereradius = sphereradius * 0.9;
    angles.truncate(0);
    angles.append(0);
    float angle = 0;

    if (PUBLIC(this)->groundAngle.getNum() > 0) {

      for (int k=0;k<PUBLIC(this)->groundAngle.getNum();++k) { 
        if (angle > PUBLIC(this)->groundAngle[k]) {
          SoDebugError::postWarning("SoVRMLBackgroundP::buildGeometry","groundAngle array values must be non-decreasing.");
          continue;
        }
        angle = PUBLIC(this)->groundAngle[k];
        if (angle > M_PI/2) {
          SoDebugError::postWarning("SoVRMLBackgroundP::buildGeometry","groundAngle=%f > PI/2 not allowed.", angle);

          angle = (float) (M_PI / 2.0);
        } else if (angle < 0) {
          SoDebugError::postWarning("SoVRMLBackgroundP::buildGeometry","groundAngle=%f < 0 not allowed.", angle);
          angle = 0;
        } 
        angles.append(angle);
      }
      if (angles.getLength() < 3) // A 'sphere' must have at least 3 faces
        angles.append(angle);

    }
    else {
      int num = PUBLIC(this)->groundColor.getNum();
      if (num == 1) ++num;
      for (int i=0;i<num;++i)
        angles.append((float) ((M_PI/(num))*i));
    }


    int len = angles.getLength();

    SbVec3f * groundvertexarray = new SbVec3f[len * slices];
    SoIndexedTriangleStripSet * ground = new SoIndexedTriangleStripSet;
    SoVertexProperty * groundproperties = new SoVertexProperty;
    groundproperties->normalBinding.setValue(SoVertexProperty::PER_VERTEX_INDEXED);

    // Calculate vertices and normals
    int counter = 0;
    double x,y,z;
    int i;
    for (i=0;i<slices;++i) {
      for (int j=0;j<len;++j) {
        x = sphereradius * cos(i * ((2 * M_PI) / slices)) * sin(angles[j]);
        y = -sphereradius * cos(angles[j]);
        z = sphereradius * sin(i * ((2 * M_PI) / slices)) * sin(angles[j]);
        groundvertexarray[counter++] = SbVec3f((float) x, (float) y, (float) z);
      }
    }
    for (i=0;i<len*slices;++i) {
      groundproperties->vertex.set1Value(i, groundvertexarray[i]);
      SbVec3f normal = -groundvertexarray[i];
      normal.normalize();
      groundproperties->normal.set1Value(i, normal);
    }
    delete [] groundvertexarray;
    ground->vertexProperty.setValue(groundproperties);


    // Setup color arrays
    if (PUBLIC(this)->groundColor.getNum() > 0) {
      for (int i=0;i<PUBLIC(this)->groundColor.getNum();++i)
        groundproperties->orderedRGBA.set1Value(i, PUBLIC(this)->groundColor[i].getPackedValue(0));
      groundproperties->materialBinding = SoMaterialBinding::PER_VERTEX_INDEXED;
    } else {
      SoDebugError::postWarning("SoVRMLBackgroundP::buildGeometry","No colors specified for the ground.");
      return;
    }

    // Build vertex and normal indices
    buildIndexList(ground, len, slices, PUBLIC(this)->groundColor.getNum());
      
    SoShapeHints * shapehintsground = new SoShapeHints;
    shapehintsground->vertexOrdering = SoShapeHints::CLOCKWISE;
    shapehintsground->shapeType = SoShapeHints::SOLID;
    shapehintsground->faceType = SoShapeHints::CONVEX;

    this->rootnode->addChild(shapehintsground);
    this->rootnode->addChild(ground);

  }



  //
  // Scenery cube
  //

  SoDB::init();
  SoInput in;
  in.setBuffer(background_scenery_data, strlen(background_scenery_data));
  SoSeparator * cubedata = SoDB::readAll(&in);

  SoShapeHints * shapehintscube = new SoShapeHints;
  shapehintscube->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
  shapehintscube->shapeType = SoShapeHints::SOLID;
  shapehintscube->faceType = SoShapeHints::CONVEX;
  this->rootnode->addChild(shapehintscube);

  SoScale * scale = new SoScale;
  SbVec3f factor(2, 2, 2);
  scale->scaleFactor.setValue(factor);
  this->rootnode->addChild(scale);
  this->rootnode->addChild(cubedata);
  
  this->frontface = NULL;
  this->backface = NULL;
  this->leftface = NULL;
  this->rightface = NULL;
  this->topface = NULL;
  this->bottomface = NULL;

  if (PUBLIC(this)->backUrl.getNum() != 0) {     
    const int32_t vindices[] = {4, 5, 6, 7, -1};
    SoSeparator * sep = this->createCubeFace(PUBLIC(this)->backUrl,this->backface, vindices);    
    cubedata->addChild(sep);
  }
  
  if (PUBLIC(this)->leftUrl.getNum() != 0) {
    const int32_t vindices[] = {0, 1, 5, 4, -1};
    SoSeparator * sep = this->createCubeFace(PUBLIC(this)->leftUrl,this->leftface, vindices);    
    cubedata->addChild(sep);
  }
  
  if (PUBLIC(this)->frontUrl.getNum() != 0) {
    const int32_t vindices[] = {3, 2, 1, 0, -1};
    SoSeparator * sep = this->createCubeFace(PUBLIC(this)->frontUrl,this->frontface, vindices);    
    cubedata->addChild(sep);
  }

  if (PUBLIC(this)->rightUrl.getNum() != 0) {
    const int32_t vindices[] = {7, 6, 2, 3, -1};
    SoSeparator * sep = this->createCubeFace(PUBLIC(this)->rightUrl,this->rightface, vindices);    
    cubedata->addChild(sep);
  }

  if (PUBLIC(this)->bottomUrl.getNum() != 0) {
    const int32_t vindices[] = {7, 3, 0, 4, -1};
    SoSeparator * sep = this->createCubeFace(PUBLIC(this)->bottomUrl,this->bottomface, vindices);    
    cubedata->addChild(sep);
  }

  if (PUBLIC(this)->topUrl.getNum() != 0) {
    const int32_t vindices[] = {2, 6, 5, 1, -1};
    SoSeparator * sep = this->createCubeFace(PUBLIC(this)->topUrl,this->topface, vindices);    
    cubedata->addChild(sep);
  }
  
  this->children->append(rootnode);
  angles.truncate(0);
 
  this->geometrybuilt = TRUE;
}


void
SoVRMLBackgroundP::buildIndexList(SoIndexedTriangleStripSet * sphere, int len, int slices, int matlength)
{

  // Build vertex and normal indices for triangle strips
  int matindex = 0;
  int counter = 0;
  int i;
  for (i=0;i<slices - 1;++i) {
    for (int j=0;j<len;++j) {
      
      sphere->materialIndex.set1Value(counter,matindex);
      sphere->normalIndex.set1Value(counter, ((i + 1) * len) + j);
      sphere->coordIndex.set1Value(counter++, ((i + 1) * len) + j);
      
      sphere->materialIndex.set1Value(counter, matindex);
      sphere->normalIndex.set1Value(counter, (i * len) + j);
      sphere->coordIndex.set1Value(counter++, (i * len) + j);
      
      ++matindex;
      if (matindex >= matlength) 
        matindex = matlength - 1;
    }
    sphere->materialIndex.set1Value(counter, -1);
    sphere->coordIndex.set1Value(counter, -1);
    sphere->normalIndex.set1Value(counter++, -1);
    matindex = 0;
  }
  
  i = slices - 1;
  for (int j=0;j<len;++j) {
    
    sphere->materialIndex.set1Value(counter, matindex);
    sphere->normalIndex.set1Value(counter, j);
    sphere->coordIndex.set1Value(counter++, j);
    
    sphere->materialIndex.set1Value(counter, matindex);
    sphere->normalIndex.set1Value(counter, (i * len) + j);
    sphere->coordIndex.set1Value(counter++, (i * len) + j);
    
    ++matindex;
    if (matindex >= matlength)
      matindex = matlength - 1;
  }
  
  sphere->materialIndex.set1Value(counter, -1);
  sphere->coordIndex.set1Value(counter, -1);
  sphere->normalIndex.set1Value(counter++, -1);
  
}

SoSeparator * 
SoVRMLBackgroundP::createCubeFace(const SoMFString & urls, SoSeparator * sep, const int32_t * vindices)
{
  const int32_t tindices[] = {1, 2, 3, 0, -1};
  sep = new SoSeparator;
  sep->ref();
  SoVRMLImageTexture * tex = new SoVRMLImageTexture;

  // manually search for each url using directoryList, which is stored in readInstance
  for (int i = 0; i < urls.getNum(); i++) {
    SbString file = SbImage::searchForFile(urls[i], this->directoryList.getArrayPtr(), this->directoryList.getLength());
    if (file == "") file = urls[i];
    tex->url.set1Value(tex->url.getNum(), file);
  }

  tex->repeatS.setValue(FALSE);
  tex->repeatT.setValue(FALSE);
  SoIndexedFaceSet * faceset = new SoIndexedFaceSet;
  faceset->coordIndex.setValues(0, 5, vindices);
  faceset->textureCoordIndex.setValues(0, 5, tindices);
  sep->addChild(tex);
  sep->addChild(faceset);
  
  return sep;

}

void
SoVRMLBackgroundP::modifyCubeFace(SoMFString & urls, SoSeparator * sep, const int32_t * vindices)
{

  SoVRMLImageTexture * tex;

  if (urls.getNum() == 0) {
    if (sep != NULL) {
      sep->unref();
      sep = NULL;
    }
    return;
  }
  else if (sep == NULL) { 
    sep = new SoSeparator;
    sep->ref();

    tex = new SoVRMLImageTexture;
    tex->ref();
    tex->repeatS.setValue(FALSE);
    tex->repeatT.setValue(FALSE);
    const int32_t tindices[] = {1, 2, 3, 0, -1};
    SoIndexedFaceSet * faceset = new SoIndexedFaceSet;
    faceset->textureCoordIndex.setValues(0, 5, tindices);

    faceset->coordIndex.setValues(0, 5, vindices);
    sep->addChild(tex);
    sep->addChild(faceset);
  }
  else {
    tex = (SoVRMLImageTexture *) sep->getChild(0);
  }
  tex->url = urls;

}


void
background_vrmltexturechangeCB(void * data, SoSensor * sensor)
{

  SoVRMLBackgroundP * pimpl = (SoVRMLBackgroundP *) data;

  if (!pimpl->geometrybuilt)
    pimpl->buildGeometry();

  SoVRMLImageTexture * tex = new SoVRMLImageTexture;
  tex->ref();
  tex->repeatS.setValue(FALSE);
  tex->repeatT.setValue(FALSE);
  const int32_t tindices[] = {1, 2, 3, 0, -1};
  SoIndexedFaceSet * faceset = new SoIndexedFaceSet;
  faceset->textureCoordIndex.setValues(0, 5, tindices);
  
  if (sensor == pimpl->backurlsensor) {
    const int32_t vindices[] = {4, 5, 6, 7, -1};
    pimpl->modifyCubeFace(pimpl->master->backUrl, pimpl->backface, vindices);
  }
  else if (sensor == pimpl->lefturlsensor) {
    const int32_t vindices[] = {0, 1, 5, 4, -1};
    pimpl->modifyCubeFace(pimpl->master->leftUrl, pimpl->leftface, vindices);
  }
  else if (sensor == pimpl->fronturlsensor) {
    const int32_t vindices[] = {3, 2, 1, 0, -1};
    pimpl->modifyCubeFace(pimpl->master->frontUrl, pimpl->frontface, vindices);
  }
  else if (sensor == pimpl->righturlsensor) {
    const int32_t vindices[] = {7, 6, 2, 3, -1};
    pimpl->modifyCubeFace(pimpl->master->rightUrl, pimpl->rightface, vindices);
  }
  else if (sensor == pimpl->bottomurlsensor) {
    const int32_t vindices[] = {7, 3, 0, 4, -1};
    pimpl->modifyCubeFace(pimpl->master->bottomUrl, pimpl->bottomface, vindices);
  }
  else if (sensor == pimpl->topurlsensor) {
    const int32_t vindices[] = {2, 6, 5, 1, -1};
    pimpl->modifyCubeFace(pimpl->master->topUrl, pimpl->topface, vindices);
  }
}

void
background_geometrychangeCB(void * data, SoSensor * COIN_UNUSED_ARG(sensor))
{
  SoVRMLBackgroundP * pimpl = (SoVRMLBackgroundP *) data;

  if (pimpl->rootnode) {
    pimpl->rootnode->removeAllChildren();
    pimpl->rootnode->unref();
    pimpl->rootnode = NULL;
  }
  pimpl->buildGeometry();
}

void
background_bindingchangeCB(void * data, SoSensor * sensor)
{
  SoVRMLBackgroundP * pimpl = (SoVRMLBackgroundP *) data;

  // FIXME: Support for 'set_bind' and 'isBound' must be implemented.
  // But first, a Coin viewer must support this kind of special node
  // treatment (this applies to 'Fog', 'NavigationInfo' and 'Viewport'
  // nodes as well) (11Aug2003 handegar)

  if (sensor == pimpl->setbindsensor) {
    SoDebugError::postWarning("background_bindingchangeCB", "'set_bind' event not implemented yet");
  }
  else if (sensor == pimpl->isboundsensor) {
    SoDebugError::postWarning("background_bindingchangeCB", "'isBound' event not implemented yet");
  }
}

SbBool 
SoVRMLBackground::readInstance(SoInput * in, unsigned short flags)
{
  SbBool readOK = inherited::readInstance(in, flags);
  if (readOK) {
    // store current search paths
    const SbStringList & sl = SoInput::getDirectories();
    for (int i = 0; i < sl.getLength(); i++) {
      PRIVATE(this)->directoryList.append(new SbString(*(sl[i])));
    }
  }
  return readOK;
}

#undef PRIVATE
#undef PUBLIC

#endif // HAVE_VRML97
