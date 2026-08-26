// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "GraphStore.h"

#include <memory>
#include <string>

namespace CadX
{

// The observer owns invalidation state only.  FreeCAD callback wiring stays
// in the App service so callbacks never mutate an immutable snapshot.
class AssemblyDocumentObserver final
{
public:
    explicit AssemblyDocumentObserver(GraphStore& store, bool connectToFreeCad = false);
    ~AssemblyDocumentObserver();

    AssemblyDocumentObserver(const AssemblyDocumentObserver&) = delete;
    AssemblyDocumentObserver& operator=(const AssemblyDocumentObserver&) = delete;

    void documentChanged(const GraphScope& scope, const std::string& reason)
    {
        _store.markScopeStale(scope, reason);
    }

    void sourceDocumentChanged(const std::string& documentUid, const std::string& reason)
    {
        _store.markSourceDocumentStale(documentUid, reason);
    }

private:
    class FreeCadObserver;

    GraphStore& _store;
    std::unique_ptr<FreeCadObserver> _freeCadObserver;
};

}  // namespace CadX
