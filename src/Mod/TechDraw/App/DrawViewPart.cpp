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

#include <BRep_Tool.hxx>
#include <BRepGProp.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepLib.hxx>
#include <BRepLProp_CurveTool.hxx>
#include <BRepLProp_CLProps.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
# include <BRep_Builder.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Geom_Curve.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GProp_GProps.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_XYZ.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_ShapeBounds.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <ShapeFix_ShapeTolerance.hxx>
#include <ShapeExtend_WireData.hxx>
#include <ShapeFix_Wire.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>

#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <GeomLib_Tool.hxx>

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

#include "DrawUtil.h"
#include "DrawViewSection.h"
#include "DrawProjectSplit.h"
#include "Geometry.h"
#include "GeometryObject.h"
#include "DrawViewPart.h"
#include "DrawHatch.h"
#include "DrawGeomHatch.h"
#include "DrawViewDimension.h"
#include "DrawViewBalloon.h"
#include "DrawViewDetail.h"
#include "DrawPage.h"
#include "EdgeWalker.h"
#include "LineGroup.h"
#include "Cosmetic.h"

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
                      group,App::Prop_None,"Projection direction. The direction you are looking from.");
    ADD_PROPERTY_TYPE(Perspective ,(false),group,App::Prop_None,"Perspective(true) or Orthographic(false) projection");
    ADD_PROPERTY_TYPE(Focus,(defDist),group,App::Prop_None,"Perspective view focus distance");

    //properties that control HLR algoaffect Appearance
    bool coarseView = hGrp->GetBool("CoarseView", false);
    ADD_PROPERTY_TYPE(CoarseView, (coarseView), sgroup, App::Prop_None, "Coarse View on/off");
    //visible outline
    ADD_PROPERTY_TYPE(SmoothVisible ,(false),sgroup,App::Prop_None,"Visible Smooth lines on/off");
    ADD_PROPERTY_TYPE(SeamVisible ,(false),sgroup,App::Prop_None,"Visible Seam lines on/off");
    ADD_PROPERTY_TYPE(IsoVisible ,(false),sgroup,App::Prop_None,"Visible Iso u,v lines on/off");
    ADD_PROPERTY_TYPE(HardHidden ,(false),sgroup,App::Prop_None,"Hidden Hard lines on/off");
    ADD_PROPERTY_TYPE(SmoothHidden ,(false),sgroup,App::Prop_None,"Hidden Smooth lines on/off");
    ADD_PROPERTY_TYPE(SeamHidden ,(false),sgroup,App::Prop_None,"Hidden Seam lines on/off");
    ADD_PROPERTY_TYPE(IsoHidden ,(false),sgroup,App::Prop_None,"Hidden Iso u,v lines on/off");
    ADD_PROPERTY_TYPE(IsoCount ,(0),sgroup,App::Prop_None,"Number of isoparameters");

    ADD_PROPERTY_TYPE(CosmeticVertexList ,(""),sgroup,App::Prop_None,"CosmeticVertex Save/Restore");
    ADD_PROPERTY_TYPE(CosmeticEdgeList ,(""),sgroup,App::Prop_None,"CosmeticEdge Save/Restore");

    geometryObject = nullptr;
    getRunControl();
}

DrawViewPart::~DrawViewPart()
{
    delete geometryObject;
}

TopoDS_Shape DrawViewPart::getSourceShape(void) const
{
//     Base::Console().Message("DVP::getSourceShape() - %s\n", getNameInDocument());
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
        std::vector<TopoDS_Shape> sourceShapes;
        for (auto& l:links) {
            std::vector<TopoDS_Shape> shapeList = getShapesFromObject(l);
            sourceShapes.insert(sourceShapes.end(),shapeList.begin(),shapeList.end());
        }

        BRep_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);
        bool found = false;
        for (auto& s:sourceShapes) {
            if (s.IsNull()) {
                continue;    //has no shape
            }
            found = true;
            BRepBuilderAPI_Copy BuilderCopy(s);
            TopoDS_Shape shape = BuilderCopy.Shape();
            builder.Add(comp, shape);
        }
        //it appears that an empty compound is !IsNull(), so we need to check a different way 
        //if we added anything to the compound.
        if (!found) {
            Base::Console().Error("DVP::getSourceShapes - source shape is empty!\n");
        } else {
            result = comp;
        }
    }
    return result;
}

std::vector<TopoDS_Shape> DrawViewPart::getShapesFromObject(App::DocumentObject* docObj) const
{
//    Base::Console().Message("DVP::getShapesFromObject() - %s\n", getNameInDocument());
    std::vector<TopoDS_Shape> result;
    App::GroupExtension* gex = dynamic_cast<App::GroupExtension*>(docObj);
    App::Property* gProp = docObj->getPropertyByName("Group");
    App::Property* sProp = docObj->getPropertyByName("Shape");
    if (docObj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        Part::Feature* pf = static_cast<Part::Feature*>(docObj);
        Part::TopoShape ts = pf->Shape.getShape();
        ts.setPlacement(pf->globalPlacement());
        result.push_back(ts.getShape());
    } else if (gex != nullptr) {           //is a group extension
        std::vector<App::DocumentObject*> objs = gex->Group.getValues();
        std::vector<TopoDS_Shape> shapes;
        for (auto& d: objs) {
            shapes = getShapesFromObject(d);
            if (!shapes.empty()) {
                result.insert(result.end(),shapes.begin(),shapes.end());
            }
        }
    //the next 2 bits are mostly for Arch module objects
    } else if (gProp != nullptr) {       //has a Group property
        App::PropertyLinkList* list = dynamic_cast<App::PropertyLinkList*>(gProp);
        if (list != nullptr) {
            std::vector<App::DocumentObject*> objs = list->getValues();
            std::vector<TopoDS_Shape> shapes;
            for (auto& d: objs) {
                shapes = getShapesFromObject(d);
                if (!shapes.empty()) {
                    result.insert(result.end(),shapes.begin(),shapes.end());
                }
            }
        } else {
                Base::Console().Log("DVP::getShapesFromObject - Group is not a PropertyLinkList!\n");
        }
    } else if (sProp != nullptr) {       //has a Shape property
        Part::PropertyPartShape* shape = dynamic_cast<Part::PropertyPartShape*>(sProp);
        if (shape != nullptr) {
            TopoDS_Shape occShape = shape->getValue();
            result.push_back(occShape);
        } else {
            Base::Console().Log("DVP::getShapesFromObject - Shape is not a PropertyPartShape!\n");
        }
    }
    return result;
}

TopoDS_Shape DrawViewPart::getSourceShapeFused(void) const
{
//     Base::Console().Message("DVP::getSourceShapeFused() - %s\n", getNameInDocument());
    TopoDS_Shape baseShape = getSourceShape();
    if (!baseShape.IsNull()) {
        TopoDS_Iterator it(baseShape);
        TopoDS_Shape fusedShape = it.Value();
        it.Next();
        for (; it.More(); it.Next()) {
            const TopoDS_Shape& aChild = it.Value();
            BRepAlgoAPI_Fuse mkFuse(fusedShape, aChild);
            // Let's check if the fusion has been successful
            if (!mkFuse.IsDone()) {
                Base::Console().Error("DVp - Fusion failed - %s\n",getNameInDocument());
                return baseShape;
            }
            fusedShape = mkFuse.Shape();
        }
        baseShape = fusedShape;
    }
    return baseShape;
}

App::DocumentObjectExecReturn *DrawViewPart::execute(void)
{
//    Base::Console().Message("DVP::execute() - %s\n",getNameInDocument());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

//    rebuildCosmoVertex();
//    rebuildCosmoEdge();

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

    TopoDS_Shape shape = getSourceShape();          //if shape is null, it is probably(?) obj creation time.
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

    gp_Pnt inputCenter;
    Base::Vector3d stdOrg(0.0,0.0,0.0);
    
    inputCenter = TechDraw::findCentroid(shape,
                                                 getViewAxis(stdOrg,Direction.getValue()));
                                                 
    shapeCentroid = Base::Vector3d(inputCenter.X(),inputCenter.Y(),inputCenter.Z());
    TopoDS_Shape mirroredShape;
    mirroredShape = TechDraw::mirrorShape(shape,
                                                  inputCenter,
                                                  getScale());

    gp_Ax2 viewAxis = getViewAxis(shapeCentroid,Direction.getValue());
    if (!DrawUtil::fpCompare(Rotation.getValue(),0.0)) {
        mirroredShape = TechDraw::rotateShape(mirroredShape,
                                                      viewAxis,
                                                      Rotation.getValue());
     }
    geometryObject =  buildGeometryObject(mirroredShape,viewAxis);

#if MOD_TECHDRAW_HANDLE_FACES
    auto start = chrono::high_resolution_clock::now();
    if (handleFaces() && !geometryObject->usePolygonHLR()) {
        try {
            extractFaces();
        }
        catch (Standard_Failure& e4) {
            Base::Console().Log("LOG - DVP::execute - extractFaces failed for %s - %s **\n",getNameInDocument(),e4.GetMessageString());
            return new App::DocumentObjectExecReturn(e4.GetMessageString());
        }
    }

    //add back the cosmetic vertices
    for (auto& v: vertexCosmetic) {
        int idx = geometryObject->addCosmeticVertex(v->point() * getScale());
        v->linkGeom = idx;
    }

    //add the cosmetic Edges to geometry Edges list
    for (auto& e: edgeCosmetic) {
        TechDraw::BaseGeom* scaledGeom = e->scaledGeometry(getScale());
        int idx = geometryObject->addCosmeticEdge(scaledGeom);
        e->m_linkGeom = idx;
    }

    auto end   = chrono::high_resolution_clock::now();
    auto diff  = end - start;
    double diffOut = chrono::duration <double, milli> (diff).count();
    Base::Console().Log("TIMING - %s DVP spent: %.3f millisecs handling Faces\n",
                        getNameInDocument(),diffOut);

#endif //#if MOD_TECHDRAW_HANDLE_FACES

    requestPaint();
    return App::DocumentObject::StdReturn;
}

short DrawViewPart::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  =  (Direction.isTouched()  ||
                    Source.isTouched()  ||
                    Scale.isTouched() ||
                    ScaleType.isTouched() ||
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

//note: slightly different than routine with same name in DrawProjectSplit
TechDraw::GeometryObject* DrawViewPart::buildGeometryObject(TopoDS_Shape shape, gp_Ax2 viewAxis)
{
//    Base::Console().Message("DVP::buildGO() - %s\n", getNameInDocument());
    TechDraw::GeometryObject* go = new TechDraw::GeometryObject(getNameInDocument(), this);
    go->setIsoCount(IsoCount.getValue());
    go->isPerspective(Perspective.getValue());
    go->setFocus(Focus.getValue());
    go->usePolygonHLR(CoarseView.getValue());

    Base::Vector3d baseProjDir = Direction.getValue();
    saveParamSpace(baseProjDir);

    if (go->usePolygonHLR()){
        go->projectShapeWithPolygonAlgo(shape,
            viewAxis);
    }
    else{
        go->projectShape(shape,
            viewAxis);
    }

    auto start = chrono::high_resolution_clock::now();

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
    auto end   = chrono::high_resolution_clock::now();
    auto diff  = end - start;
    double diffOut = chrono::duration <double, milli> (diff).count();
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

const std::vector<TechDraw::Vertex *> & DrawViewPart::getVertexGeometry() const
{
    return geometryObject->getVertexGeometry();
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
TechDraw::BaseGeom* DrawViewPart::getProjEdgeByIndex(int idx) const
{
    const std::vector<TechDraw::BaseGeom *> &geoms = getEdgeGeometry();
    if (geoms.empty()) {
        Base::Console().Log("INFO - getProjEdgeByIndex(%d) - no Edge Geometry. Probably restoring?\n",idx);
        return NULL;
    }
    if ((unsigned)idx >= geoms.size()) {
        Base::Console().Log("INFO - getProjEdgeByIndex(%d) - invalid index\n",idx);
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
                   Base::Console().Message("DVP::getFaceEdgesByIndex - found cosmetic edge\n");
                }
                result.push_back(g);
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
//            DrawUtil::dumpEdge("DVP Face edge",i,g->occEdge);
            i++;
        }
        TopoDS_Wire occwire = EdgeWalker::makeCleanWire(edges);
//        BRepLib::BuildCurves3d(occwire);   //probably don't need this
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

//used to project pt (ex SectionOrigin) onto paper plane
Base::Vector3d DrawViewPart::projectPoint(const Base::Vector3d& pt) const
{
    gp_Trsf mirrorTransform;
    mirrorTransform.SetMirror( gp_Ax2(gp_Pnt(shapeCentroid.x,shapeCentroid.y,shapeCentroid.z),
                                      gp_Dir(0, -1, 0)) );
    gp_Pnt basePt(pt.x,pt.y,pt.z);
    gp_Pnt mirrorGp = basePt.Transformed(mirrorTransform);
    Base::Vector3d mirrorPt(mirrorGp.X(),mirrorGp.Y(), mirrorGp.Z());
    Base::Vector3d centeredPoint = mirrorPt - shapeCentroid;
    Base::Vector3d direction = Direction.getValue();
    gp_Ax2 viewAxis = getViewAxis(centeredPoint,direction);
    HLRAlgo_Projector projector( viewAxis );
    gp_Pnt2d prjPnt;
    projector.Project(gp_Pnt(centeredPoint.x,centeredPoint.y,centeredPoint.z), prjPnt);
    return Base::Vector3d(prjPnt.X(),prjPnt.Y(), 0.0);
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

//boring here. gets more interesting in descendents.
gp_Ax2 DrawViewPart::getViewAxis(const Base::Vector3d& pt,
                                 const Base::Vector3d& axis,
                                 const bool flip)  const
{
    gp_Ax2 viewAxis = TechDraw::getViewAxis(pt,axis,flip);
     
    return viewAxis;
}

void DrawViewPart::saveParamSpace(const Base::Vector3d& direction, const Base::Vector3d& xAxis)
{
    (void)xAxis;
    Base::Vector3d origin(0.0,0.0,0.0);
    gp_Ax2 viewAxis = getViewAxis(origin,direction);

    gp_Dir xdir = viewAxis.XDirection();
    uDir = Base::Vector3d(xdir.X(),xdir.Y(),xdir.Z());
    gp_Dir ydir = viewAxis.YDirection();
    vDir = Base::Vector3d(ydir.X(),ydir.Y(),ydir.Z());
    wDir = Base::Vector3d(direction.x, -direction.y, direction.z);
    wDir.Normalize();
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

//is this really the projection plane??
gp_Pln DrawViewPart::getProjPlane() const
{
    Base::Vector3d plnPnt(0.0,0.0,0.0);
    Base::Vector3d plnNorm = Direction.getValue();
    gp_Ax2 viewAxis = getViewAxis(plnPnt,plnNorm,false);
    gp_Ax3 viewAxis3(viewAxis);

    return gp_Pln(viewAxis3);
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
//    Base::Console().Message("DVP::unsetupObject()\n");
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

//********
//* Cosmetics
//********

//build vertexCosmetic from CosmeticVertexList
void DrawViewPart::rebuildCosmoVertex(void)
{
//    Base::Console().Message("DVP::rebuildCosmoVertex()\n");
    vertexCosmetic.clear();
    std::vector<std::string> restoreVerts = CosmeticVertexList.getValues();
    for (auto& rv: restoreVerts) {
       if (!rv.empty()) {
           CosmeticVertex* cv = new CosmeticVertex();
           bool rc = cv->fromCSV(rv);
           if (rc) {
               vertexCosmetic.push_back(cv);
           } else {
               delete cv;
           }
       }
    }
}

//build edgeCosmetic from CosmeticEdgeList
void DrawViewPart::rebuildCosmoEdge(void)
{
//    Base::Console().Message("DVP::rebuildCosmoEdge()\n");
    edgeCosmetic.clear();
    std::vector<std::string> restoreEdges = CosmeticEdgeList.getValues();
    for (auto& re: restoreEdges) {
       if (!re.empty()) {
           CosmeticEdge* ce = new CosmeticEdge();
           bool rc = ce->fromCSV(re);
           if (rc) {
               edgeCosmetic.push_back(ce);
           } else {
               delete ce;
           }
       }
    }
}

// adds a cosmetic vertex to vertexCosmetic and CosmeticVertexList
int DrawViewPart::addCosmeticVertex(Base::Vector3d pos)
{
//    Base::Console().Message("DVP::addCosmeticVertex(%s)\n", DrawUtil::formatVector(pos).c_str());
    TechDraw::CosmeticVertex* cv = new TechDraw::CosmeticVertex(pos);
    vertexCosmetic.push_back(cv);
    int newIdx = (int) (vertexCosmetic.size() - 1);
    std::string csv = cv->toCSV();
    std::vector<std::string> vertexList = CosmeticVertexList.getValues();
    vertexList.push_back(csv);
    CosmeticVertexList.setValues(vertexList);

    return newIdx;
}

void DrawViewPart::stuffCosmeticVertexList(void)
{
//    Base::Console().Message("DVP::stuffCosmeticVertexList()\n");
    std::vector<std::string> saveVerts;
    const std::vector<TechDraw::CosmeticVertex*> cosVerts = getCosmeticVertex();
    for (auto& cv: cosVerts) {
        std::string csv = cv->toCSV();
        saveVerts.push_back(csv);
    }
    CosmeticVertexList.setValues(saveVerts);
}

void DrawViewPart::removeCosmeticVertex(TechDraw::CosmeticVertex* cv)
{
//    Base::Console().Message("DVP::removeCosmeticVertex(cv) - cvs in: %d\n", vertexCosmetic.size());
    bool found = false;
    std::vector<TechDraw::CosmeticVertex*> newCosmoVertex;
    for (auto& v: vertexCosmetic) {
        if (cv == v) {
            found = true;
            continue;
        } else {
            newCosmoVertex.push_back(v);
        }
    }
    if ( (cv != nullptr)  &&
         (found) )  {
        delete cv;
    }
     
    vertexCosmetic = newCosmoVertex;
    stuffCosmeticVertexList();
    recomputeFeature();
}

void DrawViewPart::removeCosmeticVertex(int idx)
{
    if (idx < (int) vertexCosmetic.size()) {
        TechDraw::CosmeticVertex* cvSave = vertexCosmetic.at(idx);
        vertexCosmetic.erase(vertexCosmetic.begin() + idx);
        delete cvSave;
    }
     
    stuffCosmeticVertexList();
    recomputeFeature();
}

TechDraw::CosmeticVertex* DrawViewPart::getCosmeticVertexByIndex(int idx) const
{
//    Base::Console().Message("DVP::getCosmeticVertexByIndex(%d)\n", idx);
    CosmeticVertex* result = nullptr;
    const std::vector<TechDraw::CosmeticVertex*> verts = getCosmeticVertex();
    if ((unsigned) idx < verts.size())  {
        result = verts.at(idx);
    }
    return result;
}

//find the cosmetic vertex corresponding to geometry vertex idx
TechDraw::CosmeticVertex* DrawViewPart::getCosmeticVertexByLink(int idx) const
{
//    Base::Console().Message("DVP::getVosmeticVertexByLinkIndex(%d)\n", idx);
    CosmeticVertex* result = nullptr;
    const std::vector<TechDraw::CosmeticVertex*> verts = getCosmeticVertex();
    for (auto& cv: verts) {
        if (cv->linkGeom == idx) {
            result = cv;
            break;
        }
    }
    return result;
}

void DrawViewPart::clearCosmeticVertices(void) 
{
//    Base::Console().Message("DVP::clearCosmeticVertices()\n");
    vertexCosmetic.clear();
    std::vector<std::string> noVerts;
    CosmeticVertexList.setValues(noVerts);
}

void DrawViewPart::clearCosmeticEdges(void) 
{
//    Base::Console().Message("DVP::clearCosmeticEdges()\n");
    edgeCosmetic.clear();
    std::vector<std::string> noEdges;
    CosmeticEdgeList.setValues(noEdges);
}

// adds a cosmetic edge to edgeCosmetic and CosmeticEdgeList
int DrawViewPart::addCosmeticEdge(Base::Vector3d p1, Base::Vector3d p2)
{
//    Base::Console().Message("DVP::addCosmeticEdge(%s, %s)\n",
//                              DrawUtil::formatVector(p1).c_str(),
//                              DrawUtil::formatVector(p2).c_str());
    TechDraw::CosmeticEdge* ce = new TechDraw::CosmeticEdge(p1, p2);
    edgeCosmetic.push_back(ce);
    int newIdx = (int) (edgeCosmetic.size() - 1);
//    stuffCosmeticEdgeList();
    std::string csv = ce->toCSV();
    std::vector<std::string> edgeList = CosmeticEdgeList.getValues();
    edgeList.push_back(csv);
    CosmeticEdgeList.setValues(edgeList);
    return newIdx;
}

int DrawViewPart::addCosmeticEdge(TopoDS_Edge e)
{
//    Base::Console().Message("DVP::addCosmeticEdge(TopoDS_Edge)\n");
    TechDraw::CosmeticEdge* ce = new TechDraw::CosmeticEdge(e);
    edgeCosmetic.push_back(ce);
    int newIdx = (int) (edgeCosmetic.size() - 1);
//    stuffCosmeticEdgeList();
    std::string csv = ce->toCSV();
    std::vector<std::string> edgeList = CosmeticEdgeList.getValues();
    edgeList.push_back(csv);
    CosmeticEdgeList.setValues(edgeList);
    return newIdx;
}

int DrawViewPart::addCosmeticEdge(CosmeticEdge* ce)
{
//    Base::Console().Message("DVP::addCosmeticEdge(CosmeticEdge)\n");
    edgeCosmetic.push_back(ce);
    int newIdx = (int) (edgeCosmetic.size() - 1);
//    stuffCosmeticEdgeList();
    std::string csv = ce->toCSV();
    std::vector<std::string> edgeList = CosmeticEdgeList.getValues();
    edgeList.push_back(csv);
    CosmeticEdgeList.setValues(edgeList);
    return newIdx;
}

void DrawViewPart::stuffCosmeticEdgeList(void)
{
//    Base::Console().Message("DVP::stuffCosmeticEdgeList()\n");
    std::vector<std::string> saveEdges;
    const std::vector<TechDraw::CosmeticEdge*> cosEdges = getCosmeticEdge();
    for (auto& ce: cosEdges) {
        std::string csv = ce->toCSV();
        saveEdges.push_back(csv);
    }
    CosmeticEdgeList.setValues(saveEdges);
}

void DrawViewPart::removeCosmeticEdge(TechDraw::CosmeticEdge* ce)
{
//    Base::Console().Message("DVP::removeCosmeticEdge(ce) - ces in: %d\n", edgeCosmetic.size());
    bool found = false;
    std::vector<TechDraw::CosmeticEdge*> newCosmoEdge;
    for (auto& e: edgeCosmetic) {
        if (ce == e) {
            found = true;
            continue;
        } else {
            newCosmoEdge.push_back(e);
        }
    }
    if ( (ce != nullptr)  &&
         (found) )  {
        delete ce;
    }
    edgeCosmetic = newCosmoEdge;
    stuffCosmeticEdgeList();
    recomputeFeature();
}

void DrawViewPart::removeCosmeticEdge(int idx)
{
//    Base::Console().Message("DVP::removeCosmeticEdge(%d) - ces in: %d\n", idx, edgeCosmetic.size());
    if (idx < (int) edgeCosmetic.size()) {
        TechDraw::CosmeticEdge* ceSave = edgeCosmetic.at(idx);
        edgeCosmetic.erase(edgeCosmetic.begin() + idx);
        delete ceSave;
    }
    stuffCosmeticEdgeList();
    recomputeFeature();
}

TechDraw::CosmeticEdge* DrawViewPart::getCosmeticEdgeByIndex(int idx) const
{
//    Base::Console().Message("DVP::getCosmeticEdgeByIndex(%d)\n", idx);
    CosmeticEdge* result = nullptr;
    const std::vector<TechDraw::CosmeticEdge*> edges = getCosmeticEdge();
    if (idx < (int) edges.size())  {
        result = edges.at(idx);
    }
    return result;
}

//find the cosmetic edge corresponding to geometry edge idx
TechDraw::CosmeticEdge* DrawViewPart::getCosmeticEdgeByLink(int idx) const
{
//    Base::Console().Message("DVP::getCosmeticEdgeByLinkIndex(%d)\n", idx);
    CosmeticEdge* result = nullptr;
    const std::vector<TechDraw::CosmeticEdge*> edges = getCosmeticEdge();
    for (auto& ce: edges) {
        if (ce->m_linkGeom == idx) {
            result = ce;
            break;
        }
    }
    return result;
}

void DrawViewPart::onDocumentRestored()
{
    rebuildCosmoVertex();
    rebuildCosmoEdge();
//    requestPaint();
    //if execute has not run yet, there will be no GO, and paint will not do anything.
    execute();
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
