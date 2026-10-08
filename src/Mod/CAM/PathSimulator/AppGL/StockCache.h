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

#pragma once

#include <functional>

namespace CAMSimulator
{

class SimDisplay;

/**
 * The cut stock as drawn up to a step, kept between frames. While the view is unchanged and
 * playback moves forward, a frame draws only the new moves into it; any other change draws
 * everything again. It holds only the stock, so showing or hiding anything else keeps it.
 */
class StockCache
{
public:
    // the stock or the program changed: the next frame draws everything again
    void Invalidate();
    // Draws the stock cut up to pathStep / subStep. renderSweeps(first, fromScratch) draws the
    // sweeps from segment first on, from the uncut stock when fromScratch.
    void Render(
        SimDisplay& display,
        int pathStep,
        int subStep,
        const std::function<void(int first, bool fromScratch)>& renderSweeps
    );

private:
    bool mValid = false;
    int mPathStep = -1;
    int mSubStep = 0;
    unsigned int mViewVersion = 0;
};

}  // namespace CAMSimulator
