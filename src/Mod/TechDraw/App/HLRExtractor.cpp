// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Morten Vajhøj
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "HLRExtractor.h"

#include <BRep_Builder.hxx>
#include <BRepLib_MakeEdge2d.hxx>
#include <HLRAlgo_BiPoint.hxx>
#include <HLRAlgo_EdgeIterator.hxx>
#include <HLRAlgo_EdgeStatus.hxx>
#include <HLRBRep.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_Data.hxx>
#include <HLRBRep_EdgeData.hxx>
#include <HLRBRep_FaceIterator.hxx>
#include <HLRBRep_PolyAlgo.hxx>
#include <HLRBRep_ShapeBounds.hxx>
#include <HLRTopoBRep_Data.hxx>
#include <HLRTopoBRep_OutLiner.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Trsf.hxx>
#include <gp_XY.hxx>
#include <gp_XYZ.hxx>

HLRExtractor::HLRExtractor(const Handle(HLRBRep_Algo) & algo)
    : m_algo(algo)
{}



std::vector<HLREdge> HLRExtractor::extract(HLREdgeType type, bool visible)
{
    std::vector<HLREdge> result;
    if (m_algo.IsNull()) {
        return result;
    }

    Handle(HLRBRep_Data) data = m_algo->DataStructure();
    if (data.IsNull()) {
        return result;
    }


    // Note: This function is adapted from HLRBRep_HLRToShape::InternalCompound
    // Small changes made like other variable names for better readability 
    // And removed unnecessary code that we dont need for our use case 
    // Internal compound is a private function in HLRBRep_HLRToShape

    data->Projector().Scaled(Standard_True);

    const Standard_Integer firstEdge = 1;
    const Standard_Integer lastEdge = data->NbEdges();
    const Standard_Integer firstFace = 1;
    const Standard_Integer lastFace = data->NbFaces();

    for (Standard_Integer iEdge = firstEdge; iEdge <= lastEdge; ++iEdge) {
        HLRBRep_EdgeData& edge = data->EDataArray().ChangeValue(iEdge);
        if (edge.Selected() && !edge.Vertical()) {
            edge.Used(false);
            edge.HideCount(0);
        }
        else {
            edge.Used(true);
        }
    }

    for (Standard_Integer iFace = firstFace; iFace <= lastFace; ++iFace) {
        extractFace(visible, type, iFace, data, result);
    }

    if (static_cast<int>(type) >= 3) {
        for (Standard_Integer iEdge = firstEdge; iEdge <= lastEdge; ++iEdge) {
            HLRBRep_EdgeData& edge = data->EDataArray().ChangeValue(iEdge);
            if (!edge.Used()) {
                extractEdge(visible, false, type, iEdge, edge, result);
                edge.Used(Standard_True);
            }
        }
    }

    data->Projector().Scaled(Standard_False);
    return result;
}

void HLRExtractor::extractFace(bool visible,
                                   HLREdgeType hlrType,
                                   int iFace,
                                   Handle(HLRBRep_Data) & data,
                                   std::vector<HLREdge>& result)
{

    // Note: This function is adapted from HLRBRep_HLRToShape::DrawFace
    // Small changes made like other variable names for better readability 
    // The logic is fully the same as the DrawFace function
    // The main difference is that this function does not have any in3d parameter, since we do not need to extract the 3D edges, only the 2D edges

    int type = static_cast<int>(hlrType);

    HLRBRep_FaceIterator faceIterator;
    for (faceIterator.InitEdge(data->FDataArray().ChangeValue(iFace)); faceIterator.MoreEdge(); faceIterator.NextEdge()) {
        const Standard_Integer iEdge = faceIterator.Edge();
        
        HLRBRep_EdgeData& edge = data->EDataArray().ChangeValue(iEdge);
        if (edge.Used()) {
            continue;
        }

        bool toExtract = false;

        if (type == 1) {
            toExtract = faceIterator.IsoLine();
        }
        else if (type == 2) {
            toExtract = faceIterator.Internal();
        }
        else if (type == 3) {
            toExtract = edge.Rg1Line() && !edge.RgNLine() && !faceIterator.OutLine();
        }
        else if (type == 4) {
            toExtract = edge.RgNLine() && !faceIterator.OutLine();
        }
        else if (type == 5) {
            toExtract = !faceIterator.IsoLine() && !faceIterator.Internal() && (!edge.Rg1Line() || faceIterator.OutLine());
        }

        if (toExtract) {
            extractEdge(visible, true, hlrType, iEdge, edge, result);
            edge.Used(Standard_True);
            continue;
        }

        if ((type == 5 || type == 2) && edge.Rg1Line() && !faceIterator.OutLine()) {
            if (edge.HideCount() == 0) {
                edge.HideCount(1);
            }
            else {
                edge.Used(Standard_True);
            }
        }
    }
}

void HLRExtractor::extractEdge(bool visible,
                                   bool inFace,
                                   HLREdgeType hlrType,
                                   int iEdge,
                                   HLRBRep_EdgeData& edge,
                                   std::vector<HLREdge>& result)
{
    // Note: This function is adapted from HLRBRep_HLRToShape::DrawEdge
    // Small changes made like other variable names for better readability 
    // The logic is fully the same as the DrawEdge function
    // The main difference is that this function does not have any in3d parameter, since we do not need to extract the 3D edges, only the 2D edges

    const int type = static_cast<int>(hlrType);

    bool toExtract = false;

    if (inFace) {
        toExtract = true;
    }
    else if (type == 3) {
        toExtract = edge.Rg1Line() && !edge.RgNLine();
    }
    else if (type == 4) {
        toExtract = edge.RgNLine();
    }
    else {
        toExtract = !edge.Rg1Line();
    }

    if (!toExtract) {
        return;
    }

    // Not part of the DrawEdge function
    // This part is what makes the toponaming possible
    HLREdge hlrEdge;
    findSource(iEdge, hlrEdge);

    Standard_Real sta = 0.0;
    Standard_Real end = 0.0;
    Standard_ShortReal tolsta = 0.0;
    Standard_ShortReal tolend = 0.0;

    HLRAlgo_EdgeIterator edgeIterator;
    if (visible) {
        for (edgeIterator.InitVisible(edge.Status()); edgeIterator.MoreVisible(); edgeIterator.NextVisible()) {

            edgeIterator.Visible(sta, tolsta, end, tolend);

            const TopoDS_Edge newEdge = HLRBRep::MakeEdge(edge.Geometry(), sta, end);
            if (newEdge.IsNull()) {
                continue;
            }

            // DrawEdge adds the edge to a BRep_Builder, but we add it to the result vector instead
            hlrEdge.edge = newEdge;
            result.push_back(hlrEdge);
        }
    }
    else {
        for (edgeIterator.InitHidden(edge.Status()); edgeIterator.MoreHidden(); edgeIterator.NextHidden()) {

            edgeIterator.Hidden(sta, tolsta, end, tolend);

            const TopoDS_Edge newEdge = HLRBRep::MakeEdge(edge.Geometry(), sta, end);
            if (newEdge.IsNull()) {
                continue;
            }

            // DrawEdge adds the edge to a BRep_Builder, but we add it to the result vector instead
            hlrEdge.edge = newEdge;
            result.push_back(hlrEdge);
        }
    }
}

void HLRExtractor::findSource(int iEdge, HLREdge& hlrEdge)
{
    // Note: This function is the only part of the HLRExtractor that is not adapted from HLRBRep_HLRToShape
    // This function adds the ability to find the source shape of an edge
    // Which is not possible in HLRBRep_HLRToShape
    // This source shape can be used to find the mapped name of the edge, which is used for toponaming
    // The mapped name gets applied in GeometryObject.cpp

    Handle(HLRBRep_Data) data = m_algo->DataStructure();
    if (data.IsNull() || iEdge < 1 || iEdge > data->EdgeMap().Extent()) {
        return;
    }

    // Edge that we are trying to find the source of
    const TopoDS_Shape edge = data->EdgeMap().FindKey(iEdge);


    const Standard_Integer nbShapes = m_algo->NbShapes();

    // For every shape that has been added to the HLRBrep_Algo
    for (Standard_Integer shape = 1; shape <= nbShapes; ++shape) {
        
        // Check if the edge is within the bounds of the shape
        Standard_Integer v1 {}, v2 {}, e1 {}, e2 {}, f1 {}, f2 {};
        m_algo->ShapeBounds(shape).Bounds(v1, v2, e1, e2, f1, f2);
        
        if (iEdge < e1 || iEdge > e2) {
            continue;
        }

        // Get the outliner for the shape
        const Handle(HLRTopoBRep_OutLiner)& outliner = m_algo->ShapeBounds(shape).Shape();
        if (outliner.IsNull()) {
            return;
        }

        // Get the original shape from the outliner
        const TopoDS_Shape original = outliner->DataStructure().NewSOldS(edge);
        hlrEdge.sourceShape = original;

        // if the original shape is a face or an edge, we can set the sourceType to "Face" or "Edge"
        if (original.ShapeType() == TopAbs_FACE) {
            hlrEdge.sourceType = "Face";
        }
        else if (original.ShapeType() == TopAbs_EDGE) {
            hlrEdge.sourceType = "Edge";
        }

        return;
    }
}

TopoDS_Shape HLRExtractor::toCompound(const std::vector<HLREdge>& hlrEdges)
{
    // small helper function to convert a vector of HLREdge to a TopoDS_Compound
    // used in geometryobject.cpp to convert the extracted edges to a compound for toponaming

    BRep_Builder builder;
    TopoDS_Compound compound;

    builder.MakeCompound(compound);
    for (const auto& hlrEdge : hlrEdges) {
        if (!hlrEdge.edge.IsNull()) {
            builder.Add(compound, hlrEdge.edge);
        }
    }
    return compound;
}


PolyHLRExtractor::PolyHLRExtractor(const Handle(HLRBRep_PolyAlgo) & algo)
    : m_algo(algo)
{
    update();
}

void PolyHLRExtractor::update()
{
    // Note: This function is adapted from HLRBRep_PolyHLRToShape::Update
    // Small changes made like other variable names for better readability
    // Instead of storing HLRBRep_BiPnt2D, every segment keeps the shape it came from

    m_segments.clear();
    if (m_algo.IsNull()) {
        return;
    }

    const gp_Trsf& transform = m_algo->Projector().Transformation();

    HLRAlgo_EdgeStatus status;
    TopoDS_Shape original;
    Standard_Boolean reg1 = Standard_False;
    Standard_Boolean regn = Standard_False;
    Standard_Boolean outl = Standard_False;
    Standard_Boolean intl = Standard_False;

    Standard_Real sta = 0.0;
    Standard_Real end = 0.0;
    Standard_ShortReal tolsta = 0.0;
    Standard_ShortReal tolend = 0.0;

    HLRAlgo_EdgeIterator edgeIterator;

    for (m_algo->InitHide(); m_algo->MoreHide(); m_algo->NextHide()) {
        HLRAlgo_BiPoint::PointsT& points = m_algo->Hide(status, original, reg1, regn, outl, intl);

        gp_XYZ start3d = points.Pnt1;
        gp_XYZ finish3d = points.Pnt2;
        transform.Transforms(start3d);
        transform.Transforms(finish3d);

        const gp_XY start(start3d.X(), start3d.Y());
        const gp_XY finish(finish3d.X(), finish3d.Y());
        const gp_XY delta = finish - start;

        if (delta.Modulus() <= 1.e-10) {
            continue;
        }

        Segment segment;
        segment.reg1 = reg1;
        segment.regn = regn;
        segment.outl = outl;
        segment.intl = intl;

        // Not part of the Update function
        // This part is what makes the toponaming possible
        findSource(original, segment.hlrEdge);

        for (edgeIterator.InitVisible(status); edgeIterator.MoreVisible(); edgeIterator.NextVisible()) {
            edgeIterator.Visible(sta, tolsta, end, tolend);
            extractSegment(segment, true, start + sta * delta, start + end * delta);
        }

        for (edgeIterator.InitHidden(status); edgeIterator.MoreHidden(); edgeIterator.NextHidden()) {
            edgeIterator.Hidden(sta, tolsta, end, tolend);
            extractSegment(segment, false, start + sta * delta, start + end * delta);
        }
    }
}

void PolyHLRExtractor::extractSegment(Segment segment,
                                      bool visible,
                                      const gp_XY& start,
                                      const gp_XY& finish)
{
    const gp_Pnt2d first(start);
    const gp_Pnt2d last(finish);
    if (first.SquareDistance(last) <= 1.e-20) {
        return;
    }

    BRepLib_MakeEdge2d makeEdge(first, last);
    if (!makeEdge.IsDone()) {
        return;
    }

    segment.hlrEdge.edge = makeEdge.Edge();
    segment.visible = visible;
    m_segments.push_back(segment);
}

void PolyHLRExtractor::findSource(const TopoDS_Shape& original, HLREdge& hlrEdge)
{
    hlrEdge.sourceShape = original;
    if (original.IsNull()) {
        return;
    }

    if (original.ShapeType() == TopAbs_FACE) {
        hlrEdge.sourceType = "Face";
    }
    else if (original.ShapeType() == TopAbs_EDGE) {
        hlrEdge.sourceType = "Edge";
    }
}

std::vector<HLREdge> PolyHLRExtractor::extract(HLREdgeType type, bool visible) const
{
    // Note: This function is adapted from HLRBRep_PolyHLRToShape::InternalCompound
    // The filters are the same, but we return the tagged edges instead of a compound

    std::vector<HLREdge> result;

    for (const auto& segment : m_segments) {
        if (segment.visible != visible) {
            continue;
        }

        bool toExtract = false;
        switch (type) {
            case HLREdgeType::Outline:
                toExtract = segment.intl;
                break;
            case HLREdgeType::Smooth:
                toExtract = segment.reg1 && !segment.regn && !segment.outl;
                break;
            case HLREdgeType::Seam:
                toExtract = segment.regn && !segment.outl;
                break;
            case HLREdgeType::Hard:
                toExtract = !(segment.intl || (segment.reg1 && !segment.outl));
                break;
            default:  // Iso: the polygon algorithm never produces one
                toExtract = false;
                break;
        }

        if (toExtract) {
            result.push_back(segment.hlrEdge);
        }
    }

    return result;
}

TopoDS_Shape PolyHLRExtractor::toCompound(const std::vector<HLREdge>& hlrEdges)
{

    if (hlrEdges.empty()) {
        return {};
    }

    BRep_Builder builder;
    TopoDS_Compound compound;

    builder.MakeCompound(compound);
    for (const auto& hlrEdge : hlrEdges) {
        if (!hlrEdge.edge.IsNull()) {
            builder.Add(compound, hlrEdge.edge);
        }
    }
    return compound;
}