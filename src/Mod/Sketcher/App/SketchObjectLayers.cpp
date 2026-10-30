// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <charconv>
#include <limits>
#include <set>
#include <unordered_set>
#include <App/Document.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include "SketchObject.h"
#include "LayerDefaults.h"

using namespace Sketcher;

std::map<int, std::string> SketchObject::getLayers() const
{
    std::map<int, std::string> result;
    for (const auto& [key, name] : Layers.getValues()) {
        int id = -1;
        const auto parsed = std::from_chars(key.data(), key.data() + key.size(), id);
        if (parsed.ec == std::errc() && parsed.ptr == key.data() + key.size() && id >= 0) {
            result.emplace(id, name);
        }
    }
    result.try_emplace(0, "Default");
    return result;
}

bool SketchObject::hasLayer(int layerId) const
{
    return layerId >= 0 && getLayers().contains(layerId);
}

namespace
{
void validateLayerName(const std::string& name, const std::map<int, std::string>& layers, int id)
{
    if (name.empty() || name.find_first_not_of(" \t\r\n") == std::string::npos) {
        throw Base::ValueError("A layer name cannot be empty");
    }
    for (const auto& [otherId, otherName] : layers) {
        if (otherId != id && otherName == name) {
            throw Base::ValueError("A layer with this name already exists");
        }
    }
}
}  // namespace

int SketchObject::addLayer(const std::string& name)
{
    const auto layers = getLayers();
    validateLayerName(name, layers, -1);
    const long next = std::max(NextLayerId.getValue(), static_cast<long>(layers.rbegin()->first) + 1);
    if (next >= std::numeric_limits<int>::max()) {
        throw Base::ValueError("Too many sketch layers");
    }
    NextLayerId.setValue(next + 1);
    Layers.setValue(std::to_string(next), name);
    applyLayerDefaults(static_cast<int>(next));
    return static_cast<int>(next);
}

void SketchObject::applyLayerDefaults(int layerId)
{
    const auto defaults = layerDefaults();
    if (!defaults->GetBool("UseConstraints", true)) {
        auto unconstrained = UnconstrainedLayers.getValues();
        unconstrained.push_back(layerId);
        UnconstrainedLayers.setValues(std::move(unconstrained));
    }
}

void SketchObject::renameLayer(int layerId, const std::string& name)
{
    if (!hasLayer(layerId)) {
        throw Base::ValueError("Invalid layer ID");
    }
    validateLayerName(name, getLayers(), layerId);
    Layers.setValue(std::to_string(layerId), name);
}

void SketchObject::setActiveLayer(int layerId)
{
    if (!hasLayer(layerId)) {
        throw Base::ValueError("Invalid layer ID");
    }
    ActiveLayer.setValue(layerId);
}

int SketchObject::getGeometryLayer(int geoId) const
{
    if (geoId == GeoEnum::GeoUndef || (geoId >= 0 && geoId >= Geometry.getSize())
        || (geoId < 0
            && (geoId > GeoEnum::RefExt || -static_cast<long long>(geoId) > ExternalGeo.getSize()))) {
        throw Base::ValueError("Invalid geometry ID for layer assignment");
    }
    const int layer = getGeometryFacade(geoId)->getGeometryLayerId();
    return hasLayer(layer) ? layer : 0;
}

void SketchObject::initializeGeometryLayer(Part::Geometry* geometry) const
{
    auto facade = GeometryFacade::getFacade(geometry);
    // Setting construction/internal flags may already have created an extension.
    // Only a valid layer assignment, rather than the extension itself, is preserved.
    if (!hasLayer(facade->getGeometryLayerId())) {
        const int active = static_cast<int>(ActiveLayer.getValue());
        facade->setGeometryLayerId(hasLayer(active) ? active : 0);
    }
}

void SketchObject::setGeometryLayer(const std::vector<int>& geoIds, int layerId)
{
    if (!hasLayer(layerId)) {
        throw Base::ValueError("Invalid layer ID");
    }
    std::set<int> selection(geoIds.begin(), geoIds.end());
    for (int id : selection) {
        getGeometryLayer(id);  // Validate the whole selection before changing either property.
    }
    // A control point may lead to a grouped parent, whose other members can have
    // internal geometry of their own. Expand both relationships to a fixed point.
    bool expanded;
    do {
        const auto previousSize = selection.size();
        const auto currentSelection = selection;
        for (int id : currentSelection) {
            if (id >= 0) {
                const int handle = getGroupHandleIfInGroup(id);
                if (isGroupHandle(handle)) {
                    selection.insert(handle);
                    const auto members = getGroupGeometries(handle);
                    selection.insert(members.begin(), members.end());
                }
            }
        }
        for (const auto* constraint : Constraints.getValues()) {
            if (constraint->Type == InternalAlignment
                && (selection.contains(constraint->First) || selection.contains(constraint->Second))) {
                selection.insert(constraint->First);
                selection.insert(constraint->Second);
            }
        }
        expanded = selection.size() != previousSize;
    } while (expanded);

    if (isLayerLocked(layerId)) {
        throw Base::ValueError("Unlock the destination layer before moving geometry to it");
    }
    for (int id : selection) {
        checkGeometryUnlocked(id);
    }
    const auto previousExcluded = getUnconstrainedGeometry();
    Base::StateLocker lock(internaltransaction, true);
    for (auto* property : {&Geometry, &ExternalGeo}) {
        auto values = property->getValues();
        std::vector<std::unique_ptr<Part::Geometry>> copies;
        for (int id : selection) {
            if ((id >= 0) != (property == &Geometry)) {
                continue;
            }
            const int index = id >= 0 ? id : -id - 1;
            if (index < 0 || index >= static_cast<int>(values.size())) {
                continue;
            }
            if (GeometryFacade::getFacade(values[index])->getGeometryLayerId() == layerId) {
                continue;
            }
            copies.emplace_back(values[index]->clone());
            GeometryFacade::getFacade(copies.back().get())->setGeometryLayerId(layerId);
            values[index] = copies.back().get();
        }
        if (!copies.empty()) {
            property->setValues(values);
        }
    }
    // Refresh the solver's geometry metadata on its next use (no solve is needed here).
    solverNeedsUpdate = true;
    if (previousExcluded != getUnconstrainedGeometry()) {
        solve(false);
    }
    else {
        signalSolverUpdate();
    }
}

void SketchObject::removeLayer(int layerId)
{
    if (layerId == 0 || !hasLayer(layerId)) {
        throw Base::ValueError("Cannot remove the default layer or an invalid layer");
    }
    std::vector<int> members;
    for (int id = 0; id < Geometry.getSize(); ++id) {
        if (getGeometryLayer(id) == layerId) {
            members.push_back(id);
        }
    }
    for (int i = 2; i < ExternalGeo.getSize(); ++i) {
        if (getGeometryLayer(-i - 1) == layerId) {
            members.push_back(-i - 1);
        }
    }
    setGeometryLayer(members, 0);
    if (ActiveLayer.getValue() == layerId) {
        setActiveLayer(0);
    }
    auto layers = Layers.getValues();
    layers.erase(std::to_string(layerId));
    Layers.setValues(std::move(layers));
}

bool SketchObject::isLayerLocked(int layerId) const
{
    const auto& layers = LockedLayers.getValues();
    return std::find(layers.begin(), layers.end(), layerId) != layers.end();
}

bool SketchObject::layerUsesConstraints(int layerId) const
{
    const auto& layers = UnconstrainedLayers.getValues();
    return std::find(layers.begin(), layers.end(), layerId) == layers.end();
}

bool SketchObject::geometryUsesConstraints(int geoId) const
{
    if (geoId == GeoEnum::GeoUndef || geoId == GeoEnum::HAxis || geoId == GeoEnum::VAxis) {
        return true;
    }
    return layerUsesConstraints(getGeometryLayer(geoId));
}

bool SketchObject::constraintUsesLayers(const Constraint* constraint) const
{
    for (int i = 0; constraint->hasElement(i); ++i) {
        if (!geometryUsesConstraints(constraint->getGeoId(i))) {
            return false;
        }
    }
    return true;
}

void SketchObject::checkGeometryUnlocked(int geoId) const
{
    if (geoId == GeoEnum::GeoUndef || geoId == GeoEnum::HAxis || geoId == GeoEnum::VAxis) {
        return;
    }
    if (isLayerLocked(getGeometryLayer(geoId))) {
        throw Base::ValueError("Unlock the layer before modifying its geometry");
    }
}

std::set<int> SketchObject::getLockedGeometry() const
{
    std::set<int> result;
    for (int id = 0; id < Geometry.getSize(); ++id) {
        if (isLayerLocked(getGeometryLayer(id))) {
            result.insert(id);
        }
    }
    return result;
}

std::set<int> SketchObject::getUnconstrainedGeometry() const
{
    std::set<int> result;
    for (int id = -ExternalGeo.getSize(); id < Geometry.getSize(); ++id) {
        if (!geometryUsesConstraints(id)) {
            result.insert(id);
        }
    }
    return result;
}

void PropertyLayerGeometryList::validateValue(const std::vector<Part::Geometry*>& values) const
{
    const auto* sketch = dynamic_cast<const SketchObject*>(getContainer());
    if (!sketch || sketch->isRestoring() || !sketch->getDocument()
        || sketch->getDocument()->isPerformingTransaction() || sketch->LockedLayers.getSize() == 0) {
        return;
    }
    // External geometry follows its linked source on every rebuild; locking protects it
    // from the user, not from its link.
    const bool followsLinks = this == &sketch->ExternalGeo;
    // Match stable geometry IDs, since deleting another layer renumbers indices.
    std::map<long, const Part::Geometry*> candidates;
    for (const auto* geo : values) {
        candidates.emplace(GeometryFacade::getId(geo), geo);
    }
    for (const auto* geo : getValues()) {
        const auto old = GeometryFacade::getFacade(geo);
        if (!sketch->isLayerLocked(old->getGeometryLayerId())) {
            continue;
        }
        const auto found = candidates.find(old->getId());
        if (found == candidates.end()) {
            throw Base::ValueError("Cannot delete geometry on a locked layer");
        }
        const auto replacement = GeometryFacade::getFacade(found->second);
        if (old->getGeometryLayerId() != replacement->getGeometryLayerId()
            || old->getConstruction() != replacement->getConstruction()
            || old->getInternalType() != replacement->getInternalType()
            || (!followsLinks && !geo->isSame(*found->second, 1e-12, 1e-12))) {
            throw Base::ValueError("Cannot modify geometry on a locked layer");
        }
    }
    // One lookup per new geometry: a locked imported drawing can hold thousands.
    std::unordered_set<long> existing;
    existing.reserve(getValues().size());
    for (const auto* old : getValues()) {
        existing.insert(GeometryFacade::getId(old));
    }
    for (const auto* geo : values) {
        const auto facade = GeometryFacade::getFacade(geo);
        if (!existing.contains(facade->getId())
            && sketch->isLayerLocked(facade->getGeometryLayerId())) {
            throw Base::ValueError("Cannot add geometry to a locked layer");
        }
    }
}
