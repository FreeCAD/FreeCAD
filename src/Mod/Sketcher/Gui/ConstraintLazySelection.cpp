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
#include <utility>

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
    static int getPreselectLazyExternalId(const ViewProviderSketch& viewProvider);
    static Base::Type getLazyExternalGeometryType(
        const ViewProviderSketch& viewProvider,
        int lazyExternalId
    );
    static bool isLazyExternalReferenceVertex(const ViewProviderSketch& viewProvider, int lazyExternalId);
    static bool getLazyExternalSourceReference(
        const ViewProviderSketch& viewProvider,
        int lazyExternalId,
        std::string& sourceObjectName,
        std::string& subName,
        bool& intersection,
        bool& vertex
    );
    static int materializeLazyExternalSourceReference(
        ViewProviderSketch& viewProvider,
        const std::string& sourceObjectName,
        const std::string& subName,
        bool intersection,
        bool defining
    );
    static bool selectLazyExternalReference(
        ViewProviderSketch& viewProvider,
        int lazyExternalId,
        bool toggle
    );
    static bool isLazyExternalReferenceSelected(
        const ViewProviderSketch& viewProvider,
        int lazyExternalId
    );
    static std::vector<int> getSelectedLazyExternalReferenceIds(const ViewProviderSketch& viewProvider);
    static void clearSelectedLazyExternalReferences(ViewProviderSketch& viewProvider);
    static void resetPreselectPoint(ViewProviderSketch& viewProvider);
};

int ViewProviderSketchLazySelectionAttorney::getPreselectLazyExternalId(
    const ViewProviderSketch& viewProvider
)
{
    return viewProvider.getPreselectLazyExternalId();
}

Base::Type ViewProviderSketchLazySelectionAttorney::getLazyExternalGeometryType(
    const ViewProviderSketch& viewProvider,
    int lazyExternalId
)
{
    return viewProvider.getLazyExternalGeometryType(lazyExternalId);
}

bool ViewProviderSketchLazySelectionAttorney::isLazyExternalReferenceVertex(
    const ViewProviderSketch& viewProvider,
    int lazyExternalId
)
{
    return viewProvider.isLazyExternalReferenceVertex(lazyExternalId);
}

bool ViewProviderSketchLazySelectionAttorney::getLazyExternalSourceReference(
    const ViewProviderSketch& viewProvider,
    int lazyExternalId,
    std::string& sourceObjectName,
    std::string& subName,
    bool& intersection,
    bool& vertex
)
{
    return viewProvider.getLazyExternalSourceReference(
        lazyExternalId,
        sourceObjectName,
        subName,
        intersection,
        vertex
    );
}

int ViewProviderSketchLazySelectionAttorney::materializeLazyExternalSourceReference(
    ViewProviderSketch& viewProvider,
    const std::string& sourceObjectName,
    const std::string& subName,
    bool intersection,
    bool defining
)
{
    return viewProvider
        .materializeLazyExternalSourceReference(sourceObjectName, subName, intersection, defining);
}

bool ViewProviderSketchLazySelectionAttorney::selectLazyExternalReference(
    ViewProviderSketch& viewProvider,
    int lazyExternalId,
    bool toggle
)
{
    return viewProvider.selectLazyExternalReference(lazyExternalId, toggle);
}

bool ViewProviderSketchLazySelectionAttorney::isLazyExternalReferenceSelected(
    const ViewProviderSketch& viewProvider,
    int lazyExternalId
)
{
    return viewProvider.isLazyExternalReferenceSelected(lazyExternalId);
}

std::vector<int> ViewProviderSketchLazySelectionAttorney::getSelectedLazyExternalReferenceIds(
    const ViewProviderSketch& viewProvider
)
{
    return viewProvider.getSelectedLazyExternalReferenceIds();
}

void ViewProviderSketchLazySelectionAttorney::clearSelectedLazyExternalReferences(
    ViewProviderSketch& viewProvider
)
{
    viewProvider.clearSelectedLazyExternalReferences();
}

void ViewProviderSketchLazySelectionAttorney::resetPreselectPoint(ViewProviderSketch& viewProvider)
{
    viewProvider.resetPreselectPoint();
}

namespace
{
constexpr const char* LazyExternalVertexSubelementPrefix = "LazyExternalVertex";
constexpr const char* LazyExternalEdgeSubelementPrefix = "LazyExternalEdge";

std::string makeExternalSubNameFromGeoId(int geoId)
{
    return "ExternalEdge" + std::to_string(Sketcher::GeoEnum::RefExt + 1 - geoId);
}

std::string makeLazyExternalSubelement(int lazyExternalId, bool lazyExternalVertex)
{
    return std::string(
               lazyExternalVertex ? LazyExternalVertexSubelementPrefix : LazyExternalEdgeSubelementPrefix
           )
        + std::to_string(lazyExternalId);
}

bool containsSelectionPair(const std::vector<SelIdPair>& items, const SelIdPair& item)
{
    return std::any_of(items.begin(), items.end(), [&](const SelIdPair& existing) {
        return selectionPairsEqual(existing, item);
    });
}

std::vector<int> collectLazyExternalIds(ViewProviderSketch* sketchgui, bool includePreselection)
{
    std::vector<int> lazyExternalIds;
    if (!sketchgui) {
        return lazyExternalIds;
    }

    lazyExternalIds = ViewProviderSketchLazySelectionAttorney::getSelectedLazyExternalReferenceIds(
        *sketchgui
    );

    if (includePreselection) {
        const int preselectLazyExternalId
            = ViewProviderSketchLazySelectionAttorney::getPreselectLazyExternalId(*sketchgui);
        if (preselectLazyExternalId >= 0
            && std::find(lazyExternalIds.begin(), lazyExternalIds.end(), preselectLazyExternalId)
                == lazyExternalIds.end()) {
            lazyExternalIds.push_back(preselectLazyExternalId);
        }
    }

    return lazyExternalIds;
}

bool isLazyExternalVertexReference(ViewProviderSketch* sketchgui, int lazyExternalId)
{
    return sketchgui
        && ViewProviderSketchLazySelectionAttorney::isLazyExternalReferenceVertex(
               *sketchgui,
               lazyExternalId
        );
}

template<typename Callback>
void forEachLazyExternalReference(
    ViewProviderSketch* sketchgui,
    bool includeEdges,
    bool includeVertices,
    bool includePreselection,
    Callback&& callback
)
{
    for (int lazyExternalId : collectLazyExternalIds(sketchgui, includePreselection)) {
        const bool lazyExternalVertex = isLazyExternalVertexReference(sketchgui, lazyExternalId);
        if ((lazyExternalVertex && includeVertices) || (!lazyExternalVertex && includeEdges)) {
            callback(lazyExternalId, lazyExternalVertex);
        }
    }
}

template<typename Callback>
void forEachPendingLazyExternalReference(
    ViewProviderSketch* sketchgui,
    bool includeEdges,
    bool includeVertices,
    Callback&& callback
)
{
    forEachLazyExternalReference(
        sketchgui,
        includeEdges,
        includeVertices,
        true,
        std::forward<Callback>(callback)
    );
}

void clearPendingSelection(ViewProviderSketch* sketchgui)
{
    if (!sketchgui) {
        return;
    }

    ViewProviderSketchLazySelectionAttorney::resetPreselectPoint(*sketchgui);
    ViewProviderSketchLazySelectionAttorney::clearSelectedLazyExternalReferences(*sketchgui);
}

bool makeSelectionPair(
    ViewProviderSketch* sketchgui,
    int lazyExternalId,
    bool lazyExternalVertex,
    SelIdPair& item,
    std::string* subName = nullptr,
    Base::Type* geoType = nullptr
)
{
    if (!sketchgui || lazyExternalId < 0) {
        return false;
    }

    std::string sourceObjectName;
    std::string sourceSubName;
    bool sourceIntersection = false;
    bool sourceVertex = false;
    if (!ViewProviderSketchLazySelectionAttorney::getLazyExternalSourceReference(
            *sketchgui,
            lazyExternalId,
            sourceObjectName,
            sourceSubName,
            sourceIntersection,
            sourceVertex
        )) {
        return false;
    }
    if (sourceVertex != lazyExternalVertex) {
        return false;
    }

    item.GeoId = Sketcher::GeoEnum::GeoUndef;
    item.PosId = sourceVertex ? Sketcher::PointPos::start : Sketcher::PointPos::none;
    item.LazyExternalId = lazyExternalId;
    item.IsLazyExternal = true;
    item.LazyExternalVertex = sourceVertex;
    item.LazyExternalSourceObjectName = std::move(sourceObjectName);
    item.LazyExternalSubelement = std::move(sourceSubName);
    item.LazyExternalIntersection = sourceIntersection;

    if (subName) {
        *subName = makeLazyExternalSubelement(lazyExternalId, sourceVertex);
    }
    if (geoType) {
        *geoType = sourceVertex
            ? Part::GeomPoint::getClassTypeId()
            : ViewProviderSketchLazySelectionAttorney::getLazyExternalGeometryType(
                  *sketchgui,
                  lazyExternalId
              );
    }

    return true;
}

bool materializeSelectionPair(
    ViewProviderSketch* sketchgui,
    SelIdPair& item,
    bool preserveLazyExternalIdentity
)
{
    if (!item.IsLazyExternal) {
        return true;
    }
    if (!sketchgui) {
        return false;
    }

    const int geoId = ViewProviderSketchLazySelectionAttorney::materializeLazyExternalSourceReference(
        *sketchgui,
        item.LazyExternalSourceObjectName,
        item.LazyExternalSubelement,
        item.LazyExternalIntersection,
        false
    );
    if (geoId == Sketcher::GeoEnum::GeoUndef) {
        return false;
    }

    const bool wasLazyVertex = item.LazyExternalVertex;
    item.GeoId = geoId;
    item.PosId = wasLazyVertex ? Sketcher::PointPos::start : Sketcher::PointPos::none;

    if (!preserveLazyExternalIdentity) {
        item.LazyExternalId = -1;
        item.IsLazyExternal = false;
        item.LazyExternalVertex = false;
        item.LazyExternalSourceObjectName.clear();
        item.LazyExternalSubelement.clear();
        item.LazyExternalIntersection = false;
    }

    return true;
}

bool materializeLazyExternalSelection(
    ViewProviderSketch* sketchgui,
    bool includeLazyExternalVertices,
    bool includePreselection
)
{
    if (!sketchgui || !sketchgui->getSketchObject()) {
        return true;
    }

    bool consumedLazyExternalSelection = false;
    bool materializationFailed = false;
    forEachLazyExternalReference(
        sketchgui,
        true,
        includeLazyExternalVertices,
        includePreselection,
        [&](int lazyExternalId, bool lazyExternalVertex) {
            consumedLazyExternalSelection = true;
            SelIdPair item;
            if (!makeSelectionPair(sketchgui, lazyExternalId, lazyExternalVertex, item)) {
                materializationFailed = true;
                return;
            }
            if (!materializeSelectionPair(sketchgui, item, false)) {
                materializationFailed = true;
                return;
            }

            const std::string subName = makeExternalSubNameFromGeoId(item.GeoId);
            if (!sketchgui->isSelected(subName)) {
                sketchgui->addSelection(subName);
            }
        }
    );

    if (consumedLazyExternalSelection) {
        clearPendingSelection(sketchgui);
    }
    return !materializationFailed;
}

}  // namespace

ActivatedLazySelection::ActivatedLazySelection(
    Gui::Command& command,
    Gui::Document* guiDocument,
    bool includeLazyExternalVertices,
    bool* sharedCommandActive
)
    : command(&command)
    , sharedCommandActive(sharedCommandActive)
{
    begin(guiDocument, includeLazyExternalVertices);
    selection = Gui::Command::getSelection().getSelectionEx();
}

ActivatedLazySelection::ActivatedLazySelection(ActivatedLazySelection&& other) noexcept
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

ActivatedLazySelection::~ActivatedLazySelection()
{
    if (abortOnDestroy) {
        abortLazyExternalCommand();
    }
}

const std::vector<Gui::SelectionObject>& ActivatedLazySelection::getSelection() const
{
    return selection;
}

void ActivatedLazySelection::openCommand(const char* name)
{
    if (isLazyExternalCommandActive()) {
        return;
    }

    command->openCommand(name);
}

void ActivatedLazySelection::commitCommand()
{
    command->commitCommand();
    setLazyExternalCommandActive(false);
    abortOnDestroy = false;
}

void ActivatedLazySelection::abortCommand()
{
    command->abortCommand();
    setLazyExternalCommandActive(false);
    abortOnDestroy = false;
}

void ActivatedLazySelection::begin(Gui::Document* guiDocument, bool includeLazyExternalVertices)
{
    auto* sketchgui = getActiveSketchGui(guiDocument);
    if (!LazyExternalSelectionResolver::hasPendingLazyExternalSelection(
            sketchgui,
            true,
            includeLazyExternalVertices
        )) {
        return;
    }

    command->openCommand(QT_TRANSLATE_NOOP("Command", "Add constraint"));
    setLazyExternalCommandActive(true);

    if (!LazyExternalSelectionResolver::materializePendingLazyExternalSelection(
            sketchgui,
            includeLazyExternalVertices
        )) {
        abortCommand();
    }
}

void ActivatedLazySelection::abortLazyExternalCommand()
{
    if (isLazyExternalCommandActive()) {
        abortCommand();
    }
}

bool ActivatedLazySelection::isLazyExternalCommandActive() const
{
    return sharedCommandActive ? *sharedCommandActive : localCommandActive;
}

void ActivatedLazySelection::setLazyExternalCommandActive(bool active)
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
        return lhs.IsLazyExternal == rhs.IsLazyExternal
            && lhs.LazyExternalVertex == rhs.LazyExternalVertex
            && lhs.LazyExternalSourceObjectName == rhs.LazyExternalSourceObjectName
            && lhs.LazyExternalSubelement == rhs.LazyExternalSubelement
            && lhs.LazyExternalIntersection == rhs.LazyExternalIntersection;
    }
    return lhs.GeoId == rhs.GeoId && lhs.PosId == rhs.PosId;
}

std::optional<LazyExternalSubelement> parseLazyExternalSubelement(const std::string& subName)
{
    const Data::IndexedName indexedSubName(subName.c_str());
    if (!indexedSubName || indexedSubName.getIndex() <= 0) {
        return std::nullopt;
    }

    if (std::strcmp(indexedSubName.getType(), LazyExternalVertexSubelementPrefix) == 0) {
        return LazyExternalSubelement {indexedSubName.getIndex(), true};
    }
    if (std::strcmp(indexedSubName.getType(), LazyExternalEdgeSubelementPrefix) == 0) {
        return LazyExternalSubelement {indexedSubName.getIndex(), false};
    }
    return std::nullopt;
}

bool LazyExternalSelectionResolver::hasPendingLazyExternalSelection(
    ViewProviderSketch* sketchgui,
    bool includeEdges,
    bool includeVertices
)
{
    bool hasPending = false;
    forEachPendingLazyExternalReference(sketchgui, includeEdges, includeVertices, [&](int, bool) {
        hasPending = true;
    });
    return hasPending;
}

bool LazyExternalSelectionResolver::materializePendingLazyExternalSelection(
    ViewProviderSketch* sketchgui,
    bool includeLazyExternalVertices
)
{
    return materializeLazyExternalSelection(sketchgui, includeLazyExternalVertices, true);
}

void LazyExternalSelectionResolver::appendPendingLazyExternalSelectionPairs(
    ViewProviderSketch* sketchgui,
    std::vector<SelIdPair>& points,
    std::vector<SelIdPair>& curves,
    bool includeEdges,
    bool includeVertices
)
{
    forEachPendingLazyExternalReference(
        sketchgui,
        includeEdges,
        includeVertices,
        [&](int lazyExternalId, bool lazyExternalVertex) {
            SelIdPair item;
            if (!makeSelectionPair(sketchgui, lazyExternalId, lazyExternalVertex, item)) {
                return;
            }

            auto& target = lazyExternalVertex ? points : curves;
            if (!containsSelectionPair(target, item)) {
                target.push_back(item);
            }
            setLazyExternalSelectionSelected(sketchgui, item, true);
        }
    );
}

void LazyExternalSelectionResolver::clearPendingLazyExternalSelection(ViewProviderSketch* sketchgui)
{
    clearPendingSelection(sketchgui);
}

bool LazyExternalSelectionResolver::makeLazyExternalSelectionPair(
    ViewProviderSketch* sketchgui,
    int lazyExternalId,
    bool lazyExternalVertex,
    SelIdPair& item,
    std::string* subName,
    Base::Type* geoType
)
{
    return makeSelectionPair(sketchgui, lazyExternalId, lazyExternalVertex, item, subName, geoType);
}

void LazyExternalSelectionResolver::setLazyExternalSelectionSelected(
    ViewProviderSketch* sketchgui,
    const SelIdPair& item,
    bool selected
)
{
    if (!sketchgui || !item.IsLazyExternal || item.LazyExternalId < 0) {
        return;
    }

    if (selected) {
        ViewProviderSketchLazySelectionAttorney::selectLazyExternalReference(
            *sketchgui,
            item.LazyExternalId,
            false
        );
    }
    else if (
        ViewProviderSketchLazySelectionAttorney::isLazyExternalReferenceSelected(
            *sketchgui,
            item.LazyExternalId
        )
    ) {
        ViewProviderSketchLazySelectionAttorney::selectLazyExternalReference(
            *sketchgui,
            item.LazyExternalId,
            true
        );
    }
}

bool LazyExternalSelectionResolver::materializeLazyExternalSelectionPairs(
    ViewProviderSketch* sketchgui,
    std::vector<SelIdPair>& items,
    bool preserveLazyExternalIdentity
)
{
    bool consumedLazyExternalSelection = false;
    for (auto& item : items) {
        consumedLazyExternalSelection = consumedLazyExternalSelection || item.IsLazyExternal;
        if (!materializeSelectionPair(sketchgui, item, preserveLazyExternalIdentity)) {
            if (consumedLazyExternalSelection) {
                clearPendingSelection(sketchgui);
            }
            return false;
        }
    }

    if (consumedLazyExternalSelection) {
        clearPendingSelection(sketchgui);
    }
    return true;
}

}  // namespace SketcherGui
