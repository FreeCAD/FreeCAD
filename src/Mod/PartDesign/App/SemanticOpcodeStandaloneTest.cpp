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
//
// Phase 5 standalone scenario: Sketch → Pad → Pocket → Fillet.
// Metric is ResolutionState + provenance family, not FaceN equality.
//
//   src/Mod/PartDesign/App/compile_semantic_opcode_test.sh
//   /tmp/semantic-opcode-phase5

#include "SemanticOpcode.h"
#include <App/SemanticDocumentState.h>

#include <App/SemanticId.h>
#include <App/SemanticReference.h>
#include <App/SemanticTopology.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace App;
using namespace PartDesign;

namespace
{

int g_failed = 0;
int g_passed = 0;

void check(bool cond, const char* expr, const char* file, int line)
{
    if (cond) {
        ++g_passed;
        return;
    }
    ++g_failed;
    std::cerr << "FAIL " << file << ":" << line << "  " << expr << "\n";
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

SemanticBinding makeBinding(const SemanticId& id, ObjectId feature, EvalSerial eval, const char* name)
{
    SemanticBinding b;
    b.stid = id;
    b.feature = feature;
    b.eval = eval;
    b.kind = id.kind;
    b.index = ElementIndex::fromString(name);
    b.occ = nullptr;
    return b;
}

SemanticReference makeRef(const SemanticId& seed,
                          CardinalityReducer reducer,
                          SemanticKind kind)
{
    SemanticReference ref;
    ref.seed = seed;
    ref.kind = kind;
    ref.reducer = reducer;
    ref.filter.flags = FilterFlag::DescendantsOfSeed | FilterFlag::SameKind;
    return ref;
}

void testNoopWithoutGraph()
{
    CHECK(!SemanticEmitter::attached(nullptr));
    CHECK(SemanticEmitter::graphFor(nullptr) == nullptr);

    const auto sides = SemanticEmitter::emitPadSides(nullptr, {}, 1, 1, 1);
    CHECK(sides.empty());
    const auto caps = SemanticEmitter::emitPadCaps(nullptr, {}, 1, 1, 1);
    CHECK(caps.empty());
    const auto split = SemanticEmitter::emitPocketSplit(nullptr, SemanticId{}, 2, 1, 1, 1);
    CHECK(split.empty());

    ResolutionResult r = SemanticEmitter::emitFillet(nullptr, SemanticId{}, {}, 1, 1,
                                                     ElementIndex::fromString("Face1"));
    CHECK(r.state == ResolutionState::Missing);

    SemanticEmitter::afterExecute(nullptr, Opcode::Pad, 0, 0);
}

void testGraphForFindsDocumentGraph()
{
    SemanticDocumentState state;
    int fakeDoc = 0;
    int fakePad = 0;
    state.bindOwner(&fakeDoc);
    state.bindAlias(&fakePad);
    CHECK(SemanticEmitter::graphFor(nullptr) == nullptr);
    CHECK(SemanticEmitter::graphFor(&fakeDoc) == &state.graph());
    CHECK(SemanticEmitter::graphFor(&fakePad) == &state.graph());
    CHECK(SemanticEmitter::attached(SemanticEmitter::graphFor(&fakePad)));
    SemanticEmitter::afterExecute(SemanticEmitter::graphFor(&fakePad), Opcode::Pad, 0, 0);
}

void testSemanticRepublishGate()
{
    SemanticGraph g;
    CHECK(SemanticEmitter::needsSemanticRepublish(&g, 40));
    CHECK(!SemanticEmitter::needsSemanticRepublish(nullptr, 40));
    CHECK(!SemanticEmitter::needsSemanticRepublish(&g, 0));

    g.beginEvaluate(1);
    g.recordGenerated(SemanticKind::Face, "Fillet", 40, 1, SemanticRole::None);
    // In-flight output is not durable feature data and must not suppress a
    // later restore/recompute republish request before commit.
    CHECK(SemanticEmitter::needsSemanticRepublish(&g, 40));
    g.commitEvaluate();
    CHECK(g.hasFeatureData(40));
    CHECK(!SemanticEmitter::needsSemanticRepublish(&g, 40));
    CHECK(SemanticEmitter::needsSemanticRepublish(&g, 41));
}

void testRoleTablesExist()
{
    CHECK(opcodeRolesFor(Opcode::Pad).size() >= 3);
    CHECK(opcodeRolesFor(Opcode::Pocket).size() >= 4);
    CHECK(opcodeRolesFor(Opcode::Fillet).size() >= 2);
    CHECK(!opcodeRolesFor(Opcode::Chamfer).empty());
    CHECK(opcodeRolesFor(Opcode::Chamfer).size() >= 2);

    const OpcodeRole& padSide = opcodeRole(OpcodeRoleId::PadSide);
    CHECK(std::string(padSide.name) == "PadSide");
    CHECK(padSide.seedKind == SemanticKind::Edge);
    CHECK(padSide.outputKind == SemanticKind::Face);
    CHECK(padSide.eventKind == EventKind::Generated);
    CHECK(padSide.defaultReducer == CardinalityReducer::AcceptAll);
    CHECK(padSide.acceptedCardinality == AcceptedCardinality::OneOrMore);
    CHECK(std::string(padSide.partOpCode) == "XTR");
    CHECK(!padSide.remnantIsContinuation);

    const OpcodeRole& padCap = opcodeRole(OpcodeRoleId::PadCap);
    CHECK(padCap.seedKind == SemanticKind::Region);
    CHECK(padCap.outputKind == SemanticKind::Face);
    CHECK(padCap.eventKind == EventKind::Generated);

    const OpcodeRole& padUp = opcodeRole(OpcodeRoleId::PadUpToFace);
    CHECK(padUp.consumerRole == SemanticRole::UpToFace);
    CHECK(padUp.defaultReducer == CardinalityReducer::RequireOne);
    CHECK(padUp.acceptedCardinality == AcceptedCardinality::One);

    const OpcodeRole& pocketSplit = opcodeRole(OpcodeRoleId::PocketSplit);
    CHECK(pocketSplit.eventKind == EventKind::Split);
    CHECK(!pocketSplit.remnantIsContinuation);
    CHECK(pocketSplit.defaultReducer == CardinalityReducer::AcceptAll);

    const OpcodeRole& pocketRemnant = opcodeRole(OpcodeRoleId::PocketRemnant);
    CHECK(pocketRemnant.remnantIsContinuation);  // S3
    CHECK(pocketRemnant.eventKind == EventKind::Modified);

    const OpcodeRole& filletEdge = opcodeRole(OpcodeRoleId::FilletEdge);
    CHECK(filletEdge.consumerRole == SemanticRole::DressUpEdge);
    CHECK(filletEdge.defaultReducer == CardinalityReducer::AcceptAll);
    CHECK(filletEdge.acceptedCardinality == AcceptedCardinality::OneOrMore);
    CHECK(std::string(filletEdge.partOpCode) == "FLT");

    const OpcodeRole& filletFace = opcodeRole(OpcodeRoleId::FilletFace);
    CHECK(filletFace.eventKind == EventKind::Generated);
    CHECK(filletFace.seedKind == SemanticKind::Edge);
    CHECK(filletFace.outputKind == SemanticKind::Face);

    const OpcodeRole& chamferEdge = opcodeRole(OpcodeRoleId::ChamferEdge);
    CHECK(chamferEdge.consumerRole == SemanticRole::DressUpEdge);
    CHECK(chamferEdge.defaultReducer == CardinalityReducer::AcceptAll);
    CHECK(std::string(chamferEdge.partOpCode) == "CHF");
    CHECK(std::string(chamferEdge.name) == "ChamferEdge");
    const OpcodeRole& chamferFace = opcodeRole(OpcodeRoleId::ChamferFace);
    CHECK(chamferFace.eventKind == EventKind::Generated);
    CHECK(chamferFace.opcode == Opcode::Chamfer);
    CHECK(std::string(chamferFace.name) == "ChamferFace");
    CHECK(std::string(opcodeName(Opcode::Chamfer)) == "Chamfer");

    CHECK(!opcodeRolesFor(Opcode::Draft).empty());
    CHECK(opcodeRolesFor(Opcode::Draft).size() >= 2);
    const OpcodeRole& draftFace = opcodeRole(OpcodeRoleId::DraftFace);
    CHECK(draftFace.defaultReducer == CardinalityReducer::AcceptAll);
    CHECK(draftFace.seedKind == SemanticKind::Face);
    CHECK(draftFace.outputKind == SemanticKind::Face);
    CHECK(std::string(draftFace.partOpCode) == "DFT");
    CHECK(std::string(draftFace.name) == "DraftFace");
    CHECK(draftFace.opcode == Opcode::Draft);
    const OpcodeRole& draftResult = opcodeRole(OpcodeRoleId::DraftResult);
    CHECK(draftResult.eventKind == EventKind::Generated);
    CHECK(draftResult.seedKind == SemanticKind::Face);
    CHECK(draftResult.opcode == Opcode::Draft);
    CHECK(std::string(draftResult.name) == "DraftResult");
    CHECK(std::string(opcodeName(Opcode::Draft)) == "Draft");

    CHECK(!opcodeRolesFor(Opcode::Thickness).empty());
    CHECK(opcodeRolesFor(Opcode::Thickness).size() >= 2);
    const OpcodeRole& thicknessFace = opcodeRole(OpcodeRoleId::ThicknessFace);
    CHECK(thicknessFace.defaultReducer == CardinalityReducer::AcceptAll);
    CHECK(thicknessFace.seedKind == SemanticKind::Face);
    CHECK(thicknessFace.outputKind == SemanticKind::Face);
    CHECK(std::string(thicknessFace.partOpCode) == "THK");
    CHECK(std::string(thicknessFace.name) == "ThicknessFace");
    CHECK(thicknessFace.opcode == Opcode::Thickness);
    const OpcodeRole& thicknessResult = opcodeRole(OpcodeRoleId::ThicknessResult);
    CHECK(thicknessResult.eventKind == EventKind::Generated);
    CHECK(thicknessResult.seedKind == SemanticKind::Face);
    CHECK(thicknessResult.opcode == Opcode::Thickness);
    CHECK(std::string(thicknessResult.name) == "ThicknessResult");
    CHECK(std::string(opcodeName(Opcode::Thickness)) == "Thickness");

    CHECK(!opcodeRolesFor(Opcode::Revolution).empty());
    CHECK(opcodeRolesFor(Opcode::Revolution).size() >= 2);
    const OpcodeRole& revSide = opcodeRole(OpcodeRoleId::RevolutionSide);
    CHECK(revSide.seedKind == SemanticKind::Edge);
    CHECK(revSide.outputKind == SemanticKind::Face);
    CHECK(std::string(revSide.partOpCode) == "RVL");
    CHECK(revSide.opcode == Opcode::Revolution);
    CHECK(std::string(opcodeName(Opcode::Revolution)) == "Revolution");
    CHECK(!opcodeRolesFor(Opcode::Groove).empty());
    const OpcodeRole& grooveSide = opcodeRole(OpcodeRoleId::GrooveSide);
    CHECK(grooveSide.opcode == Opcode::Groove);
    CHECK(std::string(opcodeName(Opcode::Groove)) == "Groove");
    const OpcodeRole& revUp = opcodeRole(OpcodeRoleId::RevolutionUpToFace);
    CHECK(revUp.defaultReducer == CardinalityReducer::RequireOne);

    CHECK(!opcodeRolesFor(Opcode::Hole).empty());
    CHECK(opcodeRolesFor(Opcode::Hole).size() >= 2);
    const OpcodeRole& holeStart = opcodeRole(OpcodeRoleId::HoleStart);
    CHECK(holeStart.opcode == Opcode::Hole);
    CHECK(holeStart.seedKind == SemanticKind::Face);
    CHECK(holeStart.outputKind == SemanticKind::Face);
    CHECK(holeStart.defaultReducer == CardinalityReducer::AcceptAll);
    CHECK(std::string(holeStart.partOpCode) == "HOL");
    CHECK(std::string(holeStart.name) == "HoleStart");
    CHECK(std::string(opcodeName(Opcode::Hole)) == "Hole");
    const OpcodeRole& holeResult = opcodeRole(OpcodeRoleId::HoleResult);
    CHECK(holeResult.eventKind == EventKind::Generated);
    CHECK(holeResult.seedKind == SemanticKind::Face);
    CHECK(holeResult.opcode == Opcode::Hole);
    CHECK(std::string(holeResult.name) == "HoleResult");
    CHECK(std::string(holeResult.partOpCode) == "HOL");
    CHECK(static_cast<int>(OpcodeRoleId::HoleStart)
          == static_cast<int>(OpcodeRoleId::GrooveUpToFace) + 1);

    CHECK(!opcodeRolesFor(Opcode::LinearPattern).empty());
    CHECK(opcodeRolesFor(Opcode::LinearPattern).size() >= 2);
    const OpcodeRole& lptFace = opcodeRole(OpcodeRoleId::LinearPatternFace);
    CHECK(lptFace.opcode == Opcode::LinearPattern);
    CHECK(lptFace.seedKind == SemanticKind::Face);
    CHECK(lptFace.outputKind == SemanticKind::Face);
    CHECK(lptFace.eventKind == EventKind::Generated);
    CHECK(std::string(lptFace.partOpCode) == "LPT");
    CHECK(std::string(lptFace.name) == "LinearPatternFace");
    CHECK(std::string(opcodeName(Opcode::LinearPattern)) == "LinearPattern");
    const OpcodeRole& lptEdge = opcodeRole(OpcodeRoleId::LinearPatternEdge);
    CHECK(lptEdge.opcode == Opcode::LinearPattern);
    CHECK(lptEdge.seedKind == SemanticKind::Edge);
    CHECK(lptEdge.outputKind == SemanticKind::Edge);
    CHECK(lptEdge.eventKind == EventKind::Generated);
    CHECK(std::string(lptEdge.partOpCode) == "LPT");
    CHECK(std::string(lptEdge.name) == "LinearPatternEdge");
    CHECK(static_cast<int>(OpcodeRoleId::LinearPatternFace)
          == static_cast<int>(OpcodeRoleId::HoleResult) + 1);

    CHECK(!opcodeRolesFor(Opcode::PolarPattern).empty());
    CHECK(opcodeRolesFor(Opcode::PolarPattern).size() >= 2);
    const OpcodeRole& pptFace = opcodeRole(OpcodeRoleId::PolarPatternFace);
    CHECK(pptFace.opcode == Opcode::PolarPattern);
    CHECK(std::string(pptFace.partOpCode) == "PPT");
    CHECK(std::string(opcodeName(Opcode::PolarPattern)) == "PolarPattern");
    const OpcodeRole& pptEdge = opcodeRole(OpcodeRoleId::PolarPatternEdge);
    CHECK(pptEdge.seedKind == SemanticKind::Edge);
    CHECK(std::string(pptEdge.name) == "PolarPatternEdge");

    CHECK(!opcodeRolesFor(Opcode::Mirrored).empty());
    CHECK(opcodeRolesFor(Opcode::Mirrored).size() >= 2);
    const OpcodeRole& mirFace = opcodeRole(OpcodeRoleId::MirroredFace);
    CHECK(mirFace.opcode == Opcode::Mirrored);
    CHECK(std::string(mirFace.partOpCode) == "PMR");
    CHECK(std::string(opcodeName(Opcode::Mirrored)) == "Mirrored");
    const OpcodeRole& mirEdge = opcodeRole(OpcodeRoleId::MirroredEdge);
    CHECK(mirEdge.seedKind == SemanticKind::Edge);
    CHECK(std::string(mirEdge.name) == "MirroredEdge");
    CHECK(static_cast<int>(OpcodeRoleId::MirroredEdge)
          == static_cast<int>(OpcodeRoleId::MirroredFace) + 1);

    CHECK(static_cast<int>(Opcode::Loft) == 12);
    CHECK(static_cast<int>(Opcode::Pipe) == 13);
    CHECK(static_cast<int>(Opcode::Helix) == 14);
    CHECK(static_cast<int>(OpcodeRoleId::LoftSide)
          == static_cast<int>(OpcodeRoleId::MirroredEdge) + 1);
    CHECK(!opcodeRolesFor(Opcode::Loft).empty());
    CHECK(opcodeRolesFor(Opcode::Loft).size() >= 2);
    const OpcodeRole& loftSide = opcodeRole(OpcodeRoleId::LoftSide);
    CHECK(loftSide.opcode == Opcode::Loft);
    CHECK(std::string(loftSide.partOpCode) == "LFT");
    CHECK(std::string(opcodeName(Opcode::Loft)) == "Loft");
    const OpcodeRole& loftCap = opcodeRole(OpcodeRoleId::LoftCap);
    CHECK(std::string(loftCap.name) == "LoftCap");
    CHECK(!opcodeRolesFor(Opcode::Pipe).empty());
    CHECK(opcodeRolesFor(Opcode::Pipe).size() >= 2);
    const OpcodeRole& pipeSide = opcodeRole(OpcodeRoleId::PipeSide);
    CHECK(pipeSide.opcode == Opcode::Pipe);
    CHECK(std::string(pipeSide.partOpCode) == "PIP");
    CHECK(std::string(opcodeName(Opcode::Pipe)) == "Pipe");
    const OpcodeRole& pipeCap = opcodeRole(OpcodeRoleId::PipeCap);
    CHECK(std::string(pipeCap.name) == "PipeCap");
    CHECK(!opcodeRolesFor(Opcode::Helix).empty());
    CHECK(opcodeRolesFor(Opcode::Helix).size() >= 2);
    const OpcodeRole& helixSide = opcodeRole(OpcodeRoleId::HelixSide);
    CHECK(helixSide.opcode == Opcode::Helix);
    CHECK(std::string(helixSide.partOpCode) == "HLX");
    CHECK(std::string(opcodeName(Opcode::Helix)) == "Helix");
    const OpcodeRole& helixCap = opcodeRole(OpcodeRoleId::HelixCap);
    CHECK(std::string(helixCap.name) == "HelixCap");

    CHECK(static_cast<int>(Opcode::Boolean) == 15);
    CHECK(static_cast<int>(OpcodeRoleId::BooleanFace)
          == static_cast<int>(OpcodeRoleId::HelixCap) + 1);
    CHECK(!opcodeRolesFor(Opcode::Boolean).empty());
    CHECK(opcodeRolesFor(Opcode::Boolean).size() >= 2);
    const OpcodeRole& boolFace = opcodeRole(OpcodeRoleId::BooleanFace);
    CHECK(boolFace.opcode == Opcode::Boolean);
    CHECK(boolFace.seedKind == SemanticKind::Face);
    CHECK(boolFace.outputKind == SemanticKind::Face);
    CHECK(boolFace.eventKind == EventKind::Generated);
    CHECK(std::string(boolFace.partOpCode) == "PBF");
    CHECK(std::string(boolFace.name) == "BooleanFace");
    CHECK(std::string(opcodeName(Opcode::Boolean)) == "Boolean");
    const OpcodeRole& boolEdge = opcodeRole(OpcodeRoleId::BooleanEdge);
    CHECK(boolEdge.opcode == Opcode::Boolean);
    CHECK(boolEdge.seedKind == SemanticKind::Edge);
    CHECK(boolEdge.outputKind == SemanticKind::Edge);
    CHECK(boolEdge.eventKind == EventKind::Generated);
    CHECK(std::string(boolEdge.partOpCode) == "PBF");
    CHECK(std::string(boolEdge.name) == "BooleanEdge");

    CHECK(static_cast<int>(Opcode::AdditiveBox) == 16);
    CHECK(static_cast<int>(OpcodeRoleId::AdditiveBoxFace)
          == static_cast<int>(OpcodeRoleId::BooleanEdge) + 1);
    CHECK(!opcodeRolesFor(Opcode::AdditiveBox).empty());
    CHECK(opcodeRolesFor(Opcode::AdditiveBox).size() >= 2);
    const OpcodeRole& boxFace = opcodeRole(OpcodeRoleId::AdditiveBoxFace);
    CHECK(boxFace.opcode == Opcode::AdditiveBox);
    CHECK(boxFace.seedKind == SemanticKind::Face);
    CHECK(boxFace.outputKind == SemanticKind::Face);
    CHECK(boxFace.eventKind == EventKind::Generated);
    CHECK(std::string(boxFace.partOpCode) == "PBX");
    CHECK(std::string(boxFace.name) == "AdditiveBoxFace");
    CHECK(std::string(opcodeName(Opcode::AdditiveBox)) == "AdditiveBox");
    const OpcodeRole& boxEdge = opcodeRole(OpcodeRoleId::AdditiveBoxEdge);
    CHECK(boxEdge.opcode == Opcode::AdditiveBox);
    CHECK(boxEdge.seedKind == SemanticKind::Edge);
    CHECK(boxEdge.outputKind == SemanticKind::Edge);
    CHECK(boxEdge.eventKind == EventKind::Generated);
    CHECK(std::string(boxEdge.partOpCode) == "PBX");
    CHECK(std::string(boxEdge.name) == "AdditiveBoxEdge");

    CHECK(static_cast<int>(Opcode::AdditiveCylinder) == 17);
    CHECK(static_cast<int>(OpcodeRoleId::AdditiveCylinderFace)
          == static_cast<int>(OpcodeRoleId::AdditiveBoxEdge) + 1);
    CHECK(!opcodeRolesFor(Opcode::AdditiveCylinder).empty());
    CHECK(opcodeRolesFor(Opcode::AdditiveCylinder).size() >= 2);
    const OpcodeRole& cylFace = opcodeRole(OpcodeRoleId::AdditiveCylinderFace);
    CHECK(cylFace.opcode == Opcode::AdditiveCylinder);
    CHECK(cylFace.seedKind == SemanticKind::Face);
    CHECK(cylFace.outputKind == SemanticKind::Face);
    CHECK(cylFace.eventKind == EventKind::Generated);
    CHECK(std::string(cylFace.partOpCode) == "PCY");
    CHECK(std::string(cylFace.name) == "AdditiveCylinderFace");
    CHECK(std::string(opcodeName(Opcode::AdditiveCylinder)) == "AdditiveCylinder");
    const OpcodeRole& cylEdge = opcodeRole(OpcodeRoleId::AdditiveCylinderEdge);
    CHECK(cylEdge.opcode == Opcode::AdditiveCylinder);
    CHECK(cylEdge.seedKind == SemanticKind::Edge);
    CHECK(cylEdge.outputKind == SemanticKind::Edge);
    CHECK(cylEdge.eventKind == EventKind::Generated);
    CHECK(std::string(cylEdge.partOpCode) == "PCY");
    CHECK(std::string(cylEdge.name) == "AdditiveCylinderEdge");

    CHECK(static_cast<int>(Opcode::AdditiveSphere) == 18);
    CHECK(static_cast<int>(OpcodeRoleId::AdditiveSphereFace)
          == static_cast<int>(OpcodeRoleId::AdditiveCylinderEdge) + 1);
    CHECK(!opcodeRolesFor(Opcode::AdditiveSphere).empty());
    CHECK(opcodeRolesFor(Opcode::AdditiveSphere).size() >= 2);
    const OpcodeRole& sphFace = opcodeRole(OpcodeRoleId::AdditiveSphereFace);
    CHECK(sphFace.opcode == Opcode::AdditiveSphere);
    CHECK(sphFace.seedKind == SemanticKind::Face);
    CHECK(sphFace.outputKind == SemanticKind::Face);
    CHECK(sphFace.eventKind == EventKind::Generated);
    CHECK(std::string(sphFace.partOpCode) == "PSP");
    CHECK(std::string(sphFace.name) == "AdditiveSphereFace");
    CHECK(std::string(opcodeName(Opcode::AdditiveSphere)) == "AdditiveSphere");
    const OpcodeRole& sphEdge = opcodeRole(OpcodeRoleId::AdditiveSphereEdge);
    CHECK(sphEdge.opcode == Opcode::AdditiveSphere);
    CHECK(sphEdge.seedKind == SemanticKind::Edge);
    CHECK(sphEdge.outputKind == SemanticKind::Edge);
    CHECK(sphEdge.eventKind == EventKind::Generated);
    CHECK(std::string(sphEdge.partOpCode) == "PSP");
    CHECK(std::string(sphEdge.name) == "AdditiveSphereEdge");

    CHECK(static_cast<int>(Opcode::AdditiveCone) == 19);
    CHECK(static_cast<int>(OpcodeRoleId::AdditiveConeFace)
          == static_cast<int>(OpcodeRoleId::AdditiveSphereEdge) + 1);
    CHECK(!opcodeRolesFor(Opcode::AdditiveCone).empty());
    CHECK(opcodeRolesFor(Opcode::AdditiveCone).size() >= 2);
    const OpcodeRole& coneFace = opcodeRole(OpcodeRoleId::AdditiveConeFace);
    CHECK(coneFace.opcode == Opcode::AdditiveCone);
    CHECK(coneFace.seedKind == SemanticKind::Face);
    CHECK(coneFace.outputKind == SemanticKind::Face);
    CHECK(coneFace.eventKind == EventKind::Generated);
    CHECK(std::string(coneFace.partOpCode) == "PCN");
    CHECK(std::string(coneFace.name) == "AdditiveConeFace");
    CHECK(std::string(opcodeName(Opcode::AdditiveCone)) == "AdditiveCone");
    const OpcodeRole& coneEdge = opcodeRole(OpcodeRoleId::AdditiveConeEdge);
    CHECK(coneEdge.opcode == Opcode::AdditiveCone);
    CHECK(coneEdge.seedKind == SemanticKind::Edge);
    CHECK(coneEdge.outputKind == SemanticKind::Edge);
    CHECK(coneEdge.eventKind == EventKind::Generated);
    CHECK(std::string(coneEdge.partOpCode) == "PCN");
    CHECK(std::string(coneEdge.name) == "AdditiveConeEdge");

    CHECK(static_cast<int>(Opcode::AdditiveTorus) == 20);
    CHECK(static_cast<int>(OpcodeRoleId::AdditiveTorusFace)
          == static_cast<int>(OpcodeRoleId::AdditiveConeEdge) + 1);
    CHECK(!opcodeRolesFor(Opcode::AdditiveTorus).empty());
    CHECK(opcodeRolesFor(Opcode::AdditiveTorus).size() >= 2);
    const OpcodeRole& torFace = opcodeRole(OpcodeRoleId::AdditiveTorusFace);
    CHECK(torFace.opcode == Opcode::AdditiveTorus);
    CHECK(torFace.seedKind == SemanticKind::Face);
    CHECK(torFace.outputKind == SemanticKind::Face);
    CHECK(torFace.eventKind == EventKind::Generated);
    CHECK(std::string(torFace.partOpCode) == "PTO");
    CHECK(std::string(torFace.name) == "AdditiveTorusFace");
    CHECK(std::string(opcodeName(Opcode::AdditiveTorus)) == "AdditiveTorus");
    const OpcodeRole& torEdge = opcodeRole(OpcodeRoleId::AdditiveTorusEdge);
    CHECK(torEdge.opcode == Opcode::AdditiveTorus);
    CHECK(torEdge.seedKind == SemanticKind::Edge);
    CHECK(torEdge.outputKind == SemanticKind::Edge);
    CHECK(torEdge.eventKind == EventKind::Generated);
    CHECK(std::string(torEdge.partOpCode) == "PTO");
    CHECK(std::string(torEdge.name) == "AdditiveTorusEdge");

    CHECK(static_cast<int>(Opcode::AdditivePrism) == 21);
    CHECK(static_cast<int>(OpcodeRoleId::AdditivePrismFace)
          == static_cast<int>(OpcodeRoleId::AdditiveTorusEdge) + 1);
    CHECK(!opcodeRolesFor(Opcode::AdditivePrism).empty());
    CHECK(opcodeRolesFor(Opcode::AdditivePrism).size() >= 2);
    const OpcodeRole& priFace = opcodeRole(OpcodeRoleId::AdditivePrismFace);
    CHECK(priFace.opcode == Opcode::AdditivePrism);
    CHECK(priFace.seedKind == SemanticKind::Face);
    CHECK(priFace.outputKind == SemanticKind::Face);
    CHECK(priFace.eventKind == EventKind::Generated);
    CHECK(std::string(priFace.partOpCode) == "PPR");
    CHECK(std::string(priFace.name) == "AdditivePrismFace");
    CHECK(std::string(opcodeName(Opcode::AdditivePrism)) == "AdditivePrism");
    const OpcodeRole& priEdge = opcodeRole(OpcodeRoleId::AdditivePrismEdge);
    CHECK(priEdge.opcode == Opcode::AdditivePrism);
    CHECK(priEdge.seedKind == SemanticKind::Edge);
    CHECK(priEdge.outputKind == SemanticKind::Edge);
    CHECK(priEdge.eventKind == EventKind::Generated);
    CHECK(std::string(priEdge.partOpCode) == "PPR");
    CHECK(std::string(priEdge.name) == "AdditivePrismEdge");

    CHECK(static_cast<int>(Opcode::AdditiveWedge) == 22);
    CHECK(static_cast<int>(OpcodeRoleId::AdditiveWedgeFace)
          == static_cast<int>(OpcodeRoleId::AdditivePrismEdge) + 1);
    CHECK(!opcodeRolesFor(Opcode::AdditiveWedge).empty());
    CHECK(opcodeRolesFor(Opcode::AdditiveWedge).size() >= 2);
    const OpcodeRole& wedFace = opcodeRole(OpcodeRoleId::AdditiveWedgeFace);
    CHECK(wedFace.opcode == Opcode::AdditiveWedge);
    CHECK(wedFace.seedKind == SemanticKind::Face);
    CHECK(wedFace.outputKind == SemanticKind::Face);
    CHECK(wedFace.eventKind == EventKind::Generated);
    CHECK(std::string(wedFace.partOpCode) == "PWD");
    CHECK(std::string(wedFace.name) == "AdditiveWedgeFace");
    CHECK(std::string(opcodeName(Opcode::AdditiveWedge)) == "AdditiveWedge");
    const OpcodeRole& wedEdge = opcodeRole(OpcodeRoleId::AdditiveWedgeEdge);
    CHECK(wedEdge.opcode == Opcode::AdditiveWedge);
    CHECK(wedEdge.seedKind == SemanticKind::Edge);
    CHECK(wedEdge.outputKind == SemanticKind::Edge);
    CHECK(wedEdge.eventKind == EventKind::Generated);
    CHECK(std::string(wedEdge.partOpCode) == "PWD");
    CHECK(std::string(wedEdge.name) == "AdditiveWedgeEdge");

    CHECK(static_cast<int>(Opcode::SubtractiveBox) == 24);
    CHECK(static_cast<int>(Opcode::SubtractiveCylinder) == 25);
    CHECK(static_cast<int>(Opcode::SubtractiveSphere) == 26);
    CHECK(static_cast<int>(OpcodeRoleId::SubtractiveBoxFace) > static_cast<int>(OpcodeRoleId::AdditiveEllipsoidEdge));
    CHECK(static_cast<int>(OpcodeRoleId::SubtractiveCylinderFace) == static_cast<int>(OpcodeRoleId::SubtractiveBoxEdge) + 1);
    CHECK(static_cast<int>(OpcodeRoleId::SubtractiveSphereFace) == static_cast<int>(OpcodeRoleId::SubtractiveCylinderEdge) + 1);
    CHECK(opcodeRolesFor(Opcode::SubtractiveBox).size() >= 2);
    CHECK(opcodeRolesFor(Opcode::SubtractiveCylinder).size() >= 2);
    CHECK(opcodeRolesFor(Opcode::SubtractiveSphere).size() >= 2);
    const OpcodeRole& subCylFace = opcodeRole(OpcodeRoleId::SubtractiveCylinderFace);
    CHECK(subCylFace.opcode == Opcode::SubtractiveCylinder);
    CHECK(std::string(subCylFace.partOpCode) == "PSC");
    CHECK(std::string(subCylFace.name) == "SubtractiveCylinderFace");
    const OpcodeRole& subCylEdge = opcodeRole(OpcodeRoleId::SubtractiveCylinderEdge);
    CHECK(subCylEdge.seedKind == SemanticKind::Edge);
    CHECK(std::string(subCylEdge.partOpCode) == "PSC");
    const OpcodeRole& subSphFace = opcodeRole(OpcodeRoleId::SubtractiveSphereFace);
    CHECK(subSphFace.opcode == Opcode::SubtractiveSphere);
    CHECK(std::string(subSphFace.partOpCode) == "PSS");
    CHECK(std::string(subSphFace.name) == "SubtractiveSphereFace");
    const OpcodeRole& subSphEdge = opcodeRole(OpcodeRoleId::SubtractiveSphereEdge);
    CHECK(subSphEdge.seedKind == SemanticKind::Edge);
    CHECK(std::string(subSphEdge.partOpCode) == "PSS");
    CHECK(std::string(opcodeName(Opcode::SubtractiveCylinder)) == "SubtractiveCylinder");
    CHECK(std::string(opcodeName(Opcode::SubtractiveSphere)) == "SubtractiveSphere");
    CHECK(static_cast<int>(Opcode::SubtractiveCone) == 27);
    CHECK(static_cast<int>(Opcode::SubtractiveEllipsoid) == 31);
    CHECK(static_cast<int>(Opcode::Scaled) == 32);
    CHECK(static_cast<int>(Opcode::MultiTransform) == 33);
    CHECK(opcodeRolesFor(Opcode::SubtractiveCone).size() >= 2);
    CHECK(opcodeRolesFor(Opcode::SubtractiveTorus).size() >= 2);
    CHECK(opcodeRolesFor(Opcode::SubtractivePrism).size() >= 2);
    CHECK(opcodeRolesFor(Opcode::SubtractiveWedge).size() >= 2);
    CHECK(opcodeRolesFor(Opcode::SubtractiveEllipsoid).size() >= 2);
    CHECK(std::string(opcodeName(Opcode::SubtractiveCone)) == "SubtractiveCone");
    CHECK(std::string(opcodeName(Opcode::SubtractiveTorus)) == "SubtractiveTorus");
    CHECK(std::string(opcodeName(Opcode::SubtractivePrism)) == "SubtractivePrism");
    CHECK(std::string(opcodeName(Opcode::SubtractiveWedge)) == "SubtractiveWedge");
    CHECK(std::string(opcodeName(Opcode::SubtractiveEllipsoid)) == "SubtractiveEllipsoid");
    CHECK(std::string(opcodeName(Opcode::Scaled)) == "Scaled");
    CHECK(std::string(opcodeName(Opcode::MultiTransform)) == "MultiTransform");

    SemanticReference up = referenceFor(SemanticId{}, OpcodeRoleId::PadUpToFace);
    CHECK(up.reducer == CardinalityReducer::RequireOne);
    SemanticReference fe = referenceFor(SemanticId{}, OpcodeRoleId::FilletEdge);
    CHECK(fe.reducer == CardinalityReducer::AcceptAll);
    CHECK(fe.kind == SemanticKind::Edge);
}

void testOpcodeAndRoleTableConsistency()
{
    // Opcode values are persisted by the semantic contract: every value must
    // have a name, and every producing opcode must remain represented by roles.
    // Scaled/MultiTransform intentionally reuse the transformed publisher's
    // source-kind roles, so they are the two metadata-only exceptions.
    for (int value = 0; value < static_cast<int>(Opcode::Count); ++value) {
        const Opcode opcode = static_cast<Opcode>(value);
        const std::string name = opcodeName(opcode);
        CHECK(!name.empty());
        CHECK(name != "Unknown");
        const auto roles = opcodeRolesFor(opcode);
        const bool metadataOnly = opcode == Opcode::Scaled || opcode == Opcode::MultiTransform;
        CHECK(metadataOnly ? roles.empty() : !roles.empty());
        for (const OpcodeRole* role : roles) {
            CHECK(role != nullptr);
            if (role) {
                CHECK(static_cast<int>(role->opcode) == value);
            }
        }
    }

    // opcodeRole() is indexed by OpcodeRoleId; keep the table physically
    // append-only so an insertion cannot silently retarget an existing role.
    for (int value = 0; value < static_cast<int>(OpcodeRoleId::Count); ++value) {
        const OpcodeRole& role = opcodeRole(static_cast<OpcodeRoleId>(value));
        CHECK(static_cast<int>(role.id) == value);
        CHECK(role.name != nullptr && *role.name != 0);
        CHECK(role.partOpCode != nullptr && std::string(role.partOpCode).size() == 3);
        CHECK(opcodeName(role.opcode) != std::string("Unknown"));
    }
}

void testPadFromSketchSeeds()
{
    SemanticGraph g;
    const ObjectId sketch = 10;
    const ObjectId pad = 20;
    g.beginEvaluate(1);
    std::vector<SemanticId> curves;
    for (int i = 1; i <= 4; ++i) {
        SemanticId c = g.recordGenerated(SemanticKind::Edge, "Sketch", sketch, 1, SemanticRole::None);
        g.bind(makeBinding(c, sketch, 1, ("Edge" + std::to_string(i)).c_str()));
        curves.push_back(c);
    }
    const SemanticId region =
        g.recordGenerated(SemanticKind::Region, "Sketch", sketch, 1, SemanticRole::None);
    g.bind(makeBinding(region, sketch, 1, "Face1"));
    g.commitEvaluate();

    g.beginEvaluate(2);
    const auto sides = SemanticEmitter::emitPadSides(&g, curves, pad, 2, 1);
    const auto caps = SemanticEmitter::emitPadCaps(&g, {region}, pad, 2, 5);
    g.commitEvaluate();

    CHECK(sides.size() == 4);
    CHECK(caps.size() == 2);
    CHECK(g.eventGraphIsDag());

    // Each side is a descendant of its curve; not of a neighbour curve (I2).
    for (std::size_t i = 0; i < curves.size(); ++i) {
        const auto desc = g.descendants(curves[i].handle);
        CHECK(desc.count(sides[i].handle) == 1);
        CHECK(desc.count(sides[(i + 1) % 4].handle) == 0);
        CHECK(desc.count(caps[0].handle) == 0);
    }
    const auto regionDesc = g.descendants(region.handle);
    CHECK(regionDesc.count(caps[0].handle) == 1);
    CHECK(regionDesc.count(caps[1].handle) == 1);
    CHECK(regionDesc.count(sides[0].handle) == 0);

    // Generated events have the curve / region as EventInput, not only a marker.
    const auto fromC0 = SemanticEmitter::generatedFrom(g, curves[0].handle);
    CHECK(fromC0.size() == 1);
    CHECK(fromC0.front().handle == sides[0].handle);

    const auto r = SemanticResolver::resolve(makeRef(caps[0], CardinalityReducer::RequireOne,
                                                     SemanticKind::Face),
                                             g);
    CHECK(r.state == ResolutionState::Resolved);
    CHECK(r.identities.front().handle == caps[0].handle);
}

void testPadCapPocketSplitAndSketchInsert()
{
    SemanticGraph g;
    const ObjectId sketch = 10;
    const ObjectId pad = 20;
    const ObjectId pocket = 30;
    const ObjectId fillet = 40;

    g.beginEvaluate(1);
    std::vector<SemanticId> curves;
    for (int i = 1; i <= 4; ++i) {
        SemanticId c = g.recordGenerated(SemanticKind::Edge, "Sketch", sketch, 1, SemanticRole::None);
        g.bind(makeBinding(c, sketch, 1, ("Edge" + std::to_string(i)).c_str()));
        curves.push_back(c);
    }
    const SemanticId region =
        g.recordGenerated(SemanticKind::Region, "Sketch", sketch, 1, SemanticRole::None);
    g.bind(makeBinding(region, sketch, 1, "Face1"));
    g.commitEvaluate();

    g.beginEvaluate(2);
    const auto sides = SemanticEmitter::emitPadSides(&g, curves, pad, 2, 1);
    const auto caps = SemanticEmitter::emitPadCaps(&g, {region}, pad, 2, 5);
    // Cap-boundary edges Generated from the same curve seeds, for fillet.
    std::vector<SemanticId> padEdges;
    for (int i = 0; i < 4; ++i) {
        padEdges.push_back(SemanticEmitter::emitGeneratedFrom(
            &g,
            {curves[static_cast<std::size_t>(i)]},
            SemanticKind::Edge,
            "Pad",
            pad,
            2,
            SemanticRole::None,
            ElementIndex::fromString(("Edge" + std::to_string(i + 1)).c_str())));
    }
    g.commitEvaluate();

    const SemanticId startCap = caps[0];
    const SemanticId endCap = caps[1];
    const SemanticId filletEdge = padEdges[0];

    // Pocket through-cut splits the start cap (not S3 remnant).
    g.beginEvaluate(3);
    g.clearBindings(pad);
    // Rebind survivors that the pocket did not split.
    for (std::size_t i = 0; i < sides.size(); ++i) {
        SemanticEmitter::bind(&g, sides[i], pad, 3,
                              ElementIndex::fromString(("Face" + std::to_string(i + 1)).c_str()));
    }
    SemanticEmitter::bind(&g, endCap, pad, 3, ElementIndex::fromString("Face6"));
    for (std::size_t i = 0; i < padEdges.size(); ++i) {
        SemanticEmitter::bind(&g, padEdges[i], pad, 3,
                              ElementIndex::fromString(("Edge" + std::to_string(i + 1)).c_str()));
    }
    const auto capHalves = SemanticEmitter::emitPocketSplit(&g, startCap, 2, pocket, 3, 10);
    g.commitEvaluate();

    CHECK(capHalves.size() == 2);
    CHECK(capHalves[0].handle != startCap.handle);
    CHECK(capHalves[1].handle != startCap.handle);

    const auto reqOne = SemanticResolver::resolve(
        makeRef(startCap, CardinalityReducer::RequireOne, SemanticKind::Face), g);
    CHECK(reqOne.state == ResolutionState::Ambiguous);
    CHECK(reqOne.identities.empty());

    const auto accAll = SemanticResolver::resolve(
        makeRef(startCap, CardinalityReducer::AcceptAll, SemanticKind::Face), g);
    CHECK(accAll.state == ResolutionState::ResolvedSet);
    CHECK(accAll.identities.size() == 2);
    CHECK(accAll.identities[0].handle == capHalves[0].handle
          || accAll.identities[1].handle == capHalves[0].handle);
    CHECK(accAll.identities[0].handle == capHalves[1].handle
          || accAll.identities[1].handle == capHalves[1].handle);

    // No cross-lineage: cap seed must not bind a side face.
    for (const SemanticBinding& b : accAll.bindings) {
        CHECK(b.stid.handle != sides[0].handle);
        CHECK(b.stid.handle != sides[1].handle);
        CHECK(b.stid.handle != endCap.handle);
    }
    const auto sideOfOther = SemanticResolver::resolve(
        makeRef(sides[0], CardinalityReducer::AcceptAll, SemanticKind::Face), g);
    CHECK(sideOfOther.state == ResolutionState::Resolved);
    CHECK(sideOfOther.identities.front().handle == sides[0].handle);
    CHECK(sideOfOther.identities.front().handle != capHalves[0].handle);

    // Fillet: resolve edge first, generate face from (edge, adjacent faces).
    g.beginEvaluate(4);
    const ResolutionResult fil = SemanticEmitter::emitFillet(
        &g, filletEdge, {sides[0], startCap}, fillet, 4, ElementIndex::fromString("Face20"));
    g.commitEvaluate();
    CHECK(fil.state == ResolutionState::Resolved);
    CHECK(fil.identities.size() >= 1);

    // Sketch-insert split of the generating curve → side + pad-edge split.
    g.beginEvaluate(5);
    const auto curveKids =
        g.recordSplit(curves[0], 2, "SketchInsert", sketch, 5, SemanticRole::None);
    CHECK(curveKids.size() == 2);
    g.bind(makeBinding(curveKids[0], sketch, 5, "Edge1"));
    g.bind(makeBinding(curveKids[1], sketch, 5, "Edge5"));
    const auto prop =
        SemanticEmitter::propagateSourceSplit(&g, curves[0], 2, "Pad", pad, 5, 30);
    CHECK(!prop.empty());
    g.commitEvaluate();

    const auto sideAfterInsert = SemanticResolver::resolve(
        makeRef(sides[0], CardinalityReducer::RequireOne, SemanticKind::Face), g);
    CHECK(sideAfterInsert.state == ResolutionState::Ambiguous);
    const auto sideAll = SemanticResolver::resolve(
        makeRef(sides[0], CardinalityReducer::AcceptAll, SemanticKind::Face), g);
    CHECK(sideAll.state == ResolutionState::ResolvedSet);
    CHECK(sideAll.identities.size() >= 2);
    // The two split remnants of the side must be in the set.
    int sideRemnants = 0;
    for (const SemanticId& id : sideAll.identities) {
        for (const SemanticId& k : prop) {
            if (k.kind == SemanticKind::Face && id.handle == k.handle) {
                ++sideRemnants;
            }
        }
    }
    CHECK(sideRemnants == 2);

    // Cap seed is still the pocket split (sketch insert was a different curve).
    const auto capStill = SemanticResolver::resolve(
        makeRef(startCap, CardinalityReducer::RequireOne, SemanticKind::Face), g);
    CHECK(capStill.state == ResolutionState::Ambiguous);
    const auto capStillAll = SemanticResolver::resolve(
        makeRef(startCap, CardinalityReducer::AcceptAll, SemanticKind::Face), g);
    CHECK(capStillAll.state == ResolutionState::ResolvedSet);
    CHECK(capStillAll.identities.size() >= 2);
    bool hasH0 = false;
    bool hasH1 = false;
    for (const SemanticId& id : capStillAll.identities) {
        if (id.handle == capHalves[0].handle) {
            hasH0 = true;
        }
        if (id.handle == capHalves[1].handle) {
            hasH1 = true;
        }
    }
    CHECK(hasH0);
    CHECK(hasH1);

    // Fillet after the generating curve split: edge remnants, AcceptAll → set.
    const auto edgeAfter = SemanticResolver::resolve(
        makeRef(filletEdge, CardinalityReducer::AcceptAll, SemanticKind::Edge), g);
    CHECK(edgeAfter.state == ResolutionState::ResolvedSet
          || edgeAfter.state == ResolutionState::Resolved);

    // I2: cap seed must not bind a face that is only in another curve's lineage
    // (other pad sides). A multi-parent fillet face may legally appear in both
    // the cap and the adjacent-side descendant sets — that is not a silent
    // cross-lineage retarget.
    for (const SemanticBinding& b : capStillAll.bindings) {
        CHECK(b.stid.handle != sides[1].handle);
        CHECK(b.stid.handle != sides[2].handle);
        CHECK(b.stid.handle != sides[3].handle);
        CHECK(b.stid.handle != endCap.handle);
    }
    for (const SemanticBinding& s : sideAll.bindings) {
        CHECK(s.stid.handle != sides[1].handle);
        CHECK(s.stid.handle != capHalves[0].handle);
        CHECK(s.stid.handle != capHalves[1].handle);
    }
    CHECK(g.eventGraphIsDag());
}

void testFilletMissingIfEdgeDeleted()
{
    SemanticGraph g;
    const ObjectId pad = 20;
    const ObjectId fillet = 40;
    g.beginEvaluate(1);
    const SemanticId edge =
        g.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::None);
    const SemanticId f1 =
        g.recordGenerated(SemanticKind::Face, "Pad", pad, 1, SemanticRole::None);
    const SemanticId f2 =
        g.recordGenerated(SemanticKind::Face, "Pad", pad, 1, SemanticRole::None);
    g.bind(makeBinding(edge, pad, 1, "Edge1"));
    g.bind(makeBinding(f1, pad, 1, "Face1"));
    g.bind(makeBinding(f2, pad, 1, "Face2"));
    g.commitEvaluate();

    // Delete the edge. Neighbour faces remain. Fillet must be Missing, not a
    // similar-length neighbour bind (I2 / I10 / §10).
    g.beginEvaluate(2);
    g.clearBindings(pad);
    SemanticEmitter::bind(&g, f1, pad, 2, ElementIndex::fromString("Face1"));
    SemanticEmitter::bind(&g, f2, pad, 2, ElementIndex::fromString("Face2"));
    SemanticEmitter::emitDeleted(&g, edge, "Pocket", 30, 2, SemanticRole::DressUpEdge);
    g.commitEvaluate();

    const ResolutionResult fil = SemanticEmitter::emitFillet(
        &g, edge, {f1, f2}, fillet, 3, ElementIndex::fromString("Face9"));
    CHECK(fil.state == ResolutionState::Missing);
    CHECK(fil.identities.empty());

    // Neighbour faces did not become the fillet seed.
    const auto neighbour = SemanticResolver::resolve(
        makeRef(edge, CardinalityReducer::AcceptAll, SemanticKind::Edge), g);
    CHECK(neighbour.state == ResolutionState::Missing);
    CHECK(neighbour.bindings.empty());

    // A later feature resolving the deleted edge under RequireOne is also Missing.
    const auto req = SemanticResolver::resolve(
        makeRef(edge, CardinalityReducer::RequireOne, SemanticKind::Edge), g);
    CHECK(req.state == ResolutionState::Missing);
}

void testPocketS3RemnantKeepsHandle()
{
    SemanticGraph g;
    const ObjectId pad = 20;
    const ObjectId pocket = 30;
    g.beginEvaluate(1);
    const SemanticId cap =
        g.recordGenerated(SemanticKind::Face, "Pad", pad, 1, SemanticRole::None);
    const SemanticId holeRegion =
        g.recordGenerated(SemanticKind::Region, "Sketch", 11, 1, SemanticRole::None);
    g.bind(makeBinding(cap, pad, 1, "Face5"));
    g.commitEvaluate();

    CHECK(opcodeRole(OpcodeRoleId::PocketRemnant).remnantIsContinuation);

    g.beginEvaluate(2);
    g.clearBindings(pad);
    const SemanticId same = SemanticEmitter::emitPocketHole(
        &g, cap, holeRegion, 4, pocket, 2, ElementIndex::fromString("Face5"), 20);
    g.commitEvaluate();

    CHECK(same.handle == cap.handle);
    const auto r = SemanticResolver::resolve(
        makeRef(cap, CardinalityReducer::RequireOne, SemanticKind::Face), g);
    CHECK(r.state == ResolutionState::Resolved);
    CHECK(r.identities.front().handle == cap.handle);
    CHECK(!g.aliasUsedForModified());

    const auto walls = SemanticEmitter::generatedFrom(g, holeRegion.handle);
    CHECK(walls.size() == 4);
    for (const SemanticId& w : walls) {
        CHECK(w.handle != cap.handle);
    }
}

void testUpToFaceIncompatibleOnSet()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId face =
        g.recordGenerated(SemanticKind::Face, "Pad", 20, 1, SemanticRole::None);
    g.bind(makeBinding(face, 20, 1, "Face1"));
    g.commitEvaluate();

    g.beginEvaluate(2);
    g.clearBindings(20);
    SemanticEmitter::emitPocketSplit(&g, face, 2, 30, 2, 1);
    g.commitEvaluate();

    SemanticReference ref = referenceFor(face, OpcodeRoleId::PadUpToFace);
    ref.seed = face;
    const ReferenceRequirement req = requirementFor(OpcodeRoleId::PadUpToFace);
    const auto r = SemanticResolver::resolve(ref, g, &req);
    // RequireOne + N descendants → Ambiguous (not a coerced singleton). I12.
    CHECK(r.state == ResolutionState::Ambiguous || r.state == ResolutionState::Incompatible);
}

void testNamedOutputTypeGuards()
{
    SemanticGraph pad;
    const SemanticId padCurve =
        pad.recordGenerated(SemanticKind::Edge, "Sketch", 11, 1, SemanticRole::None);
    AfterExecuteRequest padRequest;
    padRequest.curveSeeds = {padCurve};
    padRequest.namedFaceIndices = {ElementIndex::fromString("Edge2")};
    SemanticEmitter::afterExecute(&pad, Opcode::Pad, 20, 1, padRequest);
    CHECK(SemanticEmitter::lastAfterExecuteNote().find("unnamed=1") != std::string::npos);
    CHECK(SemanticEmitter::generatedFrom(pad, padCurve.handle).empty());

    SemanticGraph revolution;
    const SemanticId revolutionCurve =
        revolution.recordGenerated(SemanticKind::Edge, "Sketch", 12, 1, SemanticRole::None);
    AfterExecuteRequest revolutionRequest;
    revolutionRequest.curveSeeds = {revolutionCurve};
    revolutionRequest.namedFaceIndices = {ElementIndex::fromString("Edge3")};
    SemanticEmitter::afterExecute(
        &revolution, Opcode::Revolution, 21, 1, revolutionRequest);
    CHECK(SemanticEmitter::lastAfterExecuteNote().find("unnamed=1") != std::string::npos);
    CHECK(SemanticEmitter::generatedFrom(revolution, revolutionCurve.handle).empty());
}

void testPocketAfterExecuteBindsNamedEdge2OnPocketId()
{
    // Pocket afterExecute namedEdgeIndices path: type-local Edge2 bound with
    // Pocket getID(), not Pad's id and not sequential Edge1.
    SemanticGraph g;
    const ObjectId sketch = 11;
    const ObjectId pocket = 30;
    g.beginEvaluate(1);
    const SemanticId vertex =
        g.recordGenerated(SemanticKind::Vertex, "Sketch", sketch, 1, SemanticRole::None);
    AfterExecuteRequest req;
    req.vertexSeeds = {vertex};
    req.namedEdgeIndices = {ElementIndex::fromString("Edge2")};
    req.allowSequentialFaceN = false;
    SemanticEmitter::afterExecute(&g, Opcode::Pocket, pocket, 1, req);
    g.commitEvaluate();

    CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted pocket") == 0);
    CHECK(SemanticEmitter::lastAfterExecuteNote().find("edges=1") != std::string::npos);
    const auto kids = SemanticEmitter::generatedFrom(g, vertex.handle);
    CHECK(kids.size() == 1);
    CHECK(kids.front().kind == SemanticKind::Edge);
    const auto rows = g.bindingsOf(kids.front().handle);
    CHECK(rows.size() == 1);
    CHECK(rows.front().feature == pocket);
    CHECK(rows.front().index.toString() == "Edge2");
    CHECK(rows.front().index.toString() != "Edge1");
}

void testIdenticalGeometryDifferentProvenance()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId curveA =
        g.recordGenerated(SemanticKind::Edge, "Sketch", 10, 1, SemanticRole::None);
    const SemanticId curveB =
        g.recordGenerated(SemanticKind::Edge, "Sketch", 10, 1, SemanticRole::None);
    g.commitEvaluate();

    g.beginEvaluate(2);
    const auto sideA = SemanticEmitter::emitPadSides(&g, {curveA}, 20, 2, 1);
    const auto sideB = SemanticEmitter::emitPadSides(&g, {curveB}, 20, 2, 2);
    g.commitEvaluate();

    CHECK(sideA.size() == 1);
    CHECK(sideB.size() == 1);

    const auto r = SemanticResolver::resolve(
        makeRef(sideA[0], CardinalityReducer::AcceptAll, SemanticKind::Face), g);
    CHECK(r.state == ResolutionState::Resolved);
    CHECK(r.identities.size() == 1);
    CHECK(r.identities.front().handle == sideA[0].handle);
    CHECK(r.identities.front().handle != sideB[0].handle);
}

void testUniqueFaceBindingOnLinkedFeature()
{
    // Consume uniqueness (I13): 0 or >1 Face Bindings on the linked feature
    // refuse. Leftover rows on another feature do not count. Never first-wins.
    SemanticGraph g;
    const ObjectId pad = 20;
    const ObjectId leftover = 30;
    g.beginEvaluate(1);
    const SemanticId face =
        g.recordGenerated(SemanticKind::Face, "Pad", pad, 1, SemanticRole::None);
    g.commitEvaluate();

    CHECK(!uniqueFaceBindingOnFeature(nullptr, face, pad).has_value());
    CHECK(!uniqueFaceBindingOnFeature(&g, SemanticId{}, pad).has_value());
    CHECK(!uniqueFaceBindingOnFeature(&g, face, 0).has_value());
    CHECK(!uniqueFaceBindingOnFeature(&g, face, pad).has_value());  // 0 rows

    g.beginEvaluate(2);
    g.bind(makeBinding(face, pad, 2, "Face6"));
    g.commitEvaluate();
    const auto one = uniqueFaceBindingOnFeature(&g, face, pad);
    CHECK(one.has_value());
    CHECK(one->feature == pad);
    CHECK(one->index.toString() == std::string("Face6"));
    CHECK(!uniqueFaceBindingOnFeature(&g, face, leftover).has_value());

    g.beginEvaluate(3);
    g.bind(makeBinding(face, leftover, 3, "Face6"));
    g.commitEvaluate();
    const auto still = uniqueFaceBindingOnFeature(&g, face, pad);
    CHECK(still.has_value());
    CHECK(still->feature == pad);
    CHECK(still->index.toString() == std::string("Face6"));

    g.beginEvaluate(4);
    g.bind(makeBinding(face, pad, 4, "Face7"));
    g.commitEvaluate();
    CHECK(!uniqueFaceBindingOnFeature(&g, face, pad).has_value());  // >1 on Pad
}

void testUniqueResolvedFaceReference()
{
    SemanticGraph g;
    const ObjectId feature = 20;
    const ObjectId other = 30;
    g.beginEvaluate(1);
    const SemanticId face =
        g.recordGenerated(SemanticKind::Face, "Pad", feature, 1, SemanticRole::None);
    g.commitEvaluate();

    SemanticReference ref = makeRef(face, CardinalityReducer::RequireOne, SemanticKind::Face);
    CHECK(!uniqueResolvedFaceReference(&g, ref, feature).has_value());

    g.beginEvaluate(2);
    g.bind(makeBinding(face, feature, 2, "Face6"));
    g.commitEvaluate();
    const auto resolved = uniqueResolvedFaceReference(&g, ref, feature);
    CHECK(resolved.has_value());
    CHECK(resolved->index.toString() == std::string("Face6"));
    CHECK(!uniqueResolvedFaceReference(&g, ref, other).has_value());

    g.beginEvaluate(3);
    g.bind(makeBinding(face, feature, 3, "Face7"));
    g.commitEvaluate();
    CHECK(!uniqueResolvedFaceReference(&g, ref, feature).has_value());

    SemanticReference wrongKind = ref;
    wrongKind.kind = SemanticKind::Edge;
    CHECK(!uniqueResolvedFaceReference(&g, wrongKind, feature).has_value());
}

void testUniqueEdgeBindingOnLinkedFeature()
{
    // Edge consumers use the same feature-scoped I13 uniqueness policy as Face consumers.
    SemanticGraph g;
    const ObjectId base = 20;
    const ObjectId leftover = 30;
    g.beginEvaluate(1);
    const SemanticId edge =
        g.recordGenerated(SemanticKind::Edge, "Pad", base, 1, SemanticRole::None);
    g.commitEvaluate();

    CHECK(!uniqueEdgeBindingOnFeature(nullptr, edge, base).has_value());
    CHECK(!uniqueEdgeBindingOnFeature(&g, SemanticId{}, base).has_value());
    CHECK(!uniqueEdgeBindingOnFeature(&g, edge, 0).has_value());
    CHECK(!uniqueEdgeBindingOnFeature(&g, edge, base).has_value());

    g.beginEvaluate(2);
    g.bind(makeBinding(edge, base, 2, "Edge6"));
    g.commitEvaluate();
    const auto one = uniqueEdgeBindingOnFeature(&g, edge, base);
    CHECK(one.has_value());
    CHECK(one->feature == base);
    CHECK(one->index.toString() == std::string("Edge6"));
    CHECK(!uniqueEdgeBindingOnFeature(&g, edge, leftover).has_value());

    g.beginEvaluate(3);
    g.bind(makeBinding(edge, leftover, 3, "Edge6"));
    g.commitEvaluate();
    CHECK(uniqueEdgeBindingOnFeature(&g, edge, base).has_value());

    g.beginEvaluate(4);
    g.bind(makeBinding(edge, base, 4, "Edge7"));
    g.commitEvaluate();
    CHECK(!uniqueEdgeBindingOnFeature(&g, edge, base).has_value());
}

void testRequestSeedDeduplication()
{
    std::vector<SemanticId> seeds;
    SemanticId face;
    face.handle = 41;
    face.kind = SemanticKind::Face;
    SemanticId edge = face;
    edge.handle = 42;
    edge.kind = SemanticKind::Edge;

    CHECK(!appendUniqueSemanticSeed(seeds, SemanticId{}));
    CHECK(appendUniqueSemanticSeed(seeds, face));
    CHECK(!appendUniqueSemanticSeed(seeds, face));
    CHECK(appendUniqueSemanticSeed(seeds, edge));
    CHECK(seeds.size() == 2);
    CHECK(seeds[0] == face);
    CHECK(seeds[1] == edge);
}

void testCollectProfileSeedsPromotesFaceCacheOnlyWithUniqueBinding()
{
    const ObjectId profile = 40;
    SemanticReference cached;
    cached.kind = SemanticKind::Face;
    cached.fallback = ElementIndex::fromString("Face4");

    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId face =
        g.recordGenerated(SemanticKind::Face, "Fillet", profile, 1, SemanticRole::None);
    g.bind(makeBinding(face, profile, 1, "Face4"));
    g.commitEvaluate();

    // Loft, Pipe, and Helix share this collector; duplicate LinkSub rows
    // must not duplicate one profile seed in either role zipper.
    const AfterExecuteRequest promoted =
        SemanticEmitter::collectProfileSeeds(&g, profile, {cached, cached});
    CHECK(promoted.curveSeeds.size() == 1);
    CHECK(promoted.regionSeeds.size() == 1);
    CHECK(promoted.curveSeeds.front() == face);
    CHECK(promoted.regionSeeds.front() == face);

    SemanticGraph noBinding;
    noBinding.beginEvaluate(1);
    noBinding.commitEvaluate();
    const AfterExecuteRequest refused =
        SemanticEmitter::collectProfileSeeds(&noBinding, profile, {cached});
    CHECK(refused.curveSeeds.empty());
    CHECK(refused.regionSeeds.empty());
}

}  // namespace

int main()
{
    testNoopWithoutGraph();
    testGraphForFindsDocumentGraph();
    testSemanticRepublishGate();
    testRoleTablesExist();
    testOpcodeAndRoleTableConsistency();
    testPadFromSketchSeeds();
    testPadCapPocketSplitAndSketchInsert();
    testFilletMissingIfEdgeDeleted();
    testPocketS3RemnantKeepsHandle();
    testUpToFaceIncompatibleOnSet();
    testNamedOutputTypeGuards();
    testPocketAfterExecuteBindsNamedEdge2OnPocketId();
    testIdenticalGeometryDifferentProvenance();
    testUniqueFaceBindingOnLinkedFeature();
    testUniqueResolvedFaceReference();
    testUniqueEdgeBindingOnLinkedFeature();
    testRequestSeedDeduplication();
    testCollectProfileSeedsPromotesFaceCacheOnlyWithUniqueBinding();

    std::cout << "Semantic opcode Phase 5: " << g_passed << " checks passed, " << g_failed
              << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
