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
  \class SoLocateHighlight SoLocateHighlight.h Inventor/nodes/SoLocateHighlight.h
  \brief The SoLocateHighlight class highlights geometry under the cursor.

  \ingroup coin_nodes

  Note: this node is supposed to draw to the front buffer. However, in
  Coin we always draw to the back buffer, forcing a scene redraw
  whenever a highlight state changes.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    LocateHighlight {
        renderCaching AUTO
        boundingBoxCaching AUTO
        renderCulling AUTO
        pickCulling AUTO
        color 0.3 0.3 0.3
        style EMISSIVE
        mode AUTO
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoLocateHighlight.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/SoFullPath.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/SoPickedPoint.h>

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbStorage.h>
#endif // COIN_THREADSAFE

#include "tidbitsp.h"
#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \enum SoLocateHighlight::Modes
  Enum type for behaviour modes.
*/
/*!
  \var SoLocateHighlight::Modes SoLocateHighlight::AUTO
  Highlight when mouse cursor is over the contents of the node.
*/
/*!
  \var SoLocateHighlight::Modes SoLocateHighlight::ON
  Always highlight.
*/
/*!
  \var SoLocateHighlight::Modes SoLocateHighlight::OFF
  Never highlight.
*/
/*!
  \enum SoLocateHighlight::Styles
  Enum type for highlight styles.
*/
/*!
  \var SoLocateHighlight::Styles SoLocateHighlight::EMISSIVE
  Highlight using emissive color override.
*/
/*!
  \var SoLocateHighlight::Styles SoLocateHighlight::EMISSIVE_DIFFUSE
  Highlight using emissive and diffuse color override.
*/

/*!
  \var SoSFColor SoLocateHighlight::color

  The color used for highlighting. Default is [0.3, 0.3, 0.3], a dark
  gray.
*/

/*!
  \var SoSFEnum SoLocateHighlight::style

  The highlight style. Default is SoLocateHighlight::EMISSIVE.
*/
/*!
  \var SoSFEnum SoLocateHighlight::mode

  The highlight mode. Default is SoLocateHighlight::AUTO.
*/

// *************************************************************************

class SoLocateHighlightP {
public:
  SoLocateHighlightP() 
#ifdef COIN_THREADSAFE
    : colorpacker_storage(sizeof(void*), alloc_colorpacker, free_colorpacker)
#endif // COIN_THREADSAFE
  {}

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
  SbBool highlighted;
  static SoFullPath * currenthighlight;

  static void atexit_cleanup(void) {
    if (SoLocateHighlightP::currenthighlight) {
      SoLocateHighlightP::currenthighlight->unref();
      SoLocateHighlightP::currenthighlight = NULL;
    }
  }
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

SoFullPath * SoLocateHighlightP::currenthighlight = NULL;

// *************************************************************************

#define PRIVATE(p) ((p)->pimpl)

// *************************************************************************

SO_NODE_SOURCE(SoLocateHighlight);

// *************************************************************************

/*!
  Constructor.
*/
SoLocateHighlight::SoLocateHighlight()
{
  PRIVATE(this) = new SoLocateHighlightP;
  SO_NODE_INTERNAL_CONSTRUCTOR(SoLocateHighlight);

  SO_NODE_ADD_FIELD(color, (SbColor(0.3f, 0.3f, 0.3f)));
  SO_NODE_ADD_FIELD(style, (EMISSIVE));
  SO_NODE_ADD_FIELD(mode, (AUTO));

  SO_NODE_DEFINE_ENUM_VALUE(Styles, EMISSIVE);
  SO_NODE_DEFINE_ENUM_VALUE(Styles, EMISSIVE_DIFFUSE);
  SO_NODE_SET_SF_ENUM_TYPE(style, Styles);

  SO_NODE_DEFINE_ENUM_VALUE(Modes, AUTO);
  SO_NODE_DEFINE_ENUM_VALUE(Modes, ON);
  SO_NODE_DEFINE_ENUM_VALUE(Modes, OFF);
  SO_NODE_SET_SF_ENUM_TYPE(mode, Modes);

  PRIVATE(this)->highlighted = FALSE;
}

/*!
  Destructor.
*/
SoLocateHighlight::~SoLocateHighlight()
{
  delete PRIVATE(this);
}

// doc from parent
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoLocateHighlight::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoLocateHighlight, SO_FROM_INVENTOR_1);
  coin_atexit((coin_atexit_f*)SoLocateHighlightP::atexit_cleanup, CC_ATEXIT_NORMAL);
}

/*!
  Static method that can be used to turn off the current highlight.
*/
void
SoLocateHighlight::turnOffCurrentHighlight(SoGLRenderAction * action)
{
  SoLocateHighlight::turnoffcurrent(action);
}

// doc from parent
void
SoLocateHighlight::handleEvent(SoHandleEventAction * action)
{
  Modes mymode = (Modes) this->mode.getValue();
  if (mymode == AUTO) {
    const SoEvent * event = action->getEvent();
    if (event->isOfType(SoLocation2Event::getClassTypeId())) {
      const SoPickedPoint * pp = action->getPickedPoint();
      if (pp && pp->getPath()->containsPath(action->getCurPath())) {
        if (!PRIVATE(this)->highlighted) {
          SoLocateHighlight::turnoffcurrent(action);
          SoLocateHighlightP::currenthighlight = (SoFullPath*)
            action->getCurPath()->copy();
          SoLocateHighlightP::currenthighlight->ref();
          PRIVATE(this)->highlighted = TRUE;
          this->touch(); // force scene redraw
          this->redrawHighlighted(action, TRUE);
        }
      }
      else {
        if (PRIVATE(this)->highlighted) {
          SoLocateHighlight::turnoffcurrent(action);
        }
      }
    }
  }
  inherited::handleEvent(action);
}

// doc from parent
void
SoLocateHighlight::GLRenderBelowPath(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  state->push();
  if (PRIVATE(this)->highlighted || this->mode.getValue() == ON) {
    this->setOverride(action);
  }
  inherited::GLRenderBelowPath(action);
  state->pop();
}

// doc from parent
void
SoLocateHighlight::GLRenderInPath(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  state->push();
  if (PRIVATE(this)->highlighted || this->mode.getValue() == ON) {
    this->setOverride(action);
  }
  inherited::GLRenderInPath(action);
  state->pop();
}

/*!
  Empty method in Coin. Can be used by subclasses to be told
  when status change.
*/
void
SoLocateHighlight::redrawHighlighted(SoAction * /* act */, SbBool /* flag */)
{
}

//
// update override state before rendering
//
void
SoLocateHighlight::setOverride(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  SoLazyElement::setEmissive(state, &this->color.getValue());
  SoOverrideElement::setEmissiveColorOverride(state, this, TRUE);

  Styles mystyle = (Styles) this->style.getValue();
  if (mystyle == SoLocateHighlight::EMISSIVE_DIFFUSE) {
    SoLazyElement::setDiffuse(state, this,
                               1, &this->color.getValue(),
                              PRIVATE(this)->getColorPacker());
    SoOverrideElement::setDiffuseColorOverride(state, this, TRUE);
  }
}

// private convenience method
void
SoLocateHighlight::turnoffcurrent(SoAction * action)
{
  if (SoLocateHighlightP::currenthighlight &&
      SoLocateHighlightP::currenthighlight->getLength()) {
    SoNode * tail = SoLocateHighlightP::currenthighlight->getTail();
    if (tail->isOfType(SoLocateHighlight::getClassTypeId())) {
      ((SoLocateHighlight*)tail)->pimpl->highlighted = FALSE;
      ((SoLocateHighlight*)tail)->touch(); // force scene redraw
      if (action) ((SoLocateHighlight*)tail)->redrawHighlighted(action, FALSE);
    }
  }
  if (SoLocateHighlightP::currenthighlight) {
    SoLocateHighlightP::currenthighlight->unref();
    SoLocateHighlightP::currenthighlight = NULL;
  }
}

#undef PRIVATE
