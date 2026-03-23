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
  \class SoPathSwitch SoPathSwitch.h Inventor/nodes/SoPathSwitch.h
  \brief The SoPathSwitch class traverses only when current path matches a configurable path.

  \ingroup coin_nodes

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    PathSwitch {
        path NULL
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoPathSwitch.h>

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoAudioRenderAction.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFPath SoPathSwitch::path

  The path that must match the current action path. A \c NULL path
  will never match. An empty path will always match. The path should
  go up to (not including) the SoPathSwitch node, but need not go all
  the way back to the root node.
*/

// *************************************************************************

SO_NODE_SOURCE(SoPathSwitch);

// *************************************************************************

/*!
  Default constructor.
*/
SoPathSwitch::SoPathSwitch(void)
{
  this->commonConstructor();
}


static SbBool 
is_matching_paths(const SoPath * currentpath, const SoPath * pathswitchpath)
{
  if (pathswitchpath == NULL) return FALSE;

  const SoFullPath * current = (const SoFullPath*) currentpath;
  const SoFullPath * swpath = (const SoFullPath *) pathswitchpath;
  
  int swidx = swpath->getLength() - 1;
  if (swidx < 0) return TRUE; // an empty path will always match

  int curidx = current->getLength() - 2; // last node is this node. Skip it.
  
  // test if swpath is a valid path. Return FALSE if not.
  if (swidx > curidx) return FALSE;

  // we know curidx >= swidx
  while (swidx > 0) {
    if ((swpath->getNode(swidx) != current->getNode(curidx)) ||
        (swpath->getIndex(swidx) != current->getIndex(curidx))) break;
    swidx--;
    curidx--;
  }
  
  if (swidx == 0) { // don't test index for head node
    return swpath->getHead() == current->getNode(curidx);
  }
  return FALSE;
}


/*!
  Constructor.

  The argument should be the approximate number of children which is
  expected to be inserted below this node. The number need not be
  exact, as it is only used as a hint for better memory resource
  allocation.
*/
SoPathSwitch::SoPathSwitch(int numchildren)
  : inherited(numchildren)
{
  this->commonConstructor();
}

// private
void
SoPathSwitch::commonConstructor(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoPathSwitch);
  SO_NODE_ADD_FIELD(path, (NULL));
}

/*!
  Destructor.
*/
SoPathSwitch::~SoPathSwitch()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoPathSwitch::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoPathSwitch, SO_FROM_INVENTOR_1);
}

// doc in parent
void
SoPathSwitch::getBoundingBox(SoGetBoundingBoxAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::getBoundingBox(action);
  }
}

// doc in parent
void
SoPathSwitch::doAction(SoAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::doAction(action);
  }
}

// doc in parent
void
SoPathSwitch::GLRender(SoGLRenderAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::GLRender(action);
  }
}

// doc in parent
void
SoPathSwitch::pick(SoPickAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::pick(action);
  }
}

// doc in parent
void
SoPathSwitch::audioRender(SoAudioRenderAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::audioRender(action);
  }
}

// doc in parent
void
SoPathSwitch::handleEvent(SoHandleEventAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::handleEvent(action);
  }
}

// doc in parent
void
SoPathSwitch::getMatrix(SoGetMatrixAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::getMatrix(action);
  }
}

// doc in parent
void
SoPathSwitch::search(SoSearchAction * action)
{
  if (action->isSearchingAll()) inherited::search(action);
  else {
    SoNode::search(action);
    if (!action->isFound()) {
      SoPathSwitch::doAction(action);
    }
  }
}

// doc in parent
void
SoPathSwitch::callback(SoCallbackAction *action)
{
  if (action->isCallbackAll()) inherited::doAction(action);
  else SoPathSwitch::doAction(action);
}

// doc in parent
void
SoPathSwitch::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  if (is_matching_paths(action->getCurPath(), this->path.getValue())) {
    inherited::getPrimitiveCount(action);
  }
}
