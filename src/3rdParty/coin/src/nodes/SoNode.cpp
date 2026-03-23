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
  \class SoNode SoNode.h Inventor/nodes/SoNode.h
  \brief The SoNode class is the base class for nodes used in scene graphs.

  \ingroup coin_nodes

  Coin is a \e retained \e mode 3D visualization library (built on top
  of the \e immediate \e mode OpenGL library). "Retained mode" means
  that instead of passing commands to draw graphics primitives
  directly to the renderer, you build up data structures which are
  rendered by the library \e on \e demand.

  The node classes are the main "primitive" for building these data
  structures. In Coin, you build tree hierarchies made up of different
  node types: group nodes (for the tree structure layout of the other
  nodes), appearance nodes (for setting up materials, textures, etc),
  shape nodes (for the actual geometry), and nodes for lighting and
  camera positioning.

  One common issue with newcomers to the API is that you should not
  and cannot use the C++ delete operator on nodes -- the destructor
  is protected. This is because node instances are using a common
  technique for memory resource handling called "reference
  counting". Nodes are deleted (actually, they delete themselves) when
  their unref() method is called and the reference count goes to zero.

  One important side-effect of this is that SoNode-derived classes
  should \e not be statically allocated, neither in static module
  memory nor on functions stack frames. SoNode-derived classes must
  \e always be allocated dynamically from the memory heap with the \c
  new operator (or else the scheme with self-destruction upon
  dereferencing to 0 would not work).


  Usually application programmers won't manually ref() and unref()
  nodes a lot, because you pass the nodes directly to
  SoGroup::addChild() or So\@Gui\@Viewer\::setSceneGraph() or something
  similar.  These functions will ref() the nodes they are passed, and
  unref() them when they are finished with them.

  Make sure you do ref() nodes that you keep pointers to so they
  aren't accidentally deleted prematurely due to an unref() call from
  within the library itself.  If you haven't manually called ref() on
  a top level root node, it will then be deleted automatically. This
  code shows how to do it:

  \code
  SoSeparator * root = new SoSeparator; // root's refcount starts out at zero
  root->addChild(foo_node); // foo_node refcount is increased by 1
  root->addChild(bar_node); // bar_node refcount +1

  // increase refcount before passing it to setScenegraph(), to avoid
  // premature destruction
  root->ref();

  myviewer->setSceneGraph(root); // root's refcount +1, is now 2

  // [misc visualization and processing]

  // myviewer will let go of its reference to the root node, thereby
  // decreasing its reference count by 1
  myviewer->setSceneGraph(NULL);

  // root's refcount goes from +1 to 0, and it will self-destruct controllably
  root->unref();
  // avoid dangling pointer, in case "root" is attempted used again
  // (not really necessary, but good for smoking out bugs early)
  root = NULL;
  \endcode

  For full information and tutorial-style introductions to all API
  issues, see the "Inventor Mentor: Programming Object-Oriented 3D
  Graphics with Open Inventor" (ISBN 0-201-62495-8). It has detailed
  explanations on all the basic principles involved.

  See specifically the section "References and Deletion" in Chapter 3
  to learn about the reference counting techniques.


  Often when using the Coin library, one is interested in making
  extensions to it. Of particular interest is setting up extension
  nodes, which are then traversed, rendered and otherwise used by the
  rest of the library as any internal node.

  The Coin header file Inventor/nodes/SoSubNode.h includes a set of
  convenience macros for quick and easy construction of extension
  nodes. Here's a complete snippet of code which shows how to set up a
  skeleton framework for an extension node class:

  \code
  #include <Inventor/nodes/SoWWWInline.h>

  //// Definition of extension class "MyWWWInline" ///////////////

  class MyWWWInline : public SoWWWInline {
    SO_NODE_HEADER(MyWWWInline);

  public:
    static void initClass(void);
    MyWWWInline(void);

  protected:
    virtual ~MyWWWInline();
  };

  //// Implementation of extension class "MyWWWInline" ///////////

  SO_NODE_SOURCE(MyWWWInline);

  MyWWWInline::MyWWWInline(void)
  {
    SO_NODE_CONSTRUCTOR(MyWWWInline);
  }

  MyWWWInline::~MyWWWInline()
  {
  }

  void
  MyWWWInline::initClass(void)
  {
    SO_NODE_INIT_CLASS(MyWWWInline, SoWWWInline, "SoWWWInline");
  }

  //// main //////////////////////////////////////////////////////

  int
  main(int argc, char ** argv)
  {
    SoDB::init();
    MyWWWInline::initClass();

    // [...]

    return 0;
  }
  \endcode

  You can then override for instance the GLRender() method to have
  your new class render OpenGL geometry different from its
  superclass.

  \TOOLMAKER_REF

  For information about dynamic loading of extension nodes, see the
  documentation of SoType::fromName().
*/

/*!
\class SbUniqueId SbBasic.h Inventor/SbBasic.h
\brief SbUniqueId is an integer type for node identifiers.

\ingroup coin_base

SbUniqueId is meant to be a "32/64 bit portable" way of defining an
integer type that is used for storing unique node identifiers.

SbUniqueId is not really a class, just a \c typedef.
*/


// *************************************************************************

#include <Inventor/nodes/SoNode.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cassert>
#include <cstdlib>

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/actions/SoActions.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/misc/SoProto.h>
#include <Inventor/misc/SoProtoInstance.h>
#include <Inventor/nodes/SoNodes.h>
#include <Inventor/engines/SoNodeEngine.h>
#include <Inventor/lists/SoEngineOutputList.h>

#include "tidbitsp.h"
#include "misc/SbHash.h"
#include "rendering/SoGL.h"
#include "nodes/SoSubNodeP.h"
#include "nodes/SoUnknownNode.h"
#include "threads/threadsutilp.h"
#include "glue/glp.h"
#include "misc/SoDBP.h" // for global envvar COIN_PROFILER
#include "coindefs.h"   // COIN_CHECK_THREAD

// *************************************************************************

/*!
  \var SbUniqueId SoNode::uniqueId
  \COININTERNAL
*/
/*!
  \var SbUniqueId SoNode::nextUniqueId
  \COININTERNAL
*/
/*!
  \var int SoNode::nextActionMethodIndex
  \COININTERNAL
*/

/*!
  \enum SoNode::NodeType
  Used to store node type.
*/

/*!
  \var SoNode::NodeType SoNode::INVENTOR
  Specifies Inventor node type.
*/

/*!
  \var SoNode::NodeType SoNode::VRML1
  Node is VRML V1.0 compatible.
*/

/*!
  \var SoNode::NodeType SoNode::VRML2
  Node is from the VRML V2.0 specification.
*/

/*!
  \var SoNode::NodeType SoNode::INVENTOR_1
  Node was part of SGI Inventor version 1.
*/

/*!
  \var SoNode::NodeType SoNode::INVENTOR_2_0
  Node was part of SGI Inventor version 2.0.
*/

/*!
  \var SoNode::NodeType SoNode::INVENTOR_2_1
  Node was introduced with SGI / TGS Inventor version 2.1.
*/

/*!
  \var SoNode::NodeType SoNode::INVENTOR_2_5
  Node was introduced with TGS Inventor version 2.5.
*/

/*!
  \var SoNode::NodeType SoNode::INVENTOR_2_6
  Node was introduced with TGS Inventor version 2.6.
*/

/*!
  \var SoNode::NodeType SoNode::COIN_1_0
  Node was part of Coin version 1.0.
*/

/*!
  \var SoNode::NodeType SoNode::COIN_2_0
  Node was introduced with Coin 2.0.
*/

/*!
  \var SoNode::NodeType SoNode::COIN_2_2
  Node was introduced with Coin 2.2.
*/

/*!
  \var SoNode::NodeType SoNode::COIN_2_3
  Node was introduced with Coin 2.3.
*/

/*!
  \var SoNode::NodeType SoNode::COIN_2_4
  Node was introduced with Coin 2.4.
*/

/*!
  \var SoNode::NodeType SoNode::INVENTOR_5_0
  Node was introduced with TGS Inventor version 5.0.
*/

/*!
  \var SoNode::NodeType SoNode::COIN_2_5
  Node was introduced with Coin 2.5.
*/

/*!
  \var SoNode::NodeType SoNode::COIN_3_0
  Node was introduced with Coin 3.0.
*/

/*!
  \var SoNode::NodeType SoNode::INVENTOR_6_0
  Node was introduced with TGS Inventor version 6.0.
*/

/*!
  \var SoNode::NodeType SoNode::COIN_4_0
  Node was introduced with Coin 4.0.
*/

/*!
  \var SoNode::NodeType SoNode::EXTENSION
  Node is a client code extension.
*/

// *************************************************************************

SbUniqueId SoNode::nextUniqueId = 1;
int SoNode::nextActionMethodIndex = 0;
SoType SoNode::classTypeId STATIC_SOTYPE_INIT;
static void * sonode_mutex = NULL;

typedef SbHash<int16_t, uint32_t> Int16ToUInt32Map;
static Int16ToUInt32Map * compatibility_dict = NULL;

static void init_action_methods(void);

// *************************************************************************

/*!
  This static method returns the SoType object associated with
  objects of this class.
*/
SoType
SoNode::getClassTypeId(void)
{
  return SoNode::classTypeId;
}

// *************************************************************************

// defines for node state flags

// we can currently have 31 node types. The last bit is used to store
// the override flag.
#define FLAG_TYPEMASK 0x7fffffff
#define FLAG_OVERRIDE 0x80000000

// private methods. Inlined inside this file only.

// clear bits in stateflags
inline void
SoNode::clearStateFlags(const unsigned int bits)
{
  this->stateflags &= ~bits;
}

// sets bits in stateflags
inline void
SoNode::setStateFlags(const unsigned int bits)
{
  this->stateflags |= bits;
}

// return TRUE if any of bits are set
inline SbBool
SoNode::getState(const unsigned int bits) const
{
  return (this->stateflags & bits) != 0;
}

// Macro which sets the node id for the node to a new unique id.  The
// node id is used by the caching system in Coin to quickly identify
// changes in the scene graph (and to optimize notification). To
// simplify the VBO handling in attribute nodes, no node can have
// nodeid == 0 (making it possible for VBO caches to set the current
// dataid to 0 to mark the data as invalid / not set).
#define SET_UNIQUE_NODE_ID(obj) \
  CC_MUTEX_LOCK(sonode_mutex); \
  (obj)->uniqueId = SoNode::nextUniqueId++;   \
  if (obj->uniqueId == 0) {                 \
    obj->uniqueId = SoNode::nextUniqueId++; \
  } \
  CC_MUTEX_UNLOCK(sonode_mutex)

// *************************************************************************

/*!
  Default constructor, initializes node instance.
*/
SoNode::SoNode(void)
{
  SET_UNIQUE_NODE_ID(this);
  this->stateflags = 0; // clear all flags

  // set node type to Inventor by default.
  this->setNodeType(SoNode::INVENTOR);
}

/*!
  Destructor.
*/
SoNode::~SoNode()
{
  // check if this is an SoProtoInstance root node.
  SoProtoInstance * inst = SoProtoInstance::findProtoInstance(this);
  if (inst) {
    // unref the instance
    inst->unref();
  }
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoNode::~SoNode", "%p", this);
#endif // debug
}

// *************************************************************************

/*!
  Make a duplicate of this node and return a pointer to the duplicate.

  If this node is a group node, children are also copied and we return
  a pointer to the root of a full copy of the subgraph rooted here.

  If \a copyconnections is \c TRUE, we also copy the connections to
  fields within this node (and ditto for any children and children's
  children etc.).


  Note that this function has been made virtual in Coin, which is not
  the case in the original Open Inventor API. We may change this
  method back into being non-virtual again for major Coin versions
  after this, as it was made virtual more or less by mistake. So
  please don't write application code that depends on SoNode::copy()
  being virtual.

  The reason this method should not be virtual is because this is \e
  not the function the application programmer should override in
  extension nodes if she needs some special behavior during a copy
  operation (like copying the value of internal data not exposed as
  fields).

  For that purpose, override the copyContents() method. Your
  overridden copyContents() method should then \e both copy internal
  data as well as calling the parent superclass' copyContents() method
  for automatically handling of fields and other common data.
*/
SoNode *
SoNode::copy(SbBool copyconnections) const
{
  // FIXME: "de-virtualize" this method for next major Coin release?
  // See method documentation above. 20011220 mortene.

  SoFieldContainer::initCopyDict();
  SoNode * cp = this->addToCopyDict();
  // ref() to make sure the copy is not destructed while copying
  cp->ref();
  // Call findCopy() to have copyContents() run only once.
#if COIN_DEBUG
  SoNode * cp2 = (SoNode *)SoFieldContainer::findCopy(this, copyconnections);
  assert(cp == cp2);
#else // COIN_DEBUG
  (void) SoFieldContainer::findCopy(this, copyconnections);
#endif
  SoFieldContainer::copyDone();
  // unrefNoDelete() so that we return a copy with reference count 0
  cp->unrefNoDelete();
  return cp;
}


// Overridden from parent.
void
SoNode::startNotify(void)
{
  inherited::startNotify();
}

// Overridden from parent.
void
SoNode::notify(SoNotList * l)
{
  COIN_CHECK_THREAD();

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoNode::notify", "node %p (%s \"%s\"), list %p",
                         this, this->getTypeId().getName().getString(),
                         this->getName().getString(), l);
#endif // debug

  // only continue if node hasn't already been notified.
  // The time stamp is set in the SoNotList constructor.
  if (l->getTimeStamp() > this->uniqueId) {
    SET_UNIQUE_NODE_ID(this);
    inherited::notify(l);
  }
}

/*!
  \COININTERNAL
*/
int
SoNode::getActionMethodIndex(const SoType type)
{
  return type.getData();
}

/*!
  \COININTERNAL

  Only in TGS Inventor on Win32 -- to avoid needing to export the
  nextActionMethodIndex member, see SoNode.h for more information.
 */
void
SoNode::setNextActionMethodIndex(int index)
{
  SoNode::nextActionMethodIndex = index;
}

/*!
  \COININTERNAL

  Only in TGS Inventor on Win32 -- to avoid needing to export the
  nextActionMethodIndex member, see SoNode.h for more information.
*/
int
SoNode::getNextActionMethodIndex(void)
{
  return SoNode::nextActionMethodIndex;
}

/*!
  \COININTERNAL

  Only in TGS Inventor on Win32 -- to avoid needing to export the
  nextActionMethodIndex member, see SoNode.h for more information.
 */
void
SoNode::incNextActionMethodIndex(void)
{
  SoNode::nextActionMethodIndex++;
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoNode::initClass(void)
{
  // Make sure we only initialize once.
  assert(SoNode::classTypeId == SoType::badType());
  // Make sure parent class has been initialized.
  assert(inherited::getClassTypeId() != SoType::badType());

  CC_MUTEX_CONSTRUCT(sonode_mutex);
  SoNode::classTypeId =
    SoType::createType(inherited::getClassTypeId(), "Node", NULL,
                       SoNode::nextActionMethodIndex++);

  // initialize the compatibility dict
  compatibility_dict = new Int16ToUInt32Map;
  coin_atexit((coin_atexit_f*)SoNode::cleanupClass, CC_ATEXIT_NORMAL);

  SoNode::setCompatibilityTypes(SoNode::getClassTypeId(),
                                SO_FROM_INVENTOR_1);

  SoNode::initClasses();

  // action methods must be initialized here, since both nodes and
  // actions must be initialized before we can use
  // SO_ACTION_ADD_METHOD
  init_action_methods();
}

/*!
  Initialize all the node classes of Coin.
*/
void
SoNode::initClasses(void)
{
  SoCamera::initClass();
  SoPerspectiveCamera::initClass();
  SoReversePerspectiveCamera::initClass();
  SoOrthographicCamera::initClass();
  SoFrustumCamera::initClass();
  SoShape::initClass();
  SoAsciiText::initClass();
  SoCone::initClass();
  SoCube::initClass();
  SoCylinder::initClass();
  SoVertexShape::initClass();
  SoNonIndexedShape::initClass();
  SoFaceSet::initClass();
  SoLineSet::initClass();
  SoPointSet::initClass();
  SoMarkerSet::initClass();
  SoQuadMesh::initClass();
  SoTriangleStripSet::initClass();
  SoIndexedShape::initClass();
  SoIndexedFaceSet::initClass();
  SoIndexedLineSet::initClass();
  SoIndexedPointSet::initClass();
  SoIndexedMarkerSet::initClass();
  SoIndexedTriangleStripSet::initClass();
  SoImage::initClass();
  SoIndexedNurbsCurve::initClass();
  SoIndexedNurbsSurface::initClass();
  SoNurbsCurve::initClass();
  SoNurbsSurface::initClass();
  SoSphere::initClass();
  SoText2::initClass();
  SoText3::initClass();
  SoGroup::initClass();
  SoSeparator::initClass();
  SoAnnotation::initClass();
  SoLocateHighlight::initClass();
  SoWWWAnchor::initClass();
  SoArray::initClass();
  SoSwitch::initClass();
  SoBlinker::initClass();
  SoLOD::initClass();
  SoLevelOfDetail::initClass();
  SoMultipleCopy::initClass();
  SoPathSwitch::initClass();
  SoTransformSeparator::initClass();
  SoTransformation::initClass();
  SoMatrixTransform::initClass();
  SoRotation::initClass();
  SoPendulum::initClass();
  SoRotor::initClass();
  SoResetTransform::initClass();
  SoRotationXYZ::initClass();
  SoScale::initClass();
  SoTranslation::initClass();
  SoShuttle::initClass();
  SoTransform::initClass();
  SoUnits::initClass();
  SoBaseColor::initClass();
  SoCallback::initClass();
  SoClipPlane::initClass();
  SoColorIndex::initClass();
  SoComplexity::initClass();
  SoCoordinate3::initClass();
  SoCoordinate4::initClass();
  SoLight::initClass();
  SoDirectionalLight::initClass();
  SoSpotLight::initClass();
  SoPointLight::initClass();
  SoDrawStyle::initClass();
  SoEnvironment::initClass();
  SoEventCallback::initClass();
  SoFile::initClass();
  SoFont::initClass();
  SoFontStyle::initClass();
  SoInfo::initClass();
  SoLabel::initClass();
  SoLightModel::initClass();
  SoProfile::initClass();
  SoLinearProfile::initClass();
  SoNurbsProfile::initClass();
  SoMaterial::initClass();
  SoMaterialBinding::initClass();
  SoVertexAttributeBinding::initClass();
  SoNormal::initClass();
  SoNormalBinding::initClass();
  SoPackedColor::initClass();
  SoPickStyle::initClass();
  SoPolygonOffset::initClass();
  SoProfileCoordinate2::initClass();
  SoProfileCoordinate3::initClass();
  SoShapeHints::initClass();
  SoTexture::initClass();
  SoTexture2::initClass();
  SoTexture3::initClass();
  SoTexture2Transform::initClass();
  SoTexture3Transform::initClass();
  SoTextureMatrixTransform::initClass();
  SoTextureCoordinate2::initClass();
  SoTextureCoordinate3::initClass();
  SoTextureCoordinateBinding::initClass();
  SoTextureCoordinateFunction::initClass();
  SoTextureCoordinateDefault::initClass();
  SoTextureCoordinateEnvironment::initClass();
  SoTextureCoordinatePlane::initClass();
  SoUnknownNode::initClass();
  SoVertexProperty::initClass();
  SoWWWInline::initClass();
  SoListener::initClass();

  SoTransparencyType::initClass();
  SoTextureScalePolicy::initClass();

  SoTextureUnit::initClass();

  SoBumpMap::initClass();
  SoBumpMapCoordinate::initClass();
  SoBumpMapTransform::initClass();

  SoSceneTexture2::initClass();
  SoSceneTextureCubeMap::initClass();

  SoTextureCoordinateCube::initClass();
  SoTextureCoordinateSphere::initClass();
  SoTextureCoordinateCylinder::initClass();

  SoTextureCombine::initClass();
  SoCacheHint::initClass();
  SoTextureCubeMap::initClass();
  SoTextureCoordinateNormalMap::initClass();
  SoTextureCoordinateReflectionMap::initClass();
  SoTextureCoordinateObject::initClass();
  SoVertexAttribute::initClass();

  SoDepthBuffer::initClass();
  SoAlphaTest::initClass();
}

/*!
  Set the override flag.

  If this flag is \c TRUE, the field values of this node will override
  the field values of other nodes of the same type during scene graph
  traversal.

  A common application for "override nodes" is to place them at the top
  of the tree as a convenient way to force e.g. a common draw style on
  the complete tree.

  The override flag does not exist in the Inventor file format.  This
  flag is in other words not persistent, and must be programmatically
  set.  The rationale for this flag is for viewers to be able to
  control rendering style of the 3D models, so it would look stupid if
  some parts of certain models suddenly didn't adhere to the viewer
  mode.
*/
void
SoNode::setOverride(const SbBool state)
{
  if (state != this->getState(FLAG_OVERRIDE)) {
    // This change affects caches in the tree, so we must change our id
    // setting, so the caches are regenerated.
    SET_UNIQUE_NODE_ID(this);

    if (state) this->setStateFlags(FLAG_OVERRIDE);
    else this->clearStateFlags(FLAG_OVERRIDE);
  }
}

/*!
  Return status of override flag.

  \sa setOverride()
*/
SbBool
SoNode::isOverride(void) const
{
  return this->getState(FLAG_OVERRIDE);
}

/*!
  Sets the node type for this node to \a type. Since some nodes
  should be handled differently in VRML1 vs. Inventor, this
  should be used to get correct behavior for those cases.
  The default node type is INVENTOR.

  This method is an extension versus the Open Inventor API.

  \sa getNodeType()
*/
void
SoNode::setNodeType(const NodeType type)
{
  // make sure we have enough bits to store this type
  assert((uint32_t) type <= FLAG_TYPEMASK);
  // clear old type
  this->clearStateFlags(FLAG_TYPEMASK);
  // set new type
  this->setStateFlags((uint32_t) type);
}

/*!
  Returns the node type set for this node.

  This method is an extension versus the Open Inventor API.

  \sa setNodeType()
*/
SoNode::NodeType
SoNode::getNodeType(void) const
{
  uint32_t type = this->stateflags & FLAG_TYPEMASK;
  return (NodeType) type;
}

/*!
  Returns the last node that was registered under \a name.

  \sa SoBase::setName()
*/
SoNode *
SoNode::getByName(const SbName & name)
{
  SoBase * b = SoBase::getNamedBase(name, SoNode::getClassTypeId());
  if (!b) return NULL;
  return (SoNode *)b;
}

/*!
  Finds all nodes with \a name and appends them to the \a l node list.
  Returns the number of nodes with the specified name.

  \sa SoBase::setName()
*/
int
SoNode::getByName(const SbName & name, SoNodeList & l)
{
  SoBaseList bl;
  int nr = SoBase::getNamedBases(name, bl, SoNode::getClassTypeId());
  for (int i=0; i < nr; i++) l.append((SoNode *)bl[i]);
  return nr;
}


// *************************************************************************
// * ACTION STUFF
// *************************************************************************

/*!
  This function performs the typical operation of a node for any
  action.
*/
void
SoNode::doAction(SoAction * COIN_UNUSED_ARG(action))
{
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Returns \c TRUE if the node could have any effect on the state
  during traversal.

  If it returns \c FALSE, no data in the traversal state will change
  from the pre-traversal state to the post-traversal state. The
  SoSeparator node will for instance return \c FALSE, as it pushes and
  pops the state before and after traversal of its children. All
  SoShape nodes will also return \c FALSE, as just pushing out
  geometry data to the rendering engine won't affect the actual
  rendering state.

  The default method returns \c TRUE, on a "better safe than sorry"
  philosophy.
*/
SbBool
SoNode::affectsState(void) const
{
  return TRUE;
}

/*!
  This is a static "helper" method registered with the action, and
  used for calling the SoNode::getBoundingBox() virtual method which
  does the \e real work.
*/
void
SoNode::getBoundingBoxS(SoAction * action, SoNode * node)
{
  assert(action && node);
  SoGetBoundingBoxAction * bboxaction = (SoGetBoundingBoxAction *)action;
  bboxaction->checkResetBefore();
  node->getBoundingBox(bboxaction);
  bboxaction->checkResetAfter();
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Action method for the SoGetBoundingBoxAction.

  Calculates bounding box and center coordinates for node and modifies
  the values of the \a action to encompass the bounding box for this
  node and to shift the center point for the scene more towards the
  one for this node.

  Nodes influencing how geometry nodes calculate their bounding box
  also override this method to change the relevant state variables.
*/
void
SoNode::getBoundingBox(SoGetBoundingBoxAction * COIN_UNUSED_ARG(action))
{
}

/*!
  This is a static "helper" method registered with the action, and
  used for calling the SoNode::getPrimitiveCount() virtual method
  which does the \e real work.
*/
void
SoNode::getPrimitiveCountS(SoAction * action, SoNode * node)
{
  assert(action && node);
  node->getPrimitiveCount((SoGetPrimitiveCountAction *)action);
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Action method for the SoGetPrimitiveCountAction.

  Calculates the number of triangle, line segment and point primitives
  for the node and adds these to the counters of the \a action.

  Nodes influencing how geometry nodes calculate their primitive
  count also override this method to change the relevant state
  variables.
*/
void
SoNode::getPrimitiveCount(SoGetPrimitiveCountAction * COIN_UNUSED_ARG(action))
{
}

// *************************************************************************

/*!
  This is a static "helper" method registered with the action, and
  used for calling the SoNode::GLRender() virtual method which does
  the \e real work.
*/
void
SoNode::GLRenderS(SoAction * action, SoNode * node)
{
  if ((action->getCurPathCode() != SoAction::OFF_PATH) ||
      node->affectsState()) {
    if (((SoGLRenderAction*)action)->abortNow()) {
      SoCacheElement::invalidate(action->getState());
    }
    else {
      node->GLRender((SoGLRenderAction*)action);
    }
  }

  if (COIN_DEBUG) {
    // Note: debugging code like this is also present in
    // SoSeparator::GLRenderBelowPath() and SoState::lazyEvaluate(),
    // but they are default disabled -- even when COIN_DEBUG=1 (due to
    // performance reasons).
    //
    // If you're seeing notifications about GL-errors from this place,
    // the first thing to do is to enable those debugging checks too
    // by setting COIN_GLERROR_DEBUGGING to "1".
    cc_string str;
    cc_string_construct(&str);
    const unsigned int errs = coin_catch_gl_errors(&str);
    if (errs > 0) {
      const SbBool extradebug = sogl_glerror_debugging();
      SoDebugError::post("SoNode::GLRenderS",
                         "GL error: '%s', nodetype: %s %s",
                         cc_string_get_text(&str),
                         node->getTypeId().getName().getString(),
                         extradebug ? "" :
                         "(set envvar COIN_GLERROR_DEBUGGING=1 "
                         "and re-run to get more information)");
    }
    cc_string_clean(&str);
  }
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Action method for the SoGLRenderAction.

  This is called during rendering traversals. Nodes influencing the
  rendering state in any way or want to throw geometry primitives
  at OpenGL override this method.
*/
void
SoNode::GLRender(SoGLRenderAction * COIN_UNUSED_ARG(action))
{
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Implements the SoAction::BELOW_PATH traversal method for the
  rendering action.
*/
void
SoNode::GLRenderBelowPath(SoGLRenderAction * action)
{
  this->GLRender(action);
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Implements the SoAction::IN_PATH traversal method for the rendering
  action.
*/
void
SoNode::GLRenderInPath(SoGLRenderAction * action)
{
  this->GLRender(action);
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Implements the SoAction::OFF_PATH traversal method for the rendering
  action.
*/
void
SoNode::GLRenderOffPath(SoGLRenderAction * action)
{
  this->GLRender(action);
}

// *************************************************************************

/*!
  This is a static "helper" method registered with the action, and
  used for calling the SoNode::callback() virtual method which does
  the \e real work.
*/
void
SoNode::callbackS(SoAction * action, SoNode * node)
{
  assert(action && node);
  SoCallbackAction * const cbAction = (SoCallbackAction *)(action);
  if (cbAction->hasTerminated()) return;
  cbAction->setCurrentNode(node);

  cbAction->invokePreCallbacks(node);
  if (cbAction->getCurrentResponse() == SoCallbackAction::CONTINUE) {
    node->callback(cbAction);
  }
  cbAction->invokePostCallbacks(node);
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Action method for SoCallbackAction.

  Simply updates the state according to how the node behaves for the
  render action, so the application programmer can use the
  SoCallbackAction for extracting information about the scene graph.
*/
void
SoNode::callback(SoCallbackAction * COIN_UNUSED_ARG(action))
{
}

/*!
  This is a static "helper" method registered with the action, and
  used for calling the SoNode::getMatrix() virtual method which does
  the \e real work.
*/
void
SoNode::getMatrixS(SoAction * action, SoNode * node)
{
  assert(action && node);
  assert(action->getTypeId() == SoGetMatrixAction::getClassTypeId());
  SoGetMatrixAction * const getMatrixAction = (SoGetMatrixAction *)(action);
  node->getMatrix(getMatrixAction);
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Action method for SoGetMatrixAction.

  Updates \a action by accumulating with the transformation matrix of
  this node (if any).
*/
void
SoNode::getMatrix(SoGetMatrixAction * COIN_UNUSED_ARG(action))
{
}

/*!
  This is a static "helper" method registered with the action, and
  used for calling the SoNode::handleEvent() virtual method which does
  the \e real work.
*/
void
SoNode::handleEventS(SoAction * action, SoNode * node)
{
  assert(action && node);
  assert(action->getTypeId().isDerivedFrom(SoHandleEventAction::getClassTypeId()));
  SoHandleEventAction * handleEventAction = (SoHandleEventAction *)(action);
  node->handleEvent(handleEventAction);
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Action method for SoHandleEventAction.

  Inspects the event data from \a action, and processes it if it is
  something which this node should react to.

  Nodes influencing relevant state variables for how event handling is
  done also override this method.
*/
void
SoNode::handleEvent(SoHandleEventAction * COIN_UNUSED_ARG(action))
{
}

/*!
  This is a static "helper" method registered with the action, and
  used for calling the SoNode::pick() virtual method which does the \e
  real work.
*/
void
SoNode::pickS(SoAction * action, SoNode * node)
{
  assert(action && node);
  assert(action->getTypeId().isDerivedFrom(SoPickAction::getClassTypeId()));
  SoPickAction * const pickAction = (SoPickAction *)(action);
  node->pick(pickAction);
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Action method for SoPickAction.

  Does common processing for SoPickAction \a action instances.
*/
void
SoNode::pick(SoPickAction * COIN_UNUSED_ARG(action))
{
}

/*!
  This is a static "helper" method registered with the action, and
  used for calling the SoNode::rayPick() virtual method which does the
  \e real work.
*/
void
SoNode::rayPickS(SoAction * action, SoNode * node)
{
  assert(action && node);
  assert(action->getTypeId().isDerivedFrom(SoRayPickAction::getClassTypeId()));
  SoRayPickAction * const rayPickAction = (SoRayPickAction *)(action);
  node->rayPick(rayPickAction);
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Action method for SoRayPickAction.

  Checks the ray specification of the \a action and tests for
  intersection with the data of the node.

  Nodes influencing relevant state variables for how picking is done
  also override this method.
*/
void
SoNode::rayPick(SoRayPickAction * action)
{
  // if node has no defined rayPick(), try the pick method
  this->pick(action);
}

/*!
  This is a static "helper" method registered with the action, and
  used for calling the SoNode::search() virtual method which does the
  \e real work.
*/
void
SoNode::searchS(SoAction * action, SoNode * node)
{
  assert(action && node);
  assert(action->getTypeId().isDerivedFrom(SoSearchAction::getClassTypeId()));
  SoSearchAction * const searchAction = (SoSearchAction *)(action);
  node->search(searchAction);
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Action method for SoSearchAction.

  Compares the search criteria from the \a action to see if this node
  is a match. Searching is done by matching up \e all criteria set up
  in the SoSearchAction -- if \e any of the requested criteria is a
  miss, the search is not deemed successful for the node.

  \sa SoSearchAction
*/
void
SoNode::search(SoSearchAction * action)
{
  if (action->isFound()) { return; }

  int lookfor = action->getFind();
  SbBool hit = FALSE;

  // A little tidbit of history, which could be relevant when
  // answering support inquiries: Coin v1.0.0 was released with a bug
  // where just one hit out of the criteria would make the search
  // operation on the node successful.  Since this doesn't match
  // neither the behavior of SGI Inventor nor the documentation for
  // SoSearchAction, we corrected the behavior for Coin v1.0.1 even
  // though this is on the borderline of what is acceptable for fixing
  // in a minor patch-release update.
  //
  // mortene.

  if (lookfor & SoSearchAction::NODE) {
    hit = this == action->getNode();
    if (!hit) { return; }
  }

  if (lookfor & SoSearchAction::NAME) {
    hit = this->getName() == action->getName();
    if (!hit) { return; }
  }

  if (lookfor & SoSearchAction::TYPE) {
    SbBool chkderived;
    SoType searchtype = action->getType(chkderived);
    hit = (this->getTypeId() == searchtype) ||
      (chkderived && this->getTypeId().isDerivedFrom(searchtype));
    if (!hit) { return; }
  }

  if (hit) { action->addPath(action->getCurPath()->copy()); }
}

/*!
  This is a static "helper" method registered with the action, and
  used for calling the SoNode::write() virtual method which does the
  \e real work.
*/
void
SoNode::writeS(SoAction * action, SoNode * node)
{
  assert(action && node);
  assert(action->getTypeId().isDerivedFrom(SoWriteAction::getClassTypeId()));
  SoWriteAction * const writeAction = (SoWriteAction *)(action);

  // Do not write Proto instance graphs. Just let the Proto instance
  // class handle the writing.
  SoProtoInstance * proto = SoProtoInstance::findProtoInstance(node);
  if (proto) {
    node = proto;
  }
  node->write(writeAction);
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Action method for SoWriteAction.

  Writes out a node object, and any connected nodes, engines etc, if
  necessary.
*/
void
SoNode::write(SoWriteAction * action)
{
  SoOutput * out = action->getOutput();

  SoNode * node = this;

  SoProtoInstance * proto = SoProtoInstance::findProtoInstance(this);
  if (proto) { node = proto; }

  if (out->getStage() == SoOutput::COUNT_REFS) {
    node->addWriteReference(out, FALSE);
  }
  else if (out->getStage() == SoOutput::WRITE) {
    if (node->writeHeader(out, FALSE, FALSE)) return;

    // check for special case where we actually have to write out an
    // SoEngineOutput "field". An engine output might be connected via
    // an IS reference in a PROTO, and we then need to write back this
    // IS reference when exporting the VRML file.
    SoProto * proto = out->getCurrentProto();
    if (proto && node->isOfType(SoNodeEngine::getClassTypeId())) {
      SoEngineOutputList l;
      const int num = ((SoNodeEngine*)node)->getOutputs(l);

      for (int i = 0; i < num; i++) {
        SbName name;
        if (((SoNodeEngine*)node)->getOutputName(l[i], name)) {
          SbName pname = proto->findISReference(node, name);
          if (pname.getLength()) {
            out->indent();
            out->write(name.getString());
            out->write(" IS ");
            out->write(pname.getString());
            out->write("\n");
          }
        }
      }
    }
    node->getFieldData()->write(out, node);
    node->writeFooter(out);
  }
  else assert(0 && "unknown stage");
}

/*!
  This is a static "helper" method registered with the action, and
  used for calling the SoNode::audioRender() virtual method which does the \e
  real work.
*/
void
SoNode::audioRenderS(SoAction * action, SoNode * node)
{
  assert(action && node);
  assert(action->getTypeId().isDerivedFrom(SoAudioRenderAction::getClassTypeId()));
  SoAudioRenderAction * const ara = (SoAudioRenderAction *)(action);
  node->audioRender(ara);
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Action method for SoAudioRenderAction.

  Does common processing for SoAudioRenderAction \a action instances.
*/
void
SoNode::audioRender(SoAudioRenderAction * COIN_UNUSED_ARG(action))
{
}

// Note that this documentation will also be used for all subclasses
// which reimplements the method, so keep the doc "generic enough".
/*!
  Returns list of children for this node.
*/
SoChildList *
SoNode::getChildren(void) const
{
  return NULL;
}

/*!
  Called from SoHandleEventAction::setGrabber() to notify a node when
  it becomes the node where all events are sent.
*/
void
SoNode::grabEventsSetup(void)
{
}

/*!
  Called from SoHandleEventAction to notify a node when it looses
  status as the node where events are sent.
*/
void
SoNode::grabEventsCleanup(void)
{
}

/*!
  This returns the node's current unique identification number. It is
  unlikely that application programmers will ever need use this method
  from client application code, unless working with extensions to the
  core library (and probably not even then).

  The id number is only valid for as long as the node is kept
  unchanged -- upon \e any kind of change the internal id will be
  updated (in the notify() method), and the old id number forgotten.

  The technique described above plays an important role in the way
  internal scene graph caches are set up and invalidated.

  \sa SoNode::getNextNodeId()
*/
SbUniqueId
SoNode::getNodeId(void) const
{
  return this->uniqueId;
}

// Documentation in superclass.
void
SoNode::writeInstance(SoOutput * out)
{
  SoNode * node = this;

  SoProtoInstance * proto = SoProtoInstance::findProtoInstance(this);
  if (proto) { node = proto; }

  // Catch common misuse of SoOutput (a single pass instead of two,
  // lacking the setStage() initialization).
  assert(((out->getStage() == SoOutput::COUNT_REFS) ||
          (out->getStage() == SoOutput::WRITE)) &&
         "unknown write stage");
  SoWriteAction wa(out);
  wa.continueToApply(node);
}

/*!
  Add a copy of this node and (recursively) all children to the copy
  dictionary of SoFieldContainer if this has not already been done.

  Used internally during copy operations.
*/
SoNode *
SoNode::addToCopyDict(void) const
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoNode::addToCopyDict",
                         "%s node", this->getTypeId().getName().getString());
#endif // debug

  SoNode * cp = (SoNode *)SoFieldContainer::checkCopy(this);
  if (!cp) {
    // We need to do some extra work when copying nodes that are
    // ProtoInstance root nodes. We create a new ProtoInstance node,
    // and register its root node as the copy. pederb, 2002-06-17
    SoProtoInstance * inst = SoProtoInstance::findProtoInstance(this);
    if (inst) {
      SoProto * proto = inst->getProtoDefinition();
      SoProtoInstance * newinst = proto->createProtoInstance();
      if (inst->getName().getLength()) newinst->setName(inst->getName());
      cp = newinst->getRootNode();
      assert(cp);
      // We have to call addCopy() before calling copyContents() since
      // the proto instance might have a field that has a pointer to
      // the root node. pederb, 2002-09-04
      SoFieldContainer::addCopy(this, cp);
      newinst->copyContents(inst, FALSE);
    }
    else {
      if (this->isOfType(SoProto::getClassTypeId())) {
        // just copy the pointer. A PROTO definition is
        // read-only. It's not possible to change it after it has been
        // created so this should be safe.
        cp = (SoNode*) this;
      }
      else {
        cp = (SoNode *)this->getTypeId().createInstance();
      }
      assert(cp);
      SoFieldContainer::addCopy(this, cp);

      SoChildList * l = this->getChildren();
      for (int i=0; l && (i < l->getLength()); i++)
        (void)(*l)[i]->addToCopyDict();
    }
  }
  return cp;
}

// Doc in superclass.
void
SoNode::copyContents(const SoFieldContainer * from, SbBool copyconnections)
{
  // workaround when copying PROTO definitions. A PROTO definition is
  // read-only, and we just copy the pointer (in
  // SoNode::addToCopyDict(), not the contents.
  if (!this->isOfType(SoProto::getClassTypeId())) {
    inherited::copyContents(from, copyconnections);

    SoNode * src = (SoNode *)from;
    this->stateflags = src->stateflags;
  }
}

// Overridden from parent class.
SoFieldContainer *
SoNode::copyThroughConnection(void) const
{
  // Important note: _don't_ try to optimize by skipping the
  // checkCopy() call, as we're not supposed to create copies of
  // containers "outside" the part of the scene graph which is
  // involved in the copy operation.
  SoFieldContainer * connfc = SoFieldContainer::checkCopy(this);
  // if a copy has been made, return the findCopy instance (findCopy
  // will run copyContents() the first time it's called on an
  // instance).
  if (connfc) return SoFieldContainer::findCopy(this, TRUE);
  // if no copy has been made, just return self
  return (SoFieldContainer*) this;
}

/*!
  Return the next unique identification number to be assigned upon
  node construction or change.  It is unlikely that application
  programmers will ever need use this method from client application
  code, unless working with extensions to the core library (and
  probably not even then).

  \sa SoNode::getNodeId
*/
SbUniqueId
SoNode::getNextNodeId(void)
{
  return SoNode::nextUniqueId;
}

/*!
  \COININTERNAL

  Returns the SoFieldData class which holds information about fields
  in this node.
*/
const SoFieldData **
SoNode::getFieldDataPtr(void)
{
  return NULL;
}

// Doc in super.
SbBool
SoNode::readInstance(SoInput * in, unsigned short flags)
{
  // Overridden to set node type.

  SbBool ret = inherited::readInstance(in, flags);
  if (ret) {
    if (in->isFileVRML1()) this->setNodeType(SoNode::VRML1);
    else if (in->isFileVRML2()) this->setNodeType(SoNode::VRML2);
  }
  return ret;
}

/*!
  Get the node compatibility mask for node type \a nodetype.  The
  return value will be a bit mask of SoNode::NodeType flags,
  containing one or several flags.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
uint32_t
SoNode::getCompatibilityTypes(const SoType & nodetype)
{
  assert(compatibility_dict);
  assert(nodetype.isDerivedFrom(SoNode::getClassTypeId()));

  uint32_t tmp;
  if (compatibility_dict->get(nodetype.getKey(), tmp)) { return tmp; }
  return SoNode::EXTENSION;
}

/*!
  Set the node compatibility mask for node type \a nodetype.  The mask
  specifies for which file formats the node is supported.

  \COIN_FUNCTION_EXTENSION

  \sa getCompatibilityMode()
  \since Coin 2.0
*/
void
SoNode::setCompatibilityTypes(const SoType & nodetype, const uint32_t bitmask)
{
  assert(compatibility_dict);
  assert(nodetype.isDerivedFrom(SoNode::getClassTypeId()));
  compatibility_dict->put(nodetype.getKey(), bitmask);
}

/*!
  This static method cleans up static data of the SoNode class.
*/
void
SoNode::cleanupClass(void)
{
  delete compatibility_dict;
  SoNode::classTypeId STATIC_SOTYPE_INIT;
  CC_MUTEX_DESTRUCT(sonode_mutex);
}

// just undef flags here

#undef FLAG_TYPEMASK
#undef FLAG_OVERRIDE

// The following function should probably eventually be renamed/moved
// to SoAction::initActionMethods(). We cannot initialize action
// methods in SoAction::initClass() since nodes must be initialized
// before we can set up action methods, and we cannot initialize nodes
// before actions, since elements (which also depend on actions) are
// enabled in nodes.
static void
init_action_methods(void)
{
  SoCallbackAction::addMethod(SoNode::getClassTypeId(), SoNode::callbackS);
  SoGLRenderAction::addMethod(SoNode::getClassTypeId(), SoNode::GLRenderS);
  SoGetBoundingBoxAction::addMethod(SoNode::getClassTypeId(), SoNode::getBoundingBoxS);
  SoGetMatrixAction::addMethod(SoNode::getClassTypeId(), SoNode::getMatrixS);
  SoGetPrimitiveCountAction::addMethod(SoNode::getClassTypeId(), SoNode::getPrimitiveCountS);
  SoHandleEventAction::addMethod(SoNode::getClassTypeId(), SoNode::handleEventS);
  SoPickAction::addMethod(SoNode::getClassTypeId(), SoNode::pickS);

  // most methods for SoRayPickAction are inherited from SoPickAction
  SoRayPickAction::addMethod(SoCamera::getClassTypeId(), SoNode::rayPickS);
  SoRayPickAction::addMethod(SoSeparator::getClassTypeId(), SoNode::rayPickS);
  SoRayPickAction::addMethod(SoLOD::getClassTypeId(), SoNode::rayPickS);
  SoRayPickAction::addMethod(SoLevelOfDetail::getClassTypeId(), SoNode::rayPickS);
  SoRayPickAction::addMethod(SoShape::getClassTypeId(), SoNode::rayPickS);
  SoRayPickAction::addMethod(SoTexture2::getClassTypeId(), SoNode::rayPickS);
  SoRayPickAction::addMethod(SoBumpMap::getClassTypeId(), SoNode::rayPickS);
  SoRayPickAction::addMethod(SoImage::getClassTypeId(), SoNode::rayPickS);
  SoRayPickAction::addMethod(SoSceneTexture2::getClassTypeId(), SoNode::rayPickS);
  SoRayPickAction::addMethod(SoSceneTextureCubeMap::getClassTypeId(), SoNode::rayPickS);
  SoRayPickAction::addMethod(SoTextureCubeMap::getClassTypeId(), SoNode::rayPickS);

  SoSearchAction::addMethod(SoNode::getClassTypeId(), SoNode::searchS);
  SoWriteAction::addMethod(SoNode::getClassTypeId(), SoNode::writeS);

  SoAudioRenderAction::addMethod(SoNode::getClassTypeId(),
                                 SoAction::nullAction);
  SoAudioRenderAction::addMethod(SoListener::getClassTypeId(),
                                 SoNode::audioRenderS);
  SoAudioRenderAction::addMethod(SoCamera::getClassTypeId(),
                                 SoNode::audioRenderS);
  SoAudioRenderAction::addMethod(SoGroup::getClassTypeId(),
                                 SoNode::audioRenderS);
  SoAudioRenderAction::addMethod(SoWWWInline::getClassTypeId(),
                                 SoNode::audioRenderS);
  SoAudioRenderAction::addMethod(SoFile::getClassTypeId(),
                                 SoNode::audioRenderS);
  // just call doAction() for all transformation nodes. This will make
  // sound nodes work even for extension nodes that implements the
  // doAction() method
  SoAudioRenderAction::addMethod(SoTransformation::getClassTypeId(),
                                 SoAudioRenderAction::callDoAction);
}

#undef SET_UNIQUE_NODE_ID
