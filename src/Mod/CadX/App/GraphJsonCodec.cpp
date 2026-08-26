// SPDX-License-Identifier: LGPL-2.1-or-later

#include "GraphJsonCodec.h"

#include "GraphRevision.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include <cmath>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace CadX
{
namespace
{
GraphDecodeResult failure(const char* code, const std::string& diagnostic)
{
    return {nullptr, code, diagnostic};
}

QJsonArray placementJson(const Placement& placement)
{
    return {placement.x, placement.y, placement.z, placement.qx,
            placement.qy, placement.qz, placement.qw};
}

QJsonObject geometryJson(const GeometrySummary& geometry)
{
    return {
        {"available", geometry.available},
        {"valid", geometry.valid},
        {"kind", QString::fromStdString(geometry.kind)},
        {"solids", static_cast<qint64>(geometry.solids)},
        {"shells", static_cast<qint64>(geometry.shells)},
        {"faces", static_cast<qint64>(geometry.faces)},
        {"edges", static_cast<qint64>(geometry.edges)},
        {"vertices", static_cast<qint64>(geometry.vertices)},
        {"volume", geometry.volume},
        {"area", geometry.area},
        {"signature", QString::fromStdString(geometry.signature)}
    };
}

QJsonObject jointConnectorJson(const JointConnectorPayload& connector)
{
    // Offsets are part of the semantic connector identity.  Normalize a copy
    // so the evidence format is stable even when a caller populated a
    // mathematically equivalent quaternion with a different scale.
    auto offset = connector.offset;
    if (!offset.normalize()) {
        // GraphSnapshot::finalize currently validates node placements, not
        // payload placements.  Preserve an invalid value here so decode()
        // rejects the evidence instead of silently manufacturing a value.
        offset = connector.offset;
    }
    return {
        {"component", QString::fromStdString(connector.componentObject)},
        {"connector_type", QString::fromStdString(connector.connectorType)},
        {"connector", QString::fromStdString(connector.connector)},
        {"has_offset", connector.hasOffset},
        {"offset", placementJson(offset)}
    };
}

QJsonObject payloadJson(const NodePayload& payload)
{
    QJsonObject result;
    std::visit(
        [&result](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                result.insert("type", "none");
            }
            else if constexpr (std::is_same_v<T, DefinitionPayload>) {
                result = {
                    {"type", "definition"},
                    {"role", QString::fromStdString(value.role)},
                    {"container_kind", QString::fromStdString(value.containerKind)},
                    {"geometry_kind", QString::fromStdString(value.geometryKind)},
                    {"provenance_kind", QString::fromStdString(value.provenanceKind)},
                    {"semantic_part_kind", QString::fromStdString(value.semanticPartKind)}
                };
            }
            else if constexpr (std::is_same_v<T, OccurrencePayload>) {
                QJsonArray path;
                for (const auto& element : value.occurrencePath) {
                    path.push_back(QString::fromStdString(element));
                }
                result = {
                    {"type", "occurrence"},
                    {"occurrence_path", path},
                    {"rigid", value.rigid},
                    {"flexible", value.flexible},
                    {"geometry", geometryJson(value.geometry)}
                };
            }
            else if constexpr (std::is_same_v<T, RelationPayload>) {
                result = {
                    {"type", "relation"},
                    {"relation_type", QString::fromStdString(value.relationType)},
                    {"suppressed", value.suppressed}
                };
            }
            else if constexpr (std::is_same_v<T, ArtifactPayload>) {
                result = {
                    {"type", "artifact"},
                    {"artifact_type", QString::fromStdString(value.artifactType)}
                };
            }
            else if constexpr (std::is_same_v<T, PrimitivePayload>) {
                result = {
                    {"type", "primitive"},
                    {"primitive_kind", QString::fromStdString(value.primitiveKind)},
                    {"length", value.length},
                    {"width", value.width},
                    {"height", value.height},
                    {"radius", value.radius},
                    {"sweep_degrees", value.sweepDegrees}
                };
            }
            else if constexpr (std::is_same_v<T, JointPayload>) {
                result = {
                    {"type", "joint"},
                    {"joint_type", QString::fromStdString(value.jointType)},
                    {"reverse", value.reverse},
                    {"has_limits", value.hasLimits},
                    {"min_degrees", value.minDegrees},
                    {"max_degrees", value.maxDegrees},
                    {"first", jointConnectorJson(value.first)},
                    {"second", jointConnectorJson(value.second)}
                };
            }
            else if constexpr (std::is_same_v<T, GroundConstraintPayload>) {
                result = {
                    {"type", "ground_constraint"},
                    {"grounded", value.grounded},
                    {"constrained_object", QString::fromStdString(value.constrainedObject)},
                    {"grounded_joint_object", QString::fromStdString(value.groundedJointObject)}
                };
            }
            else {
                result = {
                    {"type", "unresolved"},
                    {"requested_document", QString::fromStdString(value.requestedDocument)},
                    {"requested_object", QString::fromStdString(value.requestedObject)},
                    {"diagnostic", QString::fromStdString(value.diagnostic)}
                };
            }
        },
        payload);
    return result;
}

QJsonObject nodeJson(const NodeRecord& node)
{
    QJsonArray placement = placementJson(node.localPlacement);
    return {
        {"id", QString::fromStdString(node.id)},
        {"kind", QString::fromLatin1(nodeKindName(node.kind))},
        {"native", QJsonObject {
             {"document_uid", QString::fromStdString(node.native.documentUid)},
             {"object_name", QString::fromStdString(node.native.objectName)},
             {"type_id", QString::fromStdString(node.native.typeId)}}},
        {"display", QJsonObject {
             {"label", QString::fromStdString(node.display.label)},
             {"normalized_label", QString::fromStdString(node.display.normalizedLabel)}}},
        {"provenance", QJsonObject {
             {"kind", QString::fromStdString(node.provenance.kind)},
             {"evidence", QString::fromStdString(node.provenance.evidence)}}},
        {"presentation", QJsonObject {
             {"visible", node.presentation.visible},
             {"selected", node.presentation.selected},
             {"view_id", QString::fromStdString(node.presentation.viewId)}}},
        {"local_placement", placement},
        {"world_placement", placementJson(node.worldPlacement)},
        {"geometry", geometryJson(node.geometry)},
        {"payload", payloadJson(node.payload)},
        {"unresolved", node.unresolved}
    };
}

QJsonObject edgeJson(const EdgeRecord& edge)
{
    return {
        {"id", QString::fromStdString(edge.id)},
        {"kind", QString::fromLatin1(edgeKindName(edge.kind))},
        {"from", QString::fromStdString(edge.from)},
        {"to", QString::fromStdString(edge.to)},
        {"provenance", QJsonObject {
             {"kind", QString::fromStdString(edge.provenance.kind)},
             {"evidence", QString::fromStdString(edge.provenance.evidence)}}},
        {"relation", QString::fromStdString(edge.relation)}
    };
}

bool stringValue(const QJsonObject& object,
                 const char* key,
                 std::string& output,
                 std::string& diagnostic,
                 bool required = true)
{
    const auto value = object.value(key);
    if (!value.isString()) {
        if (required) {
            diagnostic = std::string("missing string field '") + key + "'";
            return false;
        }
        return true;
    }
    output = value.toString().toStdString();
    return true;
}

bool placementValue(const QJsonValue& value, Placement& placement, std::string& diagnostic)
{
    const auto array = value.toArray();
    if (!value.isArray() || array.size() != 7) {
        diagnostic = "placement must contain seven values";
        return false;
    }
    double* values[] = {&placement.x, &placement.y, &placement.z, &placement.qx,
                        &placement.qy, &placement.qz, &placement.qw};
    for (int index = 0; index < 7; ++index) {
        if (!array[index].isDouble() || !std::isfinite(array[index].toDouble())) {
            diagnostic = "placement contains a non-finite value";
            return false;
        }
        *values[index] = array[index].toDouble();
    }
    return placement.normalize();
}

bool boolValue(const QJsonObject& object,
               const char* key,
               bool& output,
               std::string& diagnostic)
{
    const auto value = object.value(key);
    if (!value.isBool()) {
        diagnostic = std::string("missing boolean field '") + key + "'";
        return false;
    }
    output = value.toBool();
    return true;
}

bool finiteValue(const QJsonObject& object,
                 const char* key,
                 double& output,
                 std::string& diagnostic)
{
    const auto value = object.value(key);
    if (!value.isDouble() || !std::isfinite(value.toDouble())) {
        diagnostic = std::string("missing finite number field '") + key + "'";
        return false;
    }
    output = value.toDouble();
    return true;
}

bool jointConnectorValue(const QJsonValue& value,
                         JointConnectorPayload& connector,
                         std::string& diagnostic)
{
    if (!value.isObject()) {
        diagnostic = "joint connector must be an object";
        return false;
    }
    const auto object = value.toObject();
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (it.key() != "component" && it.key() != "connector_type"
            && it.key() != "connector" && it.key() != "has_offset" && it.key() != "offset") {
            diagnostic = "unknown joint connector field '" + it.key().toStdString() + "'";
            return false;
        }
    }
    if (!stringValue(object, "component", connector.componentObject, diagnostic)
        || !stringValue(object, "connector_type", connector.connectorType, diagnostic)
        || !stringValue(object, "connector", connector.connector, diagnostic)
        || !boolValue(object, "has_offset", connector.hasOffset, diagnostic)
        || !placementValue(object.value("offset"), connector.offset, diagnostic)) {
        return false;
    }
    if (connector.componentObject.empty() || connector.connectorType.empty()
        || connector.connector.empty()) {
        diagnostic = "joint connector string fields must not be empty";
        return false;
    }
    return true;
}

bool geometryValue(const QJsonValue& value,
                   GeometrySummary& geometry,
                   std::string& diagnostic)
{
    if (!value.isObject()) {
        diagnostic = "geometry must be an object";
        return false;
    }
    const auto object = value.toObject();
    geometry.available = object.value("available").toBool();
    geometry.valid = object.value("valid").toBool();
    geometry.kind = object.value("kind").toString().toStdString();
    geometry.solids = static_cast<std::uint64_t>(object.value("solids").toInteger());
    geometry.shells = static_cast<std::uint64_t>(object.value("shells").toInteger());
    geometry.faces = static_cast<std::uint64_t>(object.value("faces").toInteger());
    geometry.edges = static_cast<std::uint64_t>(object.value("edges").toInteger());
    geometry.vertices = static_cast<std::uint64_t>(object.value("vertices").toInteger());
    geometry.volume = object.value("volume").toDouble();
    geometry.area = object.value("area").toDouble();
    geometry.signature = object.value("signature").toString().toStdString();
    if (!std::isfinite(geometry.volume) || !std::isfinite(geometry.area)) {
        diagnostic = "geometry contains a non-finite measure";
        return false;
    }
    return true;
}

bool payloadValue(const QJsonValue& value, NodePayload& payload, std::string& diagnostic)
{
    if (!value.isObject()) {
        diagnostic = "payload must be an object";
        return false;
    }
    const auto object = value.toObject();
    const auto type = object.value("type").toString();
    if (type == "none") {
        payload = std::monostate {};
    }
    else if (type == "definition") {
        payload = DefinitionPayload {
            object.value("role").toString().toStdString(),
            object.value("container_kind").toString().toStdString(),
            object.value("geometry_kind").toString().toStdString(),
            object.value("provenance_kind").toString().toStdString(),
            object.value("semantic_part_kind").toString().toStdString()};
    }
    else if (type == "occurrence") {
        OccurrencePayload occurrence;
        for (const auto& element : object.value("occurrence_path").toArray()) {
            if (!element.isString()) {
                diagnostic = "occurrence path must contain strings";
                return false;
            }
            occurrence.occurrencePath.push_back(element.toString().toStdString());
        }
        occurrence.rigid = object.value("rigid").toBool();
        occurrence.flexible = object.value("flexible").toBool();
        if (!geometryValue(object.value("geometry"), occurrence.geometry, diagnostic)) {
            return false;
        }
        payload = std::move(occurrence);
    }
    else if (type == "relation") {
        payload = RelationPayload {
            object.value("relation_type").toString().toStdString(),
            object.value("suppressed").toBool()};
    }
    else if (type == "artifact") {
        payload = ArtifactPayload {object.value("artifact_type").toString().toStdString()};
    }
    else if (type == "primitive") {
        PrimitivePayload primitive;
        primitive.primitiveKind = object.value("primitive_kind").toString().toStdString();
        primitive.length = object.value("length").toDouble();
        primitive.width = object.value("width").toDouble();
        primitive.height = object.value("height").toDouble();
        primitive.radius = object.value("radius").toDouble();
        primitive.sweepDegrees = object.value("sweep_degrees").toDouble();
        if (!std::isfinite(primitive.length) || !std::isfinite(primitive.width)
            || !std::isfinite(primitive.height) || !std::isfinite(primitive.radius)
            || !std::isfinite(primitive.sweepDegrees)) {
            diagnostic = "primitive payload contains a non-finite dimension";
            return false;
        }
        payload = primitive;
    }
    else if (type == "joint") {
        JointPayload joint;
        for (auto it = object.begin(); it != object.end(); ++it) {
            if (it.key() != "type" && it.key() != "joint_type" && it.key() != "reverse"
                && it.key() != "has_limits" && it.key() != "min_degrees"
                && it.key() != "max_degrees" && it.key() != "first" && it.key() != "second") {
                diagnostic = "unknown joint payload field '" + it.key().toStdString() + "'";
                return false;
            }
        }
        if (!stringValue(object, "joint_type", joint.jointType, diagnostic)
            || !boolValue(object, "reverse", joint.reverse, diagnostic)
            || !boolValue(object, "has_limits", joint.hasLimits, diagnostic)
            || !finiteValue(object, "min_degrees", joint.minDegrees, diagnostic)
            || !finiteValue(object, "max_degrees", joint.maxDegrees, diagnostic)
            || !jointConnectorValue(object.value("first"), joint.first, diagnostic)
            || !jointConnectorValue(object.value("second"), joint.second, diagnostic)) {
            return false;
        }
        if (joint.jointType.empty()) {
            diagnostic = "joint_type must not be empty";
            return false;
        }
        if (!object.contains("first") || !object.contains("second")) {
            diagnostic = "joint payload requires both connector endpoints";
            return false;
        }
        if (!std::isfinite(joint.minDegrees) || !std::isfinite(joint.maxDegrees)) {
            diagnostic = "joint payload contains a non-finite limit";
            return false;
        }
        payload = joint;
    }
    else if (type == "ground_constraint") {
        payload = GroundConstraintPayload {
            object.value("grounded").toBool(),
            object.value("constrained_object").toString().toStdString(),
            object.value("grounded_joint_object").toString().toStdString()};
    }
    else if (type == "unresolved") {
        payload = UnresolvedPayload {
            object.value("requested_document").toString().toStdString(),
            object.value("requested_object").toString().toStdString(),
            object.value("diagnostic").toString().toStdString()};
    }
    else {
        diagnostic = "unsupported node payload type: " + type.toStdString();
        return false;
    }
    return true;
}

bool nodeValue(const QJsonValue& value, NodeRecord& node, std::string& diagnostic)
{
    if (!value.isObject()) {
        diagnostic = "node must be an object";
        return false;
    }
    const auto object = value.toObject();
    if (!stringValue(object, "id", node.id, diagnostic)) {
        return false;
    }
    NodeKind kind;
    if (!parseNodeKind(object.value("kind").toString().toStdString(), kind)) {
        diagnostic = "node has an unsupported kind";
        return false;
    }
    node.kind = kind;
    const auto native = object.value("native").toObject();
    if (!stringValue(native, "document_uid", node.native.documentUid, diagnostic)
        || !stringValue(native, "object_name", node.native.objectName, diagnostic)
        || !stringValue(native, "type_id", node.native.typeId, diagnostic)) {
        return false;
    }
    const auto display = object.value("display").toObject();
    if (!stringValue(display, "label", node.display.label, diagnostic)
        || !stringValue(display, "normalized_label", node.display.normalizedLabel, diagnostic)) {
        return false;
    }
    const auto provenance = object.value("provenance").toObject();
    if (!stringValue(provenance, "kind", node.provenance.kind, diagnostic)
        || !stringValue(provenance, "evidence", node.provenance.evidence, diagnostic)) {
        return false;
    }
    const auto presentation = object.value("presentation").toObject();
    node.presentation.visible = presentation.value("visible").toBool();
    node.presentation.selected = presentation.value("selected").toBool();
    if (!stringValue(presentation, "view_id", node.presentation.viewId, diagnostic)) {
        return false;
    }
    if (!placementValue(object.value("local_placement"), node.localPlacement, diagnostic)
        || !placementValue(object.value("world_placement"), node.worldPlacement, diagnostic)
        || !geometryValue(object.value("geometry"), node.geometry, diagnostic)
        || !payloadValue(object.value("payload"), node.payload, diagnostic)) {
        return false;
    }
    node.unresolved = object.value("unresolved").toBool();
    return true;
}

bool edgeValue(const QJsonValue& value, EdgeRecord& edge, std::string& diagnostic)
{
    if (!value.isObject()) {
        diagnostic = "edge must be an object";
        return false;
    }
    const auto object = value.toObject();
    if (!stringValue(object, "id", edge.id, diagnostic)
        || !stringValue(object, "from", edge.from, diagnostic)
        || !stringValue(object, "to", edge.to, diagnostic)
        || !stringValue(object, "relation", edge.relation, diagnostic)) {
        return false;
    }
    EdgeKind kind;
    if (!parseEdgeKind(object.value("kind").toString().toStdString(), kind)) {
        diagnostic = "edge has an unsupported kind";
        return false;
    }
    edge.kind = kind;
    const auto provenance = object.value("provenance").toObject();
    return stringValue(provenance, "kind", edge.provenance.kind, diagnostic)
        && stringValue(provenance, "evidence", edge.provenance.evidence, diagnostic);
}

QJsonObject headerJson(const GraphHeader& header)
{
    QJsonArray diagnostics;
    for (const auto& item : header.diagnostics) {
        diagnostics.push_back(QString::fromStdString(item));
    }
    return {
        {"schema_version", QString::fromStdString(header.schemaVersion)},
        {"graph_id", QString::fromStdString(header.graphId)},
        {"graph_revision", QString::fromStdString(header.graphRevision)},
        {"presentation_revision", QString::fromStdString(header.presentationRevision)},
        {"document_uid", QString::fromStdString(header.documentUid)},
        {"document_name", QString::fromStdString(header.documentName)},
        {"active_assembly_node_id", QString::fromStdString(header.activeAssemblyNodeId)},
        {"active_assembly_object_name", QString::fromStdString(header.activeAssemblyObjectName)},
        {"active_assembly_label", QString::fromStdString(header.activeAssemblyLabel)},
        {"active_view_id", QString::fromStdString(header.activeViewId)},
        {"camera_state", QString::fromStdString(header.cameraState)},
        {"complete", header.complete},
        {"stale", header.stale},
        {"diagnostics", diagnostics}
    };
}

}  // namespace

std::string GraphJsonCodec::encode(const GraphSnapshot& snapshot)
{
    QJsonArray nodes;
    for (const auto& node : snapshot.nodes()) {
        nodes.push_back(nodeJson(node));
    }
    QJsonArray edges;
    for (const auto& edge : snapshot.edges()) {
        edges.push_back(edgeJson(edge));
    }
    QJsonObject output {
        {"schema_version", schemaVersion},
        {"header", headerJson(snapshot.header())},
        {"nodes", nodes},
        {"edges", edges},
        {"canonical_semantic_hash", QString::fromStdString(
             sha256Revision(canonicalSemantic(snapshot)))},
        {"canonical_presentation_hash", QString::fromStdString(
             sha256Revision(canonicalPresentation(snapshot)))}
    };
    return QJsonDocument(output).toJson(QJsonDocument::Compact).toStdString();
}

GraphDecodeResult GraphJsonCodec::decode(const std::string& json)
{
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(QByteArray::fromStdString(json), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return failure("CADX_GRAPH_EVIDENCE_INVALID", "graph evidence is not a JSON object");
    }
    const auto root = document.object();
    if (root.value("schema_version").toString() != schemaVersion) {
        return failure("CADX_GRAPH_EVIDENCE_INVALID", "unsupported graph evidence schema");
    }
    const auto header = root.value("header").toObject();
    auto snapshot = std::make_shared<GraphSnapshot>();
    std::string diagnostic;
    if (!stringValue(header, "schema_version", snapshot->header().schemaVersion, diagnostic)
        || !stringValue(header, "graph_id", snapshot->header().graphId, diagnostic)
        || !stringValue(header, "graph_revision", snapshot->header().graphRevision, diagnostic)
        || !stringValue(header, "presentation_revision", snapshot->header().presentationRevision, diagnostic)
        || !stringValue(header, "document_uid", snapshot->header().documentUid, diagnostic)
        || !stringValue(header, "document_name", snapshot->header().documentName, diagnostic)
        || !stringValue(header, "active_assembly_node_id", snapshot->header().activeAssemblyNodeId, diagnostic)
        || !stringValue(header, "active_assembly_object_name", snapshot->header().activeAssemblyObjectName, diagnostic)
        || !stringValue(header, "active_assembly_label", snapshot->header().activeAssemblyLabel, diagnostic)
        || !stringValue(header, "active_view_id", snapshot->header().activeViewId, diagnostic)
        || !stringValue(header, "camera_state", snapshot->header().cameraState, diagnostic)) {
        return failure("CADX_GRAPH_EVIDENCE_INVALID", diagnostic);
    }
    snapshot->header().complete = header.value("complete").toBool();
    snapshot->header().stale = header.value("stale").toBool();
    for (const auto& item : header.value("diagnostics").toArray()) {
        if (!item.isString()) {
            return failure("CADX_GRAPH_EVIDENCE_INVALID", "header diagnostics must contain strings");
        }
        snapshot->header().diagnostics.push_back(item.toString().toStdString());
    }
    for (const auto& value : root.value("nodes").toArray()) {
        NodeRecord node;
        if (!nodeValue(value, node, diagnostic)) {
            return failure("CADX_GRAPH_EVIDENCE_INVALID", diagnostic);
        }
        snapshot->nodes().push_back(std::move(node));
    }
    for (const auto& value : root.value("edges").toArray()) {
        EdgeRecord edge;
        if (!edgeValue(value, edge, diagnostic)) {
            return failure("CADX_GRAPH_EVIDENCE_INVALID", diagnostic);
        }
        snapshot->edges().push_back(std::move(edge));
    }
    const auto expectedGraphRevision = snapshot->header().graphRevision;
    const auto expectedPresentationRevision = snapshot->header().presentationRevision;
    if (!snapshot->finalize(diagnostic)) {
        return failure("CADX_GRAPH_EVIDENCE_INVALID", diagnostic);
    }
    if (snapshot->header().graphRevision != expectedGraphRevision
        || snapshot->header().presentationRevision != expectedPresentationRevision) {
        return failure("CADX_GRAPH_EVIDENCE_MISMATCH",
                       "reconstructed graph revision differs from evidence");
    }
    const auto semanticHash = root.value("canonical_semantic_hash").toString().toStdString();
    const auto presentationHash = root.value("canonical_presentation_hash").toString().toStdString();
    if (semanticHash != sha256Revision(canonicalSemantic(*snapshot))
        || presentationHash != sha256Revision(canonicalPresentation(*snapshot))) {
        return failure("CADX_GRAPH_EVIDENCE_MISMATCH",
                       "canonical graph hash differs from evidence");
    }
    return {std::move(snapshot), {}, {}};
}

GraphDecodeResult GraphJsonCodec::roundTrip(const GraphSnapshot& snapshot)
{
    return decode(encode(snapshot));
}

}  // namespace CadX
