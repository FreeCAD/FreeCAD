// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Boyer Pierre-Louis <pierrelouis.boyer@gmail.com>    *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "LinkArray.h"

#include <algorithm>

#include <gp_Trsf.hxx>

#include <App/Document.h>
#include <Base/Matrix.h>
#include <Base/Tools.h>

using namespace Part;

PROPERTY_SOURCE_WITH_EXTENSIONS(Part::LinkArray, App::Link)

LinkArray::LinkArray()
{
    enforceLinkArrayPropertyStatus();
}

App::DocumentObjectExecReturn* LinkArray::execute()
{
    enforceLinkArrayPropertyStatus();
    syncGeneratedElementPlacements(getElementPlacements());

    return inherited::execute();
}

void LinkArray::onDocumentRestored()
{
    inherited::onDocumentRestored();
    enforceLinkArrayPropertyStatus();
}

void LinkArray::onChanged(const App::Property* prop)
{
    const bool keepLabel = prop == &LinkedObject && !isRestoring();
    std::string label;
    if (keepLabel) {
        label = Label.getValue();
    }

    inherited::onChanged(prop);

    if (keepLabel && Label.getValue() != label) {
        Label.setValue(label);
        Label.purgeTouched();
    }
}

Base::Placement LinkArray::getPlacementOf(const std::string& sub, App::DocumentObject* targetObj)
{
    Base::Placement placement;
    auto* propPlacement = dynamic_cast<App::PropertyPlacement*>(getPropertyByName("Placement"));
    if (propPlacement) {
        placement = propPlacement->getValue();
    }

    if (sub.empty() || this == targetObj) {
        return placement;
    }

    const char* remaining = nullptr;
    const int index = getElementIndex(sub.c_str(), &remaining);
    if (index < 0) {
        return placement;
    }

    const auto elements = ElementList.getValues();
    if (ShowElement.getValue() && elements.size() > static_cast<size_t>(index) && elements[index]) {
        return placement * elements[index]->getPlacementOf(remaining ? remaining : "", targetObj);
    }

    const auto placements = PlacementList.getValues();
    if (placements.size() <= static_cast<size_t>(index)) {
        return placement;
    }

    placement = placement * placements[index];
    if (!remaining || !remaining[0]) {
        return placement;
    }

    App::DocumentObject* linked = getTrueLinkedObject(false);
    if (!linked) {
        return placement;
    }

    std::vector<std::string> names = Base::Tools::splitSubName(remaining);
    if (names.empty()) {
        return placement;
    }

    App::DocumentObject* subObj = linked->getDocument()->getObject(names.front().c_str());
    if (!subObj) {
        return placement;
    }

    std::vector<std::string> newNames(names.begin() + 1, names.end());
    std::string newSub = Base::Tools::joinList(newNames, ".");

    return placement * subObj->getPlacementOf(newSub, targetObj);
}

bool LinkArray::isLink() const
{
    return true;
}

bool LinkArray::isLinkGroup() const
{
    return false;
}

std::vector<Base::Placement> LinkArray::getElementPlacements()
{
    if (PlacementList.getSize()) {
        return PlacementList.getValues();
    }

    return {Base::Placement()};
}

void LinkArray::syncGeneratedElementPlacements(const std::vector<Base::Placement>& placements)
{
    const auto count = static_cast<int>(placements.size());

    if (ElementCount.getValue() != count) {
        ElementCount.setStatus(App::Property::Immutable, false);
        ElementCount.setValue(count);
        ElementCount.setStatus(App::Property::Immutable, true);
    }

    if (ShowElement.getValue()) {
        syncGeneratedElementLinkPlacements(placements);
        return;
    }

    if (PlacementList.getValues() != placements) {
        PlacementList.setStatus(App::Property::Immutable, false);
        PlacementList.setValue(placements);
        PlacementList.setStatus(App::Property::Immutable, true);
    }
}

void LinkArray::syncGeneratedElementLinkPlacements(const std::vector<Base::Placement>& placements)
{
    const std::vector<App::DocumentObject*> elements = ElementList.getValues();
    const auto count = std::min(elements.size(), placements.size());

    for (size_t i = 0; i < count; ++i) {
        auto* placement = dynamic_cast<App::PropertyPlacement*>(
            elements[i] ? elements[i]->getPropertyByName("Placement") : nullptr
        );
        if (!placement || placement->getValue().isSame(placements[i])) {
            continue;
        }

        placement->setValue(placements[i]);
        elements[i]->purgeTouched();
    }
}

void LinkArray::enforceLinkArrayPropertyStatus()
{
    _LinkTouched.setStatus(App::Property::Output, true);
    _LinkTouched.setStatus(App::Property::NoRecompute, true);
    ShowElement.setStatus(App::Property::Immutable, false);
    ShowElement.setStatus(App::Property::Hidden, false);
    ShowElement.setStatus(App::Property::NoRecompute, false);
    ElementCount.setStatus(App::Property::Immutable, true);
    ElementCount.setStatus(App::Property::Hidden, true);
    ElementCount.setStatus(App::Property::Output, true);
    ElementCount.setStatus(App::Property::NoRecompute, true);
    PlacementList.setStatus(App::Property::Hidden, true);
    PlacementList.setStatus(App::Property::Immutable, true);
    PlacementList.setStatus(App::Property::Output, true);
    PlacementList.setStatus(App::Property::NoRecompute, true);
    ScaleList.setStatus(App::Property::Hidden, true);
    ScaleList.setStatus(App::Property::Immutable, true);
    ScaleList.setStatus(App::Property::Output, true);
    ScaleList.setStatus(App::Property::NoRecompute, true);
    ElementList.setStatus(App::Property::Hidden, true);
    ElementList.setStatus(App::Property::Immutable, true);
    ElementList.setStatus(App::Property::Output, true);
    ElementList.setStatus(App::Property::NoRecompute, true);
}

Base::Placement LinkArray::placementFromTransform(const gp_Trsf& transform)
{
    Base::Matrix4D matrix(
        transform.Value(1, 1),
        transform.Value(1, 2),
        transform.Value(1, 3),
        transform.Value(1, 4),
        transform.Value(2, 1),
        transform.Value(2, 2),
        transform.Value(2, 3),
        transform.Value(2, 4),
        transform.Value(3, 1),
        transform.Value(3, 2),
        transform.Value(3, 3),
        transform.Value(3, 4),
        0.0,
        0.0,
        0.0,
        1.0
    );

    return Base::Placement(matrix);
}

std::vector<Base::Placement> LinkArray::placementsFromTransforms(
    const std::list<gp_Trsf>& transformations
)
{
    std::vector<Base::Placement> placements;
    placements.reserve(transformations.size());

    for (const auto& transform : transformations) {
        placements.push_back(placementFromTransform(transform));
    }

    return placements;
}
