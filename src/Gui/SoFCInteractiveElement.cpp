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

#include "SoFCInteractiveElement.h"

using namespace Gui;

SO_ELEMENT_SOURCE(SoFCInteractiveElement);

void SoFCInteractiveElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoFCInteractiveElement, inherited);
  SO_ENABLE(SoGLRenderAction, SoFCInteractiveElement);
}

void SoFCInteractiveElement::init(SoState * state)
{
  this->interactiveMode = false;
}

SoFCInteractiveElement::~SoFCInteractiveElement()
{
}

void SoFCInteractiveElement::set(SoState * const state, SoNode * const node, SbBool mode)
{
  SoFCInteractiveElement * elem = (SoFCInteractiveElement *)
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

SO_ELEMENT_SOURCE(SoGLWidgetElement);

void SoGLWidgetElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLWidgetElement, inherited);
  SO_ENABLE(SoGLRenderAction, SoGLWidgetElement);
  SO_ENABLE(SoHandleEventAction, SoGLWidgetElement);
}

void SoGLWidgetElement::init(SoState * state)
{
  inherited::init(state);
  this->window = 0;
}

SoGLWidgetElement::~SoGLWidgetElement()
{
}

void SoGLWidgetElement::set(SoState * state, QGLWidget * window)
{
  SoGLWidgetElement * elem = static_cast<SoGLWidgetElement *>
        (SoElement::getElement(state, classStackIndex));
  elem->window = window;
}

void SoGLWidgetElement::get(SoState * state, QGLWidget *& window)
{
    const SoGLWidgetElement* that =  static_cast<const SoGLWidgetElement *>
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

SbBool SoGLWidgetElement::matches(const SoElement * element) const
{
    return TRUE;
}

SoElement * SoGLWidgetElement::copyMatchInfo(void) const
{
    return 0;
}

// ---------------------------------

SO_ELEMENT_SOURCE(SoGLRenderActionElement);

void SoGLRenderActionElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLRenderActionElement, inherited);
  SO_ENABLE(SoGLRenderAction, SoGLRenderActionElement);
  SO_ENABLE(SoHandleEventAction, SoGLRenderActionElement);
}

void SoGLRenderActionElement::init(SoState * state)
{
  inherited::init(state);
  this->glRenderAction = 0;
}

SoGLRenderActionElement::~SoGLRenderActionElement()
{
}

void SoGLRenderActionElement::set(SoState * state, SoGLRenderAction * action)
{
  SoGLRenderActionElement * elem = static_cast<SoGLRenderActionElement *>
        (SoElement::getElement(state, classStackIndex));
  elem->glRenderAction = action;
}

void SoGLRenderActionElement::get(SoState * state, SoGLRenderAction * & action)
{
    const SoGLRenderActionElement* that =  static_cast<const SoGLRenderActionElement *>
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

SbBool SoGLRenderActionElement::matches(const SoElement * element) const
{
    return TRUE;
}

SoElement * SoGLRenderActionElement::copyMatchInfo(void) const
{
    return 0;
}

// ---------------------------------

SO_NODE_SOURCE(SoGLWidgetNode);

/*!
  Constructor.
*/
SoGLWidgetNode::SoGLWidgetNode(void) : window(0)
{
    SO_NODE_CONSTRUCTOR(SoGLWidgetNode);
}

/*!
  Destructor.
*/
SoGLWidgetNode::~SoGLWidgetNode()
{
}

// Doc from superclass.
void SoGLWidgetNode::initClass(void)
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
