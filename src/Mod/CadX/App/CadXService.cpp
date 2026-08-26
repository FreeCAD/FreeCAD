// SPDX-License-Identifier: LGPL-2.1-or-later

#include "CadXService.h"
#include "GraphQuery.h"
#include "GraphJsonCodec.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <algorithm>
#include <cctype>

namespace CadX
{
namespace
{
constexpr const char* kSnapshotSchema =
    R"({"type":"object","properties":{"geometry_detail":{"type":"string","enum":["none","summary"]},"include_view_state":{"type":"boolean"},"refresh":{"type":"string","enum":["if_stale","always"]}},"additionalProperties":false})";
constexpr const char* kQuerySchema =
    R"({"oneOf":[
      {"type":"object","properties":{"graph_id":{"type":"string"},"graph_revision":{"type":"string"},"operation":{"type":"string","enum":["summary"]},"limit":{"type":"integer","minimum":1,"maximum":100},"cursor":{"type":"string"}},"required":["graph_id","graph_revision","operation"],"additionalProperties":false},
      {"type":"object","properties":{"graph_id":{"type":"string"},"graph_revision":{"type":"string"},"operation":{"type":"string","enum":["find_nodes"]},"limit":{"type":"integer","minimum":1,"maximum":100},"cursor":{"type":"string"},"node_kinds":{"type":"array"},"native_type":{"type":"string"},"label":{"type":"string"},"label_match":{"type":"string","enum":["exact","contains"]},"semantic_part_kind":{"type":"string"},"visible":{"type":"boolean"},"source_document_uid":{"type":"string"}},"required":["graph_id","graph_revision","operation"],"additionalProperties":false},
      {"type":"object","properties":{"graph_id":{"type":"string"},"graph_revision":{"type":"string"},"operation":{"type":"string","enum":["neighbors"]},"limit":{"type":"integer","minimum":1,"maximum":100},"cursor":{"type":"string"},"start_node_ids":{"type":"array"},"direction":{"type":"string","enum":["incoming","outgoing","both"]},"edge_kinds":{"type":"array"}},"required":["graph_id","graph_revision","operation","start_node_ids"],"additionalProperties":false},
      {"type":"object","properties":{"graph_id":{"type":"string"},"graph_revision":{"type":"string"},"operation":{"type":"string","enum":["subgraph"]},"limit":{"type":"integer","minimum":1,"maximum":100},"cursor":{"type":"string"},"start_node_ids":{"type":"array"},"max_depth":{"type":"integer","minimum":0,"maximum":4},"edge_kinds":{"type":"array"}},"required":["graph_id","graph_revision","operation","start_node_ids","max_depth","edge_kinds"],"additionalProperties":false},
      {"type":"object","properties":{"graph_id":{"type":"string"},"graph_revision":{"type":"string"},"operation":{"type":"string","enum":["shortest_path"]},"limit":{"type":"integer","minimum":1,"maximum":100},"cursor":{"type":"string"},"start_node_id":{"type":"string"},"target_node_id":{"type":"string"},"max_depth":{"type":"integer","minimum":0,"maximum":4},"edge_kinds":{"type":"array"}},"required":["graph_id","graph_revision","operation","start_node_id","target_node_id","max_depth","edge_kinds"],"additionalProperties":false}
    ],"additionalProperties":false})";
constexpr const char* kAssemblyCreateSchema = R"json({
  "type":"object",
  "properties":{
    "operation":{"const":"create_assembly"},
    "operation_id":{"type":"string","minLength":1,"maxLength":128},
    "expected_graph_revision":{"type":"string","maxLength":128},
    "label":{"type":"string","minLength":1,"maxLength":160}
  },
  "required":["operation","operation_id","expected_graph_revision","label"],
  "additionalProperties":false
})json";
constexpr const char* kAssemblyInsertSchema = R"json({
  "type":"object",
  "properties":{
    "operation":{"const":"insert_component"},
    "operation_id":{"type":"string","minLength":1,"maxLength":128},
    "expected_graph_revision":{"type":"string","maxLength":128},
    "assembly":{"type":"object","properties":{"object_name":{"type":"string","minLength":1,"maxLength":256}},"required":["object_name"],"additionalProperties":false},
    "source":{"type":"object","properties":{"document_name":{"type":"string","minLength":1,"maxLength":256},"object_name":{"type":"string","minLength":1,"maxLength":256}},"required":["document_name","object_name"],"additionalProperties":false},
    "label":{"type":"string","minLength":1,"maxLength":160},
    "placement":{"type":"array","minItems":7,"maxItems":7,"items":{"type":"number"}}
  },
  "required":["operation","operation_id","expected_graph_revision","assembly","source"],
  "additionalProperties":false
})json";
constexpr const char* kPrimitiveSchema = R"json({
  "oneOf":[
    {"type":"object","properties":{
      "operation":{"const":"box"},"operation_id":{"type":"string","minLength":1,"maxLength":128},"expected_graph_revision":{"type":"string","maxLength":128},
      "label":{"type":"string","minLength":1,"maxLength":160},"center_mm":{"type":"object","properties":{"x":{"type":"number","minimum":-1000000,"maximum":1000000},"y":{"type":"number","minimum":-1000000,"maximum":1000000},"z":{"type":"number","minimum":-1000000,"maximum":1000000}},"required":["x","y","z"],"additionalProperties":false},
      "rotation":{"type":"object","properties":{"axis":{"type":"object","properties":{"x":{"type":"number"},"y":{"type":"number"},"z":{"type":"number"}},"required":["x","y","z"],"additionalProperties":false},"angle_degrees":{"type":"number","minimum":-360,"maximum":360}},"required":["axis","angle_degrees"],"additionalProperties":false},
      "length_mm":{"type":"number","exclusiveMinimum":0,"maximum":1000000},"width_mm":{"type":"number","exclusiveMinimum":0,"maximum":1000000},"height_mm":{"type":"number","exclusiveMinimum":0,"maximum":1000000}
    },"required":["operation","operation_id","expected_graph_revision","label","center_mm","length_mm","width_mm","height_mm"],"additionalProperties":false},
    {"type":"object","properties":{
      "operation":{"const":"cylinder"},"operation_id":{"type":"string","minLength":1,"maxLength":128},"expected_graph_revision":{"type":"string","maxLength":128},
      "label":{"type":"string","minLength":1,"maxLength":160},"center_mm":{"type":"object","properties":{"x":{"type":"number","minimum":-1000000,"maximum":1000000},"y":{"type":"number","minimum":-1000000,"maximum":1000000},"z":{"type":"number","minimum":-1000000,"maximum":1000000}},"required":["x","y","z"],"additionalProperties":false},
      "rotation":{"type":"object","properties":{"axis":{"type":"object","properties":{"x":{"type":"number"},"y":{"type":"number"},"z":{"type":"number"}},"required":["x","y","z"],"additionalProperties":false},"angle_degrees":{"type":"number","minimum":-360,"maximum":360}},"required":["axis","angle_degrees"],"additionalProperties":false},
      "radius_mm":{"type":"number","exclusiveMinimum":0,"maximum":1000000},"height_mm":{"type":"number","exclusiveMinimum":0,"maximum":1000000},"sweep_degrees":{"type":"number","exclusiveMinimum":0,"maximum":360}
    },"required":["operation","operation_id","expected_graph_revision","label","center_mm","radius_mm","height_mm"],"additionalProperties":false}
  ]
})json";
constexpr const char* kAssemblyGroundSchema = R"json({
  "oneOf":[
    {"type":"object","properties":{"operation":{"const":"set_grounded"},"operation_id":{"type":"string","minLength":1,"maxLength":128},"expected_graph_revision":{"type":"string","maxLength":128},"assembly":{"type":"object","properties":{"object_name":{"type":"string","minLength":1,"maxLength":256}},"required":["object_name"],"additionalProperties":false},"components":{"type":"array","minItems":1,"maxItems":16,"uniqueItems":true,"items":{"type":"string","minLength":1,"maxLength":128}},"expected_component_count":{"type":"integer","minimum":0,"maximum":1000000},"expected_grounded_count":{"type":"integer","minimum":0,"maximum":1000000}},"required":["operation","operation_id","expected_graph_revision","assembly","components"],"additionalProperties":false},
    {"type":"object","properties":{"operation":{"const":"set_movable"},"operation_id":{"type":"string","minLength":1,"maxLength":128},"expected_graph_revision":{"type":"string","maxLength":128},"assembly":{"type":"object","properties":{"object_name":{"type":"string","minLength":1,"maxLength":256}},"required":["object_name"],"additionalProperties":false},"components":{"type":"array","minItems":1,"maxItems":16,"uniqueItems":true,"items":{"type":"string","minLength":1,"maxLength":128}},"expected_component_count":{"type":"integer","minimum":0,"maximum":1000000},"expected_grounded_count":{"type":"integer","minimum":0,"maximum":1000000}},"required":["operation","operation_id","expected_graph_revision","assembly","components"],"additionalProperties":false}
  ]
})json";
constexpr const char* kAssemblyJointSchema = R"json({
  "type":"object",
  "properties":{
    "operation":{"const":"create"},"operation_id":{"type":"string","minLength":1,"maxLength":128},"expected_graph_revision":{"type":"string","maxLength":128},
    "assembly":{"type":"object","properties":{"object_name":{"type":"string","minLength":1,"maxLength":256}},"required":["object_name"],"additionalProperties":false},
    "first":{"$ref":"#/$defs/connector"},"second":{"$ref":"#/$defs/connector"},"joint_type":{"type":"string","enum":["fixed","revolute"]},"label":{"type":"string","minLength":1,"maxLength":160},"reverse":{"type":"boolean"},"limits":{"type":"object","properties":{"minimum_degrees":{"type":"number","minimum":-180,"maximum":180},"maximum_degrees":{"type":"number","minimum":-180,"maximum":180}},"required":["minimum_degrees","maximum_degrees"],"additionalProperties":false}
  },
  "required":["operation","operation_id","expected_graph_revision","assembly","first","second","joint_type"],
  "additionalProperties":false,
  "$defs":{"connector":{"type":"object","properties":{"component":{"type":"string","minLength":1,"maxLength":128},"connector_type":{"type":"string","enum":["element","interface"]},"connector":{"type":"string","minLength":1,"maxLength":512},"offset":{"type":"object","properties":{"translation_mm":{"type":"array","minItems":3,"maxItems":3,"items":{"type":"number","minimum":-1000000,"maximum":1000000}},"rotation_axis":{"type":"array","minItems":3,"maxItems":3,"items":{"type":"number","minimum":-1,"maximum":1}},"rotation_degrees":{"type":"number","minimum":-360,"maximum":360}},"required":["translation_mm","rotation_axis","rotation_degrees"],"additionalProperties":false}},"required":["component","connector_type","connector"],"additionalProperties":false}}
})json";

std::string jsonEscape(const std::string& value)
{
    std::string result;
    for (char character : value) {
        if (character == '\\' || character == '"') {
            result += '\\';
        }
        result += character;
    }
    return result;
}

std::string compactJson(const QJsonObject& object)
{
    return QJsonDocument(object).toJson(QJsonDocument::Compact).toStdString();
}

bool requiredString(const QJsonObject& object,
                    const char* key,
                    std::string& value,
                    std::string& diagnostic)
{
    const auto jsonValue = object.value(key);
    if (!jsonValue.isString() || jsonValue.toString().isEmpty()) {
        diagnostic = std::string("query field '") + key + "' must be a non-empty string";
        return false;
    }
    value = jsonValue.toString().toStdString();
    return true;
}

bool stringArray(const QJsonObject& object,
                 const char* key,
                 std::vector<std::string>& values,
                 std::string& diagnostic)
{
    if (!object.contains(key)) {
        return true;
    }
    const auto jsonValue = object.value(key);
    if (!jsonValue.isArray()) {
        diagnostic = std::string("query field '") + key + "' must be an array";
        return false;
    }
    for (const auto& element : jsonValue.toArray()) {
        if (!element.isString() || element.toString().isEmpty()) {
            diagnostic = std::string("query field '") + key + "' must contain strings";
            return false;
        }
        values.push_back(element.toString().toStdString());
    }
    return true;
}

bool nodeKindArray(const QJsonObject& object,
                   const char* key,
                   std::vector<NodeKind>& values,
                   std::string& diagnostic)
{
    std::vector<std::string> names;
    if (!stringArray(object, key, names, diagnostic)) {
        return false;
    }
    for (const auto& name : names) {
        NodeKind kind;
        if (!parseNodeKind(name, kind)) {
            diagnostic = "unknown node kind: " + name;
            return false;
        }
        values.push_back(kind);
    }
    return true;
}

bool edgeKindArray(const QJsonObject& object,
                   const char* key,
                   std::vector<EdgeKind>& values,
                   std::string& diagnostic)
{
    std::vector<std::string> names;
    if (!stringArray(object, key, names, diagnostic)) {
        return false;
    }
    for (const auto& name : names) {
        EdgeKind kind;
        if (!parseEdgeKind(name, kind)) {
            diagnostic = "unknown edge kind: " + name;
            return false;
        }
        values.push_back(kind);
    }
    return true;
}

bool parseQuery(const std::string& arguments, QueryRequest& request, std::string& diagnostic)
{
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(QByteArray::fromStdString(arguments), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        diagnostic = "query arguments must be a JSON object";
        return false;
    }
    const auto object = document.object();
    std::string operationName;
    if (!requiredString(object, "graph_id", request.graphId, diagnostic)
        || !requiredString(object, "graph_revision", request.graphRevision, diagnostic)
        || !requiredString(object, "operation", operationName, diagnostic)) {
        return false;
    }
    if (operationName == "summary") {
        request.operation = QueryOperation::Summary;
    }
    else if (operationName == "find_nodes") {
        request.operation = QueryOperation::FindNodes;
    }
    else if (operationName == "neighbors") {
        request.operation = QueryOperation::Neighbors;
    }
    else if (operationName == "subgraph") {
        request.operation = QueryOperation::Subgraph;
    }
    else if (operationName == "shortest_path") {
        request.operation = QueryOperation::ShortestPath;
    }
    else {
        diagnostic = "unsupported graph operation: " + operationName;
        return false;
    }

    if (object.contains("limit")) {
        const auto value = object.value("limit");
        if (!value.isDouble() || value.toInt() < 1 || value.toInt() > 100) {
            diagnostic = "query limit must be between 1 and 100";
            return false;
        }
        request.limit = static_cast<std::size_t>(value.toInt());
    }
    if (object.contains("cursor")) {
        if (!object.value("cursor").isString()) {
            diagnostic = "query cursor must be a string";
            return false;
        }
        request.cursor = object.value("cursor").toString().toStdString();
    }
    if (!nodeKindArray(object, "node_kinds", request.nodeKinds, diagnostic)
        || !edgeKindArray(object, "edge_kinds", request.edgeKinds, diagnostic)) {
        return false;
    }
    if (object.contains("start_node_ids")
        && !stringArray(object, "start_node_ids", request.startNodeIds, diagnostic)) {
        return false;
    }
    if (object.contains("native_type")) {
        if (!object.value("native_type").isString()) {
            diagnostic = "native_type must be a string";
            return false;
        }
        request.nativeType = object.value("native_type").toString().toStdString();
    }
    if (object.contains("label")) {
        if (!object.value("label").isString()) {
            diagnostic = "label must be a string";
            return false;
        }
        request.label = object.value("label").toString().toStdString();
    }
    if (object.contains("label_match")) {
        const auto match = object.value("label_match").toString();
        if (match != "exact" && match != "contains") {
            diagnostic = "label_match must be exact or contains";
            return false;
        }
        request.labelContains = match == "contains";
    }
    if (object.contains("semantic_part_kind")) {
        request.semanticPartKind = object.value("semantic_part_kind").toString().toStdString();
    }
    if (object.contains("source_document_uid")) {
        request.sourceDocumentUid = object.value("source_document_uid").toString().toStdString();
    }
    if (object.contains("visible")) {
        if (!object.value("visible").isBool()) {
            diagnostic = "visible must be boolean";
            return false;
        }
        request.filterVisible = true;
        request.visible = object.value("visible").toBool();
    }
    if (object.contains("direction")) {
        const auto direction = object.value("direction").toString();
        if (direction == "incoming") {
            request.direction = QueryDirection::Incoming;
        }
        else if (direction == "outgoing") {
            request.direction = QueryDirection::Outgoing;
        }
        else if (direction != "both") {
            diagnostic = "direction must be incoming, outgoing, or both";
            return false;
        }
    }
    if (object.contains("max_depth")) {
        const auto value = object.value("max_depth");
        if (!value.isDouble() || value.toInt() < 0 || value.toInt() > 4) {
            diagnostic = "max_depth must be between 0 and 4";
            return false;
        }
        request.maxDepth = static_cast<std::size_t>(value.toInt());
    }
    if (request.operation == QueryOperation::Neighbors
        || request.operation == QueryOperation::Subgraph) {
        if (request.startNodeIds.empty()) {
            diagnostic = "this operation requires start_node_ids";
            return false;
        }
    }
    if (request.operation == QueryOperation::Subgraph && !object.contains("max_depth")) {
        diagnostic = "subgraph requires max_depth";
        return false;
    }
    if (request.operation == QueryOperation::ShortestPath) {
        if (!requiredString(object, "start_node_id", request.startNodeId, diagnostic)
            || !requiredString(object, "target_node_id", request.targetNodeId, diagnostic)
            || !object.contains("max_depth")) {
            if (diagnostic.empty()) {
                diagnostic = "shortest_path requires max_depth";
            }
            return false;
        }
    }
    return true;
}

QJsonArray placementJson(const Placement& placement)
{
    return {placement.x, placement.y, placement.z, placement.qx,
            placement.qy, placement.qz, placement.qw};
}

QJsonObject nodeJson(const NodeRecord& node)
{
    QJsonObject native {
        {"document_uid", QString::fromStdString(node.native.documentUid)},
        {"object_name", QString::fromStdString(node.native.objectName)},
        {"type_id", QString::fromStdString(node.native.typeId)}};
    QJsonObject display {
        {"label", QString::fromStdString(node.display.label)},
        {"normalized_label", QString::fromStdString(node.display.normalizedLabel)}};
    QJsonObject provenance {
        {"kind", QString::fromStdString(node.provenance.kind)},
        {"evidence", QString::fromStdString(node.provenance.evidence)}};
    QJsonObject presentation {
        {"visible", node.presentation.visible},
        {"selected", node.presentation.selected},
        {"view_id", QString::fromStdString(node.presentation.viewId)}};
    return {
        {"id", QString::fromStdString(node.id)},
        {"kind", QString::fromLatin1(nodeKindName(node.kind))},
        {"native", native},
        {"display", display},
        {"provenance", provenance},
        {"presentation", presentation},
        {"local_placement", placementJson(node.localPlacement)},
        {"world_placement", placementJson(node.worldPlacement)},
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
        {"relation", QString::fromStdString(edge.relation)},
        {"provenance", QJsonObject {
             {"kind", QString::fromStdString(edge.provenance.kind)},
             {"evidence", QString::fromStdString(edge.provenance.evidence)}}}
    };
}

QString operationName(QueryOperation operation)
{
    switch (operation) {
        case QueryOperation::Summary: return QStringLiteral("summary");
        case QueryOperation::FindNodes: return QStringLiteral("find_nodes");
        case QueryOperation::Neighbors: return QStringLiteral("neighbors");
        case QueryOperation::Subgraph: return QStringLiteral("subgraph");
        case QueryOperation::ShortestPath: return QStringLiteral("shortest_path");
    }
    return QStringLiteral("unknown");
}

std::string queryJson(const GraphSnapshot& snapshot,
                      const QueryRequest& request,
                      const QueryResult& result)
{
    QJsonArray nodes;
    for (const auto& id : result.nodeIds) {
        if (const auto* node = snapshot.findNode(id)) {
            nodes.push_back(nodeJson(*node));
        }
    }
    QJsonArray edges;
    for (const auto& id : result.edgeIds) {
        if (const auto* edge = snapshot.findEdge(id)) {
            edges.push_back(edgeJson(*edge));
        }
    }
    QJsonObject output {
        {"schema_version", "cadx.assembly-graph-query-result.v1"},
        {"ok", true},
        {"graph_id", QString::fromStdString(snapshot.header().graphId)},
        {"graph_revision", QString::fromStdString(snapshot.header().graphRevision)},
        {"operation", operationName(request.operation)},
        {"node_count", static_cast<qint64>(snapshot.nodes().size())},
        {"edge_count", static_cast<qint64>(snapshot.edges().size())},
        {"returned_node_count", static_cast<qint64>(result.returnedNodeCount)},
        {"returned_edge_count", static_cast<qint64>(result.returnedEdgeCount)},
        {"truncated", result.truncated},
        {"next_cursor", result.nextCursor.empty()
                ? QJsonValue(QJsonValue::Null)
                : QJsonValue(QString::fromStdString(result.nextCursor))},
        {"nodes", nodes},
        {"edges", edges},
        {"diagnostics", QJsonArray {}}
    };
    return compactJson(output);
}

std::string summaryJson(const GraphSnapshot& snapshot)
{
    std::size_t visibleOccurrences = 0;
    std::size_t unresolved = 0;
    for (const auto& node : snapshot.nodes()) {
        if ((node.kind == NodeKind::Occurrence || node.kind == NodeKind::AssemblyOccurrence)
            && node.presentation.visible) {
            ++visibleOccurrences;
        }
        unresolved += node.unresolved ? 1 : 0;
    }
    const auto& header = snapshot.header();
    return "{\"schema_version\":\"cadx.assembly-graph-result.v1\",\"ok\":true,"
        "\"graph_id\":\"" + jsonEscape(header.graphId) + "\",\"graph_revision\":\""
        + jsonEscape(header.graphRevision) + "\",\"presentation_revision\":\""
        + jsonEscape(header.presentationRevision) + "\",\"document\":{\"document_uid\":\""
        + jsonEscape(header.documentUid) + "\",\"document_name\":\""
        + jsonEscape(header.documentName) + "\"},\"active_assembly\":{\"node_id\":\""
        + jsonEscape(header.activeAssemblyNodeId) + "\",\"object_name\":\""
        + jsonEscape(header.activeAssemblyObjectName) + "\",\"label\":\""
        + jsonEscape(header.activeAssemblyLabel) + "\"},\"complete\":"
        + (header.complete ? "true" : "false") + ",\"node_count\":"
        + std::to_string(snapshot.nodes().size()) + ",\"edge_count\":"
        + std::to_string(snapshot.edges().size()) + ",\"visible_occurrence_count\":"
        + std::to_string(visibleOccurrences) + ",\"unresolved_reference_count\":"
        + std::to_string(unresolved) + ",\"diagnostics\":[]}";
}

}  // namespace

CadXService::CadXService(bool observeFreeCadDocuments)
    : _audit(GraphAuditLog::fromEnvironment())
    , _observer(_graphs, observeFreeCadDocuments)
    , _gateway()
    , _assemblyMutations(_graphs, _audit)
#ifdef CADX_HAVE_PART_DESIGN
    , _primitiveMutations(_graphs, _audit)
#endif
    , _constraintMutations(_graphs, _audit)
{
    _tools.setThreadDispatcher([this](ThreadRequirement requirement, std::function<void()> task) {
        if (requirement == ThreadRequirement::MainThread) {
            _gateway.run(std::move(task));
        }
        else {
            task();
        }
    });
    registerTools();
}

void CadXService::registerTools()
{
    std::string diagnostic;
    // model.primitive remains intentionally withheld: standalone primitive
    // creation still lacks a defined part-graph capture scope and cannot yet
    // prove CAD-to-graph parity through the shared mutation pipeline.
    (void)kPrimitiveSchema;
    _tools.registerDefinition(
        {"assembly.graph_query",
         "Query nodes and relationships from one exact stored Assembly graph revision.",
         ToolClassification::Read,
         kQuerySchema,
         "cadx.assembly-graph-query-result.v1",
         [this](const std::string& arguments) { return executeQuery(arguments); },
         ThreadRequirement::Worker,
         128 * 1024},
        diagnostic);
#ifdef CADX_HAVE_ASSEMBLY
    _tools.registerDefinition(
        {"assembly.create", "Create a FreeCAD Assembly and publish its verified graph revision.",
         ToolClassification::Mutation, kAssemblyCreateSchema, "cadx.assembly-mutation-result.v1",
         [this](const std::string& arguments) {
             return _assemblyMutations.execute("assembly.create", arguments);
         }, ThreadRequirement::MainThread, 128 * 1024},
        diagnostic);
    _tools.registerDefinition(
        {"assembly.insert", "Insert a resolved source object as an Assembly link and publish its verified graph revision.",
         ToolClassification::Mutation, kAssemblyInsertSchema, "cadx.assembly-mutation-result.v1",
         [this](const std::string& arguments) {
             return _assemblyMutations.execute("assembly.insert", arguments);
         }, ThreadRequirement::MainThread, 128 * 1024},
        diagnostic);
    _tools.registerDefinition(
        {"assembly.ground", "Ground or release Assembly components and publish their verified graph revision.",
         ToolClassification::Mutation, kAssemblyGroundSchema, "cadx.assembly-constraint-result.v1",
         [this](const std::string& arguments) {
             return _constraintMutations.execute("assembly.ground", arguments);
         }, ThreadRequirement::MainThread, 128 * 1024},
        diagnostic);
    _tools.registerDefinition(
        {"assembly.joint", "Create a fixed or revolute Assembly joint and publish its verified graph revision.",
         ToolClassification::Mutation, kAssemblyJointSchema, "cadx.assembly-constraint-result.v1",
         [this](const std::string& arguments) {
             return _constraintMutations.execute("assembly.joint", arguments);
         }, ThreadRequirement::MainThread, 128 * 1024},
        diagnostic);
#endif
}

ToolResult CadXService::executeTool(const std::string& name, const std::string& argumentsJson) const
{
    return _tools.execute(name, argumentsJson);
}

ToolResult CadXService::publishCapture(const AssemblyCapture& capture)
{
    _audit.record(makeGraphAuditEvent(
        "build", "started", nullptr, "assembly.graph_snapshot"));
    const auto built = _builder.build(capture);
    if (!built) {
        _audit.record(makeGraphAuditEvent("build", "failed", nullptr,
                                          "assembly.graph_snapshot", built.errorCode,
                                          built.diagnostic));
        return ToolResult::failure(built.errorCode, built.diagnostic, built.errorCode == "CADX_DOCUMENT_BUSY");
    }
    _audit.record(makeGraphAuditEvent("build", "passed", built.snapshot.get(),
                                      "assembly.graph_snapshot"));
    const auto evidence = GraphJsonCodec::roundTrip(*built.snapshot);
    if (!evidence) {
        _audit.record(makeGraphAuditEvent("round_trip", "failed", built.snapshot.get(),
                                          "assembly.graph_snapshot", evidence.errorCode,
                                          evidence.diagnostic));
        return ToolResult::failure(evidence.errorCode, evidence.diagnostic);
    }
    _audit.record(makeGraphAuditEvent("round_trip", "passed", built.snapshot.get(),
                                      "assembly.graph_snapshot"));
    GraphScope scope {capture.documentUid, capture.activeAssemblyObjectName};
    std::string diagnostic;
    const auto error = _graphs.publish(scope, built.snapshot, diagnostic);
    if (error != StoreError::None) {
        const auto code = error == StoreError::LimitExceeded
            ? "CADX_GRAPH_LIMIT_EXCEEDED"
            : "CADX_INTERNAL_ERROR";
        _audit.record(makeGraphAuditEvent("publish", "failed", built.snapshot.get(),
                                          "assembly.graph_snapshot", code, diagnostic));
        return ToolResult::failure(
            code,
            diagnostic);
    }
    _audit.record(makeGraphAuditEvent("publish", "passed", built.snapshot.get(),
                                      "assembly.graph_snapshot"));
    return ToolResult::success("cadx.assembly-graph-result.v1", summaryJson(*built.snapshot));
}

ToolResult CadXService::exportGraphEvidence(const std::string& graphId,
                                             const std::string& graphRevision) const
{
    const auto lookup = _graphs.lookup(graphId, graphRevision, true);
    if (!lookup) {
        const char* code = lookup.error == StoreError::GraphStale ? "CADX_GRAPH_STALE"
            : lookup.error == StoreError::RevisionMismatch ? "CADX_GRAPH_REVISION_MISMATCH"
            : "CADX_GRAPH_NOT_FOUND";
        return ToolResult::failure(code, lookup.diagnostic, true);
    }
    return ToolResult::success(GraphJsonCodec::schemaVersion,
                               GraphJsonCodec::encode(*lookup.snapshot));
}

bool CadXService::registerGuiSnapshotProvider(ToolExecutor provider, std::string& diagnostic)
{
    if (!provider) {
        diagnostic = "GUI snapshot provider is empty";
        return false;
    }
    return _tools.registerDefinition(
        {"assembly.graph_snapshot",
         "Capture the active Assembly view as a revisioned semantic graph and retain it in memory for bounded queries.",
         ToolClassification::Read,
         kSnapshotSchema,
         "cadx.assembly-graph-result.v1",
         std::move(provider),
         ThreadRequirement::MainThread,
         128 * 1024},
        diagnostic);
}

ToolResult CadXService::summarizeSnapshot(const GraphSnapshot& snapshot) const
{
    return ToolResult::success("cadx.assembly-graph-result.v1", summaryJson(snapshot));
}

ToolResult CadXService::executeQuery(const std::string& argumentsJson) const
{
    QueryRequest request;
    std::string diagnostic;
    if (!parseQuery(argumentsJson, request, diagnostic)) {
        _audit.record(makeGraphAuditEvent(
            "query", "failed", nullptr, "assembly.graph_query", "CADX_QUERY_INVALID", diagnostic));
        return ToolResult::failure("CADX_QUERY_INVALID", diagnostic);
    }
    const auto lookup = _graphs.lookup(request.graphId, request.graphRevision);
    if (!lookup) {
        const char* code = lookup.error == StoreError::GraphStale ? "CADX_GRAPH_STALE"
            : lookup.error == StoreError::RevisionMismatch ? "CADX_GRAPH_REVISION_MISMATCH"
            : "CADX_GRAPH_NOT_FOUND";
        _audit.record(makeGraphAuditEvent(
            "query", "failed", nullptr, "assembly.graph_query", code, lookup.diagnostic));
        return ToolResult::failure(code, lookup.diagnostic, true);
    }
    const auto result = GraphQueryEngine().execute(lookup.snapshot, request);
    if (!result.ok) {
        _audit.record(makeGraphAuditEvent("query", "failed", lookup.snapshot.get(),
                                          "assembly.graph_query", result.errorCode,
                                          result.diagnostic));
        return ToolResult::failure(result.errorCode, result.diagnostic, true);
    }
    _audit.record(makeGraphAuditEvent("query", "passed", lookup.snapshot.get(),
                                      "assembly.graph_query"));
    return ToolResult::success("cadx.assembly-graph-query-result.v1",
                               queryJson(*lookup.snapshot, request, result));
}

CadXService& service()
{
    static CadXService instance(true);
    return instance;
}

}  // namespace CadX
