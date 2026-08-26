// SPDX-License-Identifier: LGPL-2.1-or-later

#include "NativeAssemblyOperations.h"

#include "AssemblyCapture.h"
#include "AssemblyGraphBuilder.h"
#include "AssemblyObjectAdapter.h"
#include "GraphRevision.h"
#include "NativeMutationSupport.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>
#include <App/Part.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <Base/Interpreter.h>
#include <Base/Placement.h>

#ifdef CADX_HAVE_ASSEMBLY
#include <Mod/Assembly/App/AssemblyLink.h>
#include <Mod/Assembly/App/AssemblyObject.h>
#include <Mod/Assembly/App/AssemblyUtils.h>
#include <Mod/Assembly/App/Groups.h>
#endif

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <set>
#include <unordered_map>

namespace CadX
{
namespace
{
struct Parsed
{
    QJsonObject object;
    std::string operationId;
    std::string expectedRevision;
};

ToolResult invalid(const std::string& message)
{
    return ToolResult::failure("CADX_TOOL_ARGUMENTS_INVALID", message);
}

bool stringField(const QJsonObject& object, const char* key, std::string& value,
                 std::string& diagnostic, std::size_t minLength = 1, std::size_t maxLength = 512)
{
    const auto item = object.value(key);
    if (!item.isString()) {
        diagnostic = std::string("field '") + key + "' must be a string";
        return false;
    }
    value = item.toString().toStdString();
    if (value.size() < minLength || value.size() > maxLength) {
        diagnostic = std::string("field '") + key + "' has an invalid length";
        return false;
    }
    return true;
}

bool finiteNumber(const QJsonObject& object, const char* key, double& value,
                  std::string& diagnostic, double minimum, double maximum,
                  bool required = true)
{
    if (!object.contains(key)) {
        if (required) {
            diagnostic = std::string("missing field '") + key + "'";
            return false;
        }
        return true;
    }
    const auto item = object.value(key);
    if (!item.isDouble() || !std::isfinite(item.toDouble())) {
        diagnostic = std::string("field '") + key + "' must be finite";
        return false;
    }
    value = item.toDouble();
    if (value < minimum || value > maximum) {
        diagnostic = std::string("field '") + key + "' is outside its bounds";
        return false;
    }
    return true;
}

bool closed(const QJsonObject& object, std::initializer_list<const char*> fields,
            std::string& diagnostic)
{
    std::set<std::string> allowed(fields.begin(), fields.end());
    for (auto iterator = object.begin(); iterator != object.end(); ++iterator) {
        if (!allowed.contains(iterator.key().toStdString())) {
            diagnostic = "unknown field '" + iterator.key().toStdString() + "'";
            return false;
        }
    }
    return true;
}

bool objectRef(const QJsonValue& value, std::string& objectName, std::string& diagnostic)
{
    if (!value.isObject()) {
        diagnostic = "object reference must be an object";
        return false;
    }
    const auto object = value.toObject();
    if (!closed(object, {"object_name"}, diagnostic)
        || !stringField(object, "object_name", objectName, diagnostic, 1, 256)) {
        return false;
    }
    return true;
}

bool placement(const QJsonValue& value, Placement& result, std::string& diagnostic)
{
    if (!value.isArray() || value.toArray().size() != 7) {
        diagnostic = "placement must be [x,y,z,qx,qy,qz,qw]";
        return false;
    }
    auto array = value.toArray();
    double* fields[] = {&result.x, &result.y, &result.z, &result.qx,
                        &result.qy, &result.qz, &result.qw};
    for (int index = 0; index != 7; ++index) {
        if (!array[index].isDouble() || !std::isfinite(array[index].toDouble())) {
            diagnostic = "placement contains a non-finite number";
            return false;
        }
        *fields[index] = array[index].toDouble();
    }
    if (!result.normalize()) {
        diagnostic = "placement quaternion is invalid";
        return false;
    }
    return true;
}

bool parseCommon(const QJsonObject& object, Parsed& parsed, std::string& diagnostic,
                 std::initializer_list<const char*> fields)
{
    if (!object.contains("operation_id") || !object.contains("expected_graph_revision")
        || !stringField(object, "operation_id", parsed.operationId, diagnostic, 1, 128)
        || !stringField(object, "expected_graph_revision", parsed.expectedRevision, diagnostic, 0, 128)) {
        return false;
    }
    if (!closed(object, fields, diagnostic)) {
        return false;
    }
    return true;
}

bool parse(const std::string& json, Parsed& parsed, std::string& diagnostic)
{
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(QByteArray::fromStdString(json), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        diagnostic = "arguments must be a JSON object";
        return false;
    }
    parsed.object = document.object();
    return true;
}

std::string json(const QJsonObject& object)
{
    return QJsonDocument(object).toJson(QJsonDocument::Compact).toStdString();
}

#ifdef CADX_HAVE_ASSEMBLY
bool propertyValue(App::DocumentObject* object, const char* name, double& value)
{
    if (!object) {
        return false;
    }
    auto* property = object->getPropertyByName(name);
    if (!property) {
        return false;
    }
    if (auto* quantity = dynamic_cast<App::PropertyQuantity*>(property)) {
        value = quantity->getValue();
        return std::isfinite(value);
    }
    if (auto* floating = dynamic_cast<App::PropertyFloat*>(property)) {
        value = floating->getValue();
        return std::isfinite(value);
    }
    return false;
}

bool applyPlacement(App::DocumentObject* object,
                    const Placement& placementValue,
                    std::string& diagnostic)
{
    auto* property = object
        ? dynamic_cast<App::PropertyPlacement*>(object->getPropertyByName("Placement"))
        : nullptr;
    if (!property) {
        diagnostic = "the inserted component has no writable Placement property";
        return false;
    }
    property->setValue(Base::Placement(
        Base::Vector3d(placementValue.x, placementValue.y, placementValue.z),
        Base::Rotation(placementValue.qx, placementValue.qy,
                       placementValue.qz, placementValue.qw)));
    return true;
}

bool placementMatches(App::DocumentObject* object,
                       const Placement& expected,
                       std::string& diagnostic)
{
    if (!object) {
        diagnostic = "the inserted component disappeared";
        return false;
    }
    auto* property = dynamic_cast<App::PropertyPlacement*>(
        object->getPropertyByName("Placement"));
    if (!property) {
        diagnostic = "the inserted component has no Placement property";
        return false;
    }
    const auto value = property->getValue();
    double qx = 0.0;
    double qy = 0.0;
    double qz = 0.0;
    double qw = 1.0;
    value.getRotation().getValue(qx, qy, qz, qw);
    Placement actual {value.getPosition().x, value.getPosition().y, value.getPosition().z,
                      qx, qy, qz, qw};
    actual.normalize();
    const auto close = [](double left, double right) {
        return std::abs(left - right) <= 1.0e-8 * std::max({1.0, std::abs(left), std::abs(right)});
    };
    if (!close(actual.x, expected.x) || !close(actual.y, expected.y)
        || !close(actual.z, expected.z) || !close(actual.qx, expected.qx)
        || !close(actual.qy, expected.qy) || !close(actual.qz, expected.qz)
        || !close(actual.qw, expected.qw)) {
        diagnostic = "inserted component placement does not match the request";
        return false;
    }
    return true;
}

bool expectedBaseAvailable(const GraphStore& graphs,
                           const GraphScope& scope,
                           const std::string& expectedRevision,
                           std::string& diagnostic)
{
    const auto current = graphs.current(scope, true);
    if (expectedRevision.empty()) {
        if (current) {
            diagnostic = "an empty expected graph revision requires an unpublished graph scope";
            return false;
        }
        return true;
    }
    if (!current) {
        diagnostic = current.diagnostic.empty()
            ? "the expected graph revision is not available"
            : current.diagnostic;
        return false;
    }
    if (current.snapshot->header().graphRevision != expectedRevision) {
        diagnostic = "the expected graph revision is stale";
        return false;
    }
    return true;
}

bool equalCommittedGraph(const GraphSnapshot& candidate,
                         const GraphSnapshot& committed,
                         std::string& diagnostic)
{
    if (candidate.header().graphId != committed.header().graphId
        || candidate.header().graphRevision != committed.header().graphRevision
        || canonicalSemantic(candidate) != canonicalSemantic(committed)) {
        diagnostic = "post-commit CAD capture differs from the verified candidate graph";
        return false;
    }
    if (candidate.header().presentationRevision != committed.header().presentationRevision
        || canonicalPresentation(candidate) != canonicalPresentation(committed)) {
        diagnostic = "post-commit CAD presentation differs from the verified candidate graph";
        return false;
    }
    return true;
}

void recordMutationFailure(GraphAuditLog& audit,
                           const std::string& stage,
                           const std::string& operation,
                           const Parsed& parsed,
                           const std::string& parentRevision,
                           const std::string& predicted,
                           const std::string& observed,
                           const std::string& transactionStatus,
                           const GraphSnapshot* snapshot,
                           const std::string& errorCode,
                           const std::string& diagnostic)
{
    audit.record(makeMutationAuditEvent(stage, "failed", operation, parsed.operationId,
                                        parentRevision,
                                        snapshot ? snapshot->header().graphRevision : std::string {},
                                        predicted, observed, "invalid", transactionStatus,
                                        snapshot, errorCode, diagnostic));
}

NativeIdentity native(App::DocumentObject* object)
{
    const auto type = object->getTypeId().getName();
    return {object->getDocument()->Uid.getValueStr(), object->getNameInDocument(),
            std::string(type.data(), type.size())};
}

NodeId nodeId(const std::string& role, const NativeIdentity& identity,
              const std::string& path = {})
{
    return "node:" + sha256Revision("cadx.native-capture.v2|" + role + "|"
                                     + identity.canonical() + (path.empty() ? "" : "|" + path));
}

EdgeId edgeId(EdgeKind kind, const NodeId& from, const NodeId& to,
              const std::string& relation = {})
{
    return "edge:" + sha256Revision("cadx.native-capture.edge.v2|"
                                     + std::string(edgeKindName(kind)) + "|" + from + "|" + to
                                     + "|" + relation);
}

void addToAssembly(Assembly::AssemblyObject* assembly, App::DocumentObject* object)
{
    if (!assembly || !object) {
        return;
    }
    auto objects = assembly->Group.getValues();
    objects.push_back(object);
    assembly->Group.setValues(std::move(objects));
}

AssemblyCapture captureDocument(App::Document* document, Assembly::AssemblyObject* assembly)
{
    AssemblyCapture capture;
    capture.documentUid = document->Uid.getValueStr();
    capture.documentName = document->getName();
    capture.activeAssemblyObjectName = assembly->getNameInDocument();
    capture.activeAssemblyLabel = assembly->Label.getValue();
    NativeIdentity docNative {capture.documentUid, capture.documentName, "App::Document"};
    NodeRecord docNode;
    docNode.id = nodeId("document", docNative);
    docNode.kind = NodeKind::Document;
    docNode.native = docNative;
    docNode.display = {document->Label.getValue(), document->Label.getValue()};
    capture.nodes.push_back(docNode);

    const auto assemblyNative = native(assembly);
    NodeRecord assemblyNode;
    assemblyNode.id = nodeId("assembly-definition", assemblyNative);
    assemblyNode.kind = NodeKind::AssemblyDefinition;
    assemblyNode.native = assemblyNative;
    assemblyNode.display = {assembly->Label.getValue(), assembly->Label.getValue()};
    assemblyNode.payload = DefinitionPayload {"definition", "assembly", "compound",
                                               "local_parametric", "unknown"};
    capture.activeAssemblyNodeId = assemblyNode.id;
    capture.nodes.push_back(assemblyNode);
    capture.edges.push_back({edgeId(EdgeKind::SourceDocument, assemblyNode.id, docNode.id),
                             EdgeKind::SourceDocument, assemblyNode.id, docNode.id, {}, {}});
    std::unordered_map<std::string, NodeId> occurrenceIds;

    for (auto* child : assembly->Group.getValues()) {
        if (!child || !document->containsObject(child)) {
            continue;
        }
        auto* definition = child;
        if (child->isDerivedFrom<App::Link>()) {
            definition = static_cast<App::Link*>(child)->getTrueLinkedObject(false);
        }
        const bool unresolved = !definition;
        if (!definition) {
            definition = child;
        }
        const auto definitionNative = native(definition);
        NodeRecord definitionNode;
        definitionNode.id = nodeId("definition", definitionNative);
        const auto type = definition->getTypeId().getName();
        const auto classification = AssemblyObjectAdapter::classify(
            std::string(type.data(), type.size()));
        definitionNode.kind = unresolved ? NodeKind::UnresolvedDefinition
                                         : classification.nodeKind;
        if (definitionNode.kind == NodeKind::Occurrence) {
            definitionNode.kind = NodeKind::FeatureDefinition;
        }
        definitionNode.native = definitionNative;
        definitionNode.display = {definition->Label.getValue(), definition->Label.getValue()};
        definitionNode.provenance.kind = classification.provenanceKind;
        definitionNode.unresolved = unresolved;
        if (unresolved) {
            definitionNode.payload = UnresolvedPayload {definitionNative.documentUid,
                                                         definitionNative.objectName,
                                                         "link target is unresolved"};
        }
        else {
            definitionNode.payload = DefinitionPayload {classification.role,
                                                         classification.containerKind,
                                                         classification.geometryKind,
                                                         classification.provenanceKind,
                                                         "unknown"};
        }
        if (definition->getTypeId().getName() == std::string("Part::Box")) {
            PrimitivePayload payload;
            payload.primitiveKind = "box";
            propertyValue(definition, "Length", payload.length);
            propertyValue(definition, "Width", payload.width);
            propertyValue(definition, "Height", payload.height);
            definitionNode.payload = payload;
        }
        else if (definition->getTypeId().getName() == std::string("Part::Cylinder")) {
            PrimitivePayload payload;
            payload.primitiveKind = "cylinder";
            propertyValue(definition, "Radius", payload.radius);
            propertyValue(definition, "Height", payload.height);
            propertyValue(definition, "Angle", payload.sweepDegrees);
            definitionNode.payload = payload;
        }
        capture.nodes.push_back(definitionNode);
        capture.edges.push_back({edgeId(EdgeKind::SourceDocument, definitionNode.id, docNode.id),
                                 EdgeKind::SourceDocument, definitionNode.id, docNode.id, {}, {}});

        const auto childNative = native(child);
        NodeRecord occurrence;
        occurrence.id = nodeId("occurrence", childNative,
                               std::string(assembly->getNameInDocument()) + "|"
                                   + child->getNameInDocument());
        occurrence.kind = NodeKind::Occurrence;
        occurrence.native = childNative;
        occurrence.display = {child->Label.getValue(), child->Label.getValue()};
        occurrence.payload = OccurrencePayload {{assembly->getNameInDocument(),
                                                  child->getNameInDocument()}, false, false, {}};
        occurrence.unresolved = unresolved;
        capture.nodes.push_back(occurrence);
        occurrenceIds[child->getNameInDocument()] = occurrence.id;
        capture.edges.push_back({edgeId(EdgeKind::Contains, assemblyNode.id, occurrence.id),
                                 EdgeKind::Contains, assemblyNode.id, occurrence.id, {}, {}});
        capture.edges.push_back({edgeId(unresolved ? EdgeKind::UnresolvedSource : EdgeKind::InstanceOf,
                                        occurrence.id, definitionNode.id),
                                 unresolved ? EdgeKind::UnresolvedSource : EdgeKind::InstanceOf,
                                 occurrence.id, definitionNode.id, {}, {}});
    }

    for (auto* joint : assembly->getJoints(false, false)) {
        if (!joint) continue;
        const auto jointNative = native(joint);
        NodeRecord node;
        node.id = nodeId("joint", jointNative);
        node.kind = NodeKind::Joint;
        node.native = jointNative;
        node.display = {joint->Label.getValue(), joint->Label.getValue()};
        JointPayload payload;
        if (auto* type = joint->getPropertyByName("JointType")) {
            if (auto* enumeration = dynamic_cast<App::PropertyEnumeration*>(type)) {
                payload.jointType = enumeration->getValueAsString();
            }
        }
        node.payload = payload;
        capture.nodes.push_back(node);
        capture.edges.push_back({edgeId(EdgeKind::HasJoint, assemblyNode.id, node.id),
                                 EdgeKind::HasJoint, assemblyNode.id, node.id, {}, {}});
        for (const char* refName : {"Reference1", "Reference2"}) {
            auto* property = dynamic_cast<App::PropertyXLinkSub*>(joint->getPropertyByName(refName));
            auto* target = property ? Assembly::getObjFromRef(property) : nullptr;
            if (target) {
                auto found = occurrenceIds.find(target->getNameInDocument());
                if (found != occurrenceIds.end()) {
                    capture.edges.push_back({edgeId(EdgeKind::JointEndpoint, node.id, found->second,
                                                   refName),
                                             EdgeKind::JointEndpoint, node.id, found->second, {}, refName});
                }
            }
        }
    }
    for (auto* grounded : assembly->getGroundedJoints()) {
        if (!grounded) continue;
        auto* targetProperty = grounded->getPropertyByName("ObjectToGround");
        auto* linkProperty = targetProperty
            ? dynamic_cast<App::PropertyLink*>(targetProperty)
            : nullptr;
        auto* target = linkProperty ? linkProperty->getValue() : nullptr;
        if (!target) continue;
        auto found = occurrenceIds.find(target->getNameInDocument());
        if (found == occurrenceIds.end()) continue;
        const auto groundedNative = native(grounded);
        NodeRecord node;
        node.id = nodeId("ground", groundedNative);
        node.kind = NodeKind::GroundConstraint;
        node.native = groundedNative;
        node.display = {grounded->Label.getValue(), grounded->Label.getValue()};
        node.payload = GroundConstraintPayload {true, target->getNameInDocument(), grounded->getNameInDocument()};
        capture.nodes.push_back(node);
        capture.edges.push_back({edgeId(EdgeKind::GroundedBy, node.id, found->second),
                                 EdgeKind::GroundedBy, node.id, found->second, {}, {}});
    }
    return capture;
}

ToolResult receipt(const std::string& operation, const Parsed& parsed,
                   const std::string& parent, const std::string& final,
                   const std::string& predicted, const std::string& observed,
                   const std::string& physical, const std::string& transaction,
                   const GraphSnapshot* snapshot = nullptr)
{
    QJsonObject output {
        {"schema_version", "cadx.assembly-mutation-result.v1"}, {"ok", true},
        {"operation", QString::fromStdString(operation)},
        {"operation_id", QString::fromStdString(parsed.operationId)},
        {"parent_revision", QString::fromStdString(parent)},
        {"final_revision", QString::fromStdString(final)},
        {"predicted_delta_hash", QString::fromStdString(predicted)},
        {"observed_delta_hash", QString::fromStdString(observed)},
        {"physical_verdict", QString::fromStdString(physical)},
        {"transaction_status", QString::fromStdString(transaction)}};
    if (snapshot) {
        output.insert("graph_id", QString::fromStdString(snapshot->header().graphId));
    }
    return ToolResult::success("cadx.assembly-mutation-result.v1", json(output));
}
#endif
}  // namespace

CommitPublication publishCommittedGraph(GraphStore& graphs,
                                        const GraphScope& scope,
                                        std::shared_ptr<GraphSnapshot> snapshot,
                                        const std::string& expectedBaseRevision,
                                        std::string& diagnostic)
{
    const auto compareAndSwap = graphs.publishIfCurrent(
        scope, snapshot, expectedBaseRevision, diagnostic);
    if (compareAndSwap == StoreError::None) {
        return CommitPublication::Published;
    }

    const auto casDiagnostic = diagnostic;
    std::string reconciliationDiagnostic;
    if (graphs.publish(scope, std::move(snapshot), reconciliationDiagnostic) == StoreError::None) {
        diagnostic = "SEVERE graph publication race after CAD commit; committed graph was reconciled "
            "as current after compare-and-swap failure: " + casDiagnostic;
        return CommitPublication::Reconciled;
    }

    diagnostic = "SEVERE graph publication failed after CAD commit; compare-and-swap: "
        + casDiagnostic + "; reconciliation: " + reconciliationDiagnostic;
    graphs.markScopeStale(scope, diagnostic);
    return CommitPublication::Failed;
}

NativeAssemblyOperations::NativeAssemblyOperations(GraphStore& graphs, GraphAuditLog& audit)
    : _graphs(graphs), _audit(audit)
{}

ToolResult NativeAssemblyOperations::execute(const std::string& toolName,
                                             const std::string& argumentsJson) const
{
    ToolResult result;
#ifndef CADX_HAVE_ASSEMBLY
    result = ToolResult::failure("CADX_UNSUPPORTED_OBJECT", "the Assembly module is not built");
#else
        Parsed parsed;
        std::string diagnostic;
        if (!parse(argumentsJson, parsed, diagnostic)) {
            result = invalid(diagnostic);
            return result;
        }
        const auto operation = parsed.object.value("operation").toString().toStdString();
        const auto document = App::GetApplication().getActiveDocument();
        if (!document) {
            result = ToolResult::failure("CADX_NO_ACTIVE_DOCUMENT", "there is no active FreeCAD document");
            return result;
        }
        if (toolName == "assembly.create") {
            if (operation != "create_assembly"
                || !parseCommon(parsed.object, parsed, diagnostic,
                                {"operation", "operation_id", "expected_graph_revision", "label"})) {
                result = invalid(diagnostic.empty() ? "operation must be create_assembly" : diagnostic);
                return result;
            }
            std::string label;
            if (!stringField(parsed.object, "label", label, diagnostic, 1, 160)) {
                result = invalid(diagnostic);
                return result;
            }
            const GraphScope scope {document->Uid.getValueStr(), "Assembly"};
            if (document->getObject("Assembly") || _graphs.current(scope, false)) {
                return ToolResult::failure("CADX_GRAPH_REVISION_MISMATCH",
                                           "assembly.create requires a new Assembly scope",
                                           true);
            }
            DocumentMutationTransaction transaction(document, "CadX assembly.create");
            auto* assemblyObject = dynamic_cast<Assembly::AssemblyObject*>(
                document->addObject("Assembly::AssemblyObject", "Assembly"));
            if (!assemblyObject) {
                result = ToolResult::failure("CADX_INTERNAL_ERROR", "FreeCAD could not create the Assembly object");
                return result;
            }
            assemblyObject->Label.setValue(label.c_str());
            assemblyObject->Type.setValue("Assembly");
            auto* jointGroup = dynamic_cast<Assembly::JointGroup*>(
                assemblyObject->addObject("Assembly::JointGroup", "Joints"));
            if (!jointGroup) {
                result = ToolResult::failure("CADX_INTERNAL_ERROR",
                                             "FreeCAD could not create the Assembly JointGroup");
                return result;
            }
            document->recompute({assemblyObject}, true);
            const auto built = captureNativeAssemblyGraph(document, assemblyObject);
            if (!built) {
                result = ToolResult::failure(built.errorCode, built.diagnostic);
                return result;
            }
            const auto predicted = sha256Revision("create_assembly|" + label);
            const auto observed = sha256Revision(canonicalSemantic(*built.snapshot));
            transaction.commit();

            const auto committedBuilt = captureNativeAssemblyGraph(document, assemblyObject);
            if (!committedBuilt) {
                const auto consistency = "post-commit graph capture failed: " + committedBuilt.diagnostic;
                _graphs.markScopeStale(scope, consistency);
                recordMutationFailure(_audit, "post_commit_capture", toolName, parsed,
                                      parsed.expectedRevision, predicted, observed, "committed",
                                      built.snapshot.get(), committedBuilt.errorCode, consistency);
                result = ToolResult::failure("CADX_GRAPH_CONSISTENCY_FAILURE", consistency, true);
                return result;
            }
            if (!equalCommittedGraph(*built.snapshot, *committedBuilt.snapshot, diagnostic)) {
                _graphs.markScopeStale(scope, diagnostic);
                recordMutationFailure(_audit, "post_commit_verify", toolName, parsed,
                                      parsed.expectedRevision, predicted, observed, "committed",
                                      committedBuilt.snapshot.get(), "CADX_GRAPH_CONSISTENCY_FAILURE",
                                      diagnostic);
                result = ToolResult::failure("CADX_GRAPH_CONSISTENCY_FAILURE", diagnostic, true);
                return result;
            }
            std::string publishDiagnostic;
            const auto publication = publishCommittedGraph(
                _graphs, scope, committedBuilt.snapshot, parsed.expectedRevision, publishDiagnostic);
            if (publication != CommitPublication::Published) {
                recordMutationFailure(_audit, "publish", toolName, parsed,
                                      parsed.expectedRevision, predicted, observed, "committed",
                                      committedBuilt.snapshot.get(), "CADX_GRAPH_CONSISTENCY_FAILURE",
                                      publishDiagnostic);
                result = ToolResult::failure("CADX_GRAPH_CONSISTENCY_FAILURE", publishDiagnostic, true);
                return result;
            }
            result = receipt(toolName, parsed, parsed.expectedRevision,
                             committedBuilt.snapshot->header().graphRevision, predicted, observed,
                             "valid", "committed", committedBuilt.snapshot.get());
            _audit.record(makeMutationAuditEvent("mutation", "passed", toolName, parsed.operationId,
                                                 parsed.expectedRevision,
                                                 committedBuilt.snapshot->header().graphRevision, predicted, observed,
                                                 "valid", "committed", committedBuilt.snapshot.get()));
            return result;
        }

        if (toolName == "assembly.insert") {
            if (operation != "insert_component"
                || !parseCommon(parsed.object, parsed, diagnostic,
                                {"operation", "operation_id", "expected_graph_revision", "assembly", "source", "label", "placement"})) {
                result = invalid(diagnostic.empty() ? "operation must be insert_component" : diagnostic);
                return result;
            }
            std::string assemblyName;
            if (!objectRef(parsed.object.value("assembly"), assemblyName, diagnostic)) {
                result = invalid(diagnostic); return result;
            }
            auto* assembly = dynamic_cast<Assembly::AssemblyObject*>(document->getObject(assemblyName.c_str()));
            if (!assembly) {
                result = ToolResult::failure("CADX_UNSUPPORTED_OBJECT", "assembly reference is not an Assembly object");
                return result;
            }
            const auto source = parsed.object.value("source").toObject();
            if (!closed(source, {"document_name", "object_name"}, diagnostic)) { result = invalid(diagnostic); return result; }
            std::string sourceDocumentName, sourceObjectName;
            if (!stringField(source, "document_name", sourceDocumentName, diagnostic, 1, 256)
                || !stringField(source, "object_name", sourceObjectName, diagnostic, 1, 256)) { result = invalid(diagnostic); return result; }
            auto* sourceDocument = App::GetApplication().getDocument(sourceDocumentName.c_str());
            auto* sourceObject = sourceDocument ? sourceDocument->getObject(sourceObjectName.c_str()) : nullptr;
            if (!sourceObject) { result = ToolResult::failure("CADX_UNRESOLVED_SOURCE", "source object was not found"); return result; }
            GraphScope scope {document->Uid.getValueStr(), assembly->getNameInDocument()};
            Placement requestedPlacement;
            if (parsed.object.contains("placement")
                && !placement(parsed.object.value("placement"), requestedPlacement, diagnostic)) {
                result = invalid(diagnostic);
                return result;
            }
            const bool hasPlacement = parsed.object.contains("placement");
            const NativeIdentity sourceIdentity {
                sourceDocument->Uid.getValueStr(),
                sourceObject->getNameInDocument(),
                std::string(sourceObject->getTypeId().getName().data(),
                            sourceObject->getTypeId().getName().size())};
            std::shared_ptr<const GraphSnapshot> base;
            if (!loadMutationBase(_graphs, document, assembly, parsed.expectedRevision,
                                  base, scope, diagnostic)) {
                return ToolResult::failure("CADX_GRAPH_REVISION_MISMATCH", diagnostic, true);
            }
            DocumentMutationTransaction transaction(document, "CadX assembly.insert");
            const bool sourceIsAssembly = sourceObject->getTypeId().getName()
                == Assembly::AssemblyObject::getClassName();
            const auto linkType = sourceIsAssembly ? "Assembly::AssemblyLink" : "App::Link";
            auto* link = assembly->addObject(linkType, "Component");
            if (!link) { result = ToolResult::failure("CADX_INTERNAL_ERROR", "FreeCAD could not create the link"); return result; }
            if (sourceIsAssembly) {
                auto* assemblyLink = dynamic_cast<Assembly::AssemblyLink*>(link);
                if (!assemblyLink) {
                    result = ToolResult::failure("CADX_INTERNAL_ERROR", "FreeCAD created an invalid Assembly link");
                    return result;
                }
                assemblyLink->LinkedObject.setValue(sourceObject);
            }
            else {
                auto* ordinaryLink = dynamic_cast<App::Link*>(link);
                if (!ordinaryLink) {
                    result = ToolResult::failure("CADX_INTERNAL_ERROR", "FreeCAD created an invalid component link");
                    return result;
                }
                ordinaryLink->LinkedObject.setValue(sourceObject);
            }
            if (parsed.object.contains("label")) {
                std::string label;
                if (!stringField(parsed.object, "label", label, diagnostic, 1, 160)) { result = invalid(diagnostic); return result; }
                link->Label.setValue(label.c_str());
            }
            if (hasPlacement && !applyPlacement(link, requestedPlacement, diagnostic)) {
                result = ToolResult::failure("CADX_PRECONDITION_FAILED", diagnostic);
                return result;
            }
            document->recompute({link, assembly}, true);
            auto* ordinaryLink = dynamic_cast<App::Link*>(link);
            auto* assemblyLink = dynamic_cast<Assembly::AssemblyLink*>(link);
            auto* resolvedLinkTarget = ordinaryLink
                ? ordinaryLink->getTrueLinkedObject(false)
                : (assemblyLink ? assemblyLink->getLinkedObject2(false) : nullptr);
            if (!sourceDocument || sourceDocument->Uid.getValueStr() != sourceIdentity.documentUid
                || sourceDocument->getObject(sourceIdentity.objectName.c_str()) != sourceObject
                || sourceObject->getTypeId().getName() != sourceIdentity.typeId
                || resolvedLinkTarget != sourceObject
                || (hasPlacement && !placementMatches(link, requestedPlacement, diagnostic))) {
                result = ToolResult::failure("CADX_SOURCE_IDENTITY_CHANGED",
                                             diagnostic.empty()
                                                 ? "linked source identity or placement changed during insert"
                                                 : diagnostic,
                                             true);
                return result;
            }
            const auto built = captureNativeAssemblyGraph(document, assembly);
            if (!built) { result = ToolResult::failure(built.errorCode, built.diagnostic); return result; }
            const auto predicted = sha256Revision("insert_component|" + sourceDocumentName + "|" + sourceObjectName);
            const auto observed = sha256Revision(canonicalSemantic(*built.snapshot));
            transaction.commit();

            const auto committedBuilt = captureNativeAssemblyGraph(document, assembly);
            if (!committedBuilt) {
                const auto consistency = "post-commit graph capture failed: " + committedBuilt.diagnostic;
                _graphs.markScopeStale(scope, consistency);
                recordMutationFailure(_audit, "post_commit_capture", toolName, parsed,
                                      parsed.expectedRevision, predicted, observed, "committed",
                                      built.snapshot.get(), committedBuilt.errorCode, consistency);
                result = ToolResult::failure("CADX_GRAPH_CONSISTENCY_FAILURE", consistency, true);
                return result;
            }
            if (!equalCommittedGraph(*built.snapshot, *committedBuilt.snapshot, diagnostic)) {
                _graphs.markScopeStale(scope, diagnostic);
                recordMutationFailure(_audit, "post_commit_verify", toolName, parsed,
                                      parsed.expectedRevision, predicted, observed, "committed",
                                      committedBuilt.snapshot.get(), "CADX_GRAPH_CONSISTENCY_FAILURE",
                                      diagnostic);
                result = ToolResult::failure("CADX_GRAPH_CONSISTENCY_FAILURE", diagnostic, true);
                return result;
            }
            std::string publishDiagnostic;
            const auto publication = publishCommittedGraph(
                _graphs, scope, committedBuilt.snapshot, parsed.expectedRevision, publishDiagnostic);
            if (publication != CommitPublication::Published) {
                recordMutationFailure(_audit, "publish", toolName, parsed,
                                      parsed.expectedRevision, predicted, observed, "committed",
                                      committedBuilt.snapshot.get(), "CADX_GRAPH_CONSISTENCY_FAILURE",
                                      publishDiagnostic);
                result = ToolResult::failure("CADX_GRAPH_CONSISTENCY_FAILURE", publishDiagnostic, true);
                return result;
            }
            result = receipt(toolName, parsed, parsed.expectedRevision,
                             committedBuilt.snapshot->header().graphRevision,
                             predicted, observed, "valid", "committed", committedBuilt.snapshot.get());
            _audit.record(makeMutationAuditEvent("mutation", "passed", toolName, parsed.operationId,
                                                 parsed.expectedRevision,
                                                 committedBuilt.snapshot->header().graphRevision,
                                                 predicted, observed, "valid", "committed",
                                                 committedBuilt.snapshot.get()));
            return result;
        }

        result = ToolResult::failure("CADX_UNSUPPORTED_OBJECT", "mutation operation is not enabled");
#endif
    return result;
}

}  // namespace CadX
