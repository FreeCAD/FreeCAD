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
  \class SoVRMLLOD SoVRMLLOD.h Inventor/VRMLnodes/SoVRMLLOD.h
  \brief The SoVRMLLOD class is used to represent various levels of detail based on distance.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  LOD {
    exposedField MFNode  level    []
    field        SFVec3f center   0 0 0    # (-,)
    field        MFFloat range    []       # (0,)
  }
  \endverbatim

  The LOD node specifies various levels of detail or complexity for a
  given object, and provides hints allowing browsers to automatically
  choose the appropriate version of the object based on the distance
  from the user. The level field contains a list of nodes that
  represent the same object or objects at varying levels of detail,
  ordered from highest level of detail to the lowest level of
  detail. The range field specifies the ideal distances at which to
  switch between the levels. Subclause 4.6.5, Grouping and children
  nodes
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.5>),
  contains details on the types of nodes that are legal values
  for level.

  The center field is a translation offset in the local coordinate
  system that specifies the centre of the LOD node for distance
  calculations.

  The number of nodes in the level field shall exceed the number of
  values in the range field by one (i.e., N+1 level values for N range
  values). The range field contains monotonic increasing values that
  shall be greater than zero. In order to calculate which level to
  display, first the distance is calculated from the viewer's
  location, transformed into the local coordinate system of the LOD
  node (including any scaling transformations), to the center point of
  the LOD node.  Then, the LOD node evaluates the step function L(d)
  to choose a level for a given value of d (where d is the distance
  from the viewer position to the centre of the LOD node).  Let n
  ranges, R0, R1, R2, ..., Rn-1, partition the domain (0, +infinity)
  into n+1 subintervals given by (0, R0), [R0, R1)...  , [Rn-1,
  +infinity). Also, let n levels L0, L1, L2, ..., Ln-1 be the values
  of the step function function L(d). The level node, L(d), for a
  given distance d is defined as follows:

  \verbatim
    L(d) = L0,   if d < R0,
         = Li+1, if Ri <= d < Ri+1, for -1 < i < n-1,
         = Ln-1, if d >= Rn-1.
  \endverbatim

  Specifying too few levels will result in the last level being used
  repeatedly for the lowest levels of detail. If more levels than
  ranges are specified, the extra levels are ignored. An empty range
  field is an exception to this rule. This case is a hint to the
  browser that it may choose a level automatically to maintain a
  constant display rate. Each value in the range field shall be
  greater than the previous value.

  LOD nodes are evaluated top-down in the scene graph. Only the
  descendants of the currently selected level are rendered. All nodes
  under an LOD node continue to receive and send events regardless of
  which LOD node's level is active. For example, if an active
  TimeSensor node is contained within an inactive level of an LOD
  node, the TimeSensor node sends events regardless of the LOD node's
  state.
*/

/*!
  \var SoMFFloat SoVRMLLOD::range
  The range for each level.
*/

/*!
  \var SoSFVec3f SoVRMLLOD::center
  The center used when calculating distance.
*/

/*!
  \var SoMFNode SoVRMLLOD::level
  The different levels.
*/

#include <Inventor/VRMLnodes/SoVRMLLOD.h>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/VRMLnodes/SoVRMLParent.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/system/gl.h>

#include "glue/glp.h"
#include "rendering/SoGL.h"
#include "nodes/SoSubNodeP.h"
#include "nodes/SoSoundElementHelper.h"
#include "profiler/SoNodeProfiling.h"

// *************************************************************************

class SoVRMLLODP  : public SoSoundElementHelper
{
public:
  SbBool childlistvalid;
};

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

SO_NODE_SOURCE(SoVRMLLOD);

// *************************************************************************

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLLOD::initClass(void) // static
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLLOD, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLLOD::SoVRMLLOD(void)
{
  this->commonConstructor();
}

/*!
  Destructor.
*/
SoVRMLLOD::~SoVRMLLOD() // virtual, protected
{
  delete PRIVATE(this);
}

/*!
  Constructor. \a levels is the expected number of levels.
*/
SoVRMLLOD::SoVRMLLOD(int levels)
  : SoGroup(levels)
{
  this->commonConstructor();
}

void
SoVRMLLOD::commonConstructor(void)
{
  PRIVATE(this) = new SoVRMLLODP;
  PRIVATE(this)->childlistvalid = FALSE;
  
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLLOD);

  SO_VRMLNODE_ADD_FIELD(center, (0.0f, 0.0f, 0.0f));
  SO_VRMLNODE_ADD_EMPTY_MFIELD(range);
  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(level);

  // HACK WARNING: All children of this node are stored in the level
  // field. Avoid double notifications (because of notification
  // through SoChildList) be reallocating the SoChildList with a
  // NULL-parent here. SoGroup will have allocated an SoChildList in
  // its constructor when we get here.
  delete this->SoGroup::children;
  this->SoGroup::children = new SoChildList(NULL);
}

// *************************************************************************

// Doc in parent
SbBool
SoVRMLLOD::affectsState(void) const // virtual
{
  return FALSE;
}

/*!
  Adds a new level.
*/
void
SoVRMLLOD::addLevel(SoNode * levelptr)
{
  this->addChild(levelptr);
}

/*!
  Inserts a new level.
*/
void
SoVRMLLOD::insertLevel(SoNode * levelptr,
                       int idx)
{
  this->insertChild(levelptr, idx);
}

/*!
  Returns a level.
*/
SoNode *
SoVRMLLOD::getLevel(int idx) const
{
  return this->getChild(idx);
}

/*!
  Find \a node, and return the level index or -1 if not found.
*/
int
SoVRMLLOD::findLevel(const SoNode * node) const
{
  return this->findChild(node);
}

/*!
  Return the number of levels.
*/
int
SoVRMLLOD::getNumLevels(void) const
{
  return this->level.getNum();
}

/*!
  Removes the level at index \a idx.
*/
void
SoVRMLLOD::removeLevel(int idx)
{
  this->removeChild(idx);
}

/*!
  Find \a level, and remove it if found.
*/
void
SoVRMLLOD::removeLevel(SoNode * levelptr)
{
  this->removeChild(levelptr);
}

/*!
  Remove all levels.
*/
void
SoVRMLLOD::removeAllLevels(void)
{
  this->removeAllChildren();
}

/*!
  Replace the level at \a idx with \a node.
*/
void
SoVRMLLOD::replaceLevel(int idx,
                        SoNode * node)
{
  this->replaceChild(idx, node);
}

/*!
  Find \a oldnode, and replace it with \a newnode.
*/
void
SoVRMLLOD::replaceLevel(SoNode * oldnode,
                        SoNode * newnode)
{
  this->replaceChild(oldnode, newnode);
}

// Doc in parent
void
SoVRMLLOD::doAction(SoAction * action)
{
  int numindices;
  const int * indices;
  SoAction::PathCode pathcode = action->getPathCode(numindices, indices);
  if (pathcode == SoAction::IN_PATH) {
    this->getChildren()->traverseInPath(action, numindices, indices);
  }
  else {
    int idx = this->whichToTraverse(action);;
    if (idx >= 0) {
      this->getChildren()->traverse(action, idx);
      PRIVATE(this)->enableTraversingOfInactiveChildren();
      PRIVATE(this)->traverseInactiveChildren(this, action, idx, pathcode,
                                              this->getNumChildren(), 
                                              this->getChildren());
    }
  }
}

// Doc in parent
void
SoVRMLLOD::callback(SoCallbackAction * action)
{
  SoVRMLLOD::doAction((SoAction*)action);
}

// Doc in parent
void
SoVRMLLOD::GLRender(SoGLRenderAction * action)
{
  switch (action->getCurPathCode()) {
  case SoAction::NO_PATH:
  case SoAction::BELOW_PATH:
    SoVRMLLOD::GLRenderBelowPath(action);
    break;
  case SoAction::IN_PATH:
    SoVRMLLOD::GLRenderInPath(action);
    break;
  case SoAction::OFF_PATH:
    SoVRMLLOD::GLRenderOffPath(action);
    break;
  default:
    assert(0 && "unknown path code.");
    break;
  }
}

// Doc in parent
void
SoVRMLLOD::rayPick(SoRayPickAction * action)
{
  SoVRMLLOD::doAction((SoAction*) action);
}

// Doc in parent
void
SoVRMLLOD::getBoundingBox(SoGetBoundingBoxAction * action)
{
  // FIXME: SGI OIV seems to do some extra work here, but the manual
  // pages states that it should do a normal SoGroup traversal.
  // we should _not_ use whichToTraverse() to calculate bbox as
  // this would cause cache dependencies on the camera and
  // the model matrix.                       pederb, 2001-02-21
  inherited::getBoundingBox(action);
}

// Doc in parent
void
SoVRMLLOD::search(SoSearchAction * action)
{
  inherited::search(action);
}

// Doc in parent
void
SoVRMLLOD::write(SoWriteAction * action)
{
  SoNode::write(action);
}

// Doc in parent
void
SoVRMLLOD::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoVRMLLOD::doAction((SoAction*) action);
}

// Doc in parent
void
SoVRMLLOD::audioRender(SoAudioRenderAction * action)
{
  PRIVATE(this)->preAudioRender(this, action);
  SoVRMLLOD::doAction((SoAction*) action);
  PRIVATE(this)->postAudioRender(this, action);
}

// Doc in parent
void
SoVRMLLOD::GLRenderBelowPath(SoGLRenderAction * action)
{
  int idx = this->whichToTraverse(action);
  if (idx >= 0) {
    SoNode * child = (SoNode*) this->getChildren()->get(idx);
    action->pushCurPath(idx, child);
    if (!action->abortNow()) {
      SoNodeProfiling profiling;
      profiling.preTraversal(action);
      child->GLRenderBelowPath(action);
      profiling.postTraversal(action);
#if COIN_DEBUG
      // The GL error test is default disabled for this optimized
      // path.  If you get a GL error reporting an error in the
      // Separator node, enable this code by setting the environment
      // variable COIN_GLERROR_DEBUGGING to "1" to see exactly which
      // node caused the error.
      static SbBool chkglerr = sogl_glerror_debugging();
      if (chkglerr) {
        cc_string str;
        cc_string_construct(&str);
        const unsigned int errs = coin_catch_gl_errors(&str);
        if (errs > 0) {
          SoDebugError::post("SoVRMLLOD::GLRenderBelowPath",
                             "glGetError()s => '%s', nodetype: '%s'",
                             cc_string_get_text(&str),
                             (*this->getChildren())[idx]->getTypeId().getName().getString());
        }
        cc_string_clean(&str);
      }
#endif // COIN_DEBUG
    }
    action->popCurPath();
  }
  // don't auto cache LOD nodes.
  SoGLCacheContextElement::shouldAutoCache(action->getState(),
                                           SoGLCacheContextElement::DONT_AUTO_CACHE);
}

// Doc in parent
void
SoVRMLLOD::GLRenderInPath(SoGLRenderAction * action)
{
  int numindices;
  const int * indices;
  SoAction::PathCode pathcode = action->getPathCode(numindices, indices);

  if (pathcode == SoAction::IN_PATH) {
    for (int i = 0; (i < numindices) && !action->hasTerminated(); i++) {
      int idx = indices[i];
      SoNode * child = this->getChild(idx);
      action->pushCurPath(idx, child);
      if (!action->abortNow()) {
        SoNodeProfiling profiling;
        profiling.preTraversal(action);
        child->GLRenderInPath(action);
        profiling.postTraversal(action);
      }
      action->popCurPath(pathcode);
    }
  }
  else {
    assert(pathcode == SoAction::BELOW_PATH);
    SoVRMLLOD::GLRenderBelowPath(action);
  }
}

// Doc in parent
void
SoVRMLLOD::GLRenderOffPath(SoGLRenderAction * action)
{
  int idx = this->whichToTraverse(action);;
  if (idx >= 0) {
    SoNode * child = this->getChild(idx);
    if (child->affectsState()) {
      action->pushCurPath(idx, child);
      if (!action->abortNow()) {
        SoNodeProfiling profiling;
        profiling.preTraversal(action);
        child->GLRenderOffPath(action);
        profiling.postTraversal(action);
      }
      action->popCurPath();
    }
  }
}

// Doc in parent
void
SoVRMLLOD::addChild(SoNode * child)
{
  this->level.addNode(child);
  PRIVATE(this)->childlistvalid = FALSE;
}

// Doc in parent
void
SoVRMLLOD::insertChild(SoNode * child, int idx)
{
  this->level.insertNode(child, idx);
  PRIVATE(this)->childlistvalid = FALSE;
}

// Doc in parent
SoNode *
SoVRMLLOD::getChild(int idx) const
{
  return this->level.getNode(idx);
}

// Doc in parent
int
SoVRMLLOD::findChild(const SoNode * child) const
{
  return this->level.findNode(child);
}

// Doc in parent
int
SoVRMLLOD::getNumChildren(void) const // virtual
{
  return this->level.getNumNodes();
}

// Doc in parent
void
SoVRMLLOD::removeChild(int idx)
{
  this->level.removeNode(idx);
  PRIVATE(this)->childlistvalid = FALSE;
}

// Doc in parent
void
SoVRMLLOD::removeChild(SoNode * child)
{
  this->level.removeNode(child);
  PRIVATE(this)->childlistvalid = FALSE;
}

// Doc in parent
void
SoVRMLLOD::removeAllChildren(void)
{
  this->level.removeAllNodes();
  SoGroup::children->truncate(0);
  PRIVATE(this)->childlistvalid = TRUE;
}

// Doc in parent
void
SoVRMLLOD::replaceChild(int idx, SoNode * child)
{
  this->level.replaceNode(idx, child);
  PRIVATE(this)->childlistvalid = FALSE;
}

// Doc in parent
void
SoVRMLLOD::replaceChild(SoNode * old,
                           SoNode * child)
{
  this->level.replaceNode(old, child);
  PRIVATE(this)->childlistvalid = FALSE;
}

// Doc in parent
void
SoVRMLLOD::notify(SoNotList * list)
{
  SoField * f = list->getLastField();
  if (f == &this->level) {
    PRIVATE(this)->childlistvalid = FALSE;
  }
  inherited::notify(list);
  PRIVATE(this)->notifyCalled();
}

// Doc in parent
SbBool
SoVRMLLOD::readInstance(SoInput * in,
                        unsigned short flags)
{
  SoGroup::children->truncate(0);
  SbBool oldnot = this->level.enableNotify(FALSE);
  SbBool ret = inherited::readInstance(in, flags);
  if (oldnot) this->level.enableNotify(TRUE);
  PRIVATE(this)->childlistvalid = FALSE;
  return ret;
}

// Doc in parent
void
SoVRMLLOD::copyContents(const SoFieldContainer * from,
                        SbBool copyConn)
{
  SoGroup::children->truncate(0);
  SoNode::copyContents(from, copyConn);
  PRIVATE(this)->childlistvalid = FALSE;
}

/*!
  Returns the child to traverse based on distance to current viewpoint.
*/
int
SoVRMLLOD::whichToTraverse(SoAction * action)
{
  // FIXME: according to the spec, if range is empty, we should decide
  // a level to try to maintain a constant/high frame rate...
  // pederb, 2002-06-10
  SoState * state = action->getState();
  const SbMatrix & mat = SoModelMatrixElement::get(state);
  const SbViewVolume & vv = SoViewVolumeElement::get(state);

  SbVec3f worldcenter;
  mat.multVecMatrix(this->center.getValue(), worldcenter);

  float dist = (vv.getProjectionPoint() - worldcenter).length();

  int i;
  int n = this->range.getNum();

  for (i = 0; i < n; i++) {
    if (dist < this->range[i]) break;
  }
  if (i >= this->getNumChildren()) i = this->getNumChildren() - 1;
  return i;
}

// Doc in parent
SoChildList *
SoVRMLLOD::getChildren(void) const
{
  if (!PRIVATE(this)->childlistvalid) {
    SoVRMLParent::updateChildList(this->level.getValues(0),
                                  this->level.getNum(),
                                  *SoGroup::children);
    ((SoVRMLLOD*)this)->pimpl->childlistvalid = TRUE;
  }
  return SoGroup::children;
}

#undef PRIVATE

#endif // HAVE_VRML97
