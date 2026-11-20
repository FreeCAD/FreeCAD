// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

//! a class to the projection of shapes, removal/identifying hidden lines and
//! converting the output for OCC HLR into the BaseGeom intermediate representation.


#include <BRepAdaptor_Curve.hxx>
#include <BRepAlgo_NormalProjection.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepLProp_CLProps.hxx>
#include <BRepLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <HLRBRep_PolyAlgo.hxx>
#include <NCollection_DataMap.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <chrono>
#include <set>

#include <Base/Console.h>
#include <Mod/Part/App/PartFeature.h>

#include "Cosmetic.h"
#include "DrawUtil.h"
#include "DrawViewDetail.h"
#include "DrawViewPart.h"
#include "GeometryObject.h"
#include "DrawProjectSplit.h"
#include "ShapeUtils.h"

using namespace TechDraw;
using namespace std;

using DU = DrawUtil;

GeometryObject::GeometryObject(const string& parent, TechDraw::DrawView* parentObj)
    : m_parentName(parent), m_parent(parentObj), m_isoCount(0), m_isPersp(false), m_focus(100.0),
      m_usePolygonHLR(false), m_scrubCount(0)

{}

GeometryObject::~GeometryObject() { clear(); }

const BaseGeomPtrVector GeometryObject::getVisibleFaceEdges(const bool smooth,
                                                            const bool seam) const
{
    BaseGeomPtrVector result;
    bool smoothOK = smooth;
    bool seamOK = seam;

    for (auto& e : edgeGeom) {
        if (e->getHlrVisible()) {
            switch (e->getClassOfEdge()) {
                case EdgeClass::HARD:
                case EdgeClass::OUTLINE:
                    result.push_back(e);
                    break;
                case EdgeClass::SMOOTH:
                    if (smoothOK) {
                        result.push_back(e);
                    }
                    break;
                case EdgeClass::SEAM:
                    if (seamOK) {
                        result.push_back(e);
                    }
                    break;
                default:;
            }
        }
    }
    //debug
    //make compound of edges and save as brep file
    //    BRep_Builder builder;
    //    TopoDS_Compound comp;
    //    builder.MakeCompound(comp);
    //    for (auto& r: result) {
    //        builder.Add(comp, r->getOCCEdge());
    //    }
    //    BRepTools::Write(comp, "GOVizFaceEdges.brep");            //debug

    return result;
}


void GeometryObject::clear()
{
    //shared pointers will delete v/e/f when reference counts go to zero.

    vertexGeom.clear();
    faceGeom.clear();
    edgeGeom.clear();
}

HLRAlgo_Projector GeometryObject::getProjector(const gp_Ax2& viewAxis) const
{
    if (m_isPersp) {
        double fLength = std::max(Precision::Confusion(), m_focus);
        return HLRAlgo_Projector(viewAxis, fLength);
    }

    return HLRAlgo_Projector(viewAxis);
}

void GeometryObject::projectShape(const TopoDS_Shape& inPartShape, const gp_Ax2& viewAxis)
{
    clear();

    m_partShape = inPartShape;
    TopoDS_Shape inShape = inPartShape;

    Handle(HLRBRep_Algo) brep_hlr;
    try {
        brep_hlr = new HLRBRep_Algo();
        //        brep_hlr->Debug(true);
        brep_hlr->Add(inShape, m_isoCount);
        brep_hlr->Projector(getProjector(viewAxis));
        brep_hlr->Update();
        brep_hlr->Hide();
    }
    catch (const Standard_Failure& e) {
        Base::Console().error("GO::projectShape - OCC error - {} - while projecting shape\n",
                              e.GetMessageString());
        throw Base::RuntimeError("GeometryObject::projectShape - OCC error");
    }
    catch (...) {
        throw Base::RuntimeError("GeometryObject::projectShape - unknown error");
    }

    try {

        TopTools_IndexedMapOfShape inputEdgeMap;
        TopExp::MapShapes(inShape, TopAbs_EDGE, inputEdgeMap);
        TopTools_IndexedMapOfShape inputFaceMap;
        TopExp::MapShapes(inShape, TopAbs_FACE, inputFaceMap);

        HLRExtractor extractor(brep_hlr);

        auto extractEdges = [&](HLREdgeType type, bool visible, TopoDS_Shape& compound,
                                    std::vector<EdgeSegment>& segments) {
            
            std::vector<HLREdge> hlrEdges = extractor.extract(type, visible);
            compound = extractor.toCompound(hlrEdges);
            segments = buildSegmentsFromTagged(hlrEdges, inputEdgeMap, inputFaceMap);
        };

        // All visible edges
        extractEdges(HLREdgeType::Hard, true, visHard, m_visHardSegments);
        extractEdges(HLREdgeType::Smooth, true, visSmooth, m_visSmoothSegments);
        extractEdges(HLREdgeType::Seam, true, visSeam, m_visSeamSegments);
        extractEdges(HLREdgeType::Iso, true, visIso, m_visIsoSegments);
        extractEdges(HLREdgeType::Outline, true, visOutline, m_visOutlineSegments);

        // All hidden edges
        extractEdges(HLREdgeType::Hard, false, hidHard, m_hidHardSegments);
        extractEdges(HLREdgeType::Smooth, false, hidSmooth, m_hidSmoothSegments);
        extractEdges(HLREdgeType::Seam, false, hidSeam, m_hidSeamSegments);
        extractEdges(HLREdgeType::Iso, false, hidIso, m_hidIsoSegments);
        extractEdges(HLREdgeType::Outline, false, hidOutline, m_hidOutlineSegments);

        buildAndInvert(visHard);
        buildAndInvert(visSmooth);
        buildAndInvert(visSeam);
        buildAndInvert(visIso);
        buildAndInvert(visOutline);
        buildAndInvert(hidHard);
        buildAndInvert(hidSmooth);
        buildAndInvert(hidSeam);
        buildAndInvert(hidIso);
        buildAndInvert(hidOutline);
    }
    catch (const Standard_Failure&) {
        throw Base::RuntimeError(
            "GeometryObject::projectShape - OCC error occurred while extracting edges");
    }
    catch (...) {
        throw Base::RuntimeError(
            "GeometryObject::projectShape - unknown error occurred while extracting edges");
    }

    makeTDGeometry();
}

void GeometryObject::buildAndInvert(TopoDS_Shape& shape)
{
    if (!shape.IsNull()) {
        BRepLib::BuildCurves3d(shape);
        shape = ShapeUtils::invertGeometry(shape);
    }
}


std::vector<TechDraw::GeometryObject::EdgeSegment>
GeometryObject::buildSegmentsFromTagged(const std::vector<HLREdge>& hlrEdges,
                                        const TopTools_IndexedMapOfShape& edgeMap,
                                        const TopTools_IndexedMapOfShape& faceMap)
{
    std::vector<EdgeSegment> segments;
    segments.reserve(hlrEdges.size());

    std::map<std::string, int> segmentCounter;

    // for each edge find the toponame of the source shape and store it in the segment
    for (const auto& hlrEdge : hlrEdges) {
        EdgeSegment segment;
        segment.edge = hlrEdge.edge;
        segment.number = 0;

        int index = 0;
        if (hlrEdge.sourceType == "Edge") {

            // since we have the source shape from outliner, we can find the index of the edge in the original shape
            index = edgeMap.FindIndex(hlrEdge.sourceShape);

            if (index > 0) {
                segment.parentName = "Edge" + std::to_string(index);

                Data::MappedName mapped = m_partShape.getMappedName(
                    Data::IndexedName::fromConst("Edge", index), false);
                segment.mappedName = mapped.toString();
            }
        }
        else if (hlrEdge.sourceType == "Face") {

            index = faceMap.FindIndex(hlrEdge.sourceShape);

            if (index > 0) {
                segment.parentName = "Face" + std::to_string(index);

                Data::MappedName mapped = m_partShape.getMappedName(
                    Data::IndexedName::fromConst("Face", index), false);
                segment.mappedName = mapped.toString();
            }
        }

        // since multiple edges can come from the same source shape, we also keep track of a segment number
        if (index > 0) {
            int count = segmentCounter[segment.parentName]++;
            segment.number = count + 1;
        }

        segments.push_back(segment);
    }

    return segments;
}

//convert the hlr output into TD Geometry
void GeometryObject::makeTDGeometry()
{
//    Base::Console().message("GO::makeTDGeometry()\n");
    extractGeometry(EdgeClass::HARD,                   //always show the hard&outline visible lines
                        true);
    extractGeometry(EdgeClass::OUTLINE,
                        true);

    const DrawViewPart* dvp = static_cast<const DrawViewPart*>(m_parent);
    if (!dvp) {
        return;//some routines do not have a dvp (ex shape outline)
    }

    if (dvp->SmoothVisible.getValue()) {
        extractGeometry(EdgeClass::SMOOTH, true);
    }
    if (dvp->SeamVisible.getValue()) {
        extractGeometry(EdgeClass::SEAM, true);
    }
    if ((dvp->IsoVisible.getValue()) && (dvp->IsoCount.getValue() > 0)) {
        extractGeometry(EdgeClass::UVISO, true);
    }
    if (dvp->HardHidden.getValue()) {
        extractGeometry(EdgeClass::HARD, false);
        extractGeometry(EdgeClass::OUTLINE, false);
    }
    if (dvp->SmoothHidden.getValue()) {
        extractGeometry(EdgeClass::SMOOTH, false);
    }
    if (dvp->SeamHidden.getValue()) {
        extractGeometry(EdgeClass::SEAM, false);
    }
    if (dvp->IsoHidden.getValue() && (dvp->IsoCount.getValue() > 0)) {
        extractGeometry(EdgeClass::UVISO, false);
    }
}


//!set up a hidden line remover and project a shape with it
void GeometryObject::projectShapeWithPolygonAlgo(const TopoDS_Shape& input, const gp_Ax2& viewAxis)
{
//    Base::Console().message("GO::projectShapeWithPolygonAlgo()\n");
    // Clear previous Geometry
    clear();

    m_partShape = Part::TopoShape(input);
    TopoDS_Shape inShape = input;

    //work around for Mantis issue #3332
    //if 3332 gets fixed in OCC, this will produce shifted views and will need
    //to be reverted.
    TopoDS_Shape inCopy;
    if (!m_isPersp) {
        gp_Pnt gCenter = ShapeUtils::findCentroid(inShape, viewAxis);
        Base::Vector3d motion(-gCenter.X(), -gCenter.Y(), -gCenter.Z());
        inCopy = ShapeUtils::moveShape(inShape, motion);
    }
    else {
        BRepBuilderAPI_Copy BuilderCopy(inShape);
        inCopy = BuilderCopy.Shape();
    }

    Handle(HLRBRep_PolyAlgo) brep_hlrPoly;

    try {
        // HLRBRep_PolyAlgo will fail if the whole input shape has not been meshed.
        // meshing the faces is not sufficient.
        BRepMesh_IncrementalMesh(inCopy, 0.10);

        brep_hlrPoly = new HLRBRep_PolyAlgo();
        brep_hlrPoly->Load(inCopy);

        if (m_isPersp) {
            double fLength = std::max(Precision::Confusion(), m_focus);
            HLRAlgo_Projector projector(viewAxis, fLength);
            brep_hlrPoly->Projector(projector);
        }
        else {// non perspective
            HLRAlgo_Projector projector(viewAxis);
            brep_hlrPoly->Projector(projector);
        }
        brep_hlrPoly->Update();
    }
    catch (const Standard_Failure& e) {
        Base::Console().error(
            "GO::projectShapeWithPolygonAlgo - OCC error - {} - while projecting shape\n",
            e.GetMessageString());
        throw Base::RuntimeError("GeometryObject::projectShapeWithPolygonAlgo - OCC error");
    }
    catch (...) {
        throw Base::RuntimeError("GeometryObject::projectShapeWithPolygonAlgo - unknown error");
    }

    try {
        TopTools_IndexedMapOfShape inputEdgeMap;
        TopExp::MapShapes(inCopy, TopAbs_EDGE, inputEdgeMap);
        TopTools_IndexedMapOfShape inputFaceMap;
        TopExp::MapShapes(inCopy, TopAbs_FACE, inputFaceMap);

        PolyHLRExtractor extractor(brep_hlrPoly);

        auto extractEdges = [&](HLREdgeType type, bool visible, TopoDS_Shape& compound,
                                std::vector<EdgeSegment>& segments) {
            std::vector<HLREdge> hlrEdges = extractor.extract(type, visible);
            compound = extractor.toCompound(hlrEdges);
            segments = buildSegmentsFromTagged(hlrEdges, inputEdgeMap, inputFaceMap);
        };

        // All visible edges
        extractEdges(HLREdgeType::Hard, true, visHard, m_visHardSegments);
        extractEdges(HLREdgeType::Smooth, true, visSmooth, m_visSmoothSegments);
        extractEdges(HLREdgeType::Seam, true, visSeam, m_visSeamSegments);
        extractEdges(HLREdgeType::Iso, true, visIso, m_visIsoSegments);
        extractEdges(HLREdgeType::Outline, true, visOutline, m_visOutlineSegments);

        // All hidden edges
        extractEdges(HLREdgeType::Hard, false, hidHard, m_hidHardSegments);
        extractEdges(HLREdgeType::Smooth, false, hidSmooth, m_hidSmoothSegments);
        extractEdges(HLREdgeType::Seam, false, hidSeam, m_hidSeamSegments);
        extractEdges(HLREdgeType::Iso, false, hidIso, m_hidIsoSegments);
        extractEdges(HLREdgeType::Outline, false, hidOutline, m_hidOutlineSegments);

        buildAndInvert(visHard);
        buildAndInvert(visSmooth);
        buildAndInvert(visSeam);
        buildAndInvert(visIso);
        buildAndInvert(visOutline);
        buildAndInvert(hidHard);
        buildAndInvert(hidSmooth);
        buildAndInvert(hidSeam);
        buildAndInvert(hidIso);
        buildAndInvert(hidOutline);
    }
    catch (const Standard_Failure& e) {
        Base::Console().error(
            "GO::projectShapeWithPolygonAlgo - OCC error - {} - while extracting edges\n",
            e.GetMessageString());
        throw Base::RuntimeError("GeometryObject::projectShapeWithPolygonAlgo - OCC error occurred "
                                 "while extracting edges");
    }
    catch (...) {
        throw Base::RuntimeError("GeometryObject::projectShapeWithPolygonAlgo - unknown error "
                                 "occurred while extracting edges");
    }

    makeTDGeometry();
}

//project the edges in shape onto XY.mirrored plane of CS.  mimics the projection
//of the main hlr routine. Only the visible hard edges are returned, so this method
//is only suitable for simple shapes that have no hidden edges, like faces or wires.
//TODO: allow use of perspective projector
TopoDS_Shape GeometryObject::projectSimpleShape(const TopoDS_Shape& shape, const gp_Ax2& CS, bool invertYRequired)
{
    //    Base::Console().message("GO::()\n");
    if (shape.IsNull()) {
        throw Base::ValueError("GO::projectSimpleShape - input shape is NULL");
    }

    HLRBRep_Algo* brep_hlr = new HLRBRep_Algo();
    brep_hlr->Add(shape);
    HLRAlgo_Projector projector(CS);
    brep_hlr->Projector(projector);
    brep_hlr->Update();
    brep_hlr->Hide();

    HLRBRep_HLRToShape hlrToShape(brep_hlr);
    TopoDS_Shape hardEdges = hlrToShape.VCompound();
    BRepLib::BuildCurves3d(hardEdges);
    if (invertYRequired) {
        hardEdges =ShapeUtils::invertGeometry(hardEdges);
    }

    return hardEdges;
}

//project the edges of a shape onto the XY plane of projCS. This does not give
//the same result as the hlr projections
TopoDS_Shape GeometryObject::simpleProjection(const TopoDS_Shape& shape, const gp_Ax2& projCS)
{
    gp_Pln plane(projCS);
    TopoDS_Face paper = BRepBuilderAPI_MakeFace(plane);
    BRepAlgo_NormalProjection projector(paper);
    projector.Add(shape);
    projector.Build();
    return projector.Projection();
}

TopoDS_Shape GeometryObject::projectFace(const TopoDS_Shape& face, const gp_Ax2& CS)
{
    //    Base::Console().message("GO::projectFace()\n");
    if (face.IsNull()) {
        throw Base::ValueError("GO::projectFace - input Face is NULL");
    }

    HLRBRep_Algo* brep_hlr = new HLRBRep_Algo();
    brep_hlr->Add(face);
    HLRAlgo_Projector projector(CS);
    brep_hlr->Projector(projector);
    brep_hlr->Update();
    brep_hlr->Hide();

    HLRBRep_HLRToShape hlrToShape(brep_hlr);
    TopoDS_Shape hardEdges = hlrToShape.VCompound();
    BRepLib::BuildCurves3d(hardEdges);
    hardEdges =ShapeUtils::invertGeometry(hardEdges);

    return hardEdges;
}

//!add edges meeting filter criteria for category, visibility
void GeometryObject::extractGeometry(EdgeClass category, bool hlrVisible)
{
    //    Base::Console().message("GO::extractGeometry({}, {})\n", category, hlrVisible);
    TopoDS_Shape filtEdges;
    std::vector<EdgeSegment> filtNames;
    if (hlrVisible) {
        switch (category) {
            case EdgeClass::HARD:
                filtEdges = visHard;
                filtNames = m_visHardSegments;
                break;
            case EdgeClass::OUTLINE:
                filtEdges = visOutline;
                filtNames = m_visOutlineSegments;
                break;
            case EdgeClass::SMOOTH:
                filtEdges = visSmooth;
                filtNames = m_visSmoothSegments;
                break;
            case EdgeClass::SEAM:
                filtEdges = visSeam;
                filtNames = m_visSeamSegments;
                break;
            case EdgeClass::UVISO:
                filtEdges = visIso;
                filtNames = m_visIsoSegments;
                break;
            default:
                Base::Console().warning(
                    "GeometryObject::ExtractGeometry - unsupported hlrVisible EdgeClass: {}\n",
                    static_cast<int>(category));
                return;
        }
    }
    else {
        switch (category) {
            case EdgeClass::HARD:
                filtEdges = hidHard;
                filtNames = m_hidHardSegments;
                break;
            case EdgeClass::OUTLINE:
                filtEdges = hidOutline;
                filtNames = m_hidOutlineSegments;
                break;
            case EdgeClass::SMOOTH:
                filtEdges = hidSmooth;
                filtNames = m_hidSmoothSegments;
                break;
            case EdgeClass::SEAM:
                filtEdges = hidSeam;
                filtNames = m_hidSeamSegments;
                break;
            case EdgeClass::UVISO:
                filtEdges = hidIso;
                filtNames = m_hidIsoSegments;
                break;
            default:
                Base::Console().warning(
                    "GeometryObject::ExtractGeometry - unsupported hidden EdgeClass: {}\n",
                    static_cast<int>(category));
                return;
        }
    }

    addGeomFromCompound(filtEdges, category, hlrVisible, filtNames);
}

//! update edgeGeom and vertexGeom from Compound of edges
void GeometryObject::addGeomFromCompound(TopoDS_Shape edgeCompound, EdgeClass category,
                                         bool hlrVisible, const std::vector<EdgeSegment>& segments)
{
    if (edgeCompound.IsNull()) {
        return;    // There is no OpenCascade Geometry to be calculated
    }


    TopoDS_Shape cleanShape;
    std::vector<EdgeSegment> edgeSegments = segments;

    // remove overlapping edges
    if (m_scrubCount > 0) {
        std::vector<TopoDS_Edge> edgeVector = DU::shapeToVector(edgeCompound);
        std::vector<TopoDS_Edge> originalEdgeVector = edgeVector;
        for (int iPass = 0; iPass < m_scrubCount; iPass++)  {
            edgeVector = DrawProjectSplit::removeOverlapEdges(edgeVector);
        }

        edgeSegments.clear();
        for (auto& edge : edgeVector) {
            EdgeSegment matchedSegment;
            for (size_t iOld = 0; iOld < originalEdgeVector.size(); iOld++) {
                if (edge.IsSame(originalEdgeVector.at(iOld))) {
                    matchedSegment = segments.at(iOld);
                    break;
                }
            }
            edgeSegments.push_back(matchedSegment);
        }

        bool invertResult = false;
        cleanShape = DU::vectorToCompound(edgeVector, invertResult);

    } else {
        cleanShape = edgeCompound;
    }

    int edgeType = static_cast<int>(category);

    BaseGeomPtr base;
    TopExp_Explorer edges(cleanShape, TopAbs_EDGE);
    int i = 1;
    for (; edges.More(); edges.Next(), i++) {
        const TopoDS_Edge& edge = TopoDS::Edge(edges.Current());
        if (edge.IsNull()) {
            continue;
        }
        if (DU::isZeroEdge(edge)) {
            continue;
        }
        if (DU::isCrazy(edge)) {
            continue;
        }

        base = BaseGeom::baseFactory(edge);
        if (!base) {
            continue;
        }

        EdgeSegment segment = edgeSegments.at(i - 1);


        base->source(SourceType::GEOMETRY);
        base->sourceIndex(i - 1);
        base->setClassOfEdge(category);
        base->setHlrVisible(hlrVisible);
        base->setMappedName(segment.mappedName);
        base->setSegmentType(edgeType);
        base->setSegmentNumber(segment.number);
        edgeGeom.push_back(base);

        //add vertices of new edge if not already in list
        // note that if a vertex belongs to both a hidden and a visible edge, it will be treated as
        // a visible vertex.
        BaseGeomPtr lastAdded = edgeGeom.back();
        bool v1Add = true, v2Add = true;
        bool c1Add = true;
        TechDraw::VertexPtr v1 = std::make_shared<TechDraw::Vertex>(lastAdded->getStartPoint());
        TechDraw::VertexPtr v2 = std::make_shared<TechDraw::Vertex>(lastAdded->getEndPoint());
        TechDraw::CirclePtr circle = std::dynamic_pointer_cast<TechDraw::Circle>(lastAdded);
        TechDraw::VertexPtr c1;
        if (circle) {
            c1 = std::make_shared<TechDraw::Vertex>(circle->center);
            c1->isCenter(true);
            c1->setHlrVisible(hlrVisible);
        }

        std::vector<VertexPtr>::iterator itVertex = vertexGeom.begin();
        for (; itVertex != vertexGeom.end(); itVertex++) {
            if ((*itVertex)->isEqual(*v1, Precision::Confusion())) {
                v1Add = false;
                // Append the segment mapped name to the existing vertex
                (*itVertex)->setMappedNames(segment.mappedName);
                (*itVertex)->setSegmentTypes(edgeType);
                (*itVertex)->setSegmentNumbers(segment.number);
            }
            if ((*itVertex)->isEqual(*v2, Precision::Confusion())) {
                v2Add = false;
                (*itVertex)->setMappedNames(segment.mappedName);
                (*itVertex)->setSegmentTypes(edgeType);
                (*itVertex)->setSegmentNumbers(segment.number);
            }
            if (circle) {
                if ((*itVertex)->isEqual(*c1, Precision::Confusion())) {
                    c1Add = false;
                    (*itVertex)->setMappedNames(segment.mappedName);
                    (*itVertex)->setSegmentTypes(edgeType);
                    (*itVertex)->setSegmentNumbers(segment.number);
                }
            }
        }
        if (v1Add) {
            vertexGeom.push_back(v1);
            v1->setHlrVisible(hlrVisible);
            v1->setMappedNames(segment.mappedName);
            v1->setSegmentTypes(edgeType);
            v1->setSegmentNumbers(segment.number);
        }
        else {
            //    delete v1;
        }
        if (v2Add) {
            vertexGeom.push_back(v2);
            v2->setHlrVisible(hlrVisible);
            v2->setMappedNames(segment.mappedName);
            v2->setSegmentTypes(edgeType);
            v2->setSegmentNumbers(segment.number);
        }
        else {
            //    delete v2;
        }

        if (circle) {
            if (c1Add) {
                vertexGeom.push_back(c1);
                c1->setHlrVisible(hlrVisible);
                c1->setMappedNames(segment.mappedName);
                c1->setSegmentTypes(edgeType);
                c1->setSegmentNumbers(segment.number);
            }
            else {
                //    delete c1;
            }
        }
    // }
    }//end TopExp
}

void GeometryObject::addVertex(TechDraw::VertexPtr v) { vertexGeom.push_back(v); }

void GeometryObject::addEdge(TechDraw::BaseGeomPtr bg) { edgeGeom.push_back(bg); }

//********** Cosmetic Vertex ***************************************************

//adds a new GeomVert surrogate for CV
//returns GeomVert selection index  ("Vertex3")
// insertGeomForCV(cv)
// is this ever used?
int GeometryObject::addCosmeticVertex(CosmeticVertex* cv)
{
    double scale = m_parent->getScale();
    Base::Vector3d pos = cv->scaled(scale);
    TechDraw::VertexPtr v(std::make_shared<TechDraw::Vertex>(pos.x, pos.y));
    v->setCosmetic(true);
    v->setCosmeticTag(cv->getTagAsString());
    v->setHlrVisible(true);
    int idx = vertexGeom.size();
    vertexGeom.push_back(v);
    return idx;
}

//adds a new GeomVert to list
//should probably be called addVertex since not connect to CV by tag
int GeometryObject::addCosmeticVertex(Base::Vector3d pos)
{
    TechDraw::VertexPtr v(std::make_shared<TechDraw::Vertex>(pos.x, pos.y));
    v->setCosmetic(true);
    v->setCosmeticTag("tbi");//not connected to CV
    v->setHlrVisible(true);
    int idx = vertexGeom.size();
    vertexGeom.push_back(v);
    return idx;
}

int GeometryObject::addCosmeticVertex(Base::Vector3d pos, std::string tagString)
{
    TechDraw::VertexPtr v(std::make_shared<TechDraw::Vertex>(pos.x, pos.y));
    v->setCosmetic(true);
    v->setCosmeticTag(tagString);//connected to CV
    v->setHlrVisible(true);
    int idx = vertexGeom.size();
    vertexGeom.push_back(v);
    return idx;
}

//********** Cosmetic Edge *****************************************************

//adds a new GeomEdge surrogate for CE
//returns GeomEdge selection index  ("Edge3")
// insertGeomForCE(ce)
int GeometryObject::addCosmeticEdge(CosmeticEdge* ce)
{
    //    Base::Console().message("GO::addCosmeticEdge({:X}) 0\n", ce);
    double scale = m_parent->getScale();
    TechDraw::BaseGeomPtr e = ce->scaledGeometry(scale);
    e->setCosmetic(true);
    e->setCosmeticTag(ce->getTagAsString());
    e->setHlrVisible(true);
    int idx = edgeGeom.size();
    edgeGeom.push_back(e);
    return idx;
}

//adds a new GeomEdge to list for ce[link]
//this should be made obsolete and the variant with tag used instead
int GeometryObject::addCosmeticEdge(Base::Vector3d start, Base::Vector3d end)
{
    //    Base::Console().message("GO::addCosmeticEdge() 1 - deprec?\n");
    gp_Pnt gp1(start.x, start.y, start.z);
    gp_Pnt gp2(end.x, end.y, end.z);
    TopoDS_Edge occEdge = BRepBuilderAPI_MakeEdge(gp1, gp2);
    TechDraw::BaseGeomPtr e = BaseGeom::baseFactory(occEdge);
    e->setCosmetic(true);
    //    e->cosmeticLink = link;
    e->setCosmeticTag("tbi");
    e->setHlrVisible(true);
    int idx = edgeGeom.size();
    edgeGeom.push_back(e);
    return idx;
}

int GeometryObject::addCosmeticEdge(Base::Vector3d start, Base::Vector3d end, std::string tagString)
{
    //    Base::Console().message("GO::addCosmeticEdge() 2\n");
    gp_Pnt gp1(start.x, start.y, start.z);
    gp_Pnt gp2(end.x, end.y, end.z);
    TopoDS_Edge occEdge = BRepBuilderAPI_MakeEdge(gp1, gp2);
    TechDraw::BaseGeomPtr base = BaseGeom::baseFactory(occEdge);
    base->setCosmetic(true);
    base->setCosmeticTag(tagString);
    base->source(SourceType::COSMETICEDGE);
    base->setHlrVisible(true);
    int idx = edgeGeom.size();
    edgeGeom.push_back(base);
    return idx;
}

int GeometryObject::addCosmeticEdge(TechDraw::BaseGeomPtr base, std::string tagString)
{
    //    Base::Console().message("GO::addCosmeticEdge({:X}, {}) 3\n", base, tagString);
    base->setCosmetic(true);
    base->setHlrVisible(true);
    base->source(SourceType::COSMETICEDGE);
    base->setCosmeticTag(tagString);
    base->sourceIndex(-1);
    int idx = edgeGeom.size();
    edgeGeom.push_back(base);
    return idx;
}

int GeometryObject::addCenterLine(TechDraw::BaseGeomPtr base, std::string tag)
//                                    int s, int si)
{
    //    Base::Console().message("GO::addCenterLine()\n");
    base->setCosmetic(true);
    base->setCosmeticTag(tag);
    base->source(SourceType::CENTERLINE);
    //    base->sourceIndex(si);     //index into source;
    int idx = edgeGeom.size();
    edgeGeom.push_back(base);
    return idx;
}


//! empty Face geometry
void GeometryObject::clearFaceGeom() { faceGeom.clear(); }

//! add a Face to Face Geometry
void GeometryObject::addFaceGeom(FacePtr f) { faceGeom.push_back(f); }

TechDraw::DrawViewDetail* GeometryObject::isParentDetail()
{
    if (!m_parent) {
        return nullptr;
    }
    TechDraw::DrawViewDetail* detail = dynamic_cast<TechDraw::DrawViewDetail*>(m_parent);
    return detail;
}


bool GeometryObject::isWithinArc(double theta, double first, double last, bool cw) const
{
    using std::numbers::pi;

    if (fabs(last - first) >= 2 * pi) {
        return true;
    }

    // Put params within [0, 2*pi) - not totally sure this is necessary
    theta = fmod(theta, 2 * pi);
    if (theta < 0) {
        theta += 2 * pi;
    }

    first = fmod(first, 2 * pi);
    if (first < 0) {
        first += 2 * pi;
    }

    last = fmod(last, 2 * pi);
    if (last < 0) {
        last += 2 * pi;
    }

    if (cw) {
        if (first > last) {
            return theta <= first && theta >= last;
        }
        else {
            return theta <= first || theta >= last;
        }
    }
    else {
        if (first > last) {
            return theta >= first || theta <= last;
        }
        else {
            return theta >= first && theta <= last;
        }
    }
}

//note bbx is scaled
Base::BoundBox3d GeometryObject::calcBoundingBox() const
{
    //    Base::Console().message("GO::calcBoundingBox() - edges: {}\n", edgeGeom.size());
    Bnd_Box testBox;
    testBox.SetGap(0.0);
    if (!edgeGeom.empty()) {
        for (BaseGeomPtrVector::const_iterator it(edgeGeom.begin()); it != edgeGeom.end(); ++it) {
            BRepBndLib::AddOptimal((*it)->getOCCEdge(), testBox);
        }
    }

    double xMin = 0, xMax = 0, yMin = 0, yMax = 0, zMin = 0, zMax = 0;
    if (!testBox.IsVoid()) {
        testBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    }
    Base::BoundBox3d bbox(xMin, yMin, zMin, xMax, yMax, zMax);
    return bbox;
}

void GeometryObject::pruneVertexGeom(Base::Vector3d center, double radius)
{
    const std::vector<VertexPtr>& oldVerts = getVertexGeometry();
    std::vector<VertexPtr> newVerts;
    for (auto& v : oldVerts) {
        Base::Vector3d v3 = v->point();
        double length = (v3 - center).Length();
        if (length < Precision::Confusion()) {
            continue;
        }
        else if (length < radius) {
            newVerts.push_back(v);
        }
    }
    vertexGeom = newVerts;
}

//! does this GeometryObject already have this vertex
bool GeometryObject::findVertex(Base::Vector3d v)
{
    std::vector<VertexPtr>::iterator it = vertexGeom.begin();
    for (; it != vertexGeom.end(); it++) {
        double dist = (v - (*it)->point()).Length();
        if (dist < Precision::Confusion()) {
            return true;
        }
    }
    return false;
}