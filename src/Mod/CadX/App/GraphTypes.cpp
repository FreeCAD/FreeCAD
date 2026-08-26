// SPDX-License-Identifier: LGPL-2.1-or-later

#include "GraphTypes.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace CadX
{
namespace
{
template<typename T>
std::string number(T value)
{
    if (value == 0) {
        value = 0;
    }
    std::ostringstream stream;
    stream << std::setprecision(17) << value;
    return stream.str();
}

}  // namespace

const char* nodeKindName(NodeKind kind) noexcept
{
    switch (kind) {
        case NodeKind::Document: return "Document";
        case NodeKind::AssemblyDefinition: return "AssemblyDefinition";
        case NodeKind::AssemblyOccurrence: return "AssemblyOccurrence";
        case NodeKind::PartDefinition: return "PartDefinition";
        case NodeKind::BodyDefinition: return "BodyDefinition";
        case NodeKind::FeatureDefinition: return "FeatureDefinition";
        case NodeKind::Occurrence: return "Occurrence";
        case NodeKind::OccurrenceGroup: return "OccurrenceGroup";
        case NodeKind::Joint: return "Joint";
        case NodeKind::JointConnector: return "JointConnector";
        case NodeKind::RigidGroup: return "RigidGroup";
        case NodeKind::GroundConstraint: return "GroundConstraint";
        case NodeKind::SemanticInterface: return "SemanticInterface";
        case NodeKind::Datum: return "Datum";
        case NodeKind::Material: return "Material";
        case NodeKind::BomIdentity: return "BomIdentity";
        case NodeKind::OrganizationalGroup: return "OrganizationalGroup";
        case NodeKind::AssemblyArtifact: return "AssemblyArtifact";
        case NodeKind::UnresolvedDefinition: return "UnresolvedDefinition";
    }
    return "UnresolvedDefinition";
}

const char* edgeKindName(EdgeKind kind) noexcept
{
    switch (kind) {
        case EdgeKind::Contains: return "CONTAINS";
        case EdgeKind::HasDefinition: return "HAS_DEFINITION";
        case EdgeKind::InstanceOf: return "INSTANCE_OF";
        case EdgeKind::OccursIn: return "OCCURS_IN";
        case EdgeKind::HasBody: return "HAS_BODY";
        case EdgeKind::HasFeature: return "HAS_FEATURE";
        case EdgeKind::HasInterface: return "HAS_INTERFACE";
        case EdgeKind::HasDatum: return "HAS_DATUM";
        case EdgeKind::HasMaterial: return "HAS_MATERIAL";
        case EdgeKind::HasBomIdentity: return "HAS_BOM_IDENTITY";
        case EdgeKind::SourceObject: return "SOURCE_OBJECT";
        case EdgeKind::SourceDocument: return "SOURCE_DOCUMENT";
        case EdgeKind::NestedOccurrence: return "NESTED_OCCURRENCE";
        case EdgeKind::ExpandsTo: return "EXPANDS_TO";
        case EdgeKind::HasJoint: return "HAS_JOINT";
        case EdgeKind::JointEndpoint: return "JOINT_ENDPOINT";
        case EdgeKind::ReferencesInterface: return "REFERENCES_INTERFACE";
        case EdgeKind::ReferencesTopology: return "REFERENCES_TOPOLOGY";
        case EdgeKind::GroundedBy: return "GROUNDED_BY";
        case EdgeKind::MemberOfRigidGroup: return "MEMBER_OF_RIGID_GROUP";
        case EdgeKind::DependsOn: return "DEPENDS_ON";
        case EdgeKind::VisibleIn: return "VISIBLE_IN";
        case EdgeKind::SelectedIn: return "SELECTED_IN";
        case EdgeKind::HasArtifact: return "HAS_ARTIFACT";
        case EdgeKind::UnresolvedSource: return "UNRESOLVED_SOURCE";
    }
    return "CONTAINS";
}

bool parseNodeKind(const std::string& value, NodeKind& kind)
{
    for (int index = 0; index <= static_cast<int>(NodeKind::UnresolvedDefinition); ++index) {
        auto candidate = static_cast<NodeKind>(index);
        if (value == nodeKindName(candidate)) {
            kind = candidate;
            return true;
        }
    }
    return false;
}

bool parseEdgeKind(const std::string& value, EdgeKind& kind)
{
    for (int index = 0; index <= static_cast<int>(EdgeKind::UnresolvedSource); ++index) {
        auto candidate = static_cast<EdgeKind>(index);
        if (value == edgeKindName(candidate)) {
            kind = candidate;
            return true;
        }
    }
    return false;
}

std::string NativeIdentity::canonical() const
{
    return documentUid + "\x1f" + objectName + "\x1f" + typeId;
}

bool Placement::normalize()
{
    const double length = std::sqrt(qx * qx + qy * qy + qz * qz + qw * qw);
    if (!std::isfinite(length) || length <= std::numeric_limits<double>::epsilon()) {
        return false;
    }
    qx /= length;
    qy /= length;
    qz /= length;
    qw /= length;
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

std::string Placement::canonical() const
{
    return number(x) + "," + number(y) + "," + number(z) + "," + number(qx) + ","
        + number(qy) + "," + number(qz) + "," + number(qw);
}

}  // namespace CadX
