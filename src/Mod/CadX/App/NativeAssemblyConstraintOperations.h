// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "GraphAudit.h"
#include "GraphStore.h"
#include "GraphTypes.h"
#include "ToolResult.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace App
{
class Document;
class DocumentObject;
}  // namespace App

namespace CadX
{

struct ConstraintConnectorRequest
{
    std::string component;
    std::string connectorType;
    std::string connector;
    Placement offset;
    bool hasOffset = false;
};

struct GroundingRequest
{
    std::string operationId;
    std::string expectedGraphRevision;
    std::string assembly;
    std::vector<std::string> components;
    bool grounded = false;
    int expectedComponentCount = -1;
    int expectedGroundedCount = -1;
};

struct JointRequest
{
    std::string operationId;
    std::string expectedGraphRevision;
    std::string assembly;
    ConstraintConnectorRequest first;
    ConstraintConnectorRequest second;
    std::string jointType;
    std::string label;
    bool reverse = false;
    bool hasLimits = false;
    double minimumDegrees = 0.0;
    double maximumDegrees = 0.0;
};

// These parsers are deliberately independent of FreeCAD.  They are the
// strict boundary used by both the future ToolRegistry wiring and unit tests.
bool parseGroundingRequest(const std::string& argumentsJson,
                           const std::string& expectedOperation,
                           GroundingRequest& request,
                           std::string& diagnostic);
bool parseJointRequest(const std::string& argumentsJson,
                       JointRequest& request,
                       std::string& diagnostic);

// The bridge owns only the Python-backed Assembly object/proxy seam. It must
// never open/commit a document transaction or publish graph state.
class AssemblyConstraintBridge
{
public:
    virtual ~AssemblyConstraintBridge() = default;

    virtual bool createGroundedJoint(const std::string& documentName,
                                     const std::string& assemblyName,
                                     const std::string& componentName,
                                     std::string& createdJointName,
                                     std::string& diagnostic) const = 0;

    virtual bool createRegularJoint(const std::string& documentName,
                                    const std::string& assemblyName,
                                    const std::string& jointType,
                                    const std::string& label,
                                    const ConstraintConnectorRequest& first,
                                    const ConstraintConnectorRequest& second,
                                    bool reverse,
                                    std::string& createdJointName,
                                    std::string& diagnostic) const = 0;

    virtual bool verifyGroundedProxy(const std::string& documentName,
                                     const std::string& jointName,
                                     std::string& diagnostic) const = 0;

    virtual bool verifyRegularProxy(const std::string& documentName,
                                    const std::string& jointName,
                                    std::string& diagnostic) const = 0;
};

class NativeAssemblyConstraintOperations
{
public:
    NativeAssemblyConstraintOperations(GraphStore& graphs,
                                       GraphAuditLog& audit,
                                       std::shared_ptr<const AssemblyConstraintBridge> bridge = {});

    ToolResult execute(const std::string& toolName, const std::string& argumentsJson) const;

private:
    ToolResult executeGrounding(const GroundingRequest& request,
                                const std::string& toolName) const;
    ToolResult executeJoint(const JointRequest& request,
                            const std::string& toolName) const;

    GraphStore& _graphs;
    GraphAuditLog& _audit;
    std::shared_ptr<const AssemblyConstraintBridge> _bridge;
};

}  // namespace CadX
