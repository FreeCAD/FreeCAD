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
  \class SoGLMultiTextureImageElement Inventor/elements/SoGLMultiTextureImageElement.h
  \brief The SoGLMultiTextureImageElement class is used to control the current GL texture for texture units.

  \ingroup coin_elements

  FIXME: write doc.
*/

// *************************************************************************

#include <Inventor/elements/SoGLMultiTextureImageElement.h>

#include <cstdlib>

#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLDisplayList.h>
#include <Inventor/elements/SoTextureCombineElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoGLShaderProgramElement.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/misc/SoGLImage.h>
#include <Inventor/misc/SoGLBigImage.h>
#include <Inventor/SbImage.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/lists/SbList.h>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/system/gl.h>
#include <Inventor/C/glue/gl.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "shaders/SoGLShaderProgram.h"
#include "rendering/SoGL.h" // GL wrapper.

// *************************************************************************

#define PRIVATE(obj) obj->pimpl

class SoGLMultiTextureImageElementP {
public:
  void ensureCapacity(int unit) const {
    while (unit >= this->unitdata.getLength()) {
      this->unitdata.append(SoGLMultiTextureImageElement::GLUnitData());
    }
  }

  SoGLMultiTextureImageElement::GLUnitData defaultdata;
  mutable SbList<SoGLMultiTextureImageElement::GLUnitData> unitdata;
  SoState * state;
  uint32_t cachecontext;
};

SO_ELEMENT_CUSTOM_CONSTRUCTOR_SOURCE(SoGLMultiTextureImageElement);


/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLMultiTextureImageElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLMultiTextureImageElement, inherited);
}

/*!
  Constructor.
*/

SoGLMultiTextureImageElement::SoGLMultiTextureImageElement(void)
{
  PRIVATE(this) = new SoGLMultiTextureImageElementP;

  this->setTypeId(SoGLMultiTextureImageElement::classTypeId);
  this->setStackIndex(SoGLMultiTextureImageElement::classStackIndex);
}

/*!
  Destructor.
*/

SoGLMultiTextureImageElement::~SoGLMultiTextureImageElement(void)
{
  delete PRIVATE(this);
}

// doc from parent
void
SoGLMultiTextureImageElement::init(SoState * state)
{
  inherited::init(state);

  SoAction * action = state->getAction();
  assert(action->isOfType(SoGLRenderAction::getClassTypeId()));

  // fetch cache context from action since SoGLCacheContextElement
  // might not be initialized yet.
  SoGLRenderAction * glaction = (SoGLRenderAction*) action;
  PRIVATE(this)->cachecontext = glaction->getCacheContext();
  PRIVATE(this)->state = state;
}


// Documented in superclass. Overridden to pass GL state to the next
// element.
void
SoGLMultiTextureImageElement::push(SoState * state)
{
  inherited::push(state);
  SoGLMultiTextureImageElement * prev = (SoGLMultiTextureImageElement*)
    this->getNextInStack();
  PRIVATE(this)->state = state;
  PRIVATE(this)->cachecontext = PRIVATE(prev)->cachecontext;
  PRIVATE(this)->unitdata = PRIVATE(prev)->unitdata;
  
  // capture previous element since we might or might not change the
  // GL state in set/pop
  prev->capture(state);
}


// Documented in superclass. Overridden to pass GL state to the
// previous element.
void
SoGLMultiTextureImageElement::pop(SoState * state,
                                  const SoElement * prevTopElement)
{
  inherited::pop(state, prevTopElement);
  SoGLMultiTextureImageElement * prev = (SoGLMultiTextureImageElement*)
    prevTopElement;

  SoGLShaderProgram * prog = SoGLShaderProgramElement::get(state);
  SbString str;
  
  const int maxunits = SbMax(PRIVATE(prev)->unitdata.getLength(),
                             PRIVATE(this)->unitdata.getLength());

  for (int i = 0; i < maxunits; i++) {
    const GLUnitData & prevud = 
      (i < PRIVATE(prev)->unitdata.getLength()) ?
      PRIVATE(prev)->unitdata[i] :
      PRIVATE(prev)->defaultdata;
    
    const GLUnitData & thisud = 
      (i < PRIVATE(this)->unitdata.getLength()) ?
      PRIVATE(this)->unitdata[i] :
      PRIVATE(this)->defaultdata;

    if (thisud.glimage != prevud.glimage) this->updateGL(i);
    str.sprintf("coin_texunit%d_model", i);
    if (prog) prog->updateCoinParameter(state, SbName(str.getString()),
                                        thisud.glimage != NULL ? this->getUnitData(i).model : 0);
  }
}

static SoMultiTextureImageElement::Wrap
multi_translateWrap(const SoGLImage::Wrap wrap)
{
  if (wrap == SoGLImage::REPEAT) return SoMultiTextureImageElement::REPEAT;
  return SoMultiTextureImageElement::CLAMP;
}

/*!
  Sets the current texture. Id \a didapply is TRUE, it is assumed
  that the texture image already is the current GL texture. Do not
  use this feature unless you know what you're doing.
*/
void
SoGLMultiTextureImageElement::set(SoState * const state, SoNode * const node,
                                  const int unit,
                                  SoGLImage * image,
                                  Model model,
                                  const SbColor & blendColor)
{
  SoGLMultiTextureImageElement * elem = (SoGLMultiTextureImageElement*)
    state->getElement(classStackIndex);

  PRIVATE(elem)->ensureCapacity(unit);
  GLUnitData & ud = PRIVATE(elem)->unitdata[unit];
  
  // FIXME: buggy. Find some solution to handle this. pederb, 2003-11-12
  // if (ud.glimage && ud.glimage->getImage()) ud.glimage->getImage()->readUnlock();

  if (image) {
    // keep SoMultiTextureImageElement "up-to-date"
    inherited::set(state, node,
                   unit,
                   SbVec3s(0,0,0),
                   0,
                   NULL,
                   multi_translateWrap(image->getWrapS()),
                   multi_translateWrap(image->getWrapT()),
                   multi_translateWrap(image->getWrapR()),
                   model,
                   blendColor);
    ud.glimage = image;
    // make sure image isn't changed while this is the active texture
    // FIXME: buggy. Find some solution to handle this. pederb, 2003-11-12
    // if (image->getImage()) image->getImage()->readLock();
  }
  else {
    ud.glimage = NULL;
    inherited::setDefault(state, node, unit);
  }
  elem->updateGL(unit);

  // FIXME: check if it is possible to support for other units as well
  if ((unit == 0) && image && image->isOfType(SoGLBigImage::getClassTypeId())) {
    SoShapeStyleElement::setBigImageEnabled(state, TRUE);
  }
  SoShapeStyleElement::setTransparentTexture(state,
                                             SoGLMultiTextureImageElement::hasTransparency(state));
  
  SoGLShaderProgram * prog = SoGLShaderProgramElement::get(state);
  if (prog) {
    SbString str;
    str.sprintf("coin_texunit%d_model", unit);
    prog->updateCoinParameter(state, SbName(str.getString()), ud.glimage ? model : 0);
  }
}

void
SoGLMultiTextureImageElement::restore(SoState * state, const int unit)
{
  SoGLMultiTextureImageElement * elem = (SoGLMultiTextureImageElement*)
    state->getConstElement(classStackIndex);
  
  elem->updateGL(unit);
}

SoGLImage *
SoGLMultiTextureImageElement::get(SoState * state,
                                  const int unit,
                                  Model & model,
                                  SbColor & blendcolor)
{
  const SoGLMultiTextureImageElement * elem = (const SoGLMultiTextureImageElement*)
    getConstElement(state, classStackIndex);

  if (unit < elem->getNumUnits()) {
    const UnitData & ud = elem->getUnitData(unit);
    model = ud.model;
    blendcolor = ud.blendColor;
    return PRIVATE(elem)->unitdata[unit].glimage;
  }
  return NULL;
}

/*!
  Returns TRUE if any of the images have at least one transparent pixel.
  
  \since Coin 3.1
*/
SbBool 
SoGLMultiTextureImageElement::hasTransparency(SoState * state)
{
  const SoGLMultiTextureImageElement * elem = (const SoGLMultiTextureImageElement*)
    getConstElement(state, classStackIndex);
  
  for (int i = 0; i <= PRIVATE(elem)->unitdata.getLength(); i++) {
    if (elem->hasTransparency(i)) return TRUE;
  }
  return FALSE;
}

// doc from parent
SbBool
SoGLMultiTextureImageElement::hasTransparency(const int unit) const
{
  if (unit < PRIVATE(this)->unitdata.getLength()) {
    const GLUnitData & ud = PRIVATE(this)->unitdata[unit];
    if (ud.glimage) {
      return ud.glimage->hasTransparency();
    }
  }
  return FALSE;
}

void
SoGLMultiTextureImageElement::updateGL(const int unit)
{
  const GLUnitData & glud = 
    (unit < PRIVATE(this)->unitdata.getLength()) ? 
    PRIVATE(this)->unitdata[unit] :
    PRIVATE(this)->defaultdata;
  
  if (glud.glimage) {
    const cc_glglue * glue = cc_glglue_instance(PRIVATE(this)->cachecontext);
    cc_glglue_glActiveTexture(glue, (GLenum) (int(GL_TEXTURE0) + unit));

    const UnitData & ud = this->getUnitData(unit);
    SoState * state = PRIVATE(this)->state;
    SoGLDisplayList * dl = glud.glimage->getGLDisplayList(state);

    // tag image (for GLImage LRU cache).
    SoGLImage::tagImage(state, glud.glimage);

    if (SoTextureCombineElement::isDefault(state, unit)) {
      switch (ud.model) {
      case DECAL:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        break;
      case MODULATE:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        break;
      case BLEND:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
        glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, ud.blendColor.getValue());
        break;
      case REPLACE:
        // GL_REPLACE mode was introduced with OpenGL 1.1. It is
        // considered the client code's responsibility to check
        // that it can use this mode.
        //
        // FIXME: ..but we should do a sanity check anyway.
        // 20030901 mortene.
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        break;
      default:
        assert(0 && "unknown model");
        break;
      }
    }
    else {
      SoTextureCombineElement::apply(state, unit);
    }
    if (dl) {
      dl->call(state);
    }
    cc_glglue_glActiveTexture(glue, (GLenum) GL_TEXTURE0);

    GLenum glerror = sogl_glerror_debugging() ? glGetError() : GL_NO_ERROR;
    while (glerror) {
        SoDebugError::postWarning("SoGLMultiTextureImageElement::updateGL",
            "glError() = %d\n", glerror);
        glerror = glGetError();
    }
  }
}

/*!
  The size returned by this function will just be a very coarse
  estimate as it only uses the more or less obsoleted technique of
  calling glGetIntegerv(GL_MAX_TEXTURE_SIZE).
  
  For a better estimate, use
  SoGLTextureImageElement::isTextureSizeLegal().
  
  Note that this function needs an OpenGL context to be made current
  for it to work. Without that, you will most likely get a faulty
  return value or even a crash.
*/
int32_t
SoGLMultiTextureImageElement::getMaxGLTextureSize(void)
{
  SoDebugError::postWarning("SoGLMultiTextureImageElement::getMaxGLTextureSize",
                            "This function is obsoleted. It should not "
                            "be used because its interface is fubar: "
                            "the maximum texture size handled by "
                            "the OpenGL driver depends on the context, and "
                            "this function does not know which context this "
                            "information is requested for.");

  GLint val;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &val);
  return (int32_t)val;
}

#undef PRIVATE
