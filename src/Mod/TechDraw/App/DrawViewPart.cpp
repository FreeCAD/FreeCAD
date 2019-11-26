/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
 *   Copyright (c) Luke Parry             (l.parry@warwick.ac.uk) 2013     *
 *   Copyright (c) WandererFan            (wandererfan@gmail.com) 2016     *
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
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>

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

#include "Cosmetic.h"
#include "DrawGeomHatch.h"
#include "DrawHatch.h"
#include "DrawPage.h"
#include "DrawProjectSplit.h"
#include "DrawUtil.h"
#include "DrawViewBalloon.h"
#include "DrawViewDetail.h"
#include "DrawViewDimension.h"
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


PROPERTY_SOURCE(TechDraw::DrawViewPart, TechDraw::DrawView)

DrawViewPart::DrawViewPart(void) :
    geometryObject(0)
{
    static const char *group = "Projection";
    static const char *sgroup = "HLR Parameters";
    nowUnsetting = false;

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                                               GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    double defDist = hGrp->GetFloat("FocusDistance",100.0);

    //properties that affect Geometry
    ADD_PROPERTY_TYPE(Source ,(0),group,App::Prop_None,"3D Shape to view");
    Source.setScope(App::LinkScope::Global);
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
    ADD_PROPERTY_TYPE(SmoothVisible ,(false),sgroup,App::Prop_None,"Visible Smooth lines on/off");
    ADD_PROPERTY_TYPE(SeamVisible ,(false),sgroup,App::Prop_None,"Visible Seam lines on/off");
    ADD_PROPERTY_TYPE(IsoVisible ,(false),sgroup,App::Prop_None,"Visible Iso u,v lines on/off");
    ADD_PROPERTY_TYPE(HardHidden ,(false),sgroup,App::Prop_None,"Hidden Hard lines on/off");
    ADD_PROPERTY_TYPE(SmoothHidden ,(false),sgroup,App::Prop_None,"Hidden Smooth lines on/off");
    ADD_PROPERTY_TYPE(SeamHidden ,(false),sgroup,App::Prop_None,"Hidden Seam lines on/off");
    ADD_PROPERTY_TYPE(IsoHidden ,(false),sgroup,App::Prop_None,"Hidden Iso u,v lines on/off");
    ADD_PROPERTY_TYPE(IsoCount ,(0),sgroup,App::Prop_None,"Number of isoparameters");

    ADD_PROPERTY_TYPE(CosmeticVertexes ,(0),sgroup,App::Prop_Output,"CosmeticVertex Save/Restore");
    ADD_PROPERTY_TYPE(CosmeticEdges ,(0),sgroup,App::Prop_Output,"CosmeticEdge Save/Restore");
    ADD_PROPERTY_TYPE(CenterLines ,(0),sgroup,App::Prop_Output,"Geometry format Save/Restore");
    ADD_PROPERTY_TYPE(GeomFormats ,(0),sgroup,App::Prop_Output,"Geometry format Save/Restore");

    geometryObject = nullptr;
    getRunControl();
}

DrawViewPart::~DrawViewPart()
{
    delete geometryObject;
}

TopoDS_Shape DrawViewPart::getSourceShape(void) const
{
    TopoDS_Shape result;
    const std::vector<App::DocumentObject*>& links = Source.getValues();
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
    TopoDS_Shape result;
    const std::vector<App::DocumentObject*>& links = Source.getValues();
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


App::DocumentObjectExecReturn *DrawViewPart::execute(void)
{
//    Base::Console().Message("DVP::execute() - %s\n", Label.getValue());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    App::Document* doc = getDocument();
    bool isRestoring = doc->testStatus(App::Document::Status::Restoring);
    const std::vector<App::DocumentObject*>& links = Source.getValues();
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

//    m_saveShape = shape;
    geometryObject = makeGeometryForShape(shape);

#if MOD_TECHDRAW_HANDLE_FACES
    auto start = std::chrono::high_resolution_clock::now();
    if (handleFaces() && !geometryObject->usePolygonHLR()) {
        try {
            extractFaces();
        }
        catch (Standard_Failure& e4) {
            Base::Console().Log("LOG - DVP::execute - extractFaces failed for %s - %s **\n",getNameInDocument(),e4.GetMessageString());
            return new App::DocumentObjectExecReturn(e4.GetMessageString());
        }
    }

    addCosmeticVertexesToGeom();
    addCosmeticEdgesToGeom();
    addCenterLinesToGeom();

    auto end   = std::chrono::high_resolution_clock::now();
    auto diff  = end - start;
    double diffOut = std::chrono::duration <double, std::milli> (diff).count();
    Base::Console().Log("TIMING - %s DVP spent: %.3f millisecs handling Faces\n",
                        getNameInDocument(),diffOut);

#endif //#if MOD_TECHDRAW_HANDLE_FACES
    return DrawView::execute();
}

short DrawViewPart::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  =  (Direction.isTouched()  ||
                    Source.isTouched()  ||
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
                    CoarseView.isTouched());
    }

    if (result) {
        return result;
    }
    return TechDraw::DrawView::mustExecute();
}

void DrawViewPart::onChanged(const App::Property* prop)
{
    DrawView::onChanged(prop);
//TODO: when scale changes, any Dimensions for this View sb recalculated.  DVD should pick this up subject to topological naming issues.
}

GeometryObject* DrawViewPart::makeGeometryForShape(TopoDS_Shape shape)
{
//    Base::Console().Message("DVP::makeGeometryforShape() - %s\n", Label.getValue());
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
    m_saveShape = centeredShape;

    TopoDS_Shape scaledShape = TechDraw::scaleShape(centeredShape,
                                                    getScale());
    if (!DrawUtil::fpCompare(Rotation.getValue(),0.0)) {
        scaledShape = TechDraw::rotateShape(scaledShape,
                                            viewAxis,
                                            Rotation.getValue());
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

    auto start = std::chrono::high_resolution_clock::now();

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
    auto end   = std::chrono::high_resolution_clock::now();
    auto diff  = end - start;
    double diffOut = std::chrono::duration <double, std::milli> (diff).count();
    Base::Console().Log("TIMING - %s DVP spent: %.3f millisecs in GO::extractGeometry\n",getNameInDocument(),diffOut);

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
            Base::Console().Message("INFO - DVP::extractFaces for %s found ZeroEdge!\n",getNameInDocument());
        }
    }
    faceEdges = nonZero;
    origEdges = nonZero;

    //HLR algo does not provide all edge intersections for edge endpoints.
    //need to split long edges touched by Vertex of another edge
    std::vector<splitPoint> splits;
    std::vector<TopoDS_Edge>::iterator itOuter = origEdges.begin();
    int iOuter = 0;
    for (; itOuter != origEdges.end(); ++itOuter, iOuter++) {
        TopoDS_Vertex v1 = TopExp::FirstVertex((*itOuter));
        TopoDS_Vertex v2 = TopExp::LastVertex((*itOuter));
        Bnd_Box sOuter;
        BRepBndLib::Add(*itOuter, sOuter);
        sOuter.SetGap(0.1);
        if (sOuter.IsVoid()) {
            Base::Console().Message("DVP::Extract Faces - outer Bnd_Box is void for %s\n",getNameInDocument());
            continue;
        }
        if (DrawUtil::isZeroEdge(*itOuter)) {
            Base::Console().Message("DVP::extractFaces - outerEdge: %d is ZeroEdge\n",iOuter);   //this is not finding ZeroEdges
            continue;  //skip zero length edges. shouldn't happen ;)
        }
        int iInner = 0;
        std::vector<TopoDS_Edge>::iterator itInner = faceEdges.begin();
        for (; itInner != faceEdges.end(); ++itInner,iInner++) {
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
    std::vector<TopoDS_Edge> newEdges = DrawProjectSplit::splitEdges(faceEdges,sorted);

    if (newEdges.empty()) {
        Base::Console().Log("LOG - DVP::extractFaces - no newEdges\n");
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

    std::vector<TopoDS_Wire>::iterator itWire = sortedWires.begin();
    for (; itWire != sortedWires.end(); itWire++) {
        //version 1: 1 wire/face - no voids in face
        TechDraw::Face* f = new TechDraw::Face();
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
        if ((*it)->getTypeId().isDerivedFrom(DrawHatch::getClassTypeId())) {
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
        if ((*it)->getTypeId().isDerivedFrom(DrawGeomHatch::getClassTypeId())) {
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

const std::vector<TechDraw::Vertex *> DrawViewPart::getVertexGeometry() const
{
    std::vector<TechDraw::Vertex*> gVerts = geometryObject->getVertexGeometry();
    return gVerts;
}

const std::vector<TechDraw::Face *> & DrawViewPart::getFaceGeometry() const
{
    return geometryObject->getFaceGeometry();
}

const std::vector<TechDraw::BaseGeom  *> & DrawViewPart::getEdgeGeometry() const
{
    return geometryObject->getEdgeGeometry();
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
TechDraw::Vertex* DrawViewPart::getProjVertexByIndex(int idx) const
{
    const std::vector<TechDraw::Vertex *> &geoms = getVertexGeometry();
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

TechDraw::Vertex* DrawViewPart::getProjVertexByCosTag(std::string cosTag)
{
    TechDraw::Vertex* result = nullptr;
    std::vector<TechDraw::Vertex*> gVerts = getVertexGeometry();
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
    const std::vector<TechDraw::Face *>& faces = getFaceGeometry();
    if (idx < (int) faces.size()) {
        TechDraw::Face* projFace = faces.at(idx);
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
    const std::vector<TechDraw::Face *>& faces = getFaceGeometry();
    TechDraw::Face * ourFace = faces.at(idx);
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
    double x = getBoxX();
    double y = getBoxY();
    QRectF result;
    if (std::isinf(x) || std::isinf(y)) {
        //geometry isn't created yet.  return an arbitrary rect.
        result = QRectF(0.0,0.0,100.0,100.0);
    } else {
        result = QRectF(0.0,0.0,getBoxX(),getBoxY());  //this is from GO and is already scaled
    }
    return result;
}

//used to project a pt (ex SectionOrigin) onto paper plane
Base::Vector3d DrawViewPart::projectPoint(const Base::Vector3d& pt) const
{
    Base::Vector3d stdOrg(0.0,0.0,0.0);
    gp_Ax2 viewAxis = getProjectionCS(stdOrg);
    gp_Pnt gPt(pt.x,pt.y,pt.z);

    HLRAlgo_Projector projector( viewAxis );
    gp_Pnt2d prjPnt;
    projector.Project(gPt, prjPnt);
    Base::Vector3d result(prjPnt.X(),prjPnt.Y(), 0.0);
    result = DrawUtil::invertY(result);
    return result;
}

bool DrawViewPart::hasGeometry(void) const
{
    bool result = false;
    if (geometryObject == nullptr) {
        return result;
    }
    const std::vector<TechDraw::Vertex*> &verts = getVertexGeometry();
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
Base::Vector3d DrawViewPart::getCentroid(void) const
{
    Base::Vector3d stdOrg(0.0,0.0,0.0);
    Base::Vector3d centroid = TechDraw::findCentroidVec(m_saveShape,
                                                        getProjectionCS(stdOrg));
    return centroid;
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
            result.push_back(static_cast<TechDraw::DrawViewDetail*>(o));
        }
    }
    return result;
}

const std::vector<TechDraw::BaseGeom  *> DrawViewPart::getVisibleFaceEdges() const
{
    return geometryObject->getVisibleFaceEdges(SmoothVisible.getValue(),SeamVisible.getValue());
}

void DrawViewPart::getRunControl()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    m_sectionEdges = hGrp->GetBool("ShowSectionEdges", 0l);
    m_handleFaces = hGrp->GetBool("HandleFaces", 1l);
}

bool DrawViewPart::handleFaces(void)
{
    return m_handleFaces;
}

bool DrawViewPart::showSectionEdges(void)
{
    return m_sectionEdges;
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
        Base::Console().Warning("DVP - %s - XDirection property not set. Trying %s\n",
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
//    gp_Ax2 viewAxis = TechDraw::getViewAxis(pt,axis,flip);
    gp_Ax2 viewAxis = TechDraw::legacyViewAxis1(pt, axis, flip);
    gp_Dir gXDir = viewAxis.XDirection();
    Base::Vector3d result(gXDir.X(),
                          gXDir.Y(),
                          gXDir.Z());
    return result;
}

//********
//* Cosmetics
//********
void DrawViewPart::clearCosmeticVertexes(void)
{
    std::vector<CosmeticVertex*> noVerts;
    CosmeticVertexes.setValues(noVerts);
}

//CosmeticVertex x,y are stored as unscaled, but mirrored values.
//if you are creating a CV based on calculations of scaled geometry, you need to 
//unscale x,y before creation.
//if you are creating a CV based on calculations of mirrored geometry, you need to
//mirror again before creation.

//returns CosmeticVertex index! not geomVertexNumber!
int DrawViewPart::addCosmeticVertex(Base::Vector3d pos)
{
//    Base::Console().Message("DVP::addCosmeticVertex(%s)\n",
//                            DrawUtil::formatVector(pos).c_str());
    std::vector<CosmeticVertex*> verts = CosmeticVertexes.getValues();
    Base::Vector3d tempPos = DrawUtil::invertY(pos);
    TechDraw::CosmeticVertex* cv = new TechDraw::CosmeticVertex(tempPos);
    int newIdx = (int) (verts.size());
    verts.push_back(cv);
    CosmeticVertexes.setValues(verts);
    return newIdx;
}

std::string DrawViewPart::addCosmeticVertexSS(Base::Vector3d pos)
{
//    Base::Console().Message("DVP::addCosmeticVertexSS(%s)\n",
 //                           DrawUtil::formatVector(pos).c_str());
    std::vector<CosmeticVertex*> verts = CosmeticVertexes.getValues();
    Base::Vector3d tempPos = DrawUtil::invertY(pos);
    TechDraw::CosmeticVertex* cv = new TechDraw::CosmeticVertex(tempPos);
    verts.push_back(cv);
    CosmeticVertexes.setValues(verts);
    std::string result = cv->getTagAsString();
    return result;
}

int DrawViewPart::addCosmeticVertex(CosmeticVertex* cv)
{
    std::vector<CosmeticVertex*> verts = CosmeticVertexes.getValues();
    int newIdx = (int) verts.size();
    verts.push_back(cv);
    CosmeticVertexes.setValues(verts);
    return newIdx;
}

void DrawViewPart::removeCosmeticVertex(TechDraw::CosmeticVertex* cv)
{
//    Base::Console().Message("DVP::removeCosmeticVertex(%X)\n", cv);
    bool found = false;
    int i = 0;
    std::vector<CosmeticVertex*> verts = CosmeticVertexes.getValues();
    int stop = verts.size();
    for ( ; i < stop; i++) {
        TechDraw::CosmeticVertex* v = verts.at(i);
        if (cv == v) {
            found = true;
            break;
        }
    }
    if ( (cv != nullptr)  &&
         (found) )  {
        removeCosmeticVertex(i);
    }
}

//this is by CV index, not the index returned by selection
void DrawViewPart::removeCosmeticVertex(int idx)
{
//    Base::Console().Message("DVP::removeCV(%d)\n", idx);
    std::vector<CosmeticVertex*> verts = CosmeticVertexes.getValues();
    if (idx < (int) verts.size()) {
        verts.erase(verts.begin() + idx);
        CosmeticVertexes.setValues(verts);
        recomputeFeature();
    }
}

void DrawViewPart::removeCosmeticVertex(std::string delTag)
{
//    Base::Console().Message("DVP::removeCV(%s)\n", delTag.c_str());
    std::vector<CosmeticVertex*> cVerts = CosmeticVertexes.getValues();
    std::vector<CosmeticVertex*> newVerts;
    for (auto& cv: cVerts) {
        if (cv->getTagAsString() != delTag)  {
            newVerts.push_back(cv);
        }
    }
    CosmeticVertexes.setValues(newVerts);
//    recomputeFeature();
}

void DrawViewPart::removeCosmeticVertex(std::vector<std::string> delTags)
{
    std::vector<CosmeticVertex*> cVerts = CosmeticVertexes.getValues();
//    Base::Console().Message("DVP::removeCosmeticVertex(list) - cVerts: %d\n", cVerts.size());
    std::vector<CosmeticVertex*> newVerts;
    for (auto& cv: cVerts) {
        bool found = false;
        for (auto& dt: delTags) {
            if (cv->getTagAsString() == dt)  {
                found = true;          //this cv is in delete list
                break; 
            }
        }
        if (!found) {
            newVerts.push_back(cv);
        }
    }
    CosmeticVertexes.setValues(newVerts);
}

//transition code. temporary.  not used??
int DrawViewPart::getCosmeticVertexIndex(std::string tagString)
{
    Base::Console().Message("DVP::getCosmeticVertexIndex(%s) - deprec?\n", tagString.c_str());
    int result = -1;
    int iCV = 0;
    const std::vector<TechDraw::CosmeticVertex*> verts = CosmeticVertexes.getValues();
    for (auto& cv: verts) {
        std::string cvTag = cv->getTagAsString();
        if (cvTag == tagString) {
            result = iCV;
            break;
        }
        iCV++;
    }
    return result;
}

TechDraw::CosmeticVertex* DrawViewPart::getCosmeticVertex(std::string tagString) const
{
    CosmeticVertex* result = nullptr;
    const std::vector<TechDraw::CosmeticVertex*> verts = CosmeticVertexes.getValues();
    for (auto& cv: verts) {
        std::string cvTag = cv->getTagAsString();
        if (cvTag == tagString) {
            result = cv;
            break;
        }
    }
    return result;
}

TechDraw::CosmeticVertex* DrawViewPart::getCosmeticVertexByIndex(int idx) const
{
    CosmeticVertex* result = nullptr;
    const std::vector<TechDraw::CosmeticVertex*> verts = CosmeticVertexes.getValues();
    if (idx < (int) verts.size())  {
        result = verts.at(idx);
    }
    return result;
}

// find the cosmetic vertex corresponding to geometry vertex idx
// used when selecting
TechDraw::CosmeticVertex* DrawViewPart::getCosmeticVertexByGeom(int idx) const
{
    CosmeticVertex* result = nullptr;
    std::vector<CosmeticVertex*> verts = CosmeticVertexes.getValues();
    TechDraw::Vertex* v = getProjVertexByIndex(idx);
    if (v == nullptr) {
        return result;
    }
    if (!v->cosmeticTag.empty()) {
        result = getCosmeticVertex(v->cosmeticTag);
    }
    return result;
}

//add the cosmetic verts to geometry vertex list
void DrawViewPart::addCosmeticVertexesToGeom(void)
{
//    Base::Console().Message("DVP::addCosmeticVertexesToGeom()\n");
    int iCV = 0;
    const std::vector<TechDraw::CosmeticVertex*> cVerts = CosmeticVertexes.getValues();
    const std::vector<TechDraw::Vertex *> gVerts = getVertexGeometry();
    int stop = (int) cVerts.size();
    for ( ; iCV < stop; iCV++) {
        int iGV = geometryObject->addCosmeticVertex(cVerts.at(iCV)->scaled(getScale()),
                                                    cVerts.at(iCV)->getTagAsString(),
                                                    iCV);   //last param can be removed now? 
        cVerts.at(iCV)->linkGeom = iGV;
    }
}

int DrawViewPart::add1CVToGV(int iCV)
{
    Base::Console().Message("DVP::add1CVToGV(%d) 1 - deprec?\n", iCV);
    TechDraw::CosmeticVertex* cv = getCosmeticVertexByIndex(iCV);
    int iGV = geometryObject->addCosmeticVertex(cv->scaled(getScale()),
                                                cv->getTagAsString(),
                                                iCV);
    cv->linkGeom = iGV;
    return iGV;
}

int DrawViewPart::add1CVToGV(std::string tag)
{
//    Base::Console().Message("DVP::add1CVToGV(%s) 2\n", tag.c_str());
    TechDraw::CosmeticVertex* cv = getCosmeticVertex(tag);
    if (cv == nullptr) {
        Base::Console().Message("DVP::add1CVToGV 2 - cv %s not found\n", tag.c_str());
    }
//    int iCV = getCosmeticVertexIndex(tag);       //transition
    int iCV = -1;
    int iGV = geometryObject->addCosmeticVertex(cv->scaled(getScale()),
                                                cv->getTagAsString(),
                                                iCV);
    cv->linkGeom = iGV;
    return iGV;
}

//CosmeticEdges -------------------------------------------------------------------

//for completeness.  not actually used anywhere?
void DrawViewPart::clearCosmeticEdges(void)
{
    std::vector<CosmeticEdge*> noEdges;
    CosmeticEdges.setValues(noEdges);
}

// adds a cosmetic edge to CosmeticEdges property
// this should probably return tag instead of index
int DrawViewPart::addCosmeticEdge(Base::Vector3d p1, Base::Vector3d p2)
{
//    Base::Console().Message("DVP::addCosmeticEdge(p1,p2)\n");
    TechDraw::CosmeticEdge* ce = new TechDraw::CosmeticEdge(p1, p2);
    std::vector<CosmeticEdge*> edges = CosmeticEdges.getValues();
    int newIdx = (int) (edges.size());
    edges.push_back(ce);
    CosmeticEdges.setValues(edges);
    recomputeFeature();                 //execute needs to run to replace Geoms
    return newIdx;
}

// adds a cosmetic edge to CosmeticEdges property
// this should probably return tag instead of index
int DrawViewPart::addCosmeticEdge(TopoDS_Edge e)
{
//    Base::Console().Message("DVP::addCosmeticEdge(occ edge)\n");
    TechDraw::CosmeticEdge* ce = new TechDraw::CosmeticEdge(e);
    std::vector<CosmeticEdge*> edges = CosmeticEdges.getValues();
    int newIdx = (int) (edges.size());
    edges.push_back(ce);
    CosmeticEdges.setValues(edges);
    recomputeFeature();                 //execute needs to run to replace Geoms
    return newIdx;
}

// adds a cosmetic edge to CosmeticEdges property
// this should probably return tag instead of index
int DrawViewPart::addCosmeticEdge(CosmeticEdge* ce)
{
//    Base::Console().Message("DVP::addCosmeticEdge(%X)\n", ce);
    std::vector<CosmeticEdge*> edges = CosmeticEdges.getValues();
    int newIdx = (int) (edges.size());
    edges.push_back(ce);
    CosmeticEdges.setValues(edges);
    recomputeFeature();                 //execute needs to run to replace Geoms
    return newIdx;
}

void DrawViewPart::removeCosmeticEdge(TechDraw::CosmeticEdge* ce)
{
//    Base::Console().Message("DVP::removeCosmeticEdge(%X)\n", ce);
    if (ce != nullptr) {
        std::string ceTag = ce->getTagAsString();
        removeCosmeticEdge(ceTag);
    }
}

void DrawViewPart::removeCosmeticEdge(int idx)
{
//    Base::Console().Message("DVP::removeCosmeticEdge(%d) - deprecated. use by tag.\n", idx);
    std::vector<CosmeticEdge*> edges = CosmeticEdges.getValues();
    if (idx < (int) edges.size()) {
        edges.erase(edges.begin() + idx);
        CosmeticEdges.setValues(edges);
        recomputeFeature();                 //execute needs to run to replace Geoms
    }
}

void DrawViewPart::removeCosmeticEdge(std::string delTag)
{
//    Base::Console().Message("DVP::removeCE(%s)\n", delTag.c_str());
    std::vector<CosmeticEdge*> cEdges = CosmeticEdges.getValues();
    std::vector<CosmeticEdge*> newEdges;
    for (auto& ce: cEdges) {
        if (ce->getTagAsString() != delTag)  {
            newEdges.push_back(ce);
        }
    }
    CosmeticEdges.setValues(newEdges);
}

void DrawViewPart::removeCosmeticEdge(std::vector<std::string> delTags)
{
    std::vector<CosmeticEdge*> cEdges = CosmeticEdges.getValues();
    std::vector<CosmeticEdge*> newEdges;
    for (auto& ce: cEdges) {
        bool found = false;
        for (auto& dt: delTags) {
            if (ce->getTagAsString() == dt)  {
                found = true;          //this ce is in delete list
                break; 
            }
        }
        if (!found) {
            newEdges.push_back(ce);
        }
    }
    CosmeticEdges.setValues(newEdges);
}

TechDraw::CosmeticEdge* DrawViewPart::getCosmeticEdge(std::string tagString) const
{
//    Base::Console().Message("DVP::getCosmeticEdge(%s)\n", tagString.c_str());
    TechDraw::CosmeticEdge* result = nullptr;
    const std::vector<TechDraw::CosmeticEdge*> cEdges = CosmeticEdges.getValues();
    for (auto& ce: cEdges) {
        if (ce->getTagAsString() == tagString) {
            result = ce;
            break;
        }
    }
    return result;
}

TechDraw::CosmeticEdge* DrawViewPart::getCosmeticEdgeByIndex(int idx) const
{
//    Base::Console().Message("DVP::getCosmeticEdgeByIndex(%d)\n",idx);
    CosmeticEdge* result = nullptr;
    const std::vector<TechDraw::CosmeticEdge*> edges = CosmeticEdges.getValues();
    if (idx < (int) edges.size())  {
        result = edges.at(idx);
    }
    return result;
}

//find the cosmetic edge corresponding to geometry edge idx
TechDraw::CosmeticEdge* DrawViewPart::getCosmeticEdgeByGeom(int idx) const
{
//    Base::Console().Message("DVP::getCosmeticEdgeByGeom(%d)\n",idx);
    CosmeticEdge* result = nullptr;
    BaseGeom* geom = getGeomByIndex(idx);
    if (geom == nullptr) {
        return result;
    }
    if (!geom->getCosmeticTag().empty()) {
        result = getCosmeticEdge(geom->getCosmeticTag());
    }
    return result;
}

//find the index of a cosmetic edge
int DrawViewPart::getCosmeticEdgeIndex(TechDraw::CosmeticEdge* ce) const
{
//    Base::Console().Message("DVP::getCosmeticEdgeIndex(%X) - deprec?\n",ce);
    int result = -1;
    const std::vector<TechDraw::CosmeticEdge*> edges = CosmeticEdges.getValues();
    int i = 0;
    int stop = (int) edges.size();
    for (; i < stop; i++) {
        if (edges.at(i) == ce) {
            result = i;
            break;
        }
    }
    return result;
}

//add the cosmetic edges to geometryObject edges list
void DrawViewPart::addCosmeticEdgesToGeom(void)
{
    const std::vector<TechDraw::CosmeticEdge*> edges = CosmeticEdges.getValues();
    for (auto& ce: edges) {
        TechDraw::BaseGeom* scaledGeom = ce->scaledGeometry(getScale());
        if (scaledGeom == nullptr) {
            Base::Console().Error("DVP::addCosmeticEdgesToGeom - scaledGeometry is null\n");
            continue;
        }
//        int idx =
        (void) geometryObject->addCosmeticEdge(scaledGeom, 1);
    }
}

// CenterLines -----------------------------------------------------------------
void DrawViewPart::clearCenterLines(void)
{
    std::vector<CenterLine*> noLines;
    CenterLines.setValues(noLines);
}

int DrawViewPart::addCenterLine(CenterLine* cl)
{
//    Base::Console().Message("DVP::addCL(cl)\n");
    std::vector<CenterLine*> lines = CenterLines.getValues();
    int newIdx = (int) lines.size();
    lines.push_back(cl);
    CenterLines.setValues(lines);
    return newIdx;
}

void DrawViewPart::removeCenterLine(TechDraw::CenterLine* cl)
{
    bool found = false;
    int i = 0;
    std::vector<CenterLine*> lines = CenterLines.getValues();
    int stop = lines.size();
    for ( ; i < stop; i++) {
        TechDraw::CenterLine* l = lines.at(i);
        if (cl == l) {
            found = true;
            break;
        }
    }
    if ( (cl != nullptr)  &&
         (found) )  {
        removeCenterLine(i);
    }
}

void DrawViewPart::removeCenterLine(int idx)
{
    std::vector<CenterLine*> lines = CenterLines.getValues();
    if (idx < (int) lines.size()) {
        lines.erase(lines.begin() + idx);
        CenterLines.setValues(lines);
        recomputeFeature();
    }
}

void DrawViewPart::removeCenterLine(std::string delTag)
{
//    Base::Console().Message("DVP::removeCL(%s)\n", delTag.c_str());
    std::vector<CenterLine*> cLines = CenterLines.getValues();
    std::vector<CenterLine*> newLines;
    for (auto& cl: cLines) {
        if (cl->getTagAsString() != delTag)  {
            newLines.push_back(cl);
        }
    }
    CenterLines.setValues(newLines);
}

void DrawViewPart::removeCenterLine(std::vector<std::string> delTags)
{
    std::vector<CenterLine*> cLines = CenterLines.getValues();
    std::vector<CenterLine*> newLines;
    for (auto& cl: cLines) {
        bool found = false;
        for (auto& dt: delTags) {
            if (cl->getTagAsString() == dt)  {
                found = true;          //this cl is in delete list
                break; 
            }
        }
        if (!found) {
            newLines.push_back(cl);
        }
    }
    CenterLines.setValues(newLines);
}

void DrawViewPart::replaceCenterLine(int idx, TechDraw::CenterLine* cl)
{
    std::vector<CenterLine*> lines = CenterLines.getValues();
    if (idx < (int) lines.size())  {
        lines.at(idx) = cl;
        recomputeFeature();
    }
}

void DrawViewPart::replaceCenterLineByGeom(int geomIndex, TechDraw::CenterLine* cl)
{
    const std::vector<TechDraw::BaseGeom*> &geoms = getEdgeGeometry();
    int sourceIndex = geoms.at(geomIndex)->sourceIndex();
    replaceCenterLine(sourceIndex, cl);
}

TechDraw::CenterLine* DrawViewPart::getCenterLineByIndex(int idx) const
{
    CenterLine* result = nullptr;
    const std::vector<TechDraw::CenterLine*> lines = CenterLines.getValues();
    if (idx < (int) lines.size())  {
        result = lines.at(idx);
    }
    return result;
}

//find the cosmetic edge corresponding to geometry edge idx
TechDraw::CenterLine* DrawViewPart::getCenterLineByGeom(int idx) const
{
    const std::vector<TechDraw::BaseGeom*> &geoms = getEdgeGeometry();
    int sourceIndex = geoms.at(idx)->sourceIndex();
    CenterLine* result = nullptr;
    const std::vector<TechDraw::CenterLine*> lines = CenterLines.getValues();
    result = lines.at(sourceIndex);
    return result;
}

//add the center lines to geometry Edges list
void DrawViewPart::addCenterLinesToGeom(void)
{
//   Base::Console().Message("DVP::addCenterLinesToGeom()\n");
   int i = 0;
    const std::vector<TechDraw::CenterLine*> lines = CenterLines.getValues();
    int stop = (int) lines.size();
    for ( ; i < stop; i++) {
        TechDraw::BaseGeom* scaledGeom = lines.at(i)->scaledGeometry(this);
        if (scaledGeom == nullptr) {
            Base::Console().Error("DVP::addCenterLinesToGeom - scaledGeometry is null\n");
            continue;
        }
//        int idx =
        (void) geometryObject->addCenterLine(scaledGeom, 2, i);
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

int DrawViewPart::addGeomFormat(GeomFormat* gf)
{
    std::vector<GeomFormat*> fmts = GeomFormats.getValues();
    int newIdx = (int) fmts.size();
    fmts.push_back(gf);
    GeomFormats.setValues(fmts);
    return newIdx;
}

void DrawViewPart::removeGeomFormat(int idx)
{
    std::vector<GeomFormat*> fmts = GeomFormats.getValues();
    if (idx < (int) fmts.size()) {
        GeomFormat* toRemove = fmts[idx];
        fmts.erase(fmts.begin() + idx);
        GeomFormats.setValues(fmts);
        delete toRemove;
        requestPaint();
    }
}

TechDraw::GeomFormat* DrawViewPart::getGeomFormatByIndex(int idx) const
{
    GeomFormat* result = nullptr;
    const std::vector<TechDraw::GeomFormat*> fmts = GeomFormats.getValues();
    if (idx < (int) fmts.size())  {
        result = fmts.at(idx);
    }
    return result;
}

//find the format corresponding to geometry edge idx
TechDraw::GeomFormat* DrawViewPart::getGeomFormatByGeom(int idx) const
{
    GeomFormat* result = nullptr;
    const std::vector<TechDraw::GeomFormat*> fmts = GeomFormats.getValues();
    for (auto& f: fmts) {
        if (f->m_geomIndex == idx) {
            result = f;
            break;
        }
    }
    return result;
}

//------------------------------------------------------------------------------

void DrawViewPart::dumpVerts(std::string text)
{
    if (geometryObject == nullptr) {
        Base::Console().Message("no verts to dump yet\n");
        return;
    }
    std::vector<TechDraw::Vertex *> gVerts = getVertexGeometry();
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
