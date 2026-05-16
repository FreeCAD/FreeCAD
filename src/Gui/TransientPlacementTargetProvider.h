// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 * Interface that lets any FreeCAD module advertise transient, non-document *
 * placement targets (e.g. the Draft working-plane grid) to TaskTransform.  *
 * Core Gui code only depends on this thin interface; all module-specific   *
 * logic lives in the implementing module.                                  *
 ***************************************************************************/

#pragma once

#include <FCGlobal.h>
#include <optional>
#include <string>
#include <vector>

#include <Base/Placement.h>

class SoPickedPoint;

namespace Gui
{

class View3DInventorViewer;

/** Result returned by a successful transient pick. */
struct GuiExport TransientPickResult
{
    /** World-space placement at the cursor position (rotation = target orientation). */
    Base::Placement placement;
    /** Short human-readable label shown in UI (e.g. in the reference line-edit). */
    std::string label;
};

/**
 * Interface for modules that want to contribute non-document-object pick
 * targets to TaskTransform's selection modes.
 *
 * A provider is activated when TaskTransform enters a pick mode (Select-
 * AlignTarget / SelectTransformOrigin) and deactivated when it leaves.
 * During that window, every raw Coin3D pick event is offered to each
 * registered provider via resolvePick().
 *
 * Lifetime: providers must outlive the registry.  The recommended pattern
 * is a static singleton in a module-init function that self-registers and
 * self-deregisters in its destructor.
 */
class GuiExport TransientPlacementTargetProvider
{
public:
    virtual ~TransientPlacementTargetProvider() = default;

    /** Called when TaskTransform enters a pick mode. */
    virtual void activate(View3DInventorViewer* viewer) = 0;

    /** Called when TaskTransform leaves a pick mode or is destroyed. */
    virtual void deactivate(View3DInventorViewer* viewer) = 0;

    /**
     * Inspect the Coin3D pick result and return a TransientPickResult if this
     * provider claims the hit, or std::nullopt to pass through.
     * @p pp may be nullptr if the pick ray hit nothing.
     */
    virtual std::optional<TransientPickResult> resolvePick(const SoPickedPoint* pp) const = 0;
};

/**
 * Module-level singleton registry for TransientPlacementTargetProvider
 * instances.  Thread-safety is not required: all calls are expected on the
 * main GUI thread.
 */
class GuiExport TransientPlacementTargetRegistry
{
public:
    static TransientPlacementTargetRegistry& instance();

    void add(TransientPlacementTargetProvider* provider);
    void remove(TransientPlacementTargetProvider* provider);
    const std::vector<TransientPlacementTargetProvider*>& all() const;

private:
    std::vector<TransientPlacementTargetProvider*> providers_;
};

}  // namespace Gui
