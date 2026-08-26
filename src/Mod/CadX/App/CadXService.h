// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "AssemblyDocumentObserver.h"
#include "AssemblyGraphBuilder.h"
#include "GraphAudit.h"
#include "MainThreadGateway.h"
#include "NativeAssemblyConstraintOperations.h"
#include "NativeAssemblyOperations.h"
#ifdef CADX_HAVE_PART_DESIGN
#include "NativePrimitiveOperations.h"
#endif
#include "ToolRegistry.h"

#include <memory>
#include <string>

namespace CadX
{

class CadXService
{
public:
    explicit CadXService(bool observeFreeCadDocuments = false);

    ToolRegistry& toolRegistry() noexcept { return _tools; }
    GraphStore& graphStore() noexcept { return _graphs; }
    const GraphAuditLog& auditLog() const noexcept { return _audit; }

    ToolResult executeTool(const std::string& name, const std::string& argumentsJson) const;
    ToolResult publishCapture(const AssemblyCapture& capture);
    ToolResult exportGraphEvidence(const std::string& graphId,
                                   const std::string& graphRevision) const;
    bool registerGuiSnapshotProvider(ToolExecutor provider, std::string& diagnostic);
    ToolResult summarizeSnapshot(const GraphSnapshot& snapshot) const;

private:
    ToolResult executeQuery(const std::string& argumentsJson) const;
    void registerTools();

    mutable GraphStore _graphs;
    mutable ToolRegistry _tools;
    mutable GraphAuditLog _audit;
    AssemblyGraphBuilder _builder;
    AssemblyDocumentObserver _observer;
    MainThreadGateway _gateway;
    NativeAssemblyOperations _assemblyMutations;
#ifdef CADX_HAVE_PART_DESIGN
    NativePrimitiveOperations _primitiveMutations;
#endif
    NativeAssemblyConstraintOperations _constraintMutations;
};

CadXService& service();

}  // namespace CadX
