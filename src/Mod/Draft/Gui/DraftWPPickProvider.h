// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 * Draft Working Plane pick provider for TaskTransform's transient-pick    *
 * infrastructure.  Implements Gui::TransientPlacementTargetProvider so    *
 * that TaskTransform can snap the dragger to the Draft WP grid without    *
 * any Draft-specific knowledge in core Gui code.                          *
 ***************************************************************************/

#pragma once

#include <Gui/TransientPlacementTargetProvider.h>

class SoPickedPoint;

namespace DraftGui
{

/**
 * Lets TaskTransform snap to the Draft Working Plane grid.
 *
 * activate() makes the grid temporarily pickable.
 * deactivate() restores the grid to UNPICKABLE.
 * resolvePick() checks whether the pick path passes through the
 * "gridTracker" Coin node and, if so, returns a placement whose
 * position is the cursor hit point and whose rotation matches the
 * current WP orientation.
 */
class DraftWPPickProvider: public Gui::TransientPlacementTargetProvider
{
public:
    void activate(Gui::View3DInventorViewer* viewer) override;
    void deactivate(Gui::View3DInventorViewer* viewer) override;
    std::optional<Gui::TransientPickResult> resolvePick(const SoPickedPoint* pp) const override;

private:
    void setGridPickable(Gui::View3DInventorViewer* viewer, bool pickable);
};

}  // namespace DraftGui
