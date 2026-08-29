// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Inventor/SbViewVolume.h>
#include <Inventor/SbViewportRegion.h>

#include <Gui/Utilities.h>

class SoPath;

namespace SketcherGui
{

/** The Coin rendering, viewport, and tolerance state shared by one screen-pick operation. */
struct ScreenPickContext
{
    SbViewportRegion renderViewportRegion;
    SbViewVolume renderViewVolume;
    // Borrowed from the SoSearchAction that created this context. It remains valid for the
    // duration of the preselection operation.
    const SoPath* constraintGroupPath = nullptr;

    [[nodiscard]] SbVec2f projectRenderPointToViewport(const SbVec3f& point) const
    {
        return Gui::projectToViewportPixels(renderViewVolume, renderViewportRegion, point);
    }
};

}  // namespace SketcherGui
