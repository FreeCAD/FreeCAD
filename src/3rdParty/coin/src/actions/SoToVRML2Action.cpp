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
  \class SoToVRML2Action SoToVRML2Action.h Inventor/actions/SoToVRML2Action.h
  \brief The SoToVRML2Action class builds a new scene graph, using only VRML97/VRML2 nodes.

  \ingroup coin_actions

  This action is used for converting a scene graph of VRML1/Coin nodes
  to a new scene graph using only VRML97/VRML2 nodes.

  Due to the basic differences between VRML1/Coin and VRML2 (the
  latter does not really have a traversal state) the new scene graph
  will typically be somewhat larger. To minimize this effect the
  action tries to reuse nodes when possible.

  VRML1 nodes will be converted to their equivalent VRML2 nodes,
  while Coin nodes with no VRML2 equivalent are converted to
  IndexedFaceSet. If the DrawStyle is POINTS, all geometry will be
  built using PointSet; if it is LINES IndexedLineSet is used.

  Here's a basic usage example of this action, in the form of a
  complete, standalone program:

  \code
  #include <Inventor/SoDB.h>
  #include <Inventor/SoInteraction.h>
  #include <Inventor/SoInput.h>
  #include <Inventor/SoOutput.h>
  #include <Inventor/actions/SoWriteAction.h>
  #include <Inventor/actions/SoToVRML2Action.h>
  #include <Inventor/nodes/SoSeparator.h>
  #include <Inventor/VRMLnodes/SoVRMLGroup.h>

  int
  main(int argc, char *argv[])
  {
    SoDB::init();
    SoInteraction::init();
    SoInput in;
    in.openFile(argv[1]);
    printf("Reading...\n");
    SoSeparator *root = SoDB::readAll(&in);

    if (root) {
      root->ref();
      SbString hdr = in.getHeader();
      in.closeFile();

      printf("Converting...\n");
      SoToVRML2Action tovrml2;
      tovrml2.apply(root);
      SoVRMLGroup *newroot = tovrml2.getVRML2SceneGraph();
      newroot->ref();
      root->unref();

      printf("Writing...\n");

      SoOutput out;
      out.openFile("out.wrl");
      out.setHeaderString("#VRML V2.0 utf8");
      SoWriteAction wra(&out);
      wra.apply(newroot);
      out.closeFile();

      newroot->unref();
    }

    return 0;
  }
  \endcode

  Note: if VRML97 support is not present in the Coin library, this
  action does nothing and getVRML2SceneGraph always returns \c NULL.

  \sa SoToVRMLAction

  \since Coin 2.0
  \since TGS Inventor 2.5
*/

// *************************************************************************

// FIXME: SoComplexity::BOUNDING_BOX are not supported. For
// DrawStyle::LINES quads are not handled correctly (will always draw
// triangles). SoArray and SoMultipleCopy are not supported.
// Reusing of appearance and geometry nodes is not implemented.
// 20020813 kristian.

// *************************************************************************

#include <Inventor/actions/SoToVRML2Action.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cfloat>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/SbBSPTree.h>
#include <Inventor/SbName.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/elements/SoSwitchElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/lists/SoNodeList.h>
#include <Inventor/lists/SoPathList.h>
#include <Inventor/nodes/SoNodes.h>
#include <Inventor/nodes/SoTransform.h>

#ifdef HAVE_NODEKITS
#include <Inventor/nodekits/SoBaseKit.h>
#endif // HAVE_NODEKITS

#include "SbBasicP.h"
#include "actions/SoSubActionP.h"
#include "misc/SbHash.h"

// default values for cases where a viewport is needed
#define DEFAULT_VIEWPORT_WIDTH 1024
#define DEFAULT_VIEWPORT_HEIGHT 768


// *************************************************************************

SO_ACTION_SOURCE(SoToVRML2Action);

// *************************************************************************

// helper function needed to copy the name of a node
static SoNode * tovrml2_new_node(SoNode * newnode, const SoNode * oldnode)
{
  const SbName name = oldnode->getName();
  if (name != SbName::empty()) newnode->setName(name);
  return newnode;
}

// We use SoType::createInstance() instead of simply new'ing to make
// an instance, as this makes SoType::overrideType() influence the
// conversion process.

#define NEW_NODE(_type_, _oldnode_) \
        coin_assert_cast<_type_*>(tovrml2_new_node(static_cast<SoNode *>(_type_::getClassTypeId().createInstance()), \
                                   _oldnode_))

// *************************************************************************

/*!
  \copydetails SoAction::initClass(void)
*/
void
SoToVRML2Action::initClass(void)
{
  SO_ACTION_INTERNAL_INIT_CLASS(SoToVRML2Action, SoToVRMLAction);
}

/*!
  \fn SoToVRML2Action::SoToVRML2Action(void)
  Constructor.
*/

/*!
  \fn SoToVRML2Action::~SoToVRML2Action(void)
  The destructor.
*/

/*!
  \fn SoVRMLGroup * SoToVRML2Action::getVRML2SceneGraph(void) const

  Return a pointer to the root node of the generated scene graph of
  only VRML2 / VRML97 nodes.

  Will return \c NULL if VRML97 support was not compiled into the
  library.
*/

/*!
  \fn void SoToVRML2Action::reuseAppearanceNodes(SbBool appearance)

  Set the flag deciding if appearance nodes should be reused if possible.
  The default is FALSE.

  Please note that support for reusing Appearance nodes is not implemented
  yet.
*/

/*!
  \fn SbBool SoToVRML2Action::doReuseAppearanceNodes(void) const

  Get the flag deciding if appearance nodes should be reused if possible.
  The default is FALSE.

  Please note that support for reusing Appearance nodes is not implemented
  yet, so this method will always return FALSE.
*/

/*!
  \fn void SoToVRML2Action::reusePropertyNodes(SbBool property)

  Set the flag deciding if property nodes should be reused if possible.
  The default is FALSE.
*/

/*!
  \fn SbBool SoToVRML2Action::doReusePropertyNodes(void) const

  Get the flag deciding if property nodes should be reused if possible.
  The default is FALSE.
*/

/*!
  \fn void SoToVRML2Action::reuseGeometryNodes(SbBool geometry)

  Set the flag deciding if geometry nodes should be reused if possible.
  The default is FALSE.

  Please note that support for reusing Geometry nodes is not implemented
  yet.
*/

/*!
  \fn SbBool SoToVRML2Action::doReuseGeometryNodes(void) const

  Get the flag deciding if geometry nodes should be reused if possible.
  The default is FALSE.

  Please note that support for reusing Geometry nodes is not implemented
  yet, so this function will always return FALSE.
*/


// *************************************************************************

#ifndef HAVE_VRML97
class SoToVRML2ActionP {
public:
};

SoToVRML2Action::SoToVRML2Action(void)
{
  SO_ACTION_CONSTRUCTOR(SoToVRML2Action);
}

SoToVRML2Action::~SoToVRML2Action() { }
void SoToVRML2Action::apply(SoNode * node) { }
void SoToVRML2Action::apply(SoPath * path) { }
void SoToVRML2Action::apply(const SoPathList & pathlist, SbBool obeysrules) { }
SoVRMLGroup * SoToVRML2Action::getVRML2SceneGraph(void) const { return NULL; }
void SoToVRML2Action::beginTraversal(SoNode * node) { }
void SoToVRML2Action::reuseAppearanceNodes(SbBool appearance) { }
SbBool SoToVRML2Action::doReuseAppearanceNodes(void) const { return FALSE; }
void SoToVRML2Action::reusePropertyNodes(SbBool property) { }
SbBool SoToVRML2Action::doReusePropertyNodes(void) const { return FALSE; }
void SoToVRML2Action::reuseGeometryNodes(SbBool geometry) { }
SbBool SoToVRML2Action::doReuseGeometryNodes(void) const { return FALSE; }
#else // HAVE_VRML97

#include <Inventor/VRMLnodes/SoVRMLNodes.h>
#include <Inventor/VRMLnodes/SoVRML.h>

class SoToVRML2ActionP {
public:
  SoToVRML2ActionP(void)
    : master(NULL),nodefuse(FALSE),reuseAppearanceNodes(FALSE),reuseGeometryNodes(FALSE),
      bboxaction(NULL),vrml2path(NULL),vrml2root(NULL),vrmlcoords(NULL),vrmlnormals(NULL),vrmlcolors(NULL),vrmltexcoords(NULL)
  {}

  ~SoToVRML2ActionP(void)
  {
    delete this->bboxaction;
    if (this->vrml2path) {
        this->vrml2path->unref();
    }
    if (this->vrml2root) {
        this->vrml2root->unref();
    }
    delete this->vrmlcoords;
    delete this->vrmlnormals;
    delete this->vrmlcolors;
    delete this->vrmltexcoords;
  }

  void init(void)
  {
    this->bsptree = NULL;
    this->bsptreetex = NULL;
    this->bsptreenormal = NULL;
    this->coordidx = NULL;
    this->normalidx = NULL;
    this->texidx = NULL;
    this->coloridx = NULL;

    recentTex2 = NULL;
    do_post_primitives = FALSE;
    didpush = FALSE;

    delete this->vrmlcoords;
    delete this->vrmlnormals;
    delete this->vrmlcolors;
    delete this->vrmltexcoords;

    this->vrmlcoords = new SbList <SoVRMLCoordinate *>;
    this->vrmlnormals = new SbList <SoVRMLNormal *>;
    this->vrmlcolors = new SbList <SoVRMLColor *>;
    this->vrmltexcoords = new SbList <SoVRMLTextureCoordinate *>;

    if (this->vrml2path) {
      this->vrml2path->unref();
    }
    this->vrml2path = reclassify_cast<SoFullPath *>(new SoPath);
    this->vrml2path->ref();

    if (this->vrml2root) {
      this->vrml2root->unref();
    }
    this->vrml2root = new SoVRMLGroup;
    this->vrml2root->ref();
    this->vrml2path->setHead(this->vrml2root);
  }

  SoGetBoundingBoxAction * getBBoxAction(void) {
    if (this->bboxaction == NULL) {
      SbViewportRegion vp(DEFAULT_VIEWPORT_WIDTH, DEFAULT_VIEWPORT_HEIGHT);
      this->bboxaction = new SoGetBoundingBoxAction(vp);
    }
    return this->bboxaction;
  }
  float getBBoxDistance(const SbViewVolume & vv, const float screenarea, const float h) {
    const float h2 = h * 0.5f; // use half the height for simplicity
    float screenheight = float(sqrt(screenarea)) * 0.5f; // wanted height in pixels
    float vvheight = vv.getHeight() * 0.5f; // total height of view volume
    float neardist = vv.getNearDist();

    float projheight = (screenheight / 768.0f) * vvheight; // wanted projected height

    // now, find the distance the bbox must be at the achieve this projheight
    if (projheight > 0.0f) {
      return (neardist / projheight) * h2;
    }
    return FLT_MAX; // never switch
  }


  SoToVRML2Action * master;
  SbBool nodefuse;
  SbBool reuseAppearanceNodes;
  SbBool reusePropertyNodes;
  SbBool reuseGeometryNodes;
  SbHash<const SoNode *, SoGroup *> dict;
  SoCallbackAction cbaction;
  SoSearchAction searchaction;
  SbList <SoVRMLGroup*> separatorstack;

  SbBSPTree * bsptree;
  SbBSPTree * bsptreetex;
  SbBSPTree * bsptreenormal;
  SbList <int32_t> * coordidx;
  SbList <int32_t> * normalidx;
  SbList <int32_t> * texidx;
  SbList <int32_t> * coloridx;
  SoGetBoundingBoxAction * bboxaction;

  SoTexture2 * recentTex2;
  SbBool do_post_primitives;
  SbBool didpush;

  static SoCallbackAction::Response unsupported_cb(void *, SoCallbackAction *, const SoNode *);

  SoFullPath * vrml2path;
  SoVRMLGroup * vrml2root;
  SbList <SoVRMLCoordinate *> * vrmlcoords;
  SbList <SoVRMLNormal *> * vrmlnormals;
  SbList <SoVRMLColor *> * vrmlcolors;
  SbList <SoVRMLTextureCoordinate *> * vrmltexcoords;

  SoNode * search_for_recent_node(SoAction * action, const SoType & type);
  SoGroup * get_current_tail(void);
  SoVRMLCoordinate * get_or_create_coordinate(const SbVec4f *, int32_t num);
  SoVRMLCoordinate * get_or_create_coordinate(const SbVec3f *, int32_t num);
  SoVRMLNormal * get_or_create_normal(const SbVec3f *, int32_t num);
  SoVRMLColor * get_or_create_color(const uint32_t * packedColor, int32_t num);
  SoVRMLColor * get_or_create_color(const SbColor *, int32_t num);
  SoVRMLTextureCoordinate * get_or_create_texcoordinate(const SbVec2f *, int32_t num);
  void insert_shape(SoCallbackAction * action, SoVRMLGeometry * geom);

  // Shape nodes
  static SoCallbackAction::Response soasciitext_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response socone_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response socube_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response socylinder_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response soifs_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response soils_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response solineset_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response sopointset_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response sosphere_cb(void *, SoCallbackAction *, const SoNode *);

  // Property nodes
  static SoCallbackAction::Response soinfo_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response solabel_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response somattrans_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response sorotation_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response sorotationxyz_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response soscale_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response sotransform_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response sotranslation_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response sounits_cb(void *, SoCallbackAction *, const SoNode *);

  // Group nodes
  static SoCallbackAction::Response push_sep_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response pop_sep_cb(void *, SoCallbackAction *, const SoNode *);

  static SoCallbackAction::Response push_transformsep_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response pop_transformsep_cb(void *, SoCallbackAction *, const SoNode *);

  static SoCallbackAction::Response push_switch_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response pop_switch_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response push_lod_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response push_levelofdetail_cb(void *, SoCallbackAction *, const SoNode *);

  // Other nodes
  static SoCallbackAction::Response sopercam_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response sodirlight_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response sospotlight_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response sopointlight_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response sowwwinl_cb(void *, SoCallbackAction *, const SoNode *);

  // Convert nodes to SoVRMLIndexedFaceSet via triangle cb
  static SoCallbackAction::Response sotoifs_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response post_primitives_cb(void *, SoCallbackAction *, const SoNode *);
  static void triangle_cb(void * userdata, SoCallbackAction * action,
                          const SoPrimitiveVertex * v1,
                          const SoPrimitiveVertex * v2,
                          const SoPrimitiveVertex * v3);

  // Convert nodes to SoVRMLIndexedLineSet via linesegment cb
  static SoCallbackAction::Response sotoils_cb(void *, SoCallbackAction *, const SoNode *);
  static SoCallbackAction::Response post_lines_cb(void *, SoCallbackAction *, const SoNode *);
  static void linesegment_cb(void * userdata, SoCallbackAction * action,
                          const SoPrimitiveVertex * v1,
                          const SoPrimitiveVertex * v2);
};

#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->master)
#define THISP(p) (static_cast<SoToVRML2ActionP *>(p))

// *************************************************************************

// add type and all shapes inheriting type to the list of shapes
// already handled by the action
static void add_shape_handled(const SoType & type, SoTypeList & addlist)
{
  SoTypeList shapes;
  (void) SoType::getAllDerivedFrom(type, shapes);
  int i;

  for (i = 0; i < shapes.getLength(); i++) {
    SoType s = shapes[i];
    if (s.canCreateInstance() && (addlist.find(s) < 0)) {
      addlist.append(s);
    }
  }
}

// *************************************************************************


SoToVRML2Action::SoToVRML2Action(void)
{
  SO_ACTION_CONSTRUCTOR(SoToVRML2Action);

  PRIVATE(this)->master = this;

#define ADD_PRE_CB(_node_, _cb_) \
  PRIVATE(this)->cbaction.addPreCallback(_node_::getClassTypeId(), SoToVRML2ActionP::_cb_, &PRIVATE(this).get())
#define ADD_POST_CB(_node_, _cb_) \
  PRIVATE(this)->cbaction.addPostCallback(_node_::getClassTypeId(), SoToVRML2ActionP::_cb_, &PRIVATE(this).get())
#define ADD_UNSUPPORTED(_node_) \
  PRIVATE(this)->cbaction.addPreCallback(_node_::getClassTypeId(), SoToVRML2ActionP::unsupported_cb, &PRIVATE(this).get())
#define ADD_TRIANGLE_CB(_node_) \
  PRIVATE(this)->cbaction.addTriangleCallback(_node_::getClassTypeId(), SoToVRML2ActionP::triangle_cb, &PRIVATE(this).get())
#define ADD_LINE_CB(_node_) \
  PRIVATE(this)->cbaction.addLineSegmentCallback(_node_::getClassTypeId(), SoToVRML2ActionP::linesegment_cb, &PRIVATE(this).get())
#define ADD_SHAPE_CB(_node_, _cb_) \
  ADD_PRE_CB(_node_, _cb_); ADD_TRIANGLE_CB(_node_); ADD_POST_CB(_node_, post_primitives_cb); \
  add_shape_handled(_node_::getClassTypeId(), shapehandledlist);

#define ADD_SO_TO_IFS(_node_) \
  ADD_PRE_CB(_node_, sotoifs_cb); ADD_TRIANGLE_CB(_node_); ADD_POST_CB(_node_, post_primitives_cb); \
  add_shape_handled(_node_::getClassTypeId(), shapehandledlist);

#define ADD_SO_TO_ILS(_node_) \
  ADD_PRE_CB(_node_, sotoils_cb); ADD_LINE_CB(_node_); ADD_POST_CB(_node_, post_lines_cb); \
  add_shape_handled(_node_::getClassTypeId(), shapehandledlist);

  SoTypeList shapehandledlist;

  ADD_SHAPE_CB(SoAsciiText, soasciitext_cb);
  ADD_SHAPE_CB(SoCone, socone_cb);
  ADD_SHAPE_CB(SoCube, socube_cb);
  ADD_SHAPE_CB(SoCylinder, socylinder_cb);
  ADD_SHAPE_CB(SoIndexedFaceSet, soifs_cb);
  ADD_SHAPE_CB(SoIndexedLineSet, soils_cb);
  ADD_SHAPE_CB(SoPointSet, sopointset_cb);
  ADD_SHAPE_CB(SoSphere, sosphere_cb);

  // Property nodes
  ADD_PRE_CB(SoInfo, soinfo_cb);
  ADD_PRE_CB(SoLabel, solabel_cb);
  ADD_PRE_CB(SoMatrixTransform, somattrans_cb);
  ADD_PRE_CB(SoRotation, sorotation_cb);
  ADD_PRE_CB(SoRotationXYZ, sorotationxyz_cb);
  ADD_PRE_CB(SoScale, soscale_cb);
  ADD_PRE_CB(SoTransform, sotransform_cb);
  ADD_PRE_CB(SoTranslation, sotranslation_cb);
  ADD_PRE_CB(SoUnits, sounits_cb);

  // Group nodes
  ADD_PRE_CB(SoVRMLGroup, push_sep_cb); // support for VRML97 Transform and Group
  ADD_POST_CB(SoVRMLGroup, pop_sep_cb);

  ADD_PRE_CB(SoSeparator, push_sep_cb);
  ADD_POST_CB(SoSeparator, pop_sep_cb);
  
  ADD_PRE_CB(SoTransformSeparator, push_transformsep_cb);
  ADD_POST_CB(SoTransformSeparator, pop_transformsep_cb);
  
  ADD_PRE_CB(SoSwitch, push_switch_cb);
  ADD_POST_CB(SoSwitch, pop_switch_cb);
  ADD_PRE_CB(SoLOD, push_lod_cb);
  ADD_PRE_CB(SoLevelOfDetail, push_levelofdetail_cb);

  ADD_UNSUPPORTED(SoWWWAnchor); // Convert to SoVRMLAnchor

  // Other nodes
  ADD_UNSUPPORTED(SoOrthographicCamera);
  ADD_PRE_CB(SoPerspectiveCamera, sopercam_cb);
  ADD_PRE_CB(SoDirectionalLight, sodirlight_cb);
  ADD_PRE_CB(SoPointLight, sopointlight_cb);
  ADD_PRE_CB(SoSpotLight, sospotlight_cb);
  ADD_PRE_CB(SoWWWInline, sowwwinl_cb);

  // Coin nodes
  ADD_SHAPE_CB(SoLineSet, solineset_cb);
  ADD_SO_TO_IFS(SoIndexedTriangleStripSet);
  ADD_SO_TO_IFS(SoFaceSet);
  ADD_SO_TO_IFS(SoQuadMesh);
  ADD_SO_TO_IFS(SoTriangleStripSet);

  ADD_SO_TO_IFS(SoNurbsSurface);
  ADD_SO_TO_IFS(SoIndexedNurbsSurface);

  ADD_SO_TO_ILS(SoNurbsCurve);
  ADD_SO_TO_ILS(SoIndexedNurbsCurve);

  // find all shapes not handled earlier, and add generic triangle
  // handling for them
  //
  // FIXME: also add line segment callback and point callback. pederb,
  // 2005-06-10
  SoTypeList shapes;
  (void) SoType::getAllDerivedFrom(SoShape::getClassTypeId(), shapes);
  int i;

  for (i = 0; i < shapes.getLength(); i++) {
    SoType type = shapes[i];
    if (type.canCreateInstance() && (shapehandledlist.find(type) < 0)) {
      PRIVATE(this)->cbaction.addPreCallback(type, SoToVRML2ActionP::sotoifs_cb, &PRIVATE(this).get());
      PRIVATE(this)->cbaction.addTriangleCallback(type, SoToVRML2ActionP::triangle_cb, &PRIVATE(this).get());
      PRIVATE(this)->cbaction.addPostCallback(type, SoToVRML2ActionP::post_primitives_cb, &PRIVATE(this).get());
    }
  }

#undef ADD_PRE_CB
#undef ADD_POST_CB
#undef ADD_UNSUPPORTED
#undef ADD_TRIANGLE_CB
#undef ADD_LINE_CB
#undef ADD_SHAPE_CB
#undef ADD_SO_TO_IFS
#undef ADD_SO_TO_ILS
}

SoToVRML2Action::~SoToVRML2Action(void)
{
}

// Documented in superclass.
void
SoToVRML2Action::apply(SoNode * root)
{
  PRIVATE(this)->init();
  PRIVATE(this)->cbaction.apply(root);
}

// Documented in superclass.
void
SoToVRML2Action::apply(SoPath * path)
{
  PRIVATE(this)->init();
  PRIVATE(this)->cbaction.apply(path);
}

// Documented in superclass.
void
SoToVRML2Action::apply(const SoPathList & pathlist, SbBool obeysrules)
{
  PRIVATE(this)->init();
  PRIVATE(this)->cbaction.apply(pathlist, obeysrules);
}

// Documented in superclass.
void
SoToVRML2Action::beginTraversal(SoNode * COIN_UNUSED_ARG(node))
{
  assert(0 && "should never get here");
}

SoVRMLGroup *
SoToVRML2Action::getVRML2SceneGraph(void) const
{
  return PRIVATE(this)->vrml2root;
}

void
SoToVRML2Action::reuseAppearanceNodes(SbBool COIN_UNUSED_ARG(appearance))
{
  // FIXME: not implemented yet. 20020808 mortene.
  COIN_STUB();
}

SbBool
SoToVRML2Action::doReuseAppearanceNodes(void) const
{
  // FIXME: not implemented yet. 20020808 mortene.
  COIN_STUB();
  return FALSE;
}

void
SoToVRML2Action::reusePropertyNodes(SbBool property)
{
  PRIVATE(this)->reusePropertyNodes = property;
}

SbBool
SoToVRML2Action::doReusePropertyNodes(void) const
{
  return PRIVATE(this)->reusePropertyNodes;
}

void
SoToVRML2Action::reuseGeometryNodes(SbBool COIN_UNUSED_ARG(geometry))
{
  // FIXME: not implemented yet. 20020808 mortene.
  COIN_STUB();
}

SbBool
SoToVRML2Action::doReuseGeometryNodes(void) const
{
  // FIXME: not implemented yet. 20020808 mortene.
  return FALSE;
}


SoNode *
SoToVRML2ActionP::search_for_recent_node(SoAction * action, const SoType & type)
{
  this->searchaction.setSearchingAll(FALSE);
  this->searchaction.setType(type);
  this->searchaction.setInterest(SoSearchAction::LAST);

#ifdef HAVE_NODEKITS
  SbBool old = SoBaseKit::isSearchingChildren();
  SoBaseKit::setSearchingChildren(TRUE);
#endif // HAVE_NODEKITS

  this->searchaction.apply(const_cast<SoPath *>(action->getCurPath()));

  SoNode * tail = NULL;
  SoFullPath * path = reclassify_cast<SoFullPath *>(this->searchaction.getPath());
  if (path) {
    tail = path->getTail();
  }
  this->searchaction.reset();
#ifdef HAVE_NODEKITS
  SoBaseKit::setSearchingChildren(old);
#endif // HAVE_NODEKITS

  return tail;
}

SoGroup *
SoToVRML2ActionP::get_current_tail(void)
{
  SoNode * node = this->vrml2path->getTail();
  assert(node->isOfType(SoVRMLGroup::getClassTypeId()) ||
         node->isOfType(SoVRMLSwitch::getClassTypeId()) ||
         node->isOfType(SoVRMLLOD::getClassTypeId()));
  return coin_assert_cast<SoGroup *>(node);
}

SoVRMLCoordinate *
SoToVRML2ActionP::get_or_create_coordinate(const SbVec4f * coord4, int32_t num)
{
  SbList <SbVec3f> vec3f;
  for (int i = 0; i < num; i++) {
    SbVec3f tmp;
    coord4[i].getReal(tmp);
    vec3f.append(tmp);
  }
  return this->get_or_create_coordinate(vec3f.getArrayPtr(), num);
}

SoVRMLCoordinate *
SoToVRML2ActionP::get_or_create_coordinate(const SbVec3f * coord3, int32_t num)
{
  if (this->reusePropertyNodes) {
    // Search for a matching VRMLCoordinate
    int n = this->vrmlcoords->getLength();
    while (--n >= 0) {
      SoVRMLCoordinate * c = (*this->vrmlcoords)[n];
      if (c->point.getNum() == num &&
          memcmp(coord3, c->point.getValues(0), num*sizeof(SbVec3f)) == 0) {
        return c;
      }
    }
  }

  // Create new
  SoVRMLCoordinate * c = new SoVRMLCoordinate;
  c->point.setValues(0, num, coord3);
  if (this->reusePropertyNodes) this->vrmlcoords->append(c);
  return c;
}

SoVRMLNormal *
SoToVRML2ActionP::get_or_create_normal(const SbVec3f * normal, int32_t num)
{
  if (this->reusePropertyNodes) {
    // Search for a matching VRMLNormal
    int n = this->vrmlnormals->getLength();
    while (--n >= 0) {
      SoVRMLNormal * nor = (*this->vrmlnormals)[n];
      if (nor->vector.getNum() == num &&
          memcmp(normal, nor->vector.getValues(0), num*sizeof(SbVec3f)) == 0) {
        return nor;
      }
    }
  }

  // Create new
  SoVRMLNormal * nor = new SoVRMLNormal;
  nor->vector.setValues(0, num, normal);
  if (this->reusePropertyNodes) this->vrmlnormals->append(nor);
  return nor;
}

SoVRMLColor *
SoToVRML2ActionP::get_or_create_color(const uint32_t * packedColor, int32_t num)
{
  // Convert to SbColors
  SbList <SbColor> color;
  float f;
  for (int i = 0; i < num; i++) {
    SbColor tmp;
    tmp.setPackedValue(packedColor[i], f);
    color.append(tmp);
  }
  return this->get_or_create_color(color.getArrayPtr(), num);
}

SoVRMLColor *
SoToVRML2ActionP::get_or_create_color(const SbColor * color, int32_t num)
{
  if (this->reusePropertyNodes) {
    // Search for a matching VRMLColor
    int n = this->vrmlcolors->getLength();
    while (--n >= 0) {
      SoVRMLColor * c = (*this->vrmlcolors)[n];
      if (c->color.getNum() == num &&
          memcmp(color, c->color.getValues(0), num*sizeof(SbColor)) == 0) {
        return c;
      }
    }
  }

  // Create new
  SoVRMLColor * c = new SoVRMLColor;
  c->color.setValues(0, num, color);
  if (this->reusePropertyNodes) this->vrmlcolors->append(c);
  return c;
}

SoVRMLTextureCoordinate *
SoToVRML2ActionP::get_or_create_texcoordinate(const SbVec2f * texcoord2, int32_t num)
{
  if (this->reusePropertyNodes) {
    // Search for a matching VRMLTextureCoordinate
    int n = this->vrmltexcoords->getLength();
    while (--n >= 0) {
      SoVRMLTextureCoordinate * tc = (*this->vrmltexcoords)[n];
      if (tc->point.getNum() == num &&
          memcmp(texcoord2, tc->point.getValues(0), num*sizeof(SbVec2f)) == 0) {
        return tc;
      }
    }
  }

  // Create new
  SoVRMLTextureCoordinate * tc = new SoVRMLTextureCoordinate;
  tc->point.setValues(0, num, texcoord2);
  if (this->reusePropertyNodes) this->vrmltexcoords->append(tc);
  return tc;
}

void
SoToVRML2ActionP::insert_shape(SoCallbackAction * action, SoVRMLGeometry * geom)
{
  SoVRMLShape * shape = new SoVRMLShape;
  shape->geometry = geom;

  // Create appearance
  SoVRMLAppearance * appearance = new SoVRMLAppearance;
  shape->appearance = appearance;

  SoVRMLMaterial * mat = new SoVRMLMaterial;
  appearance->material = mat;

  // Get values from current state
  SbColor ambient, diffuse, specular, emissions;
  float shin, transp;
  action->getMaterial(ambient, diffuse, specular, emissions, shin, transp);

  if (!geom->isOfType(SoVRMLPointSet::getClassTypeId())) {
    if (mat->diffuseColor.getValue() != diffuse) mat->diffuseColor = diffuse;

    // Convert to grayscale for calculating the ambient intensity
    float ambientGray = ambient[0] * 77 + ambient[1] * 150 + ambient[2] * 29;
    if (ambientGray >  0) {
      float ambientIntensity = SbMin(1.0f, ambientGray / 256.0f);
      if (mat->ambientIntensity.getValue() != ambientIntensity)
        mat->ambientIntensity = ambientIntensity;
    }

    if (mat->specularColor.getValue() != specular) mat->specularColor = specular;
    if (mat->emissiveColor.getValue() != emissions) mat->emissiveColor = emissions;
    if (mat->shininess.getValue() != shin) mat->shininess = shin;
    if (mat->transparency.getValue() != transp) mat->transparency = transp;

    // Texture
    if (this->recentTex2 == NULL) {
      this->recentTex2 = coin_safe_cast<SoTexture2 *>(search_for_recent_node(action, SoTexture2::getClassTypeId()));
    }

    if (this->recentTex2 != NULL) {
      SbVec2s size;
      int numComponents;
      const unsigned char * image = this->recentTex2->image.getValue(size, numComponents);
      if (!this->recentTex2->filename.isDefault() || (size[0] > 0 && size[1] > 0)) {
        SoVRMLTexture * tex;
        if (!this->recentTex2->filename.isDefault()) {
          tex = new SoVRMLImageTexture;
          SbString url = this->master->getUrlName();
          url += this->recentTex2->filename.getValue();
          coin_assert_cast<SoVRMLImageTexture *>(tex)->url.setValue(url);
        } else {
          tex = new SoVRMLPixelTexture;
          coin_assert_cast<SoVRMLPixelTexture *>(tex)->image.setValue(size, numComponents, image);
        }
        tex->repeatS = this->recentTex2->wrapS.getValue() == SoTexture2::REPEAT;
        tex->repeatT = this->recentTex2->wrapT.getValue() == SoTexture2::REPEAT;
        appearance->texture = tex;

        // Texture transform
        const SbMatrix * matrix = &action->getTextureMatrix();

        if (!matrix->equals(SbMatrix::identity(), 0.0f)) {
          SbVec3f translation, scaleFactor;
          SbRotation rotation, scaleOrientation;
          matrix->getTransform(translation, rotation, scaleFactor, scaleOrientation);

          SoVRMLTextureTransform * textrans = new SoVRMLTextureTransform;
          textrans->translation = SbVec2f(translation[0], translation[1]);

          SbVec3f axis;
          float radians;
          rotation.getValue(axis, radians);
          if (axis[2] < 0) radians = static_cast<float>(2*M_PI) - radians;
          textrans->rotation = radians;

          textrans->scale = SbVec2f(scaleFactor[0], scaleFactor[1]);

          appearance->textureTransform = textrans;
        }
      }
      this->recentTex2 = NULL;
    }
  } else {
    if (mat->emissiveColor.getValue() != diffuse) mat->emissiveColor = diffuse;
  }

  get_current_tail()->addChild(shape);
}

SoCallbackAction::Response
SoToVRML2ActionP::push_sep_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  SoToVRML2ActionP * thisp = THISP(closure);
  SoGroup * prevgroup = thisp->get_current_tail();

  SoGroup * vp;
  if (THISP(closure)->dict.get(node, vp)) {
    // Re-use previous subgraph
    prevgroup->addChild(vp);
    return SoCallbackAction::PRUNE;
  }

  // Push a new SoVRMLGroup on the tail of the path
  SoVRMLGroup * newgroup = NULL;

  if (node->isOfType(SoVRMLTransform::getClassTypeId())) {
    const SoVRMLTransform * oldtrans = coin_assert_cast<const SoVRMLTransform*>(node);
    SoVRMLTransform * newtrans = NEW_NODE(SoVRMLTransform, node);

    newgroup = newtrans;

    newtrans->translation = oldtrans->translation;
    newtrans->rotation = oldtrans->rotation;
    newtrans->scale = oldtrans->scale;
    newtrans->scaleOrientation = oldtrans->scaleOrientation;
    newtrans->center = oldtrans->center;

  }
  else {
    newgroup = NEW_NODE(SoVRMLGroup, node);
  }

  // Push a new SoVRMLGroup on the tail of the path
  prevgroup->addChild(newgroup);
  thisp->vrml2path->append(newgroup);
  thisp->separatorstack.append(newgroup);

  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoToVRML2ActionP::pop_sep_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  SoGroup * vp;
  if (THISP(closure)->dict.get(node, vp)) {
    return SoCallbackAction::CONTINUE;
  }

  SoGroup * pushedgroup = THISP(closure)->separatorstack.pop();
  // Pop node from the tail of the path until an SoVRMLGroup has been popped
  SoGroup * grp;

  do {
    grp = THISP(closure)->get_current_tail();
    THISP(closure)->vrml2path->pop();
  } while (grp != pushedgroup);

  THISP(closure)->dict.put(node, grp);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoToVRML2ActionP::push_transformsep_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  SbString str = "SoTransformSeparator nodes do not have a VRML counterpart, and may not function correctly";
  SoDebugError::postWarning("SoToVRML2Action::push_transformsep_cb", "%s", str.getString());
  return push_sep_cb(closure, action, node);  
}

SoCallbackAction::Response
SoToVRML2ActionP::pop_transformsep_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  return pop_sep_cb(closure, action, node);
}

SoCallbackAction::Response
SoToVRML2ActionP::push_switch_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  SoToVRML2ActionP * thisp = THISP(closure);
  SoGroup * prevgroup = thisp->get_current_tail();

  SoGroup * vp;
  if (THISP(closure)->dict.get(node, vp)) {
    // Re-use previous subgraph
    prevgroup->addChild(vp);
    return SoCallbackAction::PRUNE;
  }

  const SoSwitch * oldswitch = coin_assert_cast<const SoSwitch *>(node);
  SoVRMLSwitch * newswitch = NEW_NODE(SoVRMLSwitch, node);

  // SO_SWITCH_INHERIT is not supported in VRML97, so just translate
  // it here. We could perhaps consider creating a ROUTE from the
  // inherited whichChoice field to this field...
  int wc = oldswitch->whichChild.getValue() == SO_SWITCH_INHERIT ?
    action->getSwitch() : oldswitch->whichChild.getValue();

  newswitch->whichChoice = wc;
  prevgroup->addChild(newswitch);
  thisp->vrml2path->append(newswitch);

  /* Traverse all children separately, that is, save and restore state
   * between each.  If there is a selected child, traverse it normally
   * This is needed so that traversing the not selected
   * children won't influence the selected child.
   */
  if (wc != SO_SWITCH_ALL) {
    SoState * state = action->getState();
    // update SwitchElement before traversing children (this is
    // usually done in SoSwitch::doAction) (don't push before setting
    // this element as it's supposed to be set when traversing the
    // next sibling).
    action->pushCurPath();

    SoSwitchElement::set(state, wc);
    int n = oldswitch->getNumChildren();
    for (int i = 0; i < n; i++) {
      SoNode * child = oldswitch->getChild(i);
      if (i != wc) {
        state->push();
        action->popPushCurPath(i, child);
        action->traverse(child);
        state->pop();
      }
      else {
        action->popPushCurPath(i, child);
        action->traverse(child);
      }
    }
    action->popCurPath();
    // so that the children will not be traversed
    return SoCallbackAction::PRUNE;
  }
  // traverse Switch node as a normal group node
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoToVRML2ActionP::pop_switch_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  SoGroup * vp;
  if (THISP(closure)->dict.get(node, vp)) {
    return SoCallbackAction::CONTINUE;
  }

  SoGroup * grp;
  do {
    grp = THISP(closure)->get_current_tail();
    THISP(closure)->vrml2path->pop();
  } while (grp->getTypeId() != SoVRMLSwitch::getClassTypeId());

  SoVRMLSwitch * sw = coin_assert_cast<SoVRMLSwitch *>(grp);
  int wc = sw->whichChoice.getValue();

  if (wc == SO_SWITCH_ALL) {
    // workaround since VRML97 does not support SO_SWITCH_ALL.
    SoVRMLGroup * allfix = new SoVRMLGroup;
    allfix->ref();
    for (int i = 0; i < sw->getNumChoices(); i++) {
      allfix->addChild(sw->getChoice(i));
    }
    sw->removeAllChoices();
    sw->addChoice(allfix);
    allfix->unrefNoDelete();
    // set whichChoice to point to the new group node
    sw->whichChoice = 0;
  }

  THISP(closure)->dict.put(node, grp);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoToVRML2ActionP::push_levelofdetail_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  SoToVRML2ActionP * thisp = THISP(closure);
  SoGroup * prevgroup = thisp->get_current_tail();

  SoGroup * vp;
  if (THISP(closure)->dict.get(node, vp)) {
    // Re-use previous subgraph
    prevgroup->addChild(vp);
    return SoCallbackAction::PRUNE;
  }

  const SoLevelOfDetail * oldlod = coin_assert_cast<const SoLevelOfDetail *>(node);
  SoVRMLLOD * newlod = NEW_NODE(SoVRMLLOD, node);

  // calculate bbox of children to find a reasonable conversion to range
  SoGetBoundingBoxAction * bboxAction = thisp->getBBoxAction();
  SbViewportRegion viewport(DEFAULT_VIEWPORT_WIDTH, DEFAULT_VIEWPORT_HEIGHT);
  bboxAction->setViewportRegion(viewport);
  // need to apply on the current path, not on the node, since we
  // might need coordinates from the state. Also, we need to set the
  // reset path so that we get the local bounding box for the nodes
  // below this node.
  bboxAction->setResetPath(action->getCurPath());
  bboxAction->apply(const_cast<SoPath*>(action->getCurPath())); // find bbox of all children
  SbBox3f bbox = bboxAction->getBoundingBox();

  float dx, dy, dz;
  bbox.getSize(dx,dy,dz);
  const float h = SbMax(SbMax(dx,dy), dz);

  // create a typical view volume
  SbViewVolume vv;
  vv.perspective(float(M_PI)/4.0f, float(DEFAULT_VIEWPORT_WIDTH)/float(DEFAULT_VIEWPORT_HEIGHT),
                 1.0f, 10.0f);


  newlod->range.setNum(oldlod->screenArea.getNum());
  float * rangeptr = newlod->range.startEditing();
  int i;
  for (i = 0; i < oldlod->screenArea.getNum(); i++) {
    rangeptr[i] = thisp->getBBoxDistance(vv, oldlod->screenArea[i], h);
  }
  newlod->range.finishEditing();

  prevgroup->addChild(newlod);
  thisp->vrml2path->append(newlod);

  // Traverse all children separately, with normal SoGroup traversal
  int n = oldlod->getNumChildren();
  action->pushCurPath();
  for (i=0; i < n; i++) {
    SoNode * child = oldlod->getChild(i);
    action->popPushCurPath(i, child);
    action->traverse(child);
  }
  action->popCurPath();

  thisp->vrml2path->pop();
  THISP(closure)->dict.put(node, newlod);
  return SoCallbackAction::PRUNE;
}

SoCallbackAction::Response
SoToVRML2ActionP::push_lod_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  SoToVRML2ActionP * thisp = THISP(closure);
  SoGroup * prevgroup = thisp->get_current_tail();

  SoGroup * vp;
  if (THISP(closure)->dict.get(node, vp)) {
    // Re-use previous subgraph
    prevgroup->addChild(vp);
    return SoCallbackAction::PRUNE;
  }

  const SoLOD * oldlod = coin_assert_cast<const SoLOD *>(node);
  SoVRMLLOD * newlod = NEW_NODE(SoVRMLLOD, node);

  newlod->range.setValues(0, oldlod->range.getNum(), oldlod->range.getValues(0));
  newlod->center = oldlod->center.getValue();

  prevgroup->addChild(newlod);
  thisp->vrml2path->append(newlod);

  // Traverse all children separately, with a normal SoGroup traversal
  int n = oldlod->getNumChildren();

  action->pushCurPath();
  for (int i=0; i < n; i++) {
    SoNode * child = oldlod->getChild(i);
    action->popPushCurPath(i, child);
    action->traverse(child);
  }
  action->popCurPath();

  thisp->vrml2path->pop();
  THISP(closure)->dict.put(node, newlod);
  return SoCallbackAction::PRUNE;
}

SoCallbackAction::Response
SoToVRML2ActionP::unsupported_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  SoVRMLWorldInfo * info = NEW_NODE(SoVRMLWorldInfo, node);
  SbString str;
  str.sprintf("Unsupported node: %s",
              node->getTypeId().getName().getString());
  info->title = str;
  THISP(closure)->get_current_tail()->addChild(info);

  if (THISP(closure)->master->isVerbose()) {
      SoDebugError::postWarning("SoToVRML2Action::unsupported_cb", "%s", str.getString());
  }

  return SoCallbackAction::CONTINUE;
}

// Shape nodes
SoCallbackAction::Response
SoToVRML2ActionP::soasciitext_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  if (action->getDrawStyle() != SoDrawStyle::FILLED) {
      return SoToVRML2ActionP::sotoifs_cb(closure, action, node);
  }
  SoVRMLText * text = NEW_NODE(SoVRMLText, node);
  const SoAsciiText * oldtext = coin_assert_cast<const SoAsciiText *>(node);

  text->string = oldtext->string;
  text->length = oldtext->width;

  SoVRMLFontStyle *style = new SoVRMLFontStyle;
  style->size.setValue(action->getFontSize());
  text->fontStyle.setValue(style);
  // FIXME: Better FontStyle handling (20030414 kintel)

  THISP(closure)->insert_shape(action, text);
  return SoCallbackAction::PRUNE;
}

SoCallbackAction::Response
SoToVRML2ActionP::socube_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  if (action->getDrawStyle() != SoDrawStyle::FILLED) {
      return SoToVRML2ActionP::sotoifs_cb(closure, action, node);
  }
  SoVRMLBox * box = NEW_NODE(SoVRMLBox, node);
  const SoCube * cube = coin_assert_cast<const SoCube *>(node);
  if (box->size.getValue()[0] != cube->width.getValue() ||
      box->size.getValue()[1] != cube->height.getValue() ||
      box->size.getValue()[2] != cube->depth.getValue()) {
    box->size.setValue(cube->width.getValue(), cube->height.getValue(), cube->depth.getValue());
  }
  THISP(closure)->insert_shape(action, box);
  return SoCallbackAction::PRUNE;
}

SoCallbackAction::Response
SoToVRML2ActionP::socone_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  if (action->getDrawStyle() != SoDrawStyle::FILLED) {
      return SoToVRML2ActionP::sotoifs_cb(closure, action, node);
  }
  SoVRMLCone * cone = NEW_NODE(SoVRMLCone, node);
  const SoCone * oldcone = coin_assert_cast<const SoCone *>(node);

  if (oldcone->bottomRadius != cone->bottomRadius)
    cone->bottomRadius = oldcone->bottomRadius.getValue();
  if (oldcone->height != cone->height)
    cone->height = oldcone->height.getValue();
  SbBool bottom = (oldcone->parts.getValue() & SoCone::BOTTOM) ? TRUE : FALSE;
  if (bottom != cone->bottom.getValue()) cone->bottom = bottom;
  SbBool side = (oldcone->parts.getValue() & SoCone::SIDES) ? TRUE : FALSE;
  if (side != cone->side.getValue()) cone->side = side;

  THISP(closure)->insert_shape(action, cone);
  return SoCallbackAction::PRUNE;
}

SoCallbackAction::Response
SoToVRML2ActionP::socylinder_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  if (action->getDrawStyle() != SoDrawStyle::FILLED) {
      return SoToVRML2ActionP::sotoifs_cb(closure, action, node);
  }
  SoVRMLCylinder * cyl = NEW_NODE(SoVRMLCylinder, node);
  const SoCylinder * oldcyl = coin_assert_cast<const SoCylinder *>(node);

  if (oldcyl->radius != cyl->radius)
    cyl->radius = oldcyl->radius.getValue();
  if (oldcyl->height != cyl->height)
    cyl->height = oldcyl->height.getValue();

  SbBool side = (oldcyl->parts.getValue() & SoCylinder::SIDES) ? TRUE : FALSE;
  if (side != cyl->side.getValue()) cyl->side = side;
  SbBool top = (oldcyl->parts.getValue() & SoCylinder::TOP) ? TRUE : FALSE;
  if (top != cyl->top.getValue()) cyl->top = top;
  SbBool bottom = (oldcyl->parts.getValue() & SoCylinder::BOTTOM) ? TRUE : FALSE;
  if (bottom != cyl->bottom.getValue()) cyl->bottom = bottom;

  THISP(closure)->insert_shape(action, cyl);
  return SoCallbackAction::PRUNE;
}

SoCallbackAction::Response
SoToVRML2ActionP::soifs_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  if (action->getDrawStyle() != SoDrawStyle::FILLED) {
      return SoToVRML2ActionP::sotoifs_cb(closure, action, node);
  }
  const SoIndexedFaceSet * oldifs = coin_assert_cast<const SoIndexedFaceSet*>(node);

  if (oldifs->coordIndex.getNum() == 0 ||
      oldifs->coordIndex[0] < 0)
    return SoCallbackAction::CONTINUE;

  SoToVRML2ActionP * thisp = THISP(closure);
  SoVRMLIndexedFaceSet * ifs = NEW_NODE(SoVRMLIndexedFaceSet, node);

  // Set the values from the current ShapeHints
  ifs->creaseAngle = action->getCreaseAngle();
  ifs->ccw = action->getVertexOrdering() != SoShapeHints::CLOCKWISE;
  ifs->solid = SoShapeHintsElement::getShapeType(action->getState()) == SoShapeHintsElement::SOLID;
  ifs->convex = action->getFaceType() == SoShapeHints::CONVEX;

  // If there is a VertexProperty node set we need to put it on the state stack
  SoNode *vpnode = oldifs->vertexProperty.getValue();
  SoVertexProperty * vp = coin_safe_cast<SoVertexProperty *>(vpnode);
  if (vp) {
    action->getState()->push();
    vp->callback(action);
  }

  const SoCoordinateElement * coordElem = SoCoordinateElement::getInstance(action->getState());
  if (coordElem->getNum() > 0) {
    if (coordElem->getArrayPtr3() != NULL) {
      ifs->coord = thisp->get_or_create_coordinate(coordElem->getArrayPtr3(),
                                                   coordElem->getNum());
    } else {
      ifs->coord = thisp->get_or_create_coordinate(coordElem->getArrayPtr4(),
                                                   coordElem->getNum());
    }
  }

  if (action->getNormalBinding() != SoNormalBinding::OVERALL) {
    const SoNormalElement * normalElem = SoNormalElement::getInstance(action->getState());
    if (coordElem->getNum() > 0) {
      ifs->normal = thisp->get_or_create_normal(normalElem->getArrayPtr(),
                                                normalElem->getNum());
      if (action->getNormalBinding() != SoNormalBinding::PER_VERTEX_INDEXED &&
          action->getNormalBinding() != SoNormalBinding::PER_VERTEX) {
        ifs->normalPerVertex = FALSE;
      }
    }
  }

  if (action->getMaterialBinding() != SoMaterialBinding::OVERALL) {
    SoLazyElement * lazy = SoLazyElement::getInstance(action->getState());
    if (lazy->getNumDiffuse() > 1) {
      if (lazy->isPacked()) {
        ifs->color = thisp->get_or_create_color(lazy->getPackedPointer(),
                                                lazy->getNumDiffuse());
      }
      else {
        ifs->color = thisp->get_or_create_color(lazy->getDiffusePointer(),
                                                lazy->getNumDiffuse());
      }
      if (action->getMaterialBinding() != SoMaterialBinding::PER_VERTEX_INDEXED &&
          action->getMaterialBinding() != SoMaterialBinding::PER_VERTEX) {
        ifs->colorPerVertex = FALSE;
      }
    }
  }

  const SoMultiTextureCoordinateElement * texcoordElem =
    SoMultiTextureCoordinateElement::getInstance(action->getState());
  if (texcoordElem->getNum(0) > 0) {
    ifs->texCoord = thisp->get_or_create_texcoordinate(texcoordElem->getArrayPtr2(),
                                                       texcoordElem->getNum());
  }

  ifs->coordIndex.setValues(0, oldifs->coordIndex.getNum(),
                            oldifs->coordIndex.getValues(0));
  if (!oldifs->textureCoordIndex.isDefault() &&
      oldifs->textureCoordIndex.getNum()) {
    ifs->texCoordIndex.setValues(0, oldifs->textureCoordIndex.getNum(),
                                 oldifs->textureCoordIndex.getValues(0));
  }
  if (!oldifs->materialIndex.isDefault() && oldifs->materialIndex.getNum()) {
    ifs->colorIndex.setValues(0, oldifs->materialIndex.getNum(),
                              oldifs->materialIndex.getValues(0));
  }
  if (!oldifs->normalIndex.isDefault() && oldifs->normalIndex.getNum()) {
    ifs->normalIndex.setValues(0, oldifs->normalIndex.getNum(),
                               oldifs->normalIndex.getValues(0));
  }

  THISP(closure)->insert_shape(action, ifs);

  // it's important to pop state _after_ inserting the shape to get
  // the correct material from SoVertexProperty nodes.
  if (vp) {
    action->getState()->pop();
  }

  return SoCallbackAction::PRUNE;
}

SoCallbackAction::Response
SoToVRML2ActionP::soils_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  // FIXME: test for drawstyle == POINTS and convert to a point set
  // instead.  pederb, 20060327
  SoToVRML2ActionP * thisp = THISP(closure);

  const SoIndexedLineSet * oldils = coin_assert_cast<const SoIndexedLineSet *>(node);

  if (oldils->coordIndex.getNum() == 0 ||
      oldils->coordIndex[0] < 0)
    return SoCallbackAction::CONTINUE;

  SoVRMLIndexedLineSet * ils = NEW_NODE(SoVRMLIndexedLineSet, node);

  // If there is a VertexProperty node set we need to put it on the state stack
  SoNode *vpnode = oldils->vertexProperty.getValue();
  SoVertexProperty *vp = coin_safe_cast<SoVertexProperty *>(vpnode);
  if (vp) {
    action->getState()->push();
    vp->callback(action);
  }

  SoVRMLCoordinate * newcoord = NULL;
  const SoCoordinateElement * coordElem = SoCoordinateElement::getInstance(action->getState());
  if (coordElem->getNum() > 0) {
    if (thisp->nodefuse) {
      newcoord = new SoVRMLCoordinate;
    }
    else {
      if (coordElem->getArrayPtr3() != NULL) {
        newcoord = thisp->get_or_create_coordinate(coordElem->getArrayPtr3(),
                                                   coordElem->getNum());
      } else {
        newcoord = thisp->get_or_create_coordinate(coordElem->getArrayPtr4(),
                                                   coordElem->getNum());
      }
    }
    ils->coord = newcoord;
  }

  if (action->getMaterialBinding() != SoMaterialBinding::OVERALL) {
    const SoLazyElement * colorElem =
      SoLazyElement::getInstance(action->getState());
    if (colorElem->getNumDiffuse() > 0) {
      if (colorElem->isPacked()) {
        ils->color = thisp->get_or_create_color(colorElem->getPackedPointer(),
                                                colorElem->getNumDiffuse());
      } else {
        ils->color = thisp->get_or_create_color(colorElem->getDiffusePointer(),
                                                colorElem->getNumDiffuse());
      }
      if (action->getMaterialBinding() != SoMaterialBinding::PER_VERTEX_INDEXED &&
          action->getMaterialBinding() != SoMaterialBinding::PER_VERTEX) {
        ils->colorPerVertex = FALSE;
      }
    }
  }

  if (thisp->nodefuse && coordElem->getNum() > 0) {
    SbBSPTree bsp;
    int n = oldils->coordIndex.getNum();
    const int32_t * src = oldils->coordIndex.getValues(0);

    SbVec3f * c = const_cast<SbVec3f*>(coordElem->getArrayPtr3());
    if (c == NULL) {
      SbVec3f * vec3f = new SbVec3f[coordElem->getNum()];
      const SbVec4f * coord4 = coordElem->getArrayPtr4();
      for (int i=coordElem->getNum()-1; i >= 0; i--) {
        coord4[i].getReal(vec3f[i]);
      }
      c = vec3f;
    }

    ils->coordIndex.setNum(n);
    int32_t * dst = ils->coordIndex.startEditing();

    for (int i = 0; i < n; i++) {
      int32_t idx = src[i];
      if (idx >= 0) {
        dst[i] = bsp.addPoint(c[idx]);
      }
      else dst[i] = -1;
    }
    ils->coordIndex.finishEditing();
    newcoord->point.setValues(0, bsp.numPoints(),
                              bsp.getPointsArrayPtr());

    if (coordElem->getArrayPtr3() == NULL) delete[] c;

  }
  else {
    ils->coordIndex.setValues(0, oldils->coordIndex.getNum(),
                              oldils->coordIndex.getValues(0));
  }

  if (action->getMaterialBinding() == SoMaterialBinding::PER_VERTEX_INDEXED ||
      action->getMaterialBinding() == SoMaterialBinding::PER_FACE_INDEXED) {
    ils->colorIndex.setValues(0, oldils->materialIndex.getNum(),
                              oldils->materialIndex.getValues(0));
  }
  else if (action->getMaterialBinding() == SoMaterialBinding::PER_PART_INDEXED ||
           action->getMaterialBinding() == SoMaterialBinding::PER_PART) {

    // Color per segment, convert to per vertex
    SbList <int32_t> coordIdx;
    SbBSPTree bsp;
    int32_t colidx = 0;
    SoVRMLColor * color = coin_assert_cast<SoVRMLColor *>(ils->color.getValue());
    int n = ils->coordIndex.getNum()-1;
    for (int i = 0; i < n; i++) {
      SbVec3f curcol, nextcol(0.0f, 0.0f, 0.0f);
      if (action->getMaterialBinding() == SoMaterialBinding::PER_PART_INDEXED) {
        curcol = color->color[oldils->materialIndex[colidx]];
        if (i != n-1)
          nextcol = color->color[oldils->materialIndex[colidx+1]];
      }
      else {
        curcol = color->color[colidx];
        if (i != n-1)
          nextcol = color->color[colidx+1];
      }
      colidx++;

      coordIdx.append(bsp.addPoint(curcol));

      if (i == n-1 || ils->coordIndex[i+2] == -1) {
        // Current polyline is done
        coordIdx.append(coordIdx[coordIdx.getLength()-1]);
        coordIdx.append(-1);
        i += 2;
      }
      else if (curcol != nextcol) {
        // Create a new vertex to avoid color interpolation
        ils->coordIndex.insertSpace(i+1, 1);
        ils->coordIndex.set1Value(i+1, ils->coordIndex[i+2]);
        coordIdx.append(bsp.addPoint(curcol));
        i++; n++;
      }
    }

    ils->color = thisp->get_or_create_color(static_cast<const SbColor *>(bsp.getPointsArrayPtr()),
                                            bsp.numPoints());

    ils->colorIndex.setValues(0, coordIdx.getLength(),
                              coordIdx.getArrayPtr());

    ils->colorPerVertex = TRUE;
  }

  THISP(closure)->insert_shape(action, ils);

  // it's important to pop state _after_ inserting the shape to get
  // the correct material from SoVertexProperty nodes.
  if (vp) {
    action->getState()->pop();
  }

  return SoCallbackAction::PRUNE;
}

SoCallbackAction::Response
SoToVRML2ActionP::solineset_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  // FIXME: test for drawstyle == POINTS and convert to a point set
  // instead.  pederb, 20060327
  SoToVRML2ActionP * thisp = THISP(closure);

  const SoLineSet * oldls = coin_assert_cast<const SoLineSet *>(node);

  if (oldls->numVertices.getNum() == 0)
    return SoCallbackAction::CONTINUE;

  SoVRMLIndexedLineSet * ils = NEW_NODE(SoVRMLIndexedLineSet, node);

  // If there is a VertexProperty node set we need to put it on the state stack
  SoNode *vpnode = oldls->vertexProperty.getValue();
  SoVertexProperty *vp = coin_safe_cast<SoVertexProperty *>(vpnode);
  if (vp) {
    action->getState()->push();
    vp->callback(action);
  }

  const SoCoordinateElement * coordElem = SoCoordinateElement::getInstance(action->getState());
  if (coordElem->getNum() > 0) {
    if (coordElem->getArrayPtr3() != NULL) {
      ils->coord = thisp->get_or_create_coordinate(coordElem->getArrayPtr3(),
                                                   coordElem->getNum());
    } else {
      ils->coord = thisp->get_or_create_coordinate(coordElem->getArrayPtr4(),
                                                   coordElem->getNum());
    }
  }

  if (action->getMaterialBinding() != SoMaterialBinding::OVERALL) {
    const SoLazyElement * colorElem =
      SoLazyElement::getInstance(action->getState());
    if (colorElem->getNumDiffuse() > 0) {
      if (colorElem->isPacked()) {
        ils->color = thisp->get_or_create_color(colorElem->getPackedPointer(),
                                                colorElem->getNumDiffuse());
      } else {
        ils->color = thisp->get_or_create_color(colorElem->getDiffusePointer(),
                                                colorElem->getNumDiffuse());
      }
      if (action->getMaterialBinding() != SoMaterialBinding::PER_VERTEX) {
        ils->colorPerVertex = FALSE;
      }
    }
  }

  SbList <int32_t> l;
  int n = oldls->numVertices.getNum();
  int32_t curidx = 0;

  // check for special case where lineset should render all vertices
  // on the state
  if ((n == 1) && (oldls->numVertices[0] == -1)) {
    const int numv = coordElem->getNum();
    for (int i = 0; i < numv; i++) {
      l.append(curidx++);
    }
    l.append(-1);
  }
  else {
    for (int i = 0; i < n; i++) {
      for (int j = oldls->numVertices[i]-1; j >= 0; j--) {
        l.append(curidx++);
      }
      l.append(-1);
    }
  }
  ils->coordIndex.setValues(0, l.getLength(),
                            l.getArrayPtr());

  if (action->getMaterialBinding() == SoMaterialBinding::PER_PART) {
    // Color per segment, convert to per vertex
    SbList <int32_t> coordIdx;
    SbBSPTree bsp;
    int32_t colidx = 0;
    SoVRMLColor * color = coin_assert_cast<SoVRMLColor *>(ils->color.getValue());
    int n = ils->coordIndex.getNum()-1;
    for (int i = 0; i < n; i++) {
      SbVec3f curcol, nextcol(0.0f, 0.0f, 0.0f);

      curcol = color->color[colidx];
      if (i != n-1)
        nextcol = color->color[colidx+1];
      colidx++;

      coordIdx.append(bsp.addPoint(curcol));

      if (i == n-1 || ils->coordIndex[i+2] == -1) {
        // Current polyline is done
        coordIdx.append(coordIdx[coordIdx.getLength()-1]);
        coordIdx.append(-1);
        i += 2;
      }
      else if (curcol != nextcol) {
        // Create a new vertex to avoid color interpolation
        ils->coordIndex.insertSpace(i+1, 1);
        ils->coordIndex.set1Value(i+1, ils->coordIndex[i+2]);
        coordIdx.append(bsp.addPoint(curcol));
        i++; n++;
      }
    }

    ils->color = thisp->get_or_create_color(static_cast<const SbColor *>(bsp.getPointsArrayPtr()),
                                            bsp.numPoints());

    ils->colorIndex.setValues(0, coordIdx.getLength(),
                              coordIdx.getArrayPtr());

    ils->colorPerVertex = TRUE;
  }

  THISP(closure)->insert_shape(action, ils);

  // it's important to pop state _after_ inserting the shape to get
  // the correct material from SoVertexProperty nodes.
  if (vp) {
    action->getState()->pop();
  }
  return SoCallbackAction::PRUNE;
}

SoCallbackAction::Response
SoToVRML2ActionP::sopointset_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  SoToVRML2ActionP * thisp = THISP(closure);

  const SoPointSet * oldps = coin_assert_cast<const SoPointSet *>(node);

  SoVRMLPointSet * ps = NEW_NODE(SoVRMLPointSet, node);

  // If there is a VertexProperty node set we need to put it on the state stack
  SoNode *vpnode = oldps->vertexProperty.getValue();
  SoVertexProperty *vp = coin_safe_cast<SoVertexProperty *>(vpnode);
  if (vp) {
    action->getState()->push();
    vp->callback(action);
  }


  const SoCoordinateElement * coordElem = SoCoordinateElement::getInstance(action->getState());

  int numpts = oldps->numPoints.getValue();
  // if numPts == -1, use all coordinates on the stack
  if (numpts < 0 || numpts > coordElem->getNum()) numpts = coordElem->getNum();

  if (numpts) {
    if (coordElem->getArrayPtr3() != NULL) {
      ps->coord = thisp->get_or_create_coordinate(coordElem->getArrayPtr3(), numpts);
    }
    else {
      ps->coord = thisp->get_or_create_coordinate(coordElem->getArrayPtr4(), numpts);
    }
  }

  if (action->getMaterialBinding() != SoMaterialBinding::OVERALL) {
    const SoLazyElement * colorElem = SoLazyElement::getInstance(action->getState());
    if (colorElem->getNumDiffuse() >= numpts) {
      if (colorElem->isPacked()) {
        ps->color = thisp->get_or_create_color(colorElem->getPackedPointer(),
                                               numpts);
      }
      else {
        ps->color = thisp->get_or_create_color(colorElem->getDiffusePointer(),
                                               numpts);
      }
    }
  }

  THISP(closure)->insert_shape(action, ps);
  // it's important to pop state _after_ inserting the shape to get
  // the correct material from SoVertexProperty nodes.
  if (vp) {
    action->getState()->pop();
  }

  return SoCallbackAction::PRUNE;
}

SoCallbackAction::Response
SoToVRML2ActionP::sosphere_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  if (action->getDrawStyle() != SoDrawStyle::FILLED) {
      return SoToVRML2ActionP::sotoifs_cb(closure, action, node);
  }
  SoVRMLSphere * sphere = NEW_NODE(SoVRMLSphere, node);
  const SoSphere * oldsphere = coin_assert_cast<const SoSphere *>(node);
  if (oldsphere->radius != sphere->radius)
    sphere->radius = oldsphere->radius.getValue();
  THISP(closure)->insert_shape(action, sphere);
  return SoCallbackAction::PRUNE;
}

// Property nodes
SoCallbackAction::Response
SoToVRML2ActionP::soinfo_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  const SoInfo * oldinfo = coin_assert_cast<const SoInfo *>(node);
  SoVRMLWorldInfo * info = NEW_NODE(SoVRMLWorldInfo, node);
  info->title = oldinfo->string;
  THISP(closure)->get_current_tail()->addChild(info);
  return SoCallbackAction::CONTINUE;
}

// Property nodes
SoCallbackAction::Response
SoToVRML2ActionP::solabel_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  const SoLabel * oldlabel = coin_assert_cast<const SoLabel *>(node);
  SoVRMLWorldInfo * info = NEW_NODE(SoVRMLWorldInfo, node);
  info->title = oldlabel->label.getValue().getString();
  THISP(closure)->get_current_tail()->addChild(info);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoToVRML2ActionP::somattrans_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  const SoMatrixTransform * oldt = coin_assert_cast<const SoMatrixTransform *>(node);
  SoVRMLTransform * newt = NEW_NODE(SoVRMLTransform, node);

  SbVec3f translation, scaleFactor;
  SbRotation rotation, scaleOrientation;
  oldt->matrix.getValue().getTransform(translation, rotation, scaleFactor, scaleOrientation);

  newt->translation = translation.getValue();
  newt->rotation = rotation.getValue();
  newt->scale = scaleFactor.getValue();
  newt->scaleOrientation = scaleOrientation.getValue();
  THISP(closure)->get_current_tail()->addChild(newt);
  THISP(closure)->vrml2path->append(newt);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoToVRML2ActionP::sorotation_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  const SoRotation * oldt = coin_assert_cast<const SoRotation *>(node);
  SoVRMLTransform * newt = NEW_NODE(SoVRMLTransform, node);
  newt->rotation = oldt->rotation.getValue();
  THISP(closure)->get_current_tail()->addChild(newt);
  THISP(closure)->vrml2path->append(newt);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoToVRML2ActionP::sorotationxyz_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  const SoRotationXYZ * oldt = coin_assert_cast<const SoRotationXYZ *>(node);
  SoVRMLTransform * newt = NEW_NODE(SoVRMLTransform, node);
  newt->rotation = oldt->getRotation();
  THISP(closure)->get_current_tail()->addChild(newt);
  THISP(closure)->vrml2path->append(newt);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoToVRML2ActionP::soscale_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  const SoScale * oldt = coin_assert_cast<const SoScale *>(node);
  SoVRMLTransform * newt = NEW_NODE(SoVRMLTransform, node);
  newt->scale = oldt->scaleFactor.getValue();
  THISP(closure)->get_current_tail()->addChild(newt);
  THISP(closure)->vrml2path->append(newt);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoToVRML2ActionP::sotransform_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  const SoTransform * oldt = coin_assert_cast<const SoTransform *>(node);
  SoVRMLTransform * newt = NEW_NODE(SoVRMLTransform, node);

  newt->translation = oldt->translation.getValue();
  newt->rotation = oldt->rotation.getValue();
  newt->scale = oldt->scaleFactor.getValue();
  newt->scaleOrientation = oldt->scaleOrientation.getValue();
  newt->center = oldt->center.getValue();
  THISP(closure)->get_current_tail()->addChild(newt);
  THISP(closure)->vrml2path->append(newt);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoToVRML2ActionP::sotranslation_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  const SoTranslation * oldt = coin_assert_cast<const SoTranslation *>(node);
  SoVRMLTransform * newt = NEW_NODE(SoVRMLTransform, node);
  newt->translation = oldt->translation.getValue();
  THISP(closure)->get_current_tail()->addChild(newt);
  THISP(closure)->vrml2path->append(newt);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoToVRML2ActionP::sounits_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  SoVRMLTransform * newt = NEW_NODE(SoVRMLTransform, node);

  // apply an SoGetMatrixAction to the node to find the scale factor
  SbViewportRegion dummy(100,100);
  SoGetMatrixAction gma(dummy);
  gma.apply(const_cast<SoNode *>(node));

  const SbMatrix & m = gma.getMatrix();

  // we know that the SoUnits node applies an uniform scale, so just
  // read the value in the first matrix column/row to find the scale
  // factor.
  newt->scale = SbVec3f(m[0][0], m[0][0], m[0][0]);
  THISP(closure)->get_current_tail()->addChild(newt);
  THISP(closure)->vrml2path->append(newt);
  return SoCallbackAction::CONTINUE;
}

// Other nodes
SoCallbackAction::Response
SoToVRML2ActionP::sopercam_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
    return unsupported_cb(closure, action, node);
}

SoCallbackAction::Response
SoToVRML2ActionP::sodirlight_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  SoVRMLDirectionalLight * dl = NEW_NODE(SoVRMLDirectionalLight, node);
  const SoDirectionalLight * olddl = coin_assert_cast<const SoDirectionalLight *>(node);

  dl->direction = olddl->direction.getValue();
  dl->on = olddl->on.getValue();
  dl->intensity = olddl->intensity.getValue();
  dl->color = olddl->color.getValue();
  // FIXME: SoDirectionalLight seems to not support this? 20020805 kristian.
  //dl->ambientIntensity = ambient.getValue()[0] / diffuse.getValue()[0];

  THISP(closure)->get_current_tail()->addChild(dl);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoToVRML2ActionP::sopointlight_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  SoVRMLPointLight * dl = NEW_NODE(SoVRMLPointLight, node);
  const SoPointLight * olddl = coin_assert_cast<const SoPointLight *>(node);

  dl->location = olddl->location.getValue();
  dl->on = olddl->on.getValue();
  dl->intensity = olddl->intensity.getValue();
  dl->color = olddl->color.getValue();

  THISP(closure)->get_current_tail()->addChild(dl);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoToVRML2ActionP::sospotlight_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  SoVRMLSpotLight * dl = NEW_NODE(SoVRMLSpotLight, node);
  const SoSpotLight * olddl = coin_assert_cast<const SoSpotLight *>(node);

  dl->location = olddl->location.getValue();
  dl->direction = olddl->direction.getValue();
  dl->on = olddl->on.getValue();
  dl->intensity = olddl->intensity.getValue();
  dl->color = olddl->color.getValue();
  dl->cutOffAngle = olddl->cutOffAngle.getValue();

  THISP(closure)->get_current_tail()->addChild(dl);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoToVRML2ActionP::sowwwinl_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action), const SoNode * node)
{
  SoVRMLInline * inl = NEW_NODE(SoVRMLInline, node);
  const SoWWWInline * oldinl = coin_assert_cast<const SoWWWInline *>(node);

  inl->url = oldinl->name.getValue();
  inl->bboxCenter = oldinl->bboxCenter.getValue();
  inl->bboxSize = oldinl->bboxSize.getValue();

  THISP(closure)->get_current_tail()->addChild(inl);
  return SoCallbackAction::CONTINUE;
}

// Convert nodes to ils
SoCallbackAction::Response
SoToVRML2ActionP::sotoils_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  SoToVRML2ActionP * thisp = THISP(closure);

  thisp->didpush = FALSE;
  // push state to handle SoVertexProperty node
  if ( node->isOfType(SoVertexShape::getClassTypeId()) ) {
    SoNode * vpnode = coin_assert_cast<const SoVertexShape *>(node)->vertexProperty.getValue();
    SoVertexProperty * vp = coin_safe_cast<SoVertexProperty *>(vpnode);
    if ( vp ) {
      action->getState()->push();
      vp->callback(action);
      thisp->didpush = TRUE;
    }
  }
  thisp->bsptree = new SbBSPTree;

  thisp->coordidx = new SbList <int32_t>;

  if ( action->getMaterialBinding() != SoMaterialBinding::OVERALL ) {
    const SoLazyElement * colorElem = SoLazyElement::getInstance(action->getState());
    if ( colorElem->getNumDiffuse() > 1 ) {
      thisp->coloridx = new SbList <int32_t>;
    }
  }

  thisp->do_post_primitives = TRUE;

  return SoCallbackAction::CONTINUE;
}

void
SoToVRML2ActionP::linesegment_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action),
                             const SoPrimitiveVertex * v1,
                             const SoPrimitiveVertex * v2)
{
  SoToVRML2ActionP * thisp = THISP(closure);
  assert(thisp->bsptree);

  SoPrimitiveVertex const * const arr[2] = {v1, v2};
  for (int i = 0; i < 2; i++) {
    const SoPrimitiveVertex * v = arr[i];
    thisp->coordidx->append(thisp->bsptree->addPoint(v->getPoint()));
    if (thisp->coloridx) thisp->coloridx->append(v->getMaterialIndex());
  }
  thisp->coordidx->append(-1);
  if (thisp->coloridx) thisp->coloridx->append(-1);
}

SoCallbackAction::Response
SoToVRML2ActionP::post_lines_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  SoToVRML2ActionP * thisp = THISP(closure);
  if ( !thisp->do_post_primitives ) return SoCallbackAction::CONTINUE;
  thisp->do_post_primitives = FALSE;

  SoVRMLGeometry * is;
  if ( action->getDrawStyle() == SoDrawStyle::POINTS ) {
    SoVRMLPointSet * ps = NEW_NODE(SoVRMLPointSet, node);
    is = ps;

    ps->coord = thisp->get_or_create_coordinate(thisp->bsptree->getPointsArrayPtr(),
      thisp->bsptree->numPoints());

    if ( thisp->coloridx ) {
      // Copy the colors from the state
      SoLazyElement * colorElem = SoLazyElement::getInstance(action->getState());
      if ( colorElem->getNumDiffuse() == thisp->bsptree->numPoints() ) {
        if ( colorElem->isPacked() ) {
          ps->color = thisp->get_or_create_color(colorElem->getPackedPointer(),
            colorElem->getNumDiffuse());
        }
        else {
          ps->color = thisp->get_or_create_color(colorElem->getDiffusePointer(),
            colorElem->getNumDiffuse());
        }
      }
    }

  }
  else {
    SoVRMLIndexedLineSet * ils = NEW_NODE(SoVRMLIndexedLineSet, node);
    is = ils;

    ils->coord = thisp->get_or_create_coordinate(thisp->bsptree->getPointsArrayPtr(),
      thisp->bsptree->numPoints());

    if ( thisp->coloridx ) {
      // Copy the colors from the state
      const SoLazyElement * colorElem = SoLazyElement::getInstance(action->getState());
      if ( colorElem->isPacked() ) {
        ils->color = thisp->get_or_create_color(colorElem->getPackedPointer(),
          colorElem->getNumDiffuse());
      }
      else {
        ils->color = thisp->get_or_create_color(colorElem->getDiffusePointer(),
          colorElem->getNumDiffuse());
      }

      // Index
      ils->colorIndex.setValues(0, thisp->coloridx->getLength(),
        thisp->coloridx->getArrayPtr());
    }

    ils->coordIndex.setValues(0, thisp->coordidx->getLength(),
      thisp->coordidx->getArrayPtr());
  }

  delete thisp->bsptree; thisp->bsptree = NULL;
  delete thisp->bsptreetex; thisp->bsptreetex = NULL;
  delete thisp->bsptreenormal; thisp->bsptreenormal = NULL;

  delete thisp->coordidx; thisp->coordidx = NULL;
  delete thisp->normalidx; thisp->normalidx = NULL;
  delete thisp->texidx; thisp->texidx = NULL;
  delete thisp->coloridx; thisp->coloridx = NULL;

  thisp->insert_shape(action, is);

  if ( thisp->didpush ) {
    action->getState()->pop();
  }
  return SoCallbackAction::CONTINUE;
}

// Convert nodes to ifs
SoCallbackAction::Response
SoToVRML2ActionP::sotoifs_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  SoToVRML2ActionP * thisp = THISP(closure);

  thisp->didpush = FALSE;
  // push state to handle SoVertexProperty node
  if (node->isOfType(SoVertexShape::getClassTypeId())) {
    SoNode * vpnode = coin_assert_cast<const SoVertexShape *>(node)->vertexProperty.getValue();
    SoVertexProperty * vp = coin_safe_cast<SoVertexProperty *>(vpnode);
    if (vp) {
      action->getState()->push();
      vp->callback(action);
      thisp->didpush = TRUE;
    }
  }
  thisp->bsptree = new SbBSPTree;
  thisp->bsptreenormal = new SbBSPTree;

  thisp->coordidx = new SbList <int32_t>;
  thisp->normalidx = new SbList <int32_t>;

  if (action->getMaterialBinding() != SoMaterialBinding::OVERALL) {
    const SoLazyElement * colorElem = SoLazyElement::getInstance(action->getState());
    if (colorElem->getNumDiffuse() > 1) {
      thisp->coloridx = new SbList <int32_t>;
    }
  }

  thisp->recentTex2 =
    coin_safe_cast<SoTexture2 *>(thisp->search_for_recent_node(action, SoTexture2::getClassTypeId()));
  if (thisp->recentTex2) {
    thisp->bsptreetex = new SbBSPTree;
    thisp->texidx = new SbList <int32_t>;
  }

  thisp->do_post_primitives = TRUE;

  return SoCallbackAction::CONTINUE;
}

void
SoToVRML2ActionP::triangle_cb(void * closure, SoCallbackAction * COIN_UNUSED_ARG(action),
                             const SoPrimitiveVertex * v1,
                             const SoPrimitiveVertex * v2,
                             const SoPrimitiveVertex * v3)
{
  SoToVRML2ActionP * thisp = THISP(closure);
  assert(thisp->bsptree);
  assert(thisp->bsptreenormal);

  SoPrimitiveVertex const * const arr[3] = {v1, v2, v3};
  for (int i = 0; i < 3; i++) {
    const SoPrimitiveVertex * v = arr[i];
    thisp->coordidx->append(thisp->bsptree->addPoint(v->getPoint()));
    thisp->normalidx->append(thisp->bsptreenormal->addPoint(v->getNormal()));
    if (thisp->texidx) {
      assert(thisp->bsptreetex);
      const SbVec4f & tc = v->getTextureCoords();
      thisp->texidx->append(thisp->bsptreetex->addPoint(SbVec3f(tc[0], tc[1], 0.0f)));
    }
    if (thisp->coloridx) thisp->coloridx->append(v->getMaterialIndex());
  }
  thisp->coordidx->append(-1);
  thisp->normalidx->append(-1);
  if (thisp->texidx) thisp->texidx->append(-1);
  if (thisp->coloridx) thisp->coloridx->append(-1);
}

SoCallbackAction::Response
SoToVRML2ActionP::post_primitives_cb(void * closure, SoCallbackAction * action, const SoNode * node)
{
  SoToVRML2ActionP * thisp = THISP(closure);
  if (!thisp->do_post_primitives) return SoCallbackAction::CONTINUE;
  thisp->do_post_primitives = FALSE;

  SoVRMLGeometry * is;
  if (action->getDrawStyle() == SoDrawStyle::POINTS) {
    SoVRMLPointSet * ps = NEW_NODE(SoVRMLPointSet, node);
    is = ps;

    ps->coord = thisp->get_or_create_coordinate(thisp->bsptree->getPointsArrayPtr(),
                                                thisp->bsptree->numPoints());

    if (thisp->coloridx) {
      // Copy the colors from the state
      SoLazyElement * colorElem = SoLazyElement::getInstance(action->getState());
      if (colorElem->getNumDiffuse() == thisp->bsptree->numPoints()) {
        if (colorElem->isPacked()) {
          ps->color = thisp->get_or_create_color(colorElem->getPackedPointer(),
                                                 colorElem->getNumDiffuse());
        }
        else {
          ps->color = thisp->get_or_create_color(colorElem->getDiffusePointer(),
                                                 colorElem->getNumDiffuse());
        }
      }
    }

  } else
  if (action->getDrawStyle() == SoDrawStyle::LINES) {
    SoVRMLIndexedLineSet * ils = NEW_NODE(SoVRMLIndexedLineSet, node);
    is = ils;

    ils->coord = thisp->get_or_create_coordinate(thisp->bsptree->getPointsArrayPtr(),
                                                 thisp->bsptree->numPoints());

    if (thisp->coloridx) {
      // Copy the colors from the state
      const SoLazyElement * colorElem = SoLazyElement::getInstance(action->getState());
      if (colorElem->isPacked()) {
        ils->color = thisp->get_or_create_color(colorElem->getPackedPointer(),
                                                colorElem->getNumDiffuse());
      } else {
        ils->color = thisp->get_or_create_color(colorElem->getDiffusePointer(),
                                                colorElem->getNumDiffuse());
      }

      // Index
      ils->colorIndex.setValues(0, thisp->coloridx->getLength(),
                                thisp->coloridx->getArrayPtr());
    }

    int n = thisp->coordidx->getLength();
    const int32_t * a = thisp->coordidx->getArrayPtr();
    SbList <int32_t> l;
    int32_t p = a[0];
    for (int i = 0; i < n; i++) {
      if (a[i] == -1) {
        l.append(p);
        if (i < n-1) p = a[i+1];
      }
      l.append(a[i]);
    }
    ils->coordIndex.setValues(0, l.getLength(),
                              l.getArrayPtr());

  } else {
    SoVRMLIndexedFaceSet * ifs = NEW_NODE(SoVRMLIndexedFaceSet, node);
    is = ifs;

    // we need some specific handling for VRML nodes, since these
    // nodes store the state inside the node in fields
    const SoSFBool * ccw_field = NULL;
    const SoSFBool * solid_field = NULL;
    const SoSFBool * convex_field = NULL;
    const SoSFFloat * creaseangle_field = NULL;

    if (node->isOfType(SoVRMLGeometry::getClassTypeId())) {
      ccw_field = static_cast<const SoSFBool*>(node->getField("ccw"));
      solid_field = static_cast<const SoSFBool*>(node->getField("solid"));
      convex_field = static_cast<const SoSFBool*>(node->getField("convex"));
      creaseangle_field = static_cast<const SoSFFloat*>(node->getField("creaseAngle"));
    }

    // Set the values from the current ShapeHints
    ifs->creaseAngle = creaseangle_field ? creaseangle_field->getValue() : action->getCreaseAngle();
    if (node->isOfType(SoVertexShape::getClassTypeId())) {
      ifs->ccw = action->getVertexOrdering() != SoShapeHints::CLOCKWISE;
    }
    else {
      ifs->ccw = ccw_field ? ccw_field->getValue() : TRUE;
    }
    ifs->solid = solid_field ? solid_field->getValue() : (SoShapeHintsElement::getShapeType(action->getState()) == SoShapeHintsElement::SOLID);
    ifs->convex = convex_field ? convex_field->getValue() : (action->getFaceType() == SoShapeHints::CONVEX);

    ifs->coord = thisp->get_or_create_coordinate(thisp->bsptree->getPointsArrayPtr(),
                                                thisp->bsptree->numPoints());

    ifs->normal = thisp->get_or_create_normal(thisp->bsptreenormal->getPointsArrayPtr(),
                                             thisp->bsptreenormal->numPoints());

    if (thisp->coloridx) {
      // Copy the colors from the state
      const SoLazyElement * colorElem = SoLazyElement::getInstance(action->getState());
      if (colorElem->isPacked()) {
        ifs->color = thisp->get_or_create_color(colorElem->getPackedPointer(),
                                                colorElem->getNumDiffuse());
      } else {
        ifs->color = thisp->get_or_create_color(colorElem->getDiffusePointer(),
                                                colorElem->getNumDiffuse());
      }

      // Index
      ifs->colorIndex.setValues(0, thisp->coloridx->getLength(),
                                thisp->coloridx->getArrayPtr());

    }

    if (thisp->texidx) {
      // Copy texture coordinates
      SoVRMLTextureCoordinate * tex = new SoVRMLTextureCoordinate;
      int n = thisp->bsptreetex->numPoints();
      tex->point.setNum(n);
      SbVec2f * ptr = tex->point.startEditing();
      for (int i = 0; i < n; i++) {
        SbVec3f p = thisp->bsptreetex->getPoint(i);
        ptr[i] = SbVec2f(p[0], p[1]);
      }
      tex->point.finishEditing();
      ifs->texCoord = tex;

      // Index
      ifs->texCoordIndex.setValues(0, thisp->texidx->getLength(),
                                  thisp->texidx->getArrayPtr());
    }

    ifs->coordIndex.setValues(0, thisp->coordidx->getLength(),
                             thisp->coordidx->getArrayPtr());
    ifs->normalIndex.setValues(0, thisp->normalidx->getLength(),
                              thisp->normalidx->getArrayPtr());

  }

  delete thisp->bsptree; thisp->bsptree = NULL;
  delete thisp->bsptreetex; thisp->bsptreetex = NULL;
  delete thisp->bsptreenormal; thisp->bsptreenormal = NULL;

  delete thisp->coordidx; thisp->coordidx = NULL;
  delete thisp->normalidx; thisp->normalidx = NULL;
  delete thisp->texidx; thisp->texidx = NULL;
  delete thisp->coloridx; thisp->coloridx = NULL;

  thisp->insert_shape(action, is);

  if (thisp->didpush) {
    action->getState()->pop();
  }
  return SoCallbackAction::CONTINUE;
}

#undef NEW_NODE
#undef DEFAULT_VIEWPORT_WIDTH
#undef DEFAULT_VIEWPORT_HEIGHT

#undef PRIVATE
#undef PUBLIC
#undef THISP

#endif // HAVE_VRML97
