// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "GraphAudit.h"
#include "GraphStore.h"
#include "ToolResult.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace App
{
class Document;
class DocumentObject;
}

namespace CadX
{

struct PrimitiveVector
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct PrimitiveRotation
{
    PrimitiveVector axis {0.0, 0.0, 1.0};
    double angleDegrees = 0.0;
};

struct PrimitiveRequest
{
    std::string operation;
    std::string operationId;
    std::string expectedGraphRevision;
    std::string label;
    PrimitiveVector center;
    PrimitiveRotation rotation;
    double lengthMm = 0.0;
    double widthMm = 0.0;
    double heightMm = 0.0;
    double radiusMm = 0.0;
    double sweepDegrees = 360.0;
};

struct PrimitiveParseResult
{
    bool ok = false;
    PrimitiveRequest request;
    std::string errorCode;
    std::string diagnostic;

    explicit operator bool() const noexcept { return ok; }
};

struct PrimitivePreflightResult
{
    bool ok = false;
    std::string errorCode;
    std::string diagnostic;

    explicit operator bool() const noexcept { return ok; }
};

// The App layer has no reusable public CAD-to-graph capture hook for a
// standalone model primitive. The integration layer supplies this callback.
// It must return a complete immutable graph snapshot and include the changed
// primitive as a typed PrimitivePayload node.
struct PrimitiveGraphCapture
{
    GraphScope scope;
    std::shared_ptr<GraphSnapshot> snapshot;
    std::string errorCode;
    std::string diagnostic;

    explicit operator bool() const noexcept
    {
        return snapshot != nullptr && errorCode.empty();
    }
};

using PrimitiveScopeResolver = std::function<std::optional<GraphScope>(
    App::Document*, const PrimitiveRequest&)>;
using PrimitiveGraphCaptureFunction = std::function<PrimitiveGraphCapture(
    App::Document*,
    App::DocumentObject*,
    const PrimitiveRequest&,
    std::shared_ptr<const GraphSnapshot>)>;

struct PrimitiveOperationHooks
{
    // If empty, execution falls back to FreeCAD's active document.
    std::function<App::Document*()> activeDocument;
    PrimitiveScopeResolver resolveScope;
    PrimitiveGraphCaptureFunction captureGraph;
};

class NativePrimitiveOperations
{
public:
    NativePrimitiveOperations(GraphStore& graphs,
                              GraphAuditLog& audit,
                              PrimitiveOperationHooks hooks = {});

    static PrimitiveParseResult parseRequest(const std::string& argumentsJson);
    static PrimitivePreflightResult preflight(const PrimitiveRequest& request);

    // Returns the origin of the FreeCAD primitive placement. VibeCAD centers
    // primitives at center_mm, so this subtracts the rotated local center.
    static PrimitiveVector expectedOrigin(const PrimitiveRequest& request);

    ToolResult execute(const std::string& toolName,
                       const std::string& argumentsJson) const;

private:
    GraphStore& _graphs;
    GraphAuditLog& _audit;
    PrimitiveOperationHooks _hooks;
};

}  // namespace CadX
