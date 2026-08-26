// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "AssemblyCapture.h"
#include "GraphAudit.h"
#include "GraphStore.h"
#include "ToolResult.h"

#include <memory>
#include <string>
#include <vector>

namespace App
{
class Document;
class DocumentObject;
}

namespace Assembly
{
class AssemblyObject;
}

namespace CadX
{

struct NativeGraphCaptureResult
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

struct NativeAssemblyCaptureResult
{
    AssemblyCapture capture;
    std::string errorCode;
    std::string diagnostic;

    explicit operator bool() const noexcept
    {
        return errorCode.empty() && !capture.documentUid.empty()
            && !capture.activeAssemblyObjectName.empty();
    }
};

// One transaction implementation is shared by every mutating native tool.
// Its destructor aborts any still-open transaction; callers must still call
// abort() explicitly on every pre-commit failure path.
class DocumentMutationTransaction
{
public:
    DocumentMutationTransaction(App::Document* document, const std::string& label);
    ~DocumentMutationTransaction();

    DocumentMutationTransaction(const DocumentMutationTransaction&) = delete;
    DocumentMutationTransaction& operator=(const DocumentMutationTransaction&) = delete;

    void commit();
    void abort() noexcept;
    bool closed() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

#ifdef CADX_HAVE_ASSEMBLY
NativeAssemblyCaptureResult captureNativeAssemblyCapture(
    App::Document* document,
    Assembly::AssemblyObject* assembly);

NativeGraphCaptureResult captureNativeAssemblyGraph(
    App::Document* document,
    Assembly::AssemblyObject* assembly);

NativeGraphCaptureResult captureNativeAssemblyGraph(
    App::Document* document,
    const std::string& assemblyObjectName);
#endif

bool equivalentGraphState(const GraphSnapshot& left,
                          const GraphSnapshot& right,
                          std::string& diagnostic);

bool loadMutationBase(GraphStore& graphs,
                      App::Document* document,
                      Assembly::AssemblyObject* assembly,
                      const std::string& expectedRevision,
                      std::shared_ptr<const GraphSnapshot>& base,
                      GraphScope& scope,
                      std::string& diagnostic);

std::string mutationDeltaHash(const GraphSnapshot* parent,
                              const GraphSnapshot& candidate);

ToolResult mutationReceipt(const std::string& schemaVersion,
                           const std::string& operation,
                           const std::string& operationId,
                           const std::string& parentRevision,
                           const GraphSnapshot& finalSnapshot,
                           const std::string& predictedDeltaHash,
                           const std::string& observedDeltaHash,
                           bool changed);

}  // namespace CadX
