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
  \class SoSwitch SoSwitch.h Inventor/nodes/SoSwitch.h
  \brief The SoSwitch class is a group node which selects one child subgraph for traversal.

  \ingroup coin_nodes

  Which child to traverse is controlled by the application programmer
  by using the SoSwitch::whichChild field. In addition to picking out
  a single child for traversal, it is also possible to flip all
  children on or off for traversal.

  This node is very useful for conditionally turning on or off parts
  of the scene graph based on the current application processing mode,
  visualizing mode, or whatever else the application can do.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Switch {
        whichChild -1
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoSwitch.h>

#include <Inventor/misc/SoChildList.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/elements/SoSoundElement.h>
#include <Inventor/elements/SoSwitchElement.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoDebugError.h>

#include "nodes/SoSubNodeP.h"
#include "coindefs.h" // COIN_OBSOLETED()
#include "io/SoWriterefCounter.h"

// *************************************************************************

/*!
  \var SoSFInt32 SoSwitch::whichChild

  Selects which child to traverse during rendering (and some other)
  actions.

  The value should be either \c SO_SWITCH_NONE (for traversing no
  children, like it was an empty SoGroup node), \c SO_SWITCH_ALL (for
  traversing like if we were an SoGroup node), \c SO_SWITCH_INHERIT
  (for traversing the same child as the last SoSwitch node), or an
  index value for a child.

  When using \c SO_SWITCH_INHERIT, it is important to understand how
  the SoSwitch-node is affected by other SoSwitch-nodes. If you have
  several switches in the scene graph, the last switch with its \c
  whichChild field set to anything but \c SO_SWITCH_INHERIT will be
  used. The switch does not only inherit from its parent switch node,
  but also from its siblings, located anywhere before it in the
  scene graph. An example will help clarify this:

  \code
  #Inventor V2.1 ascii

  Separator {
    Switch {
      whichChild 0
  
      Group {
        Switch {
          whichChild 1
          BaseColor { rgb 1 0 0 } # red
          BaseColor { rgb 1 1 0 } # yellow
        }
        Switch {
          whichChild -2 # SO_SWITCH_INHERIT
          BaseColor { rgb 0 1 0 } # green
          BaseColor { rgb 0 0 1 } # blue
        }
        Cube { }
      }
    }
  }
  \endcode

  This results in a blue cube on the screen. The reason being that the
  value of the previous \c whichChild field was inherited by the final
  switch, making it select child 1 - the blue BaseColor.

  When constructing ASCII Inventor files, the integer values for the
  keywords must be used instead of their names.  They are -1 for
  \c SO_SWITCH_NONE, -2 for \c SO_SWITCH_INHERIT, and -3 for
  \c SO_SWITCH_ALL.

  Default value for the field is \c SO_SWITCH_NONE.
*/

// *************************************************************************


#include "SoSoundElementHelper.h"

class SoSwitchP : public SoSoundElementHelper
{
public:
  SoSwitchP(SoSwitch * master) : master(master) {};
  SoSwitch *master;
};

#define PRIVATE(p) ((p)->pimpl)
#define PUBLIC(p) ((p)->master)

SO_NODE_SOURCE(SoSwitch);

/*!
  Default constructor.
*/
SoSwitch::SoSwitch(void)
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
SoSwitch::SoSwitch(int numchildren)
  : inherited(numchildren)
{
  this->commonConstructor();
}

void
SoSwitch::commonConstructor(void)
{
  PRIVATE(this) = new SoSwitchP(this);

  SO_NODE_INTERNAL_CONSTRUCTOR(SoSwitch);
  SO_NODE_ADD_FIELD(whichChild, (SO_SWITCH_NONE));
}

/*!
  Destructor.
*/
SoSwitch::~SoSwitch()
{
  delete PRIVATE(this);
}

// doc in super
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoSwitch::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoSwitch, SO_FROM_INVENTOR_1|SoNode::VRML1);

  SO_ENABLE(SoGetBoundingBoxAction, SoSwitchElement);
  SO_ENABLE(SoSearchAction, SoSwitchElement);
  SO_ENABLE(SoGetMatrixAction, SoSwitchElement);
  SO_ENABLE(SoGLRenderAction, SoSwitchElement);
  SO_ENABLE(SoPickAction, SoSwitchElement);

  SO_ENABLE(SoCallbackAction, SoSwitchElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoSwitchElement);
  SO_ENABLE(SoHandleEventAction, SoSwitchElement);
}

// Documented in superclass.
void
SoSwitch::GLRender(SoGLRenderAction * action)
{
  this->doAction(action);
}

// Documented in superclass.
void
SoSwitch::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoSwitch::doAction(action);
}

// Documented in superclass.
void
SoSwitch::search(SoSearchAction * action)
{
  // This method must be overridden in SoSwitch nodes to take into
  // account if the search involves every single node, or just the
  // nodes involved in normal graph traversal.

  // Include this node in the search.
  SoNode::search(action);
  if (action->isFound()) return;

  if (action->isSearchingAll()) {
    this->children->traverse(action);
  }
  else {
    SoSwitch::doAction(action);
  }
}

// Documented in superclass.
void
SoSwitch::doAction(SoAction * action)
{
  SoState * state = action->getState();
  int idx = this->whichChild.isIgnored() ?
    SO_SWITCH_NONE : this->whichChild.getValue();
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

  if (idx == SO_SWITCH_ALL ||
      (action->isOfType(SoCallbackAction::getClassTypeId()) &&
       ((SoCallbackAction *)action)->isCallbackAll())) {
    if (action->isOfType(SoGetBoundingBoxAction::getClassTypeId())) {
      SoGroup::getBoundingBox((SoGetBoundingBoxAction*) action);
    }
    else { // not a getBoundingBoxAction
      if (pathcode == SoAction::IN_PATH) {
        this->children->traverseInPath(action, numindices, indices);
      }
      else {
        this->children->traverse(action);
      }
    }
  } 
  else {
    if (idx >= 0) { // should only traverse one child
      if (pathcode == SoAction::IN_PATH) {
        // traverse only if one path matches idx
        for (int i = 0; i < numindices; i++) {
          if (indices[i] == idx) {
            this->children->traverse(action, idx);
            break;
          }
        }
      }
      else { // off, below or no path traversal
        // be robust for index out of range
        if (idx >= this->getNumChildren()) {
#if COIN_DEBUG
          static SbBool first = TRUE;
          if (first) {
            first = FALSE;
            SbString s("(warning will be printed once, but there might be more cases of this problem).");
            int lastidx = this->getNumChildren()-1;
            if (lastidx >= 0) {
              SoDebugError::post("SoSwitch::doAction",
                                 "whichChild %d out of range [0, %d] %s",
                                 idx, lastidx, s.getString());
            }
            else {
              SoDebugError::post("SoSwitch::doAction",
                                 "whichChild %d out of range -- "
                                 "switch node has no children! %s",
                                 idx, s.getString());
            }
          }
#endif // COIN_DEBUG
        }
        else {
          this->children->traverse(action, idx);
        }
      }
    }
    PRIVATE(this)->traverseInactiveChildren(this, action, idx, pathcode,
                                            this->getNumChildren(), 
                                            this->getChildren());
  }
}

// Documented in superclass.
SbBool
SoSwitch::affectsState(void) const
{
  // Overridden because when this function is called we don't know
  // which "mode" the traversing action is in. If it is an
  // SoSearchAction with isSearchingAll() set to TRUE, we should
  // behave as if whichChild == SO_SWITCH_ALL, for instance.
  //
  // (To handle this exact case, SGI and TGS Inventor seems to use a
  // global static flag SoSearchAction::duringSearchAll. We find this
  // to be an utterly crap idea, though.)
  //
  // So to be safe, we _always_ behave as if whichChild is set to
  // traverse all children. The worst that can happen is that we get a
  // "false positive", i.e. TRUE when it should be FALSE. That means the
  // action needs to traverse one level further down onto one of our
  // children -- which will just take a minuscule amount of additional
  // processing time.

  int n = this->getNumChildren();
  for (int i=0; i < n; i++) {
    if (this->getChild(i)->affectsState()) { return TRUE; }
  }
  return FALSE;
}

// Documented in superclass.
void
SoSwitch::callback(SoCallbackAction *action)
{
  SoSwitch::doAction(action);
}

// Documented in superclass.
void
SoSwitch::audioRender(SoAudioRenderAction * action)
{
  PRIVATE(this)->preAudioRender(this, action);

  SoSwitch::doAction((SoAction*)action);

  PRIVATE(this)->postAudioRender(this, action);
}

// Documented in superclass.
void
SoSwitch::pick(SoPickAction *action)
{
  SoSwitch::doAction((SoAction*)action);
}

// Documented in superclass.
void
SoSwitch::handleEvent(SoHandleEventAction *action)
{
  SoSwitch::doAction(action);
}

// Documented in superclass.
void
SoSwitch::getMatrix(SoGetMatrixAction *action)
{
  switch (action->getCurPathCode()) {
  case SoAction::OFF_PATH:
  case SoAction::IN_PATH:
    SoSwitch::doAction((SoAction*)action);
    break;
  default:
    break;
  }
}

// Documented in superclass.
void
SoSwitch::write(SoWriteAction * action)
{
  // to keep child numbering, always write out all children for a
  // switch

  SoOutput * out = action->getOutput();
  if (out->getStage() == SoOutput::COUNT_REFS) {
    this->addWriteReference(out, FALSE);
    // No need to traverse children as SoGroup::addWriteReference() already
    // handles write references of children.
  }
  else if (out->getStage() == SoOutput::WRITE) {
    if (this->writeHeader(out, TRUE, FALSE)) return;
    this->getFieldData()->write(out, this);
    if (out->isBinary()) out->write(this->getNumChildren());
    this->getChildren()->traverse(action);
    this->writeFooter(out);
  }
  else assert(0 && "unknown stage");
}

// Documented in superclass.
void
SoSwitch::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  SoSwitch::doAction((SoAction*)action);
}

/*!
  This function was part of the original SGI Inventor API, but it is
  not supported in Coin, as it looks like it should probably have been
  private in Inventor.
*/
void
SoSwitch::traverseChildren(SoAction * COIN_UNUSED_ARG(action))
{
  COIN_OBSOLETED();
}

// Doc from superclass.
void
SoSwitch::notify(SoNotList * nl)
{
  SoNotRec * rec = nl->getLastRec();
  
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

  // if getBase() == this, the notification is from a field under this
  // node, and should _not_ be ignored
  if (rec && (rec->getBase() != (SoBase*) this)) {
    int which = this->whichChild.getValue();
    if (which == -1) ignoreit = TRUE; // also ignore if no children are traversed
    else if (which >= 0) {
      int fromchild = this->findChild((SoNode*) rec->getBase());
      if (fromchild >= 0 && fromchild != which) ignoreit = TRUE;
    }
  }
  
  if (!ignoreit) {
    inherited::notify(nl);
    PRIVATE(this)->notifyCalled();
  }
}

#undef PRIVATE
#undef PUBLIC
