/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/
//DrawComplexSection processing overview
//for Strategy = Offset, DCS is much the same as DVS
//for Strategy = Aligned, there are many differences

//execute
//    sectionExec(getShapeToCut()*)

//sectionExec
//    makeSectionCut(baseShape)

//makeSectionCut (separate thread)
//    note that the section cut is not required for Aligned strategy,
//    but it is useful for debugging
//    m_cuttingTool = makeCuttingTool* (DVSTool.brep)
//    m_cutPieces = (baseShape - m_cuttingTool) (DVSCutPieces.brep)

//onSectionCutFinished
//    m_preparedShape = prepareShape(m_cutPieces)* - centered, scaled, rotated
//    geometryObject = DVP::buildGeometryObject(m_preparedShape)  (HLR)

//postHlrTasks
//    faceIntersections = findSectionPlaneIntersections
//    m_sectionTopoDSFaces = alignSectionFaces(faceIntersections)
//    m_tdSectionFaces = makeTDSectionFaces(m_sectionTopoDSFaces)

//* for Aligned, we use a different ShapeToCut, as the standard one will
//  cause many coincident face problems later
//* the cutting tool is built up from the profile, instead of the simple plane in DVS
//* for Aligned, preparing the shape is much different than Offset or DVS
//    - most of the work is done in makeAlignedPieces
//    - for each segment of the profile, make a cutting tool, then get the boolean
//      intersection of the tool and the shape to cut
//    - align and distribute the intersections along an "effective" section plane
//      which is a flattened version of the profile

#include "PreCompiled.h"

#ifndef _PreComp_
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Mod/Part/App/FCBRepAlgoAPI_Common.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
#include <BRepAlgo_NormalProjection.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepGProp.hxx>
#include <BRepLProp_SLProps.hxx>
#include <BRepLib.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <Bnd_OBB.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GProp_GProps.hxx>
#include <Geom_Plane.hxx>
#include <HLRAlgo_Projector.hxx>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <ShapeExtend_WireData.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#endif

#include <cmath>

#include <sstream>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Material.h>
#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Converter.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>

#include <Mod/Part/App/PartFeature.h>

#include "DrawComplexSection.h"
#include "DrawUtil.h"
#include "GeometryObject.h"
#include "ShapeUtils.h"

using namespace TechDraw;
using namespace std;
using DU = DrawUtil;

//===========================================================================
// DrawComplexSection
//===========================================================================

//NOLINTBEGIN
PROPERTY_SOURCE(TechDraw::DrawComplexSection, TechDraw::DrawViewSection)

const char* DrawComplexSection::ProjectionStrategyEnums[] = {"Offset", "Aligned", "NoParallel",
                                                             nullptr};
//NOLINTEND

DrawComplexSection::DrawComplexSection() :
    m_waitingForAlign(false)
{
    static const char* fgroup = "Cutting Tool";

//NOLINTBEGIN
    ADD_PROPERTY_TYPE(CuttingToolWireObject, (nullptr), fgroup, App::Prop_None,
                      "A sketch that describes the cutting tool");
    CuttingToolWireObject.setScope(App::LinkScope::Global);
    ProjectionStrategy.setEnums(ProjectionStrategyEnums);
    ADD_PROPERTY_TYPE(ProjectionStrategy, ((long)0), fgroup, App::Prop_None,
                      "Make a single cut, or use the profile in pieces");
//NOLINTEND
}

TopoDS_Shape DrawComplexSection::makeCuttingTool(double dMax)
{
    TopoDS_Wire profileWire = makeProfileWire();
    if (profileWire.IsNull()) {
        throw Base::RuntimeError("Can not make wire from cutting tool (1)");
    }

    if (debugSection()) {
        //the nose to tail version of the profile
        BRepTools::Write(profileWire, "DCSProfileWire.brep");//debug
    }

    gp_Ax2 sectionCS = getSectionCS();  // should validate this some where

    auto uSectionNormal = SectionNormal.getValue();
    uSectionNormal.Normalize();
    auto gWireToFaceDirection = Base::convertTo<gp_Vec>(getReferenceAxis());
    bool isPositionOK = validateProfilePosition(profileWire, sectionCS);
    if (!isPositionOK) {
        //profile is not in a good position.  Result might not be right.
        Base::Console().warning("DCS::makeCuttingTool - %s - profile is outside shape box\n",
                                getNameInDocument());
    }

    //move the profile wire to one side of the shape
    gp_Trsf mov;
    mov.SetTranslation(gp_Vec(gWireToFaceDirection) * (-dMax));
    TopLoc_Location loc(mov);
    profileWire.Move(loc);

    if (BRep_Tool::IsClosed(profileWire)) {
        // TopoDS_Shape makePrismForClosedProfile(profileWire)
        // Wire is closed, so make a face from it and extrude "vertically"
        BRepBuilderAPI_MakeFace mkFace(profileWire);
        TopoDS_Face toolFace = mkFace.Face();
        if (toolFace.IsNull()) {
            return {};
        }
        gp_Dir gpNormal = getFaceNormal(toolFace);
        auto extrudeDir = 2 * dMax * gpNormal;
        return BRepPrimAPI_MakePrism(toolFace, extrudeDir).Shape();
    }

    if (ProjectionStrategy.getValue() == 0) {
        // Offset. Warn if profile is not quite aligned with section normal. if
        // the profile and normal are misaligned, the check below for empty "solids"
        // will not be correct.
        constexpr double AngleThresholdDeg{5.0};
        // bool isOK =
        validateOffsetProfile(profileWire, SectionNormal.getValue(), AngleThresholdDeg);
    }

    m_toolFaceShape = extrudeWireToFace(profileWire, gWireToFaceDirection, 2 * dMax);
    if (debugSection()) {
        BRepTools::Write(m_toolFaceShape, "DCSToolFaceShape.brep");//debug
    }

    // should this use the circular arc closing method as in closeProfile()??
    auto points = getPointsForClosingProfile(profileWire, dMax);

    BRepBuilderAPI_MakeWire mkWire(profileWire);

    for(size_t iFrom = 0; iFrom < points.size() - 1 ; iFrom++) {
        size_t jTo = iFrom + 1;
        BRepBuilderAPI_MakeEdge mkEdge(Base::convertTo<gp_Pnt>(points.at(iFrom)),
                                       Base::convertTo<gp_Pnt>(points.at(jTo)));
        mkWire.Add(mkEdge.Edge());
    }

    BRepBuilderAPI_MakeFace mkFace(mkWire.Wire());
    if (debugSection()) {
        BRepTools::Write(mkFace.Face(), "DCSToolPlan.brep");   //debug
    }
    auto padVector = gWireToFaceDirection * dMax * 2;
    TopoDS_Shape roughTool = BRepPrimAPI_MakePrism(mkFace.Face(), padVector).Shape();

    if (roughTool.ShapeType() == TopAbs_COMPSOLID ||
        roughTool.ShapeType() == TopAbs_COMPOUND) {
        //Composite Solids do not cut well if they contain "solids" with no volume. This
        //happens if the profile has segments parallel to the extrude direction.
        //We need to disassemble it and only keep the real solids.
        return removeEmptyShapes(roughTool);
    }

    return roughTool;
}

TopoDS_Shape DrawComplexSection::getShapeToPrepare() const
{
    if (ProjectionStrategy.getValue() == 0) {
        //Offset. Use regular section behaviour
        return DrawViewSection::getShapeToPrepare();
    }
    //Aligned strategy
    return m_saveShape;//the original input shape
}

//get the shape ready for projection and cut surface finding
TopoDS_Shape DrawComplexSection::prepareShape(const TopoDS_Shape& cutShape, double shapeSize)
{
    if (ProjectionStrategy.getValue() == 0) {
        //Offset. Use regular section behaviour
        return DrawViewSection::prepareShape(cutShape, shapeSize);
    }

    //"Aligned" projection (Aligned Section)
    if (m_alignResult.IsNull()) {
        return {};
    }

    // our shape is already centered "left/right" and "up/down" so we don't need to
    // center it here
    // TopoDS_Shape centeredShape = ShapeUtils::centerShapeXY(m_alignResult, getProjectionCS());
    m_preparedShape = ShapeUtils::scaleShape(m_alignResult, getScale());
    if (!DrawUtil::fpCompare(Rotation.getValue(), 0.0)) {
        m_preparedShape =
            ShapeUtils::rotateShape(m_preparedShape, getProjectionCS(), Rotation.getValue());
    }

    if (debugSection()) {
        BRepTools::Write(m_preparedShape, "DCS60preparedShape.brep"); //debug
    }

    return m_preparedShape;
}


void DrawComplexSection::makeSectionCut(const TopoDS_Shape& baseShape)
{
    if (ProjectionStrategy.getValue() == 0) {
        //Offset. Use regular section behaviour
        return DrawViewSection::makeSectionCut(baseShape);
    }

    try {
        connectAlignWatcher =
            QObject::connect(&m_alignWatcher, &QFutureWatcherBase::finished, &m_alignWatcher,
                             [this] { this->onSectionCutFinished(); });

        // We create a lambda closure to hold a copy of baseShape.
        // This is important because this variable might be local to the calling
        // function and might get destructed before the parallel processing finishes.
        auto lambda = [this, baseShape]{this->makeAlignedPieces(baseShape);};
        m_alignFuture = QtConcurrent::run(std::move(lambda));
        m_alignWatcher.setFuture(m_alignFuture);
        waitingForAlign(true);
    }
    catch (...) {
        Base::Console().message("DCS::makeSectionCut - failed to make alignedPieces");
        return;
    }

    return DrawViewSection::makeSectionCut(baseShape);
}


void DrawComplexSection::onSectionCutFinished()
{
    if (m_cutFuture.isRunning() ||  //waitingForCut()
        m_alignFuture.isRunning()) {//waitingForAlign()
        //can not continue yet.  return until the other thread ends
        return;
    }

    DrawViewSection::onSectionCutFinished();

    QObject::disconnect(connectAlignWatcher);
}

//for Aligned strategy, cut the rawShape by each segment of the tool
void DrawComplexSection::makeAlignedPieces(const TopoDS_Shape& rawShape)
{
    if (!canBuild(getSectionCS(), CuttingToolWireObject.getValue())) {
        throw Base::RuntimeError("Profile is parallel to Section Normal");
    }

    if (debugSection()) {
        BRepTools::Write(rawShape, "DCSRawShape.brep");//debug
    }

    auto uSectionNormal = SectionNormal.getValue();
    uSectionNormal.Normalize();

    TopoDS_Wire profileWire = makeProfileWire();
    if (profileWire.IsNull()) {
        throw Base::RuntimeError("ComplexSection failed to make profileWire");
    }

    auto uRotateAxis = getReferenceAxis();
    uRotateAxis.Normalize();


    // the reversers control left to right vs right to left (or top to bottom vs bottom to top)
    // arrangement of the cut pieces.
    double horizReverser{1.0};
    double verticalReverser{1.0};
    gp_Vec gProfileVec = makeProfileVector(profileWire);
    auto isProfileVertical = getReversers(gProfileVec, horizReverser, verticalReverser);

    std::vector<TopoDS_Shape> pieces;       // results of cutting source with each segment's tool shape
    std::vector<double> pieceXSizeAll;      //size in sectionCS.XDirection (width)
    std::vector<double> pieceYSizeAll;      //size in sectionCS.YDirection (height)
    std::vector<double> pieceZSizeAll;      //size in sectionCS.Direction (depth)
    std::vector<double> pieceVerticalAll;   // displacement of piece in vertical direction

    auto faceNormals = getSegmentViewDirections(profileWire, uSectionNormal, uRotateAxis);
    TopExp_Explorer expFaces(m_toolFaceShape, TopAbs_FACE);
    for (int iPiece = 0; expFaces.More(); expFaces.Next(), iPiece++) {
        TopoDS_Face face = TopoDS::Face(expFaces.Current());
        auto segmentNormal = Base::convertTo<gp_Vec>(faceNormals.at(iPiece).second);
        if (!showSegment(segmentNormal)) {
            //skip this segment of the profile
            continue;
        }

        double pieceVertical{0};
        auto rotatedPiece = cutAndRotatePiece(rawShape, face, iPiece, faceNormals.at(iPiece).second, uRotateAxis, pieceVertical);

        auto sizeResponse = getAlignedSize(rotatedPiece, iPiece);
        auto pieceSize = sizeResponse.pieceSize;
        pieceXSizeAll.push_back(pieceSize.x);    // size in ProjectionCS.
        pieceYSizeAll.push_back(pieceSize.y);
        pieceZSizeAll.push_back(pieceSize.z);
        pieceVerticalAll.push_back(pieceVertical);

        if (debugSection()) {
            stringstream ss;
            ss << "DCSAlignedPiece" << iPiece << ".brep";
            BRepTools::Write(sizeResponse.alignedPiece, ss.str().c_str());//debug
        }

        auto pieceOnPlane = movePieceToPaperPlane(sizeResponse.alignedPiece, sizeResponse.zMax);
        if (debugSection()) {
            stringstream ss;
            ss << "DCSEpieceOnPlane" << iPiece << ".brep";
            BRepTools::Write(pieceOnPlane, ss.str().c_str());//debug
        }
        // pieceOnPlane is on the paper plane, with piece centroid at the origin
        pieces.push_back(pieceOnPlane);
    }

    if (pieces.empty()) {
        m_alignResult = TopoDS_Compound();
        return;
    }

    //space the pieces "horizontally" or "vertically" in OXYZ
    double movementReverser = isProfileVertical ? verticalReverser : horizReverser;
    auto movementAxis = gp_Vec(gp::OX().Direction());
    auto alignmentAxis = gp_Vec(gp::OY().Direction().Reversed());
    if (isProfileVertical) {
        movementAxis = gp_Vec(gp::OY().Direction());
        alignmentAxis = gp_Vec(gp::OX().Direction());
    }
    gp_Vec gMovementVector = movementAxis * movementReverser;

    size_t stopAt = pieces.size();
    double cursorPosition = 0.0;
    for (size_t iPiece = 0; iPiece < stopAt; iPiece++) {
        double pieceSizeMoveDist = pieceXSizeAll.at(iPiece);
        if (isProfileVertical) {
            pieceSizeMoveDist = pieceYSizeAll.at(iPiece);
        }
        auto movedPiece = distributePiece(pieces.at(iPiece), pieceSizeMoveDist, pieceVerticalAll.at(iPiece),
                                   alignmentAxis, gMovementVector, cursorPosition);
        pieces.at(iPiece) = movedPiece;
        cursorPosition += pieceSizeMoveDist;

        if (debugSection()) {
            stringstream ss;
            ss << "DCSMovedPiece" << iPiece << ".brep";
            BRepTools::Write(pieces.at(iPiece), ss.str().c_str());//debug
        }
    }

    //make a compound of the aligned pieces
    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    for (auto& piece : pieces) {
        builder.Add(comp, piece);
    }

    //center the compound along SectionCS XDirection
    Base::Vector3d centerVector = Base::convertTo<Base::Vector3d>(gMovementVector) * cursorPosition / -2;
    TopoDS_Shape centeredCompound = ShapeUtils::moveShape(comp, centerVector);

    if (debugSection()) {
        BRepTools::Write(centeredCompound, "DCSmap40CenteredCompound.brep");//debug
    }

    // re-align our shape with the projection CS
    gp_Ax3 stdCS;                                 //OXYZ
    gp_Trsf xPieceAlign;
    xPieceAlign.SetTransformation(getProjectionCS(), stdCS);
    BRepBuilderAPI_Transform mkTransAlign(centeredCompound, xPieceAlign);
    TopoDS_Shape alignedCompound = mkTransAlign.Shape();

    if (debugSection()) {
        BRepTools::Write(alignedCompound, "DCSmap50AlignedCompound.brep");//debug
    }

    m_alignResult = alignedCompound;
}

//! tries to find the intersection faces of the cut shape and the cutting tool.
//! Aligned complex sections need to intersect the final cut shape (which in this
//! case is a compound of individual cuts) with the "effective" (flattened) section plane.
//! Profiles containing curves need special handling.
TopoDS_Compound
DrawComplexSection::findSectionPlaneIntersections(const TopoDS_Shape& shapeToIntersect)
{
    if (shapeToIntersect.IsNull()) {
        // this shouldn't happen
        Base::Console().warning("DCS::findSectionPlaneInter - %s - cut shape is Null\n",
                                getNameInDocument());
        return {};
    }
    if (ProjectionStrategy.getValue() == 0) {//Offset
        return singleToolIntersections(shapeToIntersect);
    }

    return alignedToolIntersections(shapeToIntersect);
}

//Intersect cutShape with each segment of the cutting tool
TopoDS_Compound DrawComplexSection::singleToolIntersections(const TopoDS_Shape& cutShape)
{
    App::DocumentObject* toolObj = CuttingToolWireObject.getValue();
    if (!isLinearProfile(toolObj)) {
        //TODO: special handling here
        //        Base::Console().message("DCS::singleToolIntersection - profile has curves\n");
    }

    BRep_Builder builder;
    TopoDS_Compound result;
    builder.MakeCompound(result);

    if (debugSection()) {
        BRepTools::Write(cutShape, "DCSOffsetCutShape.brep");              //debug
        BRepTools::Write(m_toolFaceShape, "DCSOffsetCuttingToolFace.brep");//debug
    }

    if (m_toolFaceShape.IsNull()) {
        return result;
    }

    TopExp_Explorer expFaces(cutShape, TopAbs_FACE);
    for (; expFaces.More(); expFaces.Next()) {
        TopoDS_Face face = TopoDS::Face(expFaces.Current());
        if (!boxesIntersect(face, m_toolFaceShape)) {
            continue;
        }
        std::vector<TopoDS_Face> commonFaces = faceShapeIntersect(face, m_toolFaceShape);
        for (auto& cFace : commonFaces) {
            builder.Add(result, cFace);
        }
    }
    return result;
}

//Intersect cutShape with the effective (flattened paper plane) cutting plane to generate cut surface faces
TopoDS_Compound DrawComplexSection::alignedToolIntersections(const TopoDS_Shape& cutShape)
{
    BRep_Builder builder;
    TopoDS_Compound result;
    builder.MakeCompound(result);

    App::DocumentObject* toolObj = CuttingToolWireObject.getValue();
    if (!isLinearProfile(toolObj)) {
        //TODO: special handling here?
        //        Base::Console().message("DCS::alignedToolIntersection - profile has curves\n");
    }

    gp_Pln effectivePlane = getSectionPlane();
    //aligned result can be much wider than the shape itself, so we use an
    //infinite face.
    BRepBuilderAPI_MakeFace mkFace(effectivePlane, -Precision::Infinite(), Precision::Infinite(),
                                   -Precision::Infinite(), Precision::Infinite());
    TopoDS_Face cuttingFace = mkFace.Face();

    TopExp_Explorer expFaces(cutShape, TopAbs_FACE);
    for (; expFaces.More(); expFaces.Next()) {
        TopoDS_Face face = TopoDS::Face(expFaces.Current());
        if (!boxesIntersect(face, cuttingFace)) {
            continue;
        }
        std::vector<TopoDS_Face> commonFaces = faceShapeIntersect(face, cuttingFace);
        for (auto& cFace : commonFaces) {
            builder.Add(result, cFace);
        }
    }
    if (debugSection()) {
        BRepTools::Write(cuttingFace, "DCSAlignedCuttingFace.brep");  //debug
        BRepTools::Write(cutShape, "DCSAlignedCutShape.brep");        //debug
        BRepTools::Write(result, "DCSAlignedIntersectionResult.brep");//debug
    }
    return result;
}

TopoDS_Compound DrawComplexSection::alignSectionFaces(const TopoDS_Shape& faceIntersections)
{
    if (ProjectionStrategy.getValue() == 0) {
        //Offset. Use regular section behaviour
        return DrawViewSection::alignSectionFaces(faceIntersections);
    }

    return TopoDS::Compound(mapToPage(faceIntersections));
}

TopoDS_Shape DrawComplexSection::getShapeToIntersect()
{
    if (ProjectionStrategy.getValue() == 0) {//Offset
        return DrawViewSection::getShapeToIntersect();
    }
    //Aligned
    return m_preparedShape;
}

TopoDS_Shape DrawComplexSection::getShapeForDetail() const
{
    if (ProjectionStrategy.getValue() == 0) {//Offset
        return DrawViewSection::getShapeForDetail();
    }
    //Aligned
    return m_preparedShape;
}

TopoDS_Wire DrawComplexSection::makeProfileWire() const
{
    App::DocumentObject* toolObj = CuttingToolWireObject.getValue();
    return makeProfileWire(toolObj);
}

TopoDS_Wire DrawComplexSection::makeProfileWire(App::DocumentObject* toolObj)
{
    if (!isProfileObject(toolObj)) {
        return {};
    }

    TopoDS_Shape toolShape = Part::Feature::getShape(toolObj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
    if (toolShape.IsNull()) {
        return {};
    }

    TopoDS_Wire profileWire;
    if (toolShape.ShapeType() == TopAbs_WIRE) {
        profileWire = makeNoseToTailWire(TopoDS::Wire(toolShape));
    }
    else {//we have already checked that the shape is a wire or an edge in isProfileObject
        TopoDS_Edge edge = TopoDS::Edge(toolShape);
        profileWire = BRepBuilderAPI_MakeWire(edge).Wire();
    }
    return profileWire;
}

gp_Vec DrawComplexSection::makeProfileVector(const TopoDS_Wire& profileWire)
{
    auto ends = getWireEnds(profileWire);
    auto vec = ends.second - ends.first;
    vec.Normalize();
    return Base::convertTo<gp_Vec>(vec);
}

//methods related to section line

//make drawable td geometry for section line
BaseGeomPtrVector DrawComplexSection::makeSectionLineGeometry()
{
    BaseGeomPtrVector result;
    auto* baseDvp = freecad_cast<DrawViewPart*>(BaseView.getValue());
    if (baseDvp) {
        TopoDS_Wire lineWire = makeSectionLineWire();
        TopoDS_Shape projectedWire =
            GeometryObject::projectSimpleShape(lineWire, baseDvp->getProjectionCS());
        TopExp_Explorer expEdges(projectedWire, TopAbs_EDGE);
        for (; expEdges.More(); expEdges.Next()) {
            BaseGeomPtr edge = BaseGeom::baseFactory(TopoDS::Edge(expEdges.Current()));
            result.push_back(edge);
        }
    }
    return result;
}

//get the end points of the section wire
std::pair<Base::Vector3d, Base::Vector3d> DrawComplexSection::sectionLineEnds()
{
    std::pair<Base::Vector3d, Base::Vector3d> result;
    TopoDS_Wire lineWire = makeSectionLineWire();
    if (lineWire.IsNull()) {
        return result;
    }

    auto ends = getWireEnds(lineWire);
    Base::Vector3d first = ends.first;
    Base::Vector3d last = ends.second;

    auto* baseDvp = freecad_cast<DrawViewPart*>(BaseView.getValue());
    if (baseDvp) {
        first = baseDvp->projectPoint(first);
        last = baseDvp->projectPoint(last);
    }
    result.first = first;
    result.second = last;
    return result;
}

// get the directions of the section line arrows.
// the arrows on the section line are line of sight - from eye to preserved material.  In a simple section,
// this is opposite to the section normal.  In the complex section, we need a perpendicular direction most
// opposite to the SectionNormal.
std::pair<Base::Vector3d, Base::Vector3d> DrawComplexSection::sectionArrowDirs()
{
    std::pair<Base::Vector3d, Base::Vector3d> result;
    App::DocumentObject* toolObj = CuttingToolWireObject.getValue();
    TopoDS_Wire profileWire = makeProfileWire(toolObj);
    if (profileWire.IsNull()) {
        return result;
    }

    auto lineOfSight = SectionNormal.getValue() * -1;
    lineOfSight.Normalize();

    auto referenceAxis = getReferenceAxis();

    auto uSectionNormal = SectionNormal.getValue();
    uSectionNormal.Normalize();
    auto segmentViewDirections = getSegmentViewDirections(profileWire, uSectionNormal, referenceAxis);
    if (segmentViewDirections.empty()) {
        throw Base::RuntimeError("A complex section failed to create profile segment view directions");
    }
    Base::Vector3d firstArrowDir = segmentViewDirections.front().second;
    Base::Vector3d lastArrowDir = segmentViewDirections.back().second;

    return { firstArrowDir, lastArrowDir };
}


//! find an axis for measuring rotation vs the line of sight
Base::Vector3d DrawComplexSection::getReferenceAxis()
{
    auto csDir = getBaseDVP()->getProjectionCS().Direction();
    auto rawDirection = Base::convertTo<Base::Vector3d>(csDir);
    rawDirection.Normalize();
    return DU::closestBasisOriented(rawDirection);
}


//make a wire suitable for projection on a base view
TopoDS_Wire DrawComplexSection::makeSectionLineWire()
{
    TopoDS_Wire lineWire;
    App::DocumentObject* toolObj = CuttingToolWireObject.getValue();
    DrawViewPart* baseDvp = freecad_cast<DrawViewPart*>(BaseView.getValue());
    if (baseDvp) {
        TopoDS_Shape toolShape = Part::Feature::getShape(toolObj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
        if (toolShape.IsNull()) {
            // CuttingToolWireObject is likely still restoring and has no shape yet
            return {};
        }
        Base::Vector3d centroid = baseDvp->getCurrentCentroid();
        TopoDS_Shape sTrans =
            ShapeUtils::ShapeUtils::moveShape(toolShape, centroid * -1.0);
        TopoDS_Shape sScaled = ShapeUtils::scaleShape(sTrans, baseDvp->getScale());
        //we don't mirror the scaled shape here as it will be mirrored by the projection

        if (sScaled.ShapeType() == TopAbs_WIRE) {
            lineWire = makeNoseToTailWire(TopoDS::Wire(sScaled));
        }
        else if (sScaled.ShapeType() == TopAbs_EDGE) {
            TopoDS_Edge edge = TopoDS::Edge(sScaled);
            lineWire = BRepBuilderAPI_MakeWire(edge).Wire();
        }
        else {
            //probably can't happen as cut profile has been checked before this
            Base::Console().message("DCS::makeSectionLineGeometry - profile is type: %d\n",
                                    static_cast<int>(sScaled.ShapeType()));
            return TopoDS_Wire();
        }
    }
    return lineWire;
}

//find the points where the section line changes direction, and the direction
//of the profile before and after the point
ChangePointVector DrawComplexSection::getChangePointsFromSectionLine()
{
    ChangePointVector result;
    std::vector<gp_Pnt> allPoints;
    DrawViewPart* baseDvp = freecad_cast<DrawViewPart*>(BaseView.getValue());
    if (baseDvp) {
        TopoDS_Wire lineWire = makeSectionLineWire();
        TopoDS_Shape projectedWire =
            GeometryObject::projectSimpleShape(lineWire, baseDvp->getProjectionCS());
        if (projectedWire.IsNull()) {
            return result;
        }
        //get UNIQUE points along the projected profile
        TopExp_Explorer expVertex(projectedWire, TopAbs_VERTEX);
        gp_Pnt previousPoint(Precision::Infinite(), Precision::Infinite(), Precision::Infinite());
        for (; expVertex.More(); expVertex.Next()) {
            TopoDS_Vertex vert = TopoDS::Vertex(expVertex.Current());
            gp_Pnt gPoint = BRep_Tool::Pnt(vert);
            if (gPoint.IsEqual(previousPoint, 2.0 * EWTOLERANCE)) {
                continue;
            }
            allPoints.push_back(gPoint);
            previousPoint = gPoint;
        }

        //make the intermediate marks
        for (size_t iPoint = 1; iPoint < allPoints.size() - 1; iPoint++) {
            gp_Pnt location = allPoints.at(iPoint);
            gp_Dir preDir = gp_Dir(allPoints.at(iPoint - 1).XYZ() - allPoints.at(iPoint).XYZ());
            gp_Dir postDir = gp_Dir(allPoints.at(iPoint + 1).XYZ() - allPoints.at(iPoint).XYZ());
            ChangePoint point(location, preDir, postDir);
            result.push_back(point);
        }

        //make start and end marks
        gp_Pnt location0 = allPoints.at(0);
        gp_Pnt location1 = allPoints.at(1);
        gp_Dir postDir = gp_Dir(location1.XYZ() - location0.XYZ());
        gp_Dir preDir = postDir.Reversed();
        ChangePoint startPoint(location0, preDir, postDir);
        result.push_back(startPoint);
        location0 = allPoints.at(allPoints.size() - 1);
        location1 = allPoints.at(allPoints.size() - 2);
        preDir = gp_Dir(location0.XYZ() - location1.XYZ());
        postDir = preDir.Reversed();
        ChangePoint endPoint(location0, preDir, postDir);
        result.push_back(endPoint);
    }
    return result;
}

gp_Ax2 DrawComplexSection::getCSFromBase(const std::string& sectionName) const
{
    App::DocumentObject* base = BaseView.getValue();
    if (!base
        || !base->isDerivedFrom<TechDraw::DrawViewPart>()) {//is second clause necessary?
        //if this DCS does not have a baseView, we must use the existing SectionCS
        return getSectionCS();
    }
    return DrawViewSection::getCSFromBase(sectionName);
}

// check for profile segments that are almost, but not quite in the same direction
// as the section normal direction.  this often indicates a problem with the direction
// being slightly wrong.  see https://forum.freecad.org/viewtopic.php?t=79017&sid=612a62a60f5db955ee071a7aaa362dbb
bool DrawComplexSection::validateOffsetProfile(TopoDS_Wire profile, Base::Vector3d direction, double angleThresholdDeg) const
{
    constexpr double HalfCircleDegrees{180.0};
    double angleThresholdRad = angleThresholdDeg * std::numbers::pi / HalfCircleDegrees;
    TopExp_Explorer explEdges(profile, TopAbs_EDGE);
    for (; explEdges.More(); explEdges.Next()) {
        std::pair<Base::Vector3d, Base::Vector3d> segmentEnds = getSegmentEnds(TopoDS::Edge(explEdges.Current()));
        Base::Vector3d segmentDir = segmentEnds.second - segmentEnds.first;
        double angleRad = segmentDir.GetAngle(direction);
        if (angleRad < angleThresholdRad &&
            angleRad > 0.0) {
            // profile segment is slightly skewed. possible bad SectionNormal?
            Base::Console().warning("%s profile is slightly skewed. Check SectionNormal low decimal places\n",
                                    getNameInDocument());
            return false;
        }
    }
    return true;
}


// next two methods could be templated (??) to handle edge/wire
std::pair<Base::Vector3d, Base::Vector3d>
                            DrawComplexSection::getSegmentEnds(const TopoDS_Edge& segment)
{
    TopoDS_Vertex tvFirst;
    TopoDS_Vertex tvLast;
    TopExp::Vertices(segment, tvFirst, tvLast);
    gp_Pnt gpFirst = BRep_Tool::Pnt(tvFirst);
    gp_Pnt gpLast = BRep_Tool::Pnt(tvLast);
    std::pair<Base::Vector3d, Base::Vector3d> result;
    result.first = Base::convertTo<Base::Vector3d>(gpFirst);
    result.second = Base::convertTo<Base::Vector3d>(gpLast);
    return result;
}

std::pair<Base::Vector3d, Base::Vector3d>
                            DrawComplexSection::getWireEnds(const TopoDS_Wire& wire)
{
    TopoDS_Vertex tvFirst;
    TopoDS_Vertex tvLast;
    TopExp::Vertices(wire, tvFirst, tvLast);
    gp_Pnt gpFirst = BRep_Tool::Pnt(tvFirst);
    gp_Pnt gpLast = BRep_Tool::Pnt(tvLast);
    std::pair<Base::Vector3d, Base::Vector3d> result;
    result.first = Base::convertTo<Base::Vector3d>(gpFirst);
    result.second = Base::convertTo<Base::Vector3d>(gpLast);
    return result;
}


gp_Vec DrawComplexSection::projectVector(const gp_Vec& vec, gp_Ax2 sectionCS)
{
    HLRAlgo_Projector projector(sectionCS);
    gp_Pnt2d prjPoint;
    projector.Project(gp_Pnt(vec.XYZ()), prjPoint);
    return {prjPoint.X(), prjPoint.Y(), 0};
}

//simple projection of a 3d vector onto the paper space
gp_Vec DrawComplexSection::projectVector(const gp_Vec& vec) const
{
    return projectVector(vec, getProjectionCS());
}

//get the "effective" (flattened paper plane) section plane for Aligned and
//the regular sectionPlane for Offset.
gp_Pln DrawComplexSection::getSectionPlane() const
{
    if (ProjectionStrategy.getValue() == 0) {
        //Offset. Use regular section behaviour
        return DrawViewSection::getSectionPlane();
    }

    //"Aligned" projection (Aligned Section)
    //this is the same as DVS::getSectionPlane except that the plane origin is not the SectionOrigin
    Base::Vector3d vSectionNormal = SectionNormal.getValue();
    gp_Dir gSectionNormal(vSectionNormal.x, vSectionNormal.y, vSectionNormal.z);
    gp_Pnt gOrigin(0.0, 0.0, 0.0);
    gp_Ax3 gPlaneCS(gOrigin, gSectionNormal);

    return {gPlaneCS};
}

bool DrawComplexSection::isBaseValid() const
{
    App::DocumentObject* base = BaseView.getValue();
    if (!base) {
        //complex section is not based on an existing DVP
        return true;
    }
    if (!base->isDerivedFrom<TechDraw::DrawViewPart>()) {
        //this is probably an error somewhere. the valid options are base = a DVP,
        //or no base
        return false;
    }
    //have a base and it is a DVP
    return true;
}

//if the profile is not nicely positioned within the vertical span of the shape, we might not overlap
//the shape after extrusion.  As long as the profile is within the extent of the shape in the
//extrude direction we should be ok. the extrude direction has to be perpendicular to the profile and SectionNormal
bool DrawComplexSection::validateProfilePosition(const TopoDS_Wire& profileWire, const gp_Ax2& sectionCS) const
{
    auto wireEnds = getWireEnds(profileWire);
    auto gpFirst = Base::convertTo<gp_Pnt>(wireEnds.first);
    gp_Vec gProfileVector = makeProfileVector(profileWire);

    //since bounding boxes are aligned with the cardinal directions, we need to find
    //the appropriate direction to use when validating the profile position
    gp_Vec gSectionVector = getSectionCS().Direction().Reversed();
    gp_Vec gExtrudeVector = gSectionVector.Crossed(gProfileVector);
    Base::Vector3d vClosestBasis = DrawUtil::closestBasis(gp_Dir(gExtrudeVector), sectionCS);
    auto gClosestBasis = gp_Dir(vClosestBasis.x, vClosestBasis.y, vClosestBasis.z);

    Bnd_Box shapeBox;
    shapeBox.SetGap(0.0);
    BRepBndLib::AddOptimal(m_saveShape, shapeBox);
    double xMin = 0, xMax = 0, yMin = 0, yMax = 0, zMin = 0, zMax = 0;   //NOLINT
    shapeBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    double spanLow = xMin;
    double spanHigh = xMax;
    double spanCheck = gpFirst.X();
    if (gClosestBasis.IsParallel(sectionCS.YDirection(), Precision::Angular())) {
        spanLow = yMin;
        spanHigh = yMax;
        spanCheck = gpFirst.Y();
    }
    else if (gClosestBasis.IsParallel(sectionCS.Direction(), Precision::Angular())) {
        spanLow = zMin;
        spanHigh = zMax;
        spanCheck = gpFirst.Z();
    }

    if (spanLow > spanCheck || spanHigh < spanCheck) {
        //profile is not within span of shape
        return false;
    }
    //profile is within span of shape
    return true;
}

bool DrawComplexSection::showSegment(gp_Dir segmentNormal) const
{
    if (ProjectionStrategy.getValue() < 2) {
        //Offset or Aligned are always true
        return true;
    }

    Base::Vector3d vSectionNormal = SectionNormal.getValue();
    gp_Dir gSectionNormal(vSectionNormal.x, vSectionNormal.y, vSectionNormal.z);
    //segment normal is perpendicular to section normal, so segment is parallel to section normal,
    //and for ProjectionStrategy "NoParallel", we don't display these segments.
    return !DU::fpCompare(fabs(gSectionNormal.Dot(segmentNormal)), 0.0);
}

//Can we make a ComplexSection using this profile and sectionNormal?
bool DrawComplexSection::canBuild(gp_Ax2 sectionCS, App::DocumentObject* profileObject)
{
    if (!isProfileObject(profileObject)) {
        return false;
    }

    TopoDS_Shape shape = Part::Feature::getShape(profileObject, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
    if (BRep_Tool::IsClosed(shape)) {
        //closed profiles don't have a profile vector but should always make a section?
        return true;
    }

    gp_Vec gProfileVec = makeProfileVector(makeProfileWire(profileObject));
    double dot = fabs(gProfileVec.Dot(sectionCS.Direction()));
    return !DU::fpCompare(dot, 1.0, EWTOLERANCE);
}

// general purpose geometry methods

//make a "face" (not necessarily a TopoDS_Face since the extrusion of a wire is a shell)
//from a single open wire by displacing the wire extruding it
TopoDS_Shape DrawComplexSection::extrudeWireToFace(TopoDS_Wire& wire, gp_Dir extrudeDir,
                                                   double extrudeDist)
{
    BRepPrimAPI_MakePrism mkPrism(wire, gp_Vec(extrudeDir) * extrudeDist * 2);

    return mkPrism.Shape();
}

//returns the normal of the face to be extruded into a cutting tool
//the face is expected to be planar
gp_Dir DrawComplexSection::getFaceNormal(TopoDS_Face& face)
{
    BRepAdaptor_Surface adapt(face);
    double uParmFirst = adapt.FirstUParameter();
    double uParmLast = adapt.LastUParameter();
    double vParmFirst = adapt.FirstVParameter();
    double vParmLast = adapt.LastVParameter();
    double uMid = (uParmFirst + uParmLast) / 2;
    double vMid = (vParmFirst + vParmLast) / 2;

    constexpr double PropTolerance{0.01};
    BRepLProp_SLProps prop(adapt, uMid, vMid, 1, PropTolerance);
    gp_Dir normalDir(0.0, 0.0, 1.0);//default
    if (prop.IsNormalDefined()) {
        normalDir = prop.Normal();
    }
    return normalDir;
}

bool DrawComplexSection::boxesIntersect(TopoDS_Face& face, TopoDS_Shape& shape)
{
    constexpr double OverlapTolerance{0.1};
    Bnd_Box box0;
    Bnd_Box box1;
    BRepBndLib::Add(face, box0);
    box0.SetGap(OverlapTolerance);//generous
    BRepBndLib::Add(shape, box1);
    box1.SetGap(OverlapTolerance);
    return !box0.IsOut(box1);
}

TopoDS_Shape DrawComplexSection::shapeShapeIntersect(const TopoDS_Shape& shape0,
                                                     const TopoDS_Shape& shape1)
{
    FCBRepAlgoAPI_Common anOp;
    anOp.SetFuzzyValue(EWTOLERANCE);
    TopTools_ListOfShape anArg1;
    TopTools_ListOfShape anArg2;
    anArg1.Append(shape0);
    anArg2.Append(shape1);
    anOp.SetArguments(anArg1);
    anOp.SetTools(anArg2);
    anOp.Build();
    TopoDS_Shape result = anOp.Shape();//always a compound
    if (isTrulyEmpty(result)) {
        return {};
    }
    return result;
}

//find all the intersecting regions of face and shape
std::vector<TopoDS_Face> DrawComplexSection::faceShapeIntersect(const TopoDS_Face& face,
                                                                const TopoDS_Shape& shape)
{
    TopoDS_Shape intersect = shapeShapeIntersect(face, shape);
    if (intersect.IsNull()) {
        return {};
    }
    std::vector<TopoDS_Face> intersectFaceList;
    TopExp_Explorer expFaces(intersect, TopAbs_FACE);
    for (int i = 1; expFaces.More(); expFaces.Next(), i++) {
        intersectFaceList.push_back(TopoDS::Face(expFaces.Current()));
    }
    return intersectFaceList;
}

//ensure that the edges in the output wire are in nose to tail order
TopoDS_Wire DrawComplexSection::makeNoseToTailWire(const TopoDS_Wire& inWire)
{
    if (inWire.IsNull()) {
        return inWire;
    }

    std::list<TopoDS_Edge> inList;
    TopExp_Explorer expEdges(inWire, TopAbs_EDGE);
    for (; expEdges.More(); expEdges.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(expEdges.Current());
        inList.push_back(edge);
    }

    std::list<TopoDS_Edge> sortedList;
    if (inList.empty() || inList.size() == 1) {
        return inWire;
    }

    sortedList = DrawUtil::sort_Edges(EWTOLERANCE, inList);
    BRepBuilderAPI_MakeWire mkWire;
    for (auto& edge : sortedList) {
        mkWire.Add(edge);
    }
    return mkWire.Wire();
}

//static
bool DrawComplexSection::isProfileObject(App::DocumentObject* obj)
{
    //if the object's shape is a wire or an edge, then it can be a profile object
    TopoDS_Shape shape = Part::Feature::getShape(obj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
    if (shape.IsNull()) {
        return false;
    }
    if (shape.ShapeType() == TopAbs_WIRE || shape.ShapeType() == TopAbs_EDGE) {
        return true;
    }
    //don't know what this is, but it isn't suitable as a profile
    return false;
}

bool DrawComplexSection::isMultiSegmentProfile(App::DocumentObject* obj)
{
    //if the object's shape is a wire or an edge, then it can be a profile object
    TopoDS_Shape shape = Part::Feature::getShape(obj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
    if (shape.IsNull()) {
        return false;
    }
    if (shape.ShapeType() == TopAbs_EDGE) {
        //only have 1 edge, can't be multisegment;
        return false;
    }
    if (shape.ShapeType() == TopAbs_WIRE) {
        std::vector<TopoDS_Edge> edgesInWire;
        TopExp_Explorer expEdges(shape, TopAbs_EDGE);
        for (; expEdges.More(); expEdges.Next()) {
            TopoDS_Edge edge = TopoDS::Edge(expEdges.Current());
            BRepAdaptor_Curve adapt(edge);
            if (adapt.GetType() == GeomAbs_Line) {
                edgesInWire.push_back(edge);
            }
        }
        if (edgesInWire.size() > 1) {
            return true;
        }
    }
    return false;
}

//check if the profile has curves in it
bool DrawComplexSection::isLinearProfile(App::DocumentObject* obj)
{
    TopoDS_Shape shape = Part::Feature::getShape(obj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
    if (shape.IsNull()) {
        return false;
    }
    if (shape.ShapeType() == TopAbs_EDGE) {
        //only have 1 edge
        TopoDS_Edge edge = TopoDS::Edge(shape);
        BRepAdaptor_Curve adapt(edge);
        return (adapt.GetType() == GeomAbs_Line);
    }

    if (shape.ShapeType() == TopAbs_WIRE) {
        std::vector<TopoDS_Edge> edgesInWire;
        TopExp_Explorer expEdges(shape, TopAbs_EDGE);
        for (; expEdges.More(); expEdges.Next()) {
            TopoDS_Edge edge = TopoDS::Edge(expEdges.Current());
            BRepAdaptor_Curve adapt(edge);
            if (adapt.GetType() != GeomAbs_Line) {
                return false;
            }
        }
        //all the edges in the wire are lines
        return true;
    }

    //this shouldn't happen
    return false;
}

//a compound with no content is not considered IsNull by OCC.  A more thorough check
//is required.
//https://dev.opencascade.org/content/compound-empty
bool DrawComplexSection::isTrulyEmpty(const TopoDS_Shape& inShape)
{
    bool hasContent = !inShape.IsNull() && TopoDS_Iterator(inShape).More();
    return !hasContent;
}


// this is a slightly different closed profile than getSegmentViewDirections uses. Should we use that
// here??
std::vector<Base::Vector3d>
            DrawComplexSection::getPointsForClosingProfile(const TopoDS_Wire& profileWire, double dMax)
{
    auto ends = getWireEnds(profileWire);
    auto firstPoint = ends.first;
    auto lastPoint = ends.second;
    auto midPoint = (firstPoint + lastPoint) / 2;       // midpoint of profile vector

    auto arrows = sectionArrowDirs();
    auto pseudoSectionNormal = ((arrows.first + arrows.second) / 2) * -1;
    pseudoSectionNormal.Normalize();
    auto extraPoint = midPoint + pseudoSectionNormal * dMax * 2;

    std::vector<Base::Vector3d> points;
    auto pointFromLast = lastPoint + (arrows.second * -1) * dMax;
    auto pointToFirst = firstPoint + (arrows.first * -1) * dMax;
    points.emplace_back(lastPoint);
    points.emplace_back(pointFromLast);
    points.emplace_back(extraPoint);
    points.emplace_back(pointToFirst);
    points.emplace_back(firstPoint);
    return points;
}

TopoDS_Shape DrawComplexSection::removeEmptyShapes(const TopoDS_Shape& roughTool)
{
    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    TopExp_Explorer expSolids(roughTool, TopAbs_SOLID);
    for (; expSolids.More(); expSolids.Next()) {
        TopoDS_Solid solid = TopoDS::Solid(expSolids.Current());
        GProp_GProps gprops;
        BRepGProp::VolumeProperties(solid, gprops);
        double volume = gprops.Mass();
        if (volume > EWTOLERANCE) {
            builder.Add(comp, solid);
        }
    }
    return comp;
}

//! reversers determine if the cut pieces are arranged left to right or right to left (down to up/up to down)
//! returns true if the profile vector is vertical.
bool DrawComplexSection::getReversers(const gp_Vec& gProfileVec, double& horizReverser, double& verticalReverser)
{
    bool isProfileVertical = true;
    auto sectionCS = getSectionCS();
    auto sectionCSX = sectionCS.XDirection();
    auto sectionCSY = sectionCS.YDirection();
    auto verticalDot = gProfileVec.Dot(sectionCSY);
    if (DU::fpCompare(fabs(verticalDot), 0, EWTOLERANCE)) {
        //profile is +/- normal to Y.
        isProfileVertical = false;
    }

    horizReverser = 1.0;     //profile vector points to right, so we move to right
    if (gProfileVec.Dot(sectionCSX) < 0.0) {
        //profileVec does not point towards stdX (right in paper space)
        horizReverser = -1.0;
    }

    verticalReverser = 1.0;//profile vector points to top, so we will move pieces upwards
    if (gProfileVec.Dot(sectionCSY) < 0.0) {
        //profileVec does not point towards stdY (up in paper space)
        verticalReverser = -1.0;
    }

    return isProfileVertical;
}


//! align a copy of the piece with OXYZ so we can use bounding box to get
//! width, depth, height of the piece. We copy the piece so the transformation
//! does not affect the original.
AlignedSizeResponse DrawComplexSection::getAlignedSize(const TopoDS_Shape& pieceRotated,
                                                  int iPiece) const
{
        gp_Ax3 stdCS;                                 //OXYZ
        BRepBuilderAPI_Copy BuilderPieceCopy(pieceRotated);
        TopoDS_Shape copyPieceRotatedShape = BuilderPieceCopy.Shape();
        gp_Trsf xPieceAlign;
        xPieceAlign.SetTransformation(stdCS, getProjectionCS());
        BRepBuilderAPI_Transform mkTransAlign(copyPieceRotatedShape, xPieceAlign);
        TopoDS_Shape pieceAligned = mkTransAlign.Shape();
        // we may have shifted our piece off center, so we better recenter here
        gp_Trsf xPieceRecenter;
        gp_Vec rotatedCentroid = gp_Vec(ShapeUtils::findCentroid(pieceAligned).XYZ());
        xPieceRecenter.SetTranslation(rotatedCentroid * -1.0);
        BRepBuilderAPI_Transform mkTransRecenter(pieceAligned, xPieceRecenter, true);
        pieceAligned = mkTransRecenter.Shape();

        if (debugSection()) {
            stringstream ss;
            ss << "DCSDpieceAligned" << iPiece << ".brep";
            BRepTools::Write(pieceAligned, ss.str().c_str());//debug
            ss.clear();
            ss.str(std::string());
        }
        Bnd_Box shapeBox;
        shapeBox.SetGap(0.0);
        BRepBndLib::AddOptimal(pieceAligned, shapeBox);
        double xMin = 0, xMax = 0, yMin = 0, yMax = 0, zMin = 0, zMax = 0;  //NOLINT
        shapeBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        double pieceXSize(xMax - xMin);
        double pieceYSize(yMax - yMin);
        double pieceZSize(zMax - zMin);
        Base::Vector3d pieceSize{pieceXSize, pieceYSize, pieceZSize};
        return {pieceAligned, pieceSize, zMax};
}

//! cut the rawShape with a tool derived from the segmentFace and align the result with the
//! page plane.  Also supplies the piece size.
TopoDS_Shape DrawComplexSection::cutAndRotatePiece(const TopoDS_Shape& rawShape,
                                                      const TopoDS_Face& segmentFace,
                                                      int iPiece,
                                                      Base::Vector3d uOrientedSegmentNormal,
                                                      Base::Vector3d uRotateAxis,
                                                      double& pieceVertical)
{
    auto segmentNormal = Base::convertTo<gp_Vec>(uOrientedSegmentNormal);
    auto rotateAxis = Base::convertTo<gp_Vec>(uRotateAxis);
    gp_Vec extrudeDir = segmentNormal * m_shapeSize;
    BRepPrimAPI_MakePrism mkPrism(segmentFace, extrudeDir);
    TopoDS_Shape segmentTool = mkPrism.Shape();
    TopoDS_Shape intersect = shapeShapeIntersect(segmentTool, rawShape);
    if (intersect.IsNull()) {
        return {};
    }
    if (debugSection()) {
        stringstream ss;
        ss << "DCSAintersect" << iPiece << ".brep";
        BRepTools::Write(intersect, ss.str().c_str());//debug
        ss.clear();
        ss.str(std::string());
    }

    // move intersection shape to the origin so we can rotate it without worrying about
    // center of rotation.
    gp_Trsf xPieceCenter;
    gp_Vec pieceCentroid = gp_Vec(ShapeUtils::findCentroid(intersect).XYZ());

    // save the amount we moved this piece in the vertical direction so we can
    // put it back in the right place later
    gp_Vec maskedVertical = DU::maskDirection(pieceCentroid, rotateAxis);
    maskedVertical = pieceCentroid - maskedVertical;
    pieceVertical = maskedVertical.X() + maskedVertical.Y() + maskedVertical.Z();

    xPieceCenter.SetTranslation(pieceCentroid * -1.0);
    BRepBuilderAPI_Transform mkTransXLate(intersect, xPieceCenter, true);
    TopoDS_Shape pieceCentered = mkTransXLate.Shape();
    if (debugSection()) {
        stringstream ss;
        ss << "DCSBpieceCentered" << iPiece << ".brep";
        BRepTools::Write(pieceCentered, ss.str().c_str());//debug
        ss.clear();
        ss.str(std::string());
    }

    //rotate the intersection so interesting face is aligned with what will
    // become the paper plane.
    double faceAngle =
        gp_Vec(getSectionCS().Direction().Reversed()).AngleWithRef(segmentNormal, rotateAxis);
    gp_Ax1 faceAxis(gp_Pnt(0.0, 0.0, 0.0), rotateAxis);
    gp_Ax3 stdCS;
    gp_Ax3 pieceCS;
    pieceCS.Rotate(faceAxis, faceAngle);
    gp_Trsf xPieceRotate;
    xPieceRotate.SetTransformation(stdCS, pieceCS);
    BRepBuilderAPI_Transform mkTransRotate(pieceCentered, xPieceRotate, true);
    TopoDS_Shape pieceRotated = mkTransRotate.Shape();

    return pieceRotated;
}

TopoDS_Shape DrawComplexSection::movePieceToPaperPlane(const TopoDS_Shape& piece, double sizeMax)
{
    //now we need to move the piece so that the interesting face is coincident
    //with the paper plane (stdXY).  This will be a move along stdZ by -zMax.
    gp_Vec toPaperPlane = gp::OZ().Direction().XYZ() * sizeMax * -1.0;
    gp_Trsf xPieceToPlane;
    xPieceToPlane.SetTranslation(toPaperPlane);
    BRepBuilderAPI_Transform mkTransDisplace(piece, xPieceToPlane, true);
    TopoDS_Shape pieceToPlane = mkTransDisplace.Shape();

    // piece is on the paper plane, with piece centroid at the origin
    return pieceToPlane;
}

//! move the piece to its position across the page (X for a left right profile)
TopoDS_Shape DrawComplexSection::distributePiece(const TopoDS_Shape& piece,
                                                 double pieceSizeInDirection,
                                                 double verticalDisplace,
                                                 const gp_Vec& alignmentAxis,
                                                 const gp_Vec& gMovementVector,
                                                 double cursorPosition)
{
    double pieceTotalDistanceToMove = cursorPosition + pieceSizeInDirection / 2;
    gp_Vec alignmentVector = alignmentAxis * verticalDisplace * -1;
    gp_Vec netDisplacementVector = gMovementVector * pieceTotalDistanceToMove + alignmentVector;

    gp_Trsf xPieceDistribute;
    xPieceDistribute.SetTranslation(netDisplacementVector);
    BRepBuilderAPI_Transform mkTransDistribute(piece, xPieceDistribute, true);
    auto distributedPiece = mkTransDistribute.Shape();

    return distributedPiece;
}


// ?? are these in the correct order??  profile wires are in nose to tail order
std::vector<TopoDS_Edge> DrawComplexSection::getUniqueEdges(const TopoDS_Wire& wireIn)
{
    std::vector<TopoDS_Edge> ret;
    TopTools_IndexedMapOfShape shapeMap;
    TopExp_Explorer Ex(wireIn, TopAbs_EDGE);
    while (Ex.More()) {
        shapeMap.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= shapeMap.Extent(); k++) {
        const TopoDS_Shape& shape = shapeMap(k);
        auto edge = TopoDS::Edge(shape);
        ret.push_back(edge);
    }

    return ret;
}

//! Find the correct view directions for the profile segments by making a face from the profile and
//! extruding it into a solid, then examining the faces of the solid.
std::vector<std::pair<int, Base::Vector3d>> DrawComplexSection::getSegmentViewDirections(const TopoDS_Wire& profileWire,
                                           Base::Vector3d sectionNormal,
                                           Base::Vector3d referenceAxis)  const
{
    auto edgesAll = getUniqueEdges(profileWire);
    if (edgesAll.empty()) {
        // this is bad!
        throw Base::RuntimeError("Complex section: profile wire has no edges.");
    }

    if (!checkSectionCS()) {
        // results will likely be incorrect
        // this message will show for every recompute of the complex section.
        Base::Console().warning("Coordinate system for ComplexSection is invalid. Check SectionNormal, Direction or XDirection.\n");
    }

    auto profileVector = Base::convertTo<Base::Vector3d>(makeProfileVector(profileWire));
    auto parallelDot = profileVector.Dot(sectionNormal);
    if (DU::fpCompare(std::fabs(parallelDot), 1, EWTOLERANCE)) {
        Base::Console().warning("Section normal is parallel to profile vector. Results may be incorrect.\n");
    }
    auto profilePlanWire = closeProfile(profileWire, sectionNormal, m_shapeSize);

    auto profileSolid = profileToSolid(profilePlanWire, referenceAxis, m_shapeSize);
    if (debugSection()) {
        BRepTools::Write(profileSolid, "DCSProfileSolid.brep");//debug
    }

    std::vector<Base::Vector3d> profileNormals;
    std::vector<std::pair<int, Base::Vector3d>> normalKV;
    TopExp_Explorer expFaces(profileSolid, TopAbs_FACE);
    // no guarantee of order from TopExp_Explorer??  Need to match faces to the profile segment that
    // generated it?
    for (int iFace = 0; expFaces.More(); expFaces.Next(), iFace++) {
        auto shape = expFaces.Current();
        auto face = TopoDS::Face(shape);
        auto normal = Base::convertTo<Base::Vector3d>(getFaceNormal(face));
        if (face.Orientation() == TopAbs_FORWARD) {
            // face on solid with Forward orientation will have a normal that points out of the
            // solid.  With Reverse orientation the normal will point into the solid. We want the
            // direction into the solid.
            // "Face normal shows where body material is – it lies ‘behind' the face. In a correct solid body all face normals go ‘outward' (see below):"
            // https://opencascade.blogspot.com/2009/02/continued.html
            normal *= -1;
        }
        auto topOrBottomDot = std::fabs(normal.Dot(referenceAxis));
        if (DU::fpCompare(topOrBottomDot, 1, EWTOLERANCE)) {
            continue;
        }
        BRepAdaptor_Surface adaptSurface(face);
        auto surfaceType = adaptSurface.GetType();
        if (surfaceType != GeomAbs_SurfaceType::GeomAbs_Plane) {
            continue;
        }
        // this is an interesting face, get the normal and edge
        int iSegment{0};
        for (auto& segment : edgesAll) {
            if (faceContainsEndpoints(segment, face)) {
                std::pair<int, Base::Vector3d> newEntry;
                newEntry.first = iSegment;
                newEntry.second = normal;
                normalKV.push_back(newEntry);
                break;
            }
            iSegment++;
         }
    }
    std::sort(normalKV.begin(), normalKV.end(), DrawComplexSection::normalLess);
    return normalKV;
}

//! true if the endpoints of edgeToMatch are vertexes of faceToSearch
bool DrawComplexSection::faceContainsEndpoints(const TopoDS_Edge& edgeToMatch, const TopoDS_Face& faceToSearch)
{
    bool matchedFirst{false};
    bool matchedLast{false};

    std::pair<Base::Vector3d, Base::Vector3d> edgeEnds = getSegmentEnds(edgeToMatch);
    TopExp_Explorer expVerts(faceToSearch, TopAbs_VERTEX);
    int iFace = 0;
    for (; expVerts.More(); expVerts.Next(), iFace++) {
        auto shape = expVerts.Current();
        auto vertex = TopoDS::Vertex(shape);
        auto point = Base::convertTo<Base::Vector3d>(BRep_Tool::Pnt(vertex));
        if (point.IsEqual(edgeEnds.first, EWTOLERANCE)) {
            matchedFirst = true;
        }
        if (point.IsEqual(edgeEnds.second, EWTOLERANCE)) {
            matchedLast = true;
        }
        if (matchedFirst && matchedLast) {
            break;
        }
    }
    return matchedFirst && matchedLast;
}


//! forms a closed wire by connecting the end points of the profile wire with an arc.
TopoDS_Wire DrawComplexSection::closeProfile(const TopoDS_Wire& profileWire,
                                             Base::Vector3d sectionNormal,
                                             double dMax)
{
    auto pvEnds = getWireEnds(profileWire);
    auto firstPWPoint = pvEnds.first;
    auto lastPWPoint = pvEnds.second;
    auto midPWPoint = (firstPWPoint + lastPWPoint) / 2;
    auto SNPoint = sectionNormal * dMax;
    auto awayDirection = midPWPoint - SNPoint;
    awayDirection.Normalize();
    auto radius = (midPWPoint - lastPWPoint).Length();
    auto pointOnArc = midPWPoint + awayDirection * radius;

    GC_MakeArcOfCircle mkArc(Base::convertTo<gp_Pnt>(lastPWPoint),
                             Base::convertTo<gp_Pnt>(pointOnArc),
                             Base::convertTo<gp_Pnt>(firstPWPoint));
    auto circleArc = mkArc.Value();
    if (!mkArc.IsDone()) {
        throw Base::RuntimeError("Complex section failed to create arc");
    }
    auto circleEdge = BRepBuilderAPI_MakeEdge(circleArc);

    BRepBuilderAPI_MakeWire mkWire(profileWire);
    mkWire.Add(circleEdge);
    return mkWire.Wire();
}


//! extrudes a face made from a wire along the reference axis
TopoDS_Shape DrawComplexSection::profileToSolid(const TopoDS_Wire& closedProfileWire,
                                                Base::Vector3d referenceAxis,
                                                double dMax)
{
    BRepBuilderAPI_MakeFace mkFace(closedProfileWire);
    if (!mkFace.IsDone()) {
        throw Base::RuntimeError("Complex section could not create face from closed profile");
    }

    auto extrudeVector = referenceAxis * dMax;

    BRepPrimAPI_MakePrism mkPrism(mkFace.Face(), Base::convertTo<gp_Vec>(extrudeVector));
    auto profileSolid = mkPrism.Shape();

     return profileSolid;
}


//! compare 2 normal entries for sorting on segment index
bool DrawComplexSection::normalLess(const std::pair<int, Base::Vector3d>& n1, const std::pair<int, Base::Vector3d>& n2)
{
    return n1.first < n2.first;
}


// Python Drawing feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawComplexSectionPython, TechDraw::DrawComplexSection)
template<> const char* TechDraw::DrawComplexSectionPython::getViewProviderName() const
{
    return "TechDrawGui::ViewProviderDrawingView";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawComplexSection>;
}// namespace App
