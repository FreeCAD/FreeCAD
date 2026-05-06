// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Turan Furkan Topak <furkan1795@gmail.com>          *
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

#include "ConstraintLazySelection.h"
#include "ViewProviderSketch.h"

#include <algorithm>
#include <cstring>

#include <App/IndexedName.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Mod/Part/App/Geometry.h>

namespace SketcherGui
{

class ViewProviderSketchLazySelectionAttorney
{
public:
    static int getPreselectLazyExternal(const ViewProviderSketch& viewProvider);
    static void resetPreselectPoint(ViewProviderSketch& viewProvider);
};

int ViewProviderSketchLazySelectionAttorney::getPreselectLazyExternal(
    const ViewProviderSketch& viewProvider
)
{
    return viewProvider.getPreselectLazyExternal();
}

void ViewProviderSketchLazySelectionAttorney::resetPreselectPoint(ViewProviderSketch& viewProvider)
{
    viewProvider.resetPreselectPoint();
}

namespace
{
constexpr const char* LazyExternalVertexPrefix = "LazyExternalVertex";
constexpr const char* LazyExternalEdgePrefix = "LazyExternalEdge";

std::string makeExternalSubNameFromGeoId(int geoId)
{
    return "ExternalEdge" + std::to_string(Sketcher::GeoEnum::RefExt + 1 - geoId);
}

std::string makeLazyExternalSubName(int lazyId, bool lazyVertex)
{
    return std::string(lazyVertex ? LazyExternalVertexPrefix : LazyExternalEdgePrefix)
        + std::to_string(lazyId);
}

bool containsSubName(const std::vector<std::string>& subNames, const std::string& subName)
{
    return std::find(subNames.begin(), subNames.end(), subName) != subNames.end();
}

bool containsSelectionPair(const std::vector<SelIdPair>& items, const SelIdPair& item)
{
    return std::any_of(items.begin(), items.end(), [&](const SelIdPair& existing) {
        return selectionPairsEqual(existing, item);
    });
}

std::vector<int> collectPendingLazyIds(ViewProviderSketch* sketchgui)
{
    std::vector<int> lazyIds;
    if (!sketchgui) {
        return lazyIds;
    }

    lazyIds = sketchgui->getSelectedLazyExternalReferenceIds();
    const int preselectLazyId = ViewProviderSketchLazySelectionAttorney::getPreselectLazyExternal(
        *sketchgui
    );
    if (preselectLazyId >= 0
        && std::find(lazyIds.begin(), lazyIds.end(), preselectLazyId) == lazyIds.end()) {
        lazyIds.push_back(preselectLazyId);
    }

    return lazyIds;
}

bool isLazyExternalVertexReference(ViewProviderSketch* sketchgui, int lazyId)
{
    return sketchgui && sketchgui->isLazyExternalReferenceVertex(lazyId);
}

template<typename Callback>
void forEachPendingLazyReference(
    ViewProviderSketch* sketchgui,
    bool includeEdges,
    bool includeVertices,
    Callback&& callback
)
{
    for (int lazyId : collectPendingLazyIds(sketchgui)) {
        const bool lazyVertex = isLazyExternalVertexReference(sketchgui, lazyId);
        if ((lazyVertex && includeVertices) || (!lazyVertex && includeEdges)) {
            callback(lazyId, lazyVertex);
        }
    }
}


void clearPendingSelection(ViewProviderSketch* sketchgui)
{
    if (!sketchgui) {
        return;
    }

    sketchgui->clearSelectedLazyExternalReferences();
    ViewProviderSketchLazySelectionAttorney::resetPreselectPoint(*sketchgui);
}

bool makeSelectionPair(
    ViewProviderSketch* sketchgui,
    int lazyId,
    bool lazyVertex,
    SelIdPair& item,
    std::string* subName = nullptr,
    Base::Type* geoType = nullptr
)
{
    if (!sketchgui || lazyId < 0) {
        return false;
    }

    item.GeoId = Sketcher::GeoEnum::GeoUndef;
    item.PosId = lazyVertex ? Sketcher::PointPos::start : Sketcher::PointPos::none;
    item.LazyExternalId = lazyId;
    item.IsLazyExternal = true;
    item.LazyExternalVertex = lazyVertex;

    if (subName) {
        *subName = makeLazyExternalSubName(lazyId, lazyVertex);
    }
    if (geoType) {
        *geoType = lazyVertex ? Part::GeomPoint::getClassTypeId()
                              : sketchgui->getLazyExternalGeometryType(lazyId);
    }

    return true;
}

bool materializeSelectionPair(ViewProviderSketch* sketchgui, SelIdPair& item)
{
    if (!item.IsLazyExternal) {
        return true;
    }
    if (!sketchgui) {
        return false;
    }

    const int geoId = sketchgui->materializeLazyExternalReference(item.LazyExternalId);
    if (geoId == Sketcher::GeoEnum::GeoUndef) {
        return false;
    }

    item.GeoId = geoId;
    item.PosId = item.LazyExternalVertex ? Sketcher::PointPos::start : Sketcher::PointPos::none;
    return true;
}


}  // namespace


ActivatedSelection::ActivatedSelection(
    Gui::Command& command,
    Gui::Document* guiDocument,
    bool includeLazyVertices,
    bool* sharedCommandActive
)
    : command(&command)
    , sharedCommandActive(sharedCommandActive)
{
    begin(guiDocument, includeLazyVertices);
    selection = Gui::Command::getSelection().getSelectionEx();
}

ActivatedSelection::ActivatedSelection(ActivatedSelection&& other) noexcept
    : command(other.command)
    , sharedCommandActive(other.sharedCommandActive)
    , selection(std::move(other.selection))
    , localCommandActive(other.localCommandActive)
    , abortOnDestroy(other.abortOnDestroy)
{
    other.command = nullptr;
    other.sharedCommandActive = nullptr;
    other.localCommandActive = false;
    other.abortOnDestroy = false;
}

ActivatedSelection::~ActivatedSelection()
{
    if (abortOnDestroy) {
        abortLazyCommand();
    }
}

const std::vector<Gui::SelectionObject>& ActivatedSelection::getSelection() const
{
    return selection;
}

void ActivatedSelection::openCommand(const char* name)
{
    if (isLazyCommandActive()) {
        return;
    }

    command->openCommand(name);
}

void ActivatedSelection::commitCommand()
{
    command->commitCommand();
    setLazyCommandActive(false);
    abortOnDestroy = false;
}

void ActivatedSelection::abortCommand()
{
    command->abortCommand();
    setLazyCommandActive(false);
    abortOnDestroy = false;
}

void ActivatedSelection::begin(Gui::Document* guiDocument, bool includeLazyVertices)
{
    auto* sketchgui = getActiveSketchGui(guiDocument);
    if (!LazySelectionResolver::hasPendingLazySelection(sketchgui, true, includeLazyVertices)) {
        return;
    }

    command->openCommand(QT_TRANSLATE_NOOP("Command", "Add constraint"));
    setLazyCommandActive(true);

    if (!LazySelectionResolver::materializePendingLazySelection(sketchgui, includeLazyVertices)) {
        abortCommand();
    }
}

void ActivatedSelection::abortLazyCommand()
{
    if (isLazyCommandActive()) {
        abortCommand();
    }
}

bool ActivatedSelection::isLazyCommandActive() const
{
    return sharedCommandActive ? *sharedCommandActive : localCommandActive;
}

void ActivatedSelection::setLazyCommandActive(bool active)
{
    if (sharedCommandActive) {
        *sharedCommandActive = active;
    }
    else {
        localCommandActive = active;
    }
}

ViewProviderSketch* getActiveSketchGui(Gui::Document* guiDocument)
{
    return guiDocument ? freecad_cast<ViewProviderSketch*>(guiDocument->getInEdit()) : nullptr;
}

bool selectionPairsEqual(const SelIdPair& lhs, const SelIdPair& rhs)
{
    if (lhs.IsLazyExternal || rhs.IsLazyExternal) {
        return lhs.IsLazyExternal == rhs.IsLazyExternal && lhs.LazyExternalId == rhs.LazyExternalId
            && lhs.LazyExternalVertex == rhs.LazyExternalVertex;
    }
    return lhs.GeoId == rhs.GeoId && lhs.PosId == rhs.PosId;
}

std::optional<LazyExternalSubName> parseLazyExternalSubName(const std::string& subName)
{
    const Data::IndexedName indexedSubName(subName.c_str());
    if (!indexedSubName || indexedSubName.getIndex() <= 0) {
        return std::nullopt;
    }

    if (std::strcmp(indexedSubName.getType(), LazyExternalVertexPrefix) == 0) {
        return LazyExternalSubName {indexedSubName.getIndex(), true};
    }
    if (std::strcmp(indexedSubName.getType(), LazyExternalEdgePrefix) == 0) {
        return LazyExternalSubName {indexedSubName.getIndex(), false};
    }
    return std::nullopt;
}

bool LazySelectionResolver::hasPendingLazySelection(
    ViewProviderSketch* sketchgui,
    bool includeEdges,
    bool includeVertices
)
{
    bool hasPending = false;
    forEachPendingLazyReference(sketchgui, includeEdges, includeVertices, [&](int, bool) {
        hasPending = true;
    });
    return hasPending;
}

bool LazySelectionResolver::materializePendingLazySelection(
    ViewProviderSketch* sketchgui,
    bool includeLazyVertices
)
{
    if (!sketchgui || !sketchgui->getSketchObject()) {
        return true;
    }

    bool consumedLazySelection = false;
    bool materializationFailed = false;
    forEachPendingLazyReference(sketchgui, true, includeLazyVertices, [&](int lazyId, bool) {
        consumedLazySelection = true;
        const int geoId = sketchgui->materializeLazyExternalReference(lazyId);
        if (geoId == Sketcher::GeoEnum::GeoUndef) {
            materializationFailed = true;
            return;
        }

        const std::string subName = makeExternalSubNameFromGeoId(geoId);
        if (!sketchgui->isSelected(subName)) {
            sketchgui->addSelection(subName);
        }
    });

    if (consumedLazySelection) {
        clearPendingSelection(sketchgui);
    }
    return !materializationFailed;
}

void LazySelectionResolver::appendPendingLazySubNames(
    ViewProviderSketch* sketchgui,
    std::vector<std::string>& subNames,
    bool includeEdges,
    bool includeVertices
)
{
    forEachPendingLazyReference(sketchgui, includeEdges, includeVertices, [&](int lazyId, bool lazyVertex) {
        const std::string lazySubName = makeLazyExternalSubName(lazyId, lazyVertex);
        if (!containsSubName(subNames, lazySubName)) {
            subNames.push_back(lazySubName);
        }
    });
}

void LazySelectionResolver::appendPendingLazySelectionPairs(
    ViewProviderSketch* sketchgui,
    std::vector<SelIdPair>& points,
    std::vector<SelIdPair>& curves,
    bool includeEdges,
    bool includeVertices
)
{
    forEachPendingLazyReference(sketchgui, includeEdges, includeVertices, [&](int lazyId, bool lazyVertex) {
        SelIdPair item;
        if (!makeSelectionPair(sketchgui, lazyId, lazyVertex, item)) {
            return;
        }

        auto& target = lazyVertex ? points : curves;
        if (!containsSelectionPair(target, item)) {
            target.push_back(item);
        }
        sketchgui->setLazyExternalReferenceSelected(lazyId, true);
    });
}

void LazySelectionResolver::clearPendingLazySelection(ViewProviderSketch* sketchgui)
{
    clearPendingSelection(sketchgui);
}

bool LazySelectionResolver::makeLazySelectionPair(
    ViewProviderSketch* sketchgui,
    int lazyId,
    bool lazyVertex,
    SelIdPair& item,
    std::string* subName,
    Base::Type* geoType
)
{
    return makeSelectionPair(sketchgui, lazyId, lazyVertex, item, subName, geoType);
}

void LazySelectionResolver::setLazySelectionSelected(
    ViewProviderSketch* sketchgui,
    const SelIdPair& item,
    bool selected
)
{
    if (sketchgui && item.IsLazyExternal) {
        sketchgui->setLazyExternalReferenceSelected(item.LazyExternalId, selected);
    }
}

bool LazySelectionResolver::materializeLazySelectionPairs(
    ViewProviderSketch* sketchgui,
    std::vector<SelIdPair>& items
)
{
    bool consumedLazySelection = false;
    for (auto& item : items) {
        consumedLazySelection = consumedLazySelection || item.IsLazyExternal;
        if (!materializeSelectionPair(sketchgui, item)) {
            if (consumedLazySelection) {
                clearPendingSelection(sketchgui);
            }
            return false;
        }
    }

    if (consumedLazySelection) {
        clearPendingSelection(sketchgui);
    }
    return true;
}


}  // namespace SketcherGui
