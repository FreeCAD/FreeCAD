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
  \class SoNormal SoNormal.h Inventor/nodes/SoNormal.h
  \brief The SoNormal class is a node for providing normals to the state.

  \ingroup coin_nodes

  Coin will automatically calculate normals for you if no SoNormal
  nodes are present in the scene graph, but explicitly setting normals
  is useful for at least two purposes:
    1. a potential increase in performance
    2. you can calculate and use "incorrect" normals to do various special effects.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Normal {
        vector [  ]
    }
  \endcode

  \sa SoNormalBinding
*/

// *************************************************************************

#include <Inventor/nodes/SoNormal.h>

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoGLVBOElement.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoVBO.h"

// *************************************************************************

/*!
  \var SoMFVec3f SoNormal::vector
  Sets a pool of normal vectors in the traversal state.
*/

// *************************************************************************

class SoNormalP {
 public:
  SoNormalP() : vbo(NULL) { }
  ~SoNormalP() { delete this->vbo; }

  SoVBO * vbo;
};

#define PRIVATE(obj) obj->pimpl

SO_NODE_SOURCE(SoNormal);

/*!
  Constructor.
*/
SoNormal::SoNormal(void)
{
  PRIVATE(this) = new SoNormalP;
  SO_NODE_INTERNAL_CONSTRUCTOR(SoNormal);

  SO_NODE_ADD_FIELD(vector, (NULL));
}

/*!
  Destructor.
*/
SoNormal::~SoNormal()
{
  delete PRIVATE(this);
}

// Doc in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoNormal::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoNormal, SO_FROM_INVENTOR_1|SoNode::VRML1);

  SO_ENABLE(SoCallbackAction, SoNormalElement);
  SO_ENABLE(SoGLRenderAction, SoNormalElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoNormalElement);
  SO_ENABLE(SoPickAction, SoNormalElement);
}

// Doc in superclass.
void
SoNormal::GLRender(SoGLRenderAction * action)
{
  //
  // FIXME: code to test if all normals are unit length, and store
  // this in some cached variable.  should be passed on to
  // SoGLNormalizeElement to optimize rendering (pederb)
  //
  SoNormal::doAction(action);
  SoState * state = action->getState();
  
  SoBase::staticDataLock();
  SbBool setvbo = FALSE;
  const int num = this->vector.getNum();
  if (SoGLVBOElement::shouldCreateVBO(state, num)) {
    setvbo = TRUE;
    SbBool dirty = FALSE;
    if (PRIVATE(this)->vbo == NULL) {
      PRIVATE(this)->vbo = new SoVBO(GL_ARRAY_BUFFER, GL_STATIC_DRAW); 
      dirty =  TRUE;
    }
    else if (PRIVATE(this)->vbo->getBufferDataId() != this->getNodeId()) {
      dirty = TRUE;
    }
    if (dirty) {
      PRIVATE(this)->vbo->setBufferData(this->vector.getValues(0),
                                        num*sizeof(SbVec3f),
                                        this->getNodeId());
    }
  }
  else if (PRIVATE(this)->vbo && PRIVATE(this)->vbo->getBufferDataId()) {
    // clear buffers to deallocate VBO memory
    PRIVATE(this)->vbo->setBufferData(NULL, 0, 0);
  }
  SoBase::staticDataUnlock();
  SoGLVBOElement::setNormalVBO(state, setvbo? PRIVATE(this)->vbo : NULL);
}

// Doc in superclass.
void
SoNormal::doAction(SoAction * action)
{
  SoState * state = action->getState();
  if (!this->vector.isIgnored() &&
      !SoOverrideElement::getNormalVectorOverride(state)) {
    SoNormalElement::set(state, this,
                         this->vector.getNum(), this->vector.getValues(0));
    if (this->isOverride()) {
      SoOverrideElement::setNormalVectorOverride(state, this, TRUE);
    }
  }
}

// Doc in superclass.
void
SoNormal::callback(SoCallbackAction * action)
{
  SoNormal::doAction(action);
}

// Doc in superclass.
void
SoNormal::pick(SoPickAction * action)
{
  SoNormal::doAction(action);
}

// Doc in superclass.
void
SoNormal::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoNormal::doAction(action);
}

#undef PRIVATE
