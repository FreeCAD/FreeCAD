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
  \class SoTextureCoordinatePlane SoTextureCoordinatePlane.h Inventor/nodes/SoTextureCoordinatePlane.h
  \brief The SoTextureCoordinatePlane class generates texture coordinates by projecting onto a plane.

  \ingroup coin_nodes

  SoTextureCoordinatePlane is used for generating texture coordinates
  by projecting the object onto a texture plane.  The s, t and r
  texture coordinates are computed as the distance from the origin to
  the projected point, in the respective directions. The texture plane
  is specified using two direction vectors, given as
  SoTextureCoordinatePlane::directionS and
  SoTextureCoordinatePlane::directionT in object space coordinates.
  SoTextureCoordinatePlane::directionR is used for generating the
  third coordinate, and can be used for generating texture coordinate
  for 3D textures. For 2D textures you can just leave this field alone.

  The length of the vector determines the repeat interval of the
  texture per unit length.

  A simple usage example:

  \code
  SoSeparator *root = new SoSeparator;
  root->ref();

  // the texture image
  SoTexture2 *tex = new SoTexture2;
  tex->filename.setValue("foo.png");
  root->addChild(tex);

  // the texture plane
  SoTextureCoordinatePlane *texPlane = new SoTextureCoordinatePlane;
  texPlane->directionS.setValue(SbVec3f(1,0,0));
  texPlane->directionT.setValue(SbVec3f(0,1,0));
  root->addChild(texPlane);

  // add a simple cube
  SoCube * c = new SoCube;
  c->width.setValue(1.0);
  c->height.setValue(1.0)
  c->depth.setValue(1.0);
  root->addChild(new SoCube);
  \endcode

  Here, we are projecting a texture onto a cube. The texture
  coordinate plane is specified by directionS = (1,0,0) and directionT
  = (0,1,0), meaning that it is parallel to the front face of the
  cube. Setting e.g. directionS = (0,1,0) and directionT = (-1,0,0)
  would rotate the texture counterclockwise by 90 degrees. Setting
  them to ((2,0,0), (0,2,0)) results to the texture being repeated twice
  per unit, so the texture appears four times on the 1x1 face.

  Note that when you transform the cube, the transformation will also
  affect the texture - it will be transformed vs. the rest of the
  world, but appear "fixed" on the object. If you want to change the
  placement of the texture on the object, you have to insert a
  SoTexture2Transform node before the texture coordinate plane. For
  instance in the example above, since the cube is centered in its
  coordinate system, the lower left corner of the texture appears to
  be in the middle of the face. To move the texture's origin to
  coincide with the lower left corner of the face, insert

  \code
  SoTexture2Transform * tf = new SoTexture2Transform;
  tf->translation.setValue(-0.5,-0.5);
  root->addChild(tf);
  \endcode

  before adding the texture coordinate plane.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TextureCoordinatePlane {
        directionS 1 0 0
        directionT 0 1 0
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoTextureCoordinatePlane.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/system/gl.h>
#include <Inventor/C/glue/gl.h>

#include "nodes/SoSubNodeP.h"

/*!
  \var SoSFVec3f SoTextureCoordinatePlane::directionS
  The S texture coordinate plane direction.
  The length of the vector determines the repeat interval of the
  texture per unit length.

*/
/*!
  \var SoSFVec3f SoTextureCoordinatePlane::directionT
  The T texture coordinate plane direction.
  The length of the vector determines the repeat interval of the
  texture per unit length.
*/

/*!
  \var SoSFVec3f SoTextureCoordinatePlane::directionR
  The R texture coordinate plane direction.
  The length of the vector determines the repeat interval of the
  texture per unit length.
*/


// *************************************************************************

class SoTextureCoordinatePlaneP {
public:
  SbVec3f s, t, r;
  SbVec4f ret;
};

// *************************************************************************

#define PRIVATE(obj) obj->pimpl


SO_NODE_SOURCE(SoTextureCoordinatePlane);

/*!
  Constructor.
*/
SoTextureCoordinatePlane::SoTextureCoordinatePlane()
{
  PRIVATE(this) = new SoTextureCoordinatePlaneP;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoTextureCoordinatePlane);

  SO_NODE_ADD_FIELD(directionS, (1.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(directionT, (0.0f, 1.0f, 0.0f));
  SO_NODE_ADD_FIELD(directionR, (0.0f, 0.0f, 1.0f));
}

/*!
  Destructor.
*/
SoTextureCoordinatePlane::~SoTextureCoordinatePlane()
{
  delete PRIVATE(this);
}

// doc from parent
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTextureCoordinatePlane::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTextureCoordinatePlane, SO_FROM_INVENTOR_2_0);
}

// generates texture coordinates for callback and raypick action
const SbVec4f &
SoTextureCoordinatePlane::generate(void * userdata,
                                   const SbVec3f &p,
                                   const SbVec3f & /* n */)
{
  SoTextureCoordinatePlane *thisp =
    (SoTextureCoordinatePlane*) userdata;
  
  PRIVATE(thisp)->ret.setValue(PRIVATE(thisp)->s.dot(p),
                               PRIVATE(thisp)->t.dot(p),
                               PRIVATE(thisp)->r.dot(p), 
                               1.0f);
  return PRIVATE(thisp)->ret;
}

// doc from parent
void
SoTextureCoordinatePlane::doAction(SoAction * action)
{
  this->setupGencache();
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state);
  SoMultiTextureCoordinateElement::setFunction(action->getState(), this, unit,
                                               SoTextureCoordinatePlane::generate,
                                               this);
}

// doc from parent
void
SoTextureCoordinatePlane::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state);

  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));
  int maxunits = cc_glglue_max_texture_units(glue);
  if (unit < maxunits) {
    this->setupGencache();
    SoGLMultiTextureCoordinateElement::setTexGen(action->getState(),
                                                 this, unit,
                                                 SoTextureCoordinatePlane::handleTexgen,
                                                 this,
                                                 SoTextureCoordinatePlane::generate,
                                                 this);
  }
}

// doc from parent
void
SoTextureCoordinatePlane::callback(SoCallbackAction * action)
{
  SoTextureCoordinatePlane::doAction((SoAction *)action);
}

// doc from parent
void
SoTextureCoordinatePlane::pick(SoPickAction * action)
{
  SoTextureCoordinatePlane::doAction((SoAction *)action);
}

// texgen callback. Turns on plane texgen in OpenGL
void
SoTextureCoordinatePlane::handleTexgen(void *data)
{
  SoTextureCoordinatePlane *thisp = (SoTextureCoordinatePlane*)data;
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

  float plane[4];
  plane[3] = 0.0f;
  const SbVec3f & s = thisp->directionS.getValue();
  plane[0] = s[0];
  plane[1] = s[1];
  plane[2] = s[2];
  glTexGenfv(GL_S, GL_OBJECT_PLANE, plane);
  const SbVec3f & t = thisp->directionT.getValue();
  plane[0] = t[0];
  plane[1] = t[1];
  plane[2] = t[2];
  glTexGenfv(GL_T, GL_OBJECT_PLANE, plane);

  const SbVec3f & r = thisp->directionR.getValue();
  plane[0] = r[0];
  plane[1] = r[1];
  plane[2] = r[2];
  glTexGenfv(GL_R, GL_OBJECT_PLANE, plane);

  // supply dummy plane for Q so that texture generation works
  // properly
  plane[0] = 0.0f;
  plane[1] = 0.0f;
  plane[2] = 0.0f;
  plane[3] = 1.0f;
  glTexGenfv(GL_Q, GL_OBJECT_PLANE, plane);
}

void
SoTextureCoordinatePlane::setupGencache(void)
{
  PRIVATE(this)->s = this->directionS.getValue();
  PRIVATE(this)->t = this->directionT.getValue();
  PRIVATE(this)->r = this->directionR.getValue();
}

#undef PRIVATE
