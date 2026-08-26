// SPDX-License-Identifier: LGPL-2.1-or-later

#include "AssemblyObjectAdapter.h"

namespace CadX
{

AdapterClassification AssemblyObjectAdapter::classify(const std::string& typeId)
{
    // Most-specific forms are checked before their FreeCAD base classes.
    if (typeId == "Assembly::AssemblyObject") {
        return {NodeKind::AssemblyDefinition, "definition", "assembly", "compound", "local_parametric", {}};
    }
    if (typeId == "Assembly::AssemblyLink") {
        return {NodeKind::Occurrence, "occurrence", "assembly", "compound", "external_link", {}};
    }
    if (typeId == "Assembly::JointGroup") {
        return {NodeKind::AssemblyArtifact,
                "artifact",
                "joint_group",
                "empty",
                "local_parametric",
                "joint group is a non-physical Assembly container"};
    }
    if (typeId == "App::Link") {
        return {NodeKind::Occurrence, "occurrence", "link_group", "unavailable", "external_link", {}};
    }
    if (typeId == "PartDesign::Body") {
        return {NodeKind::BodyDefinition, "definition", "body", "solid", "local_parametric", {}};
    }
    if (typeId == "Part::Feature" || typeId.rfind("Part::", 0) == 0) {
        return {NodeKind::FeatureDefinition, "definition", "none", "solid", "local_direct_shape", {}};
    }
    if (typeId == "App::Part") {
        return {NodeKind::PartDefinition, "definition", "part", "compound", "local_parametric", {}};
    }
    if (typeId == "App::DocumentObjectGroup") {
        return {NodeKind::OrganizationalGroup, "organization", "group", "empty", "unknown", {}};
    }
    AdapterClassification fallback;
    fallback.diagnostic = "unrecognized object preserved as an unresolved definition";
    return fallback;
}

}  // namespace CadX
