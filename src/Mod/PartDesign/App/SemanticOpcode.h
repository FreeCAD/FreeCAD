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

/// Phase 5: PartDesign opcode role tables + maker-side provenance emission.
/// Rev 3.1 §4.2 / §10. No OCCT history adapter. SemanticBinding carries ElementIndex
/// only (no TopoDS). Every entry point is a no-op when no graph is attached.

#include <App/SemanticReference.h>
#include <App/SemanticTopology.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace App
{
class DocumentObject;
class PropertyComplexGeoData;
}
namespace Part
{
class TopoShape;
}

#ifndef SEMANTIC_TOPOLOGY_STANDALONE
#ifndef PartDesignExport
#include <Mod/PartDesign/PartDesignGlobal.h>
#endif
#else
#ifndef PartDesignExport
#define PartDesignExport
#endif
#endif

namespace PartDesign
{

enum class Opcode : std::uint8_t
{
    Pad = 0,
    Pocket,
    Fillet,
    Chamfer,
    Draft,
    Thickness,
    Revolution,
    Groove,
    Hole,
    LinearPattern,
    PolarPattern,
    Mirrored,
    Loft,    ///< 12
    Pipe,    ///< 13
    Helix,   ///< 14
    Boolean, ///< 15  PartDesign Boolean Fuse (not Part::Fuse / Opcode::Fuse)
    AdditiveBox, ///< 16  first-solid PartDesign AdditiveBox (append-only)
    AdditiveCylinder, ///< 17  first-solid PartDesign AdditiveCylinder (append-only)
    AdditiveSphere, ///< 18  first-solid PartDesign AdditiveSphere (append-only)
    AdditiveCone, ///< 19  first-solid PartDesign AdditiveCone (append-only)
    AdditiveTorus, ///< 20  first-solid PartDesign AdditiveTorus (append-only)
    AdditivePrism, ///< 21  first-solid PartDesign AdditivePrism (append-only)
    AdditiveWedge, ///< 22  first-solid PartDesign AdditiveWedge (append-only)
    AdditiveEllipsoid, ///< 23  first-solid PartDesign AdditiveEllipsoid (append-only)
    SubtractiveBox, ///< 24  with-base PartDesign SubtractiveBox Cut (append-only)
    SubtractiveCylinder, ///< 25  with-base PartDesign SubtractiveCylinder Cut (append-only)
    SubtractiveSphere, ///< 26  with-base PartDesign SubtractiveSphere Cut (append-only)
    SubtractiveCone, ///< 27  with-base PartDesign SubtractiveCone Cut (append-only)
    SubtractiveTorus, ///< 28  with-base PartDesign SubtractiveTorus Cut (append-only)
    SubtractivePrism, ///< 29  with-base PartDesign SubtractivePrism Cut (append-only)
    SubtractiveWedge, ///< 30  with-base PartDesign SubtractiveWedge Cut (append-only)
    SubtractiveEllipsoid, ///< 31  with-base PartDesign SubtractiveEllipsoid Cut (append-only)
    Scaled, ///< 32  PartDesign Scaled transformed provenance (append-only)
    MultiTransform, ///< 33  PartDesign MultiTransform provenance (append-only)
    SubtractiveLoft, ///< 34 with-base PartDesign SubtractiveLoft sweep (append-only)
    SubtractivePipe, ///< 35 with-base PartDesign SubtractivePipe sweep (append-only)
    SubtractiveHelix, ///< 36 with-base PartDesign SubtractiveHelix sweep (append-only)
    Count, ///< Sentinel for append-only opcode table validation; not an opcode.
};

/// Producer / inbound roles for the Sketch → Pad → Pocket → Fillet prototype.
enum class OpcodeRoleId : std::uint8_t
{
    PadSide = 0,      ///< Generated from a sketch curve seed
    PadCap,           ///< Generated from a sketch region seed
    PadUpToFace,      ///< inbound termination; RequireOne
    PocketSide,       ///< Generated from a pocket sketch curve
    PocketCap,        ///< Generated from a pocket sketch region
    PocketUpToFace,   ///< inbound termination; RequireOne
    PocketRemnant,    ///< S3 hole: remnant keeps handle; walls Generated
    PocketSplit,      ///< through-cut / L: 1→N Split, no continuation
    FilletEdge,       ///< inbound dress-up edge; AcceptAll
    FilletFace,       ///< Generated from (edge seed, adjacent face seeds)
    ChamferEdge,      ///< inbound dress-up edge; AcceptAll (append-only; opcodeRole indexes kRoles)
    ChamferFace,      ///< Generated from (edge seed, adjacent face seeds)
    DraftFace,        ///< inbound dress-up face; AcceptAll (append-only; opcodeRole indexes kRoles)
    DraftResult,      ///< Generated face from that seed
    ThicknessFace,    ///< inbound dress-up face; AcceptAll (append-only; opcodeRole indexes kRoles)
    ThicknessResult,  ///< Generated face from that seed
    RevolutionSide,   ///< Generated from a sketch curve seed (append-only)
    RevolutionCap,    ///< Generated from a sketch region seed
    RevolutionUpToFace,  ///< inbound termination; stay unnamed (I10)
    GrooveSide,
    GrooveCap,
    GrooveUpToFace,
    HoleStart,        ///< inbound Hole.StartReference face; AcceptAll (append-only)
    HoleResult,       ///< Generated face from that seed (named history only)
    LinearPatternFace,  ///< inbound Originals/Base Face; AcceptAll (append-only)
    LinearPatternEdge,  ///< inbound Originals/Base Edge; unique 1-image Generated
    PolarPatternFace,
    PolarPatternEdge,
    MirroredFace,
    MirroredEdge,
    LoftSide,         ///< MirroredEdge+1; Generated from a sketch curve seed
    LoftCap,          ///< Generated from a sketch region seed
    PipeSide,
    PipeCap,
    HelixSide,
    HelixCap,
    BooleanFace,  ///< unique 1-image Generated Face (append-only; PBF)
    BooleanEdge,  ///< unique 1-image Generated Edge
    AdditiveBoxFace,  ///< unique 1-image Generated Face (append-only; PBX)
    AdditiveBoxEdge,  ///< unique 1-image Generated Edge
    AdditiveCylinderFace,  ///< unique 1-image Generated Face (append-only; PCY)
    AdditiveCylinderEdge,  ///< unique 1-image Generated Edge
    AdditiveSphereFace,  ///< unique 1-image Generated Face (append-only; PSP)
    AdditiveSphereEdge,  ///< unique 1-image Generated Edge
    AdditiveConeFace,  ///< unique 1-image Generated Face (append-only; PCN)
    AdditiveConeEdge,  ///< unique 1-image Generated Edge
    AdditiveTorusFace,  ///< unique 1-image Generated Face (append-only; PTO)
    AdditiveTorusEdge,  ///< unique 1-image Generated Edge
    AdditivePrismFace,  ///< unique 1-image Generated Face (append-only; PPR)
    AdditivePrismEdge,  ///< unique 1-image Generated Edge
    AdditiveWedgeFace,  ///< unique 1-image Generated Face (append-only; PWD)
    AdditiveWedgeEdge,  ///< unique 1-image Generated Edge
    AdditiveEllipsoidFace,  ///< unique 1-image Generated Face (append-only; PEL)
    AdditiveEllipsoidEdge,  ///< unique 1-image Generated Edge
    SubtractiveBoxFace,  ///< unique 1-image Generated Face (append-only; PSB)
    SubtractiveBoxEdge,  ///< unique 1-image Generated Edge
    SubtractiveCylinderFace, ///< unique 1-image Generated Face (append-only; PSC)
    SubtractiveCylinderEdge, ///< unique 1-image Generated Edge
    SubtractiveSphereFace, ///< unique 1-image Generated Face (append-only; PSS)
    SubtractiveSphereEdge, ///< unique 1-image Generated Edge
    SubtractiveConeFace, ///< unique 1-image Generated Face (append-only; PCN)
    SubtractiveConeEdge, ///< unique 1-image Generated Edge
    SubtractiveTorusFace, ///< unique 1-image Generated Face (append-only; PTO)
    SubtractiveTorusEdge, ///< unique 1-image Generated Edge
    SubtractivePrismFace, ///< unique 1-image Generated Face (append-only; PPR)
    SubtractivePrismEdge, ///< unique 1-image Generated Edge
    SubtractiveWedgeFace, ///< unique 1-image Generated Face (append-only; PWD)
    SubtractiveWedgeEdge, ///< unique 1-image Generated Edge
    SubtractiveEllipsoidFace, ///< unique 1-image Generated Face (append-only; PEL)
    SubtractiveEllipsoidEdge, ///< unique 1-image Generated Edge
    SubtractiveLoftSide, ///< Generated sweep side face (append-only)
    SubtractiveLoftCap, ///< Generated sweep cap face
    SubtractivePipeSide, ///< Generated sweep side face (append-only)
    SubtractivePipeCap, ///< Generated sweep cap face
    SubtractiveHelixSide, ///< Generated sweep side face (append-only)
    SubtractiveHelixCap, ///< Generated sweep cap face
    Count, ///< Sentinel for append-only role table validation; not a role.
};

struct PartDesignExport OpcodeRole
{
    Opcode opcode = Opcode::Pad;
    OpcodeRoleId id = OpcodeRoleId::PadSide;
    const char* name = "";
    const char* partOpCode = "";  ///< Part::OpCodes token: XTR / CUT / FLT
    App::SemanticKind seedKind = App::SemanticKind::Edge;
    App::SemanticKind outputKind = App::SemanticKind::Face;
    App::EventKind eventKind = App::EventKind::Generated;
    App::SemanticRole consumerRole = App::SemanticRole::None;
    App::FilterFlag filter = App::FilterFlag::DescendantsOfSeed | App::FilterFlag::SameKind;
    App::CardinalityReducer defaultReducer = App::CardinalityReducer::RequireOne;
    App::AcceptedCardinality acceptedCardinality = App::AcceptedCardinality::One;
    bool remnantIsContinuation = false;  ///< S3: remnant keeps the handle
};

PartDesignExport const OpcodeRole& opcodeRole(OpcodeRoleId id);
PartDesignExport std::vector<const OpcodeRole*> opcodeRolesFor(Opcode opcode);
PartDesignExport const char* opcodeName(Opcode opcode);

PartDesignExport App::ReferenceRequirement requirementFor(OpcodeRoleId id);
PartDesignExport App::SemanticReference referenceFor(const App::SemanticId& seed, OpcodeRoleId id);

/// Seeds named by the sketch / inbound links. Empty → skip emit (no half-map).
struct PartDesignExport AfterExecuteRequest
{
    std::vector<App::SemanticId> curveSeeds;
    std::vector<App::SemanticId> vertexSeeds;
    std::vector<App::SemanticId> regionSeeds;
    App::SemanticId pocketTarget;
    enum class PocketMode : std::uint8_t
    {
        Unknown = 0,  ///< emit Generated sides/caps only; Split vs S3 is a later decision
        Hole,         ///< S3 remnant (Length)
        ThroughCut,   ///< Split (ThroughAll)
    };
    PocketMode pocketMode = PocketMode::Unknown;
    std::size_t pocketSplitCount = 2;
    std::size_t pocketWallCount = 4;
    /// Dress-up edge seeds. Chamfer reuses these vectors (same Base.getSemanticRefs filter).
    std::vector<App::SemanticId> filletEdges;
    std::vector<App::SemanticId> filletAdjacentFaces;
    /// Draft inbound Face seeds (Draft.Base is faces, not edges).
    std::vector<App::SemanticId> draftFaces;
    /// Thickness inbound Face seeds (Thickness.Base is faces to open).
    std::vector<App::SemanticId> thicknessFaces;
    /// Hole.StartReference inbound Face seed (consume-only; no sequential FaceN).
    std::vector<App::SemanticId> holeFaces;
    int firstFaceIndex = 1;
    /// Debug/test only. Product execute() leaves this false (I10 / I13):
    /// SemanticBinding is written only when namedFaceIndices (adapter) names an output.
    /// Sequential FaceN is never product SemanticBinding.
    bool allowSequentialFaceN = false;
    std::vector<App::ElementIndex> namedFaceIndices;
    /// Named Edge indices from history (type-local: Edge8 is 8). Zipped 1:1
    /// onto vertexSeeds, else leftover/curve seeds. Unnamed slots skip (I13).
    std::vector<App::ElementIndex> namedEdgeIndices;
};

/// First Face seed in a dual-written PropertyLinkSub. Invalid if none.
/// Prefer uniqueNamedFace (I13) for product consume; firstNamedFace is first-wins
/// and remains only for callers that intentionally accept that policy.
PartDesignExport App::SemanticId firstNamedFace(const std::vector<App::SemanticReference>& refs);

/// Exactly one Face seed among refs (seed.kind or ref.kind Face). 0 or >1
/// → invalid (I13). Never first-Binding-wins. Pocket UpToFace / Extrude profile
/// use this (PD5-I13). Distinct from firstNamedFace.
PartDesignExport App::SemanticId uniqueNamedFace(const std::vector<App::SemanticReference>& refs);

/// Append one valid semantic seed by durable handle. Returns true only when
/// the request vector grew; invalid or repeated seeds are ignored.
PartDesignExport bool appendUniqueSemanticSeed(std::vector<App::SemanticId>& seeds,
                                               const App::SemanticId& seed);

/// Feature-scoped unique Face Binding for a seed. 0 or >1 on `linkedFeature`
/// with type Face and index>0 → empty (I13). Never first-Binding-wins.
/// Null graph, invalid seed, or linkedFeature 0 → empty.
PartDesignExport std::optional<App::SemanticBinding> uniqueFaceBindingOnFeature(
    const App::SemanticGraph* graph,
    const App::SemanticId& seed,
    App::ObjectId linkedFeature);
/// Resolve one stored Face reference under its own filter/reducer policy.
/// A live result is consumable only when it resolves to exactly one Face
/// Binding on the linked feature; missing, ambiguous, incompatible, or
/// cross-feature results remain unconsumed under I12/I13.
PartDesignExport std::optional<App::SemanticBinding> uniqueResolvedFaceReference(
    const App::SemanticGraph* graph,
    const App::SemanticReference& reference,
    App::ObjectId linkedFeature);

/// Feature-scoped unique Edge Binding for a seed. 0 or >1 on `linkedFeature`
/// with type Edge and index>0 → empty (I13). Never first-Binding-wins.
/// Null graph, invalid seed, or linkedFeature 0 → empty.
PartDesignExport std::optional<App::SemanticBinding> uniqueEdgeBindingOnFeature(
    const App::SemanticGraph* graph,
    const App::SemanticId& seed,
    App::ObjectId linkedFeature);

/// Maker-side emitter. Safe when graph is null (no-op, invalid ids).
class PartDesignExport SemanticEmitter
{
public:
    /// Document-owned graph via SemanticDocumentState. Null if the pointer is
    /// not a bound Document* / DocumentObject* (or is null).
    static App::SemanticGraph* graphFor(const void* featureOrDocument);

    static bool attached(const App::SemanticGraph* graph);

    /// Restore/recompute gate for semantic publishers. Bindings are excluded
    /// because they are transient and omitted from STG1.
    static bool needsSemanticRepublish(const App::SemanticGraph* graph,
                                       App::ObjectId feature);

    /// When a dress-up result is a Profile target, carry cached FaceN fallbacks
    /// from downstream Pad/Pocket Profile links into maker-named indices so
    /// promote can bind the face the product tree already references (I13).
    static void appendInverseProfileFaceIndices(
        const App::DocumentObject* profileFeature,
        const Part::TopoShape& profileShape,
        std::vector<App::ElementIndex>& namedFaceIndices);

    /// Guarded execute() hook. No-op if graph is null or no seeds are supplied.
    /// Never binds a random FaceN (no half-map). lastAfterExecuteNote explains skips.
    static void afterExecute(App::SemanticGraph* graph,
                             Opcode opcode,
                             App::ObjectId feature,
                             App::EvalSerial eval,
                             const AfterExecuteRequest& request = {});

    static const std::string& lastAfterExecuteNote();
    /// Dual-write `;:ST` onto an existing ElementMap mapped name (I8).
    /// Skips unless exactly one feature-scoped Binding owns the index; uses
    /// `SemanticId::mappedTokenPrefix()` + `MappedName::contains` (wraps
    /// `find` int/-1 — never compare to `std::string::npos`). Stamps the
    /// primary mapped name only (`names.front()`). Decode-compatible with
    /// `semanticIdFromSubName`. Call after Shape + Bindings are published.
    /// Uniqueness counts **all live evals** at (feature,index) — same fail-closed
    /// shape as `uniqueBindingOnFeature` (AG21-E1 / EM14-U1). Do **not** switch
    /// to `uniquePublishedBinding` max-eval without a TESTS re-gate.
    /// Call sites today: Loft / Pipe / Helix only (EM14-S1); Pad/Pocket/Fillet
    /// Bindings stay afterExecute-only until scored dual-write is required.
    static void stampElementMap(App::PropertyComplexGeoData& map,
                                const App::SemanticGraph* graph,
                                App::ObjectId feature);

    /// Shared with-base subtractive live Cut publisher (Batch A primitives,
    /// SubtractiveLoft/Pipe). fromMaker -> uniqueOneImageGenerated ->
    /// applyHistory + afterExecute. Call after Shape is published.
    /// occBooleanOp/tool/base are OCCT pointers (BRepAlgoAPI_BooleanOperation*,
    /// TopoDS_Shape*) — kept opaque here so this header stays TopoDS-free.
    static void publishSubtractiveCutHistory(App::DocumentObject* feature,
                                             void* occBooleanOp,
                                             const void* toolShapeOcc,
                                             const void* baseShapeOcc,
                                             App::DocumentObject* baseObj,
                                             Opcode opcode,
                                             const char* diagName);

    /// Append to the last note (Loft isolate diagnostics). No-op if suffix empty.
    static void appendAfterExecuteNote(const std::string& suffix);

    /// Generated Edge/Region outputs of `sketch` (already-ensured seeds).
    static AfterExecuteRequest collectSketchProfileSeeds(const App::SemanticGraph* graph,
                                                         App::ObjectId sketch);

    /// Collect sketch seeds, or a selected Face profile as a conservative
    /// curve/region seed when Profile is a non-Sketch LinkSub.
    static AfterExecuteRequest collectProfileSeeds(
        const App::SemanticGraph* graph,
        App::ObjectId profile,
        const std::vector<App::SemanticReference>& profileRefs);

    static void bind(App::SemanticGraph* graph,
                     const App::SemanticId& id,
                     App::ObjectId feature,
                     App::EvalSerial eval,
                     const App::ElementIndex& index);

    /// Generated from seeds + SemanticBinding row. Invalid id if graph is null.
    static App::SemanticId emitGeneratedFrom(App::SemanticGraph* graph,
                                             const std::vector<App::SemanticId>& seeds,
                                             App::SemanticKind outKind,
                                             const std::string& op,
                                             App::ObjectId feature,
                                             App::EvalSerial eval,
                                             App::SemanticRole role,
                                             const App::ElementIndex& index);

    static std::vector<App::SemanticId> emitSplit(App::SemanticGraph* graph,
                                                  const App::SemanticId& input,
                                                  std::size_t count,
                                                  const std::string& op,
                                                  App::ObjectId feature,
                                                  App::EvalSerial eval,
                                                  App::SemanticRole role,
                                                  const std::vector<App::ElementIndex>& indices);

    static App::EventId emitDeleted(App::SemanticGraph* graph,
                                    const App::SemanticId& input,
                                    const std::string& op,
                                    App::ObjectId feature,
                                    App::EvalSerial eval,
                                    App::SemanticRole role);

    static App::EventId emitModified(App::SemanticGraph* graph,
                                     const App::SemanticId& input,
                                     const std::string& op,
                                     App::ObjectId feature,
                                     App::EvalSerial eval,
                                     App::SemanticRole role,
                                     const App::ElementIndex& index);

    /// Pad: sides Generated from curve seeds, caps Generated from region seeds.
    static std::vector<App::SemanticId> emitPadSides(App::SemanticGraph* graph,
                                                     const std::vector<App::SemanticId>& curveSeeds,
                                                     App::ObjectId feature,
                                                     App::EvalSerial eval,
                                                     int firstFaceIndex);

    static std::vector<App::SemanticId> emitPadCaps(App::SemanticGraph* graph,
                                                    const std::vector<App::SemanticId>& regionSeeds,
                                                    App::ObjectId feature,
                                                    App::EvalSerial eval,
                                                    int firstFaceIndex);

    /// Pocket through-cut: Split the target (typically a pad cap). Not a neighbour guess.
    static std::vector<App::SemanticId> emitPocketSplit(App::SemanticGraph* graph,
                                                        const App::SemanticId& target,
                                                        std::size_t count,
                                                        App::ObjectId feature,
                                                        App::EvalSerial eval,
                                                        int firstFaceIndex);

    /// Pocket S3 hole: remnant keeps handle; walls Generated from the pocket region.
    static App::SemanticId emitPocketHole(App::SemanticGraph* graph,
                                          const App::SemanticId& remnant,
                                          const App::SemanticId& pocketRegion,
                                          std::size_t wallCount,
                                          App::ObjectId feature,
                                          App::EvalSerial eval,
                                          const App::ElementIndex& remnantIndex,
                                          int firstWallFaceIndex);

    /// Fillet: resolve the edge first. Missing edge → Missing dress-up (no similar-length
    /// neighbour). New face Generated from (edge, adjacent faces).
    static App::ResolutionResult emitFillet(App::SemanticGraph* graph,
                                            const App::SemanticId& edge,
                                            const std::vector<App::SemanticId>& adjacentFaces,
                                            App::ObjectId feature,
                                            App::EvalSerial eval,
                                            const App::ElementIndex& faceIndex);

    /// Chamfer: same Missing/I10 contract as emitFillet. Event.op token is "Chamfer" (not Fillet).
    static App::ResolutionResult emitChamfer(App::SemanticGraph* graph,
                                             const App::SemanticId& edge,
                                             const std::vector<App::SemanticId>& adjacentFaces,
                                             App::ObjectId feature,
                                             App::EvalSerial eval,
                                             const App::ElementIndex& faceIndex);

    /// Draft: resolve inbound Face first. Missing face → skip (no similar-angle neighbour).
    /// New face Generated from that seed. Event.op token is "Draft".
    static App::ResolutionResult emitDraft(App::SemanticGraph* graph,
                                           const App::SemanticId& face,
                                           App::ObjectId feature,
                                           App::EvalSerial eval,
                                           const App::ElementIndex& faceIndex);

    /// Thickness: resolve inbound Face first. Missing face → skip (no neighbour).
    /// New face Generated from that seed. Event.op token is "Thickness".
    static App::ResolutionResult emitThickness(App::SemanticGraph* graph,
                                               const App::SemanticId& face,
                                               App::ObjectId feature,
                                               App::EvalSerial eval,
                                               const App::ElementIndex& faceIndex);

    /// Hole: resolve inbound StartReference Face first. Missing face → skip
    /// (no neighbour). Bind Generated only when history named an index.
    /// Event.op token is "Hole".
    static App::ResolutionResult emitHole(App::SemanticGraph* graph,
                                          const App::SemanticId& face,
                                          App::ObjectId feature,
                                          App::EvalSerial eval,
                                          const App::ElementIndex& faceIndex);

    /// Source split implies side or cap split. Splits each live Generated descendant of source.
    static std::vector<App::SemanticId> propagateSourceSplit(App::SemanticGraph* graph,
                                                             const App::SemanticId& source,
                                                             std::size_t childCount,
                                                             const std::string& op,
                                                             App::ObjectId feature,
                                                             App::EvalSerial eval,
                                                             int firstFaceIndex);

    /// Outputs of Generated events that list `seed` as an EventInput.
    static std::vector<App::SemanticId> generatedFrom(const App::SemanticGraph& graph,
                                                      App::SemanticHandle seed);
};

}  // namespace PartDesign
