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

/// Shared I13/C1 Part semantic helpers for Boolean / Primitive / Transformed
/// source-seed collection, Extrusion/Revolution/Mirroring published-shape
/// locate, PartDesign unique-partner/coplanar/geometric/same-curve locate,
/// PartDesign Boolean/Primitive/Transformed indexOnPublished, Extrude/Part
/// Boolean partner-coplanar, SketchBased partner-sameCurve, Extrude :U
/// partner-edge-with-exclude, Pipe/Loft/Helix sew+shared-vertex published
/// locate, Pipe/Loft/Helix Generated-then-Modified maker-history sweeps
/// (and Extrude Modified-first edge images), Pipe/Loft/Helix Pad-fallback
/// Z-parallel Face-rail edge append, Pipe/Loft/Helix fromMaker empty-Face
/// Generated fallback, and Pipe/Loft/Helix refreshNamedIndices seeded-shape
/// to published-index mapping. FaceN is
/// never identity: only published, unique, shape-resolvable Bindings become
/// inputs; partner locate stays unnamed on 0 or >1 (DESIGN castle / C1 / I13).

#include <Mod/Part/PartGlobal.h>

#include <App/SemanticId.h>
#include <App/SemanticTopology.h>

#include "TopoShape.h"

#include <deque>
#include <unordered_set>
#include <utility>
#include <vector>

#include <TopoDS_Shape.hxx>
#include <TopAbs_ShapeEnum.hxx>

class BRepBuilderAPI_MakeShape;
class BRepBuilderAPI_Sewing;

namespace App
{
class DocumentObject;
}

namespace Part
{

/// Walk feature-scoped Face/Edge Bindings on `obj`. Require published + not
/// deleted + uniqueBindingOnFeature + non-null findShape, then insert into
/// caller `seenSeeds` for cross-call / cross-operand dedup before appending
/// to `held` / `inputs`. Early invalid candidates must not hide a later valid
/// binding for the same handle. Does not mint.
PartExport void collectUniqueSourceSeeds(
    App::SemanticGraph* graph,
    App::DocumentObject* obj,
    const TopoShape& sourceShape,
    std::deque<TopoDS_Shape>& held,
    std::vector<std::pair<App::SemanticId, const void*>>& inputs,
    std::unordered_set<App::SemanticHandle>& seenSeeds);

/// Locate a Face/Edge on `published`: prefer findShape, else the unique
/// IsPartner match on the published solid (I13 — 0 or >1 partners stay
/// unnamed). Returns an empty ElementIndex when unnamed. Does not mint.
PartExport App::ElementIndex uniqueNamedIndexOnPublished(
    const TopoShape& published,
    const TopoDS_Shape& image);

/// Unique IsPartner (or IsSame) of `sub` on `owner` after findShape misses
/// (Refine). EDGE and FACE only. Dedup orientations. 1 → owner's findShape
/// index; 0 or N stay unnamed (I13). Does not mint.
PartExport int uniquePartnerIndex(const TopoShape& owner, const TopoDS_Shape& sub);

/// Unique coplanar planar Face of `sub` on `owner` after findShape/partner
/// miss (Refine). Plane faces only. Dedup IsSame/IsPartner. 1 → owner's
/// findShape index; 0 or N stay unnamed (I13). Does not mint.
PartExport int uniqueCoplanarFaceIndex(const TopoShape& owner, const TopoDS_Shape& sub);

/// Unique geometric Edge of `sub` on `owner` after findShape/partner miss
/// (Refine). Endpoint-distance match within Precision::Confusion, either
/// orientation; dedup IsSame/IsPartner. 1 → owner's findShape index; 0 or N
/// stay unnamed (I13). Does not mint.
PartExport int uniqueGeometricEdgeIndex(const TopoShape& owner, const TopoDS_Shape& sub);

/// Unique same-curve Edge of `sub` on `owner` after findShape/partner miss
/// (Refine). EDGE only. GeomAbs_Line by origin+direction (or opposite);
/// Circle by radius+coaxial axis; else same endpoints. IsSame/IsPartner
/// dedup. 1 → findShape else 0 (I13). Does not mint.
PartExport int uniqueSameCurveEdgeIndex(const TopoShape& owner, const TopoDS_Shape& sub);

/// Locate Face/Edge on `owner`: findShape, else uniquePartnerIndex, else
/// uniqueCoplanarFaceIndex (FACE) / uniqueGeometricEdgeIndex (EDGE).
/// Null / n<=0 / other shape types → empty ElementIndex (I13). Does not mint.
/// Matches PartDesign Boolean / Primitive / Transformed locate chain.
PartExport App::ElementIndex indexOnPublished(const TopoShape& owner, const TopoDS_Shape& sub);

/// Locate Face/Edge/Vertex on `owner`: findShape, else uniquePartnerIndex,
/// else uniqueCoplanarFaceIndex (FACE only). No geometric Edge / same-curve
/// fallthrough. Null / n<=0 / other shape types → empty ElementIndex (I13).
/// Does not mint. Matches FeatureExtrude elementIndexOf and Part Boolean
/// partner+coplanar locate (distinct from indexOnPublished).
PartExport App::ElementIndex indexOnPublishedPartnerCoplanar(const TopoShape& owner,
                                                             const TopoDS_Shape& sub);

/// Locate Face/Edge on `owner`: findShape, else uniquePartnerIndex, else
/// uniqueCoplanarFaceIndex (FACE) / uniqueSameCurveEdgeIndex (EDGE).
/// Null / n<=0 / other shape types → empty ElementIndex (I13). Does not mint.
/// Matches FeatureSketchBased sweepElementIndexOf (distinct from
/// indexOnPublished and indexOnPublishedPartnerCoplanar).
PartExport App::ElementIndex indexOnPublishedPartnerSameCurve(const TopoShape& owner,
                                                              const TopoDS_Shape& sub);

/// Unique IsSame/IsPartner EDGE of `input` on `result`, excluding any
/// IsSame/IsPartner of non-null `excludeStashed` entries (Extrude :U bottom
/// outline after top was stashed). Orientation IsSame/IsPartner dedup.
/// size==1 → that result EDGE; else Null (I13). Does not mint.
PartExport TopoDS_Shape uniquePartnerEdgeExcluding(
    const TopoDS_Shape& input,
    const TopoShape& result,
    const std::vector<TopoDS_Shape>& excludeStashed);

/// Non-null IsSame check for OCCT shapes (Pipe/Loft/Helix/Extrude stash
/// dedup). Does not mint.
PartExport bool sameOccShape(const TopoDS_Shape& a, const TopoDS_Shape& b);

/// Count Generated(input) images whose ShapeType equals `t`. Null maker/input
/// → 0. Pipe/Loft/Helix diagnostics only; does not mint.
PartExport int countGeneratedOf(BRepBuilderAPI_MakeShape* maker,
                                const TopoDS_Shape& input,
                                TopAbs_ShapeEnum t);

/// Count ElementIndex entries that pass Part::isNamedIndex (non-empty type,
/// index>0). Pipe/Loft/Helix named-slot diagnostics; does not mint.
PartExport std::size_t namedIndexCount(const std::vector<App::ElementIndex>& xs);

/// Unique EDGE images of `input` through maker history: Generated first, else
/// Modified. Caller treats size!=1 as unnamed (I13). Used by Pipe/Loft/Helix.
/// Distinct from uniqueModifiedThenGeneratedEdgeImages (Extrude Modified-first
/// + findShape + partner fallthrough) — do not merge those contracts.
PartExport std::vector<TopoDS_Shape> uniqueGeneratedThenModifiedEdgeImages(
    BRepBuilderAPI_MakeShape* maker,
    const TopoDS_Shape& input);

/// Unique EDGE images of `input` through maker history: Modified first, else
/// Generated, else the input if it still lives on `result`, else a unique
/// IsSame/IsPartner EDGE on `result` (orientation dedup, size==1). Caller
/// treats size!=1 as unnamed (I13). Used by FeatureExtrude. Distinct from
/// uniqueGeneratedThenModifiedEdgeImages — do not merge those contracts.
/// Does not mint.
PartExport std::vector<TopoDS_Shape> uniqueModifiedThenGeneratedEdgeImages(
    BRepBuilderAPI_MakeShape* maker,
    const TopoDS_Shape& input,
    const TopoShape& result);

/// Unique 1-image FACE from Generated(input). 0 or N → Null (I13).
/// Pipe/Loft/Helix side-face capture. Does not mint.
PartExport TopoDS_Shape uniqueGeneratedFace(BRepBuilderAPI_MakeShape* maker,
                                            const TopoDS_Shape& input);

/// Unique Z-parallel Edge of a published Face (Loft/Pipe/Helix side vertical
/// rail). TopExp edges of that Face; GeomAbs_Line parallel to +Z; uniquely 1
/// then indexOnPublishedPartnerSameCurve. 0 or N unnamed (I13). Does not mint.
/// Formerly ProfileBased::uniqueZParallelEdgeOnPublishedFace (Pass 135–136).
PartExport App::ElementIndex uniqueZParallelEdgeOnPublishedFace(
    const TopoShape& owner,
    const App::ElementIndex& faceIdx);

/// Map a Pipe/Loft/Helix pre-sew shell TShape onto the published sew+solid Shape.
/// indexOnPublishedPartnerSameCurve first; MapperSewing sew hop; unique shared-vertex
/// geometry hop; element-map if still unnamed. 0 or N stay unnamed (I13).
/// Formerly Pipe/Loft/Helix *IndexOnPublished (Pass 139). sewer may be null (Helix).
/// Does not mint.
PartExport App::ElementIndex indexOnPublishedPartnerSameCurveSewSharedVertex(
    const TopoShape& published,
    const TopoDS_Shape& sub,
    const TopoShape& preSew,
    BRepBuilderAPI_Sewing* sewer);

/// Build Z-parallel Face-rail Edges and merge onto named Edge indices
/// (Pipe/Loft/Helix capture). For each Face index call
/// uniqueZParallelEdgeOnPublishedFace; if namedEdges empty, replace with
/// zEdges; else for each named zEdge skip if already present (type+index),
/// else overwrite empty slot at same i or append. Returns count of named
/// zEdges (index>0 Edge). I13 leave-unnamed. Does not mint.
PartExport std::size_t mergeUniqueZParallelEdgesOntoNamed(
    const TopoShape& published,
    const std::vector<App::ElementIndex>& namedFaces,
    std::vector<App::ElementIndex>& namedEdges);

/// Face/Edge capture row shared by Pipe/Loft/Helix Pad-fallback Z-rail append
/// (and usable as the seeded shape vector element type).
struct SemanticSeededShape
{
    App::SemanticId fromSeed;
    TopoDS_Shape shape;
};

/// For each Face in `faces`, append the unique Z-parallel Edge on `shell`
/// into `edges` when not already present (sameOccShape). Skips null/non-Face
/// and 0/N rails (I13). Returns count appended. Does not mint.
/// Formerly inlined identically in FeaturePipe/Loft/Helix capture*Maker (Pass 141).
PartExport std::size_t appendUniqueZParallelFaceRailEdges(
    const TopoShape& shell,
    const std::vector<SemanticSeededShape>& faces,
    std::vector<SemanticSeededShape>& edges);


/// When `faces` is empty, run SemanticHistoryAdapter::fromMaker on curve/vertex
/// profile inputs indexed onto `preSewShell` via indexOnPublishedPartnerSameCurve;
/// append Generated Face/Edge shapes into faces/outEdges (Edge dedup via
/// sameOccShape). Returns history table size (0 if skipped). Sets
/// *usedFromMaker when non-null and the empty-Face path runs. I13 leave-unnamed.
/// Does not mint. Formerly inlined identically in FeaturePipe/Loft/Helix
/// capture*Maker (Pass 142).
PartExport std::size_t appendFromMakerGeneratedWhenFacesEmpty(
    const void* occMaker,
    const TopoShape& preSewShell,
    const std::vector<App::SemanticId>& curveSeeds,
    const std::vector<TopoShape>& profileEdges,
    const std::vector<App::SemanticId>& vertexSeeds,
    const std::vector<TopoShape>& profileVertices,
    std::vector<SemanticSeededShape>& faces,
    std::vector<SemanticSeededShape>& outEdges,
    bool* usedFromMaker = nullptr);


/// Map Pipe/Loft/Helix captured Face/Edge seeded shapes onto the published
/// sew+solid Shape via indexOnPublishedPartnerSameCurveSewSharedVertex.
/// Clears and fills namedFaces/namedEdges (one index per seed, I13 unnamed
/// slots allowed). sewer may be null (Helix). Does not mint.
/// Formerly identical Pipe/Loft/Helix::refreshNamedIndices (Pass 143).
PartExport void refreshNamedIndicesFromSeededShapes(
    const TopoShape& published,
    const TopoShape& preSewShell,
    BRepBuilderAPI_Sewing* sewer,
    const std::vector<SemanticSeededShape>& faces,
    const std::vector<SemanticSeededShape>& edges,
    std::vector<App::ElementIndex>& namedFaces,
    std::vector<App::ElementIndex>& namedEdges);

/// A4: TESTS Extrusion/Revolution/Mirroring publish `*Diag` Console strings.
/// Default **off** so product builds stay quiet. Enable with env
/// `FREECAD_TESTS_DIAG=1` (legacy `FREECAD_TEST5_DIAG`) or `FreeCAD.setLogLevel('PartTestsDiag', 'Message')` (legacy `PartTest5Diag`)
/// (or higher). `tests_automated.py` does **not** grep these strings; docs
/// historically reference the exact text for human console reading.
/// When enabled, emit the historical strings verbatim via Console().message.
/// Gate only — does **not** change emit order, mint, applyHistory, or I13.
PartExport bool testsPublishDiagEnabled();
/// Emit `message` verbatim when enabled (include trailing `\n` if desired).
PartExport void testsPublishDiag(const char* message);
/// Emit `TESTS <diagTag> skip emit: <reason>\n` when enabled.
PartExport void testsPublishDiagSkip(const char* diagTag, const char* reason);
/// Emit `TESTS <diagTag> bound=%zu named=%zu unnamed=0\n` when enabled.
PartExport void testsPublishDiagBound(const char* diagTag,
                                     std::size_t bound,
                                     std::size_t named);

}  // namespace Part
