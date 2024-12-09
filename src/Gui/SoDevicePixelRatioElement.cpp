// SPDX-License-Identifier: LGPL-2.1-or-later
 /****************************************************************************
  *                                                                          *
  *   Copyright (c) 2024 Pierre-Louis Boyer <development@ondsel.com>         *
  *                                                                          *
  *   This file is part of FreeCAD.                                          *
  *                                                                          *
  *   FreeCAD is free software: you can redistribute it and/or modify it     *
  *   under the terms of the GNU Lesser General Public License as            *
  *   published by the Free Software Foundation, either version 2.1 of the   *
  *   License, or (at your option) any later version.                        *
  *                                                                          *
  *   FreeCAD is distributed in the hope that it will be useful, but         *
  *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
  *   Lesser General Public License for more details.                        *
  *                                                                          *
  *   You should have received a copy of the GNU Lesser General Public       *
  *   License along with FreeCAD. If not, see                                *
  *   <https://www.gnu.org/licenses/>.                                       *
  *                                                                          *
  ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
#include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/misc/SoState.h>
#endif

#include "SoDevicePixelRatioElement.h"

SO_ELEMENT_SOURCE(SoDevicePixelRatioElement);

void SoDevicePixelRatioElement::initClass() {
    SO_ELEMENT_INIT_CLASS(SoDevicePixelRatioElement, SoFloatElement);
    // Ensure the element is enabled for GLRenderAction
    SO_ENABLE(SoGLRenderAction, SoDevicePixelRatioElement);
}

void SoDevicePixelRatioElement::init(SoState* state) {
    SoFloatElement::init(state);
    data = 1.0f; // Default to a device pixel ratio of 1.0
}

void SoDevicePixelRatioElement::set(SoState* state, float dpr) {
    SoFloatElement::set(classStackIndex, state, dpr);
}

float SoDevicePixelRatioElement::get(SoState* state) {
    return SoFloatElement::get(classStackIndex, state);
}

SoDevicePixelRatioElement::~SoDevicePixelRatioElement() {}