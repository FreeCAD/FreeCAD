/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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
# include <sstream>

#include <Bnd_Box.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRep_Builder.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRepLib.hxx>
#include <BRepLProp_CLProps.hxx>
#include <BRepLProp_CurveTool.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Geom_Curve.hxx>
#include <GeomLib_Tool.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <GProp_GProps.hxx>
#include <gp_XYZ.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <HLRBRep_ShapeBounds.hxx>
#include <ShapeExtend_WireData.hxx>
#include <ShapeFix_ShapeTolerance.hxx>
#include <ShapeFix_Wire.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepAlgo_NormalProjection.hxx>

#include <TopTools_IndexedMapOfShape.hxx>

#endif

#include <limits>
#include <algorithm>
#include <cmath>

#include <App/Application.h>
#include <App/Document.h>
#include <App/GroupExtension.h>
#include <App/Part.h>
#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/PropertyTopoShape.h>

#include "Preferences.h"
#include "Cosmetic.h"
#include "DrawGeomHatch.h"
#include "DrawHatch.h"
#include "DrawPage.h"
#include "DrawProjectSplit.h"
#include "DrawUtil.h"
#include "DrawViewBalloon.h"
#include "DrawViewDetail.h"
#include "DrawViewDimension.h"
#include "LandmarkDimension.h"
#include "DrawViewPart.h"
#include "DrawViewSection.h"
#include "EdgeWalker.h"
#include "Geometry.h"
#include "GeometryObject.h"
#include "LineGroup.h"
#include "ShapeExtractor.h"

#include <Mod/TechDraw/App/DrawViewPartPy.h>  // generated from DrawViewPartPy.xml

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewPart
//===========================================================================


//PROPERTY_SOURCE(TechDraw::DrawViewPart, TechDraw::DrawView)
PROPERTY_SOURCE_WITH_EXTENSIONS(TechDraw::DrawViewPart, 
                                TechDraw::DrawView)

DrawViewPart::DrawViewPart(void) :
    geometryObject(0)
{
    static const char *group = "Projection";
    static const char *sgroup = "HLR Parameters";
    nowUnsetting = false;
    m_handleFaces = false;

    CosmeticExtension::initExtension(this);

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().
                                         GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    double defDist = hGrp->GetFloat("FocusDistance",100.0);

    //properties that affect Geometry
    ADD_PROPERTY_TYPE(Source ,(0),group,App::Prop_None,"3D Shape to view");
    Source.setScope(App::LinkScope::Global);
    Source.setAllowExternal(true);
    ADD_PROPERTY_TYPE(XSource ,(0),group,App::Prop_None,"External 3D Shape to view");


    ADD_PROPERTY_TYPE(Direction ,(0.0,-1.0,0.0),
                      group,App::Prop_None,"Projection Plane normal. The direction you are looking from.");
    ADD_PROPERTY_TYPE(XDirection ,(0.0,0.0,0.0),
                      group,App::Prop_None,"Projection Plane X Axis in R3. Rotates/Mirrors View");
    ADD_PROPERTY_TYPE(Perspective ,(false),group,App::Prop_None,
                      "Perspective(true) or Orthographic(false) projection");
    ADD_PROPERTY_TYPE(Focus,(defDist),group,App::Prop_None,"Perspective view focus distance");

    //properties that control HLR algoaffect Appearance
    bool coarseView = hGrp->GetBool("CoarseView", false);
    ADD_PROPERTY_TYPE(CoarseView, (coarseView), sgroup, App::Prop_None, "Coarse View on/off");
    //add property for visible outline?
    ADD_PROPERTY_TYPE(SmoothVisible ,(prefSmoothViz()),sgroup,App::Prop_None,"Show Visible Smooth lines");
    ADD_PROPERTY_TYPE(SeamVisible ,(prefSeamViz()),sgroup,App::Prop_None,"Show Visible Seam lines");
    ADD_PROPERTY_TYPE(IsoVisible ,(prefIsoViz()),sgroup,App::Prop_None,"Show Visible Iso u,v lines");
    ADD_PROPERTY_TYPE(HardHidden ,(prefHardHid()),sgroup,App::Prop_None,"Show Hidden Hard lines");
    ADD_PROPERTY_TYPE(SmoothHidden ,(prefSmoothHid()),sgroup,App::Prop_None,"Show Hidden Smooth lines");
    ADD_PROPERTY_TYPE(SeamHidden ,(prefSeamHid()),sgroup,App::Prop_None,"Show Hidden Seam lines");
    ADD_PROPERTY_TYPE(IsoHidden ,(prefIsoHid()),sgroup,App::Prop_None,"Show Hidden Iso u,v lines");
    ADD_PROPERTY_TYPE(IsoCount ,(prefIsoCount()),sgroup,App::Prop_None,"Number of iso parameters lines");

    geometryObject = nullptr;
    //initialize bbox to non-garbage
    bbox = Base::BoundBox3d(Base::Vector3d(0.0, 0.0, 0.0), 0.0);
}

DrawViewPart::~DrawViewPart()
{
    removeAllReferencesFromGeom();
    delete geometryObject;
}

std::vector<TopoDS_Shape> DrawViewPart::getSourceShape2d(void) const
{
//    Base::Console().Message("DVP::getSourceShape2d()\n");
    std::vector<TopoDS_Shape> result;
    const std::vector<App::DocumentObject*>& links = getAllSources();
    result = ShapeExtractor::getShapes2d(links);
    return result;
}


TopoDS_Shape DrawViewPart::getSourceShape(void) const
{
//    Base::Console().Message("DVP::getSourceShape()\n");
    TopoDS_Shape result;
    const std::vector<App::DocumentObject*>& links = getAllSources();
    if (links.empty())  {
        bool isRestoring = getDocument()->testStatus(App::Document::Status::Restoring);
        if (isRestoring) {
            Base::Console().Warning("DVP::getSourceShape - No Sources (but document is restoring) - %s\n",
                                getNameInDocument());
        } else {
            Base::Console().Error("Error: DVP::getSourceShape - No Source(s) linked. - %s\n",
                                  getNameInDocument());
        }
    } else {
        result = ShapeExtractor::getShapes(links);
    }
    return result;
}

TopoDS_Shape DrawViewPart::getSourceShapeFused(void) const
{
//    Base::Console().Message("DVP::getSourceShapeFused()\n");
    TopoDS_Shape result;
//    const std::vector<App::DocumentObject*>& links = Source.getValues();
    const std::vector<App::DocumentObject*>& links = getAllSources();
    if (links.empty())  {
        bool isRestoring = getDocument()->testStatus(App::Document::Status::Restoring);
        if (isRestoring) {
            Base::Console().Warning("DVP::getSourceShape - No Sources (but document is restoring) - %s\n",
                                getNameInDocument());
        } else {
            Base::Console().Error("Error: DVP::getSourceShape - No Source(s) linked. - %s\n",
                                  getNameInDocument());
        }
    } else {
        result = ShapeExtractor::getShapesFused(links);
    }
    return result;
}

std::vector<App::DocumentObject*> DrawViewPart::getAllSources(void) const
{
//    Base::Console().Message("DVP::getAllSources()\n");
    const std::vector<App::DocumentObject*> links = Source.getValues();
    std::vector<DocumentObject*> xLinks = XSource.getValues();
//    std::vector<DocumentObject*> xLinks;
//    XSource.getLinks(xLinks);

    std::vector<App::DocumentObject*> result = links;
    if (!xLinks.empty()) {
        result.insert(result.end(), xLinks.begin(), xLinks.end());
    }
    return result;
}

App::DocumentObjectExecReturn *DrawViewPart::execute(void)
{
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }
    
    App::Document* doc = getDocument();
    bool isRestoring = doc->testStatus(App::Document::Status::Restoring);
    const std::vector<App::DocumentObject*>& links = getAllSources();
    if (links.empty())  {
        if (isRestoring) {
            Base::Console().Warning("DVP::execute - No Sources (but document is restoring) - %s\n",
                                getNameInDocument());
        } else {
            Base::Console().Error("Error: DVP::execute - No Source(s) linked. - %s\n",
                                  getNameInDocument());
        }
        return App::DocumentObject::StdReturn;
    }

    TopoDS_Shape shape = getSourceShape();
    if (shape.IsNull()) {
        if (isRestoring) {
            Base::Console().Warning("DVP::execute - source shape is invalid - (but document is restoring) - %s\n",
                                getNameInDocument());
        } else {
            Base::Console().Error("Error: DVP::execute - Source shape is Null. - %s\n",
                                  getNameInDocument());
        }
        return App::DocumentObject::StdReturn;
    }


    bool haveX = checkXDirection();
    if (!haveX) {
        //block touch/onChanged stuff
        Base::Vector3d newX = getXDirection();
        XDirection.setValue(newX);
        XDirection.purgeTouched();  //don't trigger updates!
        //unblock
    }

    m_saveShape = shape;
    partExec(shape);
    addShapes2d();

    //second pass if required
    if (ScaleType.isValue("Automatic")) {
        if (!checkFit()) {
            double newScale = autoScale();
            Scale.setValue(newScale);
            Scale.purgeTouched();
            partExec(shape);
        }
    }

//#endif //#if MOD_TECHDRAW_HANDLE_FACES
    return DrawView::execute();
}

short DrawViewPart::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  =  (Direction.isTouched()  ||
                    Source.isTouched()  ||
                    XSource.isTouched()  ||
                    Perspective.isTouched() ||
                    Focus.isTouched() ||
                    Rotation.isTouched() ||
                    SmoothVisible.isTouched() ||
                    SeamVisible.isTouched() ||
                    IsoVisible.isTouched() ||
                    HardHidden.isTouched() ||
                    SmoothHidden.isTouched() ||
                    SeamHidden.isTouched() ||
                    IsoHidden.isTouched() ||
                    IsoCount.isTouched() ||
                    CoarseView.isTouched() ||
                    CosmeticVertexes.isTouched() ||
                    CosmeticEdges.isTouched() ||
                    CenterLines.isTouched());
    }

    if (result) {
        return result;
    }
    return TechDraw::DrawView::mustExecute();
}

void DrawViewPart::onChanged(const App::Property* prop)
{
    // If the user has set PropertyVector Direction to zero, set it along the default value instead (Front View).
    // Otherwise bad things will happen because there'll be a normalization for direction calculations later.
    Base::Vector3d dir = Direction.getValue();
    if (DrawUtil::fpCompare(dir.Length(), 0.0)) {
        Direction.setValue(Base::Vector3d(0.0, -1.0, 0.0));
    }

    DrawView::onChanged(prop);

//TODO: when scale changes, any Dimensions for this View sb recalculated.  DVD should pick this up subject to topological naming issues.
}

void DrawViewPart::partExec(TopoDS_Shape shape)
{
//    Base::Console().Message("DVP::partExec()\n");
    if (geometryObject) {
        delete geometryObject;
        geometryObject = nullptr;
    }
    geometryObject = makeGeometryForShape(shape);
    if (geometryObject == nullptr) {
        return;
    }

#if MOD_TECHDRAW_HANDLE_FACES
    if (handleFaces() && !geometryObject->usePolygonHLR()) {
        try {
            extractFaces();
        }
        catch (Standard_Failure& e4) {
            Base::Console().Log("LOG - DVP::partExec - extractFaces failed for %s - %s **\n",getNameInDocument(),e4.GetMessageString());
        }
    }
#endif //#if MOD_TECHDRAW_HANDLE_FACES
//    std::vector<TechDraw::Vertex*> verts = getVertexGeometry();
    addCosmeticVertexesToGeom();
    addCosmeticEdgesToGeom();
    addCenterLinesToGeom();

    addReferencesToGeom();
}

void DrawViewPart::addShapes2d(void)
{
    std::vector<TopoDS_Shape> shapes = getSourceShape2d();
    for (auto& s: shapes) {
        //just vertices for now
        if (s.ShapeType() == TopAbs_VERTEX) {
            gp_Pnt gp = BRep_Tool::Pnt(TopoDS::Vertex(s));
            Base::Vector3d vp(gp.X(), gp.Y(), gp.Z());
            vp = vp - m_saveCentroid;
            //need to offset the point to match the big projection
            Base::Vector3d projected = projectPoint(vp * getScale());
            TechDraw::VertexPtr v1(std::make_shared<TechDraw::Vertex>(projected));
            geometryObject->addVertex(v1);
        } else if (s.ShapeType() == TopAbs_EDGE) {
              //not supporting edges yet. 
//            Base::Console().Message("DVP::add2dShapes - found loose edge - isNull: %d\n", s.IsNull());
//            TopoDS_Shape sTrans = TechDraw::moveShape(s,
//                                                      m_saveCentroid * -1.0);
//            TopoDS_Shape sScale = TechDraw::scaleShape(sTrans,
//                                                       getScale());
//            TopoDS_Shape sMirror = TechDraw::mirrorShape(sScale);
//            TopoDS_Edge edge = TopoDS::Edge(sMirror);
//            BaseGeom* bg = projectEdge(edge);

//            geometryObject->addEdge(bg);
            //save connection between source feat and this edge
        }
    }
}

GeometryObject* DrawViewPart::makeGeometryForShape(TopoDS_Shape shape)
{
    gp_Pnt inputCenter;
    Base::Vector3d stdOrg(0.0,0.0,0.0);

    gp_Ax2 viewAxis = getProjectionCS(stdOrg);

    inputCenter = TechDraw::findCentroid(shape,
                                         viewAxis);
    Base::Vector3d centroid(inputCenter.X(),
                            inputCenter.Y(),
                            inputCenter.Z());

    //center shape on origin
    TopoDS_Shape centeredShape = TechDraw::moveShape(shape,
                                                     centroid * -1.0);
    m_saveCentroid = centroid;
    m_saveShape = centeredShape;

    TopoDS_Shape scaledShape = TechDraw::scaleShape(centeredShape,
                                                    getScale());
    if (!DrawUtil::fpCompare(Rotation.getValue(),0.0)) {
        scaledShape = TechDraw::rotateShape(scaledShape,
                                            viewAxis,
                                            Rotation.getValue());  //conventional rotation
     }
//    BRepTools::Write(scaledShape, "DVPScaled.brep");            //debug
    GeometryObject* go =  buildGeometryObject(scaledShape,viewAxis);
    return go;
}

//note: slightly different than routine with same name in DrawProjectSplit
TechDraw::GeometryObject* DrawViewPart::buildGeometryObject(TopoDS_Shape shape, gp_Ax2 viewAxis)
{
    TechDraw::GeometryObject* go = new TechDraw::GeometryObject(getNameInDocument(), this);
    go->setIsoCount(IsoCount.getValue());
    go->isPerspective(Perspective.getValue());
    go->setFocus(Focus.getValue());
    go->usePolygonHLR(CoarseView.getValue());

    if (go->usePolygonHLR()){
        go->projectShapeWithPolygonAlgo(shape,
            viewAxis);
    }
    else{
        go->projectShape(shape,
            viewAxis);
    }

    go->extractGeometry(TechDraw::ecHARD,                   //always show the hard&outline visible lines
                        true);
    go->extractGeometry(TechDraw::ecOUTLINE,
                        true);
    if (SmoothVisible.getValue()) {
        go->extractGeometry(TechDraw::ecSMOOTH,
                            true);
    }
    if (SeamVisible.getValue()) {
        go->extractGeometry(TechDraw::ecSEAM,
                            true);
    }
    if ((IsoVisible.getValue()) && (IsoCount.getValue() > 0)) {
        go->extractGeometry(TechDraw::ecUVISO,
                            true);
    }
    if (HardHidden.getValue()) {
        go->extractGeometry(TechDraw::ecHARD,
                            false);
        go->extractGeometry(TechDraw::ecOUTLINE,
                            false);
    }
    if (SmoothHidden.getValue()) {
        go->extractGeometry(TechDraw::ecSMOOTH,
                            false);
    }
    if (SeamHidden.getValue()) {
        go->extractGeometry(TechDraw::ecSEAM,
                            false);
    }
    if (IsoHidden.getValue() && (IsoCount.getValue() > 0)) {
        go->extractGeometry(TechDraw::ecUVISO,
                            false);
    }

    const std::vector<TechDraw::BaseGeom  *> & edges = go->getEdgeGeometry();
    if (edges.empty()) {
        Base::Console().Log("DVP::buildGO - NO extracted edges!\n");
    }
    bbox = go->calcBoundingBox();
    return go;
}

//! make faces from the existing edge geometry
void DrawViewPart::extractFaces()
{
    if (geometryObject == nullptr) {
        return;
    }
    geometryObject->clearFaceGeom();
    const std::vector<TechDraw::BaseGeom*>& goEdges =
                       geometryObject->getVisibleFaceEdges(SmoothVisible.getValue(),SeamVisible.getValue());
    std::vector<TechDraw::BaseGeom*>::const_iterator itEdge = goEdges.begin();
    std::vector<TopoDS_Edge> origEdges;
    for (;itEdge != goEdges.end(); itEdge++) {
        origEdges.push_back((*itEdge)->occEdge);
    }

    std::vector<TopoDS_Edge> faceEdges;
    std::vector<TopoDS_Edge> nonZero;
    for (auto& e:origEdges) {                            //drop any zero edges (shouldn't be any by now!!!)
        if (!DrawUtil::isZeroEdge(e)) {
            nonZero.push_back(e);
        } else {
            Base::Console().Log("INFO - DVP::extractFaces for %s found ZeroEdge!\n",getNameInDocument());
        }
    }

    //HLR algo does not provide all edge intersections for edge endpoints.
    //need to split long edges touched by Vertex of another edge
    std::vector<splitPoint> splits;
    std::vector<TopoDS_Edge>::iterator itOuter = nonZero.begin();
    int iOuter = 0;
    for (; itOuter != nonZero.end(); ++itOuter, iOuter++) {    //*** itOuter != nonZero.end() - 1
        TopoDS_Vertex v1 = TopExp::FirstVertex((*itOuter));
        TopoDS_Vertex v2 = TopExp::LastVertex((*itOuter));
        Bnd_Box sOuter;
        BRepBndLib::Add(*itOuter, sOuter);
        sOuter.SetGap(0.1);
        if (sOuter.IsVoid()) {
            Base::Console().Log("DVP::Extract Faces - outer Bnd_Box is void for %s\n",getNameInDocument());
            continue;
        }
        if (DrawUtil::isZeroEdge(*itOuter)) {
            Base::Console().Log("DVP::extractFaces - outerEdge: %d is ZeroEdge\n",iOuter);   //this is not finding ZeroEdges
            continue;  //skip zero length edges. shouldn't happen ;)
        }
        int iInner = 0;
        std::vector<TopoDS_Edge>::iterator itInner = nonZero.begin();   //***sb itOuter + 1;
        for (; itInner != nonZero.end(); ++itInner,iInner++) {
            if (iInner == iOuter) {
                continue;
            }
            if (DrawUtil::isZeroEdge((*itInner))) {
                continue;  //skip zero length edges. shouldn't happen ;)
            }

            Bnd_Box sInner;
            BRepBndLib::Add(*itInner, sInner);
            sInner.SetGap(0.1);
            if (sInner.IsVoid()) {
                Base::Console().Log("INFO - DVP::Extract Faces - inner Bnd_Box is void for %s\n",getNameInDocument());
                continue;
            }
            if (sOuter.IsOut(sInner)) {      //bboxes of edges don't intersect, don't bother
                continue;
            }

            double param = -1;
            if (DrawProjectSplit::isOnEdge((*itInner),v1,param,false)) {
                gp_Pnt pnt1 = BRep_Tool::Pnt(v1);
                splitPoint s1;
                s1.i = iInner;
                s1.v = Base::Vector3d(pnt1.X(),pnt1.Y(),pnt1.Z());
                s1.param = param;
                splits.push_back(s1);
            }
            if (DrawProjectSplit::isOnEdge((*itInner),v2,param,false)) {
                gp_Pnt pnt2 = BRep_Tool::Pnt(v2);
                splitPoint s2;
                s2.i = iInner;
                s2.v = Base::Vector3d(pnt2.X(),pnt2.Y(),pnt2.Z());
                s2.param = param;
                splits.push_back(s2);
            }
        } //inner loop
    }   //outer loop

    std::vector<splitPoint> sorted = DrawProjectSplit::sortSplits(splits,true);
    auto last = std::unique(sorted.begin(), sorted.end(), DrawProjectSplit::splitEqual);  //duplicates to back
    sorted.erase(last, sorted.end());                         //remove dupl splits
    std::vector<TopoDS_Edge> newEdges = DrawProjectSplit::splitEdges(nonZero,sorted);

    if (newEdges.empty()) {
        Base::Console().Log("DVP::extractFaces - no newEdges\n");
        return;
    }

    newEdges = DrawProjectSplit::removeDuplicateEdges(newEdges);        //<<< here

//find all the wires in the pile of faceEdges
    EdgeWalker ew;
    ew.loadEdges(newEdges);
    bool success = ew.perform();
    if (!success) {
        Base::Console().Warning("DVP::extractFaces - %s -Can't make faces from projected edges\n", getNameInDocument());
        return;
    }
    std::vector<TopoDS_Wire> fw = ew.getResultNoDups();

    std::vector<TopoDS_Wire> sortedWires = ew.sortStrip(fw,true);

//    int idb = 0;
    std::vector<TopoDS_Wire>::iterator itWire = sortedWires.begin();
    for (; itWire != sortedWires.end(); itWire++) {
        //version 1: 1 wire/face - no voids in face
//debug
//        std::stringstream ss;
//        ss << "DVPSWire" << idb << ".brep";
//        std::string wireName = ss.str();
//        BRepTools::Write((*itWire), wireName.c_str());            //debug
//debug        idb++;
        TechDraw::FacePtr f(std::make_shared<TechDraw::Face>());
        const TopoDS_Wire& wire = (*itWire);
        TechDraw::Wire* w = new TechDraw::Wire(wire);
        f->wires.push_back(w);
        geometryObject->addFaceGeom(f);
    }
}

std::vector<TechDraw::DrawHatch*> DrawViewPart::getHatches() const
{
    std::vector<TechDraw::DrawHatch*> result;
    std::vector<App::DocumentObject*> children = getInList();
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != children.end(); ++it) {
        if ( ((*it)->getTypeId().isDerivedFrom(DrawHatch::getClassTypeId())) &&
            (!(*it)->isRemoving()) ) {
            TechDraw::DrawHatch* hatch = dynamic_cast<TechDraw::DrawHatch*>(*it);
            result.push_back(hatch);
        }
    }
    return result;
}

std::vector<TechDraw::DrawGeomHatch*> DrawViewPart::getGeomHatches() const
{
    std::vector<TechDraw::DrawGeomHatch*> result;
    std::vector<App::DocumentObject*> children = getInList();
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != children.end(); ++it) {
        if ( ((*it)->getTypeId().isDerivedFrom(DrawGeomHatch::getClassTypeId())) &&
             (!(*it)->isRemoving()) ) {
            TechDraw::DrawGeomHatch* geom = dynamic_cast<TechDraw::DrawGeomHatch*>(*it);
            result.push_back(geom);
        }
    }
    return result;
}

//return *unique* list of Dimensions which reference this DVP
std::vector<TechDraw::DrawViewDimension*> DrawViewPart::getDimensions() const
{
    std::vector<TechDraw::DrawViewDimension*> result;
    std::vector<App::DocumentObject*> children = getInList();
    std::sort(children.begin(),children.end(),std::less<App::DocumentObject*>());
    std::vector<App::DocumentObject*>::iterator newEnd = std::unique(children.begin(),children.end());
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != newEnd; ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawViewDimension::getClassTypeId())) {
            TechDraw::DrawViewDimension* dim = dynamic_cast<TechDraw::DrawViewDimension*>(*it);
            result.push_back(dim);
        }
    }
    return result;
}

std::vector<TechDraw::DrawViewBalloon*> DrawViewPart::getBalloons() const
{
    std::vector<TechDraw::DrawViewBalloon*> result;
    std::vector<App::DocumentObject*> children = getInList();
    std::sort(children.begin(),children.end(),std::less<App::DocumentObject*>());
    std::vector<App::DocumentObject*>::iterator newEnd = std::unique(children.begin(),children.end());
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != newEnd; ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawViewBalloon::getClassTypeId())) {
            TechDraw::DrawViewBalloon* balloon = dynamic_cast<TechDraw::DrawViewBalloon*>(*it);
            result.push_back(balloon);
        }
    }
    return result;
}

const std::vector<TechDraw::VertexPtr> DrawViewPart::getVertexGeometry() const
{
    std::vector<TechDraw::VertexPtr> result;
    if (geometryObject != nullptr) {
        result = geometryObject->getVertexGeometry();
    }
    return result;
}

const std::vector<TechDraw::FacePtr> DrawViewPart::getFaceGeometry() const
{
    std::vector<TechDraw::FacePtr> result;
    if (geometryObject != nullptr) {
        result = geometryObject->getFaceGeometry();
    }
    return result;
}

const std::vector<TechDraw::BaseGeom*> DrawViewPart::getEdgeGeometry() const
{
    std::vector<TechDraw::BaseGeom  *> result;
    if (geometryObject != nullptr) {
        result = geometryObject->getEdgeGeometry();
    }
    return result;
}

//! returns existing BaseGeom of 2D Edge(idx)
TechDraw::BaseGeom* DrawViewPart::getGeomByIndex(int idx) const
{
    const std::vector<TechDraw::BaseGeom *> &geoms = getEdgeGeometry();
    if (geoms.empty()) {
        Base::Console().Log("INFO - getGeomByIndex(%d) - no Edge Geometry. Probably restoring?\n",idx);
        return NULL;
    }
    if ((unsigned)idx >= geoms.size()) {
        Base::Console().Log("INFO - getGeomByIndex(%d) - invalid index\n",idx);
        return NULL;
    }
    return geoms.at(idx);
}

//! returns existing geometry of 2D Vertex(idx)
TechDraw::VertexPtr DrawViewPart::getProjVertexByIndex(int idx) const
{
    const std::vector<TechDraw::VertexPtr> &geoms = getVertexGeometry();
    if (geoms.empty()) {
        Base::Console().Log("INFO - getProjVertexByIndex(%d) - no Vertex Geometry. Probably restoring?\n",idx);
        return NULL;
    }
    if ((unsigned)idx >= geoms.size()) {
        Base::Console().Log("INFO - getProjVertexByIndex(%d) - invalid index\n",idx);
        return NULL;
    }
    return geoms.at(idx);
}

TechDraw::VertexPtr DrawViewPart::getProjVertexByCosTag(std::string cosTag)
{
    TechDraw::VertexPtr result = nullptr;
    std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
    if (gVerts.empty()) {
        Base::Console().Log("INFO - getProjVertexByCosTag(%s) - no Vertex Geometry.\n");
        return result;
    }
    
    for (auto& gv: gVerts) {
        if (gv->cosmeticTag == cosTag) {
            result = gv;
            break;
        }
    }
    return result;
}


//! returns existing geometry of 2D Face(idx)
std::vector<TechDraw::BaseGeom*> DrawViewPart::getFaceEdgesByIndex(int idx) const
{
    std::vector<TechDraw::BaseGeom*> result;
    const std::vector<TechDraw::FacePtr>& faces = getFaceGeometry();
    if (idx < (int) faces.size()) {
        TechDraw::FacePtr projFace = faces.at(idx);
        for (auto& w: projFace->wires) {
            for (auto& g:w->geoms) {
                if (g->cosmetic) {
                    //if g is cosmetic, we should skip it
                    Base::Console().Log("DVP::getFaceEdgesByIndex - found cosmetic edge\n");
                } else {
                    result.push_back(g);
                }
            }
        }
    }
    return result;
}

std::vector<TopoDS_Wire> DrawViewPart::getWireForFace(int idx) const
{
    std::vector<TopoDS_Wire> result;
    std::vector<TopoDS_Edge> edges;
    const std::vector<TechDraw::FacePtr>& faces = getFaceGeometry();
    TechDraw::FacePtr ourFace = faces.at(idx);
    for (auto& w:ourFace->wires) {
        edges.clear();
        int i = 0;
        for (auto& g:w->geoms) {
            edges.push_back(g->occEdge);
            i++;
        }
        TopoDS_Wire occwire = EdgeWalker::makeCleanWire(edges);
        result.push_back(occwire);
    }

    return result;
}

Base::BoundBox3d DrawViewPart::getBoundingBox() const
{
    return bbox;
}

double DrawViewPart::getBoxX(void) const
{
    Base::BoundBox3d bbx = getBoundingBox();   //bbox is already scaled & centered!
    return (bbx.MaxX - bbx.MinX);
}

double DrawViewPart::getBoxY(void) const
{
    Base::BoundBox3d bbx = getBoundingBox();
    return (bbx.MaxY - bbx.MinY);
}

QRectF DrawViewPart::getRect() const
{
//    Base::Console().Message("DVP::getRect() - %s\n", getNameInDocument());
    double x = getBoxX();
    double y = getBoxY();
    QRectF result(0.0, 0.0, x, y);
    return result;
}

//used to project a pt (ex SectionOrigin) onto paper plane
Base::Vector3d DrawViewPart::projectPoint(const Base::Vector3d& pt, bool invert) const
{
//    Base::Console().Message("DVP::projectPoint()\n");
    Base::Vector3d stdOrg(0.0,0.0,0.0);
    gp_Ax2 viewAxis = getProjectionCS(stdOrg);
    gp_Pnt gPt(pt.x,pt.y,pt.z);

    HLRAlgo_Projector projector( viewAxis );
    gp_Pnt2d prjPnt;
    projector.Project(gPt, prjPnt);
    Base::Vector3d result(prjPnt.X(),prjPnt.Y(), 0.0);
    if (invert) {
        result = DrawUtil::invertY(result);
    }
    return result;
}

//project a loose edge onto the paper plane
//TODO:: loose edges not supported yet
BaseGeom* DrawViewPart::projectEdge(const TopoDS_Edge& e) const
{
    Base::Vector3d stdOrg(0.0,0.0,0.0);
    gp_Ax2 viewAxis = getProjectionCS(stdOrg);

    gp_Pln plane(viewAxis);
    TopoDS_Face paper = BRepBuilderAPI_MakeFace(plane);
    BRepAlgo_NormalProjection projector(paper);
    projector.Add(e);
    projector.Build();
    TopoDS_Shape s = projector.Projection();
//    Base::Console().Message("DVP::projectEdge - s.IsNull: %d\n", s.IsNull());
//    BaseGeom* result = BaseGeom::baseFactory(pe);
    BaseGeom* result = nullptr;
    return result;
}

bool DrawViewPart::hasGeometry(void) const
{
    bool result = false;
    if (geometryObject == nullptr) {
        return result;
    }
    const std::vector<TechDraw::VertexPtr> &verts = getVertexGeometry();
    const std::vector<TechDraw::BaseGeom*> &edges = getEdgeGeometry();
    if (verts.empty() &&
        edges.empty() ) {
        result = false;
    } else {
        result = true;
    }
    return result;
}

gp_Ax2 DrawViewPart::getProjectionCS(const Base::Vector3d pt) const
{
//    Base::Console().Message("DVP::getProjectionCS() - %s - %s\n", getNameInDocument(), Label.getValue());
    Base::Vector3d direction = Direction.getValue();
    gp_Dir gDir(direction.x,
                direction.y,
                direction.z);
    Base::Vector3d xDir = getXDirection();
    gp_Dir gXDir(xDir.x,
                 xDir.y,
                 xDir.z);
    gp_Pnt gOrg(pt.x,
                pt.y,
                pt.z);
    gp_Ax2 viewAxis(gOrg,
                    gDir);
    try {
        viewAxis = gp_Ax2(gOrg,
                          gDir,
                          gXDir);
    }
    catch (...) {
        Base::Console().Warning("DVP - %s - failed to create projection CS\n", getNameInDocument());
    }
    return viewAxis;
}

gp_Ax2 DrawViewPart::getViewAxis(const Base::Vector3d& pt,
                               const Base::Vector3d& direction,
                               const bool flip) const
{
    (void) direction;
    (void) flip;
    Base::Console().Message("DVP::getViewAxis - deprecated. Use getProjectionCS.\n");
    return getProjectionCS(pt);
}

//TODO: make saveShape a property

Base::Vector3d DrawViewPart::getOriginalCentroid(void) const
{
    return m_saveCentroid;
}

std::vector<DrawViewSection*> DrawViewPart::getSectionRefs(void) const
{
    std::vector<DrawViewSection*> result;
    std::vector<App::DocumentObject*> inObjs = getInList();
    for (auto& o:inObjs) {
        if (o->getTypeId().isDerivedFrom(DrawViewSection::getClassTypeId())) {
            result.push_back(static_cast<TechDraw::DrawViewSection*>(o));
        }
    }
    return result;
}

std::vector<DrawViewDetail*> DrawViewPart::getDetailRefs(void) const
{
    std::vector<DrawViewDetail*> result;
    std::vector<App::DocumentObject*> inObjs = getInList();
    for (auto& o:inObjs) {
        if (o->getTypeId().isDerivedFrom(DrawViewDetail::getClassTypeId())) {
            if (!o->isRemoving()) {
                result.push_back(static_cast<TechDraw::DrawViewDetail*>(o));
            }
        }
    }
    return result;
}

const std::vector<TechDraw::BaseGeom  *> DrawViewPart::getVisibleFaceEdges() const
{
    return geometryObject->getVisibleFaceEdges(SmoothVisible.getValue(),SeamVisible.getValue());
}

bool DrawViewPart::handleFaces(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    return hGrp->GetBool("HandleFaces", 1l);
}

//! remove features that are useless without this DVP
//! hatches, geomhatches, dimensions,...
void DrawViewPart::unsetupObject()
{
    nowUnsetting = true;
    App::Document* doc = getDocument();
    std::string docName = doc->getName();

    // Remove the View's Hatches from document
    std::vector<TechDraw::DrawHatch*> hatches = getHatches();
    std::vector<TechDraw::DrawHatch*>::iterator it = hatches.begin();
    for (; it != hatches.end(); it++) {
        std::string viewName = (*it)->getNameInDocument();
        Base::Interpreter().runStringArg("App.getDocument(\"%s\").removeObject(\"%s\")",
                                          docName.c_str(), viewName.c_str());
    }

    // Remove the View's GeomHatches from document
    std::vector<TechDraw::DrawGeomHatch*> gHatches = getGeomHatches();
    std::vector<TechDraw::DrawGeomHatch*>::iterator it2 = gHatches.begin();
    for (; it2 != gHatches.end(); it2++) {
        std::string viewName = (*it2)->getNameInDocument();
        Base::Interpreter().runStringArg("App.getDocument(\"%s\").removeObject(\"%s\")",
                                          docName.c_str(), viewName.c_str());
    }

    // Remove Dimensions which reference this DVP
    // must use page->removeObject first
    TechDraw::DrawPage* page = findParentPage();
    if (page != nullptr) {
        std::vector<TechDraw::DrawViewDimension*> dims = getDimensions();
        std::vector<TechDraw::DrawViewDimension*>::iterator it3 = dims.begin();
        for (; it3 != dims.end(); it3++) {
            page->removeView(*it3);
            const char* name = (*it3)->getNameInDocument();
            if (name) {
                Base::Interpreter().runStringArg("App.getDocument(\"%s\").removeObject(\"%s\")",
                                                docName.c_str(), name);
            }
        }
    }

    // Remove Balloons which reference this DVP
    // must use page->removeObject first
    page = findParentPage();
    if (page != nullptr) {
        std::vector<TechDraw::DrawViewBalloon*> balloons = getBalloons();
        std::vector<TechDraw::DrawViewBalloon*>::iterator it3 = balloons.begin();
        for (; it3 != balloons.end(); it3++) {
            page->removeView(*it3);
            const char* name = (*it3)->getNameInDocument();
            if (name) {
                Base::Interpreter().runStringArg("App.getDocument(\"%s\").removeObject(\"%s\")",
                                                docName.c_str(), name);
            }
        }
    }
}

//! is this an Isometric projection?
bool DrawViewPart::isIso(void) const
{
    bool result = false;
    Base::Vector3d dir = Direction.getValue();
    if ( DrawUtil::fpCompare(fabs(dir.x),fabs(dir.y))  &&
         DrawUtil::fpCompare(fabs(dir.x),fabs(dir.z)) ) {
        result = true;
    }
    return result;
}

bool DrawViewPart::checkXDirection(void) const
{
//    Base::Console().Message("DVP::checkXDirection()\n");
    Base::Vector3d xDir = XDirection.getValue();
    if (DrawUtil::fpCompare(xDir.Length(), 0.0))  {
        Base::Vector3d dir = Direction.getValue();
        Base::Vector3d origin(0.0,0.0,0.0);
        Base::Vector3d xDir = getLegacyX(origin,
                                         dir);
        Base::Console().Log("DVP - %s - XDirection property not set. Trying %s\n",
                                getNameInDocument(),
                                DrawUtil::formatVector(xDir).c_str());
        return false;
    }
    return true;
}

//
Base::Vector3d DrawViewPart::getXDirection(void) const
{
//    Base::Console().Message("DVP::getXDirection() - %s\n", Label.getValue());
    Base::Vector3d result(1.0, 0.0, 0.0);               //default X
    App::Property* prop = getPropertyByName("XDirection");
    if (prop != nullptr) {                              //have an XDirection property
        Base::Vector3d propVal = XDirection.getValue();
        if (DrawUtil::fpCompare(propVal.Length(), 0.0))  {   //but it has no value
            Base::Vector3d dir = Direction.getValue();       //make a sensible default
            Base::Vector3d org(0.0, 0.0, 0.0);
            result = getLegacyX(org,
                                dir);
        } else {
            result = propVal;                               //normal case.  XDirection is set.
        }
    } else {                                                //no Property.  can this happen?
            Base::Vector3d dir = Direction.getValue();      //make a sensible default
            Base::Vector3d org(0.0, 0.0, 0.0);
            result = getLegacyX(org,
                                dir);
    }
    return result;
}

Base::Vector3d DrawViewPart::getLegacyX(const Base::Vector3d& pt,
                                        const Base::Vector3d& axis,
                                        const bool flip)  const
{
//    Base::Console().Message("DVP::getLegacyX() - %s\n", Label.getValue());
    gp_Ax2 viewAxis = TechDraw::legacyViewAxis1(pt, axis, flip);
    gp_Dir gXDir = viewAxis.XDirection();
    Base::Vector3d result(gXDir.X(),
                          gXDir.Y(),
                          gXDir.Z());
    return result;
}


void DrawViewPart::updateReferenceVert(std::string tag, Base::Vector3d loc2d)
{
    for (auto& v: m_referenceVerts) {
        if (v->getTagAsString() == tag) {
            v->pnt = loc2d;
            break;
        }
    }
}

void DrawViewPart::addReferencesToGeom(void)
{
//    Base::Console().Message("DVP::addReferencesToGeom() - %s\n", getNameInDocument());
    std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
    gVerts.insert(gVerts.end(), m_referenceVerts.begin(), m_referenceVerts.end());
    getGeometryObject()->setVertexGeometry(gVerts);
}

//add a vertex that is not part of the geometry, but is used by
//ex. LandmarkDimension as a reference
std::string DrawViewPart::addReferenceVertex(Base::Vector3d v)
{
//    Base::Console().Message("DVP::addReferenceVertex(%s) - %s\n", 
//                            DrawUtil::formatVector(v).c_str(), getNameInDocument());
    std::string refTag;
//    Base::Vector3d scaledV = v * getScale();
//    TechDraw::Vertex* ref = new TechDraw::Vertex(scaledV);
    Base::Vector3d scaledV = v;
    TechDraw::VertexPtr ref(std::make_shared<TechDraw::Vertex>(scaledV));
    ref->reference = true;
    refTag = ref->getTagAsString();
    m_referenceVerts.push_back(ref);
    return refTag;
}

void DrawViewPart::removeReferenceVertex(std::string tag)
{
    std::vector<TechDraw::VertexPtr> newRefVerts;
    for (auto& v: m_referenceVerts) {
        if (v->getTagAsString() != tag) {
            newRefVerts.push_back(v);
        } else {
//            delete v;  //??? who deletes v?
        }
    }
    m_referenceVerts = newRefVerts;
    resetReferenceVerts();  
}

void DrawViewPart::removeAllReferencesFromGeom()
{
//    Base::Console().Message("DVP::removeAllReferencesFromGeom()\n");
    if (!m_referenceVerts.empty()) {
        std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
        std::vector<TechDraw::VertexPtr> newVerts;
        for (auto& gv: gVerts) {
            if (!gv->reference) {
                newVerts.push_back(gv);
            }
        }
        getGeometryObject()->setVertexGeometry(newVerts);
    }
}

void DrawViewPart::resetReferenceVerts()
{
//    Base::Console().Message("DVP::resetReferenceVerts() %s\n", getNameInDocument());
    removeAllReferencesFromGeom();
    addReferencesToGeom();
}

//********
//* Cosmetics
//********

void DrawViewPart::clearCosmeticVertexes(void)
{
    std::vector<CosmeticVertex*> noVerts;
    CosmeticVertexes.setValues(noVerts);
}

//add the cosmetic verts to geometry vertex list
void DrawViewPart::addCosmeticVertexesToGeom(void)
{
//    Base::Console().Message("DVP::addCosmeticVertexesToGeom()\n");
    const std::vector<TechDraw::CosmeticVertex*> cVerts = CosmeticVertexes.getValues();
    for (auto& cv: cVerts) {
        int iGV = geometryObject->addCosmeticVertex(cv->scaled(getScale()),
                                                    cv->getTagAsString());
        cv->linkGeom = iGV;
    }
}

int DrawViewPart::add1CVToGV(std::string tag)
{
//    Base::Console().Message("DVP::add1CVToGV(%s) 2\n", tag.c_str());
    TechDraw::CosmeticVertex* cv = getCosmeticVertex(tag);
    if (cv == nullptr) {
        Base::Console().Message("DVP::add1CVToGV 2 - cv %s not found\n", tag.c_str());
        return 0;
    }
    int iGV = geometryObject->addCosmeticVertex(cv->scaled(getScale()),
                                                cv->getTagAsString());
    cv->linkGeom = iGV;
    return iGV;
}

//update Vertex geometry with current CV's 
void DrawViewPart::refreshCVGeoms(void)
{
//    Base::Console().Message("DVP::refreshCVGeoms()\n");

    std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
    std::vector<TechDraw::VertexPtr> newGVerts;
    for (auto& gv :gVerts) {
        if (gv->cosmeticTag.empty()) {       //keep only non-cv vertices
            newGVerts.push_back(gv);
        }
    }
    getGeometryObject()->setVertexGeometry(newGVerts);
    addCosmeticVertexesToGeom();
}

//what is the CV's position in the big geometry q
int DrawViewPart::getCVIndex(std::string tag)
{
//    Base::Console().Message("DVP::getCVIndex(%s)\n", tag.c_str());
    int result = -1;
    std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
    std::vector<TechDraw::CosmeticVertex*> cVerts = CosmeticVertexes.getValues();

    int i = 0;
    bool found = false;
    for (auto& gv :gVerts) {
        if (gv->cosmeticTag == tag) {
            result = i;
            found = true;
            break;
        }
        i++;
    }
    if (!found) {       //not in vertexGeoms
        int base = gVerts.size();
        int i = 0;
        for (auto& cv: cVerts) {
//        Base::Console().Message("DVP::getCVIndex - cv tag: %s\n", 
//                                cv->getTagAsString().c_str());
            if (cv->getTagAsString() == tag) {
                result = base + i;
                break;
            }
            i++;
        }
    }
//    Base::Console().Message("DVP::getCVIndex - returns: %d\n", result);
    return result;
}


//CosmeticEdges -------------------------------------------------------------------

//for completeness.  not actually used anywhere?
void DrawViewPart::clearCosmeticEdges(void)
{
    std::vector<CosmeticEdge*> noEdges;
    CosmeticEdges.setValues(noEdges);
}

//add the cosmetic edges to geometry edge list
void DrawViewPart::addCosmeticEdgesToGeom(void)
{
//    Base::Console().Message("CEx::addCosmeticEdgesToGeom()\n");
    const std::vector<TechDraw::CosmeticEdge*> cEdges = CosmeticEdges.getValues();
    for (auto& ce: cEdges) {
        TechDraw::BaseGeom* scaledGeom = ce->scaledGeometry(getScale());
        if (scaledGeom == nullptr) {
            continue;
        }
//        int iGE = 
        geometryObject->addCosmeticEdge(scaledGeom,
                                        ce->getTagAsString());
    }
}

int DrawViewPart::add1CEToGE(std::string tag)
{
//    Base::Console().Message("CEx::add1CEToGE(%s) 2\n", tag.c_str());
    TechDraw::CosmeticEdge* ce = getCosmeticEdge(tag);
    if (ce == nullptr) {
        Base::Console().Message("CEx::add1CEToGE 2 - ce %s not found\n", tag.c_str());
        return -1;
    }
    TechDraw::BaseGeom* scaledGeom = ce->scaledGeometry(getScale());
    int iGE = geometryObject->addCosmeticEdge(scaledGeom,
                                              tag);
                                                
    return iGE;
}

//update Edge geometry with current CE's 
void DrawViewPart::refreshCEGeoms(void)
{
//    Base::Console().Message("DVP::refreshCEGeoms()\n");
    std::vector<TechDraw::BaseGeom *> gEdges = getEdgeGeometry();
    std::vector<TechDraw::BaseGeom *> oldGEdges;
    for (auto& ge :gEdges) {
        if (ge->source() != SourceType::COSEDGE)  {
            oldGEdges.push_back(ge);
        }
    }
    getGeometryObject()->setEdgeGeometry(oldGEdges);
    addCosmeticEdgesToGeom();
}


// CenterLines -----------------------------------------------------------------
void DrawViewPart::clearCenterLines(void)
{
    std::vector<CenterLine*> noLines;
    CenterLines.setValues(noLines);
}

int DrawViewPart::add1CLToGE(std::string tag)
{
//    Base::Console().Message("CEx::add1CLToGE(%s) 2\n", tag.c_str());
    TechDraw::CenterLine* cl = getCenterLine(tag);
    if (cl == nullptr) {
        Base::Console().Message("CEx::add1CLToGE 2 - cl %s not found\n", tag.c_str());
        return -1;
    }
    TechDraw::BaseGeom* scaledGeom = cl->scaledGeometry(this);
    int iGE = geometryObject->addCenterLine(scaledGeom,
                                            tag);
                                                
    return iGE;
}

//update Edge geometry with current CL's 
void DrawViewPart::refreshCLGeoms(void)
{
//    Base::Console().Message("DVP::refreshCLGeoms()\n");
    std::vector<TechDraw::BaseGeom *> gEdges = getEdgeGeometry();
    std::vector<TechDraw::BaseGeom *> newGEdges;
    for (auto& ge :gEdges) {
        if (ge->source() != SourceType::CENTERLINE)  {
            newGEdges.push_back(ge);
        }
    }
    getGeometryObject()->setEdgeGeometry(newGEdges);
    addCenterLinesToGeom();
}

//add the center lines to geometry Edges list
void DrawViewPart::addCenterLinesToGeom(void)
{
//   Base::Console().Message("DVP::addCenterLinesToGeom()\n");
    const std::vector<TechDraw::CenterLine*> lines = CenterLines.getValues();
    for (auto& cl: lines) {
        TechDraw::BaseGeom* scaledGeom = cl->scaledGeometry(this);
        if (scaledGeom == nullptr) {
            Base::Console().Error("DVP::addCenterLinesToGeom - scaledGeometry is null\n");
            continue;
        }
//        int idx =
        (void) geometryObject->addCenterLine(scaledGeom, cl->getTagAsString());
    }
}

// GeomFormats -----------------------------------------------------------------

void DrawViewPart::clearGeomFormats(void)
{
    std::vector<GeomFormat*> noFormats;
    std::vector<GeomFormat*> fmts = GeomFormats.getValues();
    GeomFormats.setValues(noFormats);
    for (auto& f: fmts) {
        delete f;
    }
}

//------------------------------------------------------------------------------

void DrawViewPart::dumpVerts(std::string text)
{
    if (geometryObject == nullptr) {
        Base::Console().Message("no verts to dump yet\n");
        return;
    }
    std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
    Base::Console().Message("%s - dumping %d vertGeoms\n",
                            text.c_str(), gVerts.size());
    for (auto& gv: gVerts) {
        gv->dump();
    }
}

void DrawViewPart::dumpCosVerts(std::string text)
{
    std::vector<TechDraw::CosmeticVertex*> cVerts = CosmeticVertexes.getValues();
    Base::Console().Message("%s - dumping %d CosmeticVertexes\n",
                            text.c_str(), cVerts.size());
    for (auto& cv: cVerts) {
        cv->dump("a CV");
    }
}

void DrawViewPart::dumpCosEdges(std::string text)
{
    std::vector<TechDraw::CosmeticEdge*> cEdges = CosmeticEdges.getValues();
    Base::Console().Message("%s - dumping %d CosmeticEdge\n",
                            text.c_str(), cEdges.size());
    for (auto& ce: cEdges) {
        ce->dump("a CE");
    }
}



void DrawViewPart::onDocumentRestored()
{
//    requestPaint();
    //if execute has not run yet, there will be no GO, and paint will not do anything.
    recomputeFeature();
    DrawView::onDocumentRestored();
}

PyObject *DrawViewPart::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewPartPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

void DrawViewPart::handleChangedPropertyName(Base::XMLReader &reader, const char* TypeName, const char* PropName)
{
//    extHandleChangedPropertyName(reader, TypeName, PropName); // CosmeticExtension
    DrawView::handleChangedPropertyName(reader, TypeName, PropName);
}

bool DrawViewPart::prefHardViz(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("HardViz", true); 
    return result;
}

bool DrawViewPart::prefSeamViz(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("SeamViz", true); 
    return result;
}

bool DrawViewPart::prefSmoothViz(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("SmoothViz", true); 
    return result;
}

bool DrawViewPart::prefIsoViz(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("IsoViz", false); 
    return result;
}

bool DrawViewPart::prefHardHid(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("HardHid",  false); 
    return result;
}

bool DrawViewPart::prefSeamHid(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("SeamHid", false); 
    return result;
}

bool DrawViewPart::prefSmoothHid(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("SmoothHid", false); 
    return result;
}

bool DrawViewPart::prefIsoHid(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("IsoHid", false); 
    return result;
}

int DrawViewPart::prefIsoCount(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    int result = hGrp->GetBool("IsoCount", 0); 
    return result;
}


// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewPartPython, TechDraw::DrawViewPart)
template<> const char* TechDraw::DrawViewPartPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderViewPart";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewPart>;
}
