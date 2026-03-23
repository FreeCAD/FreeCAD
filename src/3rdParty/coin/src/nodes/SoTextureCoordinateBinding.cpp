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
  \class SoTextureCoordinateBinding SoTextureCoordinateBinding.h Inventor/nodes/SoTextureCoordinateBinding.h
  \brief The SoTextureCoordinateBinding class says how texture coordinates should be bound to shapes.

  \ingroup coin_nodes

  SoTextureCoordinateBinding binds current coordinates to subsequent
  shapes by using either per vertex or per indexed vertex binding.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TextureCoordinateBinding {
        value PER_VERTEX_INDEXED
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/actions/SoGLRenderAction.h>

#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoCallbackAction.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \enum SoTextureCoordinateBinding::Binding

  The binding types available for the
  SoTextureCoordinateBinding::value field.
*/
/*!
  \var SoTextureCoordinateBinding::Binding SoTextureCoordinateBinding::PER_VERTEX
  Get a new texture coordinate from the pool of texture coordinates for
  each vertex of the shape.

  Texture Coordinates are fetched from index 0 and onwards, incrementing
  the index into the texture coordinates pool by 1 for each new vertex
  of the shape node.
*/
/*!
  \var SoTextureCoordinateBinding::Binding SoTextureCoordinateBinding::PER_VERTEX_INDEXED
  Get a new texture coordinate from the pool of texture coordinates for
  each vertex of the shape.

  Texture coordinates are fetched by the index value settings of the shape.
*/
/*!
  \var SoTextureCoordinateBinding::Binding SoTextureCoordinateBinding::DEFAULT

  Obsolete value, please don't use.
*/


/*!
  \var SoSFEnum SoTextureCoordinateBinding::value

  Type of texture map binding for subsequent shape nodes in the
  scene graph. Default field value is
  SoTextureCoordinateBinding::PER_VERTEX_INDEXED.
*/


// *************************************************************************

SO_NODE_SOURCE(SoTextureCoordinateBinding);

/*!
  Constructor.
*/
SoTextureCoordinateBinding::SoTextureCoordinateBinding(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTextureCoordinateBinding);

  SO_NODE_ADD_FIELD(value, (SoTextureCoordinateBinding::PER_VERTEX_INDEXED));

  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_VERTEX);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_VERTEX_INDEXED);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, DEFAULT);
  SO_NODE_SET_SF_ENUM_TYPE(value, Binding);
}

/*!
  Destructor.
*/
SoTextureCoordinateBinding::~SoTextureCoordinateBinding()
{
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTextureCoordinateBinding::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTextureCoordinateBinding, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoGLRenderAction, SoTextureCoordinateBindingElement);
  SO_ENABLE(SoPickAction, SoTextureCoordinateBindingElement);
  SO_ENABLE(SoCallbackAction, SoTextureCoordinateBindingElement);
}

// Documented in superclass.
void
SoTextureCoordinateBinding::GLRender(SoGLRenderAction * action)
{
  SoTextureCoordinateBinding::doAction(action);
}

// Documented in superclass.
void
SoTextureCoordinateBinding::doAction(SoAction *action)
{
  if (!value.isIgnored())
    SoTextureCoordinateBindingElement::set(action->getState(), this,
     (SoTextureCoordinateBindingElement::Binding)value.getValue());
}

// Documented in superclass.
void
SoTextureCoordinateBinding::callback(SoCallbackAction *action)
{
  SoTextureCoordinateBinding::doAction(action);
}

// Documented in superclass.
void
SoTextureCoordinateBinding::pick(SoPickAction *action)
{
  SoTextureCoordinateBinding::doAction(action);
}

// Documented in superclass.
SbBool
SoTextureCoordinateBinding::readInstance(SoInput * in, unsigned short flags)
{
  SbBool ret = inherited::readInstance(in, flags);
  if (ret) {
    // test for obsolete values
    if (this->value.getValue() < 2) {
      this->value = PER_VERTEX_INDEXED;
    }
  }
  return ret;
}
