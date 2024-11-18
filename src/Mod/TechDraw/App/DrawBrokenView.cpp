// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2024 WandererFan <wandererfan@gmail.com>                *
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

//! DrawBrokenView produces a view of the Source shapes after a portion of the shapes
//! has been removed.

//! DrawBrokenView processing is essentially the same as DrawViewPart, except that the
//! Source shapes are cut and the cut pieces moved before the projection/hlr steps.
//!
//! Break points are defined by
//!     - a horizontal or vertical edge whose endpoints represent where the source
//!       shape should be cut, or,
//!     - a sketch containing 2 horizontal or vertical edges whose midpoints represent
//!       where the source shape should be cut.
//! Terminology:
//!     - "break direction" is the direction pieces will need to be moved along to
//!       close a break.  for edges, the break direction is parallel to the edge.
//!       for sketch based breaks, the break direction is perpendicular to the edges
//!       in the sketch.

#include "PreCompiled.h"

#ifndef _PreComp_
#include <BRepAdaptor_Curve.hxx>
#include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <QtConcurrentRun>
#include <ShapeAnalysis.hxx>
#include <ShapeFix_Shape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#endif

#include <App/Document.h>
#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>

#include <Mod/Part/App/PartFeature.h>

#include "DrawGeomHatch.h"
#include "DrawHatch.h"
#include "DrawUtil.h"
#include "DrawViewDetail.h"
#include "GeometryObject.h"
#include "ShapeExtractor.h"
#include "ShapeUtils.h"
// #include "Preferences.h"

#include "DrawBrokenView.h"
#include "DrawBrokenViewPy.h"

using namespace TechDraw;
using DU = DrawUtil;
using SU = ShapeUtils;

//===========================================================================
// DrawBrokenView
//===========================================================================

const char *DrawBrokenView::BreakTypeEnums[] = {
    QT_TRANSLATE_NOOP("DrawBrokenView", "None"),
    QT_TRANSLATE_NOOP("DrawBrokenView", "ZigZag"),
    QT_TRANSLATE_NOOP("DrawBrokenView", "Simple"),
    nullptr};
PROPERTY_SOURCE(TechDraw::DrawBrokenView, TechDraw::DrawViewPart)

DrawBrokenView::DrawBrokenView()
{
    static const char* sgroup = "Broken View";

    ADD_PROPERTY_TYPE(Breaks, (nullptr), sgroup, App::Prop_None, "Objects in the 3d view that define the start/end points and direction of breaks in this view.");
    Breaks.setScope(App::LinkScope::Global);
    Breaks.setAllowExternal(true);
    ADD_PROPERTY_TYPE(Gap,
                      (10.0),
                      sgroup,
                      App::Prop_None,
                      "The separation distance for breaks in this view (unscaled 3d length).");
}

DrawBrokenView::~DrawBrokenView()
{
}

short DrawBrokenView::mustExecute() const
{
    if (isRestoring()) {
        return TechDraw::DrawViewPart::mustExecute();
    }

    if (Breaks.isTouched() ||
        Gap.isTouched() ) {
        return 1;
    }

    return TechDraw::DrawViewPart::mustExecute();
}


App::DocumentObjectExecReturn* DrawBrokenView::execute()
{
    // Base::Console().Message("DBV::execute() - %s\n", getNameInDocument());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    if (waitingForResult()) {
        // don't start something new until the in-progress events complete
        return DrawView::execute();
    }

    TopoDS_Shape shape = getSourceShape();
    if (shape.IsNull()) {
        Base::Console().Message("DBV::execute - %s - Source shape is Null.\n", getNameInDocument());
        return DrawView::execute();
    }

    BRepBuilderAPI_Copy BuilderCopy(shape);
    TopoDS_Shape safeShape = BuilderCopy.Shape();

    m_unbrokenCenter = SU::findCentroidVec(safeShape, getProjectionCS());

    TopoDS_Shape brokenShape = breakShape(safeShape);
    m_compressedShape = compressShape(brokenShape);
    BRepTools::Write(brokenShape, "DBVbroken.brep");            //debug
    BRepTools::Write(m_compressedShape, "DBVcompressed.brep");

    partExec(m_compressedShape);

    return DrawView::execute();
}


//! applies the breaks to the input shape.  returns a compound of broken
//! pieces moved so they are separated by a distance of Gap.
TopoDS_Shape DrawBrokenView::breakShape(const TopoDS_Shape& shapeToBreak) const
{
    // Base::Console().Message("DBV::breakShape()\n");
    auto breaksAll = Breaks.getValues();
    TopoDS_Shape updatedShape = shapeToBreak;
    for (auto& item : breaksAll) {
        updatedShape = apply1Break(*item, updatedShape);
    }
    return updatedShape;
}


//! applies a single break to the input shape.  returns a compound of the
//! broken pieces.
TopoDS_Shape DrawBrokenView::apply1Break(const App::DocumentObject& breakObj, const TopoDS_Shape& inShape) const
{
    // Base::Console().Message("DBV::apply1Break()\n");
    auto breakPoints = breakPointsFromObj(breakObj);
    if (breakPoints.first.IsEqual(breakPoints.second, EWTOLERANCE)) {
        Base::Console().Message("DBV::apply1Break - break points are equal\n");
        return inShape;
    }

    auto breakDirection = DU::closestBasisOriented(directionFromObj(breakObj));
    breakDirection.Normalize();

    // make a halfspace that is positioned at the first breakpoint and extends
    // in the direction of the second point
    Base::Vector3d moveDir0 = breakPoints.second - breakPoints.first;
    moveDir0.Normalize();
    moveDir0 = DU::closestBasisOriented(moveDir0);
    auto halfSpace0 = makeHalfSpace(breakPoints.first, moveDir0, breakPoints.second);
    FCBRepAlgoAPI_Cut mkCut0(inShape, halfSpace0);
    if (!mkCut0.IsDone()) {
        Base::Console().Message("DBV::apply1Break - cut0 failed\n");
    }
    TopoDS_Shape cut0 = mkCut0.Shape();

    // make a halfspace that is positioned at the second breakpoint and extends
    // in the direction of the first point
    Base::Vector3d moveDir1 = breakPoints.first - breakPoints.second;
    moveDir1.Normalize();
    moveDir1 = DU::closestBasisOriented(moveDir1);
    auto halfSpace1 = makeHalfSpace(breakPoints.second, moveDir1, breakPoints.first);
    FCBRepAlgoAPI_Cut mkCut1(inShape, halfSpace1);
    if (!mkCut1.IsDone()) {
        Base::Console().Message("DBV::apply1Break - cut1 failed\n");
    }
    TopoDS_Shape cut1 = mkCut1.Shape();

    BRep_Builder builder;
    TopoDS_Compound result;
    builder.MakeCompound(result);
    builder.Add(result, cut0);
    builder.Add(result, cut1);

    // BRepTools::Write(cut0, "DBVcut0.brep");            //debug
    // BRepTools::Write(cut1, "DBVcut1.brep");

    return result;
}

//! compress the broken shape at the breaks
TopoDS_Shape DrawBrokenView::compressShape(const TopoDS_Shape& shapeToCompress) const
{
    // Base::Console().Message("DBV::compressShape()\n");
    TopoDS_Shape compressed = compressHorizontal(shapeToCompress);
    return compressVertical(compressed);
}

//! move the broken pieces in the input shape "right" to close up the removed areas.
//! note: breaks and pieces should not intersect by this point
//! a break:        BbbbbbbB
//! a piece:  PpppP                     move right by removed(B)
//! a piece:                  PppppP    no need to move
TopoDS_Shape  DrawBrokenView::compressHorizontal(const TopoDS_Shape& shapeToCompress)const
{
    // Base::Console().Message("DBV::compressHorizontal()\n");
    auto pieces = getPieces(shapeToCompress);
    auto breaksAll = Breaks.getValues();
    auto moveDirection = DU::closestBasisOriented(DU::toVector3d(getProjectionCS().XDirection()));
    bool descend = false;
    auto sortedBreaks = makeSortedBreakList(breaksAll, moveDirection, descend);
    auto limits = getPieceLimits(pieces, moveDirection);
    // for each break, move all the pieces left of the break to the right by the removed amount
    // for the break
    for (auto& breakItem : sortedBreaks) {
        // check each break against all the pieces
        Base::Vector3d netBreakDisplace =
            moveDirection * (removedLengthFromObj(*breakItem.breakObj) - Gap.getValue());
        size_t iPiece{0};
        for (auto& pieceLimit : limits) {
            // check each piece against the current break
            if (moveThisPiece(pieceLimit, breakItem, moveDirection)) {
                // piece is to left of break, so needs to move right
                TopoDS_Shape temp = ShapeUtils::moveShape(pieces.at(iPiece), netBreakDisplace);
                pieces.at(iPiece) = temp;
            }
            iPiece++;
        }
    }
    // turn updated pieces into a compound
    BRep_Builder builder;
    TopoDS_Compound result;
    builder.MakeCompound(result);
    for (auto& pieceShape : pieces) {
        builder.Add(result, pieceShape);
    }

    return result;
}

//! move the broken pieces in the input shape "Up" to close up the removed areas.
//! note: breaks and pieces should not intersect by this point
TopoDS_Shape  DrawBrokenView::compressVertical(const TopoDS_Shape& shapeToCompress)const
{
    // Base::Console().Message("DBV::compressVertical()\n");
    auto pieces = getPieces(shapeToCompress);
    auto breaksAll = Breaks.getValues();
    // not sure about using closestBasis here. may prevent oblique breaks later.
    auto moveDirection = DU::closestBasisOriented(DU::toVector3d(getProjectionCS().YDirection()));

    bool descend = false;
    auto sortedBreaks = makeSortedBreakList(breaksAll, moveDirection, descend);
    auto limits = getPieceLimits(pieces, moveDirection);
    // for each break, move all the pieces below the break up by the removed amount
    // for the break
    for (auto& breakItem : sortedBreaks) {
        // check each break against all the pieces
        Base::Vector3d netBreakDisplace =
            moveDirection * (removedLengthFromObj(*breakItem.breakObj) - Gap.getValue());
        size_t iPiece{0};
        for (auto& pieceLimit : limits) {
            // check each piece against the current break using a fuzzy equality
            if (moveThisPiece(pieceLimit, breakItem, moveDirection)) {
                // piece is below the break, move it up
                TopoDS_Shape temp = ShapeUtils::moveShape(pieces.at(iPiece), netBreakDisplace);
                pieces.at(iPiece) = temp;
            }
            iPiece++;
        }
    }
    // turn updated pieces into a compound
    BRep_Builder builder;
    TopoDS_Compound result;
    builder.MakeCompound(result);
    for (auto& pieceShape : pieces) {
        builder.Add(result, pieceShape);
    }

    return result;
}


//! returns a half space.  The half space is defined by a plane created by (planePoint,
//! plane normal) and a point inside the half space (pointInSpace).
TopoDS_Shape DrawBrokenView::makeHalfSpace(Base::Vector3d planePoint, Base::Vector3d planeNormal, Base::Vector3d pointInSpace) const
{
    // Base::Console().Message("DBV::makeHalfSpace - planePoint: %s normal: %s pointInSpace: %s\n",
    //                         DU::formatVector(planePoint).c_str(),
    //                         DU::formatVector(planeNormal).c_str(),
    //                         DU::formatVector(pointInSpace).c_str());
    gp_Pnt origin = DU::togp_Pnt(planePoint);
    gp_Dir axis   = DU::togp_Dir(planeNormal);
    gp_Pln plane(origin, axis);
    BRepBuilderAPI_MakeFace mkFace(plane);
    TopoDS_Face face = mkFace.Face();
    BRepPrimAPI_MakeHalfSpace mkHalf(face, DU::togp_Pnt(pointInSpace));

    return mkHalf.Solid();
}


//! extract the break points from the break object.
std::pair<Base::Vector3d, Base::Vector3d> DrawBrokenView::breakPointsFromObj(const App::DocumentObject& breakObj) const
{
    // Base::Console().Message("DBV::breakPointsFromObj()\n");
    if (ShapeExtractor::isSketchObject(&breakObj)) {
        return breakPointsFromSketch(breakObj);
    }

    TopoDS_Shape locShape = ShapeExtractor::getLocatedShape(&breakObj);
    if (!locShape.IsNull() && locShape.ShapeType() == TopAbs_EDGE) {
        return breakPointsFromEdge(breakObj);
    }
    return {Base::Vector3d(), Base::Vector3d()};
}


//! extract the breakDirection from the break object.  The break direction is
//! perpendicular to the break lines.
Base::Vector3d DrawBrokenView::directionFromObj(const App::DocumentObject& breakObj) const
{
    // Base::Console().Message("DBV::directionFromObj()\n");
    std::pair<Base::Vector3d, Base::Vector3d> ends = breakPointsFromObj(breakObj);
    Base::Vector3d direction = ends.second - ends.first;
    direction.Normalize();
    // does this need to be oriented?
    return DU::closestBasis(direction);
}


//! extract the breakDirection from the break object in a form the gui will understand.
Base::Vector3d DrawBrokenView::guiDirectionFromObj(const App::DocumentObject& breakObj) const
{
    return projectPoint(directionFromObj(breakObj));
}

//! calculate the length to be removed as specified by break object.
double DrawBrokenView::removedLengthFromObj(const App::DocumentObject& breakObj) const
{
    std::pair<Base::Vector3d, Base::Vector3d> ends = breakPointsFromObj(breakObj);
    Base::Vector3d direction = ends.second - ends.first;
    return direction.Length();
}

//! determine if a given object can be used as a break object
bool DrawBrokenView::isBreakObject(const App::DocumentObject& breakObj)
{
    if (ShapeExtractor::isSketchObject(&breakObj)) {
        return isBreakObjectSketch(breakObj);
    }
    TopoDS_Shape locShape = ShapeExtractor::getLocatedShape(&breakObj);
    if (!locShape.IsNull() && locShape.ShapeType() == TopAbs_EDGE) {
        // TODO: add check for vertical or horizontal?
        return true;
    }
    return false;
}

//! determine if a sketch object can be used as a break object
//! to be a break object the sketch must contain 2 edges, both of which are
//! horizontal or vertical
bool DrawBrokenView::isBreakObjectSketch(const App::DocumentObject& breakObj)
{
    TopoDS_Shape locShape = ShapeExtractor::getLocatedShape(&breakObj);
    if (locShape.IsNull()) {
        return false;
    }

    // get the edges from the shape.
    std::vector<TopoDS_Edge> sketchEdges;
    TopExp_Explorer expl(locShape, TopAbs_EDGE);
    for (; expl.More(); expl.Next()) {
        sketchEdges.push_back(TopoDS::Edge(expl.Current()));
    }
    // there should be 2
    if (sketchEdges.size() != 2) {
        Base::Console().Message("DBV::isBreakObjectSketch - wrong number of edges\n");
        return false;
    }
    // they should both have the same orientation
    TopoDS_Edge first = sketchEdges.front();
    TopoDS_Edge last  = sketchEdges.back();
    return SU::edgesAreParallel(first, last);
}

//! extract the break points from a sketch.  The sketch is expected to contain
//! 2 vertical or horizontal edges only.
std::pair<Base::Vector3d, Base::Vector3d> DrawBrokenView::breakPointsFromSketch(const App::DocumentObject& breakObj) const
{
    // Base::Console().Message("DBV::breakPointsFromSketch()\n");
    TopoDS_Shape locShape = ShapeExtractor::getLocatedShape(&breakObj);
    if (locShape.IsNull()) {
        return {Base::Vector3d(), Base::Vector3d()};;
    }

    // get the edges from the shape.
    // vertical or horizontal
    std::vector<TopoDS_Edge> sketchEdges;
    TopExp_Explorer expl(locShape, TopAbs_EDGE);
    for (; expl.More(); expl.Next()) {
        sketchEdges.push_back(TopoDS::Edge(expl.Current()));
    }
    // there should be 2
    if (sketchEdges.size() != 2) {
        return {Base::Vector3d(), Base::Vector3d()};
    }

    // they should both have the same orientation
    TopoDS_Edge first = sketchEdges.front();
    TopoDS_Edge last  = sketchEdges.back();
    if ((isVertical(first) && isVertical(last)) ||
        (isHorizontal(first) && isHorizontal(last))) {
        // get points on each edge that are in line with the center of the sketch
        // along the break direction
        Bnd_Box edgeBox;
        edgeBox.SetGap(0.0);
        BRepBndLib::AddOptimal(first, edgeBox);
        BRepBndLib::AddOptimal(last, edgeBox);
        double xMin = 0, xMax = 0, yMin = 0, yMax = 0, zMin = 0, zMax = 0;
        edgeBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        Base::Vector3d center( (xMin + xMax) / 2.0,
                               (yMin + yMax) / 2.0,
                               (zMin + zMax) / 2.0 );
        auto ends0 = SU::getEdgeEnds(first);
        auto dir0 = ends0.second - ends0.first;
        dir0.Normalize();
        // get a point on edge first on a perpendicular line through center
        auto break0 = center.Perpendicular(ends0.first, dir0);

        auto ends1 = SU::getEdgeEnds(last);
        auto dir1 = ends1.second - ends1.first;
        dir1.Normalize();
        auto break1 = center.Perpendicular(ends1.first, dir1);

        return { break0, break1 };
    }

    return {Base::Vector3d(), Base::Vector3d()};
}


//! extract the break points from an edge.  The edge should be vertical or horizontal, perpendicular to the desired
//! break lines.
std::pair<Base::Vector3d, Base::Vector3d> DrawBrokenView::breakPointsFromEdge(const App::DocumentObject& breakObj) const
{
    TopoDS_Shape locShape = ShapeExtractor::getLocatedShape(&breakObj);
    if (locShape.IsNull() || locShape.ShapeType() != TopAbs_EDGE) {
        return {Base::Vector3d(), Base::Vector3d()};
    }

    TopoDS_Edge edge = TopoDS::Edge(locShape);
    gp_Pnt start = BRep_Tool::Pnt(TopExp::FirstVertex(edge));
    gp_Pnt end = BRep_Tool::Pnt(TopExp::LastVertex(edge));
    return {DU::toVector3d(start), DU::toVector3d(end)};
}


//! determine the rectangle to be occupied by the break lines. used by gui.
std::pair<Base::Vector3d, Base::Vector3d> DrawBrokenView::breakBoundsFromObj(const App::DocumentObject& breakObj) const
{
    // Base::Console().Message("DBV::breakBoundsFromObj()\n");
    if (ShapeExtractor::isSketchObject(&breakObj)) {
        auto unscaled = breakBoundsFromSketch(breakObj);
        return scalePair(unscaled);
    }

    TopoDS_Shape locShape = ShapeExtractor::getLocatedShape(&breakObj);
    if (!locShape.IsNull() && locShape.ShapeType() == TopAbs_EDGE) {
        auto unscaled = breakBoundsFromEdge(breakObj);
        return scalePair(unscaled);
    }
    return {Base::Vector3d(), Base::Vector3d()};
}


//! extract the boundary of the break lines from a sketch (3d coords) and map to this
//! broken view.  used in making break lines.
std::pair<Base::Vector3d, Base::Vector3d> DrawBrokenView::breakBoundsFromSketch(const App::DocumentObject& breakObj) const
{
    //Base::Console().Message("DBV::breakBoundsFromSketch()\n");
    std::pair<Base::Vector3d, Base::Vector3d> breakPoints = breakPointsFromObj(breakObj);
    Base::Vector3d center = (breakPoints.first + breakPoints.second) / 2.0;
    Base::Vector3d breakDir = directionFromObj(breakObj);
    breakDir.Normalize();
    Base::Vector3d perpDir = makePerpendicular(breakDir);
    perpDir.Normalize();

    // get the midpoint of the zigzags
    Base::Vector3d ptOnLine0 = center + breakDir * removedLengthFromObj(breakObj) / 2.0;
    Base::Vector3d ptOnLine1 = center - breakDir * removedLengthFromObj(breakObj) / 2.0;
    double lineLength = breaklineLength(breakObj);

    Base::Vector3d corner0 = ptOnLine0 - perpDir * lineLength / 2.0;
    Base::Vector3d corner1 = ptOnLine1 + perpDir * lineLength / 2.0;
    corner0 = mapPoint3dToView(corner0);
    corner1 = mapPoint3dToView(corner1);

    // these are unscaled, unrotated points
    return{corner0, corner1};
}


//! extract the boundary of the break lines from an edge
std::pair<Base::Vector3d, Base::Vector3d> DrawBrokenView::breakBoundsFromEdge(const App::DocumentObject& breakObj) const
{
    // Base::Console().Message("DBV::breakBoundsFromEdge()\n");
    TopoDS_Shape locShape = ShapeExtractor::getLocatedShape(&breakObj);
    if (locShape.IsNull() || locShape.ShapeType() != TopAbs_EDGE) {
        return {Base::Vector3d(), Base::Vector3d()};
    }

    auto edge = projectEdge(TopoDS::Edge(locShape));
    auto start = edge->getStartPoint();
    auto end = edge->getEndPoint();
    Base::Vector3d direction = end - start;
    double length = direction.Length();
    direction.Normalize();
    Base::Vector3d stdX{1.0, 0.0, 0.0};
    Base::Vector3d stdY{0.0, 1.0, 0.0};
    if (DU::fpCompare(fabs(direction.Dot(stdX)), 1.0, EWTOLERANCE) ) {
        double left = std::min(start.x, end.x);
        double right = std::max(start.x, end.x);
        // not wild about this for top/bottom
        double top = start.y + length;
        double bottom = start.y - length;
        Base::Vector3d topLeft{left, top, 0.0};
        Base::Vector3d bottomRight{right, bottom, 0.0};
        return{topLeft, bottomRight};
    }

    if (!DU::fpCompare(fabs(direction.Dot(stdY)), 1.0, EWTOLERANCE) ) {
        Base::Console().Message("DBV::breakBoundsFromEdge - direction is not X or Y\n");
        // TODO: throw? return nonsense?
    }

    double left = start.x - length;
    double right = start.x + length;
    double bottom = std::min(start.y, end.y);
    double top = std::max(start.y, end.y);
    Base::Vector3d topLeft{left, top, 0.0};
    Base::Vector3d bottomRight{right, bottom, 0.0};
    return{topLeft, bottomRight};
}

//! calculate the unscaled length of the breakline
double DrawBrokenView::breaklineLength(const App::DocumentObject& breakObj) const
{
    // Base::Console().Message("DBV::breaklineLength()\n");
    if (ShapeExtractor::isSketchObject(&breakObj)) {
        return breaklineLengthFromSketch(breakObj);
    }

    TopoDS_Shape locShape = ShapeExtractor::getLocatedShape(&breakObj);
    if (!locShape.IsNull() && locShape.ShapeType() == TopAbs_EDGE) {
        return  breaklineLengthFromEdge(breakObj);
    }
    return 0.0;
}

//! calculate the length of the breakline for a sketch based break
double DrawBrokenView::breaklineLengthFromSketch(const App::DocumentObject& breakObj) const
{
    // Base::Console().Message("DBV::breaklineLengthFromSketch()\n");
    TopoDS_Shape locShape = ShapeExtractor::getLocatedShape(&breakObj);
    if (locShape.IsNull()) {
        return 0;
    }
    // get the edges from the sketch
    std::vector<TopoDS_Edge> sketchEdges;
    TopExp_Explorer expl(locShape, TopAbs_EDGE);
    for (; expl.More(); expl.Next()) {
        sketchEdges.push_back(TopoDS::Edge(expl.Current()));
    }

    if (sketchEdges.size() < 2)  {
        // need 2 edges
        Base::Console().Message("DBV::breaklineLengthFromSketch - not enough edges\n");
    }

    std::pair<Base::Vector3d, Base::Vector3d> ends0 = SU::getEdgeEnds(sketchEdges.front());
    ends0.first = projectPoint(ends0.first, false);
    ends0.second = projectPoint(ends0.second, false);

    std::pair<Base::Vector3d, Base::Vector3d> ends1 = SU::getEdgeEnds(sketchEdges.back());
    ends1.first = projectPoint(ends1.first, false);
    ends1.second = projectPoint(ends1.second, false);
    if (isVertical(ends0, true)) {
        // sketch line is vertical, so breakline is also vertical
        double yLow = std::min({ends0.first.y, ends0.second.y, ends1.first.y, ends1.second.y});
        double yHigh = std::max({ends0.first.y, ends0.second.y, ends1.first.y, ends1.second.y});
        return yHigh - yLow;
    }

    // sketch line is horizontal, so breakline is also horizontal
    double xLow = std::min({ends0.first.x, ends0.second.x, ends1.first.x, ends1.second.x});
    double xHigh = std::max({ends0.first.x, ends0.second.x, ends1.first.x, ends1.second.x});
    return xHigh - xLow;
}

//! calculate the length of the breakline for an edge based break
double DrawBrokenView::breaklineLengthFromEdge(const App::DocumentObject& breakObj) const
{
    // Base::Console().Message("DBV::breaklineLengthFromEdge()\n");
    TopoDS_Shape locShape = ShapeExtractor::getLocatedShape(&breakObj);
    if (!locShape.IsNull() && locShape.ShapeType() != TopAbs_EDGE) {
        return 0.0;
    }
    // the breakline could be very long.  do we need a max breakline length?
    auto edge = projectEdge(TopoDS::Edge(locShape));
    auto start = edge->getStartPoint();
    auto end = edge->getEndPoint();
    return (end - start).Length();
}

//! return true if the edge is vertical.
bool DrawBrokenView::isVertical(TopoDS_Edge edge, bool projected) const
{
    // Base::Console().Message("DBV::isVertical(edge, %d)\n", projected);
    Base::Vector3d stdY{0.0, 1.0, 0.0};
    auto ends = SU::getEdgeEnds(edge);
    auto edgeDir = ends.second - ends.first;
    edgeDir.Normalize();

    auto upDir = DU::toVector3d(getProjectionCS().YDirection());
    if (projected) {
        upDir = stdY;
    }
    upDir.Normalize();      // probably superfluous


    if (DU::fpCompare(std::fabs(upDir.Dot(edgeDir)), 1.0, EWTOLERANCE)) {
        return true;
    }

    return false;
}

//! return true if the input points are vertical
bool DrawBrokenView::isVertical(std::pair<Base::Vector3d, Base::Vector3d> inPoints, bool projected) const
{
    // Base::Console().Message("DBV::isVertical(%s, %s, %d)\n",
    //                         DU::formatVector(inPoints.first).c_str(),
    //                         DU::formatVector(inPoints.second).c_str(), projected);
    Base::Vector3d stdY{0.0, 1.0, 0.0};
    auto pointDir = inPoints.second - inPoints.first;
    pointDir.Normalize();

    auto upDir = DU::toVector3d(getProjectionCS().YDirection());
    if (projected) {
        upDir = stdY;
    }
    upDir.Normalize();      // probably superfluous
    if (DU::fpCompare(std::fabs(upDir.Dot(pointDir)), 1.0, EWTOLERANCE)) {
        return true;
    }

    return false;
}

//! return true if the edge is horizontal
bool DrawBrokenView::isHorizontal(TopoDS_Edge edge, bool projected) const
{
    Base::Vector3d stdX{1.0, 0.0, 0.0};
    auto ends = SU::getEdgeEnds(edge);
    auto edgeDir = ends.second - ends.first;
    edgeDir.Normalize();

    auto sideDir = DU::toVector3d(getProjectionCS().XDirection());
    if (projected) {
        sideDir = stdX;
    }
    sideDir.Normalize();      // probably superfluous

    if (DU::fpCompare(std::fabs(sideDir.Dot(edgeDir)), 1.0, EWTOLERANCE)) {
        return true;
    }

    return false;
}

//! removes break objects from a list of document objects and returns the rest of the objects.
//! used by TechDrawGui::Command
std::vector<App::DocumentObject*> DrawBrokenView::removeBreakObjects(std::vector<App::DocumentObject*> breaks, std::vector<App::DocumentObject*> shapes)
{
    // Base::Console().Message("DBV::removeBreakObjects() - breaks: %d  shapes in: %d\n", breaks.size(), shapes.size());
    std::vector<App::DocumentObject*> result;
    for (auto& shapeObj : shapes) {
        bool found = false;
        for (auto& breakObj : breaks) {
            if (breakObj == shapeObj) {
                found = true;
                break;
            }
        }

        if (!found) {
            result.push_back(shapeObj);
        }
    }
    return result;
}

std::vector<TopoDS_Edge> DrawBrokenView::edgesFromCompound(TopoDS_Shape compound)
{
    std::vector<TopoDS_Edge> edgesOut;
    TopExp_Explorer expl(compound, TopAbs_EDGE);
    for (; expl.More(); expl.Next()) {
        edgesOut.push_back(TopoDS::Edge(expl.Current()));
    }
    return edgesOut;
}


//! find the limits of each piece's bounding box in cardinal direction (if we support oblique projection directions, then the
//! piece will have to be transformed to align with OXYZ cardinal axes as in DrawViewPart::getSizeAlongVector)
PieceLimitList DrawBrokenView::getPieceLimits(const std::vector<TopoDS_Shape>& pieces, Base::Vector3d direction)
{
    // Base::Console().Message("DBV::getPieceUpperLimits(%s)\n", DU::formatVector(direction).c_str());
    Base::Vector3d stdX{1.0, 0.0, 0.0};
    Base::Vector3d stdY{0.0, 1.0, 0.0};
    Base::Vector3d stdZ{0.0, 0.0, 1.0};
    PieceLimitList limits;
    limits.reserve(pieces.size());
    for (auto& item : pieces) {
        Bnd_Box pieceBox;
        pieceBox.SetGap(0.0);
        BRepBndLib::AddOptimal(item, pieceBox);
        double xMin = 0, xMax = 0, yMin = 0, yMax = 0, zMin = 0, zMax = 0;
        pieceBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        if (DU::fpCompare(std::fabs(direction.Dot(stdX)), 1.0, EWTOLERANCE)) {
            PieceLimitEntry newEntry;
            newEntry.highLimit = xMax;
            newEntry.lowLimit = xMin;
            limits.push_back(newEntry);
        } else if (DU::fpCompare(std::fabs(direction.Dot(stdY)), 1.0, EWTOLERANCE)) {
            PieceLimitEntry newEntry;
            newEntry.highLimit = yMax;
            newEntry.lowLimit = yMin;
            limits.push_back(newEntry);
        } else {
            PieceLimitEntry newEntry;
            newEntry.highLimit = zMax;
            newEntry.lowLimit = zMin;
            limits.push_back(newEntry);
        }
    }

    return limits;
}

std::vector<TopoDS_Shape> DrawBrokenView::getPieces(TopoDS_Shape brokenShape)
{
    std::vector<TopoDS_Shape> result;

    // ?? is it reasonable to expect that we only want the solids? do we need to
    // pick based on ShapeType <= TopAbs_SHELL? to get shells, compounds etc?
    TopExp_Explorer expl(brokenShape, TopAbs_SOLID);
    for (; expl.More(); expl.Next()) {
        const TopoDS_Solid& solid = TopoDS::Solid(expl.Current());
        result.push_back(solid);
    }

    return result;
}

//! sort the breaks that match direction by their minimum limit
BreakList DrawBrokenView::makeSortedBreakList(const std::vector<App::DocumentObject*>& breaks, Base::Vector3d direction, bool descend) const
{
    // Base::Console().Message("DBV::makeSortedBreakList(%d, %s)\n", breaks.size(),
    //                         DU::formatVector(direction).c_str());
    Base::Vector3d stdX{1.0, 0.0, 0.0};
    Base::Vector3d stdY{0.0, 1.0, 0.0};
    Base::Vector3d stdZ{0.0, 0.0, 1.0};

    BreakList unsorted;
    for (auto& breakObj : breaks) {
        auto breakDirection = directionFromObj(*breakObj);
        if (DU::fpCompare(std::fabs(direction.Dot(breakDirection)), 1.0, EWTOLERANCE)) {
            // this break interests us
            BreakListEntry newEntry;
            newEntry.breakObj = breakObj;
            auto breakPoints = breakPointsFromObj(*breakObj);
            if (DU::fpCompare(std::fabs(direction.Dot(stdX)), 1.0, EWTOLERANCE )) {
                newEntry.lowLimit = std::min(breakPoints.first.x, breakPoints.second.x);
                newEntry.highLimit = std::max(breakPoints.first.x, breakPoints.second.x);
            } else if (DU::fpCompare(std::fabs(direction.Dot(stdY)), 1.0, EWTOLERANCE )) {
                newEntry.lowLimit = std::min(breakPoints.first.y, breakPoints.second.y);
                newEntry.highLimit = std::max(breakPoints.first.y, breakPoints.second.y);
            } else {
                // must be Z!
                newEntry.lowLimit = std::min(breakPoints.first.z, breakPoints.second.z);
                newEntry.highLimit = std::max(breakPoints.first.z, breakPoints.second.z);
            }
            newEntry.netRemoved = removedLengthFromObj(*breakObj) - Gap.getValue();
            unsorted.push_back(newEntry);
       }
    }
    BreakList sorted = sortBreaks(unsorted, descend);
    return sorted;
}


//! find the compressed location of the breaks, and sort the result by lower limit
BreakList DrawBrokenView::makeSortedBreakListCompressed(const std::vector<App::DocumentObject*>& breaks, Base::Vector3d moveDirection, bool descend) const
{
    // Base::Console().Message("DBV::makeSortedBreakListCompressed(%d, %s)\n", breaks.size(),
    //                         DU::formatVector(moveDirection).c_str());
    // sortedBreaks is in lowLimit order
    auto sortedBreaks = makeSortedBreakList(breaks, moveDirection, descend);
    BreakList result;
    size_t iBreak{0};
    for (auto& breakObj : sortedBreaks) {
        BreakListEntry newEntry;
        double breakSum{0};
        if (isDirectionReversed(moveDirection)) {
            // reversed  X+   high  low   X-
            // our list is sorted ascending by low limit - each is further left than the previous
            // so we must reverse our list to get the correct shift values.
            std::reverse(sortedBreaks.begin(), sortedBreaks.end());
            for (size_t iSum = iBreak + 1; iSum < sortedBreaks.size(); iSum++) {
                // shift right by the removed amount of all the breaks to the right of this break
                breakSum += sortedBreaks.at(iSum).netRemoved;
            }
            newEntry.breakObj = breakObj.breakObj;
            newEntry.lowLimit = breakObj.lowLimit - breakSum;   // move to right
            newEntry.highLimit = newEntry.lowLimit + Gap.getValue();
            newEntry.netRemoved = breakObj.netRemoved;
            result.push_back(newEntry);
        } else {
            // forward   X-   low   high  X+
            for (size_t iSum = iBreak + 1; iSum < sortedBreaks.size(); iSum++) {
                // shift right by the removed amount of all the breaks to the right of this break
                breakSum += sortedBreaks.at(iSum).netRemoved;
            }
            newEntry.breakObj = breakObj.breakObj;
            newEntry.lowLimit = breakObj.lowLimit + breakObj.netRemoved + breakSum;
            newEntry.highLimit = newEntry.lowLimit + Gap.getValue();
            newEntry.netRemoved = breakObj.netRemoved;
            result.push_back(newEntry);
        }
        iBreak++;
    }

    if (isDirectionReversed(moveDirection)) {
        std::reverse(sortedBreaks.begin(), sortedBreaks.end());
    }
    return result;
}



BreakList DrawBrokenView::sortBreaks(BreakList& inList, bool descend)
{
    // Base::Console().Message("DBV::sortBreaks(%d, %d)\n", inList.size(), descend);
    BreakList sorted = inList;
    std::sort(sorted.begin(), sorted.end(), DrawBrokenView::breakLess);
    if (descend) {
        std::reverse(sorted.begin(), sorted.end());
    }
    return sorted;
}


//! return true if entry0 "is less than" entry
/*static*/bool DrawBrokenView::breakLess(const BreakListEntry& entry0, const BreakListEntry& entry1)
{
    if (entry0.lowLimit < entry1.lowLimit) {
        return true;
    }
    return false;
}


//! transform a 3d point into its position within the broken view.  used in creating
//! dimensions.
Base::Vector3d DrawBrokenView::mapPoint3dToView(Base::Vector3d point3d) const
{
    //Base::Console().Message("DBV::mapPoint3dToView(%s)\n", DU::formatVector(point3d).c_str());
    Base::Vector3d result{point3d};

    auto breaksAll = Breaks.getValues();
    bool descend = false;
    auto moveXDirection = DU::closestBasisOriented(DU::toVector3d(getProjectionCS().XDirection()));

    // get the breaks that move us in X
    auto sortedXBreaks = makeSortedBreakList(breaksAll, moveXDirection, descend);
    double xLimit = DU::coordinateForDirection(point3d, moveXDirection);

    double xShift = shiftAmountShrink(xLimit, moveXDirection, sortedXBreaks);
    Base::Vector3d xMove = moveXDirection * xShift;    // move to the right (+X)

    auto moveYDirection = DU::closestBasisOriented(DU::toVector3d(getProjectionCS().YDirection()));
    descend = false;
    // get the breaks that move us in Y
    auto sortedYBreaks = makeSortedBreakList(breaksAll, moveYDirection, descend);
    double yLimit = DU::coordinateForDirection(point3d, moveYDirection);
    double yShift = shiftAmountShrink(yLimit, moveYDirection, sortedYBreaks);
    Base::Vector3d yMove = moveYDirection * yShift;   // move up (+Y)

    auto moved3d = point3d + xMove + yMove;
    result = moved3d - getCompressedCentroid();

    result = projectPoint(result, false);
    return result;
}


//! transform a 2d point in the broken view into the equivalent point on the XY
//! paper plane.  used in creating dimensions from points on the broken view.
Base::Vector3d DrawBrokenView::mapPoint2dFromView(Base::Vector3d point2d) const
{
    // Base::Console().Message("DBV::mapPoint2dFromView(%s)\n", DU::formatVector(point2d).c_str());
    Base::Vector3d stdX(1.0, 0.0, 0.0);
    Base::Vector3d stdY(0.0, 1.0, 0.0);

    // make pseudo 3d point from point2d
    gp_Ax3 OXYZ;
    gp_Ax3 projCS3(getProjectionCS(getCompressedCentroid()));
    gp_Trsf xTo3d;
    xTo3d.SetTransformation(projCS3, OXYZ);
    auto pseudo3d = DU::toVector3d(DU::togp_Pnt(point2d).Transformed(xTo3d));

    // now shift down and left
    auto breaksAll = Breaks.getValues();

    auto moveXDirection = DU::closestBasisOriented(DU::toVector3d(getProjectionCS().XDirection()));
    // we are expanding, so the direction should be to the "left"/"down" which is the opposite of
    // our XDirection
    auto moveXReverser = isDirectionReversed(moveXDirection) ? 1.0 : -1.0;
    bool descend = false;     // should be false so we move from lowest break to highest?
    auto sortedXBreaks = makeSortedBreakList(breaksAll, moveXDirection, descend);
    double xLimit = DU::coordinateForDirection(pseudo3d, moveXDirection);

    std::vector<size_t> fullGaps;
    int partialGapIndex{-1};
    auto compressedXBreaks = makeSortedBreakListCompressed(breaksAll, moveXDirection, descend);
    double partialGapPenetration = getExpandGaps(xLimit, compressedXBreaks, moveXDirection, fullGaps, partialGapIndex);
    double breakSum{0};
    for (auto& index : fullGaps) {
        double breakSize = sortedXBreaks.at(index).netRemoved;
        breakSum += breakSize;
    }
    if (partialGapIndex >= 0) {
        double breakSize = sortedXBreaks.at(partialGapIndex).netRemoved;
        breakSum += breakSize * partialGapPenetration;
    }
    double xCoord2 = xLimit + breakSum * moveXReverser;

    auto moveYDirection = DU::closestBasisOriented(DU::toVector3d(getProjectionCS().YDirection()));
    auto moveYReverser = isDirectionReversed(moveYDirection) ? 1.0 : -1.0;
    descend = false;
    auto sortedYBreaks = makeSortedBreakList(breaksAll, moveYDirection, descend);
    double yLimit = DU::coordinateForDirection(pseudo3d, moveYDirection);

    fullGaps.clear();
    partialGapIndex = -1;
    auto compressedYBreaks = makeSortedBreakListCompressed(breaksAll, moveYDirection, descend);
    partialGapPenetration = getExpandGaps(yLimit, compressedYBreaks, moveYDirection, fullGaps, partialGapIndex);
    breakSum = 0;
    for (auto& index : fullGaps) {
        double breakSize = sortedYBreaks.at(index).netRemoved;
        breakSum += breakSize;
    }
    if (partialGapIndex >= 0) {
        double breakSize = sortedYBreaks.at(partialGapIndex).netRemoved;
        breakSum += breakSize * partialGapPenetration;
    }
    double yCoord2 = yLimit + breakSum * moveYReverser;

    Base::Vector3d movedResult{xCoord2, yCoord2, 0.0};
    return movedResult;
}


//! returns the amount a coordinate needs to move to reflect the effect of the breaks to the right/above
//! the input.  used in mapping points to the broken view.
double DrawBrokenView::shiftAmountShrink(double pointCoord, Base::Vector3d direction, const BreakList& sortedBreaks) const
{
    // Base::Console().Message("DBV::shiftAmountShrink(%.3f, %s, %d)\n", pointCoord,
    //                         DU::formatVector(direction).c_str(), sortedBreaks.size());
    double shift{0};
    for (auto& breakItem : sortedBreaks) {
        if (isDirectionReversed(direction)) {
            if (pointCoord <= breakItem.lowLimit) {
                //        h--------l      -ve
                //                   p
                // point is right/above break
                // leave alone, this break doesn't affect us
                continue;
            }

            if (pointCoord > breakItem.highLimit ||
                DU::fpCompare(pointCoord, breakItem.highLimit, Precision::Confusion()) ) {
                //        h--------l      -ve
                //    p
                // point is left/below break
                shift += breakItem.netRemoved;
                continue;
            }

            //        h--------l      -ve
            //            p
            //               g-g
            //                p'
            // break.start < value < break.end - point is in the break area
            // we move our point by a fraction of the Gap length
            double penetration =  fabs(pointCoord - breakItem.highLimit);   // (p - h) start(high) to point distance
            double removed = removedLengthFromObj(*breakItem.breakObj);     // (h-l) full break size
            double factor = 1 - (penetration / removed);                    // fraction of break to right
            double toShift = pointCoord - (breakItem.lowLimit - factor * Gap.getValue());
            shift += fabs(toShift);


        } else {
            if (pointCoord >= breakItem.highLimit) {
                //        l--------h     +ve
                //                    p
                // leave alone, this break doesn't affect us
                continue;
            }

            if (pointCoord < breakItem.lowLimit  ||
                DU::fpCompare(pointCoord, breakItem.lowLimit, Precision::Confusion()) ) {
                //        l--------h      +ve
                //    p
                // move right/up by the removed area less the gap
                shift += removedLengthFromObj(*breakItem.breakObj) - Gap.getValue();
                continue;
            }

            //        l--------h      +ve
            //            p
            //              g--g
            //               p'
            // break.start < value < break.end - point is in the break area
            // we move our point by a fraction of the Gap length
            double penetration = fabs(pointCoord - breakItem.lowLimit);
            double removed = removedLengthFromObj(*breakItem.breakObj);
            double factor = 1 - (penetration / removed);
            double netRemoved = pointCoord - breakItem.highLimit - factor * Gap.getValue();
            shift += fabs(netRemoved);
        }
    }

    return shift;
}


// //! returns the amount a compressed coordinate needs to be shifted to reverse the effect of breaking
// //! the source shapes.  Could have problems here if gap > removed? Is that always an error?
// double DrawBrokenView::shiftAmountExpand(double pointCoord, Base::Vector3d direction, const BreakList& sortedBreaks) const
// {
//     Base::Console().Message("DBV::shiftAmountExpand(%.3f, %s, %d)\n", pointCoord,
//                             DU::formatVector(direction).c_str(), sortedBreaks.size());
//     double shift{0};
//     for (auto& breakItem : sortedBreaks) {
//         if (isDirectionReversed(direction)) {
//             Base::Console().Message("DBV::shiftAmountExpand - reversed\n");
//             if (pointCoord <= breakItem.lowLimit) {
//                 //        h--------l     -ve
//                 //                    p
//                 // leave alone, this break doesn't affect us
//                 Base::Console().Message("DBV::shiftAmountExpand - ignore\n");
//                 continue;
//             }

//             if (pointCoord > breakItem.highLimit  ||
//                 DU::fpCompare(pointCoord, breakItem.highLimit, Precision::Confusion()) ) {
//                 //        h--------l      -ve
//                 //    p

//                 // move by the whole removed area
//                 shift += breakItem.netRemoved;
//                 Base::Console().Message("DBV::shiftAmountExpand - full\n");
//                 continue;
//             }

//             //        h--------l      -ve
//             //            p
//             // break.start < value < break.end - point is in the break area
//             // we move our point by the break's removed * the penetration factor
//             Base::Console().Message("DBV::shiftAmountExpand - partial\n");
//             double breakPenetration = fabs(pointCoord - breakItem.lowLimit);
//             double removed = removedLengthFromObj(*breakItem.breakObj);
//             double factor = breakPenetration / removed;
//             double shiftAmount = breakItem.lowLimit + factor * removed;
//             Base::Console().Message("DBV::shiftAmountExpand - penetration: %.3f removed: %.3f factor: %.3f shift: %.3f\n",
//                                     breakPenetration, removed, factor, shiftAmount);
//             shift += std::fabs(shiftAmount);
//         } else {
//             Base::Console().Message("DBV::shiftAmountExpand - forward\n");
//             if (pointCoord >= breakItem.highLimit) {
//                 //        l--------h     +ve
//                 //                    p
//                 // leave alone, this break doesn't affect us
//                 Base::Console().Message("DBV::shiftAmountExpand - ignore\n");
//                 continue;
//             }

//             if (pointCoord < breakItem.lowLimit  ||
//                 DU::fpCompare(pointCoord, breakItem.lowLimit, Precision::Confusion()) ) {
//                 //        l--------h      +ve
//                 //    p
//                 // move by the whole removed area
//                 shift += breakItem.netRemoved;
//                 Base::Console().Message("DBV::shiftAmountExpand - full\n");
//                 continue;
//             }

//             //        l--------h      +ve
//             //            p
//             //            bpbpbp
//             //        rrrrrrrrrr
//             // break.start < value < break.end - point is in the break area
//             // we move our point by the break's removed * the penetration factor
//             Base::Console().Message("DBV::shiftAmountExpand - partial\n");
//             double breakPenetration = std::fabs(pointCoord - breakItem.highLimit);  // (h-p)
//             double removed = removedLengthFromObj(*breakItem.breakObj);
//             double factor = breakPenetration / removed;
//             double shiftAmount = breakItem.highLimit - factor * removed;
//             Base::Console().Message("DBV::shiftAmountExpand - penetration: %.3f removed: %.3f factor: %.3f shift: %.3f\n",
//                                     breakPenetration, removed, factor, shiftAmount);
//             shift += std::fabs(shiftAmount);
//         }
//     }

//     return shift;
// }

//! determine which gaps require pointCoord to move by a full gap and if there is a partial gap that must
//! be included in the move operation. If there is a partial gap, the penetration factor is returned.
//! penetration is measure right to left in the view.
double  DrawBrokenView::getExpandGaps (double pointCoord,
                                    const BreakList& compressedBreakList,
                                    Base::Vector3d moveDirection,
                                    std::vector<size_t>& fullGaps,
                                    int& partialGapIndex) const
{
    // Base::Console().Message("DBV::getExpandGaps(coord: %.3f moveDir: %s)\n", pointCoord,
    //                         DU::formatVector(moveDirection).c_str());
    double partialPenetrationFactor{0};
    // check pointCoord against compressed gaps
    size_t iBreak{0};
    for (auto& gap : compressedBreakList) {
        if (isDirectionReversed(moveDirection)) {
            // reversed X+ is to the left
            if (pointCoord < gap.lowLimit) {
                // not interested
                iBreak++;
                continue;
            }
            if (pointCoord > gap.highLimit ||
                DU::fpCompare(pointCoord, gap.highLimit, Precision::Confusion()) ) {
                // need to move by full length of associated break
                fullGaps.push_back(iBreak);
                iBreak++;
                continue;
            }
            // pointCoord is in gap
            // X+  high > pointCoord > low  X-
            partialGapIndex = iBreak;
            partialPenetrationFactor = (pointCoord - gap.lowLimit) / Gap.getValue();
            iBreak++;
        } else {
            // forward  +X is to the right
            if (pointCoord > gap.highLimit) {
                // not interested
                iBreak++;
                continue;
            }
            if (pointCoord < gap.lowLimit ||
                DU::fpCompare(pointCoord, gap.lowLimit, Precision::Confusion()) ) {
                // need to move by full length of associated break
                fullGaps.push_back(iBreak);
                iBreak++;
                continue;
            }
            // pointCoord is in gap
            //   low < pointCoord < highLimit  X+
            partialGapIndex = iBreak;
            partialPenetrationFactor = (gap.highLimit - pointCoord) / Gap.getValue();
            iBreak++;
        }
   }
   return partialPenetrationFactor;
}

Base::Vector3d DrawBrokenView::getCompressedCentroid() const
{
    if (m_compressedShape.IsNull()) {
        return Base::Vector3d(0.0, 0.0, 0.0);
    }
    gp_Ax2 cs = getProjectionCS();
    gp_Pnt gCenter = ShapeUtils::findCentroid(m_compressedShape, cs);
    return DU::toVector3d(gCenter);
}

//! construct a perpendicular direction in the projection CS
Base::Vector3d  DrawBrokenView::makePerpendicular(Base::Vector3d inDir) const
{
    gp_Dir gDir = DU::togp_Dir(inDir);
    gp_Pnt origin(0.0, 0.0, 0.0);
    auto dir = getProjectionCS().Direction();
    gp_Ax1 axis(origin, dir);
    auto gRotated = gDir.Rotated(axis,  M_PI_2);
    return DU::toVector3d(gRotated);
}

//! true if this piece should be moved
bool DrawBrokenView::moveThisPiece(PieceLimitEntry piece,
                                   BreakListEntry breakItem,
                                   Base::Vector3d moveDirection) const
{
    if (isDirectionReversed(moveDirection)) {
        // -ve direction
        if (piece.lowLimit > breakItem.highLimit  ||
            DU::fpCompare(piece.lowLimit, breakItem.highLimit, Precision::Confusion()) ) {
            return true;
        }
    } else {
        // +ve direction
        if (piece.highLimit < breakItem.lowLimit  ||
            DU::fpCompare(piece.highLimit, breakItem.lowLimit, Precision::Confusion()) ) {
            return true;
        }
    }
    return false;
}

//! true if direction is the reversed of a cardinal direction.  (1.0, 0.0, 0.0)
//! returns false, (-1.0, 0.0, 0.0) returns true;
bool DrawBrokenView::isDirectionReversed(Base::Vector3d direction) const
{
    Base::Vector3d stdX{1.0, 0.0, 0.0};
    Base::Vector3d stdY{0.0, 1.0, 0.0};
    Base::Vector3d stdZ{0.0, 0.0, 1.0};
    if (DU::fpCompare(std::fabs(direction.Dot(stdX)), 1.0, EWTOLERANCE)) {
        return DU::fpCompare(direction.Dot(stdX), -1.0, EWTOLERANCE);
    }
    if (DU::fpCompare(std::fabs(direction.Dot(stdY)), 1.0, EWTOLERANCE)) {
        return DU::fpCompare(direction.Dot(stdY), -1.0, EWTOLERANCE);
    }
    if (DU::fpCompare(std::fabs(direction.Dot(stdZ)), 1.0, EWTOLERANCE)) {
        return DU::fpCompare(direction.Dot(stdZ), -1.0, EWTOLERANCE);
    }
    return false;
}

void DrawBrokenView::printBreakList(const std::string& text, const BreakList& inBreaks) const
{
    Base::Console().Message("DBV - %s\n", text.c_str());
    for (auto& entry : inBreaks) {
        Base::Console().Message("   > label: %s  >  low: %.3f  >  high: %.3f  >  net: %.3f\n", entry.breakObj->Label.getValue(),
                                entry.lowLimit, entry.highLimit, entry.netRemoved);
    }
}


std::pair<Base::Vector3d, Base::Vector3d> DrawBrokenView::scalePair(std::pair<Base::Vector3d, Base::Vector3d> inPair) const
{
    std::pair<Base::Vector3d, Base::Vector3d> result;
    result.first = inPair.first * getScale();
    result.second = inPair.second * getScale();
    return result;
}

PyObject *DrawBrokenView::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawBrokenViewPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}



namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawBrokenViewPython, TechDraw::DrawBrokenView)
template<>
const char* TechDraw::DrawBrokenViewPython::getViewProviderName() const
{
    return "TechDrawGui::ViewProviderViewPart";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawBrokenView>;
}// namespace App
