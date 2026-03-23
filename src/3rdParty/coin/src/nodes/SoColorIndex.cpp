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
  \class SoColorIndex SoColorIndex.h Inventor/nodes/SoColorIndex.h
  \brief The SoColorIndex class is used to specify color indices for subsequent shapes.

  \ingroup coin_nodes

  This node should only be used in OpenGL color index mode, and only
  when the current light model is set to SoLightModel::BASE_COLOR.

  OpenGL color index mode is where the colors for individual pixels are
  fetched from a color lookup table ("CLUT"). The usual thing to do is
  to set up a canvas in RGBA true color mode.

  One common use for color index mode OpenGL canvases is to use one in
  the overlay planes (which are usually limited to only 2 or 4
  available colors), if supported by the OpenGL hardware and / or
  driver.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    ColorIndex {
        index 1
    }
  \endcode
*/

// FIXME: couldn't we check for the above mentioned pre-conditions
// (color index mode, SoLightModel::BASE_COLOR) and assert() or
// SoDebugError::post() if any of the two is not met?
//
// UPDATE: use glGetBooleanv(GL_RGBA_MODE, ...) or
// SoGLColorIndexElement::isColorIndexMode() for the color-index
// mode-check.
//
// 20010809 mortene.

#include <Inventor/nodes/SoColorIndex.h>

#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoGLColorIndexElement.h>
#include <Inventor/actions/SoGLRenderAction.h>

#include "nodes/SoSubNodeP.h"

/*!
  \var SoMFInt32 SoColorIndex::index
  Color indices which can be used by shapes.
*/

// *************************************************************************

SO_NODE_SOURCE(SoColorIndex);

/*!
  Constructor.
*/
SoColorIndex::SoColorIndex()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoColorIndex);

  SO_NODE_ADD_FIELD(index, (1));
}

/*!
  Destructor.
*/
SoColorIndex::~SoColorIndex()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoColorIndex::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoColorIndex, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoGLRenderAction, SoGLColorIndexElement);
}

// doc in parent
void
SoColorIndex::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  if (!this->index.isIgnored() &&
      !SoOverrideElement::getColorIndexOverride(state)) {
    if (this->isOverride()) {
      SoOverrideElement::setColorIndexOverride(state, this, TRUE);
    }
    SoGLColorIndexElement::set(state, this,
                               this->index.getNum(),
                               this->index.getValues(0));
  }
}
