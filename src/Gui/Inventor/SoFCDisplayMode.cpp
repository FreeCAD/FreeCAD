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
#include "SoFCDisplayModeElement.h"
#include "SoFCDisplayMode.h"

SO_NODE_SOURCE(SoFCDisplayMode)

void SoFCDisplayMode::initClass(void)
{
  SO_NODE_INIT_CLASS(SoFCDisplayMode,SoNode,"FCDisplayMode");
}

SoFCDisplayMode::SoFCDisplayMode()
{
  SO_NODE_CONSTRUCTOR(SoFCDisplayMode);

  SO_NODE_ADD_FIELD(faceColor, (SbColor()));
  SO_NODE_ADD_FIELD(lineColor, (SbColor()));
  SO_NODE_ADD_FIELD(transparency, (0.0f));
  SO_NODE_ADD_FIELD(showHiddenLines,  (FALSE));
  SO_NODE_ADD_FIELD(displayMode,  (""));
}

SoFCDisplayMode::~SoFCDisplayMode()
{
}

void SoFCDisplayMode::doAction(SoAction * action)
{
  SoState * state = action->getState();
  if (!this->displayMode.isIgnored()) {
    SoFCDisplayModeElement::set(state, this,
        this->displayMode.getValue(), this->showHiddenLines.getValue());
  }
  if (!this->faceColor.isIgnored() 
      || !this->lineColor.isIgnored()
      || !this->transparency.isIgnored())
  {
    float t = this->transparency.getValue();
    SbColor fc = this->faceColor.getValue();
    SbColor lc = this->lineColor.getValue();
    SoFCDisplayModeElement::setColors(state, this,
        this->faceColor.isIgnored() ? nullptr : &fc,
        this->lineColor.isIgnored() ? nullptr : &lc,
        this->transparency.isIgnored() ? 0.0f : t);
  }
}

void SoFCDisplayMode::GLRender(SoGLRenderAction * action)
{
  doAction(action);
}

void SoFCDisplayMode::callback(SoCallbackAction * action)
{
  doAction(action);
}

// vim: noai:ts=2:sw=2
