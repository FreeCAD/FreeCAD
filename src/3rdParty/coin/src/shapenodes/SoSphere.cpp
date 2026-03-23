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
  \class SoSphere SoSphere.h Inventor/nodes/SoSphere.h
  \brief The SoSphere class is for rendering sphere shapes.

  \ingroup coin_nodes

  Renders a sphere with the size given by the SoSphere::radius
  field. The sphere is rendered with the current material, texture and
  draw style settings (if any, otherwise the default settings are
  used).

  The SoSphere node class is provided as a convenient abstraction for
  the application programmer to use "complex" shapes of this type
  without having to do the tessellation to polygons and other
  low level programming herself.

  A good trick for rendering ellipsoidal 3D shapes is to use an
  SoSphere prefixed with an SoScale transformation to "flatten" it
  along one or more of the principal axes. (I.e., use for instance an
  SoScale node with SoScale::scaleFactor equal to [1, 1, 0.1] to
  flatten it along the Z direction.)

  A sphere is visualized by the underlying rendering system by first
  tessellating the conceptual sphere into a set of polygons. To
  control the trade-off between an as much as possible correct visual
  appearance of the sphere versus fast rendering, use an SoComplexity
  node to influence the number of polygons generated from the
  tessellation process. (The higher the complexity value, the more
  polygons will be generated, the more \e rounded the sphere will
  look.) Set the SoComplexity::value field to what you believe would
  be a good trade-off between correctness and speed for your
  particular application.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Sphere {
        radius 1
    }
  \endcode

  \sa SoCone, SoCylinder, SoCube
*/

#include <Inventor/nodes/SoSphere.h>
#include "coindefs.h"

#include <Inventor/SbSphere.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/misc/SoState.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoGL.h"
#include "misc/SoGenerate.h"
#include "misc/SoPick.h"

/*!
  \var SoSFFloat SoSphere::radius

  Radius of sphere. Default value is 1.0.
*/

#define SPHERE_NUM_SLICES 30.0f
#define SPHERE_NUM_STACKS 30.0f

// *************************************************************************

SO_NODE_SOURCE(SoSphere);

/*!
  Constructor.
*/
SoSphere::SoSphere(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoSphere);

  SO_NODE_ADD_FIELD(radius, (1.0f));
}

/*!
  Destructor.
*/
SoSphere::~SoSphere()
{
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoSphere::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoSphere, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

// Documented in superclass.
void
SoSphere::GLRender(SoGLRenderAction * action)
{
  if (!this->shouldGLRender(action)) return;

  SoState * state = action->getState();

  SoMaterialBundle mb(action);
  mb.sendFirst();

  SbBool doTextures = FALSE;
  SbBool do3DTextures = FALSE;
  if (SoGLMultiTextureEnabledElement::get(state, 0)) {
    doTextures = TRUE;
    if (SoGLMultiTextureEnabledElement::getMode(state,0) ==
        SoMultiTextureEnabledElement::TEXTURE3D) {
      do3DTextures = TRUE;
    }
  }

  SbBool sendNormals = !mb.isColorOnly() || 
    (SoMultiTextureCoordinateElement::getType(state) == SoMultiTextureCoordinateElement::FUNCTION);
  
  float complexity = SbClamp(this->getComplexityValue(action), 0.0f, 1.0f);

  unsigned int flags = 0;
  if (sendNormals) flags |= SOGL_NEED_NORMALS;
  if (doTextures) flags |= SOGL_NEED_TEXCOORDS;
  else if (do3DTextures) flags |= SOGL_NEED_3DTEXCOORDS;

  sogl_render_sphere(this->radius.getValue(),
                     (int)(SPHERE_NUM_SLICES * complexity),
                     (int)(SPHERE_NUM_STACKS * complexity),
                     &mb,
                     flags, state);
}

// Documented in superclass.
void
SoSphere::computeBBox(SoAction * COIN_UNUSED_ARG(action), SbBox3f & box, SbVec3f & center)
{
  float r = this->radius.getValue();

  // Allow negative values.
  if (r < 0.0f) r = -r;

  box.setBounds(SbVec3f(-r, -r, -r), SbVec3f(r, r, r));
  center.setValue(0.0f, 0.0f, 0.0f);
}

// Documented in superclass.
void
SoSphere::rayPick(SoRayPickAction *action)
{
  if (!shouldRayPick(action)) return;

  sopick_pick_sphere(this->radius.getValue(),
                     action);
}

// Documented in superclass.
void
SoSphere::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  float complexity = this->getComplexityValue(action);
  action->addNumTriangles((int)(complexity*2.0f*SPHERE_NUM_SLICES*(SPHERE_NUM_STACKS-1)));
}

// Documented in superclass.
void
SoSphere::generatePrimitives(SoAction * action)
{
  float complexity = this->getComplexityValue(action);

  sogen_generate_sphere(this->radius.getValue(),
                        (int)(SPHERE_NUM_SLICES * complexity),
                        (int)(SPHERE_NUM_STACKS * complexity),
                        this,
                        action);
}

#undef SPHERE_NUM_SLICES
#undef SPHERE_NUM_STACKS
