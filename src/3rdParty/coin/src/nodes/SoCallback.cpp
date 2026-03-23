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
  \class SoCallback SoCallback.h Inventor/nodes/SoCallback.h
  \brief The SoCallback class is a node type which provides a means of setting callback hooks in the scene graph.

  \ingroup coin_nodes

  By inserting SoCallback nodes in a scene graph, the application
  programmer can set up functions to be executed at certain points in
  the traversal.

  The callback function will be executed during traversal of \e any
  action, so check the type of the \a action argument of the callback
  function if you only want to run your code at specific actions.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Callback {
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoCallback.h>

#include <Inventor/actions/SoActions.h> // SoCallback uses all of them.

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \typedef void SoCallbackCB(void * userdata, SoAction * action)
  Signature that callback functions need to have.
*/

// *************************************************************************

SO_NODE_SOURCE(SoCallback);

// *************************************************************************

/*!
  Constructor.
*/
SoCallback::SoCallback(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoCallback);

  this->cbfunc = NULL;
}

/*!
  Destructor.
*/
SoCallback::~SoCallback()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoCallback::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoCallback, SO_FROM_INVENTOR_1);
}

/*!
  Set up the \a function to call at traversal of this node. \a
  userdata will be passed back as the first argument of the callback
  \a function. Setting \a function to NULL removes the previously set 
  callback function.

  If you want a callback only for a specific action, you must (in your
  callback function) remember to check which action invoked the
  callback, for instance like this:

  \code
  
  void mycallback(void * userdata, SoAction * action)
  {
    if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
      // do stuff specific for GL rendering here.
    }
  }

  \endcode

  Please note that SoCallback nodes under a Separator might be
  included in a cache. Cached nodes are not traversed, and you'll not
  receive any callbacks. If you want to make sure that your callback
  is called every time the scene graph is rendered, you must
  invalidate the render cache in your callback:

  \code
  
  void mycallback(void * userdata, SoAction * action)
  {
    if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
      SoCacheElement::invalidate(action->getState());
    }
  }

  \endcode

  If you want to invalidate all caches (for instance also the bounding
  box cache), you can do this in your callback:

  \code
  
  void mycallback(void * userdata, SoAction * action)
  {
    SoState * state = action->getState();
    if (state->isElementEnabled(SoCacheElement::getClassStackIndex())) {
      SoCacheElement::invalidate(state);
    }
  }

  \endcode
*/
void
SoCallback::setCallback(SoCallbackCB * function, void * userdata)
{
  this->cbfunc = function;
  this->cbdata = userdata;
}

// Doc from superclass.
void
SoCallback::doAction(SoAction * action)
{
  if (this->cbfunc) this->cbfunc(this->cbdata, action);
}

// Doc from superclass.
void
SoCallback::callback(SoCallbackAction * action)
{
  SoCallback::doAction(action);
}

// Doc from superclass.
void
SoCallback::GLRender(SoGLRenderAction * action)
{
  // FIXME: we've had a user report which indicates that SGI Inventor
  // may also cache OpenGL calls made from within a callback into
  // renderlists. Investigate, and consider whether or not we should
  // follow suit. 20051110 mortene.

  SoCallback::doAction(action);
}

// Doc from superclass.
void
SoCallback::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoCallback::doAction(action);
}

// Doc from superclass.
void
SoCallback::getMatrix(SoGetMatrixAction * action)
{
  SoCallback::doAction(action);
}

// Doc from superclass.
void
SoCallback::handleEvent(SoHandleEventAction * action)
{
  SoCallback::doAction(action);
}

// Doc from superclass.
void
SoCallback::pick(SoPickAction * action)
{
  SoCallback::doAction(action);
}

// Doc from superclass.
void
SoCallback::search(SoSearchAction * action)
{
  SoCallback::doAction(action);
  SoNode::search(action);
}

// Doc from superclass.
void
SoCallback::write(SoWriteAction * action)
{
  SoCallback::doAction(action);
  SoNode::write(action);
}

// Doc from superclass.
void
SoCallback::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoCallback::doAction(action);
}

// Doc from superclass. Overridden from parent class to also copy the
// callback function pointer and userdata.
void
SoCallback::copyContents(const SoFieldContainer * from, SbBool copyconnections)
{
  inherited::copyContents(from, copyconnections);

  SoCallback * fromnode = (SoCallback *)from;
  this->cbfunc = fromnode->cbfunc;
  this->cbdata = fromnode->cbdata;
}
