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
  \class SoVRMLCone SoVRMLCone.h Inventor/VRMLnodes/SoVRMLCone.h
  \brief The SoVRMLCone class is used to represent a Cone object.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  Cone {
    field     SFFloat   bottomRadius 1        # (0, inf)
    field     SFFloat   height       2        # (0, inf)
    field     SFBool    side         TRUE
    field     SFBool    bottom       TRUE
  }
  \endverbatim

  The Cone node specifies a cone which is centred in the local
  coordinate system and whose central axis is aligned with the local
  Y-axis. The bottomRadius field specifies the radius of the cone's
  base, and the height field specifies the height of the cone from the
  centre of the base to the apex.  By default, the cone has a radius
  of 1.0 at the bottom and a height of 2.0, with its apex at y =
  height/2 and its bottom at y = -height/2.  Both bottomRadius and
  height shall be greater than zero. Figure 6.3 illustrates the Cone
  node.

  <center>
  <img src="http://www.web3d.org/documents/specifications/14772/V2.0/Images/cone.gif">
  Figure 6.3
  </center>

  The side field specifies whether sides of the cone are created and
  the bottom field specifies whether the bottom cap of the cone is
  created. A value of TRUE specifies that this part of the cone
  exists, while a value of FALSE specifies that this part does not
  exist (not rendered or eligible for collision or sensor intersection
  tests).  When a texture is applied to the sides of the cone, the
  texture wraps counterclockwise (from above) starting at the back of
  the cone. The texture has a vertical seam at the back in the X=0
  plane, from the apex (0, height/2, 0) to the point (0, -height/2, -
  bottomRadius). For the bottom cap, a circle is cut out of the
  texture square centred at (0, -height/2, 0) with dimensions (2 ×
  bottomRadius) by (2 × bottomRadius).  The bottom cap texture appears
  right side up when the top of the cone is rotated towards the
  -Z-axis. SoVRMLTextureTransform affects the texture coordinates of
  the Cone.

  The Cone geometry requires outside faces only. When viewed from the
  inside the results are undefined.

*/

/*!
  \var SoSFFloat SoVRMLCone::bottomRadius
  The cone bottom radius. Default value is 1.0.
*/

/*!
  \var SoSFFloat SoVRMLCone::height
  The cone height. Default value is 2.0.
*/

/*!
  \var SoSFBool SoVRMLCone::side
  Enable/disable the cone side wall. Default value is TRUE.
*/

/*!
  \var SoSFBool SoVRMLCone::bottom
  Enable/disable the cone bottom. Default value is TRUE.
*/

#include <Inventor/VRMLnodes/SoVRMLCone.h>
#include "coindefs.h"

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/details/SoConeDetail.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/elements/SoGLShapeHintsElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoGL.h"
#include "misc/SoGenerate.h"
#include "misc/SoPick.h"

SO_NODE_SOURCE(SoVRMLCone);

#define CONE_SIDE_NUMTRIS 40.0f

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLCone::initClass(void) // static
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLCone, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLCone::SoVRMLCone(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLCone);

  SO_VRMLNODE_ADD_FIELD(bottomRadius, (1.0f));
  SO_VRMLNODE_ADD_FIELD(height, (2.0f));

  SO_VRMLNODE_ADD_FIELD(side, (TRUE));
  SO_VRMLNODE_ADD_FIELD(bottom, (TRUE));
}

/*!
  Destructor.
*/
SoVRMLCone::~SoVRMLCone()
{
}

// Doc in parent
void
SoVRMLCone::GLRender(SoGLRenderAction * action)
{
  if (!shouldGLRender(action)) return;

  SoState * state = action->getState();
  
  SbBool doTextures = SoGLMultiTextureEnabledElement::get(state);

  SoMaterialBundle mb(action);
  mb.sendFirst();

  SbBool sendNormals = !mb.isColorOnly() ||
    (SoMultiTextureCoordinateElement::getType(state) == SoMultiTextureCoordinateElement::FUNCTION);

  unsigned int flags = 0;
  if (doTextures) flags |= SOGL_NEED_TEXCOORDS;
  if (sendNormals) flags |= SOGL_NEED_NORMALS;

  if (this->side.getValue()) flags |= SOGL_RENDER_SIDE;
  if (this->bottom.getValue()) flags |= SOGL_RENDER_BOTTOM;

  float complexity = this->getComplexityValue(action);

  // enable back face culling
  SoGLShapeHintsElement::forceSend(state, TRUE, TRUE);

  sogl_render_cone(this->bottomRadius.getValue(),
                   this->height.getValue(),
                   (int)(CONE_SIDE_NUMTRIS * complexity),
                   &mb,
                   flags, state);
}

// Doc in parent
void
SoVRMLCone::rayPick(SoRayPickAction * action)
{
  if (!this->shouldRayPick(action)) return;

  unsigned int flags = 0;
  if (this->side.getValue()) flags |= SOPICK_SIDES;
  if (this->bottom.getValue()) flags |= SOPICK_BOTTOM;

  sopick_pick_cone(this->bottomRadius.getValue(),
                   this->height.getValue(),
                   flags, this, action);
}

// Doc in parent
void
SoVRMLCone::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  float complexity = this->getComplexityValue(action);
  int numtris = (int)(complexity * CONE_SIDE_NUMTRIS);

  if (this->bottom.getValue()) {
    action->addNumTriangles(numtris-2);
  }
  if (this->side.getValue()) {
    action->addNumTriangles(numtris);
  }
}

// Doc in parent
void
SoVRMLCone::generatePrimitives(SoAction * action)
{
  unsigned int flags = 0;
  if (this->side.getValue()) flags |= SOGEN_GENERATE_SIDE;
  if (this->bottom.getValue()) flags |= SOGEN_GENERATE_BOTTOM;

  float complexity = this->getComplexityValue(action);

  sogen_generate_cone(this->bottomRadius.getValue(),
                      this->height.getValue(),
                      (int)(CONE_SIDE_NUMTRIS * complexity),
                      flags,
                      this,
                      action);
}

// Doc in parent
void
SoVRMLCone::computeBBox(SoAction * COIN_UNUSED_ARG(action),
                        SbBox3f & box,
                        SbVec3f & center)
{
  float r = this->bottomRadius.getValue();
  float h = this->height.getValue();

  // Allow negative values.
  if (h < 0.0f) h = -h;
  if (r < 0.0f) r = -r;

  float half_height = h * 0.5f;

  // The SIDES are present, so just find the middle point and enclose
  // everything.
  if (this->side.getValue()) {
    center.setValue(0.0f, 0.0f, 0.0f);
    box.setBounds(SbVec3f(-r, -half_height, -r), SbVec3f(r, half_height, r));
  }
  // ..no SIDES, but we've still got the bottom (NB: OIV misses this case).
  else if (this->bottom.getValue()) {
    center.setValue(0.0f, -half_height, 0.0f);
    box.setBounds(SbVec3f(-r, -half_height, -r), SbVec3f(r, -half_height, r));
  }
  // ..no parts present. My confidence is shot -- I feel very small.
  else {
    center.setValue(0.0f, 0.0f, 0.0f);
    box.setBounds(SbVec3f(0.0f, 0.0f, 0.0f), SbVec3f(0.0f, 0.0f, 0.0f));
  }
}

#undef CONE_SIDE_NUMTRIS

#endif // HAVE_VRML97
