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

// Note: the class documentation for the basic primitive shapes
// SoSphere, SoCylinder, SoCone and SoCube have many common, or at
// least close to common, paragraphs. If you make any changes, check
// those other shapes too, to see if your updates / fixes should be
// migrated. <mortene@sim.no>.
/*!
  \class SoCube SoCube.h Inventor/nodes/SoCube.h
  \brief The SoCube class is for rendering cubes.

  \ingroup coin_nodes

  Insert a cube shape into the scene graph. The cube is rendered with
  the current material, texture and draw style settings (if any,
  otherwise the default settings are used).

  (Strictly speaking, as you can have different width, height and
  depth values for the "cube", instances of this class actually
  represents \e boxes.)

  The SoCube node class is provided as a convenient abstraction for
  the application programmer to use "complex" shapes of this type
  without having to do the calculation and book-keeping of the polygon
  sides and other low level programming herself.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Cube {
        width 2
        height 2
        depth 2
    }
  \endcode

  \sa SoCylinder, SoSphere, SoCone
*/

#include <Inventor/nodes/SoCube.h>
#include "coindefs.h"

#include <Inventor/SbPlane.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/details/SoCubeDetail.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/misc/SoState.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoGL.h"
#include "misc/SoGenerate.h"
#include "misc/SoPick.h"

/*!
  \var SoSFFloat SoCube::width
  X-axis dimension of cube, defaults to 2.0.
*/
/*!
  \var SoSFFloat SoCube::height
  Y-axis dimension of cube, defaults to 2.0.
*/
/*!
  \var SoSFFloat SoCube::depth
  Z-axis dimension of cube, defaults to 2.0.
*/


// *************************************************************************

SO_NODE_SOURCE(SoCube);

/*!
  Constructor.
*/
SoCube::SoCube(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoCube);

  SO_NODE_ADD_FIELD(width, (2.0f));
  SO_NODE_ADD_FIELD(height, (2.0f));
  SO_NODE_ADD_FIELD(depth, (2.0f));
}

/*!
  Destructor.
*/
SoCube::~SoCube()
{
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoCube::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoCube, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

// Doc in parent.
void
SoCube::GLRender(SoGLRenderAction * action)
{
  if (!this->shouldGLRender(action)) return;
  SoState * state = action->getState();

  SoMaterialBindingElement::Binding binding =
    SoMaterialBindingElement::get(state);

  SbBool materialPerPart =
    (binding == SoMaterialBindingElement::PER_PART ||
     binding == SoMaterialBindingElement::PER_PART_INDEXED ||
     binding == SoMaterialBindingElement::PER_FACE ||
     binding == SoMaterialBindingElement::PER_FACE_INDEXED);

  SbBool doTextures = FALSE;
  SbBool do3DTextures = FALSE;
  if (SoGLMultiTextureEnabledElement::get(state, 0)) {
    doTextures = TRUE;
    if (SoGLMultiTextureEnabledElement::getMode(state,0) ==
        SoMultiTextureEnabledElement::TEXTURE3D) {
      do3DTextures = TRUE;
    }
  }

  SoMaterialBundle mb(action);
  mb.sendFirst();

  SbBool sendNormals = !mb.isColorOnly() ||
    (SoMultiTextureCoordinateElement::getType(state) == SoMultiTextureCoordinateElement::FUNCTION);

  unsigned int flags = 0;
  if (materialPerPart) flags |= SOGL_MATERIAL_PER_PART;
  if (doTextures) {
    switch (SoMultiTextureEnabledElement::getMode(state, 0)) {
    default:
      flags |= SOGL_NEED_TEXCOORDS;
      break;
    case SoMultiTextureEnabledElement::CUBEMAP:
      flags |= SOGL_NEED_3DTEXCOORDS;
      break;
    }
  }
  else if (do3DTextures) flags |= SOGL_NEED_3DTEXCOORDS;
  if (sendNormals) flags |= SOGL_NEED_NORMALS;

  sogl_render_cube(width.getValue(),
                   height.getValue(),
                   depth.getValue(),
                   &mb,
                   flags, state);
}

// Doc in parent.
void
SoCube::generatePrimitives(SoAction * action)
{
  SoMaterialBindingElement::Binding binding =
    SoMaterialBindingElement::get(action->getState());

  SbBool materialPerPart =
    (binding == SoMaterialBindingElement::PER_PART ||
     binding == SoMaterialBindingElement::PER_PART_INDEXED ||
     binding == SoMaterialBindingElement::PER_FACE ||
     binding == SoMaterialBindingElement::PER_FACE_INDEXED);

  unsigned int flags = 0;
  if (materialPerPart) flags |= SOGEN_MATERIAL_PER_PART;
  sogen_generate_cube(this->width.getValue(),
                      this->height.getValue(),
                      this->depth.getValue(),
                      flags,
                      this,
                      action);
}

// Doc in parent.
void
SoCube::computeBBox(SoAction * COIN_UNUSED_ARG(action), SbBox3f & box, SbVec3f & center)
{
  center.setValue(0.0f, 0.0f, 0.0f);
  float w, h, d;
  this->getHalfSize(w, h, d);

  // Allow negative values.
  if (w < 0.0f) w = -w;
  if (h < 0.0f) h = -h;
  if (d < 0.0f) d = -d;

  box.setBounds(-w, -h, -d, w, h, d);
}

// Doc in parent.
void
SoCube::rayPick(SoRayPickAction * action)
{
  if (!shouldRayPick(action)) return;

  SoMaterialBindingElement::Binding binding =
    SoMaterialBindingElement::get(action->getState());

  SbBool materialPerPart =
    (binding == SoMaterialBindingElement::PER_PART ||
     binding == SoMaterialBindingElement::PER_PART_INDEXED);

  sopick_pick_cube(this->width.getValue(),
                   this->height.getValue(),
                   this->depth.getValue(),
                   materialPerPart ? SOPICK_MATERIAL_PER_PART : 0,
                   this, action);
}

// Convenience function for finding half the size of the box.
void
SoCube::getHalfSize(float & w, float & h, float & d)
{
  w = (this->width.isIgnored() ? 1.0f :
       this->width.getValue() * 0.5f);
  h = (this->height.isIgnored() ? 1.0f :
       this->height.getValue() * 0.5f);
  d = (this->depth.isIgnored() ? 1.0f :
       this->depth.getValue() * 0.5f);
}

// Doc from parent.
void
SoCube::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  action->addNumTriangles(12);
}
