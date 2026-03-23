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
  \class SoFragmentShader SoFragmentShader.h Inventor/nodes/SoFragmentShader.h
  \brief The SoFragmentShader class is used for setting up fragment shader programs.

  \ingroup coin_shaders

  See \ref coin_shaders_page "Shaders in Coin" for more information
  on how to set up a scene graph with shaders.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    FragmentShader {
      isActive TRUE
      sourceType FILENAME
      sourceProgram ""
      parameter []
    }
  \endcode

  \sa SoShaderObject
  \sa SoShaderProgram
  \since Coin 2.5
*/

#include <Inventor/nodes/SoFragmentShader.h>
#include <Inventor/misc/SoGLDriverDatabase.h>

#include "nodes/SoSubNodeP.h"
#include "glue/glp.h"

// *************************************************************************

SO_NODE_SOURCE(SoFragmentShader);

// *************************************************************************

/*!
  \copybrief SoNode::initClass(void)
*/
void
SoFragmentShader::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoFragmentShader,
                              SO_FROM_COIN_2_5|SO_FROM_INVENTOR_5_0);
}

/*!
  Constructor.
*/
SoFragmentShader::SoFragmentShader(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoFragmentShader);
}

/*!
  Destructor.
*/
SoFragmentShader::~SoFragmentShader()
{
}

// *************************************************************************

/*!
  Returns a boolean indicating whether the requested source type is
  supported by the OpenGL driver or not.

  <i>Beware:</i> To get a correct answer, a valid OpenGL context must
  be available.
*/
SbBool
SoFragmentShader::isSupported(SourceType sourceType)
{
  // The function signature is not very well designed, as we really
  // need a guaranteed GL context for this. (We've chosen to be
  // compatible with TGS Inventor, so don't change the signature.)

  void * ptr = coin_gl_current_context();
  assert(ptr && "No active OpenGL context found!");
  if (!ptr) return FALSE; // Always bail out. Even when compiled in 'release' mode.

  const cc_glglue * glue = cc_glglue_instance_from_context_ptr(ptr);

  if (sourceType == ARB_PROGRAM) {
    return SoGLDriverDatabase::isSupported(glue, SO_GL_ARB_FRAGMENT_PROGRAM);
  }
  else if (sourceType == GLSL_PROGRAM) {
    return SoGLDriverDatabase::isSupported(glue, SO_GL_ARB_SHADER_OBJECT);
  }
  // FIXME: Add support for detecting missing Cg support (20050427
  // handegar)
  else if (sourceType == CG_PROGRAM) return TRUE;

  return FALSE;
}

// *************************************************************************

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoFragmentShader * node = new SoFragmentShader;
  assert(node);
  node->ref();
  BOOST_CHECK_MESSAGE(node->getTypeId() != SoType::badType(),
                      "missing class initialization");
  node->unref();
}

#endif // COIN_TEST_SUITE
