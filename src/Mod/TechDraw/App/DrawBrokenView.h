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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <TopoDS_Shape.hxx>

#include <Base/Vector3D.h>

#include "DrawViewPart.h"

namespace TechDraw
{

struct BreakListEntry {
    App::DocumentObject* breakObj;
    double lowLimit;       // the value to use for shifting shapes (the low end of the break)
    double highLimit;      // the other end of the break (the high end)
    double netRemoved;     // the removed amount of the break, less the gap size
    // TODO: can the gap size change during the lifetime of BreakListEntry?  if
    // so, we need to save the gap size @ creation time?
};
using BreakList = std::vector<BreakListEntry>;

struct PieceLimitEntry {
    double lowLimit;
    double highLimit;
};
using PieceLimitList = std::vector<PieceLimitEntry>;

// NOLINTBEGIN
class TechDrawExport DrawBrokenView: public TechDraw::DrawViewPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawBrokenView);    // NOLINT

// NOLINTEND

public:
    enum class BreakType : int {
        NONE,
        ZIGZAG,
        SIMPLE
    };
    static const char* BreakTypeEnums[];    // NOLINT

    DrawBrokenView();
    ~DrawBrokenView() override = default;

    App::PropertyLinkList Breaks;   // NOLINT
    App::PropertyLength   Gap;      // NOLINT

    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    PyObject *getPyObject() override;
    const char* getViewProviderName() const override
    {
        return "TechDrawGui::ViewProviderViewPart";
    }

    std::pair<Base::Vector3d, Base::Vector3d>
                    breakPointsFromObj(const App::DocumentObject& breakObj) const;
    std::pair<Base::Vector3d, Base::Vector3d>
                    breakBoundsFromObj(const App::DocumentObject& breakObj) const;
    Base::Vector3d  directionFromObj(const App::DocumentObject& breakObj) const;
    Base::Vector3d  guiDirectionFromObj(const App::DocumentObject& breakObj) const;

    static bool isBreakObject(const App::DocumentObject& breakObj);
    static bool isBreakObjectSketch(const App::DocumentObject& breakObj);
    static std::vector<App::DocumentObject*> removeBreakObjects(std::vector<App::DocumentObject*> breaks,
                                                                std::vector<App::DocumentObject*> shapes);
    static std::vector<TopoDS_Edge> edgesFromCompound(const TopoDS_Shape& compound);

    Base::Vector3d mapPoint3dToView(Base::Vector3d point3d) const;
    Base::Vector3d mapPoint2dFromView(Base::Vector3d point2d) const;

    Base::Vector3d getCompressedCentroid() const;
    double breaklineLength(const App::DocumentObject& breakObj) const;



private:
    TopoDS_Shape    breakShape(const TopoDS_Shape& shapeToBreak) const;
    TopoDS_Shape    compressShape(const TopoDS_Shape& shapeToCompress) const;
    TopoDS_Shape    apply1Break(const App::DocumentObject& breakObj, const TopoDS_Shape& inShape) const;
    TopoDS_Shape    makeHalfSpace(const Base::Vector3d& point,
                                  const Base::Vector3d& direction,
                                  const Base::Vector3d& pointInSpace) const;
    std::pair<Base::Vector3d, Base::Vector3d>
                    breakPointsFromSketch(const App::DocumentObject& breakObj) const;
    std::pair<Base::Vector3d, Base::Vector3d>
                    breakPointsFromEdge(const App::DocumentObject& breakObj) const;
    std::pair<Base::Vector3d, Base::Vector3d>
                    breakBoundsFromSketch(const App::DocumentObject& breakObj) const;
    std::pair<Base::Vector3d, Base::Vector3d>
                    breakBoundsFromEdge(const App::DocumentObject& breakObj) const;
    double          removedLengthFromObj(const App::DocumentObject& breakObj) const;
    double          breaklineLengthFromSketch(const App::DocumentObject& breakObj) const;
    double          breaklineLengthFromEdge(const App::DocumentObject& breakObj) const;


    bool isVertical(const TopoDS_Edge& edge, const bool projected = false) const;
    bool isVertical(std::pair<Base::Vector3d, Base::Vector3d>, bool projected = false) const;
    bool isHorizontal(const TopoDS_Edge& edge, const bool projected = false) const;

    TopoDS_Shape compressHorizontal(const TopoDS_Shape& inShape) const;
    TopoDS_Shape compressVertical(const TopoDS_Shape& inShape) const;

    static PieceLimitList getPieceLimits(const std::vector<TopoDS_Shape>& pieces, Base::Vector3d direction);

    double getExpandGaps(double pointCoord,
                         const BreakList& compressedBreakList,
                         Base::Vector3d moveDirection,
                         std::vector<size_t>& fullGaps,
                         int& partialGapIndex) const;

    BreakList makeSortedBreakList(const std::vector<App::DocumentObject*>& breaks,
                                  const Base::Vector3d& direction,
                                  const bool descend = false) const;

    BreakList makeSortedBreakListCompressed(const std::vector<App::DocumentObject*>& breaks,
                                            const Base::Vector3d& moveDirection,
                                            const bool descend = false) const;

    static std::vector<TopoDS_Shape> getPieces(const TopoDS_Shape& brokenShape);
    static std::vector<TopoDS_Shape> getPiecesByType(const TopoDS_Shape& shapeToSearch,
                                                     TopAbs_ShapeEnum desiredShapeType,
                                                     TopAbs_ShapeEnum avoidShapeType = TopAbs_SHAPE);

    static BreakList sortBreaks(BreakList& inList, bool descend = false);
    static bool breakLess(const BreakListEntry& entry0, const BreakListEntry& entry1);

    double shiftAmountShrink(double pointCoord, Base::Vector3d direction, const BreakList& sortedBreaks) const;

    void printBreakList(const std::string& text, const BreakList& inBreaks) const;

    std::pair<Base::Vector3d, Base::Vector3d>
                    scalePair(std::pair<Base::Vector3d, Base::Vector3d> inPair) const;
    Base::Vector3d makePerpendicular(Base::Vector3d inDir) const;

    bool moveThisPiece(PieceLimitEntry piece, BreakListEntry breakItem, Base::Vector3d moveDirection) const;
    bool isDirectionReversed(Base::Vector3d direction) const;

    Base::Vector3d m_unbrokenCenter;
    TopoDS_Shape m_compressedShape;

};

using DrawBrokenViewPython = App::FeaturePythonT<DrawBrokenView>;

}//namespace TechDraw