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
  \class SoCoordinate3 SoCoordinate3.h Inventor/nodes/SoCoordinate3.h
  \brief The SoCoordinate3 class is a node for providing coordinates to shape nodes.

  \ingroup coin_nodes

  When encountering nodes of this type during traversal, the
  coordinates it contains will be put on the state stack for later use
  by shape nodes of types which need coordinate sets (like SoFaceSet
  nodes or SoPointSet nodes).

  Note that an SoCoordinate3 node will \e replace the coordinates
  already present in the state (if any).

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Coordinate3 {
        point 0 0 0
    }
  \endcode

  \sa SoCoordinate4
*/

#include <Inventor/nodes/SoCoordinate3.h>

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoGLVBOElement.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoVBO.h"

/*!
  \var SoMFVec3f SoCoordinate3::point
  Coordinate set of 3D points.
*/

class SoCoordinate3P {
 public:
  SoCoordinate3P() : vbo(NULL) { }
  ~SoCoordinate3P() { delete this->vbo; }
  SoVBO * vbo;
};

#define PRIVATE(obj) obj->pimpl

// *************************************************************************

SO_NODE_SOURCE(SoCoordinate3);

/*!
  Constructor.
*/
SoCoordinate3::SoCoordinate3(void)
{
  PRIVATE(this) = new SoCoordinate3P;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoCoordinate3);

  SO_NODE_ADD_FIELD(point, (0.0f, 0.0f, 0.0f));
}

/*!
  Destructor.
*/
SoCoordinate3::~SoCoordinate3()
{
  delete PRIVATE(this);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoCoordinate3::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoCoordinate3, SO_FROM_INVENTOR_1|SoNode::VRML1);

  SO_ENABLE(SoGetBoundingBoxAction, SoCoordinateElement);
  SO_ENABLE(SoGLRenderAction, SoGLCoordinateElement);
  SO_ENABLE(SoGLRenderAction, SoGLVBOElement);
  SO_ENABLE(SoPickAction, SoCoordinateElement);
  SO_ENABLE(SoCallbackAction, SoCoordinateElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoCoordinateElement);
}

// Doc from superclass.
void
SoCoordinate3::doAction(SoAction * action)
{
  SoCoordinateElement::set3(action->getState(), this,
                            point.getNum(), point.getValues(0));
}

// Doc from superclass.
void
SoCoordinate3::GLRender(SoGLRenderAction * action)
{
  SoCoordinate3::doAction(action);
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
  SoGLVBOElement::setVertexVBO(state, setvbo ? PRIVATE(this)->vbo : NULL);
}

// Doc from superclass.
void
SoCoordinate3::callback(SoCallbackAction * action)
{
  SoCoordinate3::doAction(action);
}

// Doc from superclass.
void
SoCoordinate3::pick(SoPickAction * action)
{
  SoCoordinate3::doAction(action);
}

// Doc from superclass.
void
SoCoordinate3::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoCoordinate3::doAction(action);
}

// Doc from superclass.
void
SoCoordinate3::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoCoordinate3::doAction(action);
}

#undef PRIVATE
