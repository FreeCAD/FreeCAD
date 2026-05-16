// SPDX-License-Identifier: LGPL-2.1-or-later

#include "DraftWPPickProvider.h"

#include <Base/Interpreter.h>
#include <Base/PlacementPy.h>

#include <Gui/View3DInventorViewer.h>

#include <Inventor/SoPath.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/nodes/SoPickStyle.h>

using namespace DraftGui;

// ---------------------------------------------------------------------------
// activate / deactivate
// ---------------------------------------------------------------------------

void DraftWPPickProvider::activate(Gui::View3DInventorViewer* viewer)
{
    setGridPickable(viewer, true);
}

void DraftWPPickProvider::deactivate(Gui::View3DInventorViewer* viewer)
{
    setGridPickable(viewer, false);
}

void DraftWPPickProvider::setGridPickable(Gui::View3DInventorViewer* viewer, bool pickable)
{
    if (!viewer) {
        return;
    }

    // Find the SoSwitch node named "gridTracker" in the scene graph.
    SoSearchAction sa;
    sa.setName("gridTracker");
    sa.setInterest(SoSearchAction::FIRST);
    sa.apply(viewer->getSceneGraph());
    SoPath* path = sa.getPath();
    if (!path || path->getLength() == 0) {
        return;
    }

    // Find the SoPickStyle anywhere under the gridTracker node.
    SoSearchAction sp;
    sp.setType(SoPickStyle::getClassTypeId());
    sp.setInterest(SoSearchAction::FIRST);
    sp.apply(path->getTail());
    SoPath* psPath = sp.getPath();
    if (!psPath || psPath->getLength() == 0) {
        return;
    }

    auto* ps = dynamic_cast<SoPickStyle*>(psPath->getTail());
    if (!ps) {
        return;
    }
    ps->style = pickable ? SoPickStyle::SHAPE : SoPickStyle::UNPICKABLE;
}

// ---------------------------------------------------------------------------
// resolvePick
// ---------------------------------------------------------------------------

std::optional<Gui::TransientPickResult> DraftWPPickProvider::resolvePick(const SoPickedPoint* pp) const
{
    if (!pp) {
        return std::nullopt;
    }

    // Check whether the pick path passes through the "gridTracker" node.
    const SoPath* path = pp->getPath();
    bool isGrid = false;
    for (int i = 0; i < path->getLength(); ++i) {
        if (std::string(path->getNode(i)->getName().getString()) == "gridTracker") {
            isGrid = true;
            break;
        }
    }
    if (!isGrid) {
        return std::nullopt;
    }

    // Get the hit position on the grid plane.
    SbVec3f hit = pp->getPoint();
    Base::Vector3d clickPos(hit[0], hit[1], hit[2]);

    // Get the Working Plane's orientation via Python.
    Base::Placement wpPlacement;
    try {
        // Base::Interpreter().getValue() acquires the GIL internally.
        PyObject* pyResult = Base::Interpreter().getValue(
            "import WorkingPlane as _FC_WP_M_\n"
            "_FC_WP_PL_ = _FC_WP_M_.get_working_plane().get_placement()\n",
            "_FC_WP_PL_"
        );
        if (pyResult && PyObject_TypeCheck(pyResult, &Base::PlacementPy::Type)) {
            wpPlacement = *static_cast<Base::PlacementPy*>(pyResult)->getPlacementPtr();
        }
        Py_XDECREF(pyResult);
    }
    catch (...) {
        return std::nullopt;
    }

    Gui::TransientPickResult result;
    result.placement = Base::Placement(clickPos, wpPlacement.getRotation());
    result.label = "Draft Working Plane";
    return result;
}
