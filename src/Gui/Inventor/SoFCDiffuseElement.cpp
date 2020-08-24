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

#include <Inventor/elements/SoElements.h>
#include <Inventor/actions/SoCallbackAction.h>
#include "SoFCDiffuseElement.h"

SO_ELEMENT_SOURCE(SoFCDiffuseElement);

void
SoFCDiffuseElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoFCDiffuseElement, inherited);
  SO_ENABLE(SoCallbackAction, SoFCDiffuseElement);
}

void
SoFCDiffuseElement::cleanup()
{
}

void
SoFCDiffuseElement::init(SoState * state)
{
  inherited::init(state);
  this->diffuseId = 0;
  this->transpId = 0;
}

void
SoFCDiffuseElement::push(SoState * state)
{
  inherited::init(state);
  SoFCDiffuseElement *elem = static_cast<SoFCDiffuseElement*>(getNextInStack());
  this->diffuseId = elem->diffuseId;
  this->transpId = elem->transpId;
}

SbBool
SoFCDiffuseElement::matches(const SoElement * element) const
{
  const SoFCDiffuseElement *other = static_cast<const SoFCDiffuseElement *>(element);
  return other->diffuseId == diffuseId && other->transpId == transpId;
}

SoElement *
SoFCDiffuseElement::copyMatchInfo(void) const
{
  assert(getTypeId().canCreateInstance());
  SoFCDiffuseElement * element =
    static_cast<SoFCDiffuseElement *>(getTypeId().createInstance());
  element->diffuseId = this->diffuseId;
  element->transpId = this->transpId;
  return element;
}

SbFCUniqueId
SoFCDiffuseElement::getDiffuseId(void) const
{
  return this->diffuseId;
}

SbFCUniqueId
SoFCDiffuseElement::getTransparencyId(void) const
{
  return this->transpId;
}

SbFCUniqueId
SoFCDiffuseElement::get(SoState * state, SbFCUniqueId * transpid)
{
  const SoFCDiffuseElement * elem = static_cast<const SoFCDiffuseElement*>(
          SoElement::getConstElement(state, getClassStackIndex()));
  if (!elem) {
    if (transpid) *transpid = 0;
    return 0;
  }
  if (transpid) *transpid = elem->transpId;
  return elem->diffuseId;
}

void
SoFCDiffuseElement::set(SoState * state, SbFCUniqueId * diffuseid, SbFCUniqueId *transpid)
{
  SoFCDiffuseElement * elem = static_cast<SoFCDiffuseElement *>(
          SoElement::getElement(state, getClassStackIndex()));
  if (elem) {
    if (diffuseid) elem->diffuseId = *diffuseid;
    if (transpid) elem->transpId = *transpid;
  }
}

// vim: noai:ts=2:sw=2
