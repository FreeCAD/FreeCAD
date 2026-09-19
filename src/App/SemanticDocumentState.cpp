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

#include "SemanticDocumentState.h"

#include <charconv>
#include <sstream>
#include <unordered_map>

namespace App
{
namespace
{

std::unordered_map<const void*, SemanticDocumentState*>& registry()
{
    static std::unordered_map<const void*, SemanticDocumentState*> map;
    return map;
}

void registerPointer(const void* p, SemanticDocumentState* state)
{
    if (!p || !state) {
        return;
    }
    registry()[p] = state;
}

void unregisterPointer(const void* p, SemanticDocumentState* state)
{
    if (!p) {
        return;
    }
    auto it = registry().find(p);
    if (it != registry().end() && it->second == state) {
        registry().erase(it);
    }
}

}  // namespace

SemanticDocumentState::~SemanticDocumentState()
{
    unbindOwner();
}

EvalSerial SemanticDocumentState::beginEvaluate()
{
    if (evaluating_) {
        graph_.abortEvaluate();
        evaluating_ = false;
    }
    currentEval_ = nextEval_++;
    evaluating_ = true;
    graph_.beginEvaluate(currentEval_);
    return currentEval_;
}

void SemanticDocumentState::commitEvaluate()
{
    if (!evaluating_) {
        return;
    }
    graph_.commitEvaluate();
    evaluating_ = false;
}

void SemanticDocumentState::abortEvaluate()
{
    if (!evaluating_) {
        return;
    }
    graph_.abortEvaluate();
    evaluating_ = false;
}

SemanticDocumentState::EvaluateScope::EvaluateScope(SemanticDocumentState& state)
    : state_(state)
{
    state_.beginEvaluate();
}

SemanticDocumentState::EvaluateScope::~EvaluateScope()
{
    if (!done_) {
        state_.abortEvaluate();
    }
}

void SemanticDocumentState::EvaluateScope::commit()
{
    if (done_) {
        return;
    }
    state_.commitEvaluate();
    done_ = true;
}

void SemanticDocumentState::EvaluateScope::abort()
{
    if (done_) {
        return;
    }
    state_.abortEvaluate();
    done_ = true;
}

void SemanticDocumentState::restoreSnapshot(const SemanticGraph::Snapshot& snap)
{
    if (evaluating_) {
        graph_.abortEvaluate();
        evaluating_ = false;
    }
    graph_.restorePublished(snap);
}

void SemanticDocumentState::markUndoPoint()
{
    undoStack_.push_back(graph_.snapshotPublished());
    redoStack_.clear();
}

void SemanticDocumentState::undoGraph()
{
    if (undoStack_.empty()) {
        return;
    }
    SemanticGraph::Snapshot current = graph_.snapshotPublished();
    restoreSnapshot(undoStack_.back());
    undoStack_.pop_back();
    redoStack_.push_back(std::move(current));
}

void SemanticDocumentState::redoGraph()
{
    if (redoStack_.empty()) {
        return;
    }
    SemanticGraph::Snapshot current = graph_.snapshotPublished();
    restoreSnapshot(redoStack_.back());
    redoStack_.pop_back();
    undoStack_.push_back(std::move(current));
}

void SemanticDocumentState::abortGraph()
{
    if (undoStack_.empty()) {
        return;
    }
    restoreSnapshot(undoStack_.back());
    undoStack_.pop_back();
    redoStack_.clear();
}

void SemanticDocumentState::clearRedoGraph()
{
    redoStack_.clear();
}

void SemanticDocumentState::clearUndoGraph()
{
    undoStack_.clear();
    redoStack_.clear();
}

void SemanticDocumentState::trimUndoGraph(std::size_t maxSize)
{
    if (maxSize == 0) {
        undoStack_.clear();
        return;
    }
    while (undoStack_.size() > maxSize) {
        undoStack_.erase(undoStack_.begin());
    }
}

void SemanticDocumentState::reset()
{
    if (evaluating_) {
        graph_.abortEvaluate();
        evaluating_ = false;
    }
    graph_ = SemanticGraph();
    nextEval_ = 1;
    currentEval_ = 0;
    undoStack_.clear();
    redoStack_.clear();
    // G3: drop feature aliases (keep owner). Objects are about to die.
    for (const void* a : aliases_) {
        unregisterPointer(a, this);
    }
    aliases_.clear();
}

void SemanticDocumentState::bindOwner(const void* owner)
{
    unbindOwner();
    owner_ = owner;
    registerPointer(owner_, this);
}

void SemanticDocumentState::bindAlias(const void* feature)
{
    if (!feature) {
        return;
    }
    aliases_.insert(feature);
    registerPointer(feature, this);
}

void SemanticDocumentState::unbindAlias(const void* feature)
{
    if (!feature) {
        return;
    }
    aliases_.erase(feature);
    unregisterPointer(feature, this);
}

void SemanticDocumentState::unbindOwner()
{
    unregisterPointer(owner_, this);
    owner_ = nullptr;
    for (const void* a : aliases_) {
        unregisterPointer(a, this);
    }
    aliases_.clear();
}

SemanticGraph* SemanticDocumentState::graphFor(const void* featureOrDocument)
{
    SemanticDocumentState* state = stateFor(featureOrDocument);
    return state ? &state->graph() : nullptr;
}

SemanticDocumentState* SemanticDocumentState::stateFor(const void* featureOrDocument)
{
    if (!featureOrDocument) {
        return nullptr;
    }
    auto it = registry().find(featureOrDocument);
    if (it == registry().end()) {
        return nullptr;
    }
    return it->second;
}


std::string SemanticDocumentState::serialize() const
{
    std::ostringstream ss;
    ss << "STD1\n";
    ss << "nextEval " << nextEval_ << "\n";
    ss << "GRAPH\n";
    ss << graph_.serialize();
    return ss.str();
}

bool SemanticDocumentState::deserialize(std::string_view text)
{
    std::istringstream in{std::string(text)};
    std::string line;
    if (!std::getline(in, line) || line != "STD1") {
        return false;
    }
    EvalSerial restoredNext = 1;
    if (!std::getline(in, line)) {
        return false;
    }
    {
        std::istringstream ls(line);
        std::string tag;
        std::string serialText;
        ls >> tag >> serialText;
        if (tag != "nextEval" || serialText.empty()) {
            return false;
        }
        const auto parsed = std::from_chars(
            serialText.data(), serialText.data() + serialText.size(), restoredNext);
        std::string trailing;
        if (parsed.ec != std::errc{} || parsed.ptr != serialText.data() + serialText.size()
            || restoredNext == 0 || (ls >> trailing)) {
            return false;
        }
    }
    if (!std::getline(in, line) || line != "GRAPH") {
        return false;
    }
    std::string rest;
    std::string more;
    while (std::getline(in, more)) {
        rest += more;
        rest += '\n';
    }
    // Deserialize into a candidate so a malformed document section cannot
    // partially replace the live graph. Aborting the candidate first burns
    // in-flight handles (I5); graph deserialize then replaces the published
    // set (C1) while keeping abandoned live handles burned.
    SemanticGraph restored = graph_;
    restored.abortEvaluate();
    if (!restored.deserialize(rest)) {
        return false;
    }
    if (evaluating_) {
        graph_.abortEvaluate();
    }
    graph_ = std::move(restored);
    if (restoredNext > nextEval_) {
        nextEval_ = restoredNext;
    }
    currentEval_ = 0;
    evaluating_ = false;
    undoStack_.clear();
    redoStack_.clear();
    return true;
}

std::string SemanticDocumentState::hexPayload() const
{
    return hexEncode(serialize());
}

bool SemanticDocumentState::restoreOptionalHexPayload(bool present, std::string_view hexPayload)
{
    if (!present) {
        return true;
    }
    if (hexPayload.empty()) {
        return true;
    }
    const std::string decoded = hexDecode(hexPayload);
    if (decoded.empty()) {
        return false;
    }
    return deserialize(decoded);
}

std::string SemanticDocumentState::hexEncode(std::string_view text)
{
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.resize(text.size() * 2);
    for (std::size_t i = 0; i < text.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        out[2 * i] = hex[c >> 4];
        out[2 * i + 1] = hex[c & 0x0f];
    }
    return out;
}

std::string SemanticDocumentState::hexDecode(std::string_view hex)
{
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        return -1;
    };
    if (hex.size() % 2 != 0) {
        return {};
    }
    std::string out;
    out.resize(hex.size() / 2);
    for (std::size_t i = 0; i < out.size(); ++i) {
        const int hi = nibble(hex[2 * i]);
        const int lo = nibble(hex[2 * i + 1]);
        if (hi < 0 || lo < 0) {
            return {};
        }
        out[i] = static_cast<char>((hi << 4) | lo);
    }
    return out;
}

}  // namespace App
