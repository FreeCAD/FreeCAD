// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace CadX
{

using NodeId = std::string;
using EdgeId = std::string;

enum class NodeKind
{
    Document,
    AssemblyDefinition,
    AssemblyOccurrence,
    PartDefinition,
    BodyDefinition,
    FeatureDefinition,
    Occurrence,
    OccurrenceGroup,
    Joint,
    JointConnector,
    RigidGroup,
    GroundConstraint,
    SemanticInterface,
    Datum,
    Material,
    BomIdentity,
    OrganizationalGroup,
    AssemblyArtifact,
    UnresolvedDefinition,
};

enum class EdgeKind
{
    Contains,
    HasDefinition,
    InstanceOf,
    OccursIn,
    HasBody,
    HasFeature,
    HasInterface,
    HasDatum,
    HasMaterial,
    HasBomIdentity,
    SourceObject,
    SourceDocument,
    NestedOccurrence,
    ExpandsTo,
    HasJoint,
    JointEndpoint,
    ReferencesInterface,
    ReferencesTopology,
    GroundedBy,
    MemberOfRigidGroup,
    DependsOn,
    VisibleIn,
    SelectedIn,
    HasArtifact,
    UnresolvedSource,
};

const char* nodeKindName(NodeKind kind) noexcept;
const char* edgeKindName(EdgeKind kind) noexcept;
bool parseNodeKind(const std::string& value, NodeKind& kind);
bool parseEdgeKind(const std::string& value, EdgeKind& kind);

struct NativeIdentity
{
    std::string documentUid;
    std::string objectName;
    std::string typeId;

    bool empty() const noexcept
    {
        return documentUid.empty() || objectName.empty() || typeId.empty();
    }

    std::string canonical() const;
};

struct DisplayIdentity
{
    std::string label;
    std::string normalizedLabel;
};

struct Provenance
{
    std::string kind = "unknown";
    std::string evidence;
};

struct Presentation
{
    bool visible = false;
    bool selected = false;
    std::string viewId;
};

struct Placement
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double qx = 0.0;
    double qy = 0.0;
    double qz = 0.0;
    double qw = 1.0;

    bool normalize();
    std::string canonical() const;
};

struct GeometrySummary
{
    bool available = false;
    bool valid = false;
    std::string kind = "unavailable";
    std::uint64_t solids = 0;
    std::uint64_t shells = 0;
    std::uint64_t faces = 0;
    std::uint64_t edges = 0;
    std::uint64_t vertices = 0;
    double volume = 0.0;
    double area = 0.0;
    std::string signature;
};

// Payload variants keep the graph typed without copying arbitrary FreeCAD
// properties into an unbounded map.
struct DefinitionPayload
{
    std::string role;
    std::string containerKind;
    std::string geometryKind;
    std::string provenanceKind;
    std::string semanticPartKind = "unknown";
};

struct OccurrencePayload
{
    std::vector<std::string> occurrencePath;
    bool rigid = false;
    bool flexible = false;
    GeometrySummary geometry;
};

struct RelationPayload
{
    std::string relationType;
    bool suppressed = false;
};

struct ArtifactPayload
{
    std::string artifactType;
};

struct PrimitivePayload
{
    std::string primitiveKind;
    double length = 0.0;
    double width = 0.0;
    double height = 0.0;
    double radius = 0.0;
    double sweepDegrees = 360.0;
};

struct JointConnectorPayload
{
    // The component name is the occurrence object referenced by FreeCAD's
    // PropertyXLinkSub.  The connector fields retain the request semantics;
    // FreeCAD itself does not persist connector_type as a native property.
    std::string componentObject;
    std::string connectorType;
    std::string connector;
    Placement offset;
    bool hasOffset = false;
};

struct JointPayload
{
    std::string jointType;
    bool reverse = false;
    bool hasLimits = false;
    double minDegrees = 0.0;
    double maxDegrees = 0.0;
    JointConnectorPayload first;
    JointConnectorPayload second;
};

struct GroundConstraintPayload
{
    bool grounded = false;
    std::string constrainedObject;
    std::string groundedJointObject;
};

struct UnresolvedPayload
{
    std::string requestedDocument;
    std::string requestedObject;
    std::string diagnostic;
};

using NodePayload = std::variant<std::monostate,
                                 DefinitionPayload,
                                 OccurrencePayload,
                                 RelationPayload,
                                 ArtifactPayload,
                                 PrimitivePayload,
                                 JointPayload,
                                 GroundConstraintPayload,
                                 UnresolvedPayload>;

struct NodeRecord
{
    NodeId id;
    NodeKind kind = NodeKind::UnresolvedDefinition;
    NativeIdentity native;
    DisplayIdentity display;
    Provenance provenance;
    Presentation presentation;
    Placement localPlacement;
    Placement worldPlacement;
    GeometrySummary geometry;
    NodePayload payload;
    bool unresolved = false;
};

struct EdgeRecord
{
    EdgeId id;
    EdgeKind kind = EdgeKind::Contains;
    NodeId from;
    NodeId to;
    Provenance provenance;
    std::string relation;
};

}  // namespace CadX
