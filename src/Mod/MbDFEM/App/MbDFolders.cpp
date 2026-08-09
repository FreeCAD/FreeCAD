// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDFolders.h"

#include <App/Document.h>

#include "MbDAction.h"
#include "MbDAssembly.h"
#include "MbDGroupUtils.h"
#include "MbDJoint.h"
#include "MbDMarker.h"
#include "MbDMotion.h"
#include "MbDPart.h"

PROPERTY_SOURCE(MbDFEM::MbDAssembliesFolder, App::DocumentObjectGroup)
PROPERTY_SOURCE(MbDFEM::MbDPartsFolder, App::DocumentObjectGroup)
PROPERTY_SOURCE(MbDFEM::MbDFixedPartsFolder, App::DocumentObjectGroup)
PROPERTY_SOURCE(MbDFEM::MbDMarkersFolder, App::DocumentObjectGroup)
PROPERTY_SOURCE(MbDFEM::MbDJointsFolder, App::DocumentObjectGroup)
PROPERTY_SOURCE(MbDFEM::MbDMotionsFolder, App::DocumentObjectGroup)
PROPERTY_SOURCE(MbDFEM::MbDActionsFolder, App::DocumentObjectGroup)

namespace
{

bool omitFolderFromSubName(std::ostringstream&,
                           App::DocumentObject* topParent,
                           App::DocumentObject* child)
{
    return topParent && child;
}

MbDFEM::MbDAssembly* owningAssembly(App::DocumentObjectGroup* folder)
{
    if (!folder) {
        return nullptr;
    }

    auto* document = folder->getDocument();
    if (!document) {
        return nullptr;
    }

    for (auto* assembly : document->getObjectsOfType<MbDFEM::MbDAssembly>()) {
        if (assembly->getPartsFolder() == folder || assembly->getFixedPartsFolder() == folder) {
            return assembly;
        }
    }
    return nullptr;
}

void synchronizeOwningAssembly(App::DocumentObjectGroup* folder, const App::Property* prop)
{
    if (!folder || prop != &folder->Group || folder->Group.testStatus(App::Property::User3)) {
        return;
    }
    if (auto* assembly = owningAssembly(folder)) {
        assembly->synchronizePartCategories();
    }
}

}  // namespace

bool MbDFEM::MbDAssembliesFolder::allowObject(App::DocumentObject* object)
{
    return object && object->isDerivedFrom<MbDFEM::MbDAssembly>();
}

bool MbDFEM::MbDAssembliesFolder::redirectSubName(std::ostringstream& ss,
                                                  App::DocumentObject* topParent,
                                                  App::DocumentObject* child) const
{
    return omitFolderFromSubName(ss, topParent, child);
}

bool MbDFEM::MbDPartsFolder::allowObject(App::DocumentObject* object)
{
    if (!object || !object->isDerivedFrom<MbDFEM::MbDPart>()) {
        return false;
    }
    if (auto* assembly = owningAssembly(this)) {
        removeAll(assembly->fixedparts, object);
        appendUnique(assembly->parts, object);
    }
    return true;
}

std::vector<App::DocumentObject*> MbDFEM::MbDPartsFolder::addObject(App::DocumentObject* object)
{
    auto added = App::GroupExtension::addObject(object);
    if (auto* assembly = owningAssembly(this)) {
        assembly->synchronizePartCategories();
    }
    return added;
}

std::vector<App::DocumentObject*> MbDFEM::MbDPartsFolder::removeObject(App::DocumentObject* object)
{
    auto removed = App::GroupExtension::removeObject(object);
    if (auto* assembly = owningAssembly(this)) {
        assembly->synchronizePartCategories();
    }
    return removed;
}

bool MbDFEM::MbDPartsFolder::redirectSubName(std::ostringstream& ss,
                                             App::DocumentObject* topParent,
                                             App::DocumentObject* child) const
{
    return omitFolderFromSubName(ss, topParent, child);
}

void MbDFEM::MbDPartsFolder::onChanged(const App::Property* prop)
{
    App::DocumentObjectGroup::onChanged(prop);
    synchronizeOwningAssembly(this, prop);
}

bool MbDFEM::MbDFixedPartsFolder::allowObject(App::DocumentObject* object)
{
    if (!object || !object->isDerivedFrom<MbDFEM::MbDPart>()) {
        return false;
    }
    if (auto* assembly = owningAssembly(this)) {
        removeAll(assembly->parts, object);
        appendUnique(assembly->fixedparts, object);
    }
    return true;
}

std::vector<App::DocumentObject*> MbDFEM::MbDFixedPartsFolder::addObject(
    App::DocumentObject* object)
{
    auto added = App::GroupExtension::addObject(object);
    if (auto* assembly = owningAssembly(this)) {
        assembly->synchronizePartCategories();
    }
    return added;
}

std::vector<App::DocumentObject*> MbDFEM::MbDFixedPartsFolder::removeObject(
    App::DocumentObject* object)
{
    auto removed = App::GroupExtension::removeObject(object);
    if (auto* assembly = owningAssembly(this)) {
        assembly->synchronizePartCategories();
    }
    return removed;
}

bool MbDFEM::MbDFixedPartsFolder::redirectSubName(std::ostringstream& ss,
                                                  App::DocumentObject* topParent,
                                                  App::DocumentObject* child) const
{
    return omitFolderFromSubName(ss, topParent, child);
}

void MbDFEM::MbDFixedPartsFolder::onChanged(const App::Property* prop)
{
    App::DocumentObjectGroup::onChanged(prop);
    synchronizeOwningAssembly(this, prop);
}

bool MbDFEM::MbDMarkersFolder::allowObject(App::DocumentObject* object)
{
    return object && object->isDerivedFrom<MbDFEM::MbDMarker>();
}

bool MbDFEM::MbDMarkersFolder::redirectSubName(std::ostringstream& ss,
                                               App::DocumentObject* topParent,
                                               App::DocumentObject* child) const
{
    return omitFolderFromSubName(ss, topParent, child);
}

bool MbDFEM::MbDJointsFolder::allowObject(App::DocumentObject* object)
{
    return object && object->isDerivedFrom<MbDFEM::MbDJoint>();
}

bool MbDFEM::MbDJointsFolder::redirectSubName(std::ostringstream& ss,
                                              App::DocumentObject* topParent,
                                              App::DocumentObject* child) const
{
    return omitFolderFromSubName(ss, topParent, child);
}

bool MbDFEM::MbDMotionsFolder::allowObject(App::DocumentObject* object)
{
    return object && object->isDerivedFrom<MbDFEM::MbDMotion>();
}

bool MbDFEM::MbDMotionsFolder::redirectSubName(std::ostringstream& ss,
                                               App::DocumentObject* topParent,
                                               App::DocumentObject* child) const
{
    return omitFolderFromSubName(ss, topParent, child);
}

bool MbDFEM::MbDActionsFolder::allowObject(App::DocumentObject* object)
{
    return object && object->isDerivedFrom<MbDFEM::MbDAction>();
}

bool MbDFEM::MbDActionsFolder::redirectSubName(std::ostringstream& ss,
                                               App::DocumentObject* topParent,
                                               App::DocumentObject* child) const
{
    return omitFolderFromSubName(ss, topParent, child);
}
