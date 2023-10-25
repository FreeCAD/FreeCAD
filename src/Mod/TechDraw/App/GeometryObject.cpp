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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <BRepAlgo_NormalProjection.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepLProp_CLProps.hxx>
#include <BRepLProp_CurveTool.hxx>
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
#include <HLRBRep_PolyHLRToShape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
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
#endif// #ifndef _PreComp_

#include <algorithm>
#include <chrono>

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
                case ecHARD:
                case ecOUTLINE:
                    result.push_back(e);
                    break;
                case ecSMOOTH:
                    if (smoothOK) {
                        result.push_back(e);
                    }
                    break;
                case ecSEAM:
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

void GeometryObject::projectShape(const TopoDS_Shape& inShape, const gp_Ax2& viewAxis)
{
//    Base::Console().Message("GO::projectShape()\n");
    clear();

    Handle(HLRBRep_Algo) brep_hlr;
    try {
        brep_hlr = new HLRBRep_Algo();
        //        brep_hlr->Debug(true);
        brep_hlr->Add(inShape, m_isoCount);
        if (m_isPersp) {
            double fLength = std::max(Precision::Confusion(), m_focus);
            HLRAlgo_Projector projector(viewAxis, fLength);
            brep_hlr->Projector(projector);
        }
        else {
            HLRAlgo_Projector projector(viewAxis);
            brep_hlr->Projector(projector);
        }
        brep_hlr->Update();
        brep_hlr->Hide();
    }
    catch (const Standard_Failure& e) {
        Base::Console().Error("GO::projectShape - OCC error - %s - while projecting shape\n",
                              e.GetMessageString());
        throw Base::RuntimeError("GeometryObject::projectShape - OCC error");
    }
    catch (...) {
        throw Base::RuntimeError("GeometryObject::projectShape - unknown error");
    }

    try {
        HLRBRep_HLRToShape hlrToShape(brep_hlr);

        if (!hlrToShape.VCompound().IsNull()) {
            visHard = hlrToShape.VCompound();
            BRepLib::BuildCurves3d(visHard);
            visHard =ShapeUtils::invertGeometry(visHard);
            //            BRepTools::Write(visHard, "GOvisHard.brep");            //debug
        }

        if (!hlrToShape.Rg1LineVCompound().IsNull()) {
            visSmooth = hlrToShape.Rg1LineVCompound();
            BRepLib::BuildCurves3d(visSmooth);
            visSmooth =ShapeUtils::invertGeometry(visSmooth);
        }

        if (!hlrToShape.RgNLineVCompound().IsNull()) {
            visSeam = hlrToShape.RgNLineVCompound();
            BRepLib::BuildCurves3d(visSeam);
            visSeam =ShapeUtils::invertGeometry(visSeam);
        }

        if (!hlrToShape.OutLineVCompound().IsNull()) {
            //            BRepTools::Write(hlrToShape.OutLineVCompound(), "GOOutLineVCompound.brep");            //debug
            visOutline = hlrToShape.OutLineVCompound();
            BRepLib::BuildCurves3d(visOutline);
            visOutline =ShapeUtils::invertGeometry(visOutline);
        }

        if (!hlrToShape.IsoLineVCompound().IsNull()) {
            visIso = hlrToShape.IsoLineVCompound();
            BRepLib::BuildCurves3d(visIso);
            visIso =ShapeUtils::invertGeometry(visIso);
        }

        if (!hlrToShape.HCompound().IsNull()) {
            hidHard = hlrToShape.HCompound();
            BRepLib::BuildCurves3d(hidHard);
            hidHard =ShapeUtils::invertGeometry(hidHard);
        }

        if (!hlrToShape.Rg1LineHCompound().IsNull()) {
            hidSmooth = hlrToShape.Rg1LineHCompound();
            BRepLib::BuildCurves3d(hidSmooth);
            hidSmooth =ShapeUtils::invertGeometry(hidSmooth);
        }

        if (!hlrToShape.RgNLineHCompound().IsNull()) {
            hidSeam = hlrToShape.RgNLineHCompound();
            BRepLib::BuildCurves3d(hidSeam);
            hidSeam =ShapeUtils::invertGeometry(hidSeam);
        }

        if (!hlrToShape.OutLineHCompound().IsNull()) {
            hidOutline = hlrToShape.OutLineHCompound();
            BRepLib::BuildCurves3d(hidOutline);
            hidOutline =ShapeUtils::invertGeometry(hidOutline);
        }

        if (!hlrToShape.IsoLineHCompound().IsNull()) {
            hidIso = hlrToShape.IsoLineHCompound();
            BRepLib::BuildCurves3d(hidIso);
            hidIso =ShapeUtils::invertGeometry(hidIso);
        }
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

//convert the hlr output into TD Geometry
void GeometryObject::makeTDGeometry()
{
//    Base::Console().Message("GO::makeTDGeometry()\n");
    extractGeometry(TechDraw::ecHARD,                   //always show the hard&outline visible lines
                        true);
    extractGeometry(TechDraw::ecOUTLINE,
                        true);

    const DrawViewPart* dvp = static_cast<const DrawViewPart*>(m_parent);
    if (!dvp) {
        return;//some routines do not have a dvp (ex shape outline)
    }

    if (dvp->SmoothVisible.getValue()) {
        extractGeometry(TechDraw::ecSMOOTH, true);
    }
    if (dvp->SeamVisible.getValue()) {
        extractGeometry(TechDraw::ecSEAM, true);
    }
    if ((dvp->IsoVisible.getValue()) && (dvp->IsoCount.getValue() > 0)) {
        extractGeometry(TechDraw::ecUVISO, true);
    }
    if (dvp->HardHidden.getValue()) {
        extractGeometry(TechDraw::ecHARD, false);
        extractGeometry(TechDraw::ecOUTLINE, false);
    }
    if (dvp->SmoothHidden.getValue()) {
        extractGeometry(TechDraw::ecSMOOTH, false);
    }
    if (dvp->SeamHidden.getValue()) {
        extractGeometry(TechDraw::ecSEAM, false);
    }
    if (dvp->IsoHidden.getValue() && (dvp->IsoCount.getValue() > 0)) {
        extractGeometry(TechDraw::ecUVISO, false);
    }
}

//mirror a shape thru XZ plane for Qt's inverted Y coordinate
TopoDS_Shape ShapeUtils::invertGeometry(const TopoDS_Shape s)
{
    if (s.IsNull()) {
        return s;
    }

    gp_Trsf mirrorY;
    gp_Pnt org(0.0, 0.0, 0.0);
    gp_Dir Y(0.0, 1.0, 0.0);
    gp_Ax2 mirrorPlane(org, Y);
    mirrorY.SetMirror(mirrorPlane);
    BRepBuilderAPI_Transform mkTrf(s, mirrorY, true);
    return mkTrf.Shape();
}

//!set up a hidden line remover and project a shape with it
void GeometryObject::projectShapeWithPolygonAlgo(const TopoDS_Shape& input, const gp_Ax2& viewAxis)
{
//    Base::Console().Message("GO::projectShapeWithPolygonAlgo()\n");
    // Clear previous Geometry
    clear();

    //work around for Mantis issue #3332
    //if 3332 gets fixed in OCC, this will produce shifted views and will need
    //to be reverted.
    TopoDS_Shape inCopy;
    if (!m_isPersp) {
        gp_Pnt gCenter = ShapeUtils::findCentroid(input, viewAxis);
        Base::Vector3d motion(-gCenter.X(), -gCenter.Y(), -gCenter.Z());
        inCopy = ShapeUtils::moveShape(input, motion);
    }
    else {
        BRepBuilderAPI_Copy BuilderCopy(input);
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
        Base::Console().Error(
            "GO::projectShapeWithPolygonAlgo - OCC error - %s - while projecting shape\n",
            e.GetMessageString());
        throw Base::RuntimeError("GeometryObject::projectShapeWithPolygonAlgo - OCC error");
    }
    catch (...) {
        throw Base::RuntimeError("GeometryObject::projectShapeWithPolygonAlgo - unknown error");
    }

    try {
        HLRBRep_PolyHLRToShape polyhlrToShape;
        polyhlrToShape.Update(brep_hlrPoly);

        visHard = polyhlrToShape.VCompound();
        BRepLib::BuildCurves3d(visHard);
        visHard =ShapeUtils::invertGeometry(visHard);
        //        BRepTools::Write(visHard, "GOvisHardi.brep");            //debug

        visSmooth = polyhlrToShape.Rg1LineVCompound();
        BRepLib::BuildCurves3d(visSmooth);
        visSmooth =ShapeUtils::invertGeometry(visSmooth);

        visSeam = polyhlrToShape.RgNLineVCompound();
        BRepLib::BuildCurves3d(visSeam);
        visSeam =ShapeUtils::invertGeometry(visSeam);

        visOutline = polyhlrToShape.OutLineVCompound();
        BRepLib::BuildCurves3d(visOutline);
        visOutline =ShapeUtils::invertGeometry(visOutline);

        hidHard = polyhlrToShape.HCompound();
        BRepLib::BuildCurves3d(hidHard);
        hidHard =ShapeUtils::invertGeometry(hidHard);
        //        BRepTools::Write(hidHard, "GOhidHardi.brep");            //debug

        hidSmooth = polyhlrToShape.Rg1LineHCompound();
        BRepLib::BuildCurves3d(hidSmooth);
        hidSmooth =ShapeUtils::invertGeometry(hidSmooth);

        hidSeam = polyhlrToShape.RgNLineHCompound();
        BRepLib::BuildCurves3d(hidSeam);
        hidSeam =ShapeUtils::invertGeometry(hidSeam);

        hidOutline = polyhlrToShape.OutLineHCompound();
        BRepLib::BuildCurves3d(hidOutline);
        hidOutline =ShapeUtils::invertGeometry(hidOutline);
    }
    catch (const Standard_Failure& e) {
        Base::Console().Error(
            "GO::projectShapeWithPolygonAlgo - OCC error - %s - while extracting edges\n",
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
TopoDS_Shape GeometryObject::projectSimpleShape(const TopoDS_Shape& shape, const gp_Ax2& CS)
{
    //    Base::Console().Message("GO::()\n");
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
    hardEdges =ShapeUtils::invertGeometry(hardEdges);

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
    //    Base::Console().Message("GO::projectFace()\n");
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
void GeometryObject::extractGeometry(edgeClass category, bool hlrVisible)
{
    //    Base::Console().Message("GO::extractGeometry(%d, %d)\n", category, hlrVisible);
    TopoDS_Shape filtEdges;
    if (hlrVisible) {
        switch (category) {
            case ecHARD:
                filtEdges = visHard;
                break;
            case ecOUTLINE:
                filtEdges = visOutline;
                break;
            case ecSMOOTH:
                filtEdges = visSmooth;
                break;
            case ecSEAM:
                filtEdges = visSeam;
                break;
            case ecUVISO:
                filtEdges = visIso;
                break;
            default:
                Base::Console().Warning(
                    "GeometryObject::ExtractGeometry - unsupported hlrVisible edgeClass: %d\n",
                    static_cast<int>(category));
                return;
        }
    }
    else {
        switch (category) {
            case ecHARD:
                filtEdges = hidHard;
                break;
            case ecOUTLINE:
                filtEdges = hidOutline;
                break;
            case ecSMOOTH:
                filtEdges = hidSmooth;
                break;
            case ecSEAM:
                filtEdges = hidSeam;
                break;
            case ecUVISO:
                filtEdges = hidIso;
                break;
            default:
                Base::Console().Warning(
                    "GeometryObject::ExtractGeometry - unsupported hidden edgeClass: %d\n",
                    static_cast<int>(category));
                return;
        }
    }

    addGeomFromCompound(filtEdges, category, hlrVisible);
}

//! update edgeGeom and vertexGeom from Compound of edges
void GeometryObject::addGeomFromCompound(TopoDS_Shape edgeCompound, edgeClass category,
                                         bool hlrVisible)
{
//    Base::Console().Message("GO::addGeomFromCompound(%d, %d)\n", category, hlrVisible);
    if (edgeCompound.IsNull()) {
        return;    // There is no OpenCascade Geometry to be calculated
    }

    // remove overlapping edges
    TopoDS_Shape cleanShape;
    if (m_scrubCount > 0) {
        std::vector<TopoDS_Edge> edgeVector = DU::shapeToVector(edgeCompound);
        for (int iPass = 0; iPass < m_scrubCount; iPass++)  {
            edgeVector = DrawProjectSplit::removeOverlapEdges(edgeVector);
        }
        bool invertResult = false;
        cleanShape = DU::vectorToCompound(edgeVector, invertResult);

    } else {
        cleanShape = edgeCompound;
    }

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
            //            throw Base::ValueError("GeometryObject::addGeomFromCompound - baseFactory failed");
        }

        base->source(0);//object geometry
        base->sourceIndex(i - 1);
        base->setClassOfEdge(category);
        base->setHlrVisible(hlrVisible);
        edgeGeom.push_back(base);

        //add vertices of new edge if not already in list
        if (hlrVisible) {
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
                c1->setHlrVisible(true);
            }

            std::vector<VertexPtr>::iterator itVertex = vertexGeom.begin();
            for (; itVertex != vertexGeom.end(); itVertex++) {
                if ((*itVertex)->isEqual(*v1, Precision::Confusion())) {
                    v1Add = false;
                }
                if ((*itVertex)->isEqual(*v2, Precision::Confusion())) {
                    v2Add = false;
                }
                if (circle) {
                    if ((*itVertex)->isEqual(*c1, Precision::Confusion())) {
                        c1Add = false;
                    }
                }
            }
            if (v1Add) {
                vertexGeom.push_back(v1);
                v1->setHlrVisible( true);
            }
            else {
                //    delete v1;
            }
            if (v2Add) {
                vertexGeom.push_back(v2);
                v2->setHlrVisible( true);
            }
            else {
                //    delete v2;
            }

            if (circle) {
                if (c1Add) {
                    vertexGeom.push_back(c1);
                    c1->setHlrVisible( true);
                }
                else {
                    //    delete c1;
                }
            }
        }
    }//end TopExp
}

void GeometryObject::addVertex(TechDraw::VertexPtr v) { vertexGeom.push_back(v); }

void GeometryObject::addEdge(TechDraw::BaseGeomPtr bg) { edgeGeom.push_back(bg); }

//********** Cosmetic Vertex ***************************************************

//adds a new GeomVert surrogate for CV
//returns GeomVert selection index  ("Vertex3")
// insertGeomForCV(cv)
int GeometryObject::addCosmeticVertex(CosmeticVertex* cv)
{
    //    Base::Console().Message("GO::addCosmeticVertex(%X)\n", cv);
    double scale = m_parent->getScale();
    Base::Vector3d pos = cv->scaled(scale);
    TechDraw::VertexPtr v(std::make_shared<TechDraw::Vertex>(pos.x, pos.y));
    v->setCosmetic(true);
//    v->setCosmeticLink = -1;//obs??
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
    Base::Console().Message("GO::addCosmeticVertex() 1 - deprec?\n");
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
    //    Base::Console().Message("GO::addCosmeticVertex() 2\n");
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
    //    Base::Console().Message("GO::addCosmeticEdge(%X) 0\n", ce);
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
    //    Base::Console().Message("GO::addCosmeticEdge() 1 - deprec?\n");
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
    //    Base::Console().Message("GO::addCosmeticEdge() 2\n");
    gp_Pnt gp1(start.x, start.y, start.z);
    gp_Pnt gp2(end.x, end.y, end.z);
    TopoDS_Edge occEdge = BRepBuilderAPI_MakeEdge(gp1, gp2);
    TechDraw::BaseGeomPtr base = BaseGeom::baseFactory(occEdge);
    base->setCosmetic(true);
    base->setCosmeticTag(tagString);
    base->source(1);//1-CosmeticEdge, 2-CenterLine
    base->setHlrVisible(true);
    int idx = edgeGeom.size();
    edgeGeom.push_back(base);
    return idx;
}

int GeometryObject::addCosmeticEdge(TechDraw::BaseGeomPtr base, std::string tagString)
{
    //    Base::Console().Message("GO::addCosmeticEdge(%X, %s) 3\n", base, tagString.c_str());
    base->setCosmetic(true);
    base->setHlrVisible(true);
    base->source(1);//1-CosmeticEdge, 2-CenterLine
    base->setCosmeticTag(tagString);
    base->sourceIndex(-1);
    int idx = edgeGeom.size();
    edgeGeom.push_back(base);
    return idx;
}

int GeometryObject::addCenterLine(TechDraw::BaseGeomPtr base, std::string tag)
//                                    int s, int si)
{
    //    Base::Console().Message("GO::addCenterLine()\n");
    base->setCosmetic(true);
    base->setCosmeticTag(tag);
    base->source(2);
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
    if (fabs(last - first) >= 2 * M_PI) {
        return true;
    }

    // Put params within [0, 2*pi) - not totally sure this is necessary
    theta = fmod(theta, 2 * M_PI);
    if (theta < 0) {
        theta += 2 * M_PI;
    }

    first = fmod(first, 2 * M_PI);
    if (first < 0) {
        first += 2 * M_PI;
    }

    last = fmod(last, 2 * M_PI);
    if (last < 0) {
        last += 2 * M_PI;
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
    //    Base::Console().Message("GO::calcBoundingBox() - edges: %d\n", edgeGeom.size());
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

