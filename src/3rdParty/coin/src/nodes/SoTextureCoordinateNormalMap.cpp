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
  \class SoTextureCoordinateNormalMap SoTextureCoordinateNormalMap.h Inventor/nodes/SoTextureCoordinateNormalMap.h
  \brief The SoTextureCoordinateNormalMap class generates texture coordinates by projecting onto a surrounding texture.

  \ingroup coin_nodes

  The texture specifying the environment will be mapped around the 
  scene graph below this node using a sphere. The texture will be mapped
  onto the scene graph taking camera position into account. This will
  lead to an object reflecting its environment.

  Here is a scene graph example showing how environment mapping can be
  applied to an object:

  \code
  #Inventor V2.1 ascii

  Separator {

    Texture2 {
      filename "ocean.jpg" # the environment, in this case ocean
    }
    TextureCoordinateNormalMap {}

    Cube {} # the environmentally mapped object
  }
  \endcode

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TextureCoordinateNormalMap {
    }
  \endcode
*/

// *************************************************************************

// FIXME: Can this somehow relate to 3D textures? (kintel 20020203)

#include <Inventor/nodes/SoTextureCoordinateNormalMap.h>
#include "coindefs.h"

#include <cstdlib>
#include <cfloat>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/SbVec3f.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/system/gl.h>

#include "tidbitsp.h"
#include "nodes/SoSubNodeP.h"

// *************************************************************************

class SoTextureCoordinateNormalMapP {
public:
  static SbVec4f * dummy_texcoords;
  static void cleanup_func(void);
};

SbVec4f * SoTextureCoordinateNormalMapP::dummy_texcoords = NULL;

void
SoTextureCoordinateNormalMapP::cleanup_func(void)
{
  delete SoTextureCoordinateNormalMapP::dummy_texcoords;
}

// *************************************************************************

SO_NODE_SOURCE(SoTextureCoordinateNormalMap);

/*!
  Constructor.
*/
SoTextureCoordinateNormalMap::SoTextureCoordinateNormalMap()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTextureCoordinateNormalMap);
}

/*!
  Destructor.
*/
SoTextureCoordinateNormalMap::~SoTextureCoordinateNormalMap()
{
}

// doc in super
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTextureCoordinateNormalMap::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTextureCoordinateNormalMap, SO_FROM_INVENTOR_1);

  SoTextureCoordinateNormalMapP::dummy_texcoords = new SbVec4f(0.0f, 0.0f, 0.0f, 1.0f);
  coin_atexit((coin_atexit_f *)SoTextureCoordinateNormalMapP::cleanup_func, CC_ATEXIT_NORMAL);
}

// generates texture coordinates for GLRender, callback and pick actions
const SbVec4f &
SoTextureCoordinateNormalMap::generate(void * COIN_UNUSED_ARG(userdata),
                                         const SbVec3f & COIN_UNUSED_ARG(p),
                                         const SbVec3f & COIN_UNUSED_ARG(n))
{
  return *SoTextureCoordinateNormalMapP::dummy_texcoords;
}

// doc from parent
void
SoTextureCoordinateNormalMap::doAction(SoAction * action)
{
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state);
  SoMultiTextureCoordinateElement::setFunction(action->getState(), this, unit,
                                               generate,
                                               action->getState());
}

// doc from parent
void
SoTextureCoordinateNormalMap::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state);
  SoMultiTextureCoordinateElement::setFunction(action->getState(), this,
                                               unit,
                                               generate,
                                               action->getState());
  SoGLMultiTextureCoordinateElement::setTexGen(action->getState(),
                                               this, unit, handleTexgen, 
                                               NULL,
                                               generate,
                                               action->getState());
  
}

// doc from parent
void
SoTextureCoordinateNormalMap::callback(SoCallbackAction * action)
{
  SoTextureCoordinateNormalMap::doAction((SoAction *)action);
}

// doc from parent
void
SoTextureCoordinateNormalMap::pick(SoPickAction * action)
{
  SoTextureCoordinateNormalMap::doAction((SoAction *)action);
}

void
SoTextureCoordinateNormalMap::handleTexgen(void * /* data */)
{
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP);  
  glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP);
}
