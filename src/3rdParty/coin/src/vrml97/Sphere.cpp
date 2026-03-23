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
  \class SoVRMLSphere SoVRMLSphere.h Inventor/VRMLnodes/SoVRMLSphere.h
  \brief The SoVRMLSphere class is used to represent a spherical 3D object.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  Sphere {
    field SFFloat radius  1    # (0, inf)
  }
  \endverbatim

  The Sphere node specifies a sphere centred at (0, 0, 0) in the local
  coordinate system. The radius field specifies the radius of the
  sphere and shall be greater than zero. Figure 6.15 depicts the
  fields of the Sphere node.

  <center>
  <img src="http://www.web3d.org/documents/specifications/14772/V2.0/Images/sphere.gif">
  Figure 6.15 -- Sphere node
  </center>

  When a texture is applied to a sphere, the texture covers the entire
  surface, wrapping counterclockwise from the back of the sphere
  (i.e., longitudinal arc intersecting the -Z-axis) when viewed from
  the top of the sphere. The texture has a seam at the back where the
  X=0 plane intersects the sphere and Z values are
  negative. TextureTransform affects the texture coordinates of the
  Sphere.  The Sphere node's geometry requires outside faces
  only. When viewed from the inside the results are undefined.

*/

/*!
  \var SoSFFloat SoVRMLSphere::radius
  Sphere radius. Default value is 1.0.
*/

#include <Inventor/VRMLnodes/SoVRMLSphere.h>
#include "coindefs.h"

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/elements/SoGLShapeHintsElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>

#include "nodes/SoSubNodeP.h"
#include "misc/SoGenerate.h"
#include "misc/SoPick.h"
#include "rendering/SoGL.h"

#define SPHERE_NUM_SLICES 30.0f
#define SPHERE_NUM_STACKS 30.0f

SO_NODE_SOURCE(SoVRMLSphere);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLSphere::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLSphere, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLSphere::SoVRMLSphere(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLSphere);

  SO_VRMLNODE_ADD_FIELD(radius, (1.0f));
}

/*!
  Destructor.
*/
SoVRMLSphere::~SoVRMLSphere()
{
}

// Doc in parent
void
SoVRMLSphere::GLRender(SoGLRenderAction * action)
{
  if (!shouldGLRender(action)) return;

  SoState * state = action->getState();

  SoMaterialBundle mb(action);
  mb.sendFirst();

  SbBool doTextures = SoGLMultiTextureEnabledElement::get(state);

  SbBool sendNormals = !mb.isColorOnly() ||
    (SoMultiTextureCoordinateElement::getType(state) == SoMultiTextureCoordinateElement::FUNCTION);
  
  float complexity = this->getComplexityValue(action);

  unsigned int flags = 0;
  if (sendNormals) flags |= SOGL_NEED_NORMALS;
  if (doTextures) flags |= SOGL_NEED_TEXCOORDS;

  // enable back face culling
  SoGLShapeHintsElement::forceSend(state, TRUE, TRUE);

  sogl_render_sphere(this->radius.getValue(),
                     (int)(SPHERE_NUM_SLICES * complexity),
                     (int)(SPHERE_NUM_STACKS * complexity),
                     &mb,
                     flags, state);
}

// Doc in parent
void
SoVRMLSphere::rayPick(SoRayPickAction * action)
{
  if (!shouldRayPick(action)) return;

  sopick_pick_sphere(this->radius.getValue(),
                     action);
}

// Doc in parent
void
SoVRMLSphere::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  float complexity = this->getComplexityValue(action);
  action->addNumTriangles((int)(complexity*2.0f*SPHERE_NUM_SLICES*(SPHERE_NUM_STACKS-1)));
}

void
SoVRMLSphere::generatePrimitives(SoAction * action)
{
  float complexity = this->getComplexityValue(action);

  sogen_generate_sphere(this->radius.getValue(),
                        (int)(SPHERE_NUM_SLICES * complexity),
                        (int)(SPHERE_NUM_STACKS * complexity),
                        this,
                        action);
}

// Doc in parent
void
SoVRMLSphere::computeBBox(SoAction * COIN_UNUSED_ARG(action),
                          SbBox3f & box,
                          SbVec3f & center)
{
  float r = this->radius.getValue();

  // Allow negative values.
  if (r < 0.0f) r = -r;

  box.setBounds(SbVec3f(-r, -r, -r), SbVec3f(r, r, r));
  center.setValue(0.0f, 0.0f, 0.0f);
}

#undef SPHERE_NUM_SLICES
#undef SPHERE_NUM_STACKS

#endif // HAVE_VRML97
