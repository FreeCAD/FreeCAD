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
  \class SoGLMultiTextureCoordinateElement Inventor/elements/SoGLMultiTextureCoordinateElement.h
  \brief The SoGLMultiTextureCoordinateElement class stores the current gltexture coordinates for several units.

  \ingroup coin_elements
*/

#include <Inventor/elements/SoGLMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/lists/SbList.h>

#include <cassert>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/system/gl.h>
#include <Inventor/C/glue/gl.h>

class SoGLMultiTextureCoordinateElementP {
public:
  mutable SbList<SoGLMultiTextureCoordinateElement::GLUnitData> unitdata;
  int contextid;
  
  // switch/case table for faster rendering.
  enum SendLookup {
    UNINITIALIZED,
    NONE,
    FUNCTION,
    TEXCOORD2,
    TEXCOORD3,
    TEXCOORD4
  };
  mutable SbList<SendLookup> sendlookup;
  const cc_glglue * glue;
  SoGLMultiTextureCoordinateElement::GLUnitData defaultdata;
  void ensureCapacity(int unit) const {
    while (unit >= this->unitdata.getLength()) {
      this->unitdata.append(SoGLMultiTextureCoordinateElement::GLUnitData());
    }
  }

};

#define PRIVATE(obj) obj->pimpl

SO_ELEMENT_CUSTOM_CONSTRUCTOR_SOURCE(SoGLMultiTextureCoordinateElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLMultiTextureCoordinateElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLMultiTextureCoordinateElement, inherited);
}

/*!
  Constructor.
*/
SoGLMultiTextureCoordinateElement::SoGLMultiTextureCoordinateElement(void)
{
  PRIVATE(this) = new SoGLMultiTextureCoordinateElementP;

  this->setTypeId(SoGLMultiTextureCoordinateElement::classTypeId);
  this->setStackIndex(SoGLMultiTextureCoordinateElement::classStackIndex);
}

/*!
  Destructor.
*/

SoGLMultiTextureCoordinateElement::~SoGLMultiTextureCoordinateElement()
{
  delete PRIVATE(this);
}

//!  FIXME: write doc.

void
SoGLMultiTextureCoordinateElement::init(SoState * state)
{
  SoAction * action = state->getAction();
  assert(action->isOfType(SoGLRenderAction::getClassTypeId()));
  // fetch cache context id from action since SoGLCacheContextElement
  // might not be initialized yet.
  SoGLRenderAction * glaction = (SoGLRenderAction*) action;
  PRIVATE(this)->contextid = glaction->getCacheContext();

  inherited::init(state);
  PRIVATE(this)->unitdata.truncate(0);
  PRIVATE(this)->sendlookup.truncate(0);
}

//!  FIXME: write doc.

void
SoGLMultiTextureCoordinateElement::push(SoState * state)
{
  inherited::push(state);
  SoGLMultiTextureCoordinateElement * prev = (SoGLMultiTextureCoordinateElement*)this->getNextInStack();
  
  PRIVATE(this)->contextid = PRIVATE(prev)->contextid;
  PRIVATE(this)->unitdata = PRIVATE(prev)->unitdata;
  
  prev->capture(state);
}

//!  FIXME: write doc.

void
SoGLMultiTextureCoordinateElement::pop(SoState * state,
                                       const SoElement * prevTopElement)
{
  inherited::pop(state, prevTopElement);
  SoGLMultiTextureCoordinateElement * prev = (SoGLMultiTextureCoordinateElement*) prevTopElement;

  const cc_glglue * glue = cc_glglue_instance(PRIVATE(this)->contextid);
  const int maxunits = SbMax(PRIVATE(this)->unitdata.getLength(), 
                             PRIVATE(prev)->unitdata.getLength());
  
  for (int i = 0; i < maxunits; i++) {
    const GLUnitData & thisud = 
      i < PRIVATE(this)->unitdata.getLength() ? 
      PRIVATE(this)->unitdata[i] : PRIVATE(this)->defaultdata;
    const GLUnitData & prevud = 
      i < PRIVATE(prev)->unitdata.getLength() ?
      PRIVATE(prev)->unitdata[i] : PRIVATE(prev)->defaultdata;
    
    SbBool enablegen = FALSE;
    SbBool disablegen = FALSE;
    SbBool docallback = FALSE;

    if (thisud.texgenCB && !prevud.texgenCB) {enablegen = TRUE; docallback = TRUE;}
    else if (!thisud.texgenCB && prevud.texgenCB) disablegen = TRUE;
    else if (thisud.texgenCB/* != prevud.texgenCB*/) docallback = TRUE;

/*
  See the comments in the setElt function below for the explanation for commenting
  out the second half of the above else if statement. RHW
*/

	if (enablegen || disablegen || docallback) {
      // must change texture unit while updating OpenGL
      cc_glglue_glActiveTexture(glue, (GLenum) (int(GL_TEXTURE0) + i));
    }
    if (enablegen) {
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
      glEnable(GL_TEXTURE_GEN_R);
      glEnable(GL_TEXTURE_GEN_Q);
    }
    if (disablegen) {
      glDisable(GL_TEXTURE_GEN_S);
      glDisable(GL_TEXTURE_GEN_T);
      glDisable(GL_TEXTURE_GEN_R);
      glDisable(GL_TEXTURE_GEN_Q);
    }
    if (docallback) {
      this->doCallback(i);
    }
    // restore default unit
    if (enablegen || disablegen || docallback) {
      cc_glglue_glActiveTexture(glue, (GLenum) GL_TEXTURE0);
    }
  }
}

//!  FIXME: write doc.

void
SoGLMultiTextureCoordinateElement::setTexGen(SoState * const state,
                                             SoNode * const node,
                                             const int unit,
                                             SoTexCoordTexgenCB * const texgenFunc,
                                             void * const texgenData,
                                             SoTextureCoordinateFunctionCB * const func,
                                             void * const funcData)
{
  SoMultiTextureCoordinateElement::setFunction(state, node, unit, func, funcData);

  SoGLMultiTextureCoordinateElement *element = (SoGLMultiTextureCoordinateElement *)
    SoElement::getElement(state, classStackIndex);
  if (element) {
    element->setElt(unit, texgenFunc, texgenData);
  }
}

//!  FIXME: write doc.

SoMultiTextureCoordinateElement::CoordType
SoGLMultiTextureCoordinateElement::getType(const int unit) const
{
  if (unit < PRIVATE(this)->unitdata.getLength()) {
    if (PRIVATE(this)->unitdata[unit].texgenCB) return SoMultiTextureCoordinateElement::NONE;
  }
  return inherited::getType(unit);
}

//!  FIXME: write doc.

const SoGLMultiTextureCoordinateElement *
SoGLMultiTextureCoordinateElement::getInstance(SoState * const state)
{
  return (SoGLMultiTextureCoordinateElement*)
    SoElement::getConstElement(state, classStackIndex);
}

//!  FIXME: write doc.

void
SoGLMultiTextureCoordinateElement::send(const int unit, const int index) const
{
  const UnitData & ud = this->getUnitData(unit);
  GLenum glunit = (GLenum) (int(GL_TEXTURE0) + unit);
  const cc_glglue * glue = PRIVATE(this)->glue;

  assert(unit < PRIVATE(this)->sendlookup.getLength());
  switch (PRIVATE(this)->sendlookup[unit]) {
  case SoGLMultiTextureCoordinateElementP::UNINITIALIZED:
    assert(0 && "should not happen");
    break;
  case SoGLMultiTextureCoordinateElementP::NONE:
    break;
  case SoGLMultiTextureCoordinateElementP::FUNCTION:
    assert(0 && "should not happen");
    break;
  case SoGLMultiTextureCoordinateElementP::TEXCOORD2:
    assert(index < ud.numCoords);
    cc_glglue_glMultiTexCoord2fv(glue, glunit, ud.coords2[index].getValue());
    break;
  case SoGLMultiTextureCoordinateElementP::TEXCOORD3:
    cc_glglue_glMultiTexCoord3fv(glue, glunit, ud.coords3[index].getValue());
    break;
  case SoGLMultiTextureCoordinateElementP::TEXCOORD4:
    cc_glglue_glMultiTexCoord4fv(glue, glunit, ud.coords4[index].getValue());
    break;
  default:
    assert(0 && "should not happen");
    break;
  }
}

//!  FIXME: write doc.

void
SoGLMultiTextureCoordinateElement::send(const int unit,
                                        const int index,
                                        const SbVec3f &c,
                                        const SbVec3f &n) const
{
  const UnitData & ud = this->getUnitData(unit);
  GLenum glunit = (GLenum) (int(GL_TEXTURE0) + unit);
  const cc_glglue * glue = PRIVATE(this)->glue;
  
  assert(unit < PRIVATE(this)->sendlookup.getLength());
  switch (PRIVATE(this)->sendlookup[unit]) {
  case SoGLMultiTextureCoordinateElementP::NONE:
    break;
  case SoGLMultiTextureCoordinateElementP::FUNCTION:
    assert(ud.funcCB);
    cc_glglue_glMultiTexCoord4fv(glue, glunit,
                                 ud.funcCB(ud.funcCBData, c, n).getValue());

    break;
  case SoGLMultiTextureCoordinateElementP::TEXCOORD2:
    cc_glglue_glMultiTexCoord2fv(glue, glunit, ud.coords2[index].getValue());
    break;
  case SoGLMultiTextureCoordinateElementP::TEXCOORD3:
    cc_glglue_glMultiTexCoord3fv(glue, glunit, ud.coords3[index].getValue());
    break;
  case SoGLMultiTextureCoordinateElementP::TEXCOORD4:
    cc_glglue_glMultiTexCoord4fv(glue, glunit, ud.coords4[index].getValue());
    break;
  default:
    assert(0 && "should not happen");
    break;
  }
}

//!  FIXME: write doc.

void
SoGLMultiTextureCoordinateElement::setElt(const int unit,
                                          SoTexCoordTexgenCB * func,
                                          void *data)
{
  PRIVATE(this)->ensureCapacity(unit);
  GLUnitData & ud = PRIVATE(this)->unitdata[unit];
  
  SbBool enablegen = FALSE;
  SbBool disablegen = FALSE;
  SbBool docallback = FALSE;

  if (func && !ud.texgenCB) {enablegen = TRUE; docallback = TRUE;}
  else if (!func && ud.texgenCB) disablegen = TRUE;
  else if (func /* && func != ud.texgenCB */) docallback = TRUE;

  /*
  The last part of the above if else statement was modified because example 7.3 from The Inventor
  Mentor was not being correctly reproduced. However, the above solution causes a reduction in
  execution efficiency.  So...

  FIXME: Consider whether the caching mechanism can be used to overcome this problem.

  RHW 20141007
  */

  if (func) {
    // update SoMultiTextureCoordinateElement type
    this->getUnitData(unit).whatKind = SoMultiTextureCoordinateElement::FUNCTION;
  }
  ud.texgenCB = func;
  ud.texgenData = data;

  const cc_glglue * glue = cc_glglue_instance(PRIVATE(this)->contextid);

  if (enablegen || disablegen || docallback) {
    cc_glglue_glActiveTexture(glue, (GLenum) (int(GL_TEXTURE0) + unit));
  }

  if (enablegen) {
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);
    glEnable(GL_TEXTURE_GEN_Q);
  }
  if (disablegen) {
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_Q);
  }
  if (docallback) this->doCallback(unit);

  if (enablegen || disablegen || docallback) {
    cc_glglue_glActiveTexture(glue, (GLenum) GL_TEXTURE0);
  }
}

void
SoGLMultiTextureCoordinateElement::doCallback(const int unit) const
{
  if (PRIVATE(this)->unitdata[unit].texgenCB) {
    PRIVATE(this)->unitdata[unit].texgenCB(PRIVATE(this)->unitdata[unit].texgenData);
  }
}

/*!
  Internal method that is called from SoGLTextureCoordinateBundle to
  set up optimized rendering.
*/
void
SoGLMultiTextureCoordinateElement::initRender(const SbBool * enabled, const int maxenabled) const
{
  PRIVATE(this)->glue = cc_glglue_instance(PRIVATE(this)->contextid);
  PRIVATE(this)->sendlookup.truncate(0);
  for (int i = 0; i <= maxenabled; i++) {
    PRIVATE(this)->sendlookup.append(SoGLMultiTextureCoordinateElementP::NONE);
    // init the sendloopup variable
    if (enabled[i]) {
      const UnitData & ud = this->getUnitData(i);
      switch (ud.whatKind) {
      case SoMultiTextureCoordinateElement::DEFAULT:
        assert(0 && "should not happen");
        break;
      case SoMultiTextureCoordinateElement::FUNCTION:
        if (ud.funcCB) {
          PRIVATE(this)->sendlookup[i] = SoGLMultiTextureCoordinateElementP::FUNCTION;
        }
        break;
      case SoMultiTextureCoordinateElement::NONE:
        break;
      case SoMultiTextureCoordinateElement::EXPLICIT:
        {
          switch (ud.coordsDimension) {
          case 2:
            PRIVATE(this)->sendlookup[i] = SoGLMultiTextureCoordinateElementP::TEXCOORD2;
            break;
          case 3:
            PRIVATE(this)->sendlookup[i] = SoGLMultiTextureCoordinateElementP::TEXCOORD3;
            break;
          case 4:
            PRIVATE(this)->sendlookup[i] = SoGLMultiTextureCoordinateElementP::TEXCOORD4;
            break;
          default:
            assert(0 && "should not happen");
            break;
          }
        }
        break;
      default:
        assert(0 && "should not happen");
        break;
      }
    }
  }
}

/*!
  Called from SoTextureCoordinateBundle to initialize multi texturing.

  \internal
*/
void
SoGLMultiTextureCoordinateElement::initMulti(SoState * state) const
{
  this->multienabled = SoMultiTextureEnabledElement::getEnabledUnits(state,
                                                                     this->multimax);
  this->initRender(this->multienabled, this->multimax);
}


#undef PRIVATE
