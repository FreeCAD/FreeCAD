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

#pragma once

#include <Inventor/elements/SoFloatElement.h>

class SoState;

class SoDevicePixelRatioElement: public SoFloatElement
{
    SO_ELEMENT_HEADER(SoDevicePixelRatioElement);

public:
    // Initializes the class
    static void initClass();

    // Initializes the element
    virtual void init(SoState* state) override;

    // Sets the device pixel ratio
    static void set(SoState* state, float dpr);

    // Retrieves the device pixel ratio
    static float get(SoState* state);

protected:
    virtual ~SoDevicePixelRatioElement();
};
