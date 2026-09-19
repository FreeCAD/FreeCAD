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

/// Bridge SketchEntityHandle → document-graph SemanticId (I8: one stack).
/// The sketch handle is a note on the Generated/Split event, not a second
/// identity heap. Allocation uses SemanticGraph::allocator only.

#include "SketchEntityId.h"

#include <App/SemanticId.h>
#include <App/SemanticTopology.h>

#include <string>
#include <utility>
#include <string_view>
#include <vector>

namespace Sketcher
{

/// Unique profile corner. Note is Sketch.v{handle}:{pos} on the event;
/// not a second identity heap (I8). pos is PointPos start=1 / end=2.
struct SketcherExport SketchVertexKey
{
    SketchEntityHandle handle = 0;
    int pos = 0;

    constexpr bool valid() const
    {
        return handle > 0 && pos > 0;
    }

    constexpr bool operator==(const SketchVertexKey& other) const
    {
        return handle == other.handle && pos == other.pos;
    }

    constexpr bool operator<(const SketchVertexKey& other) const
    {
        return handle < other.handle || (handle == other.handle && pos < other.pos);
    }
};

/// Optional 3d point parallel to an endpoint list (Precision::Confusion).
struct SketcherExport SketchVertexPoint
{
    double x = 0;
    double y = 0;
    double z = 0;
};

/// Profile seeds published on the document graph for one sketch.
struct SketcherExport SketchProfileSeeds
{
    std::vector<App::SemanticId> curves;     ///< kind Edge, slot order
    std::vector<App::SemanticId> regions;    ///< kind Region, regionKey order
    std::vector<App::SemanticId> vertices;   ///< kind Vertex, unique corners
    std::vector<SketchEntityHandle> curveHandles;
    std::vector<std::string> regionKeys;
    std::vector<SketchVertexKey> vertexKeys;
};

class SketcherExport SketchSemanticSeeds
{
public:
    /// Event.op note for a curve/point entity. Not a SemanticHandle.
    static std::string entityNote(SketchEntityHandle handle);
    /// Event.op note for a unique profile corner. Distinct from entityNote
    /// (Sketch.gN) so Vertex find does not return the Edge identity.
    static std::string vertexNote(SketchEntityHandle handle, int pos);
    static std::string vertexNote(const SketchVertexKey& key);
    /// Event.op note for an internal-face region (`Sketch.` + regionKey).
    static std::string regionNote(std::string_view regionKey);

    /// Allocate or reuse the document-graph seed for this sketch entity.
    /// Kind Edge for curves. Vertex corners use ensureSeedForVertex (distinct
    /// note + Binding type "Vertex", not type "g"). Never recycles the sketch
    /// handle and never mints a second heap.
    static App::SemanticId ensureSeedForEntity(App::SemanticGraph& graph,
                                               App::ObjectId sketch,
                                               App::EvalSerial eval,
                                               SketchEntityHandle handle,
                                               App::SemanticKind kind = App::SemanticKind::Edge);

    /// Allocate or reuse a Vertex seed for a unique profile corner (C1).
    /// Binding index.type is "Vertex" (never "g"); index packs handle+pos.
    static App::SemanticId ensureSeedForVertex(App::SemanticGraph& graph,
                                               App::ObjectId sketch,
                                               App::EvalSerial eval,
                                               const SketchVertexKey& key);

    static App::SemanticId ensureRegionSeed(App::SemanticGraph& graph,
                                            App::ObjectId sketch,
                                            App::EvalSerial eval,
                                            std::string_view regionKey);

    /// Dedup start/end endpoints into unique corners. Coincident (handle,pos)
    /// pairs merge; optional parallel points merge within `confusion`.
    /// Canonical key is min (handle, pos) in the component. Not a vertex heap.
    static std::vector<SketchVertexKey> uniqueProfileCorners(
        const std::vector<SketchVertexKey>& endpoints,
        const std::vector<std::pair<SketchVertexKey, SketchVertexKey>>& coincidences,
        const std::vector<SketchVertexPoint>& points = {},
        double confusion = 1e-7);

    /// Ensure curve seeds in `liveCurves` order, region seeds, and Vertex
    /// seeds for already-deduped `uniqueCorners` (rectangle → 4, not 8).
    static SketchProfileSeeds seedsForProfile(
        App::SemanticGraph& graph,
        App::ObjectId sketch,
        App::EvalSerial eval,
        const std::vector<SketchEntityHandle>& liveCurves,
        const std::vector<std::string>& regionKeys,
        const std::vector<SketchVertexKey>& uniqueCorners = {});

    /// Curve-handle lookup (Sketch.gN / split notes). Does not return Vertex.
    static App::SemanticId findSeed(const App::SemanticGraph& graph,
                                    App::ObjectId sketch,
                                    SketchEntityHandle handle);
    /// Vertex lookup by kind + note (Sketch.v{handle}:{pos}).
    static App::SemanticId findSeedForVertex(const App::SemanticGraph& graph,
                                             App::ObjectId sketch,
                                             const SketchVertexKey& key);
    static App::SemanticId findSeedByKindNote(const App::SemanticGraph& graph,
                                              App::ObjectId sketch,
                                              App::SemanticKind kind,
                                              std::string_view note);
    static App::SemanticId findRegionSeed(const App::SemanticGraph& graph,
                                          App::ObjectId sketch,
                                          std::string_view regionKey);

    /// Record Deleted for a retired handle that already had a seed. No-op
    /// if the handle never published or is already Deleted.
    static void recordRetired(App::SemanticGraph& graph,
                              App::ObjectId sketch,
                              App::EvalSerial eval,
                              SketchEntityHandle handle);

    /// Topological split of a sketch entity: parent becomes historical,
    /// children are the Split outputs. Child handles are notes, not new
    /// Generated identities (I8).
    /// Wired (S4-S1): SketchObject::split / multi-piece trim call this via
    /// replaceGeometriesRecordingSplit before Geometry mutation. Profile
    /// collect (collectSketchProfileSeeds) accepts Split outputs so Pad still
    /// gets seeds. recordRetired skips Split parents (historical via Split,
    /// not Deleted).
    static std::vector<App::SemanticId> splitEntity(App::SemanticGraph& graph,
                                                    App::ObjectId sketch,
                                                    App::EvalSerial eval,
                                                    SketchEntityHandle parent,
                                                    const std::vector<SketchEntityHandle>& children);

    static constexpr const char* NotePrefix = "Sketch.";
    static constexpr const char* SplitPrefix = "Sketch.split:";
};

}  // namespace Sketcher
