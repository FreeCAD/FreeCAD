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
  \class SoVRMLSwitch SoVRMLSwitch.h Inventor/VRMLnodes/SoVRMLSwitch.h
  \brief The SoVRMLSwitch class is a group node for traversing selected children.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  Switch {
    exposedField    MFNode  choice      []
    exposedField    SFInt32 whichChoice -1    # [-1, inf)
  }
  \endverbatim

  The Switch grouping node traverses zero or one of the nodes
  specified in the choice field.  4.6.5, Grouping and children nodes
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.5>),
  describes details on the types of nodes that are legal values for
  choice.

  The \e whichChoice field specifies the index of the child to
  traverse, with the first child having index 0. If whichChoice is
  less than zero or greater than the number of nodes in the choice
  field, nothing is chosen.  All nodes under a Switch continue to
  receive and send events regardless of the value of whichChoice. For
  example, if an active SoVRMLTimeSensor is contained within an
  inactive choice of an Switch, the SoVRMLTimeSensor sends events
  regardless of the Switch's state.

*/

/*!
  \var SoMFNode SoVRMLSwitch::choice
  Contains the children.
*/

/*!
  \var SoSFInt32 SoVRMLSwitch::whichChoice

  Selected choice. Can be a positive number from 0 to &lt;numchildren-1&gt;,
  or one of the constants SO_SWITCH_NODE, SO_SWITCH_ALL or
  SO_SWITCH_INHERIT.  Default value is SO_SWITCH_NONE.

*/

#include <Inventor/VRMLnodes/SoVRMLSwitch.h>

#include <cstddef>

#include <Inventor/SoOutput.h>
#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/VRMLnodes/SoVRMLParent.h>
#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/elements/SoSoundElement.h>
#include <Inventor/elements/SoSwitchElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoChildList.h>
#ifdef HAVE_THREADS
#include <Inventor/threads/SbMutex.h>
#endif // HAVE_THREADS

#include "nodes/SoSubNodeP.h"
#include "nodes/SoSoundElementHelper.h"

// *************************************************************************

class SoVRMLSwitchP : public SoSoundElementHelper {
public:
  SbBool childlistvalid;

#ifdef COIN_THREADSAFE
  SbMutex childlistmutex;
  void lockChildList(void) { this->childlistmutex.lock(); }
  void unlockChildList(void) { this->childlistmutex.unlock(); }
#else // !COIN_THREADSAFE
  void lockChildList(void) { }
  void unlockChildList(void) { }
#endif // !COIN_THREADSAFE
};

#define PRIVATE(thisp) ((thisp)->pimpl)

// *************************************************************************

SO_NODE_SOURCE(SoVRMLSwitch);

// *************************************************************************

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLSwitch::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLSwitch, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLSwitch::SoVRMLSwitch(void)
{
  this->commonConstructor();
}

/*!
  Constructor. \a choices is the expected number of children.
*/
SoVRMLSwitch::SoVRMLSwitch(int choices)
  : SoGroup(choices)

{
  this->commonConstructor();
}

// common constructor
void
SoVRMLSwitch::commonConstructor(void)
{
  PRIVATE(this) = new SoVRMLSwitchP;
  PRIVATE(this)->childlistvalid = FALSE;

  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLSwitch);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(whichChoice, (SO_SWITCH_NONE));
  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(choice);

  // HACK WARNING: All children of this node are stored in the choice
  // field. Avoid double notifications (because of notification
  // through SoChildList) be reallocating the SoChildList with a
  // NULL-parent here. SoGroup will have allocated an SoChildList in
  // its constructor when we get here.
  delete this->SoGroup::children;
  this->SoGroup::children = new SoChildList(NULL);
}

/*!
  Destructor.
*/
SoVRMLSwitch::~SoVRMLSwitch(void)
{
  delete PRIVATE(this);
}

// Doc in parent
SbBool
SoVRMLSwitch::affectsState(void) const // virtual
{
  int idx = this->whichChoice.getValue();
  if (idx == SO_SWITCH_NONE) return FALSE;
  if (idx >= this->getNumChildren()) return FALSE;
  if (idx >= 0 && !this->getChild(idx)->affectsState()) return FALSE;

  // FIXME: cover SO_SWITCH_INHERIT and SO_SWITCH_ALL.
  return TRUE;
}

/*!
  Adds \a choice to the \a choice field.
*/
void
SoVRMLSwitch::addChoice(SoNode * choiceptr)
{
  this->addChild(choiceptr);
}

/*!
  Inserts \a choice at index \a idx.
*/
void
SoVRMLSwitch::insertChoice(SoNode * choiceptr,
                           int idx)
{
  this->insertChild(choiceptr, idx);
}

/*!
  Returns the choice at index \a idx.
*/
SoNode *
SoVRMLSwitch::getChoice(int idx) const
{
  return this->getChild(idx);
}

/*!
  Finds the index for \a choice, or -1 if not found.
*/
int
SoVRMLSwitch::findChoice(SoNode * choiceptr) const
{
  return this->findChild(choiceptr);
}

/*!
  Returns the number of choices.
*/
int
SoVRMLSwitch::getNumChoices(void) const
{
  return this->getNumChildren();
}

/*!
  Removes the choice at index \a idx.
*/
void
SoVRMLSwitch::removeChoice(int idx)
{
  this->removeChild(idx);
}

/*!
  If \a choice is found, remove it.
*/
void
SoVRMLSwitch::removeChoice(SoNode * choiceptr)
{
  this->removeChild(choiceptr);
}

/*!
  Removes all choices.
*/
void
SoVRMLSwitch::removeAllChoices(void)
{
  this->removeAllChildren();
}

/*!
  Replace the choice at index \a idx with \a choice.
*/
void
SoVRMLSwitch::replaceChoice(int idx,
                            SoNode * choiceptr)
{
  this->replaceChild(idx, choiceptr);
}

/*!
  Find \a old, and replace it with \a choice.
*/
void
SoVRMLSwitch::replaceChoice(SoNode * old,
                            SoNode * choiceptr)
{
  this->replaceChild(old, choiceptr);
}

// Doc in parent
void
SoVRMLSwitch::doAction(SoAction * action)
{
  SoState * state = action->getState();
  int idx = this->whichChoice.isIgnored() ?
    SO_SWITCH_NONE : this->whichChoice.getValue();
  if (idx == SO_SWITCH_INHERIT) {
    idx = SoSwitchElement::get(action->getState());
    // when we inherit, idx might be out of range. Use modulo.
    if (idx >= this->getNumChildren()) idx %= this->getNumChildren();
  }
  else {
    SoSwitchElement::set(state, idx);
  }

  int numindices;
  const int * indices;
  SoAction::PathCode pathcode = action->getPathCode(numindices, indices);

  if (idx == SO_SWITCH_ALL) {
    if (action->isOfType(SoGetBoundingBoxAction::getClassTypeId())) {
      // calculate center of bbox if bboxaction. This makes the
      // switch node behave exactly like a group node
      SoGetBoundingBoxAction * bbaction = (SoGetBoundingBoxAction*) action;
      // Initialize accumulation variables.
      SbVec3f acccenter(0.0f, 0.0f, 0.0f);
      int numcenters = 0;
      // only traverse nodes in path(s) for IN_PATH traversal
      int n = pathcode == SoAction::IN_PATH ? numindices : this->getNumChildren();

      for (int i = 0; i < n; i++) {
        this->getChildren()->traverse(bbaction,
                                      pathcode == SoAction::IN_PATH ? indices[i] : i);

        // If center point is set, accumulate.
        if (bbaction->isCenterSet()) {
          acccenter += bbaction->getCenter();
          numcenters++;
          bbaction->resetCenter();
        }
      }
      if (numcenters != 0) {
        bbaction->setCenter(acccenter / float(numcenters), FALSE);
      }
    }
    else { // not a getBoundingBoxAction
      if (pathcode == SoAction::IN_PATH) {
        this->getChildren()->traverseInPath(action, numindices, indices);
      }
      else {
        this->getChildren()->traverse(action);
      }
    }
  }
  else {
    if (idx >= 0) { // should only traverse one child
      if (pathcode == SoAction::IN_PATH) {
        // traverse only if one path matches idx
        for (int i = 0; i < numindices; i++) {
          if (indices[i] == idx) {
            this->getChildren()->traverse(action, idx);
            break;
          }
        }
      }
      else { // off, below or no path traversal
        // be robust for index out of range
        if (idx >= this->getNumChildren()) {
#if COIN_DEBUG
          SoDebugError::post("SoVRMLSwitch::doAction",
                             "whichChoice %d out of range (-1 - %d).",
                             idx, this->getNumChildren()-1);
#endif // COIN_DEBUG
        }
        else {
          this->getChildren()->traverse(action, idx);
        }
      }
    }
    PRIVATE(this)->traverseInactiveChildren(this, action, idx, pathcode,
                                            this->getNumChildren(), 
                                            this->getChildren());
  }
}

// Doc in parent
void
SoVRMLSwitch::callback(SoCallbackAction * action)
{
  SoVRMLSwitch::doAction((SoAction*)action);
}

// Doc in parent
void
SoVRMLSwitch::GLRender(SoGLRenderAction * action)
{
  SoVRMLSwitch::doAction(action);
}

// Doc in parent
void
SoVRMLSwitch::pick(SoPickAction * action)
{
  SoVRMLSwitch::doAction(action);
}

// Doc in parent
void
SoVRMLSwitch::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoVRMLSwitch::doAction(action);
}

// Doc in parent
void
SoVRMLSwitch::handleEvent(SoHandleEventAction * action)
{
  SoVRMLSwitch::doAction((SoAction*)action);
}

// Doc in parent
void
SoVRMLSwitch::audioRender(SoAudioRenderAction * action)
{
  PRIVATE(this)->preAudioRender(this, action);
  
  SoVRMLSwitch::doAction((SoAction*)action);

  PRIVATE(this)->postAudioRender(this, action);
}

// Doc in parent
void
SoVRMLSwitch::getMatrix(SoGetMatrixAction * action)
{
  switch (action->getCurPathCode()) {
  case SoAction::OFF_PATH:
  case SoAction::IN_PATH:
    SoVRMLSwitch::doAction((SoAction*)action);
    break;
  default:
    break;
  }
}

// Doc in parent
void
SoVRMLSwitch::search(SoSearchAction * action)
{
  // Include this node in the search.
  SoNode::search(action);
  if (action->isFound()) return;

  if (action->isSearchingAll()) {
    this->getChildren()->traverse(action);
  }
  else {
    SoVRMLSwitch::doAction(action);
  }
}

// Doc in parent
void
SoVRMLSwitch::write(SoWriteAction * action)
{
  // use SoNode::write, not SoGroup::write() as we only want to write
  // the children in the choice field, not as Group children.
  SoNode::write(action);
}

// Doc in parent
void
SoVRMLSwitch::addChild(SoNode * child)
{
  this->choice.addNode(child);
  PRIVATE(this)->childlistvalid = FALSE;
}

// Doc in parent
void
SoVRMLSwitch::insertChild(SoNode * child, int idx)
{
  this->choice.insertNode(child, idx);
  PRIVATE(this)->childlistvalid = FALSE;
}

// Doc in parent
SoNode *
SoVRMLSwitch::getChild(int idx) const
{
  return this->choice.getNode(idx);
}

// Doc in parent
int
SoVRMLSwitch::findChild(const SoNode * child) const
{
  return this->choice.findNode(child);
}

// Doc in parent
int
SoVRMLSwitch::getNumChildren(void) const // virtual
{
  return this->choice.getNumNodes();
}

// Doc in parent
void
SoVRMLSwitch::removeChild(int idx)
{
  this->choice.removeNode(idx);
  PRIVATE(this)->childlistvalid = FALSE;
}

// Doc in parent
void
SoVRMLSwitch::removeChild(SoNode * child)
{
  this->choice.removeNode(child);
  PRIVATE(this)->childlistvalid = FALSE;
}

// Doc in parent
void
SoVRMLSwitch::removeAllChildren(void)
{
  this->choice.removeAllNodes();
  SoGroup::children->truncate(0);
  PRIVATE(this)->childlistvalid = TRUE;
}

// Doc in parent
void
SoVRMLSwitch::replaceChild(int idx, SoNode * child)
{
  this->choice.replaceNode(idx, child);
  PRIVATE(this)->childlistvalid = FALSE;
}

// Doc in parent
void
SoVRMLSwitch::replaceChild(SoNode * old,
                           SoNode * child)
{
  this->choice.replaceNode(old, child);
  PRIVATE(this)->childlistvalid = FALSE;
}

// Doc in parent
void
SoVRMLSwitch::notify(SoNotList * list)
{
  SoField * f = list->getLastField();
  if (f == &this->choice) {
    PRIVATE(this)->childlistvalid = FALSE;
  }

  SoNotRec * rec = list->getLastRec();
  
  // If whichChild is set to a specific child and we get a
  // notification from some other child, ignore it to avoid redraws
  // and invalidated caches because of inactive parts of the scene
  // graph. This fixes cases like these:
  
  // DEF Switch {
  //   whichChild 1
  //   Separator {
  //     Rotor {
  //       on TRUE
  //       speed 1
  //     }
  //   }
  //   Separator {
  //     Cube { }
  //   }
  // }

  SbBool ignoreit = FALSE;

  if (rec && (f == &this->choice)) { // we got a notification from one of the children, check which
    int which = this->whichChoice.getValue();
    if (which == -1) ignoreit = TRUE; // also ignore if no children are traversed
    else if (which >= 0) {
      // get previous notrec to find the node the notification passed through
      rec = (SoNotRec*) rec->getPrevious();
      if (rec) {
        int fromchild = this->findChild((SoNode*) rec->getBase());
        if (fromchild >= 0 && fromchild != which) ignoreit = TRUE;
      }
    }
  }
  
  if (!ignoreit) {
    inherited::notify(list);
    PRIVATE(this)->notifyCalled();
  }
}


// Doc in parent
SbBool
SoVRMLSwitch::readInstance(SoInput * in,
                           unsigned short flags)
{
  SoGroup::children->truncate(0);
  SbBool oldnot = this->choice.enableNotify(FALSE);
  SbBool ret = inherited::readInstance(in, flags);
  if (oldnot) this->choice.enableNotify(TRUE);
  PRIVATE(this)->childlistvalid = FALSE;
  return ret;
}

// Doc in parent
void
SoVRMLSwitch::copyContents(const SoFieldContainer * from,
                           SbBool copyConn)
{
  SoGroup::children->truncate(0);
  SoNode::copyContents(from, copyConn);
  PRIVATE(this)->childlistvalid = FALSE;
}

// Doc in parent
SoChildList *
SoVRMLSwitch::getChildren(void) const
{
  if (!PRIVATE(this)->childlistvalid) {
    // this is not 100% thread safe. The assumption is that no nodes
    // will be added or removed while a scene graph is being
    // traversed. For Coin, this is an ok assumption.
    PRIVATE(this)->lockChildList();
    // test again after we've locked
    if (!PRIVATE(this)->childlistvalid) {
      SoVRMLParent::updateChildList(this->choice.getValues(0),
                                    this->choice.getNum(),
                                    *SoGroup::children);
      PRIVATE(this)->childlistvalid = TRUE;
    }
    PRIVATE(this)->unlockChildList();
  }
  return SoGroup::children;
}

#undef PRIVATE

#endif // HAVE_VRML97
