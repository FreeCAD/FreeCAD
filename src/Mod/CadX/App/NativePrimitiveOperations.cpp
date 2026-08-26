// SPDX-License-Identifier: LGPL-2.1-or-later

#include "NativePrimitiveOperations.h"

#include "GraphRevision.h"
#include "NativeMutationSupport.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>
#include <Base/Vector3D.h>
#ifdef CADX_HAVE_PART_DESIGN
#include <Mod/Part/App/PropertyTopoShape.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#endif

#ifdef CADX_HAVE_PART_DESIGN
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopAbs_ShapeEnum.hxx>
#endif

#include <algorithm>
#include <cmath>
#include <exception>
#include <initializer_list>
#include <limits>
#include <set>
#include <sstream>
#include <utility>

namespace CadX
{
namespace
{
constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr double kAxisEpsilon = 1.0e-12;
constexpr double kPlacementTolerance = 1.0e-8;

PrimitiveParseResult invalid(const std::string& diagnostic)
{
    return {false, {}, "CADX_TOOL_ARGUMENTS_INVALID", diagnostic};
}

bool closed(const QJsonObject& object,
            std::initializer_list<const char*> allowed,
            std::string& diagnostic)
{
    std::set<std::string> names;
    for (const auto* name : allowed) {
        names.emplace(name);
    }
    for (auto iterator = object.begin(); iterator != object.end(); ++iterator) {
        if (!names.contains(iterator.key().toStdString())) {
            diagnostic = "unknown field '" + iterator.key().toStdString() + "'";
            return false;
        }
    }
    return true;
}

bool stringField(const QJsonObject& object,
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

bool numberField(const QJsonObject& object,
                 const char* name,
                 double& value,
                 std::string& diagnostic,
                 double minimum,
                 double maximum,
                 bool required = true)
{
    if (!object.contains(name)) {
        if (required) {
            diagnostic = std::string("missing field '") + name + "'";
            return false;
        }
        return true;
    }
    const auto item = object.value(name);
    if (!item.isDouble() || !std::isfinite(item.toDouble())) {
        diagnostic = std::string("field '") + name + "' must be finite";
        return false;
    }
    value = item.toDouble();
    if (value < minimum || value > maximum) {
        diagnostic = std::string("field '") + name + "' is outside its bounds";
        return false;
    }
    return true;
}

bool vectorField(const QJsonValue& value,
                 PrimitiveVector& result,
                 std::string& diagnostic,
                 double minimum,
                 double maximum)
{
    if (!value.isObject()) {
        diagnostic = "vector must be an object";
        return false;
    }
    const auto object = value.toObject();
    if (!closed(object, {"x", "y", "z"}, diagnostic)) {
        return false;
    }
    return numberField(object, "x", result.x, diagnostic, minimum, maximum)
        && numberField(object, "y", result.y, diagnostic, minimum, maximum)
        && numberField(object, "z", result.z, diagnostic, minimum, maximum);
}

bool rotationField(const QJsonValue& value,
                   PrimitiveRotation& result,
                   std::string& diagnostic)
{
    if (!value.isObject()) {
        diagnostic = "rotation must be an object";
        return false;
    }
    const auto object = value.toObject();
    if (!closed(object, {"axis", "angle_degrees"}, diagnostic)) {
        return false;
    }
    if (!object.contains("axis") || !object.contains("angle_degrees")
        || !vectorField(object.value("axis"), result.axis, diagnostic,
                         -std::numeric_limits<double>::max(),
                         std::numeric_limits<double>::max())
        || !numberField(object, "angle_degrees", result.angleDegrees, diagnostic,
                        -360.0, 360.0)) {
        if (diagnostic.empty()) {
            diagnostic = "rotation requires axis and angle_degrees";
        }
        return false;
    }
    const auto magnitude = std::sqrt(result.axis.x * result.axis.x
                                     + result.axis.y * result.axis.y
                                     + result.axis.z * result.axis.z);
    if (!std::isfinite(magnitude) || magnitude <= kAxisEpsilon) {
        diagnostic = "rotation axis must be finite and non-zero";
        return false;
    }
    result.axis.x /= magnitude;
    result.axis.y /= magnitude;
    result.axis.z /= magnitude;
    return true;
}

bool finiteAndInRange(double value, double minimum, double maximum)
{
    return std::isfinite(value) && value >= minimum && value <= maximum;
}

bool close(double left, double right)
{
    const auto scale = std::max({1.0, std::abs(left), std::abs(right)});
    return std::abs(left - right) <= kPlacementTolerance * scale;
}

std::string canonicalRequest(const PrimitiveRequest& request)
{
    std::ostringstream result;
    result.precision(17);
    result << "cadx.model.primitive.v1|" << request.operation << '|'
           << request.operationId << '|' << request.expectedGraphRevision << '|'
           << request.label << '|' << request.center.x << '|' << request.center.y << '|'
           << request.center.z << '|' << request.rotation.axis.x << '|'
           << request.rotation.axis.y << '|' << request.rotation.axis.z << '|'
           << request.rotation.angleDegrees << '|' << request.lengthMm << '|'
           << request.widthMm << '|' << request.heightMm << '|' << request.radiusMm << '|'
           << request.sweepDegrees;
    return result.str();
}

void auditFailure(GraphAuditLog& audit,
                  const std::string& stage,
                  const std::string& operation,
                  const PrimitiveRequest& request,
                  const std::string& parentRevision,
                  const std::string& transactionStatus,
                  const std::string& errorCode,
                  const std::string& diagnostic,
                  const GraphSnapshot* snapshot = nullptr)
{
    audit.record(makeMutationAuditEvent(stage,
                                        "failed",
                                        operation,
                                        request.operationId,
                                        parentRevision,
                                        snapshot ? snapshot->header().graphRevision : std::string {},
                                        {},
                                        {},
                                        "invalid",
                                        transactionStatus,
                                        snapshot,
                                        errorCode,
                                        diagnostic));
}

ToolResult receipt(const PrimitiveRequest& request,
                   const std::string& bodyName,
                   const std::string& featureName,
                   const std::string& parentRevision,
                   const GraphSnapshot& snapshot,
                   const std::string& predictedDelta,
                   const std::string& observedDelta)
{
    QJsonObject payload {
        {"schema_version", "cadx.model-primitive-result.v1"},
        {"operation", QString::fromStdString(request.operation)},
        {"operation_id", QString::fromStdString(request.operationId)},
        {"body", QJsonObject {{"object_name", QString::fromStdString(bodyName)}}},
        {"feature", QJsonObject {{"object_name", QString::fromStdString(featureName)}}},
        {"parent_revision", QString::fromStdString(parentRevision)},
        {"final_revision", QString::fromStdString(snapshot.header().graphRevision)},
        {"predicted_delta_hash", QString::fromStdString(predictedDelta)},
        {"observed_delta_hash", QString::fromStdString(observedDelta)},
        {"physical_verdict", "valid"},
        {"transaction_status", "committed"}
    };
    return ToolResult::success("cadx.model-primitive-result.v1",
                               QJsonDocument(payload).toJson(QJsonDocument::Compact).toStdString());
}

bool validCapture(const PrimitiveGraphCapture& capture,
                  App::Document* document,
                  App::DocumentObject* feature,
                  const PrimitiveRequest& request,
                  std::string& diagnostic)
{
    if (!capture.snapshot) {
        diagnostic = capture.diagnostic.empty() ? "graph capture returned no snapshot"
                                                : capture.diagnostic;
        return false;
    }
    if (capture.snapshot->header().documentUid != document->Uid.getValueStr()) {
        diagnostic = "graph capture belongs to a different document";
        return false;
    }
    if (!feature) {
        return true;
    }
    const auto featureName = std::string(feature->getNameInDocument());
    const auto expectedType = request.operation == "box" ? "PartDesign::Box"
                                                          : "PartDesign::Cylinder";
    for (const auto& node : capture.snapshot->nodes()) {
        if (node.native.objectName != featureName || node.native.typeId != expectedType) {
            continue;
        }
        const auto* payload = std::get_if<PrimitivePayload>(&node.payload);
        if (!payload || payload->primitiveKind != request.operation) {
            diagnostic = "graph capture did not provide a typed PrimitivePayload";
            return false;
        }
        if (request.operation == "box"
            && (!close(payload->length, request.lengthMm)
                || !close(payload->width, request.widthMm)
                || !close(payload->height, request.heightMm))) {
            diagnostic = "graph PrimitivePayload dimensions do not match the request";
            return false;
        }
        if (request.operation == "cylinder"
            && (!close(payload->radius, request.radiusMm)
                || !close(payload->height, request.heightMm)
                || !close(payload->sweepDegrees, request.sweepDegrees))) {
            diagnostic = "graph PrimitivePayload dimensions do not match the request";
            return false;
        }
        return true;
    }
    diagnostic = "graph capture does not contain the created primitive feature";
    return false;
}

bool equalGraphs(const GraphSnapshot& expected,
                 const GraphSnapshot& actual,
                 std::string& diagnostic)
{
    if (expected.header().graphId != actual.header().graphId
        || canonicalSemantic(expected) != canonicalSemantic(actual)
        || canonicalPresentation(expected) != canonicalPresentation(actual)) {
        diagnostic = "post-commit CAD graph capture differs from the verified candidate";
        return false;
    }
    return true;
}

#ifdef CADX_HAVE_PART_DESIGN
bool verifyPhysicalPrimitive(const PrimitiveRequest& request,
                             PartDesign::Body* body,
                             PartDesign::FeaturePrimitive* primitive,
                             std::string& diagnostic)
{
    if (!body || !primitive || !body->hasObject(primitive) || body->Tip.getValue() != primitive) {
        diagnostic = "primitive is not the sole tip feature of its Body";
        return false;
    }
    const auto& shape = primitive->Shape.getShape();
    if (shape.isNull() || !shape.isValid() || shape.countSubShapes(TopAbs_SOLID) != 1) {
        diagnostic = "primitive must produce exactly one valid solid";
        return false;
    }
    GProp_GProps volumeProperties;
    BRepGProp::VolumeProperties(shape.getShape(), volumeProperties);
    if (!std::isfinite(volumeProperties.Mass()) || volumeProperties.Mass() <= 0.0) {
        diagnostic = "primitive solid must have positive finite volume";
        return false;
    }
    const auto origin = NativePrimitiveOperations::expectedOrigin(request);
    const Base::Placement expectedPlacement(
        Base::Vector3d(origin.x, origin.y, origin.z),
        Base::Rotation(Base::Vector3d(request.rotation.axis.x,
                                      request.rotation.axis.y,
                                      request.rotation.axis.z),
                       request.rotation.angleDegrees * kPi / 180.0));
    if (!primitive->Placement.getValue().isSame(expectedPlacement, kPlacementTolerance)) {
        diagnostic = "primitive placement does not match centered VibeCAD placement";
        return false;
    }
    if (request.operation == "box") {
        auto* box = dynamic_cast<PartDesign::Box*>(primitive);
        if (!box || !close(box->Length.getValue(), request.lengthMm)
            || !close(box->Width.getValue(), request.widthMm)
            || !close(box->Height.getValue(), request.heightMm)) {
            diagnostic = "FreeCAD box parameters do not match the request";
            return false;
        }
    }
    else {
        auto* cylinder = dynamic_cast<PartDesign::Cylinder*>(primitive);
        if (!cylinder || !close(cylinder->Radius.getValue(), request.radiusMm)
            || !close(cylinder->Height.getValue(), request.heightMm)
            || !close(cylinder->Angle.getValue(), request.sweepDegrees)) {
            diagnostic = "FreeCAD cylinder parameters do not match the request";
            return false;
        }
    }
    return true;
}
#endif

}  // namespace

PrimitivePreflightResult NativePrimitiveOperations::preflight(const PrimitiveRequest& request)
{
    if (request.operation != "box" && request.operation != "cylinder") {
        return {false, "CADX_TOOL_ARGUMENTS_INVALID", "operation must be box or cylinder"};
    }
    if (request.operationId.empty() || request.operationId.size() > 128
        || request.label.empty() || request.label.size() > 160
        || request.expectedGraphRevision.size() > 128) {
        return {false, "CADX_TOOL_ARGUMENTS_INVALID", "operation_id, label, or graph revision has an invalid length"};
    }
    if (!finiteAndInRange(request.center.x, -1.0e6, 1.0e6)
        || !finiteAndInRange(request.center.y, -1.0e6, 1.0e6)
        || !finiteAndInRange(request.center.z, -1.0e6, 1.0e6)) {
        return {false, "CADX_TOOL_ARGUMENTS_INVALID", "center_mm coordinates are outside their bounds"};
    }
    if (!finiteAndInRange(request.rotation.axis.x, -1.0, 1.0)
        || !finiteAndInRange(request.rotation.axis.y, -1.0, 1.0)
        || !finiteAndInRange(request.rotation.axis.z, -1.0, 1.0)
        || !finiteAndInRange(request.rotation.angleDegrees, -360.0, 360.0)) {
        return {false, "CADX_TOOL_ARGUMENTS_INVALID", "rotation contains a non-finite or out-of-range value"};
    }
    const auto axisMagnitude = std::sqrt(request.rotation.axis.x * request.rotation.axis.x
                                         + request.rotation.axis.y * request.rotation.axis.y
                                         + request.rotation.axis.z * request.rotation.axis.z);
    if (!std::isfinite(axisMagnitude) || axisMagnitude <= kAxisEpsilon
        || std::abs(axisMagnitude - 1.0) > 1.0e-8) {
        return {false, "CADX_TOOL_ARGUMENTS_INVALID", "rotation axis must be normalized and non-zero"};
    }
    if (!finiteAndInRange(request.heightMm, 0.0, 1.0e6) || request.heightMm <= 0.0) {
        return {false, "CADX_TOOL_ARGUMENTS_INVALID", "height_mm must be positive and at most 1e6"};
    }
    if (request.operation == "box") {
        if (!finiteAndInRange(request.lengthMm, 0.0, 1.0e6) || request.lengthMm <= 0.0
            || !finiteAndInRange(request.widthMm, 0.0, 1.0e6) || request.widthMm <= 0.0) {
            return {false, "CADX_TOOL_ARGUMENTS_INVALID", "box dimensions must be positive and at most 1e6"};
        }
    }
    else if (!finiteAndInRange(request.radiusMm, 0.0, 1.0e6) || request.radiusMm <= 0.0
             || !finiteAndInRange(request.sweepDegrees, 0.0, 360.0)
             || request.sweepDegrees <= 0.0) {
        return {false, "CADX_TOOL_ARGUMENTS_INVALID", "cylinder dimensions and sweep must be positive and bounded"};
    }
    return {true, {}, {}};
}

PrimitiveParseResult NativePrimitiveOperations::parseRequest(const std::string& argumentsJson)
{
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(QByteArray::fromStdString(argumentsJson), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return invalid("arguments must be a JSON object");
    }
    const auto object = document.object();
    std::string diagnostic;
    PrimitiveRequest request;
    if (!stringField(object, "operation", request.operation, diagnostic, 1, 32)
        || (request.operation != "box" && request.operation != "cylinder")) {
        return invalid(diagnostic.empty() ? "operation must be box or cylinder" : diagnostic);
    }
    if (!closed(object,
                request.operation == "box"
                    ? std::initializer_list<const char*> {"operation", "operation_id", "expected_graph_revision", "label", "center_mm", "rotation", "length_mm", "width_mm", "height_mm"}
                    : std::initializer_list<const char*> {"operation", "operation_id", "expected_graph_revision", "label", "center_mm", "rotation", "radius_mm", "height_mm", "sweep_degrees"},
                diagnostic)) {
        return invalid(diagnostic);
    }
    if (!stringField(object, "operation_id", request.operationId, diagnostic, 1, 128)
        || !stringField(object, "expected_graph_revision", request.expectedGraphRevision, diagnostic, 0, 128)
        || !stringField(object, "label", request.label, diagnostic, 1, 160)
        || !vectorField(object.value("center_mm"), request.center, diagnostic, -1.0e6, 1.0e6)) {
        return invalid(diagnostic);
    }
    if (object.contains("rotation") && !rotationField(object.value("rotation"), request.rotation, diagnostic)) {
        return invalid(diagnostic);
    }
    if (request.operation == "box") {
        if (!numberField(object, "length_mm", request.lengthMm, diagnostic, 0.0, 1.0e6)
            || !numberField(object, "width_mm", request.widthMm, diagnostic, 0.0, 1.0e6)
            || !numberField(object, "height_mm", request.heightMm, diagnostic, 0.0, 1.0e6)) {
            return invalid(diagnostic);
        }
    }
    else if (!numberField(object, "radius_mm", request.radiusMm, diagnostic, 0.0, 1.0e6)
             || !numberField(object, "height_mm", request.heightMm, diagnostic, 0.0, 1.0e6)
             || !numberField(object, "sweep_degrees", request.sweepDegrees, diagnostic,
                             0.0, 360.0, false)) {
        return invalid(diagnostic);
    }
    const auto checked = preflight(request);
    if (!checked) {
        return {false, {}, checked.errorCode, checked.diagnostic};
    }
    return {true, request, {}, {}};
}

PrimitiveVector NativePrimitiveOperations::expectedOrigin(const PrimitiveRequest& request)
{
    PrimitiveVector localCenter;
    if (request.operation == "box") {
        localCenter = {request.lengthMm * 0.5, request.widthMm * 0.5, request.heightMm * 0.5};
    }
    else {
        localCenter = {0.0, 0.0, request.heightMm * 0.5};
    }
    const auto& axis = request.rotation.axis;
    const auto magnitude = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    const auto x = axis.x / magnitude;
    const auto y = axis.y / magnitude;
    const auto z = axis.z / magnitude;
    const auto angle = request.rotation.angleDegrees * kPi / 180.0;
    const auto c = std::cos(angle);
    const auto s = std::sin(angle);
    const auto dot = x * localCenter.x + y * localCenter.y + z * localCenter.z;
    const PrimitiveVector rotated {
        localCenter.x * c + (y * localCenter.z - z * localCenter.y) * s + x * dot * (1.0 - c),
        localCenter.y * c + (z * localCenter.x - x * localCenter.z) * s + y * dot * (1.0 - c),
        localCenter.z * c + (x * localCenter.y - y * localCenter.x) * s + z * dot * (1.0 - c)
    };
    return {request.center.x - rotated.x,
            request.center.y - rotated.y,
            request.center.z - rotated.z};
}

NativePrimitiveOperations::NativePrimitiveOperations(GraphStore& graphs,
                                                     GraphAuditLog& audit,
                                                     PrimitiveOperationHooks hooks)
    : _graphs(graphs), _audit(audit), _hooks(std::move(hooks))
{}

ToolResult NativePrimitiveOperations::execute(const std::string& toolName,
                                              const std::string& argumentsJson) const
{
    if (toolName != "model.primitive") {
        return ToolResult::failure("CADX_UNSUPPORTED_TOOL", "expected the model.primitive tool");
    }
    const auto parsed = parseRequest(argumentsJson);
    if (!parsed) {
        return ToolResult::failure(parsed.errorCode, parsed.diagnostic);
    }
    const auto& request = parsed.request;
    if (!_hooks.resolveScope || !_hooks.captureGraph) {
        auditFailure(_audit,
                     "integration",
                     toolName,
                     request,
                     request.expectedGraphRevision,
                     "not_started",
                     "CADX_PRIMITIVE_INTEGRATION_REQUIRED",
                     "model.primitive requires a document-to-graph capture and scope resolver hook");
        return ToolResult::failure("CADX_PRIMITIVE_INTEGRATION_REQUIRED",
                                   "model.primitive is isolated; graph capture wiring is not installed");
    }
    auto* document = _hooks.activeDocument ? _hooks.activeDocument() : App::GetApplication().getActiveDocument();
    if (!document) {
        return ToolResult::failure("CADX_NO_ACTIVE_DOCUMENT", "there is no active FreeCAD document");
    }
    const auto scope = _hooks.resolveScope(document, request);
    if (!scope) {
        return ToolResult::failure("CADX_NO_GRAPH_SCOPE", "the integration layer could not resolve a graph scope");
    }
    const auto current = _graphs.current(*scope, false);
    if (request.expectedGraphRevision.empty() && current) {
        return ToolResult::failure("CADX_GRAPH_REVISION_MISMATCH",
                                   "an empty expected graph revision requires an unpublished graph scope",
                                   true);
    }
    if (!request.expectedGraphRevision.empty()
        && (!current || current.snapshot->header().graphRevision != request.expectedGraphRevision)) {
        return ToolResult::failure("CADX_GRAPH_REVISION_MISMATCH",
                                   "the expected graph revision is stale or unavailable",
                                   true);
    }
    const auto baseCapture = _hooks.captureGraph(document, nullptr, request, current.snapshot);
    if (!baseCapture) {
        return ToolResult::failure(baseCapture.errorCode.empty() ? "CADX_GRAPH_CAPTURE_FAILED" : baseCapture.errorCode,
                                   baseCapture.diagnostic);
    }
    if (baseCapture.scope.key() != scope->key()) {
        return ToolResult::failure("CADX_GRAPH_CAPTURE_FAILED", "graph capture returned a different scope");
    }
    if (!request.expectedGraphRevision.empty()
        && baseCapture.snapshot->header().graphRevision != request.expectedGraphRevision) {
        return ToolResult::failure("CADX_GRAPH_REVISION_MISMATCH", "CAD and graph base revisions differ", true);
    }
    if (current && current.snapshot->header().graphRevision != baseCapture.snapshot->header().graphRevision) {
        return ToolResult::failure("CADX_GRAPH_CAPTURE_FAILED", "the CAD graph changed during base capture", true);
    }

#ifndef CADX_HAVE_PART_DESIGN
    return ToolResult::failure("CADX_UNSUPPORTED_OBJECT", "PartDesign is not built");
#else
    std::unique_ptr<DocumentMutationTransaction> transaction;
    PartDesign::Body* body = nullptr;
    PartDesign::FeaturePrimitive* primitive = nullptr;
    try {
        transaction = std::make_unique<DocumentMutationTransaction>(document, "CadX model.primitive");
        body = document->addObject<PartDesign::Body>("Body");
        if (!body) {
            throw std::runtime_error("FreeCAD could not create a PartDesign Body");
        }
        body->Label.setValue(request.label.c_str());
        if (request.operation == "box") {
            auto* box = document->addObject<PartDesign::Box>("Box");
            primitive = box;
            box->Length.setValue(request.lengthMm);
            box->Width.setValue(request.widthMm);
            box->Height.setValue(request.heightMm);
        }
        else {
            auto* cylinder = document->addObject<PartDesign::Cylinder>("Cylinder");
            primitive = cylinder;
            cylinder->Radius.setValue(request.radiusMm);
            cylinder->Height.setValue(request.heightMm);
            cylinder->Angle.setValue(request.sweepDegrees);
        }
        if (!primitive) {
            throw std::runtime_error("FreeCAD could not create the primitive feature");
        }
        primitive->Label.setValue(request.label.c_str());
        const auto origin = expectedOrigin(request);
        primitive->Placement.setValue(Base::Placement(
            Base::Vector3d(origin.x, origin.y, origin.z),
            Base::Rotation(Base::Vector3d(request.rotation.axis.x,
                                          request.rotation.axis.y,
                                          request.rotation.axis.z),
                           request.rotation.angleDegrees * kPi / 180.0)));
        body->addObject(primitive);
        document->recompute({body, primitive}, true);
        std::string diagnostic;
        if (!verifyPhysicalPrimitive(request, body, primitive, diagnostic)) {
            throw std::runtime_error(diagnostic);
        }
        const auto candidateCapture = _hooks.captureGraph(document,
                                                           primitive,
                                                           request,
                                                           baseCapture.snapshot);
        if (!candidateCapture) {
            throw std::runtime_error(candidateCapture.diagnostic.empty()
                                         ? "affected graph capture failed"
                                         : candidateCapture.diagnostic);
        }
        if (candidateCapture.scope.key() != scope->key()
            || !validCapture(candidateCapture, document, primitive, request, diagnostic)) {
            throw std::runtime_error(diagnostic.empty() ? "affected graph capture failed" : diagnostic);
        }
        const auto bodyName = std::string(body->getNameInDocument());
        const auto featureName = std::string(primitive->getNameInDocument());
        const auto predictedDelta = sha256Revision(canonicalRequest(request)
                                                   + "|body=" + bodyName
                                                   + "|feature=" + featureName);
        const auto observedDelta = sha256Revision(canonicalSemantic(*candidateCapture.snapshot));
        transaction->commit();

        const auto committedCapture = _hooks.captureGraph(document,
                                                           primitive,
                                                           request,
                                                           baseCapture.snapshot);
        if (!committedCapture || committedCapture.scope.key() != scope->key()
            || !validCapture(committedCapture, document, primitive, request, diagnostic)
            || !equalGraphs(*candidateCapture.snapshot, *committedCapture.snapshot, diagnostic)) {
            const auto message = diagnostic.empty() ? "post-commit graph capture failed" : diagnostic;
            _graphs.markScopeStale(*scope, message);
            auditFailure(_audit,
                         "post_commit_verify",
                         toolName,
                         request,
                         baseCapture.snapshot->header().graphRevision,
                         "committed",
                         "CADX_GRAPH_CONSISTENCY_FAILURE",
                         message,
                         committedCapture.snapshot.get());
            return ToolResult::failure("CADX_GRAPH_CONSISTENCY_FAILURE", message, true);
        }
        std::string publishDiagnostic;
        if (_graphs.publishIfCurrent(*scope,
                                     committedCapture.snapshot,
                                     request.expectedGraphRevision,
                                     publishDiagnostic)
            != StoreError::None) {
            _graphs.markScopeStale(*scope, publishDiagnostic);
            auditFailure(_audit,
                         "publish",
                         toolName,
                         request,
                         request.expectedGraphRevision,
                         "committed",
                         "CADX_GRAPH_CONSISTENCY_FAILURE",
                         publishDiagnostic,
                         committedCapture.snapshot.get());
            return ToolResult::failure("CADX_GRAPH_CONSISTENCY_FAILURE", publishDiagnostic, true);
        }
        _audit.record(makeMutationAuditEvent("mutation",
                                             "passed",
                                             toolName,
                                             request.operationId,
                                             request.expectedGraphRevision,
                                             committedCapture.snapshot->header().graphRevision,
                                             predictedDelta,
                                             observedDelta,
                                             "valid",
                                             "committed",
                                             committedCapture.snapshot.get()));
        return receipt(request,
                       bodyName,
                       featureName,
                       request.expectedGraphRevision,
                       *committedCapture.snapshot,
                       predictedDelta,
                       observedDelta);
    }
    catch (const Base::Exception& exception) {
        if (transaction) transaction->abort();
        const auto message = exception.what();
        auditFailure(_audit, "cad_mutation", toolName, request,
                     request.expectedGraphRevision, "aborted",
                     "CADX_NATIVE_MUTATION_FAILED", message);
        return ToolResult::failure("CADX_NATIVE_MUTATION_FAILED", message);
    }
    catch (const std::exception& exception) {
        if (transaction) transaction->abort();
        auditFailure(_audit, "cad_mutation", toolName, request,
                     request.expectedGraphRevision, "aborted",
                     "CADX_NATIVE_MUTATION_FAILED", exception.what());
        return ToolResult::failure("CADX_NATIVE_MUTATION_FAILED", exception.what());
    }
#endif
}

}  // namespace CadX
