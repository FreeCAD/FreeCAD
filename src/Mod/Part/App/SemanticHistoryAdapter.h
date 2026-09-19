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

/// B-Rep history adapter: maps OCCT Generated / Modified / Deleted onto
/// App::SemanticGraph events and SemanticBinding rows. FaceN is taken from history
/// (named indices) only. Sequential FaceN is never invented (I2 / I9 / I13).
///
/// Standalone g++ builds a fake history table. The OCCT path
/// (BRepTools_History) is compiled only when FC_USE_OCC is set and
/// SEMANTIC_TOPOLOGY_STANDALONE is not.

#include <App/SemanticId.h>
#include <App/SemanticReference.h>
#include <App/SemanticTopology.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

#ifndef SEMANTIC_TOPOLOGY_STANDALONE
# ifndef PartExport
#  include <Mod/Part/PartGlobal.h>
# endif
#else
# ifndef PartExport
#  define PartExport
# endif
#endif

namespace Part
{

inline bool isNamedIndex(const App::ElementIndex& idx)
{
    return !idx.type.empty() && idx.index > 0;
}

/// One OCCT (or fake) history row. `toIndex` is the named SemanticBinding address.
/// Empty `toIndex` means the adapter could not name the output — do not bind
/// a random FaceN and do not allocate a Generated identity for that row.
struct PartExport HistoryRecord
{
    App::SemanticId fromSeed;
    std::vector<App::SemanticId> extraSeeds;  ///< e.g. adjacent faces for FilletFace
    App::ElementIndex toIndex;
    App::EventKind kind = App::EventKind::Generated;
    App::SemanticKind outputKind = App::SemanticKind::Face;
    /// True for a maker output Face whose input had no semantic seed.
    /// Such rows are accepted only as unique 0-to-1 feature outputs.
    bool seedless = false;
    /// Maker input/history-kind group used to reject ambiguous seedless Face history.
    std::size_t sourceGroup = 0;
};

using HistoryTable = std::vector<HistoryRecord>;

struct PartExport ApplyResult
{
    std::size_t namedCount = 0;      ///< rows with a named toIndex
    std::size_t boundCount = 0;      ///< SemanticBinding rows written
    std::size_t skippedUnnamed = 0;  ///< Generated/Split skipped (no FaceN mint)
    std::vector<App::SemanticId> outputs;
    std::string note;
    bool halfMap = false;
};

/// R2 pre-maker result for FilletEdge. makerSkipped ⇒ do not run makeElementFillet.
struct PartExport FilletPreflight
{
    bool makerSkipped = false;
    App::ResolutionState state = App::ResolutionState::Missing;
    std::vector<App::ResolutionResult> perEdge;
    std::string note;
};

class PartExport SemanticHistoryAdapter
{
public:
    /// Map history rows onto the graph. Null graph is a no-op.
    /// Empty table → halfMap, no sequential FaceN, graph untouched.
    /// opcode is the Event.op token ("Pad" / "Fillet" / "Pocket" / "XTR" / "FLT").
    /// inputSeeds are the inbound named seeds; they are not scanned for a
    /// similar-length neighbour (I10).
    static ApplyResult applyHistory(
        App::SemanticGraph* graph,
        App::ObjectId feature,
        App::EvalSerial eval,
        const std::string& opcode,
        const std::vector<App::SemanticId>& inputSeeds,
        const HistoryTable& table
    );

    /// Resolve FilletEdge before the maker (R2). Missing / Incompatible /
    /// Ambiguous → makerSkipped. Null graph or empty edges → do not skip
    /// (FaceN dual-write fallback, I7). Never searches a neighbour (I10).
    static FilletPreflight preflightFillet(
        App::SemanticGraph* graph,
        const std::vector<App::SemanticId>& edges
    );

    /// C1/D2 restore rule for dress-ups: a valid cached Base may rebuild geometry
    /// when republishing, or when every named seed is only transiently Missing.
    /// Incompatible/Ambiguous results remain I13 failures; never choose a neighbour.
    static bool canUseCachedGeometry(const FilletPreflight& pre, bool semanticRepublish);

    /// Kind-agnostic Missing-check (Draft Face seeds). Empty list or null graph
    /// → do not skip (I7 FaceN fallback). Missing / Incompatible / Ambiguous
    /// → makerSkipped. Never searches a neighbour (I10).
    static FilletPreflight preflightNamedSeeds(
        App::SemanticGraph* graph,
        const std::vector<App::SemanticId>& seeds,
        App::SemanticKind kind
    );

    /// Draft/Thickness consume gate using the stored reference policy.
    static FilletPreflight preflightNamedReferences(
        App::SemanticGraph* graph,
        const std::vector<App::SemanticReference>& references,
        App::SemanticKind kind
    );

    /// `occHistory` is `const BRepTools_History*` when OCCT is present, else
    /// ignored. `inputShapes[].second` is `const TopoDS_Shape*`. `indexOf`
    /// must return a named ElementIndex or empty (no FaceN guess).
    static HistoryTable fromOcctHistory(
        const void* occHistory,
        const std::vector<std::pair<App::SemanticId, const void*>>& inputShapes,
        const std::function<App::ElementIndex(const void* occShape)>& indexOf
    );

    /// `occMaker` is `const BRepBuilderAPI_MakeShape*` when OCCT is present
    /// (prism / fillet / until-prism). Same contract as fromOcctHistory.
    /// An invalid seed is accepted for a live dress-up maker; only a uniquely
    /// located generated Face is later published.
    /// indexOf must return a named ElementIndex or empty (no FaceN guess).
    static HistoryTable fromMaker(
        const void* occMaker,
        const std::vector<std::pair<App::SemanticId, const void*>>& inputShapes,
        const std::function<App::ElementIndex(const void* occShape)>& indexOf
    );

    /// Input still lives on the result (OCCT Unmodified / mapped :U). Identity
    /// continues as Modified with a named toIndex. Empty index → no row (I13).
    static bool appendUnmodifiedSurvivor(
        HistoryTable& table,
        const App::SemanticId& seed,
        const App::ElementIndex& toIndex
    );

    /// I13 for Boolean (and similar) emit: keep a row only when that fromSeed
    /// has exactly one named Face/Edge toIndex. 0 or >1 images, Deleted, and
    /// unnamed rows are dropped (never first-Binding-wins; no sequential FaceN).
    /// Survivors are rewritten Generated so Binding.feature / allocatedBy is
    /// the Boolean feature, not a Base leftover.
    static HistoryTable uniqueOneImageGenerated(const HistoryTable& table);

    /// Fill gaps left when fromMaker uniquely covers Faces but not Edges
    /// (MakePrism/MakeRevol vertical edges are often unmodified survivors).
    /// Inputs must already be unique-located published slots. Does not remint
    /// slots already present in unique. Pass-32 callers that refuse full locate
    /// when makerHistory is non-empty and unique is empty should not call this
    /// in that refuse path.
    static HistoryTable supplementLocatedInputs(
        const HistoryTable& unique,
        const std::vector<std::pair<App::SemanticId, const void*>>& inputs,
        const std::function<App::ElementIndex(const void* occShape)>& indexOf
    );

    static const std::string& lastApplyNote();
};

}  // namespace Part
