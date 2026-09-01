// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel <juergen.riegel@web.de>             *
 *   Copyright (c) 2026 Turan Furkan Topak                                 *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <Inventor/SoPickedPoint.h>

#include <QApplication>
#include <QScopedValueRollback>
#include <QWidget>

#include <utility>

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "EditModeCoinManager.h"
#include "Utils.h"
#include "ViewProviderSketch.h"

namespace SketcherGui
{

std::vector<DimensionReference> ViewProviderSketch::getSelectedDimensionOptionRefs() const
{
    std::vector<DimensionReference> items;

    if (!isInEditMode() || !selection.SelConstraintSet.empty()) {
        return items;
    }

    const auto* sketch = getSketchObject();
    if (!sketch) {
        return items;
    }

    // SelectionObject::SubNames follows the global selection sequence, so no parallel order cache
    // is needed in ViewProviderSketch.
    const auto selectedSketches = Gui::Selection().getSelectionEx(
        sketch->getDocument()->getName(),
        Sketcher::SketchObject::getClassTypeId()
    );
    if (selectedSketches.size() != 1 || selectedSketches.front().getObject() != sketch) {
        return items;
    }

    const auto& subNames = selectedSketches.front().getSubNames();
    items.reserve(subNames.size());
    for (const auto& subName : subNames) {
        int geoId = Sketcher::GeoEnum::GeoUndef;
        Sketcher::PointPos posId = Sketcher::PointPos::none;
        getIdsFromName(subName, sketch, geoId, posId);
        const bool isRoot = geoId == Sketcher::GeoEnum::RtPnt && posId == Sketcher::PointPos::start;
        const bool isAxis = (geoId == Sketcher::GeoEnum::HAxis || geoId == Sketcher::GeoEnum::VAxis)
            && posId == Sketcher::PointPos::none;
        if (geoId != Sketcher::GeoEnum::GeoUndef
            && (isRoot || isAxis || posId != Sketcher::PointPos::none || sketch->getGeometry(geoId))) {
            items.emplace_back(geoId, posId);
        }
    }

    return items;
}

void ViewProviderSketch::setDimensionOptions(const std::vector<DimensionOption>& options)
{
    dimensionOptions = options;
    if (editCoinManager) {
        editCoinManager->setDimensionOptions(dimensionOptions);
    }
    if (auto* view = qobject_cast<Gui::View3DInventor*>(this->getActiveView())) {
        if (auto* viewer = view->getViewer()) {
            viewer->redraw();
        }
    }
}

void ViewProviderSketch::setDimensionOptionMouseGrab(bool grab)
{
    if (!grab && !dimensionOptionInteraction.active) {
        return;
    }

    auto* view = qobject_cast<Gui::View3DInventor*>(getActiveView());
    auto* viewer = view ? view->getViewer() : nullptr;
    auto* widget = viewer ? viewer->getGLWidget() : nullptr;
    if (!widget) {
        return;
    }

    if (grab) {
        widget->grabMouse();
    }
    else if (QWidget::mouseGrabber() == widget) {
        widget->releaseMouse();
    }
}

void ViewProviderSketch::clearDimensionOptions()
{
    setDimensionOptionMouseGrab(false);
    dimensionOptionInteraction = DimensionOptionInteraction();
    setDimensionOptions({});
}

bool ViewProviderSketch::isDimensionOptionPreviewEnabled() const
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher"
    );
    return hGrp->GetBool("EnableDimensionOptionPreview", true);
}

bool ViewProviderSketch::refreshDimensionOptionPreview()
{
    if (!isDimensionOptionPreviewEnabled() || !isInEditMode() || getSolvedSketch().hasConflicts()) {
        clearDimensionOptions();
        return false;
    }

    if (Mode == STATUS_SKETCH_Drag || Mode == STATUS_SKETCH_DragConstraint
        || Mode == STATUS_SKETCH_UseHandler || Mode == STATUS_SKETCH_StartRubberBand
        || Mode == STATUS_SKETCH_UseRubberBand || Mode == STATUS_SELECT_Constraint
        || Mode == STATUS_SELECT_Wire) {
        clearDimensionOptions();
        return false;
    }

    const auto selectionRefs = getSelectedDimensionOptionRefs();
    if (selectionRefs.empty()) {
        clearDimensionOptions();
        return false;
    }

    auto options = buildDimensionOptions(getSketchObject(), selectionRefs);
    if (options.empty()) {
        clearDimensionOptions();
        return false;
    }

    setDimensionOptions(options);
    if (editCoinManager) {
        editCoinManager->setActiveDimensionOption(-1);
    }
    return true;
}

bool ViewProviderSketch::beginDimensionOptionInteraction(
    const QPoint& screenPos,
    const SoPickedPoint* point
)
{
    if (!isDimensionOptionPreviewEnabled() || !isInEditMode() || Mode != STATUS_NONE
        || getSolvedSketch().hasConflicts() || dimensionOptions.empty()) {
        return false;
    }

    if (preselection.isPreselectPointValid()
        || preselection.PreselectCross == Preselection::Axes::RootPoint) {
        return false;
    }

    if (!editCoinManager) {
        return false;
    }

    const int idx = editCoinManager->pickDimensionOption(point);
    if (idx < 0 || idx >= static_cast<int>(dimensionOptions.size())) {
        return false;
    }

    dimensionOptionInteraction.active = true;
    dimensionOptionInteraction.dragged = false;
    dimensionOptionInteraction.finalizing = false;
    dimensionOptionInteraction.optionIndex = idx;
    dimensionOptionInteraction.pressScreenPos = screenPos;
    setDimensionOptionMouseGrab(true);

    editCoinManager->setActiveDimensionOption(idx);
    return true;
}

bool ViewProviderSketch::updateDimensionOptionInteraction(
    const QPoint& screenPos,
    const Base::Vector2d& onSketchPos
)
{
    if (!dimensionOptionInteraction.active || dimensionOptionInteraction.finalizing) {
        return false;
    }

    const int idx = dimensionOptionInteraction.optionIndex;
    if (idx < 0 || idx >= static_cast<int>(dimensionOptions.size())) {
        cancelDimensionOptionInteraction();
        return false;
    }

    const int dragDistance = (screenPos - dimensionOptionInteraction.pressScreenPos).manhattanLength();
    if (!dimensionOptionInteraction.dragged && dragDistance < QApplication::startDragDistance()) {
        return false;
    }

    dimensionOptions[idx].customLabelPosition = onSketchPos;
    dimensionOptionInteraction.dragged = true;
    setDimensionOptions(dimensionOptions);
    if (editCoinManager) {
        editCoinManager->setActiveDimensionOption(idx);
    }
    return true;
}

bool ViewProviderSketch::finalizeDimensionOptionInteraction()
{
    if (dimensionOptionInteraction.finalizing) {
        return true;
    }

    if (!dimensionOptionInteraction.active) {
        return false;
    }

    const int idx = dimensionOptionInteraction.optionIndex;
    if (idx < 0 || idx >= static_cast<int>(dimensionOptions.size())) {
        cancelDimensionOptionInteraction();
        return false;
    }

    auto* sketch = getSketchObject();
    if (!sketch) {
        cancelDimensionOptionInteraction();
        return false;
    }

    DimensionOption option = dimensionOptions[idx];
    if (editCoinManager) {
        if (auto resolvedOption = editCoinManager->resolveDimensionOption(idx)) {
            option = std::move(*resolvedOption);
        }
    }

    QScopedValueRollback<bool> finalizingGuard(dimensionOptionInteraction.finalizing, true);
    setDimensionOptionMouseGrab(false);
    dimensionOptionInteraction.active = false;
    setDimensionOptions({});

    const bool ok = commitDimensionOption(*sketch, option);

    dimensionOptionInteraction = DimensionOptionInteraction();
    if (!ok) {
        refreshDimensionOptionPreview();
    }
    return ok;
}

void ViewProviderSketch::cancelDimensionOptionInteraction()
{
    if (!dimensionOptionInteraction.active || dimensionOptionInteraction.finalizing) {
        return;
    }

    const int index = dimensionOptionInteraction.optionIndex;
    const bool restoreDefaultPlacement = dimensionOptionInteraction.dragged && index >= 0
        && index < static_cast<int>(dimensionOptions.size());
    setDimensionOptionMouseGrab(false);
    dimensionOptionInteraction = DimensionOptionInteraction();
    if (restoreDefaultPlacement) {
        dimensionOptions[index].customLabelPosition.reset();
        setDimensionOptions(dimensionOptions);
    }
    if (editCoinManager) {
        editCoinManager->setActiveDimensionOption(-1);
    }
}

}  // namespace SketcherGui
