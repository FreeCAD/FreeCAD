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

#include "PreCompiled.h"
#ifndef _PreComp_

#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepLib.hxx>
#include <BRepLProp_CurveTool.hxx>
#include <BRepLProp_CLProps.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <Geom_BSplineCurve.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Elips.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>
#include <HLRBRep.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <HLRAlgo_Projector.hxx>


#include <BRepMesh_IncrementalMesh.hxx>
#include <HLRBRep_PolyAlgo.hxx>
#include <HLRBRep_PolyHLRToShape.hxx>
#include <Poly_Triangulation.hxx>
#include <TopLoc_Location.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>

#endif  // #ifndef _PreComp_

#include <algorithm>
#include <chrono>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Tools.h>

#include <Mod/Part/App/PartFeature.h>

#include "DrawUtil.h"
#include "GeometryObject.h"
#include "DrawViewPart.h"
#include "DrawViewDetail.h"

using namespace TechDraw;
using namespace std;

struct EdgePoints {
    gp_Pnt v1, v2;
    TopoDS_Edge edge;
};

GeometryObject::GeometryObject(const string& parent, TechDraw::DrawView* parentObj) :
    m_parentName(parent),
    m_parent(parentObj),
    m_isoCount(0),
    m_isPersp(false),
    m_focus(100.0),
    m_usePolygonHLR(false)

{
}

GeometryObject::~GeometryObject()
{
    clear();
}

const std::vector<BaseGeom *> GeometryObject::getVisibleFaceEdges(const bool smooth, const bool seam) const
{
    std::vector<BaseGeom *> result;
    bool smoothOK = smooth;
    bool seamOK = seam;

    for (auto& e:edgeGeom) {
        if (e->hlrVisible) {
            switch (e->classOfEdge) {
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
                default:
                ;
            }
        }
    }
    //debug
    //make compound of edges and save as brep file
//    BRep_Builder builder;
//    TopoDS_Compound comp;
//    builder.MakeCompound(comp);
//    for (auto& r: result) {
//        builder.Add(comp, r->occEdge);
//    }
//    BRepTools::Write(comp, "GOVizFaceEdges.brep");            //debug

    return result;
}



void GeometryObject::clear()
{
    for(std::vector<BaseGeom *>::iterator it = edgeGeom.begin(); it != edgeGeom.end(); ++it) {
        delete *it;
        *it = 0;
    }

    vertexGeom.clear();
    faceGeom.clear();
    edgeGeom.clear();
}

//!set up a hidden line remover and project a shape with it
void GeometryObject::projectShape(const TopoDS_Shape& input,
                                  const gp_Ax2& viewAxis)
{
//    Base::Console().Message("GO::projectShape() - %s\n", m_parentName.c_str());
   // Clear previous Geometry
    clear();
//    DrawUtil::dumpCS("GO::projectShape - VA in", viewAxis);    //debug

    auto start = chrono::high_resolution_clock::now();

    Handle(HLRBRep_Algo) brep_hlr = NULL;
    try {
        brep_hlr = new HLRBRep_Algo();
        brep_hlr->Add(input, m_isoCount);
        if (m_isPersp) {
            double fLength = std::max(Precision::Confusion(),m_focus);
            HLRAlgo_Projector projector( viewAxis, fLength );
            brep_hlr->Projector(projector);
        } else {
            HLRAlgo_Projector projector( viewAxis );
            brep_hlr->Projector(projector);
        }
        brep_hlr->Update();
        brep_hlr->Hide();

    }
    catch (const Standard_Failure& e) {
        Base::Console().Error("GO::projectShape - OCC error - %s - while projecting shape\n",
                              e.GetMessageString());
        }
    catch (...) {
        Base::Console().Error("GeometryObject::projectShape - unknown error occurred while projecting shape\n");
//        throw Base::RuntimeError("GeometryObject::projectShape - unknown error occurred while projecting shape");
    }

    auto end   = chrono::high_resolution_clock::now();
    auto diff  = end - start;
    double diffOut = chrono::duration <double, milli> (diff).count();
    Base::Console().Log("TIMING - %s GO spent: %.3f millisecs in HLRBRep_Algo & co\n",m_parentName.c_str(),diffOut);

    start = chrono::high_resolution_clock::now();

    try {
        HLRBRep_HLRToShape hlrToShape(brep_hlr);

        visHard    = hlrToShape.VCompound();
        BRepLib::BuildCurves3d(visHard);
        visHard = invertGeometry(visHard);
//        BRepTools::Write(visHard, "GOvisHardi.brep");            //debug

        visSmooth  = hlrToShape.Rg1LineVCompound();
        BRepLib::BuildCurves3d(visSmooth);
        visSmooth = invertGeometry(visSmooth);

        visSeam    = hlrToShape.RgNLineVCompound();
        BRepLib::BuildCurves3d(visSeam);
        visSeam = invertGeometry(visSeam);

        visOutline    = hlrToShape.OutLineVCompound();
        BRepLib::BuildCurves3d(visOutline);
        visOutline = invertGeometry(visOutline);

        visIso     = hlrToShape.IsoLineVCompound();
        BRepLib::BuildCurves3d(visIso);
        visIso = invertGeometry(visIso);

        hidHard    = hlrToShape.HCompound();
        BRepLib::BuildCurves3d(hidHard);
        hidHard = invertGeometry(hidHard);
//        BRepTools::Write(hidHard, "GOhidHardi.brep");            //debug

        hidSmooth  = hlrToShape.Rg1LineHCompound();
        BRepLib::BuildCurves3d(hidSmooth);
        hidSmooth = invertGeometry(hidSmooth);

        hidSeam    = hlrToShape.RgNLineHCompound();
        BRepLib::BuildCurves3d(hidSeam);
        hidSeam = invertGeometry(hidSeam);

        hidOutline = hlrToShape.OutLineHCompound();
        BRepLib::BuildCurves3d(hidOutline);
        hidOutline = invertGeometry(hidOutline);

        hidIso     = hlrToShape.IsoLineHCompound();
        BRepLib::BuildCurves3d(hidIso);
        hidIso = invertGeometry(hidIso);

    }
    catch (const Standard_Failure& e) {
        Base::Console().Error("GO::projectShape - OCC error - %s - while extracting edges\n",
                              e.GetMessageString());
    }
    catch (...) {
        Base::Console().Error("GO::projectShape - unknown error while extracting edges\n");
//        throw Base::RuntimeError("GeometryObject::projectShape - error occurred while extracting edges");
    }
    end   = chrono::high_resolution_clock::now();
    diff  = end - start;
    diffOut = chrono::duration <double, milli> (diff).count();
    Base::Console().Log("TIMING - %s GO spent: %.3f millisecs in hlrToShape and BuildCurves\n",m_parentName.c_str(),diffOut);
}

//mirror a shape thru XZ plane for Qt's inverted Y coordinate
TopoDS_Shape GeometryObject::invertGeometry(const TopoDS_Shape s)
{
    TopoDS_Shape result;
    if (s.IsNull()) {
        result = s;
        return result;
    }

    gp_Trsf mirrorY;
    gp_Pnt org(0.0, 0.0, 0.0);
    gp_Dir Y(0.0, 1.0, 0.0);
    gp_Ax2 mirrorPlane(org,
                       Y);
    mirrorY.SetMirror(mirrorPlane);
    BRepBuilderAPI_Transform mkTrf(s, mirrorY, true);
    result = mkTrf.Shape();
    return result;
}

//!set up a hidden line remover and project a shape with it
void GeometryObject::projectShapeWithPolygonAlgo(const TopoDS_Shape& input,
                                                 const gp_Ax2 &viewAxis)
{
    // Clear previous Geometry
    clear();
    
    //work around for Mantis issue #3332
    //if 3332 gets fixed in OCC, this will produce shifted views and will need
    //to be reverted.
    TopoDS_Shape inCopy;
    if (!m_isPersp) {
        gp_Pnt gCenter = findCentroid(input,
                                      viewAxis);
        Base::Vector3d motion(-gCenter.X(),-gCenter.Y(),-gCenter.Z());
        inCopy = moveShape(input,motion);
    } else {
        BRepBuilderAPI_Copy BuilderCopy(input);
        inCopy = BuilderCopy.Shape();
    }

    auto start = chrono::high_resolution_clock::now();

    Handle(HLRBRep_PolyAlgo) brep_hlrPoly = NULL;

    try {
        TopExp_Explorer faces(inCopy, TopAbs_FACE);
        for (int i = 1; faces.More(); faces.Next(), i++) {
            const TopoDS_Face& f = TopoDS::Face(faces.Current());
            if (!f.IsNull()) {
                BRepMesh_IncrementalMesh(f, 0.10); //Poly Algo requires a mesh!
            }
        }
        brep_hlrPoly = new HLRBRep_PolyAlgo();
        brep_hlrPoly->Load(inCopy);

        if (m_isPersp) {
            double fLength = std::max(Precision::Confusion(), m_focus);
            HLRAlgo_Projector projector(viewAxis, fLength);
            brep_hlrPoly->Projector(projector);
        }
        else { // non perspective
            HLRAlgo_Projector projector(viewAxis);
            brep_hlrPoly->Projector(projector);
        }
        brep_hlrPoly->Update();
    }
    catch (const Standard_Failure& e) {
        Base::Console().Error("GO::projectShapeWithPolygonAlgo - OCC error - %s - while projecting shape\n",
                              e.GetMessageString());
    }
    catch (...) {
        Base::Console().Error("GO::projectShapeWithPolygonAlgo - unknown error while projecting shape\n");
//        throw Base::RuntimeError("GeometryObject::projectShapeWithPolygonAlgo  - error occurred while projecting shape");
//        Standard_Failure::Raise("GeometryObject::projectShapeWithPolygonAlgo  - error occurred while projecting shape");
    }

    try {
        HLRBRep_PolyHLRToShape polyhlrToShape;
        polyhlrToShape.Update(brep_hlrPoly);

        visHard    = polyhlrToShape.VCompound();
        BRepLib::BuildCurves3d(visHard);
        visHard = invertGeometry(visHard);
//        BRepTools::Write(visHard, "GOvisHardi.brep");            //debug

        visSmooth  = polyhlrToShape.Rg1LineVCompound();
        BRepLib::BuildCurves3d(visSmooth);
        visSmooth = invertGeometry(visSmooth);

        visSeam    = polyhlrToShape.RgNLineVCompound();
        BRepLib::BuildCurves3d(visSeam);
        visSeam = invertGeometry(visSeam);

        visOutline    = polyhlrToShape.OutLineVCompound();
        BRepLib::BuildCurves3d(visOutline);
        visOutline = invertGeometry(visOutline);

        hidHard    = polyhlrToShape.HCompound();
        BRepLib::BuildCurves3d(hidHard);
        hidHard = invertGeometry(hidHard);
//        BRepTools::Write(hidHard, "GOhidHardi.brep");            //debug

        hidSmooth  = polyhlrToShape.Rg1LineHCompound();
        BRepLib::BuildCurves3d(hidSmooth);
        hidSmooth = invertGeometry(hidSmooth);

        hidSeam    = polyhlrToShape.RgNLineHCompound();
        BRepLib::BuildCurves3d(hidSeam);
        hidSeam = invertGeometry(hidSeam);

        hidOutline = polyhlrToShape.OutLineHCompound();
        BRepLib::BuildCurves3d(hidOutline);
        hidOutline = invertGeometry(hidOutline);
    }
    catch (const Standard_Failure& e) {
        Base::Console().Error("GO::projectShapeWithPolygonAlgo - OCC error - %s - while extracting edges\n",
                              e.GetMessageString());
    }
    catch (...) {
        Base::Console().Error("GO::projectShapeWithPolygonAlgo - - error occurred while extracting edges\n");
//        throw Base::RuntimeError("GeometryObject::projectShapeWithPolygonAlgo  - error occurred while extracting edges");
//        Standard_Failure::Raise("GeometryObject::projectShapeWithPolygonAlgo - error occurred while extracting edges");
    }
    auto end = chrono::high_resolution_clock::now();
    auto diff = end - start;
    double diffOut = chrono::duration <double, milli>(diff).count();
    Base::Console().Log("TIMING - %s GO spent: %.3f millisecs in HLRBRep_PolyAlgo & co\n", m_parentName.c_str(), diffOut);
}

TopoDS_Shape GeometryObject::projectFace(const TopoDS_Shape &face,
                                         const gp_Ax2 &CS)
{
//    Base::Console().Message("GO::projectFace()\n");
    if(face.IsNull()) {
        throw Base::ValueError("GO::projectFace - input Face is NULL");
    }

    HLRBRep_Algo *brep_hlr = new HLRBRep_Algo();
    brep_hlr->Add(face);
    HLRAlgo_Projector projector( CS );
    brep_hlr->Projector(projector);
    brep_hlr->Update();
    brep_hlr->Hide();

    HLRBRep_HLRToShape hlrToShape(brep_hlr);
    TopoDS_Shape hardEdges = hlrToShape.VCompound();
    BRepLib::BuildCurves3d(hardEdges);
    hardEdges = invertGeometry(hardEdges);

    return hardEdges;
}

//!add edges meeting filter criteria for category, visibility
void GeometryObject::extractGeometry(edgeClass category, bool hlrVisible)
{
//    Base::Console().Message("GO::extractGeometry(%d, %d)\n",category, hlrVisible);
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
                Base::Console().Warning("GeometryObject::ExtractGeometry - unsupported hlrVisible edgeClass: %d\n",category);
                return;
        }
    } else {
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
                Base::Console().Warning("GeometryObject::ExtractGeometry - unsupported hidden edgeClass: %d\n",category);
                return;
        }
    }

    addGeomFromCompound(filtEdges, category, hlrVisible);
}

//! update edgeGeom and vertexGeom from Compound of edges
void GeometryObject::addGeomFromCompound(TopoDS_Shape edgeCompound, edgeClass category, bool hlrVisible)
{
//    Base::Console().Message("GO::addGeomFromCompound(%d, %d)\n", category, hlrVisible);
    if(edgeCompound.IsNull()) {
        Base::Console().Log("TechDraw::GeometryObject::addGeomFromCompound edgeCompound is NULL\n");
        return; // There is no OpenCascade Geometry to be calculated
    }

    BaseGeom* base;
    TopExp_Explorer edges(edgeCompound, TopAbs_EDGE);
    int i = 1;
    for ( ; edges.More(); edges.Next(),i++) {
        const TopoDS_Edge& edge = TopoDS::Edge(edges.Current());
        if (edge.IsNull()) {
            Base::Console().Log("GO::addGeomFromCompound - edge: %d is NULL\n",i);
            continue;
        }
        if (DrawUtil::isZeroEdge(edge)) {
            Base::Console().Log("GO::addGeomFromCompound - edge: %d is zeroEdge\n",i);
            continue;
        }
        if (DrawUtil::isCrazy(edge))  {
            Base::Console().Log("GO::addGeomFromCompound - edge: %d is crazy\n",i);
            continue;
        }

        base = BaseGeom::baseFactory(edge);
        if (base == nullptr) {
            Base::Console().Log("Error - GO::addGeomFromCompound - baseFactory failed for edge: %d\n",i);
            continue;
//            throw Base::ValueError("GeometryObject::addGeomFromCompound - baseFactory failed");
        }

        base->source(0);             //object geometry
        base->sourceIndex(i-1);
        base->classOfEdge = category;
        base->hlrVisible = hlrVisible;
        edgeGeom.push_back(base);

        //add vertices of new edge if not already in list
        if (hlrVisible) {
            BaseGeom* lastAdded = edgeGeom.back();
            bool v1Add = true, v2Add = true;
            bool c1Add = true;
            TechDraw::VertexPtr v1 = std::make_shared<TechDraw::Vertex>(lastAdded->getStartPoint());
            TechDraw::VertexPtr v2 = std::make_shared<TechDraw::Vertex>(lastAdded->getEndPoint());
            TechDraw::Circle* circle = dynamic_cast<TechDraw::Circle*>(lastAdded);
            TechDraw::VertexPtr c1;
            if (circle) {
                c1 = std::make_shared<TechDraw::Vertex>(circle->center);
                c1->isCenter = true;
                c1->hlrVisible = true;
            }

            std::vector<VertexPtr>::iterator itVertex = vertexGeom.begin();
            for (; itVertex != vertexGeom.end(); itVertex++) {
                if ((*itVertex)->isEqual(*v1,Precision::Confusion())) {
                    v1Add = false;
                }
                if ((*itVertex)->isEqual(*v2,Precision::Confusion())) {
                    v2Add = false;
                }
                if (circle ) {
                    if ((*itVertex)->isEqual(*c1,Precision::Confusion())) {
                        c1Add = false;
                    }
                }

            }
            if (v1Add) {
                vertexGeom.push_back(v1);
                v1->hlrVisible = true;
            } else {
            //    delete v1;
            }
            if (v2Add) {
                vertexGeom.push_back(v2);
                v2->hlrVisible = true;
            } else {
            //    delete v2;
            }

            if (circle) {
                if (c1Add) {
                    vertexGeom.push_back(c1);
                    c1->hlrVisible = true;
                } else {
                //    delete c1;
                }
            }
        }
    }  //end TopExp
}

void GeometryObject::addVertex(TechDraw::VertexPtr v)
{
    vertexGeom.push_back(v);
}

void GeometryObject::addEdge(TechDraw::BaseGeom* bg)
{
    edgeGeom.push_back(bg);
}

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
    v->cosmetic = true;
    v->cosmeticLink = -1;  //obs??
    v->cosmeticTag = cv->getTagAsString();
    v->hlrVisible = true;
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
    v->cosmetic = true;
    v->cosmeticTag = "tbi";        //not connected to CV
    v->hlrVisible = true;
    int idx = vertexGeom.size();
    vertexGeom.push_back(v);
    return idx;
}

int GeometryObject::addCosmeticVertex(Base::Vector3d pos, std::string tagString)
{
//    Base::Console().Message("GO::addCosmeticVertex() 2\n");
    TechDraw::VertexPtr v(std::make_shared<TechDraw::Vertex>(pos.x, pos.y));
    v->cosmetic = true;
    v->cosmeticTag = tagString;     //connected to CV
    v->hlrVisible = true;
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
    TechDraw::BaseGeom* e = ce->scaledGeometry(scale);
    e->cosmetic = true;
    e->setCosmeticTag(ce->getTagAsString());
    e->hlrVisible = true;
    int idx = edgeGeom.size();
    edgeGeom.push_back(e);
    return idx;
}

//adds a new GeomEdge to list for ce[link]
//this should be made obsolete and the variant with tag used instead
int GeometryObject::addCosmeticEdge(Base::Vector3d start,
                                    Base::Vector3d end)
{
//    Base::Console().Message("GO::addCosmeticEdge() 1 - deprec?\n");
    gp_Pnt gp1(start.x, start.y, start.z);
    gp_Pnt gp2(end.x, end.y, end.z);
    TopoDS_Edge occEdge = BRepBuilderAPI_MakeEdge(gp1, gp2);
    TechDraw::BaseGeom* e = BaseGeom::baseFactory(occEdge);
    e->cosmetic = true;
//    e->cosmeticLink = link;
    e->setCosmeticTag("tbi");
    e->hlrVisible = true;
    int idx = edgeGeom.size();
    edgeGeom.push_back(e);
    return idx;
}

int GeometryObject::addCosmeticEdge(Base::Vector3d start,
                                    Base::Vector3d end,
                                    std::string tagString)
{
//    Base::Console().Message("GO::addCosmeticEdge() 2\n");
    gp_Pnt gp1(start.x, start.y, start.z);
    gp_Pnt gp2(end.x, end.y, end.z);
    TopoDS_Edge occEdge = BRepBuilderAPI_MakeEdge(gp1, gp2);
    TechDraw::BaseGeom* base = BaseGeom::baseFactory(occEdge);
    base->cosmetic = true;
    base->setCosmeticTag(tagString);
    base->source(1);           //1-CosmeticEdge, 2-CenterLine
    base->hlrVisible = true;
    int idx = edgeGeom.size();
    edgeGeom.push_back(base);
    return idx;
}

int GeometryObject::addCosmeticEdge(TechDraw::BaseGeom* base,
                                    std::string tagString)
{
//    Base::Console().Message("GO::addCosmeticEdge(%X, %s) 3\n", base, tagString.c_str());
    base->cosmetic = true;
    base->hlrVisible = true;
    base->source(1);           //1-CosmeticEdge, 2-CenterLine
    base->setCosmeticTag(tagString);
    base->sourceIndex(-1);
    int idx = edgeGeom.size();
    edgeGeom.push_back(base);
    return idx;
}

int GeometryObject::addCenterLine(TechDraw::BaseGeom* base,
                                  std::string tag)
//                                    int s, int si)
{
//    Base::Console().Message("GO::addCenterLine()\n");
    base->cosmetic = true;
    base->setCosmeticTag(tag);
    base->source(2);
//    base->sourceIndex(si);     //index into source;
    int idx = edgeGeom.size();
    edgeGeom.push_back(base);
    return idx;
}


//! empty Face geometry
void GeometryObject::clearFaceGeom()
{
    faceGeom.clear();
}

//! add a Face to Face Geometry
void GeometryObject::addFaceGeom(FacePtr f)
{
    faceGeom.push_back(f);
}

TechDraw::DrawViewDetail* GeometryObject::isParentDetail()
{
    TechDraw::DrawViewDetail* result = nullptr;
    if (m_parent != nullptr) {
        TechDraw::DrawViewDetail* detail = dynamic_cast<TechDraw::DrawViewDetail*>(m_parent);
        if (detail != nullptr) {
            result = detail;
        }
    }
    return result;
}


bool GeometryObject::isWithinArc(double theta, double first,
                                 double last, bool cw) const
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
        } else {
            return theta <= first || theta >= last;
        }
    } else {
        if (first > last) {
            return theta >= first || theta <= last;
        } else {
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
        for (std::vector<BaseGeom *>::const_iterator it( edgeGeom.begin() );
                it != edgeGeom.end(); ++it) {
             BRepBndLib::Add((*it)->occEdge, testBox);
        }
    }
    

    double xMin = 0,xMax = 0,yMin = 0,yMax = 0, zMin = 0, zMax = 0;
    if (testBox.IsVoid()) {
        Base::Console().Log("INFO - GO::calcBoundingBox - testBox is void\n");
    } else {
        testBox.Get(xMin,yMin,zMin,xMax,yMax,zMax);
    }
    Base::BoundBox3d bbox(xMin,yMin,zMin,xMax,yMax,zMax);
    return bbox;
}

void GeometryObject::pruneVertexGeom(Base::Vector3d center, double radius)
{
    const std::vector<VertexPtr>&  oldVerts = getVertexGeometry();
    std::vector<VertexPtr> newVerts;
    for (auto& v: oldVerts) {
        Base::Vector3d v3 = v->point();
        double length = (v3 - center).Length();
        if (length < Precision::Confusion()) { 
            continue;
        } else if (length < radius) {
            newVerts.push_back(v);
        }
    }
    vertexGeom = newVerts;
}

//! does this GeometryObject already have this vertex
bool GeometryObject::findVertex(Base::Vector3d v)
{
    bool found = false;
    std::vector<VertexPtr>::iterator it = vertexGeom.begin();
    for (; it != vertexGeom.end(); it++) {
        double dist = (v - (*it)->pnt).Length();
        if (dist < Precision::Confusion()) {
            found = true;
            break;
        }
    }
    return found;
}

/// utility non-class member functions
//! gets a coordinate system that matches view system used in 3D with +Z up (or +Y up if necessary)
//! used for individual views, but not secondary views in projection groups
//! flip determines Y mirror or not.
// getViewAxis 1
gp_Ax2 TechDraw::getViewAxis(const Base::Vector3d origin,
                                     const Base::Vector3d& direction,
                                     const bool flip)
{
//    Base::Console().Message("GO::getViewAxis() - 1 - use only with getLegacyX\n");
    (void) flip;
    gp_Ax2 viewAxis;
    gp_Pnt inputCenter(origin.x,origin.y,origin.z);
    Base::Vector3d stdZ(0.0,0.0,1.0);
    Base::Vector3d stdOrg(0.0,0.0,0.0);
    Base::Vector3d cross = direction;
    if (TechDraw::DrawUtil::checkParallel(direction, stdZ)) {
        cross = Base::Vector3d(1.0,0.0,0.0);
    } else {
        cross.Normalize();
        cross = cross.Cross(stdZ);
    }
    
    if (cross.IsEqual(stdOrg,FLT_EPSILON)) {
        viewAxis = gp_Ax2(inputCenter,
                          gp_Dir(direction.x, direction.y, direction.z));
        return viewAxis;
    }
    
    viewAxis = gp_Ax2(inputCenter,
                      gp_Dir(direction.x, direction.y, direction.z),
                      gp_Dir(cross.x, cross.y, cross.z));
    return viewAxis;
}

//! gets a coordinate system specified by Z and X directions
//getViewAxis 2
gp_Ax2 TechDraw::getViewAxis(const Base::Vector3d origin,
                                     const Base::Vector3d& direction,
                                     const Base::Vector3d& xAxis,
                                     const bool flip)
{
//    Base::Console().Message("GO::getViewAxis() - 2\n");
    (void) flip;
    gp_Pnt inputCenter(origin.x,origin.y,origin.z);
    gp_Ax2 viewAxis;
    viewAxis = gp_Ax2(inputCenter,
                      gp_Dir(direction.x, direction.y, direction.z),
                      gp_Dir(xAxis.x, xAxis.y, xAxis.z));
    return viewAxis;
}

// was getViewAxis 1
// getViewAxis as used before XDirection property adopted
gp_Ax2 TechDraw::legacyViewAxis1(const Base::Vector3d origin,
                                     const Base::Vector3d& direction,
                                     const bool flip)
{
//    Base::Console().Message("GO::legacyViewAxis1()\n");
    gp_Ax2 viewAxis;
    gp_Pnt inputCenter(origin.x,origin.y,origin.z);
    Base::Vector3d stdZ(0.0,0.0,1.0);
    Base::Vector3d stdOrg(0.0,0.0,0.0);
    Base::Vector3d flipDirection(direction.x,-direction.y,direction.z);
    if (!flip) {
        flipDirection = Base::Vector3d(direction.x,direction.y,direction.z);
    }
    Base::Vector3d cross = flipDirection;
//    //special case
    if (TechDraw::DrawUtil::checkParallel(flipDirection, stdZ)) {
        cross = Base::Vector3d(1.0,0.0,0.0);
    } else {
        cross.Normalize();
        cross = cross.Cross(stdZ);
    }
    
    if (cross.IsEqual(stdOrg,FLT_EPSILON)) {
        viewAxis = gp_Ax2(inputCenter,
                          gp_Dir(flipDirection.x, flipDirection.y, flipDirection.z));
        return viewAxis;
    }
    
    viewAxis = gp_Ax2(inputCenter,
                      gp_Dir(flipDirection.x, flipDirection.y, flipDirection.z),
                      gp_Dir(cross.x, cross.y, cross.z));

    //this bit is to handle the old mirror Y logic, but it messes up
    //some old files.
    gp_Trsf mirrorXForm;
    gp_Ax2 mirrorCS(inputCenter, gp_Dir(0, -1, 0));
    mirrorXForm.SetMirror( mirrorCS );
    viewAxis = viewAxis.Transformed(mirrorXForm);

    return viewAxis;
}

//! Returns the centroid of shape, as viewed according to direction
gp_Pnt TechDraw::findCentroid(const TopoDS_Shape &shape,
                                    const Base::Vector3d &direction)
{
//    Base::Console().Message("GO::findCentroid() - 1\n");
    Base::Vector3d origin(0.0,0.0,0.0);
    gp_Ax2 viewAxis = getViewAxis(origin,direction);
    return findCentroid(shape,viewAxis);
}

//! Returns the centroid of shape, as viewed according to direction
gp_Pnt TechDraw::findCentroid(const TopoDS_Shape &shape,
                                      const gp_Ax2 &viewAxis)
{
//    Base::Console().Message("GO::findCentroid() - 2\n");
//    Base::Vector3d origin(0.0,0.0,0.0);
//    gp_Ax2 viewAxis = getViewAxis(origin,direction);

    gp_Trsf tempTransform;
    tempTransform.SetTransformation(viewAxis);
    BRepBuilderAPI_Transform builder(shape, tempTransform);

    Bnd_Box tBounds;
    BRepBndLib::Add(builder.Shape(), tBounds);

    tBounds.SetGap(0.0);
    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    tBounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);

    Standard_Real x = (xMin + xMax) / 2.0,
                  y = (yMin + yMax) / 2.0,
                  z = (zMin + zMax) / 2.0;

    // Get centroid back into object space
    tempTransform.Inverted().Transforms(x, y, z);

    return gp_Pnt(x, y, z);
}

Base::Vector3d TechDraw::findCentroidVec(const TopoDS_Shape &shape,
                                              const Base::Vector3d &direction)
{
//    Base::Console().Message("GO::findCentroidVec() - 1\n");
    gp_Pnt p = TechDraw::findCentroid(shape,direction);
    Base::Vector3d result(p.X(),p.Y(),p.Z());
    return result;
}

Base::Vector3d TechDraw::findCentroidVec(const TopoDS_Shape &shape,
                                         const gp_Ax2 &cs)
{
//    Base::Console().Message("GO::findCentroidVec() - 2\n");
    gp_Pnt p = TechDraw::findCentroid(shape,cs);
    Base::Vector3d result(p.X(),p.Y(),p.Z());
    return result;
}


//!scales & mirrors a shape about a center
TopoDS_Shape TechDraw::mirrorShapeVec(const TopoDS_Shape &input,
                             const Base::Vector3d& inputCenter,
                             double scale)
{
    gp_Pnt gInput(inputCenter.x, inputCenter.y, inputCenter.z);
    return TechDraw::mirrorShape(input, gInput, scale);
}

TopoDS_Shape TechDraw::mirrorShape(const TopoDS_Shape &input,
                             const gp_Pnt& inputCenter,
                             double scale)
{
    TopoDS_Shape transShape;
    if (input.IsNull()) {
        return transShape;
    }
    try {
        // Make tempTransform scale the object around it's centre point and
        // mirror about the Y axis
        gp_Trsf tempTransform;
        //BRepBuilderAPI_Transform will loop forever if asked to use 0.0 as scale
        if (!(scale > 0.0)) {
            tempTransform.SetScale(inputCenter, 1.0);
        } else {
            tempTransform.SetScale(inputCenter, scale);
        }
        gp_Trsf mirrorTransform;
        mirrorTransform.SetMirror( gp_Ax2(inputCenter, gp_Dir(0, -1, 0)) );
        tempTransform.Multiply(mirrorTransform);

        // Apply that transform to the shape.  This should preserve the centre.
        BRepBuilderAPI_Transform mkTrf(input, tempTransform);
        transShape = mkTrf.Shape();
    }
    catch (...) {
        Base::Console().Log("GeometryObject::mirrorShape - mirror/scale failed.\n");
        return transShape;
    }
    return transShape;
}

//!rotates a shape about a viewAxis
TopoDS_Shape TechDraw::rotateShape(const TopoDS_Shape &input,
                             gp_Ax2& viewAxis,
                             double rotAngle)
{
    TopoDS_Shape transShape;
    if (input.IsNull()) {
        return transShape;
    }

    gp_Ax1 rotAxis = viewAxis.Axis();
    double rotation = rotAngle * M_PI/180.0;

    try {
        gp_Trsf tempTransform;
        tempTransform.SetRotation(rotAxis,rotation);
        BRepBuilderAPI_Transform mkTrf(input, tempTransform);
        transShape = mkTrf.Shape();
    }
    catch (...) {
        Base::Console().Log("GeometryObject::rotateShape - rotate failed.\n");
        return transShape;
    }
    return transShape;
}

//!scales a shape about origin
TopoDS_Shape TechDraw::scaleShape(const TopoDS_Shape &input,
                                          double scale)
{
    TopoDS_Shape transShape;
    try {
        gp_Trsf scaleTransform;
        scaleTransform.SetScale(gp_Pnt(0,0,0), scale);

        BRepBuilderAPI_Transform mkTrf(input, scaleTransform);
        transShape = mkTrf.Shape();
    }
    catch (...) {
        Base::Console().Log("GeometryObject::scaleShape - scale failed.\n");
        return transShape;
    }
    return transShape;
}

//!moves a shape
TopoDS_Shape TechDraw::moveShape(const TopoDS_Shape &input,
                                 const Base::Vector3d& motion)
{
    TopoDS_Shape transShape;
    try {
        gp_Trsf xlate;
        xlate.SetTranslation(gp_Vec(motion.x,motion.y,motion.z));

        BRepBuilderAPI_Transform mkTrf(input, xlate);
        transShape = mkTrf.Shape();
    }
    catch (...) {
        Base::Console().Log("GeometryObject::moveShape - move failed.\n");
        return transShape;
    }
    return transShape;
}
