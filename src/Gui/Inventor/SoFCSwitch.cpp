/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#include <Inventor/misc/SoChildList.h>
#include <Inventor/actions/SoActions.h>
#include <Inventor/elements/SoSwitchElement.h>

#include "../InventorBase.h"
#include "../ViewParams.h"

#include "SoFCSwitch.h"
#include "SoFCDisplayModeElement.h"

using namespace Gui;

SO_NODE_SOURCE(SoFCSwitch)

SoFCSwitch::SoFCSwitch()
{
  SO_NODE_CONSTRUCTOR(SoFCSwitch);
  SO_NODE_ADD_FIELD(defaultChild,  (0));
  SO_NODE_ADD_FIELD(tailChild,  (-1));
  SO_NODE_ADD_FIELD(headChild,  (-1));
  SO_NODE_ADD_FIELD(childNotify, (0));
  SO_NODE_ADD_FIELD(overrideSwitch,(OverrideNone));
  SO_NODE_ADD_FIELD(childNames,(""));
  SO_NODE_ADD_FIELD(allowNamedOverride,(TRUE));
  SO_NODE_DEFINE_ENUM_VALUE(OverrideSwitch, OverrideNone);
  SO_NODE_DEFINE_ENUM_VALUE(OverrideSwitch, OverrideDefault);
  SO_NODE_DEFINE_ENUM_VALUE(OverrideSwitch, OverrideVisible);
  SO_NODE_DEFINE_ENUM_VALUE(OverrideSwitch, OverrideReset);
  SO_NODE_SET_SF_ENUM_TYPE(overrideSwitch, OverrideSwitch);

  SO_ENABLE(SoGLRenderAction, SoSwitchElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoSwitchElement);
}

// switch to defaultChild when invisible
#define FC_SWITCH_DEFAULT (0x10000000)
#define FC_SWITCH_VISIBLE (0x20000000)
#define FC_SWITCH_RESET   (0x30000000)
#define FC_SWITCH_MASK    (0xF0000000)

void
SoFCSwitch::setOverrideSwitch(SoState * state, bool enable)
{
  SoSwitchElement::set(state, enable ? FC_SWITCH_DEFAULT : -1);
}

void
SoFCSwitch::switchOverride(SoAction *action, OverrideSwitch o) {
  if(action) {
    int which;
    switch(o) {
      case OverrideDefault:
        which = FC_SWITCH_DEFAULT;
        break;
      case OverrideVisible:
        which = FC_SWITCH_VISIBLE;
        break;
      default:
        which = SO_SWITCH_NONE;
    }
    SoSwitchElement::set(action->getState(),which);
  }
}

struct SwitchInfo {
  CoinPtr<SoPath> path;
  int idx;

  SwitchInfo(SoPath *p)
    :path(p),idx(-1)
  {
    if(next()<0)
      path.reset();
  }

  int next() {
    if(!path)
      return -1;
    int count = path->getLength();
    if(idx>=count)
      return -1;
    for(++idx;idx<count;++idx) {
      if(path->getNode(idx)->isOfType(SoFCSwitch::getClassTypeId()))
        break;
    }
    return idx<count?idx:-1;
  }
};

static FC_COIN_THREAD_LOCAL std::vector<SwitchInfo> _SwitchStack;
static FC_COIN_THREAD_LOCAL std::vector<SoFCSwitch::TraverseState> _SwitchTraverseStack;

void
SoFCSwitch::pushSwitchPath(SoPath *path)
{
  _SwitchStack.emplace_back(path);
}

void
SoFCSwitch::popSwitchPath()
{
  _SwitchStack.pop_back();
}

bool
SoFCSwitch::testTraverseState(TraverseStateFlag flag)
{
  if(_SwitchTraverseStack.size())
    return _SwitchTraverseStack.back().test((std::size_t)flag);
  return false;
}

void
SoFCSwitch::doAction(SoAction *action)
{
  auto state = action->getState();

  uint32_t mask = ((uint32_t)SoSwitchElement::get(state)) & FC_SWITCH_MASK;
  int idx = -1;

  switch(overrideSwitch.getValue()) {
    case OverrideDefault:
      if(mask != FC_SWITCH_VISIBLE)
        mask = FC_SWITCH_DEFAULT;
      break;
    default:
      break;
  }

  if((mask!=FC_SWITCH_DEFAULT && mask!=FC_SWITCH_VISIBLE)
      || (action->isOfType(SoCallbackAction::getClassTypeId()) &&
        ((SoCallbackAction *)action)->isCallbackAll()))
  {
    const SbName &name = SoFCDisplayModeElement::get(state);

    if(this->whichChild.getValue()>=0 
        && this->allowNamedOverride.getValue()
        && name!=SbName::empty())
    {
      for(int i=0, c=std::min(childNames.getNum(),this->getNumChildren()); i<c; ++i) {
        if(childNames[i] == name) {
          traverseHead(action, i);
          traverseChild(action, i);
          traverseTail(action, i);
          return;
        }
      }
    }
    traverseHead(action, whichChild.getValue());
    inherited::doAction(action);
    traverseTail(action, whichChild.getValue());
    return;
  }

  int numindices = 0;
  const int * indices = 0;
  SoAction::PathCode pathcode = action->getPathCode(numindices, indices);

  if(_SwitchStack.size() && _SwitchStack.back().path) {
    auto &info = _SwitchStack.back();
    if(info.path->getNode(info.idx) == this) {
      // We are traversing inside a path from some parent SoFCPathAnnotation.
      // We shall override the switch index according to the path inside
      if(info.idx+1<info.path->getLength()) 
        idx = info.path->getIndex(info.idx+1);

      int nodeIdx = info.idx;
      if(info.next()<0) {
        if(nodeIdx+1==info.path->getLength())
          idx = this->defaultChild.getValue();
        // We are the last SoFCSwitch node inside the path, reset the
        // path so we do not override visibility below. We will still
        // override switch if the node is visible.
        info.path.reset();
      }
    }
  } else if (numindices == 1) {
    // this means we applying the action to a path, and we are traversing in the middle of it.
    idx = indices[0];
  } else if (action->getWhatAppliedTo() == SoAction::PATH) {
    auto path = action->getPathAppliedTo();
    if(path && path->getLength() && path->getNodeFromTail(0) == this)
      idx = this->defaultChild.getValue();
  }

  if(idx<0 && idx!=SO_SWITCH_ALL) {
    if((mask==FC_SWITCH_VISIBLE || whichChild.getValue()!=SO_SWITCH_NONE)
        && this->defaultChild.getValue()!=SO_SWITCH_NONE)
    {
      idx = this->defaultChild.getValue();
    } else
      idx = this->whichChild.getValue();
  }

  if(idx!=SO_SWITCH_ALL && (idx<0 || idx>=this->getNumChildren())) {
    traverseHead(action,whichChild.getValue());
    inherited::doAction(action);
    traverseTail(action,whichChild.getValue());
    return;
  }

  switch(overrideSwitch.getValue()) {
    case OverrideVisible:
      // OverrideVisible is only applicable to children
      mask = FC_SWITCH_VISIBLE;
      break;
    case OverrideReset:
      if(_SwitchStack.empty() || !_SwitchStack.back().path)
        mask = FC_SWITCH_RESET;
      break;
    default:
      break;
  }
  uint32_t uidx = (uint32_t)idx;
  SoSwitchElement::set(state, (int32_t)(mask|(uidx & ~FC_SWITCH_MASK)));

  TraverseState tstate(0);
  if(_SwitchTraverseStack.size()) {
    tstate = _SwitchTraverseStack.back();
    tstate.reset(TraverseAlternative);
  } else
    tstate.set(TraverseOverride);

  if(whichChild.getValue() == SO_SWITCH_NONE)
    tstate.set(TraverseInvisible);
  else if(whichChild.getValue()!=idx) 
    tstate.set(TraverseAlternative);

  if(!_SwitchTraverseStack.size() || _SwitchTraverseStack.back()!=tstate)
    _SwitchTraverseStack.push_back(tstate);
  else
    tstate.reset();

  if(idx == SO_SWITCH_ALL) {
    if (pathcode == SoAction::IN_PATH)
      this->children->traverseInPath(action, numindices, indices);
    else
      this->children->traverse(action);
  } else if (pathcode == SoAction::IN_PATH) {
    // traverse only if one path matches idx
    for (int i = 0; i < numindices; i++) {
      if (indices[i] == idx) {
        this->children->traverse(action, idx);
        break;
      }
    }
  } else {
    traverseHead(action,idx);
    this->children->traverse(action, idx);
    traverseTail(action,idx);
  }

  if(tstate.to_ulong())
    _SwitchTraverseStack.pop_back();
}

void
SoFCSwitch::notify(SoNotList * nl)
{
  // SoSwitch ignores child change other than whichChild. But we shall
  // include tailChild and defaultChild as well.

  SoNotRec * rec = nl->getLastRec();
  SbBool ignoreit = FALSE;

  // if getBase() == this, the notification is from a field under this
  // node, and should _not_ be ignored
  if (rec && (rec->getBase() != (SoBase*) this)) {
    int which = this->whichChild.getValue();
    if(which!=SO_SWITCH_ALL) {
      int fromchild = this->findChild((SoNode*) rec->getBase());
      if (fromchild >= 0
          && fromchild!=which 
          && childNotify.getValue()<=0)
      {
        ignoreit = TRUE;
      }
    }
  }
  if (!ignoreit)
    SoGroup::notify(nl);
}

void SoFCSwitch::traverseHead(SoAction *action, int idx)
{
  int head = headChild.getValue();
  if(idx<0 || head<0 || idx==head || head>=getNumChildren())
    return;

  traverseChild(action, head);
}

void
SoFCSwitch::traverseTail(SoAction *action, int idx)
{
  int tail = tailChild.getValue();
  if(idx<0 || tail<0 || idx==tail || tail>=getNumChildren())
    return;

  traverseChild(action, tail);
}

void
SoFCSwitch::traverseChild(SoAction *action, int idx)
{
  SoSwitchElement::set(action->getState(), idx);
  int numindices = 0;
  const int * indices = 0;
  SoAction::PathCode pathcode = action->getPathCode(numindices, indices);
  if(pathcode != SoAction::IN_PATH) {
    this->children->traverse(action,idx);
    return;
  }

  // If traverse in path, traverse only if idx is in the path
  for (int i = 0; i < numindices; i++) {
    if (indices[i] == idx) {
      this->children->traverse(action, idx);
      break;
    }
  }
}

void
SoFCSwitch::getBoundingBox(SoGetBoundingBoxAction * action)
{
  doAction(action);
}

void
SoFCSwitch::search(SoSearchAction * action)
{
  SoNode::search(action);
  if (action->isFound()) return;

  if (action->isSearchingAll()) {
    this->children->traverse(action);
  }
  else {
    doAction(action);
  }
}

void
SoFCSwitch::callback(SoCallbackAction *action)
{
  doAction(action);
}

void
SoFCSwitch::pick(SoPickAction *action)
{
  doAction((SoAction*)action);
}

void
SoFCSwitch::handleEvent(SoHandleEventAction *action)
{
  doAction(action);
}

void
SoFCSwitch::initClass(void)
{
  SO_NODE_INIT_CLASS(SoFCSwitch,SoSwitch,"FCSwitch");
}

void
SoFCSwitch::finish()
{
  atexit_cleanup();
}

// vim: noai:ts=2:sw=2
