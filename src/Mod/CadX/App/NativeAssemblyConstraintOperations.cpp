// SPDX-License-Identifier: LGPL-2.1-or-later

#include "NativeAssemblyConstraintOperations.h"
#include "NativeAssemblyOperations.h"

#include "GraphRevision.h"
#include "GraphSnapshot.h"
#include "NativeMutationSupport.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <initializer_list>
#include <numeric>
#include <set>
#include <sstream>
#include <unordered_set>

#ifdef CADX_HAVE_ASSEMBLY
#include <CXX/Objects.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <Base/Interpreter.h>
#include <Base/Placement.h>

#include <Mod/Assembly/App/AssemblyObject.h>
#include <Mod/Assembly/App/AssemblyUtils.h>
#include <Mod/Assembly/App/Groups.h>
#endif

namespace CadX
{
namespace
{
bool closed(const QJsonObject& object,
            std::initializer_list<const char*> names,
            std::string& diagnostic)
{
    std::set<std::string> allowed;
    for (const auto* name : names) {
        allowed.emplace(name);
    }
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!allowed.contains(it.key().toStdString())) {
            diagnostic = "unknown field '" + it.key().toStdString() + "'";
            return false;
        }
    }
    return true;
}

bool stringValue(const QJsonObject& object,
                 const char* name,
                 std::string& value,
                 std::string& diagnostic,
                 std::size_t minimum,
                 std::size_t maximum)
{
    const auto item = object.value(name);
    if (!item.isString()) {
        diagnostic = std::string("field '") + name + "' must be a string";
        return false;
    }
    value = item.toString().toStdString();
    if (value.size() < minimum || value.size() > maximum) {
        diagnostic = std::string("field '") + name + "' has an invalid length";
        return false;
    }
    return true;
}

bool objectName(const QJsonValue& value, std::string& result, std::string& diagnostic)
{
    if (!value.isObject()) {
        diagnostic = "object reference must be an object";
        return false;
    }
    const auto object = value.toObject();
    if (!closed(object, {"object_name"}, diagnostic)
        || !stringValue(object, "object_name", result, diagnostic, 1, 128)) {
        return false;
    }
    if ((!std::isalpha(static_cast<unsigned char>(result.front())) && result.front() != '_')
        || !std::all_of(result.begin() + 1, result.end(), [](unsigned char c) {
               return std::isalnum(c) || c == '_';
           })) {
        diagnostic = "object_name must be a FreeCAD identifier";
        return false;
    }
    return true;
}

bool finiteNumber(const QJsonValue& value,
                  double& result,
                  std::string& diagnostic,
                  double minimum,
                  double maximum)
{
    if (!value.isDouble() || !std::isfinite(value.toDouble())) {
        diagnostic = "number must be finite";
        return false;
    }
    result = value.toDouble();
    if (result < minimum || result > maximum) {
        diagnostic = "number is outside its allowed bounds";
        return false;
    }
    return true;
}

bool vector3(const QJsonValue& value,
             double result[3],
             std::string& diagnostic,
             double minimum,
             double maximum)
{
    if (!value.isArray() || value.toArray().size() != 3) {
        diagnostic = "vector must contain exactly three numbers";
        return false;
    }
    const auto array = value.toArray();
    for (int i = 0; i != 3; ++i) {
        if (!finiteNumber(array.at(i), result[i], diagnostic, minimum, maximum)) {
            return false;
        }
    }
    return true;
}

bool parseOffset(const QJsonValue& value, Placement& result, std::string& diagnostic)
{
    if (value.isArray()) {
        diagnostic = "offset must be an object with translation_mm, rotation_axis, and rotation_degrees";
        return false;
    }
    if (!value.isObject()) {
        diagnostic = "offset must be an object";
        return false;
    }
    const auto object = value.toObject();
    if (!closed(object, {"translation_mm", "rotation_axis", "rotation_degrees"}, diagnostic)
        || !object.contains("translation_mm") || !object.contains("rotation_axis")
        || !object.contains("rotation_degrees")) {
        if (diagnostic.empty()) {
            diagnostic = "offset requires translation_mm, rotation_axis, and rotation_degrees";
        }
        return false;
    }
    double translation[3] {};
    double axis[3] {};
    double degrees = 0.0;
    if (!vector3(object.value("translation_mm"), translation, diagnostic, -1'000'000.0, 1'000'000.0)
        || !vector3(object.value("rotation_axis"), axis, diagnostic, -1.0, 1.0)
        || !finiteNumber(object.value("rotation_degrees"), degrees, diagnostic, -360.0, 360.0)) {
        return false;
    }
    const double axisLength = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    if (axisLength <= 1.0e-12 && std::abs(degrees) > 1.0e-12) {
        diagnostic = "a non-zero rotation requires a non-zero axis";
        return false;
    }
    constexpr double pi = 3.14159265358979323846;
    const double halfAngle = degrees * pi / 360.0;
    const double scale = axisLength <= 1.0e-12 ? 0.0 : std::sin(halfAngle) / axisLength;
    result.x = translation[0];
    result.y = translation[1];
    result.z = translation[2];
    result.qx = axis[0] * scale;
    result.qy = axis[1] * scale;
    result.qz = axis[2] * scale;
    result.qw = std::cos(halfAngle);
    return result.normalize();
}

bool elementConnector(const std::string& value)
{
    for (const auto& prefix : {std::string("Face"), std::string("Edge"), std::string("Vertex")}) {
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

bool parseCommon(const QJsonObject& object,
                 const std::string& operation,
                 std::string& operationId,
                 std::string& expectedRevision,
                 std::string& diagnostic)
{
    std::string actual;
    if (!stringValue(object, "operation", actual, diagnostic, 1, 64) || actual != operation
        || !stringValue(object, "operation_id", operationId, diagnostic, 1, 128)
        || !stringValue(object, "expected_graph_revision", expectedRevision, diagnostic, 0, 128)) {
        if (diagnostic.empty()) {
            diagnostic = "operation must be " + operation;
        }
        return false;
    }
    return true;
}

bool parseRoot(const std::string& json, QJsonObject& object, std::string& diagnostic)
{
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(QByteArray::fromStdString(json), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        diagnostic = "arguments must be a JSON object";
        return false;
    }
    object = document.object();
    return true;
}

bool parseConnector(const QJsonValue& value,
                    ConstraintConnectorRequest& result,
                    std::string& diagnostic)
{
    if (!value.isObject()) {
        diagnostic = "connector must be an object";
        return false;
    }
    const auto object = value.toObject();
    if (!closed(object, {"component", "connector_type", "connector", "offset"}, diagnostic)
        || !stringValue(object, "component", result.component, diagnostic, 1, 128)
        || !stringValue(object, "connector_type", result.connectorType, diagnostic, 1, 16)
        || !stringValue(object, "connector", result.connector, diagnostic, 1, 512)) {
        return false;
    }
    if (result.connectorType != "element" && result.connectorType != "interface") {
        diagnostic = "connector_type must be element or interface";
        return false;
    }
    if (result.connectorType == "element" && !elementConnector(result.connector)) {
        diagnostic = "element connector must be FaceN, EdgeN, or VertexN with N greater than zero";
        return false;
    }
    if (result.connectorType == "interface" && result.connector.size() > 64) {
        diagnostic = "interface connector exceeds 64 characters";
        return false;
    }
    if (result.connectorType == "interface"
        && !std::isalpha(static_cast<unsigned char>(result.connector.front()))) {
        diagnostic = "interface connector must start with a letter";
        return false;
    }
    if (result.connectorType == "interface"
        && !std::all_of(result.connector.begin(), result.connector.end(), [](unsigned char c) {
               return std::isalnum(c) || c == '_';
           })) {
        diagnostic = "interface connector must be alphanumeric or underscore";
        return false;
    }
    if (object.contains("offset")) {
        result.hasOffset = true;
        if (!parseOffset(object.value("offset"), result.offset, diagnostic)) {
            return false;
        }
    }
    return true;
}

std::string receiptJson(const std::string& operation,
                        const std::string& operationId,
                        const std::string& parent,
                        const std::string& final,
                        const std::string& predicted,
                        const std::string& observed,
                        const std::string& transaction,
                        const std::string& physical,
                        bool changed)
{
    const QJsonObject object {
        {"schema_version", "cadx.assembly-constraint-result.v1"},
        {"ok", true},
        {"operation", QString::fromStdString(operation)},
        {"operation_id", QString::fromStdString(operationId)},
        {"parent_revision", QString::fromStdString(parent)},
        {"final_revision", QString::fromStdString(final)},
        {"predicted_delta_hash", QString::fromStdString(predicted)},
        {"observed_delta_hash", QString::fromStdString(observed)},
        {"transaction_status", QString::fromStdString(transaction)},
        {"physical_verdict", QString::fromStdString(physical)},
        {"changed", changed},
    };
    return QJsonDocument(object).toJson(QJsonDocument::Compact).toStdString();
}

#ifdef CADX_HAVE_ASSEMBLY
class PythonAssemblyConstraintBridge final : public AssemblyConstraintBridge
{
public:
    bool createGroundedJoint(const std::string& documentName,
                             const std::string& assemblyName,
                             const std::string& componentName,
                             std::string& createdJointName,
                             std::string& diagnostic) const override
    {
        return callString("create_grounded_joint",
                          {documentName, assemblyName, componentName},
                          createdJointName,
                          diagnostic);
    }

    bool createRegularJoint(const std::string& documentName,
                            const std::string& assemblyName,
                            const std::string& jointType,
                            const std::string& label,
                            const ConstraintConnectorRequest& first,
                            const ConstraintConnectorRequest& second,
                            bool reverse,
                            std::string& createdJointName,
                            std::string& diagnostic) const override
    {
        Base::PyGILStateLocker lock;
        try {
            Py::Module module(PyImport_ImportModule("CadXNativeAssemblyBridge"), true);
            Py::Callable callable(module.getAttr("create_regular_joint"));
            Py::Tuple args(9);
            args.setItem(0, Py::String(documentName));
            args.setItem(1, Py::String(assemblyName));
            args.setItem(2, Py::String(jointType));
            args.setItem(3, Py::String(label));
            args.setItem(4, Py::String(first.component));
            args.setItem(5, Py::String(first.connector));
            args.setItem(6, Py::String(second.component));
            args.setItem(7, Py::String(second.connector));
            args.setItem(8, Py::Boolean(reverse));
            const Py::Object result = Base::pyCall(callable.ptr(), args.ptr());
            createdJointName = static_cast<std::string>(Py::String(result));
            return !createdJointName.empty();
        }
        catch (...) {
            PyErr_Clear();
            diagnostic = "CadXNativeAssemblyBridge.create_regular_joint failed";
            return false;
        }
    }

    bool verifyGroundedProxy(const std::string& documentName,
                             const std::string& jointName,
                             std::string& diagnostic) const override
    {
        return callBool("verify_grounded_proxy", {documentName, jointName}, diagnostic);
    }

    bool verifyRegularProxy(const std::string& documentName,
                            const std::string& jointName,
                            std::string& diagnostic) const override
    {
        return callBool("verify_regular_proxy", {documentName, jointName}, diagnostic);
    }

private:
    bool callString(const char* function,
                    const std::vector<std::string>& values,
                    std::string& result,
                    std::string& diagnostic) const
    {
        Base::PyGILStateLocker lock;
        try {
            Py::Module module(PyImport_ImportModule("CadXNativeAssemblyBridge"), true);
            Py::Callable callable(module.getAttr(function));
            Py::Tuple args(values.size());
            for (std::size_t i = 0; i < values.size(); ++i) {
                args.setItem(i, Py::String(values[i]));
            }
            const Py::Object value = Base::pyCall(callable.ptr(), args.ptr());
            result = static_cast<std::string>(Py::String(value));
            return !result.empty();
        }
        catch (...) {
            PyErr_Clear();
            diagnostic = std::string("CadXNativeAssemblyBridge.") + function + " failed";
            return false;
        }
    }

    bool callBool(const char* function,
                  const std::vector<std::string>& values,
                  std::string& diagnostic) const
    {
        Base::PyGILStateLocker lock;
        try {
            Py::Module module(PyImport_ImportModule("CadXNativeAssemblyBridge"), true);
            Py::Callable callable(module.getAttr(function));
            Py::Tuple args(values.size());
            for (std::size_t i = 0; i < values.size(); ++i) {
                args.setItem(i, Py::String(values[i]));
            }
            const Py::Object value = Base::pyCall(callable.ptr(), args.ptr());
            if (!PyBool_Check(value.ptr())) {
                diagnostic = std::string("CadXNativeAssemblyBridge.") + function
                    + " did not return bool";
                return false;
            }
            if (!value.isTrue()) {
                diagnostic = std::string("CadXNativeAssemblyBridge.") + function
                    + " rejected the object";
                return false;
            }
            return true;
        }
        catch (...) {
            PyErr_Clear();
            diagnostic = std::string("CadXNativeAssemblyBridge.") + function + " failed";
            return false;
        }
    }
};

NativeIdentity identity(App::DocumentObject* object)
{
    const auto type = object->getTypeId().getName();
    return {object->getDocument()->Uid.getValueStr(), object->getNameInDocument(),
            std::string(type.data(), type.size())};
}

NodeId stableNodeId(const char* role, App::DocumentObject* object)
{
    return "node:" + sha256Revision(std::string("cadx.constraint.v1|") + role + "|"
                                     + identity(object).canonical());
}

EdgeId stableEdgeId(EdgeKind kind, const NodeId& from, const NodeId& to)
{
    return "edge:" + sha256Revision(std::string(edgeKindName(kind)) + "|" + from + "|" + to);
}

App::DocumentObject* componentInAssembly(Assembly::AssemblyObject* assembly,
                                         const std::string& name)
{
    for (auto* component : Assembly::getAssemblyComponents(assembly)) {
        if (component && component->getNameInDocument() == name) {
            return component;
        }
    }
    return nullptr;
}

App::DocumentObject* groundedTarget(App::DocumentObject* joint)
{
    auto* property = dynamic_cast<App::PropertyLink*>(joint->getPropertyByName("ObjectToGround"));
    return property ? property->getValue() : nullptr;
}

App::DocumentObject* groundedJointFor(Assembly::AssemblyObject* assembly,
                                     App::DocumentObject* component,
                                     std::size_t& count)
{
    App::DocumentObject* result = nullptr;
    count = 0;
    for (auto* joint : assembly->getGroundedJoints()) {
        if (groundedTarget(joint) == component) {
            result = joint;
            ++count;
        }
    }
    return result;
}

bool placementReadOnly(App::DocumentObject* component)
{
    bool found = false;
    for (const char* name : {"Placement", "LinkPlacement"}) {
        if (auto* property = component->getPropertyByName(name)) {
            found = true;
            if (!property->isReadOnly()) {
                return false;
            }
        }
    }
    return found;
}

bool connectorMatches(App::DocumentObject* joint,
                      const char* referenceName,
                      App::DocumentObject* component,
                      const ConstraintConnectorRequest& expected,
                      std::string& diagnostic)
{
    auto* property = dynamic_cast<App::PropertyXLinkSub*>(
        joint ? joint->getPropertyByName(referenceName) : nullptr);
    if (!property || property->getValue() != component) {
        diagnostic = std::string(referenceName) + " does not reference the requested component";
        return false;
    }
    const auto subValues = property->getSubValues();
    if (subValues.size() != 1 || subValues.front() != expected.connector) {
        diagnostic = std::string(referenceName) + " does not preserve the requested connector path";
        return false;
    }
    const auto actualType = elementConnector(subValues.front()) ? "element" : "interface";
    if (actualType != expected.connectorType) {
        diagnostic = std::string(referenceName) + " connector type does not match the request";
        return false;
    }
    const char* offsetName = std::string(referenceName) == "Reference1" ? "Offset1" : "Offset2";
    auto* offset = dynamic_cast<App::PropertyPlacement*>(joint->getPropertyByName(offsetName));
    if (expected.hasOffset && !offset) {
        diagnostic = std::string(offsetName) + " is unavailable for the requested connector offset";
        return false;
    }
    if (expected.hasOffset) {
        const auto actual = offset->getValue();
        double qx = 0.0;
        double qy = 0.0;
        double qz = 0.0;
        double qw = 1.0;
        actual.getRotation().getValue(qx, qy, qz, qw);
        const auto position = actual.getPosition();
        const auto close = [](double left, double right) {
            return std::abs(left - right) <= 1.0e-8
                * std::max({1.0, std::abs(left), std::abs(right)});
        };
        if (!close(position.x, expected.offset.x) || !close(position.y, expected.offset.y)
            || !close(position.z, expected.offset.z) || !close(qx, expected.offset.qx)
            || !close(qy, expected.offset.qy) || !close(qz, expected.offset.qz)
            || !close(qw, expected.offset.qw)) {
            diagnostic = std::string(offsetName) + " does not preserve the requested offset";
            return false;
        }
    }
    return true;
}

bool jointMatchesRequest(App::DocumentObject* joint,
                         App::DocumentObject* first,
                         App::DocumentObject* second,
                         const JointRequest& request,
                         std::string& diagnostic)
{
    if (!joint || !connectorMatches(joint, "Reference1", first, request.first, diagnostic)
        || !connectorMatches(joint, "Reference2", second, request.second, diagnostic)) {
        return false;
    }
    auto* type = dynamic_cast<App::PropertyEnumeration*>(joint->getPropertyByName("JointType"));
    const auto actualJointType = type ? std::string(type->getValueAsString()) : std::string {};
    if (!type || (request.jointType == "fixed" && actualJointType != "Fixed")
        || (request.jointType == "revolute" && actualJointType != "Revolute")) {
        diagnostic = "joint type does not match the request";
        return false;
    }
    if (auto* reverse = dynamic_cast<App::PropertyBool*>(joint->getPropertyByName("Reversed"));
        reverse && reverse->getValue() != request.reverse) {
        diagnostic = "joint reverse state does not match the request";
        return false;
    }
    if (request.hasLimits) {
        auto* minimum = dynamic_cast<App::PropertyQuantity*>(joint->getPropertyByName("AngleMin"));
        auto* maximum = dynamic_cast<App::PropertyQuantity*>(joint->getPropertyByName("AngleMax"));
        if (!minimum || !maximum || std::abs(minimum->getValue() - request.minimumDegrees) > 1.0e-8
            || std::abs(maximum->getValue() - request.maximumDegrees) > 1.0e-8) {
            diagnostic = "joint angle limits do not match the request";
            return false;
        }
    }
    return true;
}

NodeId occurrenceId(const GraphSnapshot& graph, const std::string& name)
{
    for (const auto& node : graph.nodes()) {
        if ((node.kind == NodeKind::Occurrence || node.kind == NodeKind::AssemblyOccurrence)
            && node.native.objectName == name) {
            return node.id;
        }
    }
    return {};
}

std::shared_ptr<GraphSnapshot> graphWithGrounding(const GraphSnapshot& base,
                                                  const std::vector<App::DocumentObject*>& joints,
                                                  const std::vector<std::string>& components,
                                                  bool grounded,
                                                  std::string& diagnostic)
{
    auto result = std::make_shared<GraphSnapshot>(base);
    std::unordered_set<NodeId> remove;
    for (const auto& node : result->nodes()) {
        if (node.kind != NodeKind::GroundConstraint) {
            continue;
        }
        if (const auto* payload = std::get_if<GroundConstraintPayload>(&node.payload);
            payload && std::find(components.begin(), components.end(), payload->constrainedObject)
                != components.end()) {
            remove.insert(node.id);
        }
    }
    if (!remove.empty()) {
        auto& nodes = result->nodes();
        nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [&](const NodeRecord& node) {
                        return remove.contains(node.id);
                    }),
                    nodes.end());
        auto& edges = result->edges();
        edges.erase(std::remove_if(edges.begin(), edges.end(), [&](const EdgeRecord& edge) {
                        return remove.contains(edge.from) || remove.contains(edge.to);
                    }),
                    edges.end());
    }
    if (grounded) {
        for (auto* joint : joints) {
            auto* target = groundedTarget(joint);
            if (!target || std::find(components.begin(), components.end(), target->getNameInDocument())
                    == components.end()) {
                continue;
            }
            const auto targetId = occurrenceId(base, target->getNameInDocument());
            if (targetId.empty()) {
                diagnostic = "graph lacks the occurrence for a grounded component";
                return {};
            }
            NodeRecord node;
            node.id = stableNodeId("ground", joint);
            node.kind = NodeKind::GroundConstraint;
            node.native = identity(joint);
            node.display = {joint->Label.getValue(), joint->Label.getValue()};
            node.provenance.kind = "freecad-assembly-grounded-joint";
            node.payload = GroundConstraintPayload {true, target->getNameInDocument(),
                                                     joint->getNameInDocument()};
            result->nodes().push_back(node);
            result->edges().push_back({stableEdgeId(EdgeKind::GroundedBy, node.id, targetId),
                                       EdgeKind::GroundedBy, node.id, targetId, {}, {}});
        }
    }
    if (!result->finalize(diagnostic)) {
        return {};
    }
    return result;
}

std::shared_ptr<GraphSnapshot> graphWithJoint(const GraphSnapshot& base,
                                              App::DocumentObject* joint,
                                              const JointRequest& request,
                                              std::string& diagnostic)
{
    const auto firstId = occurrenceId(base, request.first.component);
    const auto secondId = occurrenceId(base, request.second.component);
    if (firstId.empty() || secondId.empty()) {
        diagnostic = "graph lacks one or both joint endpoint occurrences";
        return {};
    }
    auto result = std::make_shared<GraphSnapshot>(base);
    NodeRecord node;
    node.id = stableNodeId("joint", joint);
    node.kind = NodeKind::Joint;
    node.native = identity(joint);
    node.display = {joint->Label.getValue(), joint->Label.getValue()};
    node.provenance.kind = "freecad-assembly-joint";
    node.payload = JointPayload {request.jointType, request.reverse, request.hasLimits,
                                 request.minimumDegrees, request.maximumDegrees};
    result->nodes().push_back(node);
    result->edges().push_back({stableEdgeId(EdgeKind::HasJoint,
                                            base.header().activeAssemblyNodeId,
                                            node.id),
                               EdgeKind::HasJoint, base.header().activeAssemblyNodeId, node.id,
                               {}, {}});
    result->edges().push_back({stableEdgeId(EdgeKind::JointEndpoint, node.id, firstId),
                               EdgeKind::JointEndpoint, node.id, firstId, {}, "Reference1"});
    result->edges().push_back({stableEdgeId(EdgeKind::JointEndpoint, node.id, secondId),
                               EdgeKind::JointEndpoint, node.id, secondId, {}, "Reference2"});
    if (!result->finalize(diagnostic)) {
        return {};
    }
    return result;
}

bool expectedRevision(const GraphStore& graphs,
                      const GraphScope& scope,
                      const std::string& expected,
                      std::shared_ptr<const GraphSnapshot>& current,
                      std::string& diagnostic)
{
    const auto lookup = graphs.current(scope, true);
    if (!lookup) {
        diagnostic = "a current graph is required before a constraint mutation";
        return false;
    }
    current = lookup.snapshot;
    if (current->header().graphRevision != expected) {
        diagnostic = "expected graph revision is stale";
        return false;
    }
    return true;
}

void auditFailure(GraphAuditLog& audit,
                  const std::string& stage,
                  const std::string& tool,
                  const std::string& operationId,
                  const std::string& parent,
                  const std::string& predicted,
                  const GraphSnapshot* snapshot,
                  const std::string& code,
                  const std::string& diagnostic,
                  const std::string& transaction)
{
    audit.record(makeMutationAuditEvent(stage, "failed", tool, operationId, parent,
                                         snapshot ? snapshot->header().graphRevision : std::string {},
                                         predicted, {}, "invalid", transaction, snapshot, code,
                                         diagnostic));
}

ToolResult successResult(const std::string& tool,
                         const std::string& operationId,
                         const std::string& parent,
                         const std::shared_ptr<const GraphSnapshot>& snapshot,
                         const std::string& predicted,
                         const std::string& observed,
                         bool changed)
{
    return ToolResult::success("cadx.assembly-constraint-result.v1",
                               receiptJson(tool, operationId, parent,
                                           snapshot ? snapshot->header().graphRevision : parent,
                                           predicted, observed, changed ? "committed" : "noop",
                                           "valid", changed));
}
#endif
}  // namespace

bool parseGroundingRequest(const std::string& argumentsJson,
                           const std::string& expectedOperation,
                           GroundingRequest& request,
                           std::string& diagnostic)
{
    request = GroundingRequest {};
    QJsonObject object;
    if (!parseRoot(argumentsJson, object, diagnostic)
        || !closed(object, {"operation", "operation_id", "expected_graph_revision", "assembly",
                            "components", "expected_component_count", "expected_grounded_count"},
                   diagnostic)
        || !parseCommon(object, expectedOperation, request.operationId,
                        request.expectedGraphRevision, diagnostic)
        || !objectName(object.value("assembly"), request.assembly, diagnostic)) {
        return false;
    }
    if (!object.value("components").isArray()) {
        diagnostic = "components must be an array";
        return false;
    }
    const auto array = object.value("components").toArray();
    if (array.size() < 1 || array.size() > 16) {
        diagnostic = "grounding requires 1 to 16 components";
        return false;
    }
    std::set<std::string> seen;
    for (const auto& value : array) {
        if (!value.isString()) {
            diagnostic = "grounding components must be strings";
            return false;
        }
        const auto component = value.toString().toStdString();
        if (component.empty() || component.size() > 128) {
            diagnostic = "grounding component name has an invalid length";
            return false;
        }
        if (!seen.insert(component).second) {
            diagnostic = "grounding components must be unique";
            return false;
        }
        request.components.push_back(std::move(component));
    }
    if (object.contains("expected_component_count")) {
        double count = 0.0;
        if (!finiteNumber(object.value("expected_component_count"), count, diagnostic, 0.0, 1'000'000.0)
            || std::floor(count) != count) {
            diagnostic = "expected_component_count must be a non-negative integer";
            return false;
        }
        request.expectedComponentCount = static_cast<int>(count);
    }
    if (object.contains("expected_grounded_count")) {
        double count = 0.0;
        if (!finiteNumber(object.value("expected_grounded_count"), count, diagnostic, 0.0, 1'000'000.0)
            || std::floor(count) != count) {
            diagnostic = "expected_grounded_count must be a non-negative integer";
            return false;
        }
        request.expectedGroundedCount = static_cast<int>(count);
    }
    request.grounded = expectedOperation == "set_grounded";
    return true;
}

bool parseJointRequest(const std::string& argumentsJson,
                       JointRequest& request,
                       std::string& diagnostic)
{
    request = JointRequest {};
    QJsonObject object;
    if (!parseRoot(argumentsJson, object, diagnostic)
        || !closed(object, {"operation", "operation_id", "expected_graph_revision", "assembly",
                            "first", "second", "joint_type", "label", "reverse", "limits"},
                   diagnostic)
        || !parseCommon(object, "create", request.operationId,
                        request.expectedGraphRevision, diagnostic)
        || !objectName(object.value("assembly"), request.assembly, diagnostic)
        || !parseConnector(object.value("first"), request.first, diagnostic)
        || !parseConnector(object.value("second"), request.second, diagnostic)
        || !stringValue(object, "joint_type", request.jointType, diagnostic, 1, 16)) {
        return false;
    }
    if (request.jointType != "fixed" && request.jointType != "revolute") {
        diagnostic = "joint_type must be fixed or revolute";
        return false;
    }
    request.label = request.jointType == "fixed" ? "Fixed Joint" : "Revolute Joint";
    if (object.contains("label")
        && !stringValue(object, "label", request.label, diagnostic, 1, 160)) {
        return false;
    }
    if (object.contains("reverse")) {
        if (!object.value("reverse").isBool()) {
            diagnostic = "reverse must be boolean";
            return false;
        }
        request.reverse = object.value("reverse").toBool();
    }
    if (object.contains("limits")) {
        if (request.jointType != "revolute" || !object.value("limits").isObject()) {
            diagnostic = "limits are supported only for revolute joints";
            return false;
        }
        const auto limits = object.value("limits").toObject();
        if (!closed(limits, {"minimum_degrees", "maximum_degrees"}, diagnostic)
            || !limits.contains("minimum_degrees") || !limits.contains("maximum_degrees")
            || !finiteNumber(limits.value("minimum_degrees"), request.minimumDegrees,
                             diagnostic, -180.0, 180.0)
            || !finiteNumber(limits.value("maximum_degrees"), request.maximumDegrees,
                             diagnostic, -180.0, 180.0)) {
            if (diagnostic.empty()) {
                diagnostic = "revolute limits require minimum_degrees and maximum_degrees";
            }
            return false;
        }
        if (request.minimumDegrees > request.maximumDegrees) {
            diagnostic = "revolute minimum_degrees must not exceed maximum_degrees";
            return false;
        }
        request.hasLimits = true;
    }
    if (request.first.component == request.second.component) {
        diagnostic = "joint endpoints must belong to different components";
        return false;
    }
    return true;
}

NativeAssemblyConstraintOperations::NativeAssemblyConstraintOperations(
    GraphStore& graphs,
    GraphAuditLog& audit,
    std::shared_ptr<const AssemblyConstraintBridge> bridge)
    : _graphs(graphs)
    , _audit(audit)
    , _bridge(std::move(bridge))
{
#ifdef CADX_HAVE_ASSEMBLY
    if (!_bridge) {
        _bridge = std::make_shared<PythonAssemblyConstraintBridge>();
    }
#endif
}

ToolResult NativeAssemblyConstraintOperations::execute(const std::string& toolName,
                                                       const std::string& argumentsJson) const
{
    if (toolName == "assembly.ground") {
        GroundingRequest request;
        std::string diagnostic;
        if (!parseGroundingRequest(argumentsJson, "set_grounded", request, diagnostic)
            && !parseGroundingRequest(argumentsJson, "set_movable", request, diagnostic)) {
            return ToolResult::failure("CADX_TOOL_ARGUMENTS_INVALID", diagnostic);
        }
        return executeGrounding(request, toolName);
    }
    if (toolName == "assembly.joint") {
        JointRequest request;
        std::string diagnostic;
        if (!parseJointRequest(argumentsJson, request, diagnostic)) {
            return ToolResult::failure("CADX_TOOL_ARGUMENTS_INVALID", diagnostic);
        }
        return executeJoint(request, toolName);
    }
    return ToolResult::failure("CADX_UNSUPPORTED_OPERATION", "unknown Assembly constraint tool");
}

ToolResult NativeAssemblyConstraintOperations::executeGrounding(const GroundingRequest& request,
                                                                const std::string& toolName) const
{
#ifndef CADX_HAVE_ASSEMBLY
    return ToolResult::failure("CADX_UNSUPPORTED_OBJECT", "the Assembly module is not built");
#else
    auto* document = App::GetApplication().getActiveDocument();
    if (!document) {
        return ToolResult::failure("CADX_NO_ACTIVE_DOCUMENT", "there is no active FreeCAD document");
    }
    auto* assembly = dynamic_cast<Assembly::AssemblyObject*>(document->getObject(request.assembly.c_str()));
    if (!assembly) {
        return ToolResult::failure("CADX_INVALID_ASSEMBLY", "assembly is not an Assembly::AssemblyObject");
    }
    GraphScope scope {document->Uid.getValueStr(), assembly->getNameInDocument()};
    std::shared_ptr<const GraphSnapshot> current;
    std::string diagnostic;
    if (!loadMutationBase(_graphs, document, assembly, request.expectedGraphRevision,
                          current, scope, diagnostic)) {
        return ToolResult::failure("CADX_GRAPH_REVISION_MISMATCH", diagnostic, true);
    }
    const auto components = Assembly::getAssemblyComponents(assembly);
    if (request.expectedComponentCount >= 0
        && static_cast<int>(components.size()) != request.expectedComponentCount) {
        return ToolResult::failure("CADX_PRECONDITION_FAILED", "Assembly component count changed", true);
    }
    std::vector<App::DocumentObject*> targets;
    for (const auto& name : request.components) {
        auto* component = componentInAssembly(assembly, name);
        if (!component || !component->getPlacementProperty()) {
            return ToolResult::failure("CADX_PRECONDITION_FAILED", "grounding target is not an active component");
        }
        targets.push_back(component);
    }
    std::size_t groundedCount = 0;
    for (auto* joint : assembly->getGroundedJoints()) {
        if (groundedTarget(joint)) {
            ++groundedCount;
        }
    }
    if (request.expectedGroundedCount >= 0
        && static_cast<int>(groundedCount) != request.expectedGroundedCount) {
        return ToolResult::failure("CADX_PRECONDITION_FAILED", "grounded-joint count changed", true);
    }
    const auto predicted = sha256Revision("ground|" + std::string(request.grounded ? "true|" : "false|")
                                          + std::accumulate(request.components.begin(), request.components.end(),
                                                            std::string {},
                                                            [](std::string value, const std::string& name) {
                                                                return value + name + "|";
                                                            }));
    bool changed = false;
    for (auto* component : targets) {
        std::size_t count = 0;
        auto* existing = groundedJointFor(assembly, component, count);
        if (count > 1) {
            return ToolResult::failure("CADX_PRECONDITION_FAILED", "duplicate grounded joints detected");
        }
        if (request.grounded != (existing != nullptr) || (request.grounded && !placementReadOnly(component))) {
            changed = true;
        }
    }
    if (!changed) {
        const auto noOpDelta = mutationDeltaHash(current.get(), *current);
        return successResult(toolName, request.operationId, request.expectedGraphRevision,
                             current, noOpDelta, noOpDelta, false);
    }
    DocumentMutationTransaction transaction(document, "CadX assembly.ground");
    std::vector<std::string> createdNames;
    for (auto* component : targets) {
        std::size_t count = 0;
        auto* existing = groundedJointFor(assembly, component, count);
        if (request.grounded && !existing) {
            std::string created;
            if (!_bridge || !_bridge->createGroundedJoint(document->getName(), assembly->getNameInDocument(),
                                                          component->getNameInDocument(), created, diagnostic)) {
                transaction.abort();
                auditFailure(_audit, "bridge", toolName, request.operationId,
                             request.expectedGraphRevision, predicted, current.get(),
                             "CADX_BRIDGE_FAILURE", diagnostic, "aborted");
                return ToolResult::failure("CADX_BRIDGE_FAILURE", diagnostic, true);
            }
            createdNames.push_back(created);
        }
        else if (!request.grounded && existing) {
            document->removeObject(existing->getNameInDocument());
        }
    }
    document->recompute();
    for (auto* component : targets) {
        std::size_t count = 0;
        auto* joint = groundedJointFor(assembly, component, count);
        if ((request.grounded && (count != 1 || !joint || !placementReadOnly(component)))
            || (!request.grounded && (count != 0 || placementReadOnly(component)))) {
            transaction.abort();
            diagnostic = "grounding postcondition or placement lock verification failed";
            auditFailure(_audit, "postcondition", toolName, request.operationId,
                         request.expectedGraphRevision, predicted, current.get(),
                         "CADX_POSTCONDITION_FAILED", diagnostic, "aborted");
            return ToolResult::failure("CADX_POSTCONDITION_FAILED", diagnostic, true);
        }
        if (request.grounded && !_bridge->verifyGroundedProxy(document->getName(), joint->getNameInDocument(), diagnostic)) {
            transaction.abort();
            auditFailure(_audit, "bridge_verify", toolName, request.operationId,
                         request.expectedGraphRevision, predicted, current.get(),
                         "CADX_BRIDGE_FAILURE", diagnostic, "aborted");
            return ToolResult::failure("CADX_BRIDGE_FAILURE", diagnostic, true);
        }
    }
    const auto candidateCapture = captureNativeAssemblyGraph(document, assembly);
    if (!candidateCapture) {
        transaction.abort();
        auditFailure(_audit, "graph_candidate", toolName, request.operationId,
                     request.expectedGraphRevision, predicted, current.get(),
                     candidateCapture.errorCode.empty() ? "CADX_GRAPH_BUILD_FAILED"
                                                         : candidateCapture.errorCode,
                     candidateCapture.diagnostic, "aborted");
        return ToolResult::failure(candidateCapture.errorCode.empty()
                                       ? "CADX_GRAPH_BUILD_FAILED" : candidateCapture.errorCode,
                                   candidateCapture.diagnostic, true);
    }
    const auto candidate = candidateCapture.snapshot;
    const auto predictedDelta = mutationDeltaHash(current.get(), *candidate);
    transaction.commit();

    const auto observed = captureNativeAssemblyGraph(document, assembly);
    if (!observed) {
        const auto consistency = "SEVERE graph consistency failure after assembly.ground commit: "
            + observed.diagnostic;
        _graphs.markScopeStale(scope, consistency);
        auditFailure(_audit, "post_commit_capture", toolName, request.operationId,
                     request.expectedGraphRevision, predictedDelta, candidate.get(),
                     "CADX_GRAPH_CONSISTENCY_FAILURE", consistency, "committed");
        return ToolResult::failure("CADX_GRAPH_CONSISTENCY_FAILURE", consistency, true);
    }
    if (!equivalentGraphState(*candidate, *observed.snapshot, diagnostic)) {
        const auto consistency = "SEVERE graph consistency failure after assembly.ground commit: "
            + diagnostic;
        _graphs.markScopeStale(scope, consistency);
        auditFailure(_audit, "post_commit_verify", toolName, request.operationId,
                     request.expectedGraphRevision, predictedDelta, observed.snapshot.get(),
                     "CADX_GRAPH_CONSISTENCY_FAILURE", consistency, "committed");
        return ToolResult::failure("CADX_GRAPH_CONSISTENCY_FAILURE", consistency, true);
    }
    const auto observedDelta = mutationDeltaHash(current.get(), *observed.snapshot);
    std::string publishDiagnostic;
    const auto publication = publishCommittedGraph(
        _graphs, scope, observed.snapshot, request.expectedGraphRevision, publishDiagnostic);
    if (publication != CommitPublication::Published) {
        auditFailure(_audit, "publish", toolName, request.operationId,
                     request.expectedGraphRevision, predictedDelta, observed.snapshot.get(),
                     "CADX_GRAPH_CONSISTENCY_FAILURE", publishDiagnostic, "committed");
        return ToolResult::failure("CADX_GRAPH_CONSISTENCY_FAILURE", publishDiagnostic, true);
    }
    _audit.record(makeMutationAuditEvent("mutation", "passed", toolName, request.operationId,
                                         request.expectedGraphRevision,
                                         observed.snapshot->header().graphRevision,
                                         predictedDelta, observedDelta, "valid", "committed",
                                         observed.snapshot.get()));
    return successResult(toolName, request.operationId, request.expectedGraphRevision,
                         observed.snapshot, predictedDelta, observedDelta, true);
#endif
}

ToolResult NativeAssemblyConstraintOperations::executeJoint(const JointRequest& request,
                                                            const std::string& toolName) const
{
#ifndef CADX_HAVE_ASSEMBLY
    return ToolResult::failure("CADX_UNSUPPORTED_OBJECT", "the Assembly module is not built");
#else
    auto* document = App::GetApplication().getActiveDocument();
    if (!document) {
        return ToolResult::failure("CADX_NO_ACTIVE_DOCUMENT", "there is no active FreeCAD document");
    }
    auto* assembly = dynamic_cast<Assembly::AssemblyObject*>(document->getObject(request.assembly.c_str()));
    auto* first = assembly ? componentInAssembly(assembly, request.first.component) : nullptr;
    auto* second = assembly ? componentInAssembly(assembly, request.second.component) : nullptr;
    if (!assembly || !first || !second) {
        return ToolResult::failure("CADX_PRECONDITION_FAILED", "joint endpoints are not active Assembly components");
    }
    GraphScope scope {document->Uid.getValueStr(), assembly->getNameInDocument()};
    std::shared_ptr<const GraphSnapshot> current;
    std::string diagnostic;
    if (!loadMutationBase(_graphs, document, assembly, request.expectedGraphRevision,
                          current, scope, diagnostic)) {
        return ToolResult::failure("CADX_GRAPH_REVISION_MISMATCH", diagnostic, true);
    }
    const auto predicted = sha256Revision("joint|" + request.jointType + "|"
                                          + request.first.component + "|" + request.first.connector + "|"
                                          + request.second.component + "|" + request.second.connector);
    for (auto* existing : assembly->getJoints(false, false)) {
        if (!existing) {
            continue;
        }
        auto* type = dynamic_cast<App::PropertyEnumeration*>(existing->getPropertyByName("JointType"));
        auto* ref1 = dynamic_cast<App::PropertyXLinkSub*>(existing->getPropertyByName("Reference1"));
        auto* ref2 = dynamic_cast<App::PropertyXLinkSub*>(existing->getPropertyByName("Reference2"));
        if (!type || !ref1 || !ref2) {
            continue;
        }
        const auto firstSubs = ref1->getSubValues();
        const auto secondSubs = ref2->getSubValues();
        if (type->getValueAsString() == (request.jointType == "fixed" ? "Fixed" : "Revolute")
            && ref1->getValue() == first && ref2->getValue() == second
            && firstSubs.size() == 1 && secondSubs.size() == 1
            && firstSubs.front() == request.first.connector && secondSubs.front() == request.second.connector) {
            return ToolResult::failure("CADX_DUPLICATE_JOINT", "an equivalent joint already exists");
        }
    }
    DocumentMutationTransaction transaction(document, "CadX assembly.joint");
    std::string createdName;
    if (!_bridge || !_bridge->createRegularJoint(document->getName(), assembly->getNameInDocument(),
                                                 request.jointType, request.label, request.first,
                                                 request.second, request.reverse, createdName, diagnostic)) {
        transaction.abort();
        auditFailure(_audit, "bridge", toolName, request.operationId,
                     request.expectedGraphRevision, predicted, current.get(),
                     "CADX_BRIDGE_FAILURE", diagnostic, "aborted");
        return ToolResult::failure("CADX_BRIDGE_FAILURE", diagnostic, true);
    }
    auto* joint = document->getObject(createdName.c_str());
    if (!joint) {
        transaction.abort();
        diagnostic = "bridge returned an unknown joint object";
        auditFailure(_audit, "postcondition", toolName, request.operationId,
                     request.expectedGraphRevision, predicted, current.get(),
                     "CADX_POSTCONDITION_FAILED", diagnostic, "aborted");
        return ToolResult::failure("CADX_POSTCONDITION_FAILED", diagnostic, true);
    }
    if (auto* offset = dynamic_cast<App::PropertyPlacement*>(joint->getPropertyByName("Offset1"));
        offset && request.first.hasOffset) {
        // Placement payload assignment is intentionally kept in C++; the
        // Python bridge only creates exact Assembly proxy/view state.
        offset->setValue(Base::Placement(Base::Vector3d(request.first.offset.x,
                                                         request.first.offset.y,
                                                         request.first.offset.z),
                                         Base::Rotation(request.first.offset.qx,
                                                        request.first.offset.qy,
                                                        request.first.offset.qz,
                                                        request.first.offset.qw)));
    }
    if (auto* offset = dynamic_cast<App::PropertyPlacement*>(joint->getPropertyByName("Offset2"));
        offset && request.second.hasOffset) {
        offset->setValue(Base::Placement(Base::Vector3d(request.second.offset.x,
                                                         request.second.offset.y,
                                                         request.second.offset.z),
                                         Base::Rotation(request.second.offset.qx,
                                                        request.second.offset.qy,
                                                        request.second.offset.qz,
                                                        request.second.offset.qw)));
    }
    if (request.jointType == "revolute") {
        if (auto* enable = dynamic_cast<App::PropertyBool*>(joint->getPropertyByName("EnableAngleMin"));
            enable) {
            enable->setValue(request.hasLimits);
        }
        if (auto* enable = dynamic_cast<App::PropertyBool*>(joint->getPropertyByName("EnableAngleMax"));
            enable) {
            enable->setValue(request.hasLimits);
        }
        if (request.hasLimits) {
            if (auto* minimum = dynamic_cast<App::PropertyQuantity*>(joint->getPropertyByName("AngleMin"));
                minimum) {
                minimum->setValue(request.minimumDegrees);
            }
            if (auto* maximum = dynamic_cast<App::PropertyQuantity*>(joint->getPropertyByName("AngleMax"));
                maximum) {
                maximum->setValue(request.maximumDegrees);
            }
        }
    }
    document->recompute();
    if (!_bridge->verifyRegularProxy(document->getName(), joint->getNameInDocument(), diagnostic)
        || !jointMatchesRequest(joint, first, second, request, diagnostic)) {
        transaction.abort();
        if (diagnostic.empty()) {
            diagnostic = "joint proxy or connector postcondition failed";
        }
        auditFailure(_audit, "postcondition", toolName, request.operationId,
                     request.expectedGraphRevision, predicted, current.get(),
                     "CADX_POSTCONDITION_FAILED", diagnostic, "aborted");
        return ToolResult::failure("CADX_POSTCONDITION_FAILED", diagnostic, true);
    }
    const auto candidateCapture = captureNativeAssemblyGraph(document, assembly);
    if (!candidateCapture) {
        transaction.abort();
        auditFailure(_audit, "graph_candidate", toolName, request.operationId,
                     request.expectedGraphRevision, predicted, current.get(),
                     candidateCapture.errorCode.empty() ? "CADX_GRAPH_BUILD_FAILED"
                                                         : candidateCapture.errorCode,
                     candidateCapture.diagnostic, "aborted");
        return ToolResult::failure(candidateCapture.errorCode.empty()
                                       ? "CADX_GRAPH_BUILD_FAILED" : candidateCapture.errorCode,
                                   candidateCapture.diagnostic, true);
    }
    const auto candidate = candidateCapture.snapshot;
    const auto predictedDelta = mutationDeltaHash(current.get(), *candidate);
    transaction.commit();

    const auto observed = captureNativeAssemblyGraph(document, assembly);
    if (!observed) {
        const auto consistency = "SEVERE graph consistency failure after assembly.joint commit: "
            + observed.diagnostic;
        _graphs.markScopeStale(scope, consistency);
        auditFailure(_audit, "post_commit_capture", toolName, request.operationId,
                     request.expectedGraphRevision, predictedDelta, candidate.get(),
                     "CADX_GRAPH_CONSISTENCY_FAILURE", consistency, "committed");
        return ToolResult::failure("CADX_GRAPH_CONSISTENCY_FAILURE", consistency, true);
    }
    if (!equivalentGraphState(*candidate, *observed.snapshot, diagnostic)) {
        const auto consistency = "SEVERE graph consistency failure after assembly.joint commit: "
            + diagnostic;
        _graphs.markScopeStale(scope, consistency);
        auditFailure(_audit, "post_commit_verify", toolName, request.operationId,
                     request.expectedGraphRevision, predictedDelta, observed.snapshot.get(),
                     "CADX_GRAPH_CONSISTENCY_FAILURE", consistency, "committed");
        return ToolResult::failure("CADX_GRAPH_CONSISTENCY_FAILURE", consistency, true);
    }
    const auto observedDelta = mutationDeltaHash(current.get(), *observed.snapshot);
    std::string publishDiagnostic;
    const auto publication = publishCommittedGraph(
        _graphs, scope, observed.snapshot, request.expectedGraphRevision, publishDiagnostic);
    if (publication != CommitPublication::Published) {
        auditFailure(_audit, "publish", toolName, request.operationId,
                     request.expectedGraphRevision, predictedDelta, observed.snapshot.get(),
                     "CADX_GRAPH_CONSISTENCY_FAILURE", publishDiagnostic, "committed");
        return ToolResult::failure("CADX_GRAPH_CONSISTENCY_FAILURE", publishDiagnostic, true);
    }
    _audit.record(makeMutationAuditEvent("mutation", "passed", toolName, request.operationId,
                                         request.expectedGraphRevision,
                                         observed.snapshot->header().graphRevision,
                                         predictedDelta, observedDelta, "valid", "committed",
                                         observed.snapshot.get()));
    return successResult(toolName, request.operationId, request.expectedGraphRevision,
                         observed.snapshot, predictedDelta, observedDelta, true);
#endif
}

}  // namespace CadX
