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
  \class SoTextureCoordinateObject SoTextureCoordinateObject.h Inventor/nodes/SoTextureCoordinateObject.h
  \brief The SoTextureCoordinateObject class generates texture coordinates by...

  \ingroup coin_nodes

  FIXME: not implemented yet. pederb, 2005-04-20

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TextureCoordinateObject {
        factorS 1 0 0 0
        factorT 0 1 0 0
        factorR 0 0 1 0
        factorQ 0 0 0 1
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoTextureCoordinateObject.h>
#include "coindefs.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/system/gl.h>
#include <Inventor/C/glue/gl.h>

#include "nodes/SoSubNodeP.h"

/*!
  \var SoSFVec3f SoTextureCoordinateObject::factorS
  
  FIXME doc.
*/
/*!
  \var SoSFVec3f SoTextureCoordinateObject::factorT
  
  FIXME doc.
*/
/*!
  \var SoSFVec3f SoTextureCoordinateObject::factorR
  
  FIXME doc.
*/
/*!
  \var SoSFVec3f SoTextureCoordinateObject::factorQ
  
  FIXME doc.
*/

// *************************************************************************

SO_NODE_SOURCE(SoTextureCoordinateObject);

/*!
  Constructor.
*/
SoTextureCoordinateObject::SoTextureCoordinateObject()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTextureCoordinateObject);

  SO_NODE_ADD_FIELD(factorS, (1.0f, 0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(factorT, (0.0f, 1.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(factorR, (0.0f, 0.0f, 1.0f, 0.0f));
  SO_NODE_ADD_FIELD(factorQ, (0.0f, 0.0f, 0.0f, 1.0f));
}

/*!
  Destructor.
*/
SoTextureCoordinateObject::~SoTextureCoordinateObject()
{
}

// doc from parent
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTextureCoordinateObject::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTextureCoordinateObject, SO_FROM_INVENTOR_2_0);
}

// generates texture coordinates for callback and raypick action
const SbVec4f &
SoTextureCoordinateObject::generate(void * userdata,
                                    const SbVec3f & p,
                                    const SbVec3f & COIN_UNUSED_ARG(n))
{
  SoTextureCoordinateObject *thisp =
    (SoTextureCoordinateObject*) userdata;
  
  SbVec4f p4(p[0], p[1], p[2], 1.0f);
  thisp->dummy_object.setValue(thisp->factorS.getValue().dot(p4),
                               thisp->factorT.getValue().dot(p4),
                               thisp->factorR.getValue().dot(p4),
                               thisp->factorQ.getValue().dot(p4));
  return thisp->dummy_object;
}

// doc from parent
void
SoTextureCoordinateObject::doAction(SoAction * action)
{
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state);
  SoMultiTextureCoordinateElement::setFunction(action->getState(), this, unit,
                                               SoTextureCoordinateObject::generate,
                                               this);
}

// doc from parent
void
SoTextureCoordinateObject::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state);

  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));
  int maxunits = cc_glglue_max_texture_units(glue);
  if (unit < maxunits) {        
    SoGLMultiTextureCoordinateElement::setTexGen(action->getState(),
                                                 this, unit,
                                                 SoTextureCoordinateObject::handleTexgen,
                                                 this,
                                                 SoTextureCoordinateObject::generate,
                                                 this);
  }
}

// doc from parent
void
SoTextureCoordinateObject::callback(SoCallbackAction * action)
{
  SoTextureCoordinateObject::doAction((SoAction *)action);
}

// doc from parent
void
SoTextureCoordinateObject::pick(SoPickAction * action)
{
  SoTextureCoordinateObject::doAction((SoAction *)action);
}

// texgen callback. Turns on plane texgen in OpenGL
void
SoTextureCoordinateObject::handleTexgen(void * data)
{
  SoTextureCoordinateObject *thisp = (SoTextureCoordinateObject*)data;
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

  const SbVec4f & s = thisp->factorS.getValue();
  glTexGenfv(GL_S, GL_OBJECT_PLANE, s.getValue());

  const SbVec4f & t = thisp->factorT.getValue();
  glTexGenfv(GL_T, GL_OBJECT_PLANE, t.getValue());
  
  const SbVec4f & r = thisp->factorR.getValue();
  glTexGenfv(GL_R, GL_OBJECT_PLANE, r.getValue());

  const SbVec4f & q = thisp->factorQ.getValue();
  glTexGenfv(GL_Q, GL_OBJECT_PLANE, q.getValue());
}
