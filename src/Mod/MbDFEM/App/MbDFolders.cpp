// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDFolders.h"

#include "MbDAction.h"
#include "MbDJoint.h"
#include "MbDMarker.h"
#include "MbDMotion.h"
#include "MbDPart.h"

PROPERTY_SOURCE(MbDFEM::MbDPartsFolder, App::DocumentObjectGroup)
PROPERTY_SOURCE(MbDFEM::MbDMarkersFolder, App::DocumentObjectGroup)
PROPERTY_SOURCE(MbDFEM::MbDJointsFolder, App::DocumentObjectGroup)
PROPERTY_SOURCE(MbDFEM::MbDMotionsFolder, App::DocumentObjectGroup)
PROPERTY_SOURCE(MbDFEM::MbDActionsFolder, App::DocumentObjectGroup)

bool MbDFEM::MbDPartsFolder::allowObject(App::DocumentObject* object)
{
    return object && object->isDerivedFrom<MbDFEM::MbDPart>();
}

bool MbDFEM::MbDMarkersFolder::allowObject(App::DocumentObject* object)
{
    return object && object->isDerivedFrom<MbDFEM::MbDMarker>();
}

bool MbDFEM::MbDJointsFolder::allowObject(App::DocumentObject* object)
{
    return object && object->isDerivedFrom<MbDFEM::MbDJoint>();
}

bool MbDFEM::MbDMotionsFolder::allowObject(App::DocumentObject* object)
{
    return object && object->isDerivedFrom<MbDFEM::MbDMotion>();
}

bool MbDFEM::MbDActionsFolder::allowObject(App::DocumentObject* object)
{
    return object && object->isDerivedFrom<MbDFEM::MbDAction>();
}
