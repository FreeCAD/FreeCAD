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
  \class SoVRMLTextureTransform SoVRMLTextureTransform.h Inventor/VRMLnodes/SoVRMLTextureTransform.h
  \brief The SoVRMLTextureTransform class defines a transformation applied to texture coordinates.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  TextureTransform {
    exposedField SFVec2f center      0 0     # (-inf, inf)
    exposedField SFFloat rotation    0       # (-inf, inf)
    exposedField SFVec2f scale       1 1     # (-inf, inf)
    exposedField SFVec2f translation 0 0     # (-inf, inf)
  }
  \endverbatim

  The TextureTransform node defines a 2D transformation that is
  applied to texture coordinates (see SoVRMLTextureCoordinate).  This
  node affects the way textures coordinates are applied to the
  geometric surface. The transformation consists of (in order):

  - a translation;
  - a rotation about the centre point;
  - a non-uniform scale about the centre point.

  These parameters support changes to the size, orientation, and
  position of textures on shapes. Note that these operations appear
  reversed when viewed on the surface of geometry. For example, a
  scale value of (2 2) will scale the texture coordinates and have the
  net effect of shrinking the texture size by a factor of 2 (texture
  coordinates are twice as large and thus cause the texture to
  repeat). A translation of (0.5 0.0) translates the texture
  coordinates +.5 units along the S-axis and has the net effect of
  translating the texture -0.5 along the S-axis on the geometry's
  surface.  A rotation of pi/2 of the texture coordinates results in a
  -pi/2 rotation of the texture on the geometry.

  The \e center field specifies a translation offset in texture
  coordinate space about which the rotation and scale fields are
  applied.  The \e scale field specifies a scaling factor in S and T
  of the texture coordinates about the center point. scale values
  shall be in the range (-pi, pi). The \e rotation field specifies a
  rotation in radians of the texture coordinates about the center
  point after the scale has been applied. A positive rotation value
  makes the texture coordinates rotate counterclockwise about the
  centre, thereby rotating the appearance of the texture itself
  clockwise. The \e translation field specifies a translation of the
  texture coordinates.  

  In matrix transformation notation, where Tc is the untransformed
  texture coordinate, Tc' is the transformed texture coordinate, C
  (center), T (translation), R (rotation), and S (scale) are the
  intermediate transformation matrices, 
  
  \verbatim
  Tc' = -C × S × R × C × T × Tc
  \endverbatim

  Note that this transformation order is the reverse of the Transform
  node transformation order since the texture coordinates, not the
  texture, are being transformed (i.e., the texture coordinate
  system).

*/

/*!
  \var SoSFVec2f SoVRMLTextureTransform::translation
  Translation value. Default values is (0, 0, 0).
*/

/*!
  \var SoSFFloat SoVRMLTextureTransform::rotation
  Rotation, in radians, around the centre points. Default value is 0.
*/

/*!
  \var SoSFVec2f SoVRMLTextureTransform::scale
  Scale vector about the centre point. Default value is (1, 1).
*/

/*!
  \var SoSFVec2f SoVRMLTextureTransform::center
  Texture centre. Default value is (0.0, 0.0).
*/

#include <Inventor/VRMLnodes/SoVRMLTextureTransform.h>

#include <cmath>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/elements/SoMultiTextureMatrixElement.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbRotation.h>

#include "nodes/SoSubNodeP.h"

SO_NODE_SOURCE(SoVRMLTextureTransform);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLTextureTransform::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLTextureTransform, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLTextureTransform::SoVRMLTextureTransform(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLTextureTransform);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(translation, (0.0f, 0.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(rotation, (0.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(scale, (1.0f, 1.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(center, (0.0f, 0.0f));
}

/*!
  Destructor.
*/
SoVRMLTextureTransform::~SoVRMLTextureTransform()
{
}

// Doc in parent
void
SoVRMLTextureTransform::doAction(SoAction * action)
{
  SbMatrix mat;
  this->makeMatrix(mat);
  SoMultiTextureMatrixElement::mult(action->getState(), this, 0,
                                    mat);
}

// Doc in parent
void
SoVRMLTextureTransform::callback(SoCallbackAction * action)
{
  SoVRMLTextureTransform::doAction((SoAction*)action);
}

// Doc in parent
void
SoVRMLTextureTransform::GLRender(SoGLRenderAction * action)
{
  SoVRMLTextureTransform::doAction((SoAction*)action);
}

// Doc in parent
void
SoVRMLTextureTransform::getMatrix(SoGetMatrixAction * action)
{
  SbMatrix mat;
  this->makeMatrix(mat);
  action->getTextureMatrix().multLeft(mat);
  action->getTextureInverse().multRight(mat.inverse());
}

// Doc in parent
void
SoVRMLTextureTransform::pick(SoPickAction * action)
{
  SoVRMLTextureTransform::doAction((SoAction*)action);
}

//
// generate a matrix based on the fields
//
void
SoVRMLTextureTransform::makeMatrix(SbMatrix & mat) const
{
  SbMatrix tmp;
  SbVec2f c = this->center.isIgnored() ?
    SbVec2f(0.0f, 0.0f) :
    center.getValue();

  mat.makeIdentity();
  mat[3][0] = -c[0];
  mat[3][1] = -c[1];

  SbVec2f thescale = this->scale.getValue();
  if (!this->scale.isIgnored() &&
      thescale != SbVec2f(1.0f, 1.0f)) {
    tmp.makeIdentity();
    tmp[0][0] = thescale[0];
    tmp[1][1] = thescale[1];
    mat.multRight(tmp);
  }
  if (!this->rotation.isIgnored() && (this->rotation.getValue() != 0.0f)) {
    float cosa = (float)cos(this->rotation.getValue());
    float sina = (float)sin(this->rotation.getValue());
    tmp.makeIdentity();
    tmp[0][0] = cosa;
    tmp[1][0] = -sina;
    tmp[0][1] = sina;
    tmp[1][1] = cosa;
    mat.multRight(tmp);
  }
  if (!this->translation.isIgnored()) c+= this->translation.getValue();
  if (c != SbVec2f(0.0f, 0.0f)) {
    tmp.makeIdentity();
    tmp[3][0] = c[0];
    tmp[3][1] = c[1];
    mat.multRight(tmp);
  }
}

#endif // HAVE_VRML97
