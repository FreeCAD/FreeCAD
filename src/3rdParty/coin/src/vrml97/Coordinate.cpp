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
  \class SoVRMLCoordinate SoVRMLCoordinate.h Inventor/VRMLnodes/SoVRMLCoordinate.h
  \brief The SoVRMLCoordinate class is used to define 3D coordinates for shapes.

  \ingroup coin_VRMLnodes
  
  \WEB3DCOPYRIGHT

  \verbatim
  Coordinate { 
    exposedField MFVec3f point  []      # (-inf, inf) 
  }
  \endverbatim
 
  This node defines a set of 3D coordinates to be used in the coord
  field of vertex-based geometry nodes including SoVRMLIndexedFaceSet,
  SoVRMLIndexedLineSet, and SoVRMLPointSet.

*/

/*!
  \var SoMFVec3f SoVRMLCoordinate::point
  The coordinates. Empty by default.
*/

#include <Inventor/VRMLnodes/SoVRMLCoordinate.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/actions/SoAction.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoVBO.h"

SO_NODE_SOURCE(SoVRMLCoordinate);

#define PRIVATE(obj) obj->pimpl

class SoVRMLCoordinateP {
 public:
  SoVRMLCoordinateP() : vbo(NULL) { }
  ~SoVRMLCoordinateP() { delete this->vbo; }
  SoVBO * vbo;
};

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLCoordinate::initClass(void) // static
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLCoordinate, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLCoordinate::SoVRMLCoordinate(void)
{
  PRIVATE(this) = new SoVRMLCoordinateP;
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLCoordinate);

  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(point);
}

/*!
  Destructor.
*/
SoVRMLCoordinate::~SoVRMLCoordinate()
{
  delete PRIVATE(this);
}

// Doc in parent
void
SoVRMLCoordinate::doAction(SoAction * action)
{
  SoCoordinateElement::set3(action->getState(), this,
                            point.getNum(), point.getValues(0));
}

// Doc in parent
void
SoVRMLCoordinate::GLRender(SoGLRenderAction * action)
{
  SoVRMLCoordinate::doAction((SoAction*) action);
  
  SoState * state = action->getState();
  const int num = this->point.getNum();
  SbBool setvbo = FALSE;
  SoBase::staticDataLock();
  if (SoGLVBOElement::shouldCreateVBO(state, num)) {
    SbBool dirty = FALSE;
    setvbo = TRUE;
    if (PRIVATE(this)->vbo == NULL) {
      PRIVATE(this)->vbo = new SoVBO(GL_ARRAY_BUFFER, GL_STATIC_DRAW); 
      dirty =  TRUE;
    }
    else if (PRIVATE(this)->vbo->getBufferDataId() != this->getNodeId()) {
      dirty = TRUE;
    }
    if (dirty) {
      PRIVATE(this)->vbo->setBufferData(this->point.getValues(0),
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
    SoGLVBOElement::setVertexVBO(state, PRIVATE(this)->vbo);
  }

}

// Doc in parent
void
SoVRMLCoordinate::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoVRMLCoordinate::doAction((SoAction*) action);
}

// Doc in parent
void
SoVRMLCoordinate::callback(SoCallbackAction * action)
{
  SoVRMLCoordinate::doAction((SoAction*) action);
}

// Doc in parent
void
SoVRMLCoordinate::pick(SoPickAction * action)
{
  SoVRMLCoordinate::doAction((SoAction*) action);
}

#undef PRIVATE
#endif // HAVE_VRML97
