// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Sauli Kiviranta                                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

/// Per-document owner of SemanticGraph + eval serial. Compiles without Qt.
/// Document holds one of these (lifetime = document). Allocator high-water is
/// never rewound on abort / undo (I5). graphFor looks up a bound owner or
/// feature alias — no second identity heap (I8).

#include "SemanticTopology.h"

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace App
{

class AppExport SemanticDocumentState
{
public:
    SemanticDocumentState() = default;
    ~SemanticDocumentState();

    SemanticDocumentState(const SemanticDocumentState&) = delete;
    SemanticDocumentState& operator=(const SemanticDocumentState&) = delete;

    SemanticGraph& graph()
    {
        return graph_;
    }
    const SemanticGraph& graph() const
    {
        return graph_;
    }

    EvalSerial currentEval() const
    {
        return currentEval_;
    }
    EvalSerial nextEvalSerial() const
    {
        return nextEval_;
    }
    bool isEvaluating() const
    {
        return evaluating_;
    }

    /// Increment eval serial, then graph.beginEvaluate. Nested begin aborts
    /// the in-flight eval first (burns those handles, I5).
    EvalSerial beginEvaluate();
    void commitEvaluate();
    void abortEvaluate();

    /// RAII: abort on scope exit unless commit() was called. Failed evaluate
    /// burns in-flight handles; high-water is not rewound.
    class AppExport EvaluateScope
    {
    public:
        explicit EvaluateScope(SemanticDocumentState& state);
        ~EvaluateScope();

        EvaluateScope(const EvaluateScope&) = delete;
        EvaluateScope& operator=(const EvaluateScope&) = delete;

        EvalSerial eval() const
        {
            return state_.currentEval();
        }
        void commit();
        void abort();

    private:
        SemanticDocumentState& state_;
        bool done_ = false;
    };

    /// Undo snapshot of the published graph. Allocator high-water is not
    /// stored; restorePublished leaves it in place (I5).
    void markUndoPoint();
    void undoGraph();
    void redoGraph();
    /// Restore the last mark and drop it (transaction abort, not undo).
    void abortGraph();
    void clearRedoGraph();
    void clearUndoGraph();
    void trimUndoGraph(std::size_t maxSize);

    std::size_t undoDepth() const
    {
        return undoStack_.size();
    }
    std::size_t redoDepth() const
    {
        return redoStack_.size();
    }

    /// Reload / new document: fresh graph. Not undo — new document identity.
    /// Drops feature aliases and unbinds them from the registry. Owner remains.
    void reset();

    /// Register an opaque owner (Document*) and optional feature aliases
    /// (DocumentObject*) so SemanticEmitter::graphFor can find this graph.
    void bindOwner(const void* owner);
    void bindAlias(const void* feature);
    void unbindAlias(const void* feature);
    void unbindOwner();

    const void* owner() const
    {
        return owner_;
    }

    /// Lookup. Null input → nullptr. Unknown pointer → nullptr.
    static SemanticGraph* graphFor(const void* featureOrDocument);
    static SemanticDocumentState* stateFor(const void* featureOrDocument);

    /// STG1 payload: graph tables + eval high-water. Bindings and undo
    /// stacks are session-only. Dual-write / optional Document section.
    std::string serialize() const;
    bool deserialize(std::string_view text);

    static std::string hexEncode(std::string_view text);
    static std::string hexDecode(std::string_view hex);

    /// Hex-encoded STD1 for Document Save: payload="...".
    std::string hexPayload() const;

    /// Optional FCStd <SemanticGraph> restore. !present is a no-op
    /// (pre-migration files). present: hexDecode + deserialize.
    /// Does not mint seeds from FaceN (I13). Bindings stay empty (not in
    /// STG1). High-water is not rewound (I5).
    bool restoreOptionalHexPayload(bool present, std::string_view hexPayload);

private:
    SemanticGraph graph_;
    EvalSerial nextEval_ = 1;
    EvalSerial currentEval_ = 0;
    bool evaluating_ = false;
    const void* owner_ = nullptr;
    std::unordered_set<const void*> aliases_;
    std::vector<SemanticGraph::Snapshot> undoStack_;
    std::vector<SemanticGraph::Snapshot> redoStack_;

    void restoreSnapshot(const SemanticGraph::Snapshot& snap);
};

}  // namespace App
