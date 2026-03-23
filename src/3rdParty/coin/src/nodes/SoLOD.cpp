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
  \class SoLOD SoLOD.h Inventor/nodes/SoLOD.h
  \brief The SoLOD class is used to choose a child based distance between viewer and object.

  \ingroup coin_nodes

  The class documentation for the SoLOD node class would be similar
  enough to that of SoLevelOfDetail that we will refer you to look at
  that one first. It will explain the general principles of what a
  level-of-detail mechanism is, and why and how to use it.

  (The main difference between SoLOD and SoLevelOfDetail is that SoLOD
  uses the faster "distance-to-viewer" technique for implementing
  level-of-detail functionality, versus the more correct (but
  potentially slower) "projected-bbox-area" technique used by
  SoLevelOfDetail.)

  Here's a mockup example (in Inventor file format style, but easily
  converted to code) that shows how to use this node:

  \code
  LOD {
     range [ 10, 20, 30, 40 ]

     Sphere { }
     Cylinder { }
     Cone { }
     Cube { }
     Info { }
  }
  \endcode

  For the sub-scene graph above, when the LOD-object is less than 10
  units away from the viewpoint camera, an SoSphere will be shown. For
  distances 10 - 20 units away, this will be changed to the
  SoCylinder, and so on. For distances of \e more than 40 units from
  the camera, an SoInfo node will be traversed / rendered -- i.e.,
  nothing will be shown. (This is a common "trick" used to optimize
  rendering when models get far enough away from the camera that we
  want to remove them completely).

  Note that when using an SoOrthographicCamera in the examiner viewers
  of the SoQt, SoWin and SoXt libraries, it will seem like the SoLOD
  node is not working when using the "Zoom" functionality. What
  happens is that an SoOrthographicCamera is not actually moved, the
  "zoom" effect is accomplished simply by changing its height-of-view
  setting (i.e. the value of the SoOrthographicCamera::height field),
  but its position remains constant. So the distance to the camera
  will not change, which means SoLOD will pick the same child, no
  matter how much one "zooms".

  (The SoLevelOfDetail node uses the screen space area of the object
  to decide when to switch children, so that node will still work with
  SoOrthographicCamera.)

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    LOD {
        center 0 0 0
        range [  ]
    }
  \endcode

  \since SGI Inventor 2.1
  \sa SoLevelOfDetail
*/

// *************************************************************************

#include <Inventor/nodes/SoLOD.h>

#include <Inventor/misc/SoChildList.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>

#include "nodes/SoSubNodeP.h"
#include "nodes/SoSoundElementHelper.h"
#include "profiler/SoNodeProfiling.h"

// *************************************************************************

/*!
  \var SoMFFloat SoLOD::range

  The distance ranges which decides when to use each child for
  traversal / rendering. See usage example in main class documentation
  of SoLOD for an explanation of how this vector should be set up
  correctly.

  By default this vector just contains a single value 0.0f.
*/
/*!
  \var SoSFVec3f SoLOD::center

  This vector represents an offset within the object from the
  geometric center point to the center point the application
  programmer would actually like the distance between the viewer and
  the object to be calculated from.

  Default value is [0, 0, 0]. It is usually not necessary to change
  this field.
*/

// *************************************************************************

class SoLODP : public SoSoundElementHelper
{
public:
  SoLODP(SoLOD * master) : master(master) {};
  SoLOD *master;
};

#define PRIVATE(p) ((p)->pimpl)
#define PUBLIC(p) ((p)->master)

SO_NODE_SOURCE(SoLOD);

/*!
  Default constructor.
*/
SoLOD::SoLOD(void)
{
  this->commonConstructor();
}

/*!
  Constructor.

  The argument should be the approximate number of children which is
  expected to be inserted below this node. The number need not be
  exact, as it is only used as a hint for better memory resource
  allocation.
*/
SoLOD::SoLOD(int numchildren)
  : inherited(numchildren)
{
  this->commonConstructor();
}

// private
void
SoLOD::commonConstructor(void)
{
  PRIVATE(this) = new SoLODP(this);

  SO_NODE_INTERNAL_CONSTRUCTOR(SoLOD);

  SO_NODE_ADD_FIELD(center, (SbVec3f(0, 0, 0)));
  SO_NODE_ADD_FIELD(range, (0.0f));

  // Make multivalue field empty, as that is the default.
  this->range.setNum(0);
  // So it's not written in its default state on SoWriteAction
  // traversal.
  this->range.setDefault(TRUE);
}

/*!
  Destructor.
*/
SoLOD::~SoLOD()
{
  delete PRIVATE(this);
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoLOD::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoLOD, SO_FROM_INVENTOR_2_1|SoNode::VRML1);
}


// Documented in superclass.
void
SoLOD::doAction(SoAction *action)
{
  int numindices;
  const int * indices;
  SoAction::PathCode pathcode = action->getPathCode(numindices, indices);
  if (pathcode == SoAction::IN_PATH) {
    this->children->traverseInPath(action, numindices, indices);
  }
  else {
    int idx = this->whichToTraverse(action);
    if (idx >= 0) {
      this->children->traverse(action, idx);
      PRIVATE(this)->enableTraversingOfInactiveChildren();
      PRIVATE(this)->traverseInactiveChildren(this, action, idx, pathcode,
                                              this->getNumChildren(),
                                              this->getChildren());
    }
  }
}

// Documented in superclass.
void
SoLOD::callback(SoCallbackAction *action)
{
  SoLOD::doAction((SoAction*)action);
}

// Documented in superclass.
void
SoLOD::audioRender(SoAudioRenderAction * action)
{
  PRIVATE(this)->preAudioRender(this, action);
  SoLOD::doAction((SoAction*)action);
  PRIVATE(this)->postAudioRender(this, action);
}

// Documented in superclass.
void
SoLOD::GLRender(SoGLRenderAction * action)
{
  switch (action->getCurPathCode()) {
  case SoAction::NO_PATH:
  case SoAction::BELOW_PATH:
    SoLOD::GLRenderBelowPath(action);
    break;
  case SoAction::IN_PATH:
    SoLOD::GLRenderInPath(action);
    break;
  case SoAction::OFF_PATH:
    SoLOD::GLRenderOffPath(action);
    break;
  default:
    assert(0 && "unknown path code.");
    break;
  }
}

// Documented in superclass.
void
SoLOD::GLRenderBelowPath(SoGLRenderAction * action)
{
  int idx = this->whichToTraverse(action);
  if (idx >= 0) {
    SoNode * child = (SoNode*) this->children->get(idx);
    action->pushCurPath(idx, child);
    if (!action->abortNow()) {
      SoNodeProfiling profiling;
      profiling.preTraversal(action);
      child->GLRenderBelowPath(action);
      profiling.postTraversal(action);
    }
    action->popCurPath();
  }
  // don't auto cache LOD nodes.
  SoGLCacheContextElement::shouldAutoCache(action->getState(),
                                           SoGLCacheContextElement::DONT_AUTO_CACHE);
}

// Documented in superclass.
void
SoLOD::GLRenderInPath(SoGLRenderAction * action)
{
  int numindices;
  const int * indices;
  SoAction::PathCode pathcode = action->getPathCode(numindices, indices);

  if (pathcode == SoAction::IN_PATH) {
    for (int i = 0; (i < numindices) && !action->hasTerminated(); i++) {
      int idx = indices[i];
      SoNode * node = this->getChild(idx);
      action->pushCurPath(idx, node);
      if (!action->abortNow()) {
        SoNodeProfiling profiling;
        profiling.preTraversal(action);
        node->GLRenderInPath(action);
        profiling.postTraversal(action);
      }
      action->popCurPath(pathcode);
    }
  }
  else {
    assert(pathcode == SoAction::BELOW_PATH);
    SoLOD::GLRenderBelowPath(action);
  }
}

// Documented in superclass.
void
SoLOD::GLRenderOffPath(SoGLRenderAction * action)
{
  int idx = this->whichToTraverse(action);;
  if (idx >= 0) {
    SoNode * node = this->getChild(idx);
    if (node->affectsState()) {
      action->pushCurPath(idx, node);
      if (!action->abortNow()) {
        SoNodeProfiling profiling;
        profiling.preTraversal(action);
        node->GLRenderOffPath(action);
        profiling.postTraversal(action);
      }
      action->popCurPath();
    }
  }
}

// Documented in superclass.
void
SoLOD::rayPick(SoRayPickAction *action)
{
  SoLOD::doAction((SoAction*)action);
}

// Documented in superclass.
void
SoLOD::getBoundingBox(SoGetBoundingBoxAction * action)
{
  // FIXME: SGI OIV seems to do some extra work here, but the manual
  // pages states that it should do a normal SoGroup traversal.
  // we should _not_ use whichToTraverse() to calculate bounding box as
  // this would cause cache dependencies on the camera and
  // the model matrix.                       pederb, 2001-02-21
  inherited::getBoundingBox(action);
}

// Documented in superclass.
void
SoLOD::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  SoLOD::doAction((SoAction*)action);
}

/*!
  Returns the child to traverse based on the ranges in
  SoLOD::range. Will clamp to index to the number of children.  This
  method will return -1 if no child should be traversed.  This will
  only happen if the node has no children though.
*/
int
SoLOD::whichToTraverse(SoAction *action)
{
  SoState *state = action->getState();
  const SbMatrix &mat = SoModelMatrixElement::get(state);
  const SbViewVolume &vv = SoViewVolumeElement::get(state);

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

// Doc from superclass.
void
SoLOD::notify(SoNotList * nl)
{
  inherited::notify(nl);
  PRIVATE(this)->notifyCalled();
}

#undef PRIVATE
#undef PUBLIC
