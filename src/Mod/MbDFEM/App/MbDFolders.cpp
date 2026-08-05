// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDFolders.h"

#include "MbDAction.h"
#include "MbDAssembly.h"
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
    return object && object->isDerivedFrom<MbDFEM::MbDPart>();
}

bool MbDFEM::MbDPartsFolder::redirectSubName(std::ostringstream& ss,
                                             App::DocumentObject* topParent,
                                             App::DocumentObject* child) const
{
    return omitFolderFromSubName(ss, topParent, child);
}

bool MbDFEM::MbDFixedPartsFolder::allowObject(App::DocumentObject* object)
{
    return object && object->isDerivedFrom<MbDFEM::MbDPart>();
}

bool MbDFEM::MbDFixedPartsFolder::redirectSubName(std::ostringstream& ss,
                                                  App::DocumentObject* topParent,
                                                  App::DocumentObject* child) const
{
    return omitFolderFromSubName(ss, topParent, child);
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
