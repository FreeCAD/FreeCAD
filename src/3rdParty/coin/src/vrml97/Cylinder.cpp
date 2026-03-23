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
  \class SoVRMLCylinder SoVRMLCylinder.h Inventor/VRMLnodes/SoVRMLCylinder.h
  \brief The SoVRMLCylinder class is used to represent a cylinder object.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  Cylinder {
    field    SFBool    bottom  TRUE
    field    SFFloat   height  2         # (0,)
    field    SFFloat   radius  1         # (0,)
    field    SFBool    side    TRUE
    field    SFBool    top     TRUE
  }
  \endverbatim

  The Cylinder node specifies a capped cylinder centred at (0,0,0) in
  the local coordinate system and with a central axis oriented along
  the local Y-axis. By default, the cylinder is sized at "-1" to "+1"
  in all three dimensions. The radius field specifies the radius of
  the cylinder and the height field specifies the height of the
  cylinder along the central axis. Both radius and height shall be
  greater than zero. Figure 6.4 illustrates the Cylinder node.

  The cylinder has three parts: the side, the top (Y = +height/2) and
  the bottom (Y = -height/2).  Each part has an associated SFBool
  field that indicates whether the part exists (TRUE) or does not
  exist (FALSE). Parts which do not exist are not rendered and not
  eligible for intersection tests (e.g., collision detection or sensor
  activation).

  <center>
  <img src="http://www.web3d.org/documents/specifications/14772/V2.0/Images/cylinder.gif">
  Figure 6.4
  </center>


  When a texture is applied to a cylinder, it is applied differently
  to the sides, top, and bottom. On the sides, the texture wraps
  counterclockwise (from above) starting at the back of the
  cylinder. The texture has a vertical seam at the back, intersecting
  the X=0 plane. For the top and bottom caps, a circle is cut out of
  the unit texture squares centred at (0, +/- height/2, 0) with
  dimensions 2 × radius by 2 × radius.  The top texture appears right
  side up when the top of the cylinder is tilted toward the +Z-axis,
  and the bottom texture appears right side up when the top of the
  cylinder is tilted toward the -Z-axis. SoVRMLTextureTransform
  affects the texture coordinates of the Cylinder node.  The Cylinder
  node's geometry requires outside faces only. When viewed from the
  inside the results are undefined.

*/

/*!
  \var SoSFFloat SoVRMLCylinder::radius
  The cylinder radius. Default value is 1.0.
*/

/*!
  \var SoSFFloat SoVRMLCylinder::height
  The cylinder height. Default value is 2.0.
*/

/*!
  \var SoSFBool SoVRMLCylinder::side
  Enable/disable the cylinder side wall. Default value is TRUE.
*/

/*!
  \var SoSFBool SoVRMLCylinder::top
  Enable/disable the cylinder top. Default value is TRUE.
*/

/*!
  \var SoSFBool SoVRMLCylinder::bottom
  Enable/disable the cylinder bottom. Default value is TRUE.
*/

#include <Inventor/VRMLnodes/SoVRMLCylinder.h>
#include "coindefs.h"

#include <cmath>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoComplexityTypeElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/elements/SoGLShapeHintsElement.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoGL.h"
#include "misc/SoGenerate.h"
#include "misc/SoPick.h"

#define CYL_SIDE_NUMTRIS 40.0f

SO_NODE_SOURCE(SoVRMLCylinder);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLCylinder::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLCylinder, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLCylinder::SoVRMLCylinder(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLCylinder);

  SO_VRMLNODE_ADD_FIELD(radius, (1.0f));
  SO_VRMLNODE_ADD_FIELD(height, (2.0f));
  SO_VRMLNODE_ADD_FIELD(side, (TRUE));
  SO_VRMLNODE_ADD_FIELD(top, (TRUE));
  SO_VRMLNODE_ADD_FIELD(bottom, (TRUE));
}

/*!
  Destructor.
*/
SoVRMLCylinder::~SoVRMLCylinder()
{
}

// Doc in parent
void
SoVRMLCylinder::GLRender(SoGLRenderAction * action)
{
  if (!shouldGLRender(action)) return;

  SoState * state = action->getState();

  SoMaterialBundle mb(action);
  mb.sendFirst();

  unsigned int flags = 0;

  SbBool sendNormals = !mb.isColorOnly() ||
    (SoMultiTextureCoordinateElement::getType(state) == SoMultiTextureCoordinateElement::FUNCTION);
  
  if (sendNormals)
    flags |= SOGL_NEED_NORMALS;
  if ((SoGLMultiTextureEnabledElement::get(state)) &&
      SoMultiTextureCoordinateElement::getType(state) != SoMultiTextureCoordinateElement::TEXGEN)
    flags |= SOGL_NEED_TEXCOORDS;
  if (this->side.getValue()) flags |= SOGL_RENDER_SIDE;
  if (this->top.getValue()) flags |= SOGL_RENDER_TOP;
  if (this->bottom.getValue()) flags |= SOGL_RENDER_BOTTOM;

  float complexity = this->getComplexityValue(action);

  // enable back face culling
  SoGLShapeHintsElement::forceSend(state, TRUE, TRUE);

  sogl_render_cylinder(this->radius.getValue(),
                       this->height.getValue(),
                       (int)(CYL_SIDE_NUMTRIS * complexity),
                       &mb,
                       flags, state);
}

// Doc in parent
void
SoVRMLCylinder::rayPick(SoRayPickAction * action)
{
  if (!shouldRayPick(action)) return;

  unsigned int flags = 0;
  if (this->side.getValue()) flags |= SOPICK_SIDES;
  if (this->top.getValue()) flags |= SOPICK_TOP;
  if (this->bottom.getValue()) flags |= SOPICK_BOTTOM;

  sopick_pick_cylinder(this->radius.getValue(),
                       this->height.getValue(),
                       flags,
                       this, action);
}

// Doc in parent
void
SoVRMLCylinder::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  float complexity = this->getComplexityValue(action);
  int numtris = (int)(complexity * CYL_SIDE_NUMTRIS);

  if (this->bottom.getValue()) {
    action->addNumTriangles(numtris-2);
  }
  if (this->top.getValue()) {
    action->addNumTriangles(numtris-2);
  }
  if (this->side.getValue()) {
    action->addNumTriangles(numtris * 2);
  }
}

// Doc in parent
void
SoVRMLCylinder::generatePrimitives(SoAction * action)
{
  unsigned int flags = 0;
  if (this->side.getValue()) flags |= SOGEN_GENERATE_SIDE;
  if (this->bottom.getValue()) flags |= SOGEN_GENERATE_BOTTOM;
  if (this->top.getValue()) flags |= SOGEN_GENERATE_TOP;

  float complexity = this->getComplexityValue(action);

  sogen_generate_cylinder(this->radius.getValue(),
                          this->height.getValue(),
                          (int)(CYL_SIDE_NUMTRIS * complexity),
                          flags,
                          this,
                          action);
}

// Doc in parent
void
SoVRMLCylinder::computeBBox(SoAction * COIN_UNUSED_ARG(action),
                            SbBox3f & box,
                            SbVec3f & center)
{
  float r = this->radius.getValue();
  float h = this->height.getValue();

  // Allow negative values.
  if (r < 0.0f) r = -r;
  if (h < 0.0f) h = -h;

  // Either the SIDES are present, or we've at least got both the TOP
  // and BOTTOM caps -- so just find the middle point and enclose
  // everything.
  if (this->side.getValue() ||
      (this->bottom.getValue() &&
       this->top.getValue())) {
    center.setValue(0.0f, 0.0f, 0.0f);
    box.setBounds(SbVec3f(-r, -h/2.0f, -r), SbVec3f(r, h/2.0f, r));
  }
  // ..not a "full" cylinder, but we've got the BOTTOM cap.
  else if (this->bottom.getValue()) {
    center.setValue(0.0f, -h/2.0f, 0.0f);
    box.setBounds(SbVec3f(-r, -h/2.0f, -r), SbVec3f(r, -h/2.0f, r));
  }
  // ..not a "full" cylinder, but we've got the TOP cap.
  else if (this->top.getValue()) {
    center.setValue(0.0f, h/2.0f, 0.0f);
    box.setBounds(SbVec3f(-r, h/2.0f, -r), SbVec3f(r, h/2.0f, r));
  }
  // ..no parts present. My confidence is shot -- I feel very small.
  else {
    center.setValue(0.0f, 0.0f, 0.0f);
    box.setBounds(SbVec3f(0.0f, 0.0f, 0.0f), SbVec3f(0.0f, 0.0f, 0.0f));
  }
}

#endif // HAVE_VRML97
