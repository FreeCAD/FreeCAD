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
  \class SoVertexAttributeBinding SoVertexAttributeBinding.h Inventor/nodes/SoVertexAttributeBinding.h

  \brief The SoVertexAttributeBinding class is a node for setting up
  how vertex attributes are mapped to shapes.

  \sa SoVertexAttribute

  \ingroup coin_shaders

  The material binding specified in nodes of this type decides how the
  vertex attribute values of SoVertexAttribute nodes are mapped on the
  built-in geometry shape nodes.

  Here is a simple usage example:

  \verbatim
  #Inventor V2.1 ascii
  
  ShaderProgram {
    shaderObject [
      VertexShader {
        sourceProgram "vertex.glsl"
      }
    ]
  }

  Coordinate3 {
    point [ 0 0 0, 1 0 0, 1 1 0 ]
  }

  VertexAttributeBinding { value OVERALL }

  VertexAttribute {
    typeName "SoMFVec3f"
    name "color"
    values [ 1.0 0.0 0.0 ]
  }

  IndexedFaceSet {
    coordIndex [ 0, 1, 2, -1 ]
  }
  \endverbatim

  The vertex shader (vertex.glsl):
  \code
  attribute vec3 color;

  void main(void)
  {
    gl_Position = ftransform();
    fragcolor = color;
  }
  \endcode

  With SoVertexAttributeBinding::value set to \c PER_VERTEX_INDEXED
  above, the attribute indices will be taken from the
  SoIndexedFaceSet::coordIndex field when rendering.

  If SoVertexAttributeBinding::value is set to \c PER_VERTEX,
  attributes will be fetched in a monotonically increasing manner from
  the SoVertexAttribute::value field, starting at index 0.

  If SoVertexAttributeBinding::value is set to \c OVERALL, the first
  attribute value from SoVertexAttribute::value will be sent to OpenGL
  before rendering.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    VertexAttributeBinding {
        value PER_VERTEX_INDEXED
    }
  \endcode

  \sa SoVertexAttribute
  \COIN_CLASS_EXTENSION
  \since Coin 3.0
*/

// *************************************************************************

#include <Inventor/nodes/SoVertexAttributeBinding.h>
#include <Inventor/actions/SoCallbackAction.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoVertexAttributeBindingElement.h>
#include <Inventor/elements/SoOverrideElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \enum SoVertexAttributeBinding::Binding
  Enumeration of available types of material binding.
*/
/*!
  \var SoVertexAttributeBinding::Binding SoVertexAttributeBinding::OVERALL
  Apply the same material to the complete shape.
*/
/*!
  \var SoVertexAttributeBinding::Binding SoVertexAttributeBinding::PER_VERTEX

  Get a new material from the pool of material values for each
  polygon, line or point \e vertex of the shape.

  Attributes are fetched from index 0 and onwards, incrementing the
  index into the attribute pool by 1 for each new vertex of the shape
  node.
*/
/*!
  \var SoVertexAttributeBinding::Binding SoVertexAttributeBinding::PER_VERTEX_INDEXED

  Get a new attribute from the pool of attribute values for each
  polygon, line or point \e vertex of the shape.

  Attributes are fetched by the index value settings of the shape.
*/

/*!
  \var SoSFEnum SoVertexAttributeBinding::value

  The attribute binding to use for subsequent shape nodes in the scene
  graph. The default binding is SoVertexAttributeBinding::PER_VERTEX_INDEXED
*/


// *************************************************************************

SO_NODE_SOURCE(SoVertexAttributeBinding);

/*!
  Constructor.
*/
SoVertexAttributeBinding::SoVertexAttributeBinding(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoVertexAttributeBinding);

  SO_NODE_ADD_FIELD(value, (OVERALL));

  SO_NODE_DEFINE_ENUM_VALUE(Binding, OVERALL);
//SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_FACE);
//SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_FACE_INDEXED);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_VERTEX);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_VERTEX_INDEXED);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, DEFAULT);

  SO_NODE_SET_SF_ENUM_TYPE(value, Binding);
}


/*!
  Destructor.
*/
SoVertexAttributeBinding::~SoVertexAttributeBinding()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoVertexAttributeBinding::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVertexAttributeBinding, SO_FROM_INVENTOR_1|SoNode::VRML1);

  SO_ENABLE(SoGLRenderAction, SoVertexAttributeBindingElement);
  SO_ENABLE(SoPickAction, SoVertexAttributeBindingElement);
  SO_ENABLE(SoCallbackAction, SoVertexAttributeBindingElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoVertexAttributeBindingElement);
}

// Doc from superclass.
void
SoVertexAttributeBinding::GLRender(SoGLRenderAction * action)
{
  SoVertexAttributeBinding::doAction(action);
}

// Doc from superclass.
void
SoVertexAttributeBinding::doAction(SoAction * action)
{
  if (!this->value.isIgnored()) {
    SoVertexAttributeBindingElement::set(action->getState(),
                                         (SoVertexAttributeBindingElement::Binding)
                                         this->value.getValue());
  }
}

// Doc from superclass.
void
SoVertexAttributeBinding::callback(SoCallbackAction * action)
{
  SoVertexAttributeBinding::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoVertexAttributeBinding::pick(SoPickAction * action)
{
  SoVertexAttributeBinding::doAction(action);
}

// Doc from superclass.
void
SoVertexAttributeBinding::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoVertexAttributeBinding::doAction(action);
}
