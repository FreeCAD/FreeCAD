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
  \class SoTextureCoordinateDefault SoTextureCoordinateDefault.h Inventor/nodes/SoTextureCoordinateDefault.h
  \brief The SoTextureCoordinateDefault class removes texture coordinates from the state.

  \ingroup coin_nodes

  Shapes below this node in the scene graph will have to use its default 
  texture coordinates as SoTextureCoordinateDefault cleans out all previously
  defined texture coordinates and texture coordinate functions.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TextureCoordinateDefault {
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoTextureCoordinateDefault.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

SO_NODE_SOURCE(SoTextureCoordinateDefault);

/*!
  Constructor.
*/
SoTextureCoordinateDefault::SoTextureCoordinateDefault()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTextureCoordinateDefault);
}

/*!
  Destructor.
*/
SoTextureCoordinateDefault::~SoTextureCoordinateDefault()
{
}

// doc in super
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTextureCoordinateDefault::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTextureCoordinateDefault, SO_FROM_INVENTOR_1);
}

// doc from parent
void
SoTextureCoordinateDefault::doAction(SoAction * action)
{
  int unit = SoTextureUnitElement::get(action->getState());
  SoMultiTextureCoordinateElement::setDefault(action->getState(), this, unit);
}

// doc from parent
void
SoTextureCoordinateDefault::GLRender(SoGLRenderAction * action)
{
  /*int unit = */SoTextureUnitElement::get(action->getState());
  SoGLMultiTextureCoordinateElement::setTexGen(action->getState(),
                                               this, 0, NULL);
  SoTextureCoordinateDefault::doAction((SoAction *)action);
}

// doc from parent
void
SoTextureCoordinateDefault::callback(SoCallbackAction * action)
{
  SoTextureCoordinateDefault::doAction((SoAction *)action);
}

// doc from parent
void
SoTextureCoordinateDefault::pick(SoPickAction * action)
{
  SoTextureCoordinateDefault::doAction((SoAction *)action);
}
