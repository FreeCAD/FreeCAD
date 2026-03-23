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
  \class SoTextureCoordinate3 SoTextureCoordinate3.h Inventor/nodes/SoTextureCoordinate3.h
  \brief The SoTextureCoordinate3 class contains a set of coordinates for the mapping of 2D textures.

  \ingroup coin_nodes

  When encountering a node of this type during traversal, the
  coordinates it contains will be put on the state stack. Some shape
  nodes can then use these coordinates for explicit, detailed control
  of how 3D textures are mapped.

  (If 3D textures are used without any SoTextureCoordinate3 nodes in
  the scene graph leading up to a shape node, the shape types have
  default fallbacks. So SoTextureCoordinate3 nodes are only necessary
  to use if you are not satisfied with the default mapping.)

  Note that an SoTextureCoordinate3 node will \e replace the
  coordinates already present in the state (if any).

  \COIN_CLASS_EXTENSION

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TextureCoordinate3 {
        point [  ]
    }
  \endcode

  \sa SoTextureCoordinate2
  \since Coin 2.0
  \since TGS Inventor 2.6
*/

// *************************************************************************

#include <Inventor/nodes/SoTextureCoordinate3.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/C/glue/gl.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoVBO.h"

// *************************************************************************

/*!
  \var SoMFVec3f SoTextureCoordinate3::point

  The set of 3D texture coordinates. Default value of field is an
  empty set.

  Texture coordinates are usually specified in normalized coordinates,
  i.e. in the range [0, 1]. Coordinates outside the [0, 1] range can be
  used to repeat the texture across a surface.

  \sa SoTexture3::wrapR, SoTexture3::wrapS, SoTexture3::wrapT 
*/

// *************************************************************************

class SoTextureCoordinate3P {
 public:
  SoTextureCoordinate3P() : vbo(NULL) { }
  ~SoTextureCoordinate3P() { delete this->vbo; }
  SoVBO * vbo;
};

#define PRIVATE(obj) obj->pimpl

SO_NODE_SOURCE(SoTextureCoordinate3);

/*!
  Constructor.
*/
SoTextureCoordinate3::SoTextureCoordinate3(void)
{
  PRIVATE(this) = new SoTextureCoordinate3P;
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTextureCoordinate3);
  SO_NODE_ADD_FIELD(point, (NULL));
}

/*!
  Destructor.
*/
SoTextureCoordinate3::~SoTextureCoordinate3()
{
  delete PRIVATE(this);
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTextureCoordinate3::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTextureCoordinate3, SO_FROM_INVENTOR_2_6|SO_FROM_COIN_2_0);

  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureCoordinateElement);
  SO_ENABLE(SoCallbackAction, SoMultiTextureCoordinateElement);
}

// Documented in superclass.
void
SoTextureCoordinate3::doAction(SoAction * action)
{
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state);
  
  SoMultiTextureCoordinateElement::set3(action->getState(), this, unit,
                                        this->point.getNum(),
                                        this->point.getValues(0));
}

// Documented in superclass.
void
SoTextureCoordinate3::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state);

  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));
  int maxunits = cc_glglue_max_texture_units(glue);
  
  if (unit < maxunits) {
    SoGLMultiTextureCoordinateElement::setTexGen(action->getState(), this, unit, NULL);
    SoMultiTextureCoordinateElement::set3(action->getState(), this, unit,
                                          this->point.getNum(),
                                          this->point.getValues(0));
  }
  
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

// Documented in superclass.
void
SoTextureCoordinate3::callback(SoCallbackAction * action)
{
  SoTextureCoordinate3::doAction((SoAction *)action);
}

// Documented in superclass.
void
SoTextureCoordinate3::pick(SoPickAction * action)
{
  SoTextureCoordinate3::doAction((SoAction *)action);
}

#undef PRIVATE
