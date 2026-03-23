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
  \class SoVRMLTextureCoordinate SoVRMLTextureCoordinate.h Inventor/VRMLnodes/SoVRMLTextureCoordinate.h
  \brief The SoVRMLTextureCoordinate class binds texture coordinates to vertex-based geometry.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim  
  TextureCoordinate {
    exposedField MFVec2f point  []      # (-inf, inf)
  }
  \endverbatim

  The TextureCoordinate node specifies a set of 2D texture coordinates
  used by vertex-based geometry nodes (e.g., SoVRMLIndexedFaceSet and
  SoVRMLElevationGrid) to map textures to vertices.  Textures are two
  dimensional colour functions that, given an (s, t) coordinate,
  return a colour value colour(s, t). Texture map values
  (SoVRMLImageTexture, SoVRMLMovieTexture, and SoVRMLPixelTexture)
  range from [0.0, 1.0] along the S-axis and T-axis. However,
  TextureCoordinate values, specified by the point field, may be in
  the range (-inf,inf). Texture coordinates identify a location (and
  thus a colour value) in the texture map. The horizontal coordinate s
  is specified first, followed by the vertical coordinate t.

  If the texture map is repeated in a given direction (S-axis or
  T-axis), a texture coordinate C (s or t) is mapped into a texture
  map that has N pixels in the given direction as follows:

  \verbatim
  Texture map location = (C - floor(C)) × N 
  \endverbatim

  If the texture map is not repeated, the texture coordinates are
  clamped to the 0.0 to 1.0 range as follows: 
  
  \verbatim
  Texture map location = N, if C > 1.0, 
                       = 0.0, if C < 0.0, 
                       = C × N, if 0.0 <= C <= 1.0.
  \endverbatim

  Details on repeating textures are specific to texture map node types
  described in SoVRMLImageTexture, SoVRMLMovieTexture, and
  SoVRMLPixelTexture.
  
*/

/*!
  \var SoMFVec2f SoVRMLTextureCoordinate::point
  The texture coordinates. Empty by default.
*/

#include <Inventor/VRMLnodes/SoVRMLTextureCoordinate.h>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoGLVBOElement.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoVBO.h"

class SoVRMLTextureCoordinateP {
 public:
  SoVRMLTextureCoordinateP() : vbo(NULL) { }
  ~SoVRMLTextureCoordinateP() { delete this->vbo; }
  SoVBO * vbo;
};

#define PRIVATE(obj) obj->pimpl

SO_NODE_SOURCE(SoVRMLTextureCoordinate);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLTextureCoordinate::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLTextureCoordinate, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLTextureCoordinate::SoVRMLTextureCoordinate(void)
{
  PRIVATE(this) = new SoVRMLTextureCoordinateP;
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLTextureCoordinate);

  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(point);
}

/*!
  Destructor.
*/
SoVRMLTextureCoordinate::~SoVRMLTextureCoordinate()
{
  delete PRIVATE(this);
}

// Doc in parent
void
SoVRMLTextureCoordinate::doAction(SoAction * action)
{
  SoMultiTextureCoordinateElement::set2(action->getState(), this, 0,
                                        point.getNum(),
                                        point.getValues(0));
}

// Doc in parent
void
SoVRMLTextureCoordinate::callback(SoCallbackAction * action)
{
  SoVRMLTextureCoordinate::doAction((SoAction*)action);
}

// Doc in parent
void
SoVRMLTextureCoordinate::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  SoGLMultiTextureCoordinateElement::setTexGen(state,
                                               this, 0, NULL);
  SoVRMLTextureCoordinate::doAction((SoAction*)action);

  SoBase::staticDataLock();
  const int num = this->point.getNum();
  SbBool setvbo = FALSE;
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
      PRIVATE(this)->vbo->setBufferData(this->point.getValues(0),
                                        num*sizeof(SbVec2f),
                                        this->getNodeId());
    }
  }
  else if (PRIVATE(this)->vbo && PRIVATE(this)->vbo->getBufferDataId()) {
    // clear buffers to deallocate VBO memory
    PRIVATE(this)->vbo->setBufferData(NULL, 0, 0);
  }
  SoBase::staticDataUnlock();
  if (setvbo) {
    SoGLVBOElement::setTexCoordVBO(state, 0, PRIVATE(this)->vbo);
  }
}

// Doc in parent
void
SoVRMLTextureCoordinate::pick(SoPickAction * action)
{
  SoVRMLTextureCoordinate::doAction((SoAction*)action);
}

// Doc in parent
void
SoVRMLTextureCoordinate::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoVRMLTextureCoordinate::doAction((SoAction*)action);
}

#undef PRIVATE
#endif // HAVE_VRML97
