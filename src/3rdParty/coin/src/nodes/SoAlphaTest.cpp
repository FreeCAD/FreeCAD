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
  \class SoAlphaTest SoAlphaTest.h Inventor/nodes/SoAlphaTest.h
  \brief The SoAlphaTest class is a node used to control the GL alpha test function.

  \ingroup coin_nodes

  With this node you can control the OpenGL alpha test function. The
  alpha test function enables you to discard fragments based on its
  alpha value. For instance, if you set the function to GREATER and
  value to 0.5, only fragments with alpha value greater than 0.5 will
  be rendered.

  Alpha testing is typically used when rendering textures where all
  pixels are either completely opaque or completely
  transparent. Transparency sorting problems are avoided when alpha
  testing is used instead of blending, since depth testing can still
  be enabled and no sorting or delayed rendering is needed.

  To enable alpha testing instead of transparency blending, insert a
  TransparencyType node with value = NONE, and an AlphaTest node with
  \e function set to GREATER and \e value set to for instance 0.01.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    AlphaTest {
        function NONE
        value 0.5
    }
  \endcode

  \COIN_CLASS_EXTENSION
  \since Coin 4.0
*/

#include <Inventor/nodes/SoAlphaTest.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/system/gl.h>

#include "nodes/SoSubNodeP.h"

/*!
  \enum SoAlphaTest::Function
  Enumeration for the various alpha functions.
*/

/*!
  \var SoAlphaTest::Function SoAlphaTest::NONE
  No alpha test is performed.
*/

/*!
  \var SoAlphaTest::Function SoAlphaTest::NEVER
  Never passes.
*/

/*!
  \var SoAlphaTest::Function SoAlphaTest::ALWAYS
  Always passes.
*/

/*!
  \var SoAlphaTest::Function SoAlphaTest::LESS
  Passes if the incoming alpha value is less than the stored alpha value.
*/

/*!
  \var SoAlphaTest::Function SoAlphaTest::LEQUAL
  Passes if the incoming alpha value is less than or equal to the stored alpha value.
*/

/*!
  \var SoAlphaTest::Function SoAlphaTest::EQUAL
  Passes if the incoming alpha value is equal to the stored alpha value.
*/

/*!
  \var SoAlphaTest::Function SoAlphaTest::GEQUAL
  Passes if the incoming alpha value is greater than or equal to the stored alpha value.
*/

/*!
  \var SoAlphaTest::Function SoAlphaTest::GREATER
  Passes if the incoming alpha value is greater than the stored alpha value.
*/

/*!
  \var SoAlphaTest::Function SoAlphaTest::NOTEQUAL
  Passes if the incoming alpha value is not equal to the stored alpha value.
*/

/*!
  \var SoSFEnum SoAlphaTest::function

  Which alpha function to use. Defaults to NONE.
*/

/*!
  \var SoSFFloat SoAlphaTest::value

  The value the function will compare against when applicable. Default is 0.5.
*/

SO_NODE_SOURCE(SoAlphaTest);

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoAlphaTest::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoAlphaTest, SO_FROM_COIN_4_0);
}

/*!
  Constructor.
*/
SoAlphaTest::SoAlphaTest(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoAlphaTest);

  SO_NODE_ADD_FIELD(function, (NONE));
  SO_NODE_ADD_FIELD(value, (0.5f));

  SO_NODE_DEFINE_ENUM_VALUE(Function, NONE);
  SO_NODE_DEFINE_ENUM_VALUE(Function, NEVER);
  SO_NODE_DEFINE_ENUM_VALUE(Function, ALWAYS);
  SO_NODE_DEFINE_ENUM_VALUE(Function, LESS);
  SO_NODE_DEFINE_ENUM_VALUE(Function, LEQUAL);
  SO_NODE_DEFINE_ENUM_VALUE(Function, EQUAL);
  SO_NODE_DEFINE_ENUM_VALUE(Function, GEQUAL);
  SO_NODE_DEFINE_ENUM_VALUE(Function, GREATER);
  SO_NODE_DEFINE_ENUM_VALUE(Function, NOTEQUAL);
  SO_NODE_SET_SF_ENUM_TYPE(function, Function);
}

/*!
  Destructor.
*/
SoAlphaTest::~SoAlphaTest()
{
}

// Doc from parent
void
SoAlphaTest::GLRender(SoGLRenderAction * action)
{
  int func = this->function.getValue();
  GLenum glfunc = 0;
  switch (func) {
  case NONE:
    glfunc = 0;
    break;
  case NEVER:
    glfunc = GL_NEVER;
    break;
  case ALWAYS:
    glfunc = GL_ALWAYS;
    break;
  case LESS:
    glfunc = GL_LESS;
    break;
  case LEQUAL:
    glfunc = GL_LEQUAL;
    break;
  case EQUAL:
    glfunc = GL_EQUAL;
    break;
  case GEQUAL:
    glfunc = GL_GEQUAL;
    break;
  case GREATER:
    glfunc = GL_GREATER;
    break;
  case NOTEQUAL:
    glfunc = GL_NOTEQUAL;
    break;
  default:
    break;
  }
  SoLazyElement::setAlphaTest(action->getState(),
                              glfunc, this->value.getValue());
}
