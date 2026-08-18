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

#include "CoinRenderSupport.h"

#include <Inventor/SoRenderManager.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoLazyElement.h>

#if FC_COIN_HAVE_DEVICE_PIXEL_RATIO
# include <Inventor/elements/SoDevicePixelRatioElement.h>
#else
# include "SoFCDevicePixelRatioElement.h"
#endif

namespace Gui::CoinRenderSupport
{

void invalidateScene(SoRenderManager* manager)
{
    if (!manager) {
        return;
    }
#if FC_COIN_HAVE_RENDER_INVALIDATION
    manager->invalidateScene();
#else
    manager->scheduleRedraw();
#endif
}

void invalidateForeground(SoRenderManager* manager)
{
    if (!manager) {
        return;
    }
#if FC_COIN_HAVE_RENDER_INVALIDATION
    manager->invalidateForeground();
#else
    manager->scheduleRedraw();
#endif
}

void invalidateSharedGLState(SoRenderManager* manager)
{
    if (!manager) {
        return;
    }
#if FC_COIN_HAVE_SHARED_GL_STATE_INVALIDATION
    manager->invalidateSharedGLState();
#else
    SoGLRenderAction* action = manager->getGLRenderAction();
    if (!action || !action->getState()) {
        return;
    }
    SoGLLazyElement::getInstance(action->getState())->reset(action->getState(), SoLazyElement::ALL_MASK);
#endif
}

void releaseRenderBackendResources(SoRenderManager* manager)
{
#if FC_COIN_HAVE_RENDER_BACKEND_LIFECYCLE
    if (manager) {
        manager->releaseRenderBackendResources();
    }
#else
    (void)manager;
#endif
}

void discardRenderBackendResources(SoRenderManager* manager)
{
#if FC_COIN_HAVE_RENDER_BACKEND_LIFECYCLE
    if (manager) {
        manager->discardRenderBackendResources();
    }
#else
    (void)manager;
#endif
}

void setDevicePixelRatio(SoState* state, float ratio)
{
    if (!state) {
        return;
    }
#if FC_COIN_HAVE_DEVICE_PIXEL_RATIO
    SoDevicePixelRatioElement::set(state, ratio);
#else
    SoFCDevicePixelRatioElement::set(state, ratio);
#endif
}

float devicePixelRatio(SoState* state)
{
    if (!state) {
        return 1.0F;
    }
#if FC_COIN_HAVE_DEVICE_PIXEL_RATIO
    if (!state->isElementEnabled(SoDevicePixelRatioElement::getClassStackIndex())) {
        return 1.0F;
    }
    return SoDevicePixelRatioElement::get(state);
#else
    if (!state->isElementEnabled(SoFCDevicePixelRatioElement::getClassStackIndex())) {
        return 1.0F;
    }
    return SoFCDevicePixelRatioElement::get(state);
#endif
}

}  // namespace Gui::CoinRenderSupport
