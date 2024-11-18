/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <Inventor/elements/SoOverrideElement.h>
#endif

#include "SoFCInteractiveElement.h"

using namespace Gui;

SO_ELEMENT_SOURCE(SoFCInteractiveElement)

void SoFCInteractiveElement::initClass()
{
  SO_ELEMENT_INIT_CLASS(SoFCInteractiveElement, inherited);
  SO_ENABLE(SoGLRenderAction, SoFCInteractiveElement);
}

void SoFCInteractiveElement::init(SoState * /*state*/)
{
  this->interactiveMode = false;
}

SoFCInteractiveElement::~SoFCInteractiveElement() = default;

void SoFCInteractiveElement::set(SoState * const state, SoNode * const node, SbBool mode)
{
    auto elem = (SoFCInteractiveElement *)
    SoReplacedElement::getElement(state, classStackIndex, node);
  elem->setElt(mode);
}

SbBool SoFCInteractiveElement::get(SoState * const state)
{
  return SoFCInteractiveElement::getInstance(state)->interactiveMode;
}

void SoFCInteractiveElement::setElt(SbBool mode)
{
  this->interactiveMode = mode;
}

const SoFCInteractiveElement * SoFCInteractiveElement::getInstance(SoState * state)
{
  return (const SoFCInteractiveElement *) SoElement::getConstElement(state, classStackIndex);
}

// ---------------------------------

SO_ELEMENT_SOURCE(SoGLWidgetElement)

void SoGLWidgetElement::initClass()
{
  SO_ELEMENT_INIT_CLASS(SoGLWidgetElement, inherited);
  SO_ENABLE(SoGLRenderAction, SoGLWidgetElement);
  SO_ENABLE(SoHandleEventAction, SoGLWidgetElement);
}

void SoGLWidgetElement::init(SoState * state)
{
  inherited::init(state);
  this->window = nullptr;
}

SoGLWidgetElement::~SoGLWidgetElement() = default;

void SoGLWidgetElement::set(SoState * state, QtGLWidget * window)
{
    auto elem = static_cast<SoGLWidgetElement *>
        (SoElement::getElement(state, classStackIndex));
  elem->window = window;
}

void SoGLWidgetElement::get(SoState * state, QtGLWidget *& window)
{
    const auto that =  static_cast<const SoGLWidgetElement *>
        (SoElement::getConstElement(state, classStackIndex));
    window = that->window;
}

void SoGLWidgetElement::push(SoState * state)
{
    inherited::push(state);
}

void SoGLWidgetElement::pop(SoState * state, const SoElement * prevTopElement)
{
    inherited::pop(state, prevTopElement);
}

SbBool SoGLWidgetElement::matches(const SoElement * /*element*/) const
{
    return true;
}

SoElement * SoGLWidgetElement::copyMatchInfo() const
{
    return nullptr;
}

// ---------------------------------

SO_ELEMENT_SOURCE(SoGLRenderActionElement)

void SoGLRenderActionElement::initClass()
{
  SO_ELEMENT_INIT_CLASS(SoGLRenderActionElement, inherited);
  SO_ENABLE(SoGLRenderAction, SoGLRenderActionElement);
  SO_ENABLE(SoHandleEventAction, SoGLRenderActionElement);
}

void SoGLRenderActionElement::init(SoState * state)
{
  inherited::init(state);
  this->glRenderAction = nullptr;
}

SoGLRenderActionElement::~SoGLRenderActionElement() = default;

void SoGLRenderActionElement::set(SoState * state, SoGLRenderAction * action)
{
    auto elem = static_cast<SoGLRenderActionElement *>
        (SoElement::getElement(state, classStackIndex));
  elem->glRenderAction = action;
}

void SoGLRenderActionElement::get(SoState * state, SoGLRenderAction * & action)
{
    const auto that =  static_cast<const SoGLRenderActionElement *>
        (SoElement::getConstElement(state, classStackIndex));
    action = that->glRenderAction;
}

void SoGLRenderActionElement::push(SoState * state)
{
    inherited::push(state);
}

void SoGLRenderActionElement::pop(SoState * state, const SoElement * prevTopElement)
{
    inherited::pop(state, prevTopElement);
}

SbBool SoGLRenderActionElement::matches(const SoElement * /*element*/) const
{
    return true;
}

SoElement * SoGLRenderActionElement::copyMatchInfo() const
{
    return nullptr;
}

// ---------------------------------

SO_NODE_SOURCE(SoGLWidgetNode)

/*!
  Constructor.
*/
SoGLWidgetNode::SoGLWidgetNode()
{
    SO_NODE_CONSTRUCTOR(SoGLWidgetNode);
}

/*!
  Destructor.
*/
SoGLWidgetNode::~SoGLWidgetNode() = default;

// Doc from superclass.
void SoGLWidgetNode::initClass()
{
    SO_NODE_INIT_CLASS(SoGLWidgetNode, SoNode, "Node");

    SO_ENABLE(SoGLRenderAction, SoGLWidgetElement);
}

// Doc from superclass.
void SoGLWidgetNode::doAction(SoAction * action)
{
    SoGLWidgetElement::set(action->getState(), this->window);
}

// Doc from superclass.
void SoGLWidgetNode::GLRender(SoGLRenderAction * action)
{
    SoGLWidgetNode::doAction(action);
}

// ---------------------------------

SO_ELEMENT_SOURCE(SoGLVBOActivatedElement)

void SoGLVBOActivatedElement::initClass()
{
  SO_ELEMENT_INIT_CLASS(SoGLVBOActivatedElement, inherited);
  SO_ENABLE(SoGLRenderAction, SoGLVBOActivatedElement);
  SO_ENABLE(SoHandleEventAction, SoGLVBOActivatedElement);
}

void SoGLVBOActivatedElement::init(SoState * state)
{
  inherited::init(state);
  this->active = false;
}

SoGLVBOActivatedElement::~SoGLVBOActivatedElement() = default;

void SoGLVBOActivatedElement::set(SoState * state, SbBool active)
{
    auto elem = static_cast<SoGLVBOActivatedElement *>
        (SoElement::getElement(state, classStackIndex));
  elem->active = active;
}

void SoGLVBOActivatedElement::get(SoState * state, SbBool& active)
{
    const auto self =  static_cast<const SoGLVBOActivatedElement *>
        (SoElement::getConstElement(state, classStackIndex));
    active = self->active;
    if(active) {
        uint32_t flags = SoOverrideElement::getFlags(state);
        if(flags & (SoOverrideElement::COLOR_INDEX|
                    SoOverrideElement::DIFFUSE_COLOR|
                    SoOverrideElement::MATERIAL_BINDING|
                    SoOverrideElement::TRANSPARENCY|
                    SoOverrideElement::NORMAL_VECTOR|
                    SoOverrideElement::NORMAL_BINDING))
            active = false;
    }
}

void SoGLVBOActivatedElement::push(SoState * state)
{
    inherited::push(state);
}

void SoGLVBOActivatedElement::pop(SoState * state, const SoElement * prevTopElement)
{
    inherited::pop(state, prevTopElement);
}

SbBool SoGLVBOActivatedElement::matches(const SoElement * /*element*/) const
{
    return true;
}

SoElement * SoGLVBOActivatedElement::copyMatchInfo() const
{
    return nullptr;
}
