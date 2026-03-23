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
  \class SoCoordinate4 SoCoordinate4.h Inventor/nodes/SoCoordinate4.h
  \brief The SoCoordinate4 class is a node for providing coordinates to shape nodes.

  \ingroup coin_nodes

  When encountering nodes of this type during traversal, the
  coordinates it contains will be put on the state stack for later use
  by shape nodes of types which need coordinate sets (like SoFaceSet
  nodes or SoPointSet nodes).

  The fourth element of the coordinate vectors is used for
  normalization. A node of this type where all the normalization
  values are equal to 1.0 is the equivalent of setting up an
  SoCoordinate3 node.

  Note that an SoCoordinate4 node will \e replace the coordinates
  already present in the state (if any).

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Coordinate4 {
        point 0 0 0 1
    }
  \endcode

  \sa SoCoordinate3
*/

#include <Inventor/nodes/SoCoordinate4.h>

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
  \var SoMFVec4f SoCoordinate4::point

  Coordinate set of 3D points as 4D vectors (each vector contains a 3D
  coordinate plus normalization value).
*/

class SoCoordinate4P {
 public:
  SoCoordinate4P() : vbo(NULL) { }
  ~SoCoordinate4P() { delete this->vbo; }
  SoVBO * vbo;
};

#define PRIVATE(obj) obj->pimpl

// *************************************************************************

SO_NODE_SOURCE(SoCoordinate4);

/*!
  Constructor.
*/
SoCoordinate4::SoCoordinate4(void)
{
  PRIVATE(this) = new SoCoordinate4P;
  SO_NODE_INTERNAL_CONSTRUCTOR(SoCoordinate4);

  SO_NODE_ADD_FIELD(point, (SbVec4f(0.0f, 0.0f, 0.0f, 1.0f)));
}

/*!
  Destructor.
*/
SoCoordinate4::~SoCoordinate4()
{
  delete PRIVATE(this);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoCoordinate4::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoCoordinate4, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoGetBoundingBoxAction, SoCoordinateElement);
  SO_ENABLE(SoGLRenderAction, SoGLCoordinateElement);
  SO_ENABLE(SoPickAction, SoCoordinateElement);
  SO_ENABLE(SoCallbackAction, SoCoordinateElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoCoordinateElement);
}

// Doc from superclass.
void
SoCoordinate4::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoCoordinate4::doAction(action);
}

// Doc from superclass.
void
SoCoordinate4::doAction(SoAction * action)
{
  SoCoordinateElement::set4(action->getState(), this,
                            point.getNum(), point.getValues(0));
}

// Doc from superclass.
void
SoCoordinate4::GLRender(SoGLRenderAction * action)
{
  SoCoordinate4::doAction(action);
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
                                        num*sizeof(SbVec4f),
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

// Doc from superclass.
void
SoCoordinate4::callback(SoCallbackAction * action)
{
  SoCoordinate4::doAction(action);
}

// Doc from superclass.
void
SoCoordinate4::pick(SoPickAction * action)
{
  SoCoordinate4::doAction(action);
}

// Doc from superclass.
void
SoCoordinate4::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoCoordinate4::doAction(action);
}

#undef PRIVATE
