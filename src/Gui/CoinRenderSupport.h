// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD Project Association
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include "CoinRenderFeatures.h"

class SoRenderManager;
class SoState;

namespace Gui::CoinRenderSupport
{

constexpr int rendererApiVersion()
{
    return FC_COIN_RENDERER_API_VERSION;
}

constexpr bool hasDrawListStack()
{
    return FC_COIN_HAVE_DRAWLIST_STACK;
}

void invalidateScene(SoRenderManager* manager);
void invalidateForeground(SoRenderManager* manager);
void invalidateSharedGLState(SoRenderManager* manager);
void releaseRenderBackendResources(SoRenderManager* manager);
void discardRenderBackendResources(SoRenderManager* manager);

void setDevicePixelRatio(SoState* state, float ratio);
float devicePixelRatio(SoState* state);

}  // namespace Gui::CoinRenderSupport
