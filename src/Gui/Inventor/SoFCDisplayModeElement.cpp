/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#include <Inventor/actions/SoActions.h>
#include "../SoFCUnifiedSelection.h"
#include "../SoFCSelectionAction.h"
#include "../ViewParams.h"
#include "SoFCDisplayModeElement.h"

using namespace Gui;

SO_ELEMENT_SOURCE(SoFCDisplayModeElement)

void
SoFCDisplayModeElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoFCDisplayModeElement, inherited);

  SO_ENABLE(SoGLRenderAction, SoFCDisplayModeElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoFCDisplayModeElement);
  SO_ENABLE(SoAudioRenderAction, SoFCDisplayModeElement);
  SO_ENABLE(SoSearchAction, SoFCDisplayModeElement);
  SO_ENABLE(SoGetMatrixAction, SoFCDisplayModeElement);
  SO_ENABLE(SoPickAction, SoFCDisplayModeElement);
  SO_ENABLE(SoCallbackAction, SoFCDisplayModeElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoFCDisplayModeElement);
  SO_ENABLE(SoHandleEventAction, SoFCDisplayModeElement);
  SO_ENABLE(SoSelectionElementAction, SoFCDisplayModeElement);
  SO_ENABLE(SoHighlightElementAction, SoFCDisplayModeElement);
  SO_ENABLE(SoFCEnableSelectionAction, SoFCDisplayModeElement);
  SO_ENABLE(SoFCEnableHighlightAction, SoFCDisplayModeElement);
  SO_ENABLE(SoFCSelectionColorAction, SoFCDisplayModeElement);
  SO_ENABLE(SoFCHighlightColorAction, SoFCDisplayModeElement);
  SO_ENABLE(SoFCSelectionAction, SoFCDisplayModeElement);
  SO_ENABLE(SoFCHighlightAction, SoFCDisplayModeElement);
  SO_ENABLE(SoFCDocumentAction, SoFCDisplayModeElement);
  SO_ENABLE(SoFCDocumentObjectAction, SoFCDisplayModeElement);
  SO_ENABLE(SoGLSelectAction, SoFCDisplayModeElement);
  SO_ENABLE(SoVisibleFaceAction, SoFCDisplayModeElement);
  SO_ENABLE(SoUpdateVBOAction, SoFCDisplayModeElement);
  SO_ENABLE(SoVRMLAction, SoFCDisplayModeElement);
}

SoFCDisplayModeElement::~SoFCDisplayModeElement()
{
}

void
SoFCDisplayModeElement::set(SoState * const state,
                            SoNode * const node,
                            const SbName &mode,
                            SbBool hiddenLines)
{
  auto element = static_cast<SoFCDisplayModeElement*>(
      inherited::getElement(state, classStackIndex, node));

  if (element) {
    element->displayMode = mode;
    if((element->hiddenLines = hiddenLines)) {
      float f;
      if((element->hasFaceColor = ViewParams::getHiddenLineOverrideFaceColor()))
        element->faceColor.setPackedValue(ViewParams::getHiddenLineFaceColor(),f);
      if((element->hasLineColor = ViewParams::getHiddenLineOverrideColor()))
        element->lineColor.setPackedValue(ViewParams::getHiddenLineColor(),f);
      element->transp = ViewParams::getHiddenLineTransparency();
    }
    else {
      element->hasFaceColor = false;
      element->hasLineColor = false;
      element->transp = 0.0f;
    }
  }
}

SoFCDisplayModeElement *
SoFCDisplayModeElement::getInstance(SoState *state)
{
  return static_cast<SoFCDisplayModeElement*>(
      state->getElementNoPush(classStackIndex));
}

void
SoFCDisplayModeElement::setColors(SoState * const state,
                                  SoNode * const node,
                                  const SbColor *faceColor,
                                  const SbColor *lineColor,
                                  float transp)
{
  auto element = static_cast<SoFCDisplayModeElement*>(
      inherited::getElement(state, classStackIndex, node));

  if (element) {
    if ((element->hasFaceColor = (faceColor != nullptr)))
      element->faceColor = *faceColor;
    if ((element->hasLineColor = (lineColor != nullptr)))
      element->lineColor = *lineColor;
    element->transp = transp;
  }
}

  const SbName &
SoFCDisplayModeElement::get(SoState * const state)
{
  auto element = static_cast<const SoFCDisplayModeElement*>(
      inherited::getConstElement(state, classStackIndex));
  return element->displayMode;
}

const SbName &
SoFCDisplayModeElement::get() const
{
  return displayMode;
}

  SbBool
SoFCDisplayModeElement::showHiddenLines(SoState * const state)
{
  auto element = static_cast<const SoFCDisplayModeElement*>(
      inherited::getConstElement(state, classStackIndex));
  return element->hiddenLines;
}

SbBool
SoFCDisplayModeElement::showHiddenLines() const
{
  return hiddenLines;
}

  const SbColor *
SoFCDisplayModeElement::getFaceColor(SoState * const state)
{
  auto element = static_cast<const SoFCDisplayModeElement*>(
      inherited::getConstElement(state, classStackIndex));
  return element->getFaceColor();
}

const SbColor *
SoFCDisplayModeElement::getFaceColor() const
{
  return hasFaceColor ? &faceColor : nullptr;
}

  const SbColor *
SoFCDisplayModeElement::getLineColor(SoState * const state)
{
  auto element = static_cast<const SoFCDisplayModeElement*>(
      inherited::getConstElement(state, classStackIndex));
  return element->getLineColor();
}

const SbColor *
SoFCDisplayModeElement::getLineColor() const
{
  return hasLineColor ? &lineColor : nullptr;
}

  float
SoFCDisplayModeElement::getTransparency(SoState * const state)
{
  auto element = static_cast<const SoFCDisplayModeElement*>(
      inherited::getConstElement(state, classStackIndex));
  return element->getTransparency();
}

float
SoFCDisplayModeElement::getTransparency() const
{
  return transp;
}

SbBool
SoFCDisplayModeElement::matches(const SoElement * element) const
{
  if (this == element)
    return TRUE;
  if (element->getTypeId() != SoFCDisplayModeElement::getClassTypeId())
    return FALSE;
  auto other = static_cast<const SoFCDisplayModeElement *>(element);
  if (this->displayMode != other->displayMode || this->hiddenLines != other->hiddenLines)
    return FALSE;
  if(this->hasFaceColor != other->hasFaceColor
      || this->hasLineColor != other->hasLineColor
      || this->transp != other->transp)
    return FALSE;
  if((this->hasFaceColor && this->faceColor != other->faceColor)
      || (this->hasLineColor && this->lineColor != other->lineColor))
    return FALSE;
  return TRUE;
}

SoElement *
SoFCDisplayModeElement::copyMatchInfo(void) const
{
  auto element = static_cast<SoFCDisplayModeElement *>(
      SoFCDisplayModeElement::getClassTypeId().createInstance());

  element->displayMode = this->displayMode;
  element->hiddenLines = this->hiddenLines;
  element->hasFaceColor = this->hasFaceColor;
  element->hasLineColor = this->hasLineColor;
  element->transp = this->transp;
  if(this->hasFaceColor)
    element->faceColor = this->faceColor;
  if(this->hasLineColor)
    element->lineColor = this->lineColor;
  element->nodeId = this->nodeId;
  return element;
}

void
SoFCDisplayModeElement::init(SoState * state)
{
  inherited::init(state);
  this->displayMode = SbName::empty();
  this->hiddenLines = FALSE;
  this->hasFaceColor = FALSE;
  this->hasLineColor = FALSE;
  this->transp = 0.0f;
}

// vim: noai:ts=2:sw=2
