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
  \class SoDepthBuffer SoDepthBuffer.h Inventor/nodes/SoDepthBuffer.h
  \brief The SoDepthBuffer class is a node used to control the GL depth buffer.

  \ingroup coin_nodes

  With this node you can control properties related to the OpenGL depth buffer
  in a variety of ways.

  - you can enable and disable depth buffer testing during rendering,

  - you can enable and disable writing to the depth buffer during rendering,

  - you can set the function used for the depth buffer testing, and

  - you can set the value range used in the depth buffer.

  The value range setting is useful if you need to segment the 3D world into
  different segments with different depth buffer resolutions to get a more
  optimal depth buffer resolution distribution than what a single, uniform
  depth buffer value range can give you.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    DepthBuffer {
        test TRUE
        write TRUE
        function LESS
        range 0 1
    }
  \endcode

  \COIN_CLASS_EXTENSION
  \since Coin 3.0
*/

#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLDepthBufferElement.h>
#include <Inventor/system/gl.h>

#include "nodes/SoSubNodeP.h"

/*!
  \enum SoDepthBuffer::DepthWriteFunction
  Enumeration for the various depth functions.
*/

/*!
  \var SoDepthBuffer::DepthWriteFunction SoDepthBuffer::NEVER
  Never passes.
*/

/*!
  \var SoDepthBuffer::DepthWriteFunction SoDepthBuffer::ALWAYS
  Always passes.
*/

/*!
  \var SoDepthBuffer::DepthWriteFunction SoDepthBuffer::LESS
  Passes if the incoming depth value is less than the stored depth value.
*/

/*!
  \var SoDepthBuffer::DepthWriteFunction SoDepthBuffer::LEQUAL
  Passes if the incoming depth value is less than or equal to the stored depth value.
*/

/*!
  \var SoDepthBuffer::DepthWriteFunction SoDepthBuffer::EQUAL
  Passes if the incoming depth value is equal to the stored depth value.
*/

/*!
  \var SoDepthBuffer::DepthWriteFunction SoDepthBuffer::GEQUAL
  Passes if the incoming depth value is greater than or equal to the stored depth value.
*/

/*!
  \var SoDepthBuffer::DepthWriteFunction SoDepthBuffer::GREATER
  Passes if the incoming depth value is greater than the stored depth value.
*/

/*!
  \var SoDepthBuffer::DepthWriteFunction SoDepthBuffer::NOTEQUAL
  Passes if the incoming depth value is not equal to the stored depth value.
*/

/*!
  \var SoSFBool SoDepthBuffer::test

  Enable depth buffer testing. Defaults to TRUE.
*/

/*!
  \var SoSFBool SoDepthBuffer::write

  Enable depth buffer writing. Defaults to TRUE.
*/

/*!
  \var SoSFEnum SoDepthBuffer::function

  Which depth function to use. Defaults to LESS.
*/

/*!
  \var SoSFVec2f SoDepthBuffer::range

  The value range for the depth buffer data. Defaults to [0.0, 1.0].
  The range will be clamped to [0.0, 1.0].
*/

SO_NODE_SOURCE(SoDepthBuffer);

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoDepthBuffer::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoDepthBuffer, SO_FROM_COIN_3_0);

  SO_ENABLE(SoGLRenderAction, SoGLDepthBufferElement);
}

/*!
  Constructor.
*/
SoDepthBuffer::SoDepthBuffer(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoDepthBuffer);

  SO_NODE_ADD_FIELD(test, (TRUE));
  SO_NODE_ADD_FIELD(write, (TRUE));
  SO_NODE_ADD_FIELD(function, (SoDepthBuffer::LESS));
  SO_NODE_ADD_FIELD(range, (SbVec2f(0.0f, 1.0f)));

  SO_NODE_DEFINE_ENUM_VALUE(DepthWriteFunction, NEVER);
  SO_NODE_DEFINE_ENUM_VALUE(DepthWriteFunction, ALWAYS);
  SO_NODE_DEFINE_ENUM_VALUE(DepthWriteFunction, LESS);
  SO_NODE_DEFINE_ENUM_VALUE(DepthWriteFunction, LEQUAL);
  SO_NODE_DEFINE_ENUM_VALUE(DepthWriteFunction, EQUAL);
  SO_NODE_DEFINE_ENUM_VALUE(DepthWriteFunction, GEQUAL);
  SO_NODE_DEFINE_ENUM_VALUE(DepthWriteFunction, GREATER);
  SO_NODE_DEFINE_ENUM_VALUE(DepthWriteFunction, NOTEQUAL);
  SO_NODE_SET_SF_ENUM_TYPE(function, DepthWriteFunction);
}

/*!
  Destructor.
*/
SoDepthBuffer::~SoDepthBuffer()
{
}

// Doc from parent
void
SoDepthBuffer::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  SbBool testenable = this->test.getValue();
  SbBool writeenable = this->write.getValue();
  SoDepthBufferElement::DepthWriteFunction function =
    static_cast<SoDepthBufferElement::DepthWriteFunction>(this->function.getValue());
  SbVec2f depthrange = this->range.getValue();

  // accommodate for ignored fields
  if (this->test.isIgnored()) {
    testenable = SoDepthBufferElement::getTestEnable(state);
  }
  // if we're rendering transparent objects, let SoGLRenderAction decide if
  // depth write should be enabled
  if (this->write.isIgnored() || action->isRenderingTranspPaths()) {
    writeenable = SoDepthBufferElement::getWriteEnable(state);
  }
  if (this->function.isIgnored()) {
    function = SoDepthBufferElement::getFunction(state);
  }
  if (this->range.isIgnored()) {
    range = SoDepthBufferElement::getRange(state);
  }

  // update element
  SoDepthBufferElement::set(state, testenable, writeenable,
                            function, depthrange);
}
