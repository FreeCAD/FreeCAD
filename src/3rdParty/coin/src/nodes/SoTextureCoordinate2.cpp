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
  \class SoTextureCoordinate2 SoTextureCoordinate2.h Inventor/nodes/SoTextureCoordinate2.h
  \brief The SoTextureCoordinate2 class contains a set of coordinates for the mapping of 2D textures.

  \ingroup coin_nodes

  When encountering a node of this type during traversal, the
  coordinates it contains will be put on the state stack. Some shape
  nodes (for instance SoIndexedFaceSet, among many others) can then
  use these coordinates for explicit, detailed control of how textures
  are mapped to its surfaces.

  (If texture mapping is used without any SoTextureCoordinate2 nodes in
  the scene graph leading up to a shape node, all shape types have
  default fallbacks. So SoTextureCoordinate2 nodes are only necessary
  to use if you are not satisfied with the default mapping.)

  Note that an SoTextureCoordinate2 node will \e replace the
  coordinates already present in the state (if any).

  Here's a very simple example (in Inventor scene graph file format --
  mapping it to source code is straightforward) that shows how to set
  up two quadratic polygons, one mapped 1:1 to the texture, the other
  using only the upper left quarter of the texture:

\code

Separator {
   Texture2 {
      image 6 8 3
      0x00ff0000 0x00ff0000 0x000000ff 0x000000ff 0x00ff00ff 0x00ff00ff
      0x00ff0000 0x00ff0000 0x000000ff 0x000000ff 0x00ff00ff 0x00ff00ff
      0x00ff0000 0x00ff0000 0x000000ff 0x000000ff 0x00ff00ff 0x00ff00ff
      0x0000ff00 0x0000ff00 0x0000ffff 0x0000ffff 0x0000ff00 0x0000ff00
      0x0000ff00 0x0000ff00 0x0000ffff 0x0000ffff 0x0000ff00 0x0000ff00
      0x00ffff00 0x00ffff00 0x000000ff 0x000000ff 0x00ffffff 0x00ffffff
      0x00ffff00 0x00ffff00 0x000000ff 0x000000ff 0x00ffffff 0x00ffffff
      0x00ffff00 0x00ffff00 0x000000ff 0x000000ff 0x00ffffff 0x00ffffff
   }

   Coordinate3 { point [ -1 -1 0, 1 -1 0, 1 1 0, -1 1 0 ] }

   # "1:1 mapping" to actual texture appearance. (Note that Y goes
   # from bottom to top, versus the common way of specifying bitmap
   # data from top to bottom.)
   TextureCoordinate2 { point [ 0 1, 1 1, 1 0, 0 0 ] }

   IndexedFaceSet {
      coordIndex [ 0, 1, 2, 3, -1 ]
      textureCoordIndex [ 0, 1, 2, 3, -1 ]
   }

   Translation { translation +4 0 0 }

   # Top left corner.
   TextureCoordinate2 { point [ 0 0.5, 0.5 0.5, 0.5 0, 0 0 ] }

   IndexedFaceSet {
      coordIndex [ 0, 1, 2, 3, -1 ]
      textureCoordIndex [ 0, 1, 2, 3, -1 ]
   }
}

\endcode

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TextureCoordinate2 {
        point [  ]
    }
  \endcode

  \sa SoTextureCoordinateFunction, SoTextureCoordinateBinding
*/

// *************************************************************************

#include <Inventor/nodes/SoTextureCoordinate2.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/C/glue/gl.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoVBO.h"

// *************************************************************************

/*!
  \var SoMFVec2f SoTextureCoordinate2::point

  The set of 2D texture coordinates. Default value of field is an
  empty set.

  Texture coordinates are usually specified in normalized coordinates,
  i.e. in the range [0, 1]. (0, 0) is the lower left corner, while
  (1, 1) is the upper right corner of the texture image. Coordinates
  outside the [0, 1] range can be used to repeat the texture across a
  surface.

  \sa SoTexture2::wrapS, SoTexture2::wrapT
*/

// *************************************************************************

class SoTextureCoordinate2P {
 public:
  SoTextureCoordinate2P() : vbo(NULL) { }
  ~SoTextureCoordinate2P() { delete this->vbo; }
  SoVBO * vbo;
};

#define PRIVATE(obj) obj->pimpl

SO_NODE_SOURCE(SoTextureCoordinate2);

/*!
  Constructor.
*/
SoTextureCoordinate2::SoTextureCoordinate2(void)
{
  PRIVATE(this) = new SoTextureCoordinate2P;
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTextureCoordinate2);
  SO_NODE_ADD_FIELD(point, (NULL));
}

/*!
  Destructor.
*/
SoTextureCoordinate2::~SoTextureCoordinate2()
{
  delete PRIVATE(this);
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTextureCoordinate2::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTextureCoordinate2, SO_FROM_INVENTOR_1|SoNode::VRML1);

  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureCoordinateElement);
  SO_ENABLE(SoCallbackAction, SoMultiTextureCoordinateElement);
  SO_ENABLE(SoPickAction, SoMultiTextureCoordinateElement);
}

// Documented in superclass.
void
SoTextureCoordinate2::doAction(SoAction * action)
{
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state);
  SoMultiTextureCoordinateElement::set2(action->getState(), this, unit,
                                        this->point.getNum(),
                                        this->point.getValues(0));
}

// Documented in superclass.
void
SoTextureCoordinate2::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state);

  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));
  int maxunits = cc_glglue_max_texture_units(glue);
  
  if (unit < maxunits) {
    SoGLMultiTextureCoordinateElement::setTexGen(action->getState(), this, unit, NULL);
    SoMultiTextureCoordinateElement::set2(action->getState(), this, unit,
                                          point.getNum(),
                                          point.getValues(0));
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
                                        num*sizeof(SbVec2f),
                                        this->getNodeId());
    }
  }
  else if (PRIVATE(this)->vbo && PRIVATE(this)->vbo->getBufferDataId()) {
    // clear buffers to deallocate VBO memory
    PRIVATE(this)->vbo->setBufferData(NULL, 0, 0);
  }
  SoBase::staticDataUnlock();
  SoGLVBOElement::setTexCoordVBO(state, 0, setvbo ? PRIVATE(this)->vbo : NULL);
}

// Documented in superclass.
void
SoTextureCoordinate2::callback(SoCallbackAction * action)
{
  SoTextureCoordinate2::doAction((SoAction *)action);
}

// Documented in superclass.
void
SoTextureCoordinate2::pick(SoPickAction * action)
{
  SoTextureCoordinate2::doAction((SoAction *)action);
}

#undef PRIVATE
