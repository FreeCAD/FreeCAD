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
#include <BRepLib.hxx>
#include <BRepLProp_CurveTool.hxx>
#include <BRepLProp_CLProps.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
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
#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>
#include <Mod/Part/App/PartFeature.h>

#include "DrawUtil.h"
#include "DrawViewSection.h"
#include "DrawProjectSplit.h"
#include "Geometry.h"
#include "GeometryObject.h"
#include "DrawViewPart.h"
#include "DrawHatch.h"
#include "DrawGeomHatch.h"
#include "DrawViewDimension.h"
#include "DrawPage.h"
#include "EdgeWalker.h"


#include <Mod/TechDraw/App/DrawViewPartPy.h>  // generated from DrawViewPartPy.xml

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewPart
//===========================================================================


PROPERTY_SOURCE(TechDraw::DrawViewPart, TechDraw::DrawView)

DrawViewPart::DrawViewPart(void) : geometryObject(0)
{
    static const char *group = "Projection";
    static const char *fgroup = "Format";
    static const char *sgroup = "Show";
    nowDeleting = false;

    //properties that affect Geometry
    ADD_PROPERTY_TYPE(Source ,(0),group,App::Prop_None,"3D Shape to view");
    ADD_PROPERTY_TYPE(Direction ,(0,0,1.0)    ,group,App::Prop_None,"Projection direction. The direction you are looking from.");

    //properties that affect Appearance
    //visible outline
    ADD_PROPERTY_TYPE(SmoothVisible ,(false),sgroup,App::Prop_None,"Visible Smooth lines on/off");
    ADD_PROPERTY_TYPE(SeamVisible ,(false),sgroup,App::Prop_None,"Visible Seam lines on/off");
    ADD_PROPERTY_TYPE(IsoVisible ,(false),sgroup,App::Prop_None,"Visible Iso u,v lines on/off");
    ADD_PROPERTY_TYPE(HardHidden ,(false),sgroup,App::Prop_None,"Hidden Hard lines on/off");
    ADD_PROPERTY_TYPE(SmoothHidden ,(false),sgroup,App::Prop_None,"Hidden Smooth lines on/off");
    ADD_PROPERTY_TYPE(SeamHidden ,(false),sgroup,App::Prop_None,"Hidden Seam lines on/off");
    ADD_PROPERTY_TYPE(IsoHidden ,(false),sgroup,App::Prop_None,"Hidden Iso u,v lines on/off");
    ADD_PROPERTY_TYPE(IsoCount ,(0),sgroup,App::Prop_None,"Number of isoparameters");

    ADD_PROPERTY_TYPE(LineWidth,(0.7f),fgroup,App::Prop_None,"The thickness of visible lines");
    ADD_PROPERTY_TYPE(HiddenWidth,(0.15),fgroup,App::Prop_None,"The thickness of hidden lines, if enabled");
    ADD_PROPERTY_TYPE(IsoWidth,(0.30),fgroup,App::Prop_None,"The thickness of UV isoparameter lines, if enabled");
    ADD_PROPERTY_TYPE(ArcCenterMarks ,(true),sgroup,App::Prop_None,"Center marks on/off");
    ADD_PROPERTY_TYPE(CenterScale,(2.0),fgroup,App::Prop_None,"Center mark size adjustment, if enabled");
    ADD_PROPERTY_TYPE(HorizCenterLine ,(false),sgroup,App::Prop_None,"Show a horizontal centerline through view");
    ADD_PROPERTY_TYPE(VertCenterLine ,(false),sgroup,App::Prop_None,"Show a vertical centerline through view");

    //properties that affect Section Line
    ADD_PROPERTY_TYPE(ShowSectionLine ,(true)    ,sgroup,App::Prop_None,"Show/hide section line if applicable");

    geometryObject = nullptr;
    getRunControl();

}

DrawViewPart::~DrawViewPart()
{
    delete geometryObject;
}


App::DocumentObjectExecReturn *DrawViewPart::execute(void)
{
    App::DocumentObject *link = Source.getValue();
    if (!link) {
        return new App::DocumentObjectExecReturn("DVP - No Source object linked");
    }

    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        return new App::DocumentObjectExecReturn("DVP - Linked object is not a Part object");
    }

    TopoDS_Shape shape = static_cast<Part::Feature*>(link)->Shape.getShape().getShape();
    if (shape.IsNull()) {
        return new App::DocumentObjectExecReturn("DVP - Linked shape object is empty");
    }

    (void) DrawView::execute();           //make sure Scale is up to date

    gp_Pnt inputCenter;
    inputCenter = TechDrawGeometry::findCentroid(shape,
                                                 Direction.getValue());
    shapeCentroid = Base::Vector3d(inputCenter.X(),inputCenter.Y(),inputCenter.Z());

    TopoDS_Shape mirroredShape;
    mirroredShape = TechDrawGeometry::mirrorShape(shape,
                                                  inputCenter,
                                                  Scale.getValue());

     gp_Ax2 viewAxis = getViewAxis(shapeCentroid,Direction.getValue());
     geometryObject =  buildGeometryObject(mirroredShape,viewAxis);
     
     //Base::Console().Message("TRACE - DVP::execute - u: %s v: %s w: %s\n",
     //         DrawUtil::formatVector(getUDir()).c_str(), DrawUtil::formatVector(getVDir()).c_str(), DrawUtil::formatVector(getWDir()).c_str());

#if MOD_TECHDRAW_HANDLE_FACES
    if (handleFaces()) {
        try {
            extractFaces();
        }
        catch (Standard_Failure) {
            Handle(Standard_Failure) e4 = Standard_Failure::Caught();
            Base::Console().Log("LOG - DVP::execute - extractFaces failed for %s - %s **\n",getNameInDocument(),e4->GetMessageString());
            return new App::DocumentObjectExecReturn(e4->GetMessageString());
        }
    }
#endif //#if MOD_TECHDRAW_HANDLE_FACES

//   Base::Console().Message("TRACE _ DVP::exec - %s/%s u: %s v: %s w: %s\n",getNameInDocument(),Label.getValue(),
//                           DrawUtil::formatVector(getUDir()).c_str(), DrawUtil::formatVector(getVDir()).c_str(),DrawUtil::formatVector(getWDir()).c_str());

    return App::DocumentObject::StdReturn;
}

short DrawViewPart::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  =  (Direction.isTouched()  ||
                    Source.isTouched()  ||
                    Scale.isTouched() ||
                    ScaleType.isTouched());
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
TechDrawGeometry::GeometryObject* DrawViewPart::buildGeometryObject(TopoDS_Shape shape, gp_Ax2 viewAxis)
{
    TechDrawGeometry::GeometryObject* go = new TechDrawGeometry::GeometryObject(getNameInDocument(), this);
    go->setIsoCount(IsoCount.getValue());

    Base::Vector3d baseProjDir = Direction.getValue();
    saveParamSpace(baseProjDir);

    go->projectShape(shape,
                     viewAxis);
    go->extractGeometry(TechDrawGeometry::ecHARD,                   //always show the hard&outline visible lines
                        true);
    go->extractGeometry(TechDrawGeometry::ecOUTLINE,
                        true);
    if (SmoothVisible.getValue()) {
        go->extractGeometry(TechDrawGeometry::ecSMOOTH,
                            true);
    }
    if (SeamVisible.getValue()) {
        go->extractGeometry(TechDrawGeometry::ecSEAM,
                            true);
    }
    if ((IsoVisible.getValue()) && (IsoCount.getValue() > 0)) {
        go->extractGeometry(TechDrawGeometry::ecUVISO,
                            true);
    }
    if (HardHidden.getValue()) {
        go->extractGeometry(TechDrawGeometry::ecHARD,
                            false);
        go->extractGeometry(TechDrawGeometry::ecOUTLINE,
                            false);
    }
    if (SmoothHidden.getValue()) {
        go->extractGeometry(TechDrawGeometry::ecSMOOTH,
                            false);
    }
    if (SeamHidden.getValue()) {
        go->extractGeometry(TechDrawGeometry::ecSEAM,
                            false);
    }
    if (IsoHidden.getValue() && (IsoCount.getValue() > 0)) {
        go->extractGeometry(TechDrawGeometry::ecUVISO,
                            false);
    }
    bbox = go->calcBoundingBox();
    return go;
}

//! make faces from the existing edge geometry
void DrawViewPart::extractFaces()
{
    geometryObject->clearFaceGeom();
    const std::vector<TechDrawGeometry::BaseGeom*>& goEdges =
                       geometryObject->getVisibleFaceEdges(SmoothVisible.getValue(),SeamVisible.getValue());
    std::vector<TechDrawGeometry::BaseGeom*>::const_iterator itEdge = goEdges.begin();
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
        Base::Console().Warning("DVP::extractFaces - input is not planar graph. No face detection\n");
        return;
    }
    std::vector<TopoDS_Wire> fw = ew.getResultNoDups();

    std::vector<TopoDS_Wire> sortedWires = ew.sortStrip(fw,true);

    std::vector<TopoDS_Wire>::iterator itWire = sortedWires.begin();
    for (; itWire != sortedWires.end(); itWire++) {
        //version 1: 1 wire/face - no voids in face
        TechDrawGeometry::Face* f = new TechDrawGeometry::Face();
        const TopoDS_Wire& wire = (*itWire);
        TechDrawGeometry::Wire* w = new TechDrawGeometry::Wire(wire);
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

std::vector<TechDraw::DrawViewDimension*> DrawViewPart::getDimensions() const
{
    std::vector<TechDraw::DrawViewDimension*> result;
    std::vector<App::DocumentObject*> children = getInList();
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != children.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawViewDimension::getClassTypeId())) {
            TechDraw::DrawViewDimension* dim = dynamic_cast<TechDraw::DrawViewDimension*>(*it);
            result.push_back(dim);
        }
    }
    return result;
}


const std::vector<TechDrawGeometry::Vertex *> & DrawViewPart::getVertexGeometry() const
{
    return geometryObject->getVertexGeometry();
}

const std::vector<TechDrawGeometry::Face *> & DrawViewPart::getFaceGeometry() const
{
    return geometryObject->getFaceGeometry();
}

const std::vector<TechDrawGeometry::BaseGeom  *> & DrawViewPart::getEdgeGeometry() const
{
    return geometryObject->getEdgeGeometry();
}

//! returns existing BaseGeom of 2D Edge(idx)
TechDrawGeometry::BaseGeom* DrawViewPart::getProjEdgeByIndex(int idx) const
{
    const std::vector<TechDrawGeometry::BaseGeom *> &geoms = getEdgeGeometry();
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
TechDrawGeometry::Vertex* DrawViewPart::getProjVertexByIndex(int idx) const
{
    const std::vector<TechDrawGeometry::Vertex *> &geoms = getVertexGeometry();
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


//this is never used!!
//! returns existing geometry of 2D Face(idx)
//version 1 Face has 1 wire
std::vector<TechDrawGeometry::BaseGeom*> DrawViewPart::getProjFaceByIndex(int idx) const
{
    (void) idx;
    std::vector<TechDrawGeometry::BaseGeom*> result;
    const std::vector<TechDrawGeometry::Face *>& faces = getFaceGeometry();
    for (auto& f:faces) {
        for (auto& w:f->wires) {
            for (auto& g:w->geoms) {
                result.push_back(g);
            }
        }
    }
    return result;
}

std::vector<TopoDS_Wire> DrawViewPart::getWireForFace(int idx) const
{
//    Base::Console().Message("TRACE - DVP::getWireForFace(%d)\n",idx);
    std::vector<TopoDS_Wire> result;
    std::vector<TopoDS_Edge> edges;
    const std::vector<TechDrawGeometry::Face *>& faces = getFaceGeometry();
    TechDrawGeometry::Face * ourFace = faces.at(idx);
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
 
//    Base::Console().Message("TRACE - DVP::getWireForFace(%d) returns %d wires\n",idx,result.size());
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
    Base::Vector3d centeredPoint = pt - shapeCentroid;
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
    const std::vector<TechDrawGeometry::Vertex*> &verts = getVertexGeometry();
    const std::vector<TechDrawGeometry::BaseGeom*> &edges = getEdgeGeometry();
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
     gp_Ax2 viewAxis = TechDrawGeometry::getViewAxis(pt,axis,flip);
     return viewAxis;
}

void DrawViewPart::saveParamSpace(const Base::Vector3d& direction, const Base::Vector3d& xAxis)
{
    //Base::Console().Message("TRACE - DVP::saveParamSpace()\n");
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

const std::vector<TechDrawGeometry::BaseGeom  *> DrawViewPart::getVisibleFaceEdges() const
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
    //Base::Console().Message("TRACE - DVP::getRunControl - handleFaces: %d\n",m_handleFaces);
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
    nowDeleting = true;
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
              std::string viewName = (*it3)->getNameInDocument();
              Base::Interpreter().runStringArg("App.getDocument(\"%s\").removeObject(\"%s\")",
                                                docName.c_str(), viewName.c_str());
        }
    }

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
