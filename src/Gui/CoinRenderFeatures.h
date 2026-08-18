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

#include <Inventor/C/basic.h>

// Normalize independently-upstreamable Coin renderer capabilities here. GUI
// code must not infer API availability from FREECAD_USE_EXTERNAL_COIN_PIVY.

#if defined(COIN_HAVE_DEVICE_PIXEL_RATIO)
# define FC_COIN_HAVE_DEVICE_PIXEL_RATIO 1
#else
# define FC_COIN_HAVE_DEVICE_PIXEL_RATIO 0
#endif

#if defined(COIN_HAVE_RENDER_INVALIDATION)
# define FC_COIN_HAVE_RENDER_INVALIDATION 1
#else
# define FC_COIN_HAVE_RENDER_INVALIDATION 0
#endif

#if defined(COIN_HAVE_SHARED_GL_STATE_INVALIDATION)
# define FC_COIN_HAVE_SHARED_GL_STATE_INVALIDATION 1
#else
# define FC_COIN_HAVE_SHARED_GL_STATE_INVALIDATION 0
#endif

#if defined(COIN_HAVE_RENDER_BACKEND_LIFECYCLE)
# define FC_COIN_HAVE_RENDER_BACKEND_LIFECYCLE 1
#else
# define FC_COIN_HAVE_RENDER_BACKEND_LIFECYCLE 0
#endif

#if defined(COIN_HAVE_RENDER_MANAGER_STAGES)
# define FC_COIN_HAVE_RENDER_MANAGER_STAGES 1
#else
# define FC_COIN_HAVE_RENDER_MANAGER_STAGES 0
#endif

#if defined(COIN_HAVE_RENDER_LAYER_GROUP)
# define FC_COIN_HAVE_RENDER_LAYER_GROUP 1
#else
# define FC_COIN_HAVE_RENDER_LAYER_GROUP 0
#endif

#if defined(COIN_HAVE_IR_RENDER_ACTION)
# define FC_COIN_HAVE_IR_RENDER_ACTION 1
#else
# define FC_COIN_HAVE_IR_RENDER_ACTION 0
#endif

#if defined(COIN_HAVE_RENDER_PIPELINES)
# define FC_COIN_HAVE_RENDER_PIPELINES 1
#else
# define FC_COIN_HAVE_RENDER_PIPELINES 0
#endif

#if defined(COIN_HAVE_RETAINED_PICKING)
# define FC_COIN_HAVE_RETAINED_PICKING 1
#else
# define FC_COIN_HAVE_RETAINED_PICKING 0
#endif

#if defined(COIN_RENDERER_API_VERSION)
# define FC_COIN_RENDERER_API_VERSION COIN_RENDERER_API_VERSION
#else
# define FC_COIN_RENDERER_API_VERSION 0
#endif

#define FC_COIN_HAVE_DRAWLIST_STACK \
    (FC_COIN_HAVE_IR_RENDER_ACTION && FC_COIN_HAVE_RENDER_PIPELINES \
     && FC_COIN_HAVE_RENDER_MANAGER_STAGES && FC_COIN_HAVE_RENDER_BACKEND_LIFECYCLE)
