// SPDX-License-Identifier: LGPL-2.1-or-later

#include "NativeMutationSupport.h"

#include "AssemblyGraphBuilder.h"
#include "AssemblyObjectAdapter.h"
#include "GraphRevision.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <App/Application.h>
#include <App/AutoTransaction.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <App/Link.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>

#ifdef CADX_HAVE_ASSEMBLY
#include <Mod/Assembly/App/AssemblyObject.h>
#include <Mod/Assembly/App/AssemblyUtils.h>
#include <Mod/Assembly/App/Groups.h>
#endif

#ifdef CADX_HAVE_PART_DESIGN
#include <Mod/Part/App/BodyBase.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/App/Body.h>
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <exception>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace CadX
{

struct DocumentMutationTransaction::Impl
{
    explicit Impl(App::Document* document, const std::string& label)
        : transaction(document, label)
    {}

    App::AutoTransaction transaction;
    bool closed = false;
};

DocumentMutationTransaction::DocumentMutationTransaction(App::Document* document,
                                                         const std::string& label)
    : _impl(std::make_unique<Impl>(document, label))
{}

DocumentMutationTransaction::~DocumentMutationTransaction()
{
    abort();
}

void DocumentMutationTransaction::commit()
{
    if (_impl && !_impl->closed) {
        _impl->transaction.close(App::TransactionCloseMode::Commit);
        _impl->closed = true;
    }
}

void DocumentMutationTransaction::abort() noexcept
{
    if (!_impl || _impl->closed) {
        return;
    }
    try {
        _impl->transaction.close(App::TransactionCloseMode::Abort);
    }
    catch (...) {
        // There is no safe recovery path from an abort exception at this
        // layer. The caller will report the original mutation failure; the
        // document observer will invalidate any affected graph scope.
    }
    _impl->closed = true;
}

bool DocumentMutationTransaction::closed() const noexcept
{
    return !_impl || _impl->closed;
}

namespace
{

bool sameScope(const GraphScope& left, const GraphScope& right)
{
    return left.documentUid == right.documentUid
        && left.assemblyObjectName == right.assemblyObjectName;
}

}  // namespace

#ifdef CADX_HAVE_ASSEMBLY
namespace
{

NativeIdentity nativeIdentity(App::DocumentObject* object)
{
    if (!object || !object->getDocument()) {
        return {};
    }
    const auto type = object->getTypeId().getName();
    return {object->getDocument()->Uid.getValueStr(),
            object->getNameInDocument(),
            std::string(type.data(), type.size())};
}

NodeId stableNodeId(const std::string& role,
                   const NativeIdentity& identity,
                   const std::vector<std::string>& path = {})
{
    std::string value = "cadx.native-capture.v2|" + role + "|" + identity.canonical();
    for (const auto& item : path) {
        value += "|" + item;
    }
    return "node:" + sha256Revision(value);
}

EdgeId stableEdgeId(EdgeKind kind,
                   const NodeId& from,
                   const NodeId& to,
                   const std::string& relation = {})
{
    return "edge:" + sha256Revision("cadx.native-capture.edge.v2|"
                                     + std::string(edgeKindName(kind)) + "|" + from + "|"
                                     + to + "|" + relation);
}

std::string typeName(App::DocumentObject* object)
{
    if (!object) {
        return {};
    }
    const auto type = object->getTypeId().getName();
    return {type.data(), type.size()};
}

Placement objectPlacement(App::DocumentObject* object)
{
    Placement result;
    if (!object) {
        return result;
    }
    if (auto* geo = dynamic_cast<App::GeoFeature*>(object)) {
        const auto global = App::GeoFeature::getGlobalPlacement(geo);
        const auto position = global.getPosition();
        double qx = 0.0;
        double qy = 0.0;
        double qz = 0.0;
        double qw = 1.0;
        global.getRotation().getValue(qx, qy, qz, qw);
        result.x = position.x;
        result.y = position.y;
        result.z = position.z;
        result.qx = qx;
        result.qy = qy;
        result.qz = qz;
        result.qw = qw;
        result.normalize();
        return result;
    }
    if (auto* property = dynamic_cast<App::PropertyPlacement*>(
            object->getPropertyByName("Placement"))) {
        const auto value = property->getValue();
        const auto position = value.getPosition();
        double qx = 0.0;
        double qy = 0.0;
        double qz = 0.0;
        double qw = 1.0;
        value.getRotation().getValue(qx, qy, qz, qw);
        result.x = position.x;
        result.y = position.y;
        result.z = position.z;
        result.qx = qx;
        result.qy = qy;
        result.qz = qz;
        result.qw = qw;
    }
    result.normalize();
    return result;
}

bool propertyNumber(App::DocumentObject* object, const char* name, double& result)
{
    if (!object) {
        return false;
    }
    auto* property = object->getPropertyByName(name);
    if (auto* quantity = dynamic_cast<App::PropertyQuantity*>(property)) {
        result = quantity->getValue();
        return std::isfinite(result);
    }
    if (auto* floating = dynamic_cast<App::PropertyFloat*>(property)) {
        result = floating->getValue();
        return std::isfinite(result);
    }
    return false;
}

void appendPrimitivePayload(NodeRecord& node, App::DocumentObject* object)
{
    const auto type = typeName(object);
    PrimitivePayload payload;
    if (type == "Part::Box" || type == "PartDesign::Box") {
        payload.primitiveKind = "box";
        propertyNumber(object, "Length", payload.length);
        propertyNumber(object, "Width", payload.width);
        propertyNumber(object, "Height", payload.height);
        node.payload = payload;
    }
    else if (type == "Part::Cylinder" || type == "PartDesign::Cylinder") {
        payload.primitiveKind = "cylinder";
        propertyNumber(object, "Radius", payload.radius);
        propertyNumber(object, "Height", payload.height);
        propertyNumber(object, "Angle", payload.sweepDegrees);
        node.payload = payload;
    }
}

bool isElementConnector(const std::string& value)
{
    const std::array<std::string, 3> prefixes {"Face", "Edge", "Vertex"};
    for (const auto& prefix : prefixes) {
        if (value.rfind(prefix, 0) != 0 || value.size() == prefix.size()) {
            continue;
        }
        const auto suffix = value.substr(prefix.size());
        if (!std::all_of(suffix.begin(), suffix.end(), [](unsigned char character) {
                return std::isdigit(character) != 0;
            })) {
            return false;
        }
        try {
            return std::stoull(suffix) > 0;
        }
        catch (...) {
            return false;
        }
    }
    return false;
}

JointConnectorPayload connectorPayload(App::DocumentObject* joint,
                                       const char* referenceName,
                                       App::DocumentObject* target)
{
    JointConnectorPayload result;
    result.componentObject = target ? target->getNameInDocument() : std::string {};
    if (auto* property = dynamic_cast<App::PropertyXLinkSub*>(
            joint->getPropertyByName(referenceName))) {
        const auto subValues = property->getSubValues();
        if (subValues.size() == 1) {
            result.connector = subValues.front();
            result.connectorType = isElementConnector(result.connector) ? "element" : "interface";
        }
    }
    const char* offsetName = std::string(referenceName) == "Reference1" ? "Offset1" : "Offset2";
    if (auto* property = dynamic_cast<App::PropertyPlacement*>(
            joint->getPropertyByName(offsetName))) {
        const auto value = property->getValue();
        const auto position = value.getPosition();
        double qx = 0.0;
        double qy = 0.0;
        double qz = 0.0;
        double qw = 1.0;
        value.getRotation().getValue(qx, qy, qz, qw);
        result.offset = {position.x, position.y, position.z, qx, qy, qz, qw};
        result.offset.normalize();
        result.hasOffset = true;
    }
    return result;
}

class NativeCaptureBuilder
{
public:
    NativeCaptureBuilder(App::Document* document, Assembly::AssemblyObject* assembly)
        : document(document), assembly(assembly)
    {
        capture.documentUid = document->Uid.getValueStr();
        capture.documentName = document->getName();
        capture.activeAssemblyObjectName = assembly->getNameInDocument();
        capture.activeAssemblyLabel = assembly->Label.getValue();
    }

    AssemblyCapture build()
    {
        appendDocument(document);
        const auto assemblyNative = nativeIdentity(assembly);
        NodeRecord assemblyNode;
        assemblyNode.id = stableNodeId("assembly-definition", assemblyNative);
        assemblyNode.kind = NodeKind::AssemblyDefinition;
        assemblyNode.native = assemblyNative;
        assemblyNode.display = {assembly->Label.getValue(), assembly->Label.getValue()};
        assemblyNode.localPlacement = objectPlacement(assembly);
        assemblyNode.worldPlacement = assemblyNode.localPlacement;
        assemblyNode.payload = DefinitionPayload {
            "definition", "assembly", "compound", "local_parametric", "assembly"};
        capture.activeAssemblyNodeId = assemblyNode.id;
        capture.nodes.push_back(assemblyNode);
        appendEdge(EdgeKind::SourceDocument, assemblyNode.id, documentIds.at(document->Uid.getValueStr()));

        for (auto* child : assembly->Group.getValues()) {
            appendAssemblyChild(child);
        }
        appendDocumentDesignObjects();
        appendJoints();
        appendGroundedJoints();
        return std::move(capture);
    }

private:
    void appendDocument(App::Document* sourceDocument)
    {
        if (!sourceDocument) {
            return;
        }
        const auto uid = sourceDocument->Uid.getValueStr();
        if (documentIds.contains(uid)) {
            return;
        }
        const auto identity = NativeIdentity {uid, sourceDocument->getName(), "App::Document"};
        NodeRecord node;
        node.id = stableNodeId("document", identity);
        node.kind = NodeKind::Document;
        node.native = identity;
        node.display = {sourceDocument->Label.getValue(), sourceDocument->Label.getValue()};
        node.localPlacement.normalize();
        node.worldPlacement.normalize();
        documentIds.emplace(uid, node.id);
        capture.nodes.push_back(node);
    }

    NodeId appendDefinition(App::DocumentObject* object,
                            AdapterClassification classification,
                            bool unresolved = false,
                            const std::string& diagnostic = {})
    {
        if (!object || !object->getDocument()) {
            return {};
        }
        const auto identity = nativeIdentity(object);
        const auto existing = definitionIds.find(identity.canonical());
        if (existing != definitionIds.end()) {
            return existing->second;
        }
        appendDocument(object->getDocument());
        NodeRecord node;
        node.id = stableNodeId("definition", identity);
        node.kind = unresolved ? NodeKind::UnresolvedDefinition : classification.nodeKind;
        if (node.kind == NodeKind::Occurrence) {
            node.kind = NodeKind::FeatureDefinition;
        }
        node.native = identity;
        node.display = {object->Label.getValue(), object->Label.getValue()};
        node.provenance.kind = classification.provenanceKind;
        node.provenance.evidence = diagnostic.empty() ? classification.diagnostic : diagnostic;
        node.localPlacement = objectPlacement(object);
        node.worldPlacement = node.localPlacement;
        node.unresolved = unresolved;
        if (unresolved) {
            node.payload = UnresolvedPayload {identity.documentUid, identity.objectName,
                                              diagnostic.empty() ? "unresolved source" : diagnostic};
        }
        else if (classification.nodeKind == NodeKind::AssemblyArtifact) {
            node.payload = ArtifactPayload {classification.containerKind};
        }
        else {
            node.payload = DefinitionPayload {classification.role,
                                               classification.containerKind,
                                               classification.geometryKind,
                                               classification.provenanceKind,
                                               classification.nodeKind == NodeKind::BodyDefinition
                                                   ? "body"
                                                   : "unknown"};
            appendPrimitivePayload(node, object);
        }
        definitionIds.emplace(identity.canonical(), node.id);
        capture.nodes.push_back(node);
        appendEdge(EdgeKind::SourceDocument, node.id, documentIds.at(identity.documentUid));
        if (!node.provenance.evidence.empty()) {
            capture.diagnostics.push_back(identity.objectName + ": " + node.provenance.evidence);
        }
        return node.id;
    }

    void appendAssemblyChild(App::DocumentObject* child)
    {
        if (!child || !document->containsObject(child)) {
            capture.diagnostics.push_back("Assembly child disappeared during capture");
            return;
        }
        const auto childType = typeName(child);
        auto classification = AssemblyObjectAdapter::classify(childType);
        App::DocumentObject* definition = child;
        bool unresolved = false;
        std::string diagnostic = classification.diagnostic;
        if (child->isDerivedFrom<App::Link>()) {
            auto* link = static_cast<App::Link*>(child);
            definition = link->getTrueLinkedObject(false);
            if (!definition) {
                unresolved = true;
                diagnostic = "App::Link has no resolvable LinkedObject";
                definition = child;
                classification = {};
            }
            else {
                classification = AssemblyObjectAdapter::classify(typeName(definition));
            }
        }
        const auto definitionId = appendDefinition(definition, classification, unresolved, diagnostic);
        if (classification.nodeKind == NodeKind::AssemblyArtifact
            || classification.nodeKind == NodeKind::OrganizationalGroup) {
            appendEdge(EdgeKind::Contains, capture.activeAssemblyNodeId, definitionId);
            return;
        }

        const auto identity = nativeIdentity(child);
        const std::vector<std::string> path {assembly->getNameInDocument(), child->getNameInDocument()};
        NodeRecord occurrence;
        occurrence.id = stableNodeId("occurrence", identity, path);
        occurrence.kind = typeName(child) == "Assembly::AssemblyLink"
            ? NodeKind::AssemblyOccurrence
            : NodeKind::Occurrence;
        occurrence.native = identity;
        occurrence.display = {child->Label.getValue(), child->Label.getValue()};
        occurrence.localPlacement = objectPlacement(child);
        occurrence.worldPlacement = occurrence.localPlacement;
        occurrence.unresolved = unresolved;
        OccurrencePayload payload;
        payload.occurrencePath = path;
        if (auto* rigid = dynamic_cast<App::PropertyBool*>(child->getPropertyByName("Rigid"))) {
            payload.rigid = rigid->getValue();
            payload.flexible = !payload.rigid;
        }
        occurrence.payload = payload;
        occurrenceIds[child->getNameInDocument()] = occurrence.id;
        capture.nodes.push_back(occurrence);
        appendEdge(EdgeKind::Contains, capture.activeAssemblyNodeId, occurrence.id);
        appendEdge(unresolved ? EdgeKind::UnresolvedSource : EdgeKind::InstanceOf,
                   occurrence.id,
                   definitionId);
    }

    void appendDocumentDesignObjects()
    {
#ifdef CADX_HAVE_PART_DESIGN
        for (auto* object : document->getObjects()) {
            if (!object) {
                continue;
            }
            const auto type = typeName(object);
            if (type == "PartDesign::Body") {
                const auto bodyId = appendDefinition(
                    object,
                    AssemblyObjectAdapter::classify(type));
                for (auto* feature : static_cast<PartDesign::Body*>(object)->Group.getValues()) {
                    const auto featureId = appendDefinition(
                        feature,
                        AssemblyObjectAdapter::classify(typeName(feature)));
                    if (!featureId.empty()) {
                        appendEdge(EdgeKind::HasFeature, bodyId, featureId);
                    }
                }
            }
        }
#endif
    }

    void appendJoints()
    {
        for (auto* joint : assembly->getJoints(false, false)) {
            if (!joint) {
                continue;
            }
            const auto identity = nativeIdentity(joint);
            NodeRecord node;
            node.id = stableNodeId("joint", identity);
            node.kind = NodeKind::Joint;
            node.native = identity;
            node.display = {joint->Label.getValue(), joint->Label.getValue()};
            node.localPlacement = objectPlacement(joint);
            node.worldPlacement = node.localPlacement;
            JointPayload payload;
            if (auto* property = dynamic_cast<App::PropertyEnumeration*>(
                    joint->getPropertyByName("JointType"))) {
                payload.jointType = property->getValueAsString();
            }
            if (auto* property = dynamic_cast<App::PropertyBool*>(
                    joint->getPropertyByName("Reversed"))) {
                payload.reverse = property->getValue();
            }
            if (auto* enabled = dynamic_cast<App::PropertyBool*>(
                    joint->getPropertyByName("EnableAngleMin"))) {
                payload.hasLimits = enabled->getValue();
            }
            if (auto* minimum = dynamic_cast<App::PropertyQuantity*>(
                    joint->getPropertyByName("AngleMin"))) {
                payload.minDegrees = minimum->getValue();
            }
            if (auto* maximum = dynamic_cast<App::PropertyQuantity*>(
                    joint->getPropertyByName("AngleMax"))) {
                payload.maxDegrees = maximum->getValue();
            }
            for (const char* referenceName : {"Reference1", "Reference2"}) {
                auto* property = dynamic_cast<App::PropertyXLinkSub*>(
                    joint->getPropertyByName(referenceName));
                auto* target = property ? Assembly::getObjFromRef(property) : nullptr;
                const auto connector = connectorPayload(joint, referenceName, target);
                if (std::string(referenceName) == "Reference1") {
                    payload.first = connector;
                }
                else {
                    payload.second = connector;
                }
                if (!target) {
                    capture.diagnostics.push_back(
                        std::string(joint->getNameInDocument()) + ": unresolved " + referenceName);
                    continue;
                }
                const auto found = occurrenceIds.find(target->getNameInDocument());
                if (found != occurrenceIds.end() && !connector.connector.empty()) {
                    appendEdge(EdgeKind::JointEndpoint,
                               node.id,
                               found->second,
                               referenceName + std::string("|") + connector.connectorType + "|"
                                   + connector.connector);
                }
            }
            node.payload = payload;
            capture.nodes.push_back(node);
        }
    }

    void appendGroundedJoints()
    {
        for (auto* joint : assembly->getGroundedJoints()) {
            if (!joint) {
                continue;
            }
            auto* property = dynamic_cast<App::PropertyLink*>(
                joint->getPropertyByName("ObjectToGround"));
            auto* target = property ? property->getValue() : nullptr;
            if (!target) {
                continue;
            }
            const auto occurrence = occurrenceIds.find(target->getNameInDocument());
            if (occurrence == occurrenceIds.end()) {
                continue;
            }
            const auto identity = nativeIdentity(joint);
            NodeRecord node;
            node.id = stableNodeId("ground", identity);
            node.kind = NodeKind::GroundConstraint;
            node.native = identity;
            node.display = {joint->Label.getValue(), joint->Label.getValue()};
            node.localPlacement = objectPlacement(joint);
            node.worldPlacement = node.localPlacement;
            node.payload = GroundConstraintPayload {
                true, target->getNameInDocument(), joint->getNameInDocument()};
            capture.nodes.push_back(node);
            appendEdge(EdgeKind::GroundedBy, node.id, occurrence->second);
        }
    }

    void appendEdge(EdgeKind kind,
                    const NodeId& from,
                    const NodeId& to,
                    const std::string& relation = {})
    {
        capture.edges.push_back({stableEdgeId(kind, from, to, relation),
                                 kind,
                                 from,
                                 to,
                                 {},
                                 relation});
    }

    App::Document* document;
    Assembly::AssemblyObject* assembly;
    AssemblyCapture capture;
    std::unordered_map<std::string, NodeId> documentIds;
    std::unordered_map<std::string, NodeId> definitionIds;
    std::unordered_map<std::string, NodeId> occurrenceIds;
};

}  // namespace

NativeGraphCaptureResult captureNativeAssemblyGraph(App::Document* document,
                                                    Assembly::AssemblyObject* assembly)
{
    if (!document || !assembly || assembly->getDocument() != document) {
        return {{}, nullptr, "CADX_INVALID_ASSEMBLY", "the Assembly does not belong to the document"};
    }
    const auto captured = captureNativeAssemblyCapture(document, assembly);
    if (!captured) {
        return {{captured.capture.documentUid, captured.capture.activeAssemblyObjectName},
                nullptr,
                captured.errorCode,
                captured.diagnostic};
    }
    auto capture = captured.capture;
    const auto built = AssemblyGraphBuilder().build(capture);
    if (!built) {
        return {{capture.documentUid, capture.activeAssemblyObjectName},
                nullptr,
                built.errorCode,
                built.diagnostic};
    }
    return {{capture.documentUid, capture.activeAssemblyObjectName}, built.snapshot, {}, {}};
}

NativeAssemblyCaptureResult captureNativeAssemblyCapture(App::Document* document,
                                                         Assembly::AssemblyObject* assembly)
{
    if (!document || !assembly || assembly->getDocument() != document) {
        return {{}, "CADX_INVALID_ASSEMBLY", "the Assembly does not belong to the document"};
    }
    // Capture is read-only and is also used to form the candidate graph while
    // the owning document transaction is still open.  The caller controls the
    // transaction boundary and publishes only after a post-commit recapture.
    return {NativeCaptureBuilder(document, assembly).build(), {}, {}};
}

NativeGraphCaptureResult captureNativeAssemblyGraph(App::Document* document,
                                                    const std::string& assemblyObjectName)
{
    auto* object = document ? document->getObject(assemblyObjectName.c_str()) : nullptr;
    auto* assembly = dynamic_cast<Assembly::AssemblyObject*>(object);
    return captureNativeAssemblyGraph(document, assembly);
}
#endif

bool equivalentGraphState(const GraphSnapshot& left,
                          const GraphSnapshot& right,
                          std::string& diagnostic)
{
    if (left.header().graphId != right.header().graphId
        || left.header().documentUid != right.header().documentUid
        || left.header().activeAssemblyObjectName != right.header().activeAssemblyObjectName
        || canonicalSemantic(left) != canonicalSemantic(right)
        || canonicalPresentation(left) != canonicalPresentation(right)) {
        diagnostic = "CAD graph recapture does not equal the candidate graph";
        return false;
    }
    return true;
}

bool loadMutationBase(GraphStore& graphs,
                      App::Document* document,
                      Assembly::AssemblyObject* assembly,
                      const std::string& expectedRevision,
                      std::shared_ptr<const GraphSnapshot>& base,
                      GraphScope& scope,
                      std::string& diagnostic)
{
#ifndef CADX_HAVE_ASSEMBLY
    (void)graphs;
    (void)document;
    (void)assembly;
    (void)expectedRevision;
    (void)base;
    (void)scope;
    diagnostic = "the Assembly module is not built";
    return false;
#else
    if (!document || !assembly) {
        diagnostic = "an exact Assembly document scope is required";
        return false;
    }
    scope = {document->Uid.getValueStr(), assembly->getNameInDocument()};
    const auto current = graphs.current(scope, false);
    if (expectedRevision.empty()) {
        diagnostic = "a current graph revision is required for a mutation";
        return false;
    }
    if (!current || current.snapshot->header().graphRevision != expectedRevision) {
        diagnostic = current.diagnostic.empty()
            ? "the expected graph revision is stale or unavailable"
            : current.diagnostic;
        return false;
    }
    const auto recaptured = captureNativeAssemblyGraph(document, assembly);
    if (!recaptured) {
        diagnostic = recaptured.diagnostic;
        return false;
    }
    if (!equivalentGraphState(*current.snapshot, *recaptured.snapshot, diagnostic)) {
        diagnostic = "FreeCAD changed outside the graph revision: " + diagnostic;
        return false;
    }
    base = current.snapshot;
    return true;
#endif
}

std::string mutationDeltaHash(const GraphSnapshot* parent,
                              const GraphSnapshot& candidate)
{
    const auto before = parent ? canonicalSemantic(*parent) : std::string("<empty>");
    return sha256Revision("cadx.mutation-delta.v1|" + before + "|" + canonicalSemantic(candidate));
}

ToolResult mutationReceipt(const std::string& schemaVersion,
                           const std::string& operation,
                           const std::string& operationId,
                           const std::string& parentRevision,
                           const GraphSnapshot& finalSnapshot,
                           const std::string& predictedDeltaHash,
                           const std::string& observedDeltaHash,
                           bool changed)
{
    const QJsonObject output {
        {"schema_version", QString::fromStdString(schemaVersion)},
        {"ok", true},
        {"operation", QString::fromStdString(operation)},
        {"operation_id", QString::fromStdString(operationId)},
        {"parent_revision", QString::fromStdString(parentRevision)},
        {"final_revision", QString::fromStdString(finalSnapshot.header().graphRevision)},
        {"graph_id", QString::fromStdString(finalSnapshot.header().graphId)},
        {"predicted_delta_hash", QString::fromStdString(predictedDeltaHash)},
        {"observed_delta_hash", QString::fromStdString(observedDeltaHash)},
        {"physical_verdict", "valid"},
        {"transaction_status", "committed"},
        {"changed", changed},
    };
    return ToolResult::success(schemaVersion,
                               QJsonDocument(output).toJson(QJsonDocument::Compact).toStdString());
}

}  // namespace CadX
