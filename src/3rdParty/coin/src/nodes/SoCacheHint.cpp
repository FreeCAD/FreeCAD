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
  \class SoCacheHint SoCacheHint.h Inventor/nodes/SoCacheHint.h
  \brief The SoCacheHint class is a node containing hints about how to cache geometry.

  \ingroup coin_nodes

  The SoCacheHint node is used to set up clues to the rendering
  subsystem about how Coin should cache vertex data.

  Please note that this is an experimental class. The API might change
  a lot before/if it is included in any official Coin release.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    CacheHint {
        memValue 0.5
        gfxValue 0.5
    }
  \endcode
*/

/*!
  \var SoSFFloat SoCacheHint::memValue

  Sets the value for main memory usage. Should be a number between 0
  and 1. A higher value will use more memory for caching.  Default
  value is 0.5

*/

/*!
  \var SoSFFloat SoCacheHint::gfxValue

  Sets the value for gfx memory usage. Should be a number between 0
  and 1. A higher value will use more memory for caching.  Default
  value is 0.5

*/

#include <Inventor/nodes/SoCacheHint.h>

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoCacheHintElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

SO_NODE_SOURCE(SoCacheHint);

/*!
  Constructor.
*/
SoCacheHint::SoCacheHint(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoCacheHint);
  SO_NODE_ADD_FIELD(memValue, (0.5f));
  SO_NODE_ADD_FIELD(gfxValue, (0.5f));
}

/*!
  Destructor.
*/
SoCacheHint::~SoCacheHint()
{
}

// doc in super
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoCacheHint::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoCacheHint, SO_FROM_COIN_2_4);
  
  SO_ENABLE(SoGLRenderAction, SoCacheHintElement);
}

void
SoCacheHint::doAction(SoAction * action)
{
  SoState * state = action->getState();
  SoCacheHintElement::set(state, this, 
                          this->memValue.getValue(),
                          this->gfxValue.getValue());
}

void
SoCacheHint::GLRender(SoGLRenderAction * action)
{
  SoCacheHint::doAction(action);
}

void
SoCacheHint::callback(SoCallbackAction * action)
{
  // do nothing
  SoNode::callback(action);
}

void
SoCacheHint::pick(SoPickAction * action)
{
  // do nothing
  SoNode::pick(action);
}

void
SoCacheHint::getBoundingBox(SoGetBoundingBoxAction * action)
{
  // do nothing
  SoNode::getBoundingBox(action);
}
