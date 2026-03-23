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
  \class SoBaseColor SoBaseColor.h Inventor/nodes/SoBaseColor.h
  \brief The SoBaseColor class provides a node type for convenient setting of the base material color.

  \ingroup coin_nodes

  If you want to just set the diffuse color of the following geometry,
  you can use this node for simplicity.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    BaseColor {
        rgb 0.8 0.8 0.8
    }
  \endcode

  \sa SoMaterial
*/

#include <Inventor/nodes/SoBaseColor.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoDiffuseColorElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbStorage.h>
#endif // COIN_THREADSAFE

#include "rendering/SoVBO.h"
#include "nodes/SoSubNodeP.h"

/*!
  \var SoMFColor SoBaseColor::rgb

  Color values. Default value of field is to have a single gray color.
*/

#ifndef DOXYGEN_SKIP_THIS

class SoBaseColorP {
 public:
  SoBaseColorP() : 
#ifdef COIN_THREADSAFE
    colorpacker_storage(sizeof(void*), alloc_colorpacker, free_colorpacker),
#endif // COIN_THREADSAFE
    vbo(NULL) { }
  ~SoBaseColorP() { delete this->vbo; }

#ifdef COIN_THREADSAFE
  SbStorage colorpacker_storage;
#else // COIN_THREADSAFE
  SoColorPacker single_colorpacker;
#endif // COIN_THREADSAFE
  
  SoColorPacker * getColorPacker(void) {
#ifdef COIN_THREADSAFE
    SoColorPacker ** cptr = (SoColorPacker**) this->colorpacker_storage.get();
    return * cptr;
#else // COIN_THREADSAFE
    return &this->single_colorpacker;
#endif // COIN_THREADSAFE
  }
  SoVBO * vbo;
#ifdef COIN_THREADSAFE
private:
  static void alloc_colorpacker(void * data) {
    SoColorPacker ** cptr = (SoColorPacker**) data;
    *cptr = new SoColorPacker;
  }
  static void free_colorpacker(void * data) {
    SoColorPacker ** cptr = (SoColorPacker**) data;
    delete *cptr;
  }
#endif // COIN_THREADSAFE
};

#endif // DOXYGEN_SKIP_THIS

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

SO_NODE_SOURCE(SoBaseColor);

/*!
  Constructor.
*/
SoBaseColor::SoBaseColor()
{
  PRIVATE(this) = new SoBaseColorP;
  SO_NODE_INTERNAL_CONSTRUCTOR(SoBaseColor);

  SO_NODE_ADD_FIELD(rgb, (SbColor(0.8f, 0.8f, 0.8f)));
}

/*!
  Destructor.
*/
SoBaseColor::~SoBaseColor()
{
  delete PRIVATE(this);
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoBaseColor::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoBaseColor, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoGLRenderAction, SoGLLazyElement);
  SO_ENABLE(SoCallbackAction, SoLazyElement);

  SO_ENABLE(SoCallbackAction, SoDiffuseColorElement);
  SO_ENABLE(SoGLRenderAction, SoDiffuseColorElement);
}

// Doc from superclass.
void
SoBaseColor::GLRender(SoGLRenderAction * action)
{
  SoBaseColor::doAction(action);
}

// Doc from superclass.
void
SoBaseColor::doAction(SoAction * action)
{
  SoState * state = action->getState();
  
  if (!this->rgb.isIgnored() && this->rgb.getNum() &&
      !SoOverrideElement::getDiffuseColorOverride(state)) {
    const int num = this->rgb.getNum();
    SoLazyElement::setDiffuse(state, this, num,
                              this->rgb.getValues(0), PRIVATE(this)->getColorPacker());
    
    if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
      SbBool setvbo = FALSE;
      SoBase::staticDataLock();
      if (SoGLVBOElement::shouldCreateVBO(state, num)) {
        setvbo = TRUE;
        if (PRIVATE(this)->vbo == NULL) {
          PRIVATE(this)->vbo = new SoVBO(GL_ARRAY_BUFFER, GL_STATIC_DRAW);
        }
      }
      else if (PRIVATE(this)->vbo) {
        PRIVATE(this)->vbo->setBufferData(NULL, 0, 0);
      }
      // don't fill in any data in the VBO. Data will be filled in
      // using the ColorPacker right before the VBO is used
      SoBase::staticDataUnlock();
      if (setvbo) {
        SoGLVBOElement::setColorVBO(state, PRIVATE(this)->vbo);
      }
    }
    if (this->isOverride()) {
      SoOverrideElement::setDiffuseColorOverride(state, this, TRUE);
    }
  }
}

// Doc from superclass.
void
SoBaseColor::callback(SoCallbackAction * action)
{
  SoBaseColor::doAction(action);
}

#undef PRIVATE
