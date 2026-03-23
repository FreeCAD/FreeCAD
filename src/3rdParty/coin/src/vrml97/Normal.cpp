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
  \class SoVRMLNormal SoVRMLNormal.h Inventor/VRMLnodes/SoVRMLNormal.h
  \brief The SoVRMLNormal class is used to bind normals to geometry.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  Normal {
    exposedField MFVec3f vector  []   # (-,)
  }
  \endverbatim

  This node defines a set of 3D surface normal vectors to be used in
  the vector field of some geometry nodes (e.g., SoVRMLIndexedFaceSet
  and SoVRMLElevationGrid). This node contains one multiple-valued
  field that contains the normal vectors. Normals shall be of unit
  length.
*/

/*!
  \var SoMFVec3f SoVRMLNormal::vector
  The normal vectors. Empty by default.
*/

#include <Inventor/VRMLnodes/SoVRMLNormal.h>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/elements/SoGLVBOElement.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoVBO.h"

SO_NODE_SOURCE(SoVRMLNormal);

class SoVRMLNormalP {
 public:
  SoVRMLNormalP() : vbo(NULL) { }
  ~SoVRMLNormalP() { delete this->vbo; }
  
  SoVBO * vbo;
};

#define PRIVATE(obj) obj->pimpl

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLNormal::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLNormal, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLNormal::SoVRMLNormal(void)
{
  PRIVATE(this) = new SoVRMLNormalP;
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLNormal);

  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(vector);
}

/*!
  Destructor.
*/
SoVRMLNormal::~SoVRMLNormal()
{
  delete PRIVATE(this);
}

// Doc in parent
void
SoVRMLNormal::doAction(SoAction * action)
{
  SoNormalElement::set(action->getState(), this,
                       this->vector.getNum(), this->vector.getValues(0));
}

// Doc in parent
void
SoVRMLNormal::GLRender(SoGLRenderAction * action)
{
  //
  // FIXME: code to test if all normals are unit length, and store
  // this in some cached variable.  should be passed on to
  // SoGLNormalizeElement to optimize rendering (pederb)
  //
  SoVRMLNormal::doAction((SoAction*) action);
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
  if (setvbo) {
    SoGLVBOElement::setNormalVBO(state, PRIVATE(this)->vbo);
  }

}

// Doc in parent
void
SoVRMLNormal::callback(SoCallbackAction * action)
{
  SoVRMLNormal::doAction((SoAction*) action);
}

// Doc in parent
void
SoVRMLNormal::pick(SoPickAction * action)
{
  SoVRMLNormal::doAction((SoAction*) action);
}

// Doc in parent
void
SoVRMLNormal::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoVRMLNormal::doAction((SoAction*) action);
}

#undef PRIVATE
#endif // HAVE_VRML97
