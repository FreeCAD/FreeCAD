// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
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

#include "StockCache.h"

#include "SimDisplay.h"

namespace CAMSimulator
{

void StockCache::Invalidate()
{
    mValid = false;
}

void StockCache::Render(
    SimDisplay& display,
    int pathStep,
    int subStep,
    const std::function<void(int first, bool fromScratch)>& renderSweeps
)
{
    // The move under way is drawn again in full each frame.
    const bool forward = pathStep > mPathStep || (pathStep == mPathStep && subStep >= mSubStep);
    const bool reuse = mValid && mPathStep >= 0 && forward && mViewVersion == display.ViewVersion();

    display.BeginCacheDraw(!reuse);
    renderSweeps(reuse ? mPathStep : 0, !reuse);
    display.EndCacheDraw();
    display.CopyCacheToFrame();

    mValid = true;
    mPathStep = pathStep;
    mSubStep = subStep;
    mViewVersion = display.ViewVersion();
}

}  // namespace CAMSimulator
