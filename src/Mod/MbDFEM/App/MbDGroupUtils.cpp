// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDGroupUtils.h"

#include <algorithm>

#include <App/DocumentObjectGroup.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/GroupExtension.h>

#include "MbDAssembly.h"
#include "MbDPart.h"

namespace
{

void addToGeoGroup(App::DocumentObject* owner, App::DocumentObject* child)
{
    if (!owner || !child) {
        return;
    }

    auto* group = owner->getExtensionByType<App::GroupExtension>();
    if (group) {
        MbDFEM::appendUnique(group->Group, child);
    }
}

void removeFromGeoGroup(App::DocumentObject* owner, App::DocumentObject* child)
{
    if (!owner || !child) {
        return;
    }

    auto* group = owner->getExtensionByType<App::GroupExtension>();
    if (group) {
        MbDFEM::removeAll(group->Group, child);
    }
}

void removeFromFolder(App::DocumentObjectGroup* folder, App::DocumentObject* child)
{
    if (folder && child && folder->hasObject(child)) {
        folder->removeObject(child);
    }
}

void removeFromAssembly(MbDFEM::MbDAssembly* assembly,
                        App::DocumentObject* child,
                        bool removeGeoGroup)
{
    if (!assembly || !child) {
        return;
    }

    assembly->removeFromCategories(child);

    if (removeGeoGroup) {
        removeFromGeoGroup(assembly, child);
    }
}

void removeFromPart(MbDFEM::MbDPart* part, App::DocumentObject* child, bool removeGeoGroup)
{
    if (!part || !child) {
        return;
    }

    MbDFEM::removeAll(part->markers, child);
    if (part->massMarker.getValue() == child) {
        part->massMarker.setValue(nullptr);
    }
    removeFromFolder(part->getMarkersFolder(), child);

    if (removeGeoGroup) {
        removeFromGeoGroup(part, child);
    }
}

}  // namespace

bool MbDFEM::appendUnique(App::PropertyLinkList& list, App::DocumentObject* object)
{
    if (!object) {
        return false;
    }

    auto values = list.getValues();
    if (std::find(values.begin(), values.end(), object) != values.end()) {
        return false;
    }

    values.push_back(object);
    list.setValues(values);
    return true;
}

bool MbDFEM::removeAll(App::PropertyLinkList& list, App::DocumentObject* object)
{
    if (!object) {
        return false;
    }

    auto values = list.getValues();
    const auto originalSize = values.size();
    values.erase(std::remove(values.begin(), values.end(), object), values.end());
    if (values.size() == originalSize) {
        return false;
    }

    list.setValues(values);
    return true;
}

void MbDFEM::addChildToListFolderAndGeoGroup(App::DocumentObject* owner,
                                             App::PropertyLinkList& list,
                                             App::DocumentObjectGroup* folder,
                                             App::DocumentObject* child)
{
    if (!owner || !child) {
        return;
    }

    removeChildFromMbDFEMSemanticOwners(child, owner);
    appendUnique(list, child);

    if (folder && !folder->hasObject(child)) {
        folder->addObject(child);
    }

    addToGeoGroup(owner, child);
}

void MbDFEM::removeChildFromListFolderAndGeoGroup(App::DocumentObject* owner,
                                                  App::PropertyLinkList& list,
                                                  App::DocumentObjectGroup* folder,
                                                  App::DocumentObject* child)
{
    removeAll(list, child);
    removeFromFolder(folder, child);

    const auto* assembly = freecad_cast<const MbDFEM::MbDAssembly*>(owner);
    if (assembly && child) {
        const auto parts = assembly->parts.getValues();
        const auto fixedparts = assembly->fixedparts.getValues();
        const bool retainedAsPart =
            std::find(parts.begin(), parts.end(), child) != parts.end()
            || std::find(fixedparts.begin(), fixedparts.end(), child) != fixedparts.end();
        if (retainedAsPart) {
            return;
        }
    }

    removeFromGeoGroup(owner, child);
}

void MbDFEM::removeChildFromMbDFEMSemanticOwners(App::DocumentObject* child,
                                                 App::DocumentObject* exceptOwner)
{
    if (!child) {
        return;
    }

    auto* previousOwner = App::GeoFeatureGroupExtension::getGroupOfObject(child);
    if (!previousOwner || previousOwner == exceptOwner) {
        return;
    }

    if (auto* assembly = freecad_cast<MbDFEM::MbDAssembly*>(previousOwner)) {
        removeFromAssembly(assembly, child, true);
        return;
    }
    if (auto* part = freecad_cast<MbDFEM::MbDPart*>(previousOwner)) {
        removeFromPart(part, child, true);
        return;
    }

    removeFromGeoGroup(previousOwner, child);
}
