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

#include "SemanticOpcode.h"

#include <App/SemanticDocumentState.h>
#include <App/SemanticReference.h>
#include <App/ComplexGeoData.h>
#include <App/PropertyGeo.h>
#include <App/IndexedName.h>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/SemanticHistoryAdapter.h>
#include <Mod/Part/App/SemanticSourceCollector.h>
#include <Mod/PartDesign/App/FeatureAddSub.h>
#include <Base/Console.h>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Shape.hxx>
#include <deque>
#include <unordered_set>

#include <algorithm>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <string>

namespace PartDesign
{
namespace
{

constexpr App::FilterFlag kSameKind =
    App::FilterFlag::DescendantsOfSeed | App::FilterFlag::SameKind;

const OpcodeRole kRoles[] = {
    // Pad / pocket: sides from curves, caps from regions. Source split
    // implies side or cap split. UpToFace is inbound RequireOne (§4.2 / §10).
    {Opcode::Pad,
     OpcodeRoleId::PadSide,
     "PadSide",
     "XTR",
     App::SemanticKind::Edge,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Pad,
     OpcodeRoleId::PadCap,
     "PadCap",
     "XTR",
     App::SemanticKind::Region,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Pad,
     OpcodeRoleId::PadUpToFace,
     "PadUpToFace",
     "XTR",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Modified,
     App::SemanticRole::UpToFace,
     kSameKind,
     App::CardinalityReducer::RequireOne,
     App::AcceptedCardinality::One,
     false},
    {Opcode::Pocket,
     OpcodeRoleId::PocketSide,
     "PocketSide",
     "CUT",
     App::SemanticKind::Edge,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Pocket,
     OpcodeRoleId::PocketCap,
     "PocketCap",
     "CUT",
     App::SemanticKind::Region,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Pocket,
     OpcodeRoleId::PocketUpToFace,
     "PocketUpToFace",
     "CUT",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Modified,
     App::SemanticRole::UpToFace,
     kSameKind,
     App::CardinalityReducer::RequireOne,
     App::AcceptedCardinality::One,
     false},
    {Opcode::Pocket,
     OpcodeRoleId::PocketRemnant,
     "PocketRemnant",
     "CUT",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Modified,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::RequireOne,
     App::AcceptedCardinality::One,
     true},  // S3: remnant is the continuation
    {Opcode::Pocket,
     OpcodeRoleId::PocketSplit,
     "PocketSplit",
     "CUT",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Split,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},  // through-cut / L: no continuation
    // Fillet: resolve the edge first. Default reducer AcceptAll. Missing
    // edge is Missing dress-up, not "similar length nearby." New faces
    // Generated from (edge seed, adjacent face seeds).
    {Opcode::Fillet,
     OpcodeRoleId::FilletEdge,
     "FilletEdge",
     "FLT",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Modified,
     App::SemanticRole::DressUpEdge,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Fillet,
     OpcodeRoleId::FilletFace,
     "FilletFace",
     "FLT",
     App::SemanticKind::Edge,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // Chamfer: clone Fillet roles. Append-only so opcodeRole() index stays valid.
    {Opcode::Chamfer,
     OpcodeRoleId::ChamferEdge,
     "ChamferEdge",
     "CHF",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Modified,
     App::SemanticRole::DressUpEdge,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Chamfer,
     OpcodeRoleId::ChamferFace,
     "ChamferFace",
     "CHF",
     App::SemanticKind::Edge,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // Draft: inbound Face (AcceptAll). Generated result from that seed.
    // Append-only so opcodeRole() index stays valid. partOpCode DFT.
    {Opcode::Draft,
     OpcodeRoleId::DraftFace,
     "DraftFace",
     "DFT",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Modified,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Draft,
     OpcodeRoleId::DraftResult,
     "DraftResult",
     "DFT",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // Thickness: inbound Face (AcceptAll). Generated result from that seed.
    // Append-only so opcodeRole() index stays valid. partOpCode THK.
    {Opcode::Thickness,
     OpcodeRoleId::ThicknessFace,
     "ThicknessFace",
     "THK",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Modified,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Thickness,
     OpcodeRoleId::ThicknessResult,
     "ThicknessResult",
     "THK",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // Revolution / Groove: Pad/Pocket clone. Append-only. partOpCode RVL.
    // UpTo* stay unnamed (I10) — role exists so opcodeRole() is defined.
    {Opcode::Revolution,
     OpcodeRoleId::RevolutionSide,
     "RevolutionSide",
     "RVL",
     App::SemanticKind::Edge,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Revolution,
     OpcodeRoleId::RevolutionCap,
     "RevolutionCap",
     "RVL",
     App::SemanticKind::Region,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Revolution,
     OpcodeRoleId::RevolutionUpToFace,
     "RevolutionUpToFace",
     "RVL",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Modified,
     App::SemanticRole::UpToFace,
     kSameKind,
     App::CardinalityReducer::RequireOne,
     App::AcceptedCardinality::One,
     false},
    {Opcode::Groove,
     OpcodeRoleId::GrooveSide,
     "GrooveSide",
     "RVL",
     App::SemanticKind::Edge,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Groove,
     OpcodeRoleId::GrooveCap,
     "GrooveCap",
     "RVL",
     App::SemanticKind::Region,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Groove,
     OpcodeRoleId::GrooveUpToFace,
     "GrooveUpToFace",
     "RVL",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Modified,
     App::SemanticRole::UpToFace,
     kSameKind,
     App::CardinalityReducer::RequireOne,
     App::AcceptedCardinality::One,
     false},
    // Hole: inbound StartReference Face (AcceptAll). Generated result from
    // that seed when history names an index. Append-only. partOpCode HOL.
    {Opcode::Hole,
     OpcodeRoleId::HoleStart,
     "HoleStart",
     "HOL",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Modified,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Hole,
     OpcodeRoleId::HoleResult,
     "HoleResult",
     "HOL",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // LinearPattern / PolarPattern / Mirrored: unique 1-image Face/Edge
    // fromMaker in FeatureTransformed::execute. Append-only. partOpCode LPT/PPT/PMR.
    {Opcode::LinearPattern,
     OpcodeRoleId::LinearPatternFace,
     "LinearPatternFace",
     "LPT",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::LinearPattern,
     OpcodeRoleId::LinearPatternEdge,
     "LinearPatternEdge",
     "LPT",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::PolarPattern,
     OpcodeRoleId::PolarPatternFace,
     "PolarPatternFace",
     "PPT",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::PolarPattern,
     OpcodeRoleId::PolarPatternEdge,
     "PolarPatternEdge",
     "PPT",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Mirrored,
     OpcodeRoleId::MirroredFace,
     "MirroredFace",
     "PMR",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Mirrored,
     OpcodeRoleId::MirroredEdge,
     "MirroredEdge",
     "PMR",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // Loft / Pipe / Helix: Revolution clone (Side+Cap). Append-only so
    // opcodeRole() index stays valid. partOpCode LFT / PIP / HLX.
    {Opcode::Loft,
     OpcodeRoleId::LoftSide,
     "LoftSide",
     "LFT",
     App::SemanticKind::Edge,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Loft,
     OpcodeRoleId::LoftCap,
     "LoftCap",
     "LFT",
     App::SemanticKind::Region,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Pipe,
     OpcodeRoleId::PipeSide,
     "PipeSide",
     "PIP",
     App::SemanticKind::Edge,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Pipe,
     OpcodeRoleId::PipeCap,
     "PipeCap",
     "PIP",
     App::SemanticKind::Region,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Helix,
     OpcodeRoleId::HelixSide,
     "HelixSide",
     "HLX",
     App::SemanticKind::Edge,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Helix,
     OpcodeRoleId::HelixCap,
     "HelixCap",
     "HLX",
     App::SemanticKind::Region,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // PartDesign Boolean Fuse: unique 1-image Face/Edge. Append-only so
    // opcodeRole() index stays valid. partOpCode PBF (not Part FUS).
    {Opcode::Boolean,
     OpcodeRoleId::BooleanFace,
     "BooleanFace",
     "PBF",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::Boolean,
     OpcodeRoleId::BooleanEdge,
     "BooleanEdge",
     "PBF",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // PartDesign AdditiveBox first-solid: unique 1-image Face/Edge. Append-only
    // so opcodeRole() index stays valid. partOpCode PBX (not Part BOX).
    {Opcode::AdditiveBox,
     OpcodeRoleId::AdditiveBoxFace,
     "AdditiveBoxFace",
     "PBX",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::AdditiveBox,
     OpcodeRoleId::AdditiveBoxEdge,
     "AdditiveBoxEdge",
     "PBX",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // PartDesign AdditiveCylinder first-solid: unique 1-image Face/Edge. Append-only
    // so opcodeRole() index stays valid. partOpCode PCY (not Part CYL).
    {Opcode::AdditiveCylinder,
     OpcodeRoleId::AdditiveCylinderFace,
     "AdditiveCylinderFace",
     "PCY",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::AdditiveCylinder,
     OpcodeRoleId::AdditiveCylinderEdge,
     "AdditiveCylinderEdge",
     "PCY",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // PartDesign AdditiveSphere first-solid: unique 1-image Face/Edge. Append-only
    // so opcodeRole() index stays valid. partOpCode PSP (not Part SPH).
    {Opcode::AdditiveSphere,
     OpcodeRoleId::AdditiveSphereFace,
     "AdditiveSphereFace",
     "PSP",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::AdditiveSphere,
     OpcodeRoleId::AdditiveSphereEdge,
     "AdditiveSphereEdge",
     "PSP",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // PartDesign AdditiveCone first-solid: unique 1-image Face/Edge. Append-only
    // so opcodeRole() index stays valid. partOpCode PCN (not Part CON).
    {Opcode::AdditiveCone,
     OpcodeRoleId::AdditiveConeFace,
     "AdditiveConeFace",
     "PCN",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::AdditiveCone,
     OpcodeRoleId::AdditiveConeEdge,
     "AdditiveConeEdge",
     "PCN",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // PartDesign AdditiveTorus first-solid: unique 1-image Face/Edge. Append-only
    // so opcodeRole() index stays valid. partOpCode PTO (not Part TOR).
    {Opcode::AdditiveTorus,
     OpcodeRoleId::AdditiveTorusFace,
     "AdditiveTorusFace",
     "PTO",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::AdditiveTorus,
     OpcodeRoleId::AdditiveTorusEdge,
     "AdditiveTorusEdge",
     "PTO",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // PartDesign AdditivePrism first-solid: unique 1-image Face/Edge. Append-only
    // so opcodeRole() index stays valid. partOpCode PPR (not Part PRI).
    {Opcode::AdditivePrism,
     OpcodeRoleId::AdditivePrismFace,
     "AdditivePrismFace",
     "PPR",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::AdditivePrism,
     OpcodeRoleId::AdditivePrismEdge,
     "AdditivePrismEdge",
     "PPR",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // PartDesign AdditiveWedge first-solid: unique 1-image Face/Edge. Append-only
    // so opcodeRole() index stays valid. partOpCode PWD (Box/Prism style).
    {Opcode::AdditiveWedge,
     OpcodeRoleId::AdditiveWedgeFace,
     "AdditiveWedgeFace",
     "PWD",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::AdditiveWedge,
     OpcodeRoleId::AdditiveWedgeEdge,
     "AdditiveWedgeEdge",
     "PWD",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // PartDesign AdditiveEllipsoid first-solid: unique 1-image Face/Edge. Append-only
    // so opcodeRole() index stays valid. partOpCode PEL (Sphere/Wedge style).
    {Opcode::AdditiveEllipsoid,
     OpcodeRoleId::AdditiveEllipsoidFace,
     "AdditiveEllipsoidFace",
     "PEL",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::AdditiveEllipsoid,
     OpcodeRoleId::AdditiveEllipsoidEdge,
     "AdditiveEllipsoidEdge",
     "PEL",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // PartDesign SubtractiveBox with-base Cut: unique 1-image Face/Edge. Append-only
    // so opcodeRole() index stays valid. Dedicated opcode 24 (not AdditiveBox/Boolean).
    // partOpCode PSB. Emit on SubtractiveBox feature via live FCBRepAlgoAPI_Cut.
    {Opcode::SubtractiveBox,
     OpcodeRoleId::SubtractiveBoxFace,
     "SubtractiveBoxFace",
     "PSB",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::SubtractiveBox,
     OpcodeRoleId::SubtractiveBoxEdge,
     "SubtractiveBoxEdge",
     "PSB",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // PartDesign SubtractiveCylinder/Sphere with-base Cut roles.
    {Opcode::SubtractiveCylinder,
     OpcodeRoleId::SubtractiveCylinderFace,
     "SubtractiveCylinderFace",
     "PSC",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::SubtractiveCylinder,
     OpcodeRoleId::SubtractiveCylinderEdge,
     "SubtractiveCylinderEdge",
     "PSC",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::SubtractiveSphere,
     OpcodeRoleId::SubtractiveSphereFace,
     "SubtractiveSphereFace",
     "PSS",
     App::SemanticKind::Face,
     App::SemanticKind::Face,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    {Opcode::SubtractiveSphere,
     OpcodeRoleId::SubtractiveSphereEdge,
     "SubtractiveSphereEdge",
     "PSS",
     App::SemanticKind::Edge,
     App::SemanticKind::Edge,
     App::EventKind::Generated,
     App::SemanticRole::None,
     kSameKind,
     App::CardinalityReducer::AcceptAll,
     App::AcceptedCardinality::OneOrMore,
     false},
    // Remaining PartDesign subtractive primitive with-base Cut roles.
    {Opcode::SubtractiveCone, OpcodeRoleId::SubtractiveConeFace, "SubtractiveConeFace", "PCN", App::SemanticKind::Face, App::SemanticKind::Face, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractiveCone, OpcodeRoleId::SubtractiveConeEdge, "SubtractiveConeEdge", "PCN", App::SemanticKind::Edge, App::SemanticKind::Edge, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractiveTorus, OpcodeRoleId::SubtractiveTorusFace, "SubtractiveTorusFace", "PTO", App::SemanticKind::Face, App::SemanticKind::Face, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractiveTorus, OpcodeRoleId::SubtractiveTorusEdge, "SubtractiveTorusEdge", "PTO", App::SemanticKind::Edge, App::SemanticKind::Edge, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractivePrism, OpcodeRoleId::SubtractivePrismFace, "SubtractivePrismFace", "PPR", App::SemanticKind::Face, App::SemanticKind::Face, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractivePrism, OpcodeRoleId::SubtractivePrismEdge, "SubtractivePrismEdge", "PPR", App::SemanticKind::Edge, App::SemanticKind::Edge, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractiveWedge, OpcodeRoleId::SubtractiveWedgeFace, "SubtractiveWedgeFace", "PWD", App::SemanticKind::Face, App::SemanticKind::Face, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractiveWedge, OpcodeRoleId::SubtractiveWedgeEdge, "SubtractiveWedgeEdge", "PWD", App::SemanticKind::Edge, App::SemanticKind::Edge, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractiveEllipsoid, OpcodeRoleId::SubtractiveEllipsoidFace, "SubtractiveEllipsoidFace", "PEL", App::SemanticKind::Face, App::SemanticKind::Face, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractiveEllipsoid, OpcodeRoleId::SubtractiveEllipsoidEdge, "SubtractiveEllipsoidEdge", "PEL", App::SemanticKind::Edge, App::SemanticKind::Edge, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractiveLoft, OpcodeRoleId::SubtractiveLoftSide, "SubtractiveLoftSide", "SLF", App::SemanticKind::Edge, App::SemanticKind::Face, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractiveLoft, OpcodeRoleId::SubtractiveLoftCap, "SubtractiveLoftCap", "SLF", App::SemanticKind::Region, App::SemanticKind::Face, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractivePipe, OpcodeRoleId::SubtractivePipeSide, "SubtractivePipeSide", "SPP", App::SemanticKind::Edge, App::SemanticKind::Face, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractivePipe, OpcodeRoleId::SubtractivePipeCap, "SubtractivePipeCap", "SPP", App::SemanticKind::Region, App::SemanticKind::Face, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractiveHelix, OpcodeRoleId::SubtractiveHelixSide, "SubtractiveHelixSide", "SHX", App::SemanticKind::Edge, App::SemanticKind::Face, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},
    {Opcode::SubtractiveHelix, OpcodeRoleId::SubtractiveHelixCap, "SubtractiveHelixCap", "SHX", App::SemanticKind::Region, App::SemanticKind::Face, App::EventKind::Generated, App::SemanticRole::None, kSameKind, App::CardinalityReducer::AcceptAll, App::AcceptedCardinality::OneOrMore, false},

};

App::ElementIndex faceIndex(int n)
{
    App::ElementIndex idx;
    idx.type = "Face";
    idx.index = n;
    return idx;
}

void appendUniqueSemanticId(std::vector<App::SemanticId>& seeds,
                            const App::SemanticId& addition)
{
    if (!addition.valid()) {
        return;
    }
    if (std::find(seeds.begin(), seeds.end(), addition) == seeds.end()) {
        seeds.push_back(addition);
    }
}

void appendUniqueSemanticIds(std::vector<App::SemanticId>& seeds,
                             const std::vector<App::SemanticId>& additions)
{
    for (const App::SemanticId& addition : additions) {
        appendUniqueSemanticId(seeds, addition);
    }
}

bool isValidNamedIndex(const App::ElementIndex& index, const char* expectedType);

bool emitSeedlessDressUpFaces(App::SemanticGraph* graph,
                              const char* op,
                              App::ObjectId feature,
                              App::EvalSerial eval,
                              const std::vector<App::ElementIndex>& namedFaces,
                              std::string& note)
{
    if (!graph || namedFaces.empty()) {
        return false;
    }
    std::map<int, int> faceCounts;
    for (const App::ElementIndex& idx : namedFaces) {
        if (isValidNamedIndex(idx, "Face")) {
            ++faceCounts[idx.index];
        }
    }
    int nOk = 0;
    int nAmbiguous = 0;
    std::set<int> emitted;
    for (const App::ElementIndex& idx : namedFaces) {
        if (!isValidNamedIndex(idx, "Face")) {
            continue;
        }
        if (faceCounts[idx.index] != 1) {
            ++nAmbiguous;
            continue;
        }
        if (!emitted.insert(idx.index).second) {
            continue;
        }
        const App::SemanticId child = graph->recordGeneratedFrom(
            {}, App::SemanticKind::Face, op, feature, eval, App::SemanticRole::None);
        App::SemanticBinding row;
        row.stid = child;
        row.feature = feature;
        row.eval = eval;
        row.kind = App::SemanticKind::Face;
        row.index = idx;
        graph->bind(row);
        ++nOk;
    }
    if (nOk > 0) {
        note = std::string("emitted seedless ") + op + " faces=" + std::to_string(nOk)
            + " ambiguous=" + std::to_string(nAmbiguous);
        return true;
    }
    note.clear();
    return false;
}

}  // namespace

const OpcodeRole& opcodeRole(OpcodeRoleId id)
{
    const auto n = static_cast<std::size_t>(id);
    if (n < (sizeof(kRoles) / sizeof(kRoles[0]))) {
        return kRoles[n];
    }
    return kRoles[0];
}

std::vector<const OpcodeRole*> opcodeRolesFor(Opcode opcode)
{
    std::vector<const OpcodeRole*> out;
    for (const OpcodeRole& r : kRoles) {
        if (r.opcode == opcode) {
            out.push_back(&r);
        }
    }
    return out;
}

const char* opcodeName(Opcode opcode)
{
    switch (opcode) {
        case Opcode::Pad:
            return "Pad";
        case Opcode::Pocket:
            return "Pocket";
        case Opcode::Fillet:
            return "Fillet";
        case Opcode::Chamfer:
            return "Chamfer";
        case Opcode::Draft:
            return "Draft";
        case Opcode::Thickness:
            return "Thickness";
        case Opcode::Revolution:
            return "Revolution";
        case Opcode::Groove:
            return "Groove";
        case Opcode::Hole:
            return "Hole";
        case Opcode::LinearPattern:
            return "LinearPattern";
        case Opcode::PolarPattern:
            return "PolarPattern";
        case Opcode::Mirrored:
            return "Mirrored";
        case Opcode::Loft:
            return "Loft";
        case Opcode::Pipe:
            return "Pipe";
        case Opcode::Helix:
            return "Helix";
        case Opcode::Boolean:
            return "Boolean";
        case Opcode::AdditiveBox:
            return "AdditiveBox";
        case Opcode::AdditiveCylinder:
            return "AdditiveCylinder";
        case Opcode::AdditiveSphere:
            return "AdditiveSphere";
        case Opcode::AdditiveCone:
            return "AdditiveCone";
        case Opcode::AdditiveTorus:
            return "AdditiveTorus";
        case Opcode::AdditivePrism:
            return "AdditivePrism";
        case Opcode::AdditiveWedge:
            return "AdditiveWedge";
        case Opcode::AdditiveEllipsoid:
            return "AdditiveEllipsoid";
        case Opcode::SubtractiveCylinder:
            return "SubtractiveCylinder";
        case Opcode::SubtractiveSphere:
            return "SubtractiveSphere";
        case Opcode::SubtractiveCone:
            return "SubtractiveCone";
        case Opcode::SubtractiveTorus:
            return "SubtractiveTorus";
        case Opcode::SubtractivePrism:
            return "SubtractivePrism";
        case Opcode::SubtractiveWedge:
            return "SubtractiveWedge";
        case Opcode::SubtractiveEllipsoid:
            return "SubtractiveEllipsoid";
        case Opcode::Scaled:
            return "Scaled";
        case Opcode::MultiTransform:
            return "MultiTransform";
        case Opcode::SubtractiveLoft:
            return "SubtractiveLoft";
        case Opcode::SubtractivePipe:
            return "SubtractivePipe";
        case Opcode::SubtractiveHelix:
            return "SubtractiveHelix";
        case Opcode::SubtractiveBox:
            return "SubtractiveBox";
    }
    return "Unknown";
}

App::SemanticId firstNamedFace(const std::vector<App::SemanticReference>& refs)
{
    // First-wins (not I13 unique). Prefer uniqueNamedFace for product consume.
    for (const App::SemanticReference& ref : refs) {
        if (ref.seed.valid()
            && (ref.seed.kind == App::SemanticKind::Face
                || ref.kind == App::SemanticKind::Face)) {
            return ref.seed;
        }
    }
    return {};
}

App::SemanticId uniqueNamedFace(const std::vector<App::SemanticReference>& refs)
{
    App::SemanticId found;
    int n = 0;
    for (const App::SemanticReference& ref : refs) {
        if (!ref.seed.valid()) {
            continue;
        }
        if (ref.seed.kind != App::SemanticKind::Face && ref.kind != App::SemanticKind::Face) {
            continue;
        }
        ++n;
        found = ref.seed;
    }
    return n == 1 ? found : App::SemanticId{};
}

bool appendUniqueSemanticSeed(std::vector<App::SemanticId>& seeds,
                              const App::SemanticId& seed)
{
    if (!seed.valid()
        || std::find(seeds.begin(), seeds.end(), seed) != seeds.end()) {
        return false;
    }
    seeds.push_back(seed);
    return true;
}

std::optional<App::SemanticBinding> uniqueFaceBindingOnFeature(
    const App::SemanticGraph* graph,
    const App::SemanticId& seed,
    App::ObjectId linkedFeature)
{
    // Keep the PartDesign Face convenience API on the shared App helper so
    // Part and PartDesign consume exactly the same I13 uniqueness policy.
    return App::uniqueBindingOnFeature(graph, seed, linkedFeature, "Face");
}

std::optional<App::SemanticBinding> uniqueResolvedFaceReference(
    const App::SemanticGraph* graph,
    const App::SemanticReference& reference,
    App::ObjectId linkedFeature)
{
    if (!graph || !graph->hasBindings() || linkedFeature == 0 || !reference.seed.valid()
        || reference.seed.kind != App::SemanticKind::Face
        || reference.kind != App::SemanticKind::Face) {
        return std::nullopt;
    }

    App::ReferenceRequirement requirement;
    requirement.expectedKind = App::SemanticKind::Face;
    requirement.acceptedCardinality = App::AcceptedCardinality::One;
    requirement.acceptedReducers = {reference.reducer};
    const App::ResolutionResult result =
        App::SemanticResolver::resolve(reference, *graph, &requirement);
    if (result.state != App::ResolutionState::Resolved || result.bindings.size() != 1) {
        return std::nullopt;
    }

    const App::SemanticBinding& binding = result.bindings.front();
    if (binding.stid.handle != reference.seed.handle
        || binding.stid.kind != App::SemanticKind::Face || binding.feature != linkedFeature
        || !isValidNamedIndex(binding.index, "Face")) {  // PD32-N1
        return std::nullopt;
    }
    return binding;
}


std::optional<App::SemanticBinding> uniqueEdgeBindingOnFeature(
    const App::SemanticGraph* graph,
    const App::SemanticId& seed,
    App::ObjectId linkedFeature
)
{
    // Keep the PartDesign Edge convenience API on the shared App helper so
    // Part and PartDesign consume exactly the same I13 uniqueness policy.
    return App::uniqueBindingOnFeature(graph, seed, linkedFeature, "Edge");
}

App::ReferenceRequirement requirementFor(OpcodeRoleId id)
{
    const OpcodeRole& role = opcodeRole(id);
    App::ReferenceRequirement req;
    req.expectedKind = role.outputKind;
    req.acceptedCardinality = role.acceptedCardinality;
    req.acceptedReducers = {role.defaultReducer};
    if (role.defaultReducer == App::CardinalityReducer::AcceptAll) {
        req.acceptedReducers.insert(App::CardinalityReducer::RequireOne);
    }
    return req;
}

App::SemanticReference referenceFor(const App::SemanticId& seed, OpcodeRoleId id)
{
    const OpcodeRole& role = opcodeRole(id);
    App::SemanticReference ref;
    ref.seed = seed;
    ref.kind = role.outputKind;
    ref.role = role.consumerRole;
    ref.filter.flags = role.filter;
    ref.reducer = role.defaultReducer;
    if (role.consumerRole != App::SemanticRole::None) {
        App::applyRoleDefaults(ref);
        // applyRoleDefaults may overwrite kind/reducer; keep table as authority
        // for opcode-specific roles, then re-apply §4.2 only for known consumer roles.
        ref.kind = (role.consumerRole == App::SemanticRole::DressUpEdge
                    || role.consumerRole == App::SemanticRole::ExternalEdge
                    || role.consumerRole == App::SemanticRole::PatternAxis)
            ? App::SemanticKind::Edge
            : role.outputKind;
        ref.reducer = role.defaultReducer;
        ref.filter.flags = role.filter;
    }
    return ref;
}

App::SemanticGraph* SemanticEmitter::graphFor(const void* featureOrDocument)
{
    // Document owns SemanticDocumentState (lifetime = document).
    // featureOrDocument is a Document* or a DocumentObject* bound as an alias.
    return App::SemanticDocumentState::graphFor(featureOrDocument);
}

bool SemanticEmitter::attached(const App::SemanticGraph* graph)
{
    return graph != nullptr;
}

bool SemanticEmitter::needsSemanticRepublish(const App::SemanticGraph* graph,
                                               App::ObjectId feature)
{
    return graph && feature != 0 && !graph->hasFeatureData(feature);
}

void SemanticEmitter::appendInverseProfileFaceIndices(
    const App::DocumentObject* profileFeature,
    const Part::TopoShape& profileShape,
    std::vector<App::ElementIndex>& namedFaceIndices)
{
    if (!profileFeature || profileShape.isNull()) {
        return;
    }
    App::Document* doc = profileFeature->getDocument();
    if (!doc) {
        return;
    }
    std::set<int> seen;
    for (const App::ElementIndex& existing : namedFaceIndices) {
        if (isValidNamedIndex(existing, "Face")) {  // PD32-N2
            seen.insert(existing.index);
        }
    }
    for (App::DocumentObject* consumer : doc->getObjects()) {
        if (!consumer || consumer == profileFeature) {
            continue;
        }
        std::vector<App::Property*> props;
        consumer->getPropertyList(props);
        for (App::Property* prop : props) {
            auto* link = freecad_cast<App::PropertyLinkSub*>(prop);
            if (!link || std::strcmp(prop->getName(), "Profile") != 0) {
                continue;
            }
            if (link->getValue() != profileFeature) {
                continue;
            }
            for (const App::SemanticReference& ref : link->getSemanticRefs()) {
                if (!isValidNamedIndex(ref.fallback, "Face")) {  // PD32-N3
                    continue;
                }
                const int faceIdx = ref.fallback.index;
                if (seen.count(faceIdx) != 0) {
                    continue;
                }
                if (profileShape.getSubShape(TopAbs_FACE, faceIdx, true).IsNull()) {
                    continue;
                }
                App::ElementIndex idx;
                idx.type = "Face";
                idx.index = faceIdx;
                namedFaceIndices.push_back(idx);
                seen.insert(faceIdx);
            }
        }
    }
}

namespace
{
std::string g_lastAfterExecuteNote;

bool alreadyGeneratedKind(const App::SemanticGraph& graph,
                          App::SemanticHandle seed,
                          App::SemanticKind kind)
{
    for (const App::SemanticId& id : SemanticEmitter::generatedFrom(graph, seed)) {
        if (id.kind == kind) {
            return true;
        }
    }
    return false;
}

bool alreadyGeneratedFace(const App::SemanticGraph& graph, App::SemanticHandle seed)
{
    return alreadyGeneratedKind(graph, seed, App::SemanticKind::Face);
}

bool isValidNamedIndex(const App::ElementIndex& index, const char* expectedType);

void emitNamedEdgesFromRequest(App::SemanticGraph* graph,
                               const char* op,
                               App::ObjectId feature,
                               App::EvalSerial eval,
                               const AfterExecuteRequest& request,
                               const std::vector<App::SemanticId>& edgeSeeds,
                               std::size_t& nEdges,
                               std::size_t& nSkip,
                               bool protectOutputOwnership = false)
{
    std::size_t ni = 0;
    for (const App::SemanticId& seed : edgeSeeds) {
        App::ElementIndex idx;
        if (ni < request.namedEdgeIndices.size()) {
            idx = request.namedEdgeIndices[ni++];
        }
        if (App::shouldRefuseNamedEmitSlot(graph, feature, idx, "Edge", protectOutputOwnership)) {
            ++nSkip;
            continue;
        }
        SemanticEmitter::emitGeneratedFrom(graph,
                                           {seed},
                                           App::SemanticKind::Edge,
                                           op,
                                           feature,
                                           eval,
                                           App::SemanticRole::None,
                                           idx);
        ++nEdges;
    }
}

std::vector<App::SemanticId> edgeZipSeeds(const App::SemanticGraph& graph,
                                          const AfterExecuteRequest& request)
{
    std::vector<App::SemanticId> vertices;
    for (const App::SemanticId& v : request.vertexSeeds) {
        if (!v.valid() || graph.hasDeletedEvent(v.handle)) {
            continue;
        }
        if (alreadyGeneratedKind(graph, v.handle, App::SemanticKind::Edge)) {
            continue;
        }
        vertices.push_back(v);
    }
    if (!vertices.empty()) {
        return vertices;
    }
    std::vector<App::SemanticId> curves;
    for (const App::SemanticId& c : request.curveSeeds) {
        if (!c.valid() || graph.hasDeletedEvent(c.handle)) {
            continue;
        }
        if (alreadyGeneratedKind(graph, c.handle, App::SemanticKind::Edge)) {
            continue;
        }
        curves.push_back(c);
    }
    return curves;
}

bool isValidNamedIndex(const App::ElementIndex& index)
{
    return (index.type == "Face" || index.type == "Edge") && index.index > 0;
}

bool isValidNamedIndex(const App::ElementIndex& index, const char* expectedType)
{
    return isValidNamedIndex(index) && (!expectedType || index.type == expectedType);
}

std::size_t countValidNamedIndices(const std::vector<App::ElementIndex>& indices,
                                   const char* type)
{
    std::size_t count = 0;
    for (const App::ElementIndex& index : indices) {
        if (isValidNamedIndex(index) && (!type || index.type == type)) {
            ++count;
        }
    }
    return count;
}

int nextIndexAfter(const std::vector<App::SemanticId>& ids, int first)
{
    return first + static_cast<int>(ids.size());
}

}  // namespace

const std::string& SemanticEmitter::lastAfterExecuteNote()
{
    return g_lastAfterExecuteNote;
}

void SemanticEmitter::stampElementMap(App::PropertyComplexGeoData& map,
                                      const App::SemanticGraph* graph,
                                      App::ObjectId feature)
{
    const Data::ComplexGeoData* geoConst = map.getComplexData();
    if (!graph || !geoConst || !geoConst->hasElementMap()) {
        return;
    }
    Data::ComplexGeoData* geo = const_cast<Data::ComplexGeoData*>(geoConst);
    // MappedName::find returns int (-1 miss). Never compare to std::string::npos
    // (Sweep #3 E1). Travelling docs that say the opposite are wrong — see
    // QUALITY-SWEEP #14 EM14-D1.
    const char* tokenMark = App::SemanticId::mappedTokenPrefix();
    const auto bindings = graph->allBindings();
    for (const App::SemanticBinding& row : bindings) {
        if (row.feature != feature || !Part::isNamedIndex(row.index)  // PD32-N4
            || !row.stid.valid()) {
            continue;
        }
        // I8 / I13: do not stamp a Face index with an Edge seed (or vice
        // versa). Cross-kind ElementMap tokens would teach consumers the
        // wrong durable kind for a FaceN/EdgeN cache slot.
        if (!App::SemanticId::kindMatchesElementType(row.stid.kind, row.index.type)) {
            continue;
        }
        // Uniqueness is feature+index across all live Binding rows (no max-eval
        // filter) — intentional lockstep with uniqueBindingOnFeature (AG21-E1).
        // Ambiguous / multi-eval → skip stamp (fail-closed). Aligning either
        // path with uniquePublishedBinding max-eval is EM14-U1 / AG13-E1
        // watchlist — do not change without TESTS re-gate (QUALITY-SWEEP #22).
        std::size_t matches = 0;
        for (const App::SemanticBinding& candidate : bindings) {
            if (candidate.feature == feature && candidate.index == row.index) {
                ++matches;
            }
        }
        if (matches != 1) {
            continue;
        }
        const Data::IndexedName index(row.index.type.c_str(), row.index.index);
        auto names = geo->getElementMappedNames(index, true);
        // Primary mapped name only (front). Prefer MappedName::contains
        // over find()/npos — int/-1 contract, harder to re-break (EM30-C1).
        if (names.empty() || names.front().first.contains(tokenMark)) {
            continue;
        }
        Data::MappedName stamped = names.front().first;
        const std::string token = row.stid.toMappedToken();
        stamped += token.c_str();
        auto sidRefs = names.front().second;
        geo->setElementName(index, stamped, 0, &sidRefs, true);
    }
}


void SemanticEmitter::publishSubtractiveCutHistory(App::DocumentObject* feature,
                                                   void* occBooleanOp,
                                                   const void* toolShapeOcc,
                                                   const void* baseShapeOcc,
                                                   App::DocumentObject* baseObj,
                                                   Opcode opcode,
                                                   const char* diagName)
{
    // Shared with-base Cut publisher for Batch A subtractive twins and
    // SubtractiveLoft/Pipe. Dedicated opcode; never reuse Boolean=15.
    // Live Cut fromMaker + uniqueOneImageGenerated + applyHistory keeps
    // Fillet.Base allocatedBy on this subtractive feature.
    auto* mkCut = static_cast<BRepAlgoAPI_BooleanOperation*>(occBooleanOp);
    if (!feature || !mkCut || !mkCut->IsDone() || !toolShapeOcc || !baseShapeOcc) {
        return;
    }
    auto* addSub = freecad_cast<FeatureAddSub*>(feature);
    if (!addSub || addSub->getAddSubType() != FeatureAddSub::Type::Subtractive) {
        return;
    }
    if (!addSub->getBaseObject(/* silent = */ true)) {
        return;
    }
    const Part::TopoShape published = addSub->Shape.getShape();
    if (published.isNull()) {
        return;
    }
    App::SemanticGraph* graph = SemanticEmitter::graphFor(feature);
    if (!graph && feature->getDocument()) {
        graph = &feature->getDocument()->semanticGraph();
    }
    if (!graph) {
        return;
    }

    const App::ObjectId selfId = static_cast<App::ObjectId>(feature->getID());
    App::EvalSerial eval = 0;
    if (App::Document* doc = feature->getDocument()) {
        eval = doc->semanticState().currentEval();
    }

    const TopoDS_Shape& toolShape = *static_cast<const TopoDS_Shape*>(toolShapeOcc);
    const TopoDS_Shape& baseShape = *static_cast<const TopoDS_Shape*>(baseShapeOcc);

    std::deque<TopoDS_Shape> held;
    std::vector<std::pair<App::SemanticId, const void*>> inputs;
    std::unordered_set<App::SemanticHandle> seenSourceSeeds;
    const Part::TopoShape baseTopo(baseShape);
    Part::collectUniqueSourceSeeds(graph, baseObj, baseTopo, held, inputs, seenSourceSeeds);

    auto considerTool = [&](const TopoDS_Shape& sub) {
        if (sub.IsNull() || (sub.ShapeType() != TopAbs_FACE && sub.ShapeType() != TopAbs_EDGE)) {
            return;
        }
        for (const auto& h : held) {
            if (h.IsSame(sub) || h.IsPartner(sub)) {
                return;
            }
        }
        App::SemanticId seed;
        const App::ElementIndex named = Part::indexOnPublished(published, sub);
        if (Part::isNamedIndex(named)) {
            if (App::shouldRefuseBoundAt(graph, selfId, named)) {
                seed = App::uniqueIdentityAtIndex(graph, selfId, named);
                if (!seed.valid()) {
                    return;
                }
            }
        }
        if (!seed.valid()) {
            const App::SemanticKind kind =
                sub.ShapeType() == TopAbs_FACE ? App::SemanticKind::Face
                                               : App::SemanticKind::Edge;
            seed = graph->recordGenerated(
                kind, opcodeName(opcode), selfId, eval, App::SemanticRole::None);
        }
        if (!seed.valid()) {
            return;
        }
        if (!seenSourceSeeds.insert(seed.handle).second) {
            return;
        }
        held.push_back(sub);
        inputs.push_back({seed, &held.back()});
    };
    if (!toolShape.IsNull()) {
        for (TopExp_Explorer ex(toolShape, TopAbs_FACE); ex.More(); ex.Next()) {
            considerTool(ex.Current());
        }
        for (TopExp_Explorer ex(toolShape, TopAbs_EDGE); ex.More(); ex.Next()) {
            considerTool(ex.Current());
        }
    }

    AfterExecuteRequest req;
    req.allowSequentialFaceN = false;
    if (inputs.empty()) {
        SemanticEmitter::afterExecute(graph, opcode, selfId, eval, req);
        SemanticEmitter::appendAfterExecuteNote(
            std::string("skip emit: no unique ") + opcodeName(opcode) + " Cut source seeds");
        Base::Console().message(
            "%s %s\n", diagName ? diagName : "subtractiveDiag",
            SemanticEmitter::lastAfterExecuteNote().c_str());
        return;
    }

    auto indexOf = [&published](const void* occ) -> App::ElementIndex {
        App::ElementIndex idx;
        if (!occ) {
            return idx;
        }
        return Part::indexOnPublished(published, *static_cast<const TopoDS_Shape*>(occ));
    };

    const Part::HistoryTable raw =
        Part::SemanticHistoryAdapter::fromMaker(mkCut, inputs, indexOf);
    const Part::HistoryTable unique =
        Part::SemanticHistoryAdapter::uniqueOneImageGenerated(raw);

    Part::HistoryTable toApply;
    std::vector<App::SemanticId> seeds;
    toApply.reserve(unique.size());
    seeds.reserve(unique.size());
    // PD5-R1: remember named indices only after shouldRefuseGeneratedMint (match
    // Fillet/Boolean). Refused slots must not linger in namedFaceIndices/Edges.
    auto rememberNamedIndex = [&](const App::ElementIndex& index) {
        if (index.type == "Face") {
            for (const App::ElementIndex& existing : req.namedFaceIndices) {
                if (existing == index) {
                    return;
                }
            }
            req.namedFaceIndices.push_back(index);
        }
        else if (index.type == "Edge") {
            for (const App::ElementIndex& existing : req.namedEdgeIndices) {
                if (existing == index) {
                    return;
                }
            }
            req.namedEdgeIndices.push_back(index);
        }
    };
    for (const Part::HistoryRecord& rec : unique) {
        if (!Part::isNamedIndex(rec.toIndex) || !rec.fromSeed.valid()) {
            continue;
        }
        if (App::shouldRefuseGeneratedMint(graph, rec.fromSeed, selfId, rec.toIndex)) {
            continue;
        }
        rememberNamedIndex(rec.toIndex);
        toApply.push_back(rec);
        seeds.push_back(rec.fromSeed);
    }

    if (!toApply.empty()) {
        Part::SemanticHistoryAdapter::applyHistory(
            graph, selfId, eval, opcodeName(opcode), seeds, toApply);
    }
    SemanticEmitter::afterExecute(graph, opcode, selfId, eval, req);
    SemanticEmitter::appendAfterExecuteNote(
        Part::SemanticHistoryAdapter::lastApplyNote());
    Base::Console().message(
        "%s %s\n", diagName ? diagName : "subtractiveDiag",
        SemanticEmitter::lastAfterExecuteNote().c_str());
}

void SemanticEmitter::appendAfterExecuteNote(const std::string& suffix)
{
    if (suffix.empty()) {
        return;
    }
    if (!g_lastAfterExecuteNote.empty()) {
        g_lastAfterExecuteNote += "; ";
    }
    g_lastAfterExecuteNote += suffix;
}

AfterExecuteRequest SemanticEmitter::collectSketchProfileSeeds(const App::SemanticGraph* graph,
                                                               App::ObjectId sketch)
{
    AfterExecuteRequest req;
    if (!graph || sketch == 0) {
        return req;
    }
    // S4-S1: Split children are live profile edges after sketch split/trim.
    // Scan Generated and Split so Pad still collects seeds when replaceGeometries
    // RecordingSplit wired splitEntity (children are not re-minted Generated).
    for (const App::Event& ev : graph->events()) {
        if (ev.feature != sketch) {
            continue;
        }
        if (ev.kind != App::EventKind::Generated && ev.kind != App::EventKind::Split) {
            continue;
        }
        for (const App::EventOutput& o : graph->outputs()) {
            if (o.event != ev.id) {
                continue;
            }
            if (o.semantic.kind == App::SemanticKind::Edge) {
                appendUniqueSemanticId(req.curveSeeds, o.semantic);
            }
            else if (o.semantic.kind == App::SemanticKind::Vertex) {
                appendUniqueSemanticId(req.vertexSeeds, o.semantic);
            }
            else if (o.semantic.kind == App::SemanticKind::Region) {
                appendUniqueSemanticId(req.regionSeeds, o.semantic);
            }
        }
    }
    return req;
}

AfterExecuteRequest SemanticEmitter::collectProfileSeeds(
    const App::SemanticGraph* graph,
    App::ObjectId profile,
    const std::vector<App::SemanticReference>& profileRefs)
{
    AfterExecuteRequest req = collectSketchProfileSeeds(graph, profile);
    if (!req.curveSeeds.empty() || !req.regionSeeds.empty() || !req.vertexSeeds.empty()) {
        return req;
    }

    // A non-Sketch Profile has no Sketch.gN Edge/Region events. Its LinkSub
    // normally carries the authoritative Face seed, so retain that provenance
    // instead of silently taking the half-map path. A restored LinkSub may
    // have only its FaceN cache, though: D2/I13 allow promotion here only when
    // that cache identifies exactly one published Face Binding on this
    // profile feature. Never turn an unverified FaceN into a seed.
    for (const App::SemanticReference& ref : profileRefs) {
        App::SemanticId seed;
        if (ref.seed.valid()
            && (ref.seed.kind == App::SemanticKind::Face
                || ref.kind == App::SemanticKind::Face)) {
            seed = ref.seed;
        }
        else if (graph && isValidNamedIndex(ref.fallback, "Face")) {  // PD32-N5
            const std::optional<App::SemanticBinding> promoted =
                App::uniquePublishedBinding(*graph, ref.fallback, profile);
            if (promoted && isValidNamedIndex(promoted->index, "Face")
                && promoted->kind == App::SemanticKind::Face
                && promoted->stid.kind == App::SemanticKind::Face) {
                seed = promoted->stid;
            }
        }
        if (!seed.valid()) {
            continue;
        }
        appendUniqueSemanticId(req.curveSeeds, seed);
        appendUniqueSemanticId(req.regionSeeds, seed);
    }
    return req;
}

void SemanticEmitter::afterExecute(App::SemanticGraph* graph,
                                   Opcode opcode,
                                   App::ObjectId feature,
                                   App::EvalSerial eval,
                                   const AfterExecuteRequest& request)
{
    g_lastAfterExecuteNote.clear();
    if (!graph) {
        g_lastAfterExecuteNote = "skip emit: no graph";
        return;
    }
    if (feature == 0) {
        g_lastAfterExecuteNote = "skip emit: ObjectId 0 (caller must pass getID())";
        // Still allow emit if seeds are named — ObjectId 0 is a diagnostic, not
        // a hard block, but we record it. Continue.
    }
    if (eval == 0) {
        if (!g_lastAfterExecuteNote.empty()) {
            g_lastAfterExecuteNote += "; ";
        }
        g_lastAfterExecuteNote += "EvalSerial 0 (caller should pass currentEval())";
    }

    int faceIdx = request.firstFaceIndex > 0 ? request.firstFaceIndex : 1;

    if (opcode == Opcode::Pad) {
        std::vector<App::SemanticId> curves;
        for (const App::SemanticId& c : request.curveSeeds) {
            if (!c.valid() || graph->hasDeletedEvent(c.handle)) {
                continue;
            }
            if (alreadyGeneratedFace(*graph, c.handle)) {
                continue;
            }
            curves.push_back(c);
        }
        std::vector<App::SemanticId> regions;
        for (const App::SemanticId& r : request.regionSeeds) {
            if (!r.valid() || graph->hasDeletedEvent(r.handle)) {
                continue;
            }
            if (alreadyGeneratedFace(*graph, r.handle)) {
                continue;
            }
            regions.push_back(r);
        }
        const std::vector<App::SemanticId> edgeSeeds = edgeZipSeeds(*graph, request);
        const bool hasEdgeWork = !request.namedEdgeIndices.empty() && !edgeSeeds.empty();
        if (curves.empty() && regions.empty() && !hasEdgeWork) {
            if (request.curveSeeds.empty() && request.regionSeeds.empty()
                && request.vertexSeeds.empty()) {
                g_lastAfterExecuteNote = "skip emit: no named pad curve/region seeds (half-map)";
            }
            else {
                g_lastAfterExecuteNote = "pad: seeds already emitted this lineage";
            }
            return;
        }
        const bool useNamed =
            !request.namedFaceIndices.empty() || !request.namedEdgeIndices.empty();
        const bool useSeq = request.allowSequentialFaceN && !useNamed;
        if (!useNamed && !useSeq) {
            g_lastAfterExecuteNote = "skip emit: no named pad history (half-map)";
            return;
        }
        if (useNamed) {
            // 1:1 with namedFaceIndices / namedEdgeIndices. Unnamed slots skip (I13).
            std::size_t ni = 0;
            std::size_t nSides = 0;
            std::size_t nCaps = 0;
            std::size_t nSkip = 0;
            std::size_t nEdges = 0;
            auto takeNamed = [&]() -> App::ElementIndex {
                if (ni < request.namedFaceIndices.size()) {
                    return request.namedFaceIndices[ni++];
                }
                return {};
            };
            for (const App::SemanticId& curve : curves) {
                const App::ElementIndex idx = takeNamed();
                if (App::shouldRefuseNamedEmitSlot(graph, feature, idx, "Face")) {
                    ++nSkip;
                    continue;
                }
                emitGeneratedFrom(graph,
                                  {curve},
                                  App::SemanticKind::Face,
                                  "Pad",
                                  feature,
                                  eval,
                                  App::SemanticRole::None,
                                  idx);
                ++nSides;
            }
            for (const App::SemanticId& region : regions) {
                for (int cap = 0; cap < 2; ++cap) {
                    const App::ElementIndex idx = takeNamed();
                    if (App::shouldRefuseNamedEmitSlot(graph, feature, idx, "Face")) {
                        ++nSkip;
                        continue;
                    }
                    emitGeneratedFrom(graph,
                                      {region},
                                      App::SemanticKind::Face,
                                      "Pad",
                                      feature,
                                      eval,
                                      App::SemanticRole::None,
                                      idx);
                    ++nCaps;
                }
            }
            if (hasEdgeWork) {
                emitNamedEdgesFromRequest(
                    graph, "Pad", feature, eval, request, edgeSeeds, nEdges, nSkip, true);
            }
            g_lastAfterExecuteNote = "emitted pad sides=" + std::to_string(nSides)
                + " caps=" + std::to_string(nCaps) + " edges=" + std::to_string(nEdges)
                + " unnamed=" + std::to_string(nSkip);
            return;
        }
        const auto sides = emitPadSides(graph, curves, feature, eval, faceIdx);
        faceIdx = nextIndexAfter(sides, faceIdx);
        const auto caps = emitPadCaps(graph, regions, feature, eval, faceIdx);
        g_lastAfterExecuteNote = "emitted pad sides=" + std::to_string(sides.size())
            + " caps=" + std::to_string(caps.size());
        return;
    }

    if (opcode == Opcode::Pocket) {
        std::vector<App::SemanticId> curves;
        for (const App::SemanticId& c : request.curveSeeds) {
            if (!c.valid() || graph->hasDeletedEvent(c.handle)) {
                continue;
            }
            if (alreadyGeneratedFace(*graph, c.handle)) {
                continue;
            }
            curves.push_back(c);
        }
        std::vector<App::SemanticId> regions;
        for (const App::SemanticId& r : request.regionSeeds) {
            if (!r.valid() || graph->hasDeletedEvent(r.handle)) {
                continue;
            }
            if (alreadyGeneratedFace(*graph, r.handle)) {
                continue;
            }
            regions.push_back(r);
        }

        std::size_t nSides = 0;
        std::size_t nCaps = 0;
        std::size_t nEdges = 0;
        std::size_t nEdgeSkip = 0;
        const bool useNamedFaces = !request.namedFaceIndices.empty();
        const bool bindSequential = request.allowSequentialFaceN && !useNamedFaces;
        if (!curves.empty() || !regions.empty()) {
            if (useNamedFaces) {
                std::size_t ni = 0;
                auto takeNamed = [&]() -> App::ElementIndex {
                    if (ni < request.namedFaceIndices.size()) {
                        return request.namedFaceIndices[ni++];
                    }
                    return {};
                };
                for (const App::SemanticId& curve : curves) {
                    const App::ElementIndex idx = takeNamed();
                    if (App::shouldRefuseNamedEmitSlot(graph, feature, idx, "Face")) {
                        continue;
                    }
                    emitGeneratedFrom(graph,
                                      {curve},
                                      App::SemanticKind::Face,
                                      "Pocket",
                                      feature,
                                      eval,
                                      App::SemanticRole::None,
                                      idx);
                    ++nSides;
                }
                for (const App::SemanticId& region : regions) {
                    for (int cap = 0; cap < 2; ++cap) {
                        const App::ElementIndex idx = takeNamed();
                        if (App::shouldRefuseNamedEmitSlot(graph, feature, idx, "Face")) {
                            continue;
                        }
                        emitGeneratedFrom(graph,
                                          {region},
                                          App::SemanticKind::Face,
                                          "Pocket",
                                          feature,
                                          eval,
                                          App::SemanticRole::None,
                                          idx);
                        ++nCaps;
                    }
                }
            }
            else if (bindSequential) {
                int idx = faceIdx;
                for (const App::SemanticId& curve : curves) {
                    emitGeneratedFrom(graph,
                                      {curve},
                                      App::SemanticKind::Face,
                                      "Pocket",
                                      feature,
                                      eval,
                                      App::SemanticRole::None,
                                      faceIndex(idx++));
                    ++nSides;
                }
                for (const App::SemanticId& region : regions) {
                    emitGeneratedFrom(graph,
                                      {region},
                                      App::SemanticKind::Face,
                                      "Pocket",
                                      feature,
                                      eval,
                                      App::SemanticRole::None,
                                      faceIndex(idx++));
                    emitGeneratedFrom(graph,
                                      {region},
                                      App::SemanticKind::Face,
                                      "Pocket",
                                      feature,
                                      eval,
                                      App::SemanticRole::None,
                                      faceIndex(idx++));
                    nCaps += 2;
                }
                faceIdx = idx;
            }
        }

        const std::vector<App::SemanticId> edgeSeeds = edgeZipSeeds(*graph, request);
        if (!request.namedEdgeIndices.empty() && !edgeSeeds.empty()) {
            emitNamedEdgesFromRequest(
                graph, "Pocket", feature, eval, request, edgeSeeds, nEdges, nEdgeSkip, true);
        }

        std::string splitNote;
        if (request.pocketTarget.valid()) {
            if (request.pocketMode == AfterExecuteRequest::PocketMode::ThroughCut) {
                if (bindSequential) {
                    emitPocketSplit(graph,
                                    request.pocketTarget,
                                    request.pocketSplitCount == 0 ? 2 : request.pocketSplitCount,
                                    feature,
                                    eval,
                                    faceIdx);
                    splitNote = "pocket ThroughCut Split";
                }
                else {
                    const std::size_t n =
                        request.pocketSplitCount == 0 ? 2 : request.pocketSplitCount;
                    graph->unbind(request.pocketTarget.handle);
                    graph->recordSplit(request.pocketTarget, n, "Pocket", feature, eval,
                                       App::SemanticRole::None);
                    splitNote = "pocket ThroughCut Split (no FaceN bind)";
                }
            }
            else if (request.pocketMode == AfterExecuteRequest::PocketMode::Hole) {
                if (bindSequential) {
                    App::SemanticId wallRegion =
                        regions.empty() ? (request.regionSeeds.empty() ? App::SemanticId{}
                                                                       : request.regionSeeds.front())
                                        : regions.front();
                    App::ElementIndex remnantIdx;
                    remnantIdx.type = "Face";
                    remnantIdx.index = faceIdx;
                    emitPocketHole(graph,
                                   request.pocketTarget,
                                   wallRegion,
                                   request.pocketWallCount,
                                   feature,
                                   eval,
                                   remnantIdx,
                                   faceIdx + 1);
                    splitNote = "pocket Length S3 hole";
                }
                else {
                    graph->recordModified(request.pocketTarget, "Pocket", feature, eval,
                                          App::SemanticRole::None);
                    splitNote = "pocket Length S3 hole (no FaceN bind)";
                }
            }
            else {
                splitNote = "Split vs S3 decision point: Type not Length/ThroughAll; "
                            "target named but mode Unknown — remnant not mutated";
            }
        }
        else {
            splitNote = "Split vs S3 decision point: no named remnant/target cap "
                        "(would be a half-map). Emitted Generated sides/caps only.";
        }

        if (nSides == 0 && nCaps == 0 && nEdges == 0 && !request.pocketTarget.valid()
            && request.curveSeeds.empty() && request.regionSeeds.empty()
            && request.vertexSeeds.empty()) {
            g_lastAfterExecuteNote = "skip emit: no named pocket seeds (half-map)";
            return;
        }
        g_lastAfterExecuteNote = "emitted pocket sides=" + std::to_string(nSides)
            + " caps=" + std::to_string(nCaps) + " edges=" + std::to_string(nEdges)
            + " unnamed=" + std::to_string(nEdgeSkip) + "; " + splitNote;
        return;
    }

    if (opcode == Opcode::Revolution || opcode == Opcode::Groove
        || opcode == Opcode::Loft || opcode == Opcode::Pipe || opcode == Opcode::Helix
        || opcode == Opcode::SubtractiveLoft || opcode == Opcode::SubtractivePipe
        || opcode == Opcode::SubtractiveHelix) {
        const char* opName = opcodeName(opcode);
        const bool protectOutputOwnership =
            opcode == Opcode::Revolution || opcode == Opcode::Groove
            || opcode == Opcode::Loft || opcode == Opcode::Pipe || opcode == Opcode::Helix
            || opcode == Opcode::SubtractiveLoft || opcode == Opcode::SubtractivePipe
            || opcode == Opcode::SubtractiveHelix;
        std::vector<App::SemanticId> curves;
        for (const App::SemanticId& c : request.curveSeeds) {
            if (!c.valid() || graph->hasDeletedEvent(c.handle)) {
                continue;
            }
            if (alreadyGeneratedFace(*graph, c.handle)) {
                continue;
            }
            curves.push_back(c);
        }
        std::vector<App::SemanticId> regions;
        for (const App::SemanticId& r : request.regionSeeds) {
            if (!r.valid() || graph->hasDeletedEvent(r.handle)) {
                continue;
            }
            if (alreadyGeneratedFace(*graph, r.handle)) {
                continue;
            }
            regions.push_back(r);
        }
        const std::vector<App::SemanticId> edgeSeeds = edgeZipSeeds(*graph, request);
        const bool hasEdgeWork = !request.namedEdgeIndices.empty() && !edgeSeeds.empty();
        if (curves.empty() && regions.empty() && !hasEdgeWork) {
            g_lastAfterExecuteNote = std::string("skip emit: no named ") + opName
                + " curve/region seeds (half-map)";
            return;
        }
        const bool useNamed =
            !request.namedFaceIndices.empty() || !request.namedEdgeIndices.empty();
        if (!useNamed) {
            // UpTo* and discarded makers: unnamed, no sequential FaceN (I13).
            g_lastAfterExecuteNote = std::string("skip emit: no named ") + opName
                + " history (half-map)";
            return;
        }
        std::size_t ni = 0;
        std::size_t nSides = 0;
        std::size_t nCaps = 0;
        std::size_t nSkip = 0;
        std::size_t nEdges = 0;
        auto takeNamed = [&]() -> App::ElementIndex {
            if (ni < request.namedFaceIndices.size()) {
                return request.namedFaceIndices[ni++];
            }
            return {};
        };
        for (const App::SemanticId& curve : curves) {
            const App::ElementIndex idx = takeNamed();
            if (App::shouldRefuseNamedEmitSlot(graph,
                                          feature,
                                          idx,
                                          "Face",
                                          protectOutputOwnership)) {
                ++nSkip;
                continue;
            }
            emitGeneratedFrom(graph,
                              {curve},
                              App::SemanticKind::Face,
                              opName,
                              feature,
                              eval,
                              App::SemanticRole::None,
                              idx);
            ++nSides;
        }
        for (const App::SemanticId& region : regions) {
            for (int cap = 0; cap < 2; ++cap) {
                const App::ElementIndex idx = takeNamed();
                if (App::shouldRefuseNamedEmitSlot(graph,
                                              feature,
                                              idx,
                                              "Face",
                                              protectOutputOwnership)) {
                    ++nSkip;
                    continue;
                }
                emitGeneratedFrom(graph,
                                  {region},
                                  App::SemanticKind::Face,
                                  opName,
                                  feature,
                                  eval,
                                  App::SemanticRole::None,
                                  idx);
                ++nCaps;
            }
        }
        if (hasEdgeWork) {
            emitNamedEdgesFromRequest(graph,
                                       opName,
                                       feature,
                                       eval,
                                       request,
                                       edgeSeeds,
                                       nEdges,
                                       nSkip,
                                       protectOutputOwnership);
        }
        g_lastAfterExecuteNote = std::string("emitted ") + opName
            + " sides=" + std::to_string(nSides)
            + " caps=" + std::to_string(nCaps) + " edges=" + std::to_string(nEdges)
            + " unnamed=" + std::to_string(nSkip);
        return;
    }

    if (opcode == Opcode::Fillet) {
        if (request.filletEdges.empty()) {
            std::string seedlessNote;
            if (emitSeedlessDressUpFaces(
                    graph, "Fillet", feature, eval, request.namedFaceIndices, seedlessNote)) {
                g_lastAfterExecuteNote = seedlessNote;
                return;
            }
            g_lastAfterExecuteNote = "skip emit: no named FilletEdge seeds (half-map)";
            return;
        }
        int nOk = 0;
        int nMissing = 0;
        int nUnnamed = 0;
        int idx = faceIdx;
        for (std::size_t i = 0; i < request.filletEdges.size(); ++i) {
            const App::SemanticId& edge = request.filletEdges[i];
            if (!edge.valid()) {
                ++nMissing;
                continue;
            }
            App::ElementIndex bindIdx;
            if (i < request.namedFaceIndices.size()
                && isValidNamedIndex(request.namedFaceIndices[i], "Face")) {
                bindIdx = request.namedFaceIndices[i];
            }
            else if (!request.allowSequentialFaceN) {
                bindIdx = {};
            }
            else {
                bindIdx = faceIndex(idx++);
            }
            const App::ResolutionResult r =
                emitFillet(graph, edge, request.filletAdjacentFaces, feature, eval, bindIdx);
            if (r.state == App::ResolutionState::Missing
                || r.state == App::ResolutionState::Incompatible) {
                ++nMissing;
                continue;
            }
            if (r.state == App::ResolutionState::Ambiguous) {
                g_lastAfterExecuteNote = "FilletEdge Ambiguous; not coerced to a neighbour";
                return;
            }
            if (!Part::isNamedIndex(bindIdx)) {  // PD23-N1 / PD15-style DRY
                ++nUnnamed;
                continue;
            }
            ++nOk;
        }
        if (nOk == 0) {
            if (nUnnamed > 0 && nMissing == 0) {
                g_lastAfterExecuteNote =
                    "skip emit: no named fillet history (half-map)";
            }
            else {
                g_lastAfterExecuteNote =
                    "FilletEdge Missing; no neighbour invented (I2/I10)";
            }
            return;
        }
        g_lastAfterExecuteNote = "emitted fillet faces=" + std::to_string(nOk)
            + " missing=" + std::to_string(nMissing);
        return;
    }

    if (opcode == Opcode::Chamfer) {
        // Chamfer reuses request.filletEdges / filletAdjacentFaces (dress-up seeds).
        if (request.filletEdges.empty()) {
            std::string seedlessNote;
            if (emitSeedlessDressUpFaces(
                    graph, "Chamfer", feature, eval, request.namedFaceIndices, seedlessNote)) {
                g_lastAfterExecuteNote = seedlessNote;
                return;
            }
            g_lastAfterExecuteNote = "skip emit: no named ChamferEdge seeds (half-map)";
            return;
        }
        int nOk = 0;
        int nMissing = 0;
        int nUnnamed = 0;
        int idx = faceIdx;
        for (std::size_t i = 0; i < request.filletEdges.size(); ++i) {
            const App::SemanticId& edge = request.filletEdges[i];
            if (!edge.valid()) {
                ++nMissing;
                continue;
            }
            App::ElementIndex bindIdx;
            if (i < request.namedFaceIndices.size()
                && isValidNamedIndex(request.namedFaceIndices[i], "Face")) {
                bindIdx = request.namedFaceIndices[i];
            }
            else if (!request.allowSequentialFaceN) {
                bindIdx = {};
            }
            else {
                bindIdx = faceIndex(idx++);
            }
            const App::ResolutionResult r =
                emitChamfer(graph, edge, request.filletAdjacentFaces, feature, eval, bindIdx);
            if (r.state == App::ResolutionState::Missing
                || r.state == App::ResolutionState::Incompatible) {
                ++nMissing;
                continue;
            }
            if (r.state == App::ResolutionState::Ambiguous) {
                g_lastAfterExecuteNote = "ChamferEdge Ambiguous; not coerced to a neighbour";
                return;
            }
            if (!Part::isNamedIndex(bindIdx)) {  // PD23-N1 / PD15-style DRY
                ++nUnnamed;
                continue;
            }
            ++nOk;
        }
        if (nOk == 0) {
            if (nUnnamed > 0 && nMissing == 0) {
                g_lastAfterExecuteNote =
                    "skip emit: no named chamfer history (half-map)";
            }
            else {
                g_lastAfterExecuteNote =
                    "ChamferEdge Missing; no neighbour invented (I2/I10)";
            }
            return;
        }
        g_lastAfterExecuteNote = "emitted chamfer faces=" + std::to_string(nOk)
            + " missing=" + std::to_string(nMissing);
        return;
    }

    if (opcode == Opcode::Draft) {
        // Draft uses Face seeds (Draft.Base), not filletEdges.
        if (request.draftFaces.empty()) {
            g_lastAfterExecuteNote = "skip emit: no named DraftFace seeds (half-map)";
            return;
        }
        int nOk = 0;
        int nMissing = 0;
        int nUnnamed = 0;
        int idx = faceIdx;
        for (std::size_t i = 0; i < request.draftFaces.size(); ++i) {
            const App::SemanticId& face = request.draftFaces[i];
            if (!face.valid()) {
                ++nMissing;
                continue;
            }
            App::ElementIndex bindIdx;
            if (i < request.namedFaceIndices.size()
                && isValidNamedIndex(request.namedFaceIndices[i], "Face")) {
                bindIdx = request.namedFaceIndices[i];
            }
            else if (!request.allowSequentialFaceN) {
                bindIdx = {};
            }
            else {
                bindIdx = faceIndex(idx++);
            }
            const App::ResolutionResult r =
                emitDraft(graph, face, feature, eval, bindIdx);
            if (r.state == App::ResolutionState::Missing
                || r.state == App::ResolutionState::Incompatible) {
                ++nMissing;
                continue;
            }
            if (r.state == App::ResolutionState::Ambiguous) {
                g_lastAfterExecuteNote = "DraftFace Ambiguous; not coerced to a neighbour";
                return;
            }
            if (!Part::isNamedIndex(bindIdx)) {  // PD23-N1 / PD15-style DRY
                ++nUnnamed;
                continue;
            }
            ++nOk;
        }
        if (nOk == 0) {
            if (nUnnamed > 0 && nMissing == 0) {
                g_lastAfterExecuteNote =
                    "skip emit: no named draft history (half-map)";
            }
            else {
                g_lastAfterExecuteNote =
                    "DraftFace Missing; no neighbour invented (I2/I10)";
            }
            return;
        }
        g_lastAfterExecuteNote = "emitted draft faces=" + std::to_string(nOk)
            + " missing=" + std::to_string(nMissing);
        return;
    }

    if (opcode == Opcode::Thickness) {
        if (request.thicknessFaces.empty()) {
            g_lastAfterExecuteNote = "skip emit: no named ThicknessFace seeds (half-map)";
            return;
        }
        int nOk = 0;
        int nMissing = 0;
        int nUnnamed = 0;
        int idx = faceIdx;
        for (std::size_t i = 0; i < request.thicknessFaces.size(); ++i) {
            const App::SemanticId& face = request.thicknessFaces[i];
            if (!face.valid()) {
                ++nMissing;
                continue;
            }
            App::ElementIndex bindIdx;
            if (i < request.namedFaceIndices.size()
                && isValidNamedIndex(request.namedFaceIndices[i], "Face")) {
                bindIdx = request.namedFaceIndices[i];
            }
            else if (!request.allowSequentialFaceN) {
                bindIdx = {};
            }
            else {
                bindIdx = faceIndex(idx++);
            }
            const App::ResolutionResult r =
                emitThickness(graph, face, feature, eval, bindIdx);
            if (r.state == App::ResolutionState::Missing
                || r.state == App::ResolutionState::Incompatible) {
                ++nMissing;
                continue;
            }
            if (r.state == App::ResolutionState::Ambiguous) {
                g_lastAfterExecuteNote = "ThicknessFace Ambiguous; not coerced to a neighbour";
                return;
            }
            if (!Part::isNamedIndex(bindIdx)) {  // PD23-N1 / PD15-style DRY
                ++nUnnamed;
                continue;
            }
            ++nOk;
        }
        if (nOk == 0) {
            if (nUnnamed > 0 && nMissing == 0) {
                g_lastAfterExecuteNote =
                    "skip emit: no named thickness history (half-map)";
            }
            else {
                g_lastAfterExecuteNote =
                    "ThicknessFace Missing; no neighbour invented (I2/I10)";
            }
            return;
        }
        g_lastAfterExecuteNote = "emitted thickness faces=" + std::to_string(nOk)
            + " missing=" + std::to_string(nMissing);
        return;
    }

    if (opcode == Opcode::Hole) {
        // Hole uses StartReference Face seeds. Empty namedFaceIndices → skip
        // history mint (I10). Never sequential FaceN.
        if (request.holeFaces.empty()) {
            g_lastAfterExecuteNote = "skip emit: no named HoleStart seeds (half-map)";
            return;
        }
        int nOk = 0;
        int nMissing = 0;
        int nUnnamed = 0;
        int idx = faceIdx;
        for (std::size_t i = 0; i < request.holeFaces.size(); ++i) {
            const App::SemanticId& face = request.holeFaces[i];
            if (!face.valid()) {
                ++nMissing;
                continue;
            }
            App::ElementIndex bindIdx;
            if (i < request.namedFaceIndices.size()
                && isValidNamedIndex(request.namedFaceIndices[i], "Face")) {
                bindIdx = request.namedFaceIndices[i];
            }
            else if (!request.allowSequentialFaceN) {
                bindIdx = {};
            }
            else {
                bindIdx = faceIndex(idx++);
            }
            const App::ResolutionResult r =
                emitHole(graph, face, feature, eval, bindIdx);
            if (r.state == App::ResolutionState::Missing
                || r.state == App::ResolutionState::Incompatible) {
                ++nMissing;
                continue;
            }
            if (r.state == App::ResolutionState::Ambiguous) {
                g_lastAfterExecuteNote = "HoleStart Ambiguous; not coerced to a neighbour";
                return;
            }
            if (!Part::isNamedIndex(bindIdx)) {  // PD23-N1 / PD15-style DRY
                ++nUnnamed;
                continue;
            }
            ++nOk;
        }
        if (nOk == 0) {
            if (nUnnamed > 0 && nMissing == 0) {
                g_lastAfterExecuteNote =
                    "skip emit: no named hole history (half-map)";
            }
            else {
                g_lastAfterExecuteNote =
                    "HoleStart Missing; no neighbour invented (I2/I10)";
            }
            return;
        }
        g_lastAfterExecuteNote = "emitted hole faces=" + std::to_string(nOk)
            + " missing=" + std::to_string(nMissing);
        return;
    }

    const bool directHistoryOpcode =
        opcode == Opcode::Boolean || opcode == Opcode::AdditiveBox
        || opcode == Opcode::AdditiveCylinder || opcode == Opcode::AdditiveSphere
        || opcode == Opcode::AdditiveCone || opcode == Opcode::AdditiveTorus
        || opcode == Opcode::AdditivePrism || opcode == Opcode::AdditiveWedge
        || opcode == Opcode::AdditiveEllipsoid || opcode == Opcode::SubtractiveBox
        || opcode == Opcode::SubtractiveCylinder || opcode == Opcode::SubtractiveSphere
        || opcode == Opcode::SubtractiveCone || opcode == Opcode::SubtractiveTorus
        || opcode == Opcode::SubtractivePrism || opcode == Opcode::SubtractiveWedge
        || opcode == Opcode::SubtractiveEllipsoid;
    if (directHistoryOpcode) {
        // These publishers bind maker history before this note-only hook. Callers
        // may retain invalid placeholders to preserve positional slots; count
        // values, not vector length, or a half-map would look successful (I13).
        const std::size_t nFace = countValidNamedIndices(request.namedFaceIndices, "Face");
        const std::size_t nEdge = countValidNamedIndices(request.namedEdgeIndices, "Edge");
        const std::string name = opcodeName(opcode);
        if (nFace == 0 && nEdge == 0) {
            g_lastAfterExecuteNote = "skip emit: no named " + name + " history (half-map)";
            return;
        }
        g_lastAfterExecuteNote = "emitted " + name + " faces=" + std::to_string(nFace)
            + " edges=" + std::to_string(nEdge);
        return;
    }

    g_lastAfterExecuteNote = "skip emit: unknown opcode";
}

void SemanticEmitter::bind(App::SemanticGraph* graph,
                           const App::SemanticId& id,
                           App::ObjectId feature,
                           App::EvalSerial eval,
                           const App::ElementIndex& index)
{
    if (!graph || !id.valid()) {
        return;
    }
    App::SemanticBinding row;
    row.stid = id;
    row.feature = feature;
    row.eval = eval;
    row.kind = id.kind;
    row.index = index;
    row.occ = nullptr;
    graph->bind(row);
}

App::SemanticId SemanticEmitter::emitGeneratedFrom(App::SemanticGraph* graph,
                                                   const std::vector<App::SemanticId>& seeds,
                                                   App::SemanticKind outKind,
                                                   const std::string& op,
                                                   App::ObjectId feature,
                                                   App::EvalSerial eval,
                                                   App::SemanticRole role,
                                                   const App::ElementIndex& index)
{
    if (!graph) {
        return {};
    }
    const App::SemanticId child =
        graph->recordGeneratedFrom(seeds, outKind, op, feature, eval, role);
    bind(graph, child, feature, eval, index);
    return child;
}

std::vector<App::SemanticId> SemanticEmitter::emitSplit(
    App::SemanticGraph* graph,
    const App::SemanticId& input,
    std::size_t count,
    const std::string& op,
    App::ObjectId feature,
    App::EvalSerial eval,
    App::SemanticRole role,
    const std::vector<App::ElementIndex>& indices)
{
    if (!graph || !input.valid() || count == 0) {
        return {};
    }
    graph->unbind(input.handle);  // parent is historical; FaceN lives on children
    const auto kids = graph->recordSplit(input, count, op, feature, eval, role);
    for (std::size_t i = 0; i < kids.size(); ++i) {
        if (i < indices.size() && Part::isNamedIndex(indices[i])) {
            bind(graph, kids[i], feature, eval, indices[i]);
        }
    }
    return kids;
}

App::EventId SemanticEmitter::emitDeleted(App::SemanticGraph* graph,
                                          const App::SemanticId& input,
                                          const std::string& op,
                                          App::ObjectId feature,
                                          App::EvalSerial eval,
                                          App::SemanticRole role)
{
    if (!graph || !input.valid()) {
        return 0;
    }
    return graph->recordDeleted(input, op, feature, eval, role);
}

App::EventId SemanticEmitter::emitModified(App::SemanticGraph* graph,
                                           const App::SemanticId& input,
                                           const std::string& op,
                                           App::ObjectId feature,
                                           App::EvalSerial eval,
                                           App::SemanticRole role,
                                           const App::ElementIndex& index)
{
    if (!graph || !input.valid()) {
        return 0;
    }
    const App::EventId ev = graph->recordModified(input, op, feature, eval, role);
    if (Part::isNamedIndex(index)) {
        bind(graph, input, feature, eval, index);
    }
    return ev;
}

std::vector<App::SemanticId> SemanticEmitter::emitPadSides(
    App::SemanticGraph* graph,
    const std::vector<App::SemanticId>& curveSeeds,
    App::ObjectId feature,
    App::EvalSerial eval,
    int firstFaceIndex)
{
    if (!graph) {
        return {};
    }
    std::vector<App::SemanticId> sides;
    sides.reserve(curveSeeds.size());
    int idx = firstFaceIndex;
    for (const App::SemanticId& curve : curveSeeds) {
        sides.push_back(emitGeneratedFrom(graph,
                                          {curve},
                                          App::SemanticKind::Face,
                                          "Pad",
                                          feature,
                                          eval,
                                          App::SemanticRole::None,
                                          faceIndex(idx++)));
    }
    return sides;
}

std::vector<App::SemanticId> SemanticEmitter::emitPadCaps(
    App::SemanticGraph* graph,
    const std::vector<App::SemanticId>& regionSeeds,
    App::ObjectId feature,
    App::EvalSerial eval,
    int firstFaceIndex)
{
    if (!graph) {
        return {};
    }
    std::vector<App::SemanticId> caps;
    // Two caps (start, end) per region seed — both Generated from that region.
    int idx = firstFaceIndex;
    for (const App::SemanticId& region : regionSeeds) {
        caps.push_back(emitGeneratedFrom(graph,
                                         {region},
                                         App::SemanticKind::Face,
                                         "Pad",
                                         feature,
                                         eval,
                                         App::SemanticRole::None,
                                         faceIndex(idx++)));
        caps.push_back(emitGeneratedFrom(graph,
                                         {region},
                                         App::SemanticKind::Face,
                                         "Pad",
                                         feature,
                                         eval,
                                         App::SemanticRole::None,
                                         faceIndex(idx++)));
    }
    return caps;
}

std::vector<App::SemanticId> SemanticEmitter::emitPocketSplit(
    App::SemanticGraph* graph,
    const App::SemanticId& target,
    std::size_t count,
    App::ObjectId feature,
    App::EvalSerial eval,
    int firstFaceIndex)
{
    if (!graph) {
        return {};
    }
    std::vector<App::ElementIndex> indices;
    indices.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        indices.push_back(faceIndex(firstFaceIndex + static_cast<int>(i)));
    }
    return emitSplit(graph,
                     target,
                     count,
                     "Pocket",
                     feature,
                     eval,
                     App::SemanticRole::None,
                     indices);
}

App::SemanticId SemanticEmitter::emitPocketHole(App::SemanticGraph* graph,
                                                const App::SemanticId& remnant,
                                                const App::SemanticId& pocketRegion,
                                                std::size_t wallCount,
                                                App::ObjectId feature,
                                                App::EvalSerial eval,
                                                const App::ElementIndex& remnantIndex,
                                                int firstWallFaceIndex)
{
    if (!graph) {
        return {};
    }
    // S3: remnant keeps the handle (Modified). Walls are new Generated.
    emitModified(graph, remnant, "Pocket", feature, eval, App::SemanticRole::None, remnantIndex);
    int idx = firstWallFaceIndex;
    for (std::size_t i = 0; i < wallCount; ++i) {
        emitGeneratedFrom(graph,
                          {pocketRegion},
                          App::SemanticKind::Face,
                          "Pocket",
                          feature,
                          eval,
                          App::SemanticRole::None,
                          faceIndex(idx++));
    }
    return remnant;
}

App::ResolutionResult SemanticEmitter::emitFillet(
    App::SemanticGraph* graph,
    const App::SemanticId& edge,
    const std::vector<App::SemanticId>& adjacentFaces,
    App::ObjectId feature,
    App::EvalSerial eval,
    const App::ElementIndex& faceIndex_)
{
    App::ResolutionResult inbound;
    if (!graph) {
        inbound.state = App::ResolutionState::Missing;
        return inbound;
    }

    App::SemanticReference ref = referenceFor(edge, OpcodeRoleId::FilletEdge);
    ref.kind = App::SemanticKind::Edge;
    const App::ReferenceRequirement req = requirementFor(OpcodeRoleId::FilletEdge);
    inbound = App::SemanticResolver::resolve(ref, *graph, &req);

    if (inbound.state == App::ResolutionState::Missing
        || inbound.state == App::ResolutionState::Incompatible) {
        // Missing dress-up. Do not search for a similar-length neighbour (I10 / §10).
        return inbound;
    }
    if (inbound.state == App::ResolutionState::Ambiguous) {
        return inbound;
    }

    // Bind Generated fillet faces only when history named an index. Sequential
    // FaceN is not a substitute (I13 / half-map).
    if (!Part::isNamedIndex(faceIndex_)) {  // PD23-N1 / PD15-style DRY
        return inbound;
    }

    std::vector<App::SemanticId> seeds;
    seeds.push_back(edge);
    appendUniqueSemanticIds(seeds, adjacentFaces);
    const App::SemanticId face = emitGeneratedFrom(graph,
                                                   seeds,
                                                   App::SemanticKind::Face,
                                                   "Fillet",
                                                   feature,
                                                   eval,
                                                   App::SemanticRole::None,
                                                   faceIndex_);
    inbound.identities.push_back(face);
    return inbound;
}

App::ResolutionResult SemanticEmitter::emitChamfer(
    App::SemanticGraph* graph,
    const App::SemanticId& edge,
    const std::vector<App::SemanticId>& adjacentFaces,
    App::ObjectId feature,
    App::EvalSerial eval,
    const App::ElementIndex& faceIndex_)
{
    App::ResolutionResult inbound;
    if (!graph) {
        inbound.state = App::ResolutionState::Missing;
        return inbound;
    }

    App::SemanticReference ref = referenceFor(edge, OpcodeRoleId::ChamferEdge);
    ref.kind = App::SemanticKind::Edge;
    const App::ReferenceRequirement req = requirementFor(OpcodeRoleId::ChamferEdge);
    inbound = App::SemanticResolver::resolve(ref, *graph, &req);

    if (inbound.state == App::ResolutionState::Missing
        || inbound.state == App::ResolutionState::Incompatible) {
        // Missing dress-up. Do not search for a similar-length neighbour (I10 / section 10).
        return inbound;
    }
    if (inbound.state == App::ResolutionState::Ambiguous) {
        return inbound;
    }

    // Bind Generated chamfer faces only when history named an index. Sequential
    // FaceN is not a substitute (I13 / half-map).
    if (!Part::isNamedIndex(faceIndex_)) {  // PD23-N1 / PD15-style DRY
        return inbound;
    }

    std::vector<App::SemanticId> seeds;
    seeds.push_back(edge);
    appendUniqueSemanticIds(seeds, adjacentFaces);
    const App::SemanticId face = emitGeneratedFrom(graph,
                                                   seeds,
                                                   App::SemanticKind::Face,
                                                   "Chamfer",
                                                   feature,
                                                   eval,
                                                   App::SemanticRole::None,
                                                   faceIndex_);
    inbound.identities.push_back(face);
    return inbound;
}

App::ResolutionResult SemanticEmitter::emitDraft(
    App::SemanticGraph* graph,
    const App::SemanticId& face,
    App::ObjectId feature,
    App::EvalSerial eval,
    const App::ElementIndex& faceIndex_)
{
    App::ResolutionResult inbound;
    if (!graph) {
        inbound.state = App::ResolutionState::Missing;
        return inbound;
    }

    App::SemanticReference ref = referenceFor(face, OpcodeRoleId::DraftFace);
    ref.kind = App::SemanticKind::Face;
    const App::ReferenceRequirement req = requirementFor(OpcodeRoleId::DraftFace);
    inbound = App::SemanticResolver::resolve(ref, *graph, &req);

    if (inbound.state == App::ResolutionState::Missing
        || inbound.state == App::ResolutionState::Incompatible) {
        // Missing dress-up. Do not search for a similar-angle neighbour (I10 / section 10).
        return inbound;
    }
    if (inbound.state == App::ResolutionState::Ambiguous) {
        return inbound;
    }

    // Bind Generated draft faces only when history named an index. Sequential
    // FaceN is not a substitute (I13 / half-map).
    if (!Part::isNamedIndex(faceIndex_)) {  // PD23-N1 / PD15-style DRY
        return inbound;
    }

    const App::SemanticId out = emitGeneratedFrom(graph,
                                                  {face},
                                                  App::SemanticKind::Face,
                                                  "Draft",
                                                  feature,
                                                  eval,
                                                  App::SemanticRole::None,
                                                  faceIndex_);
    inbound.identities.push_back(out);
    return inbound;
}

App::ResolutionResult SemanticEmitter::emitThickness(
    App::SemanticGraph* graph,
    const App::SemanticId& face,
    App::ObjectId feature,
    App::EvalSerial eval,
    const App::ElementIndex& faceIndex_)
{
    App::ResolutionResult inbound;
    if (!graph) {
        inbound.state = App::ResolutionState::Missing;
        return inbound;
    }

    App::SemanticReference ref = referenceFor(face, OpcodeRoleId::ThicknessFace);
    ref.kind = App::SemanticKind::Face;
    const App::ReferenceRequirement req = requirementFor(OpcodeRoleId::ThicknessFace);
    inbound = App::SemanticResolver::resolve(ref, *graph, &req);

    if (inbound.state == App::ResolutionState::Missing
        || inbound.state == App::ResolutionState::Incompatible) {
        // Missing dress-up. Do not search for a neighbour (I10 / section 10).
        return inbound;
    }
    if (inbound.state == App::ResolutionState::Ambiguous) {
        return inbound;
    }

    // Bind Generated thickness faces only when history named an index. Sequential
    // FaceN is not a substitute (I13 / half-map).
    if (!Part::isNamedIndex(faceIndex_)) {  // PD23-N1 / PD15-style DRY
        return inbound;
    }

    const App::SemanticId out = emitGeneratedFrom(graph,
                                                  {face},
                                                  App::SemanticKind::Face,
                                                  "Thickness",
                                                  feature,
                                                  eval,
                                                  App::SemanticRole::None,
                                                  faceIndex_);
    inbound.identities.push_back(out);
    return inbound;
}

App::ResolutionResult SemanticEmitter::emitHole(
    App::SemanticGraph* graph,
    const App::SemanticId& face,
    App::ObjectId feature,
    App::EvalSerial eval,
    const App::ElementIndex& faceIndex_)
{
    App::ResolutionResult inbound;
    if (!graph) {
        inbound.state = App::ResolutionState::Missing;
        return inbound;
    }

    App::SemanticReference ref = referenceFor(face, OpcodeRoleId::HoleStart);
    ref.kind = App::SemanticKind::Face;
    const App::ReferenceRequirement req = requirementFor(OpcodeRoleId::HoleStart);
    inbound = App::SemanticResolver::resolve(ref, *graph, &req);

    if (inbound.state == App::ResolutionState::Missing
        || inbound.state == App::ResolutionState::Incompatible) {
        // Missing StartReference. Do not search for a neighbour (I10).
        return inbound;
    }
    if (inbound.state == App::ResolutionState::Ambiguous) {
        return inbound;
    }

    // Bind Generated hole faces only when history named an index. Sequential
    // FaceN is not a substitute (I13 / half-map).
    if (!Part::isNamedIndex(faceIndex_)) {  // PD23-N1 / PD15-style DRY
        return inbound;
    }

    // Hole keeps raw hasOutputOwnershipConflict: invalid index returns inbound
    // without Ambiguous; conflict sets Ambiguous — different refuse contract than
    // App::shouldRefuseNamedEmitSlot (invalid typed index → refuse emit / leave unnamed).
    if (App::hasOutputOwnershipConflict(graph, feature, faceIndex_)) {
        inbound.state = App::ResolutionState::Ambiguous;
        return inbound;
    }

    const App::SemanticId out = emitGeneratedFrom(graph,
                                                  {face},
                                                  App::SemanticKind::Face,
                                                  "Hole",
                                                  feature,
                                                  eval,
                                                  App::SemanticRole::None,
                                                  faceIndex_);
    inbound.identities.push_back(out);
    return inbound;
}

std::vector<App::SemanticId> SemanticEmitter::generatedFrom(const App::SemanticGraph& graph,
                                                            App::SemanticHandle seed)
{
    std::vector<App::SemanticId> out;
    if (seed == 0) {
        return out;
    }
    std::vector<App::EventId> events;
    for (const App::EventInput& in : graph.inputs()) {
        if (in.semantic.handle != seed) {
            continue;
        }
        const App::Event* ev = graph.eventById(in.event);
        if (ev && ev->kind == App::EventKind::Generated) {
            events.push_back(in.event);
        }
    }
    for (const App::EventOutput& o : graph.outputs()) {
        if (std::find(events.begin(), events.end(), o.event) == events.end()) {
            continue;
        }
        out.push_back(o.semantic);
    }
    return out;
}

std::vector<App::SemanticId> SemanticEmitter::propagateSourceSplit(
    App::SemanticGraph* graph,
    const App::SemanticId& source,
    std::size_t childCount,
    const std::string& op,
    App::ObjectId feature,
    App::EvalSerial eval,
    int firstFaceIndex)
{
    if (!graph || !source.valid() || childCount == 0) {
        return {};
    }

    // Only Split children count as "already split" remnants of a side/cap.
    // Generated descendants (fillet faces from an adjacent pad face) are a
    // different opcode and must not steal the source-split walk (I2).
    const auto produced = generatedFrom(*graph, source.handle);
    std::vector<App::SemanticId> roots;
    for (const App::SemanticId& p : produced) {
        std::vector<App::SemanticId> splitKids;
        for (const App::EventInput& in : graph->inputs()) {
            if (in.semantic.handle != p.handle) {
                continue;
            }
            const App::Event* ev = graph->eventById(in.event);
            if (!ev || ev->kind != App::EventKind::Split) {
                continue;
            }
            for (const App::EventOutput& o : graph->outputs()) {
                if (o.event != in.event) {
                    continue;
                }
                splitKids.push_back(o.semantic);
            }
        }
        // Generated-from (fillet using this face as adjacent) does not retire
        // the side/cap. Only a Split event does. Do not call isHistorical()
        // here — that treats any EventOutput parent as retired.
        if (splitKids.empty()) {
            roots.push_back(p);
            continue;
        }
        bool anyLive = false;
        for (const App::SemanticId& k : splitKids) {
            if (!graph->bindingsOf(k.handle).empty()) {
                roots.push_back(k);
                anyLive = true;
            }
        }
        if (!anyLive) {
            roots.insert(roots.end(), splitKids.begin(), splitKids.end());
        }
    }

    std::vector<App::SemanticId> allKids;
    int idx = firstFaceIndex;
    for (const App::SemanticId& root : roots) {
        std::vector<App::ElementIndex> indices;
        for (std::size_t i = 0; i < childCount; ++i) {
            indices.push_back(faceIndex(idx++));
        }
        auto kids = emitSplit(graph, root, childCount, op, feature, eval,
                              App::SemanticRole::None, indices);
        allKids.insert(allKids.end(), kids.begin(), kids.end());
    }
    return allKids;
}

}  // namespace PartDesign
