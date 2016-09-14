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
#include <BRepLProp_CurveTool.hxx>
#include <BRepLProp_CLProps.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Geom_Curve.hxx>
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
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#endif

#include <algorithm>

#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Mod/Part/App/PartFeature.h>

#include "DrawUtil.h"
#include "DrawViewSection.h"
#include "Geometry.h"
#include "GeometryObject.h"
#include "DrawViewPart.h"
#include "DrawHatch.h"
#include "EdgeWalker.h"


#include <Mod/TechDraw/App/DrawViewPartPy.h>  // generated from DrawViewPartPy.xml

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewPart
//===========================================================================

App::PropertyFloatConstraint::Constraints DrawViewPart::floatRange = {0.01f,5.0f,0.05f};

PROPERTY_SOURCE(TechDraw::DrawViewPart, TechDraw::DrawView)

DrawViewPart::DrawViewPart(void) : geometryObject(0)
{
    static const char *group = "Projection";
    static const char *fgroup = "Format";
    static const char *lgroup = "SectionLine";

    //properties that affect Geometry
    ADD_PROPERTY_TYPE(Source ,(0),group,App::Prop_None,"3D Shape to view");
    ADD_PROPERTY_TYPE(Direction ,(0,0,1.0)    ,group,App::Prop_None,"Projection direction. The direction you are looking from.");
    ADD_PROPERTY_TYPE(XAxisDirection ,(1,0,0) ,group,App::Prop_None,"Where to place projection's XAxis (rotation)");
    ADD_PROPERTY_TYPE(Tolerance,(0.05f),group,App::Prop_None,"Internal tolerance for calculations");
    Tolerance.setConstraints(&floatRange);

    //properties that affect Appearance
    ADD_PROPERTY_TYPE(ShowHiddenLines ,(false),fgroup,App::Prop_None,"Hidden lines on/off");
    ADD_PROPERTY_TYPE(ShowSmoothLines ,(false),fgroup,App::Prop_None,"Smooth lines on/off");
    ADD_PROPERTY_TYPE(ShowSeamLines ,(false),fgroup,App::Prop_None,"Seam lines on/off");
    //ADD_PROPERTY_TYPE(ShowIsoLines ,(false),group,App::Prop_None,"Iso u,v lines on/off");
    ADD_PROPERTY_TYPE(LineWidth,(0.7f),fgroup,App::Prop_None,"The thickness of visible lines");
    ADD_PROPERTY_TYPE(HiddenWidth,(0.15),fgroup,App::Prop_None,"The thickness of hidden lines, if enabled");
    ADD_PROPERTY_TYPE(ShowCenters ,(true),fgroup,App::Prop_None,"Center marks on/off");
    ADD_PROPERTY_TYPE(CenterScale,(2.0),fgroup,App::Prop_None,"Center mark size adjustment, if enabled");
    ADD_PROPERTY_TYPE(HorizCenterLine ,(false),fgroup,App::Prop_None,"Show a horizontal centerline through view");
    ADD_PROPERTY_TYPE(VertCenterLine ,(false),fgroup,App::Prop_None,"Show a vertical centerline through view");

    //properties that affect Section Line
    ADD_PROPERTY_TYPE(ShowSectionLine ,(true)    ,lgroup,App::Prop_None,"Show/hide section line if applicable");
    ADD_PROPERTY_TYPE(HorizSectionLine ,(true)    ,lgroup,App::Prop_None,"Section line is horizontal");
    ADD_PROPERTY_TYPE(ArrowUpSection ,(false)    ,lgroup,App::Prop_None,"Section line arrows point up");
    ADD_PROPERTY_TYPE(SymbolSection,("A") ,lgroup,App::Prop_None,"Section identifier");

    geometryObject = new TechDrawGeometry::GeometryObject();
}

DrawViewPart::~DrawViewPart()
{
    delete geometryObject;
}


App::DocumentObjectExecReturn *DrawViewPart::execute(void)
{
    App::DocumentObject *link = Source.getValue();
    if (!link) {
        return new App::DocumentObjectExecReturn("FVP - No Source object linked");
    }

    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        return new App::DocumentObjectExecReturn("FVP - Linked object is not a Part object");
    }

    TopoDS_Shape shape = static_cast<Part::Feature*>(link)->Shape.getShape().getShape();
    if (shape.IsNull()) {
        return new App::DocumentObjectExecReturn("FVP - Linked shape object is empty");
    }

    geometryObject->setTolerance(Tolerance.getValue());
    double s = Scale.getValue();
    if (!s) {                                           //might be problem, might be mid property change
        Base::Console().Log("INFO - DVP::execute - Scale: %.3f\n",s);
        return DrawView::execute();
    }
    geometryObject->setScale(s);

    //TODO: remove these try/catch block when code is stable
    gp_Pnt inputCenter;
    try {
        inputCenter = TechDrawGeometry::findCentroid(shape,
                                                     Direction.getValue(),
                                                     getValidXDir());
        shapeCentroid = Base::Vector3d(inputCenter.X(),inputCenter.Y(),inputCenter.Z());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e1 = Standard_Failure::Caught();
        Base::Console().Log("LOG - DVP::execute - findCentroid failed for %s - %s **\n",getNameInDocument(),e1->GetMessageString());
        return new App::DocumentObjectExecReturn(e1->GetMessageString());
    }

    TopoDS_Shape mirroredShape;
    try {
        mirroredShape = TechDrawGeometry::mirrorShape(shape,
                                                       inputCenter,
                                                       Scale.getValue());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e2 = Standard_Failure::Caught();
        Base::Console().Log("LOG - DVP::execute - mirrorShape failed for %s - %s **\n",getNameInDocument(),e2->GetMessageString());
        return new App::DocumentObjectExecReturn(e2->GetMessageString());
    }

    try {
        buildGeometryObject(mirroredShape,inputCenter);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e3 = Standard_Failure::Caught();
        Base::Console().Log("LOG - DVP::execute - buildGeometryObject failed for %s - %s **\n",getNameInDocument(),e3->GetMessageString());
        return new App::DocumentObjectExecReturn(e3->GetMessageString());
    }

#if MOD_TECHDRAW_HANDLE_FACES
    try {
        extractFaces();
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e4 = Standard_Failure::Caught();
        Base::Console().Log("LOG - DVP::execute - extractFaces failed for %s - %s **\n",getNameInDocument(),e4->GetMessageString());
        return new App::DocumentObjectExecReturn(e4->GetMessageString());
    }
#endif //#if MOD_TECHDRAW_HANDLE_FACES

    //TODO: not sure about this
    // There is a guaranteed change so check any references linked to this and touch
    // We need to update all views pointing at this (ProjectionGroup, ClipGroup, Section, etc)
//    std::vector<App::DocumentObject*> parent = getInList();
//    for (std::vector<App::DocumentObject*>::iterator it = parent.begin(); it != parent.end(); ++it) {
//        if ((*it)->getTypeId().isDerivedFrom(DrawView::getClassTypeId())) {
//            TechDraw::DrawView *view = static_cast<TechDraw::DrawView *>(*it);
//            view->touch();
//        }
//    }
    return DrawView::execute();
}

short DrawViewPart::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  =  (Direction.isTouched()  ||
                    XAxisDirection.isTouched()  ||
                    Source.isTouched()  ||
                    Scale.isTouched()  ||
                    ScaleType.isTouched()  ||
                    Tolerance.isTouched()  ||
                    ShowHiddenLines.isTouched()  ||
                    ShowSmoothLines.isTouched()  ||
                    ShowSeamLines.isTouched() );
        }

    if (result) {
        return result;
    }
    return TechDraw::DrawView::mustExecute();
}

void DrawViewPart::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &SymbolSection && getSectionRef()) {
            getSectionRef()->touch();
        }
        if (prop == &Direction ||
            prop == &XAxisDirection ||
            prop == &Source ||
            prop == &Scale ||
            prop == &ScaleType  ||
            prop == &ShowHiddenLines ||
            prop == &ShowSmoothLines ||
            prop == &ShowSeamLines)
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
    }
    DrawView::onChanged(prop);

//TODO: when scale changes, any Dimensions for this View sb recalculated.  (might happen anyway if document is recomputed?)
}

void DrawViewPart::buildGeometryObject(TopoDS_Shape shape, gp_Pnt& inputCenter)
{
    Base::Vector3d baseProjDir = Direction.getValue();
    Base::Vector3d validXDir = getValidXDir();

    saveParamSpace(baseProjDir,
                   validXDir);

    geometryObject->projectShape(shape,
                            inputCenter,
                            Direction.getValue(),
                            validXDir);
    //TODO: why not be all line type in 1 call to extract geometry
    geometryObject->extractGeometry(TechDrawGeometry::ecHARD,
                                    true);
    geometryObject->extractGeometry(TechDrawGeometry::ecOUTLINE,
                                    true);
    //if (ShowSmoothLines.getValue()) {
        geometryObject->extractGeometry(TechDrawGeometry::ecSMOOTH,
                                        true);
    //}
    //if (ShowSeamLines.getValue()) {
        geometryObject->extractGeometry(TechDrawGeometry::ecSEAM,
                                        true);
    //}
    //if (ShowIsoLines.getValue()) {
    //    geometryObject->extractGeometry(TechDrawGeometry::ecUVISO,
    //                                    true);
    //}
    //if (ShowHiddenLines.getValue()) {
        geometryObject->extractGeometry(TechDrawGeometry::ecHARD,
                                        false);
        //geometryObject->extractGeometry(TechDrawGeometry::ecOUTLINE,     //hidden outline,smooth,seam??
        //                                true);
    //}
    bbox = geometryObject->calcBoundingBox();
}

//! make faces from the existing edge geometry
void DrawViewPart::extractFaces()
{
    geometryObject->clearFaceGeom();
    const std::vector<TechDrawGeometry::BaseGeom*>& goEdges = geometryObject->getEdgeGeometry();
    std::vector<TechDrawGeometry::BaseGeom*>::const_iterator itEdge = goEdges.begin();
    std::vector<TopoDS_Edge> origEdges;
    for (;itEdge != goEdges.end(); itEdge++) {
        if ((*itEdge)->visible) {                        //don't make invisible faces!
            origEdges.push_back((*itEdge)->occEdge);
        }
    }

    std::vector<TopoDS_Edge> faceEdges;
    std::vector<TopoDS_Edge> nonZero;
    for (auto& e:origEdges) {                            //drop any zero edges
        if (!DrawUtil::isZeroEdge(e)) {
            nonZero.push_back(e);
        }
    }
    faceEdges = nonZero;
    origEdges = nonZero;

    std::vector<TopoDS_Edge>::iterator itOrig = origEdges.begin();

    //HLR algo does not provide all edge intersections for edge endpoints.
    //need to split long edges touched by Vertex of another edge
    int idb = 0;
    for (; itOrig != origEdges.end(); itOrig++, idb++) {
        TopoDS_Vertex v1 = TopExp::FirstVertex((*itOrig));
        TopoDS_Vertex v2 = TopExp::LastVertex((*itOrig));
        if (DrawUtil::isSamePoint(v1,v2)) {
            continue;  //skip zero length edges. shouldn't happen ;)
        }
        std::vector<TopoDS_Edge>::iterator itNew = faceEdges.begin();
        std::vector<size_t> deleteList;
        std::vector<TopoDS_Edge> edgesToAdd;
        int idx = 0;
        for (; itNew != faceEdges.end(); itNew++,idx++) {
            if ( itOrig->IsSame(*itNew) ){
                continue;
            }
            if (DrawUtil::isZeroEdge((*itNew))) {
                continue;  //skip zero length edges. shouldn't happen ;)
            }

            bool removeThis = false;
            std::vector<TopoDS_Vertex> splitPoints;
            if (isOnEdge((*itNew),v1,false)) {
                splitPoints.push_back(v1);
                removeThis = true;
            }
            if (isOnEdge((*itNew),v2,false)) {
                splitPoints.push_back(v2);
                removeThis = true;
            }
            if (removeThis) {
                deleteList.push_back(idx);
            }

            if (!splitPoints.empty()) {
                if (!DrawUtil::isZeroEdge((*itNew))) {
                    std::vector<TopoDS_Edge> subEdges = splitEdge(splitPoints,(*itNew));
                    edgesToAdd.insert(std::end(edgesToAdd), std::begin(subEdges), std::end(subEdges));
                }
            }
        }
        //delete the split edge(s) and add the subedges
        //TODO: look into sets or maps or???? for all this
        std::sort(deleteList.begin(),deleteList.end());                 //ascending
        auto last = std::unique(deleteList.begin(), deleteList.end());  //duplicates at back
        deleteList.erase(last, deleteList.end());                       //remove dupls
        std::vector<size_t>::reverse_iterator ritDel = deleteList.rbegin();
        for ( ; ritDel != deleteList.rend(); ritDel++) {
            faceEdges.erase(faceEdges.begin() + (*ritDel));
        }
        faceEdges.insert(std::end(faceEdges), std::begin(edgesToAdd),std::end(edgesToAdd));
    }


    if (faceEdges.empty()) {
        Base::Console().Log("LOG - DVP::extractFaces - no faceEdges\n");
        return;
    }


//find all the wires in the pile of faceEdges
    EdgeWalker ew;
    ew.loadEdges(faceEdges);
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

double DrawViewPart::simpleMinDist(TopoDS_Shape s1, TopoDS_Shape s2) const
{
    Standard_Real minDist = -1;

    BRepExtrema_DistShapeShape extss(s1, s2);
    if (!extss.IsDone()) {
        Base::Console().Message("DVP - BRepExtrema_DistShapeShape failed");
        return -1;
    }
    int count = extss.NbSolution();
    if (count != 0) {
        minDist = extss.Value();
    } else {
        minDist = -1;
    }
    return minDist;
}

bool DrawViewPart::isOnEdge(TopoDS_Edge e, TopoDS_Vertex v, bool allowEnds)
{
    bool result = false;
    double dist = simpleMinDist(v,e);
    if (dist < 0.0) {
        Base::Console().Error("DVP::isOnEdge - simpleMinDist failed: %.3f\n",dist);
        result = false;
    } else if (dist < Precision::Confusion()) {
        result = true;
    }
    if (result) {
        TopoDS_Vertex v1 = TopExp::FirstVertex(e);
        TopoDS_Vertex v2 = TopExp::LastVertex(e);
        if (DrawUtil::isSamePoint(v,v1) || DrawUtil::isSamePoint(v,v2)) {
            if (!allowEnds) {
                result = false;
            }
        }
    }
    return result;
}

std::vector<TopoDS_Edge> DrawViewPart::splitEdge(std::vector<TopoDS_Vertex> splitPoints, TopoDS_Edge e)
{
    std::vector<TopoDS_Edge> result;
    if (splitPoints.empty()) {
        return result;
    }
    TopoDS_Vertex vStart = TopExp::FirstVertex(e);
    TopoDS_Vertex vEnd = TopExp::LastVertex(e);

    BRepAdaptor_Curve adapt(e);
    Handle_Geom_Curve c = adapt.Curve().Curve();
    //simple version for 1 splitPoint
    //TODO: handle case where e is split in multiple points (ie circular edge cuts line twice)
// for some reason, BRepBuilderAPI_MakeEdge sometimes fails without reporting IsDone status or error, so we use try/catch
    try {
        BRepBuilderAPI_MakeEdge mkBuilder1(c, vStart ,splitPoints[0]);
        if (mkBuilder1.IsDone()) {
            TopoDS_Edge e1 = mkBuilder1.Edge();
            result.push_back(e1);
        }
    }
    catch (Standard_Failure) {
        Base::Console().Message("LOG - DVP::splitEdge failed building 1st edge\n");
    }

    try{
        BRepBuilderAPI_MakeEdge mkBuilder2(c, splitPoints[0], vEnd);
        if (mkBuilder2.IsDone()) {
            TopoDS_Edge e2 = mkBuilder2.Edge();
            result.push_back(e2);
        }
    }
    catch (Standard_Failure) {
        Base::Console().Message("LOG - DVP::splitEdge failed building 2nd  edge\n");
    }
    return result;
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
    return geoms[idx];
}

//! returns existing geometry of 2D Vertex(idx)
TechDrawGeometry::Vertex* DrawViewPart::getProjVertexByIndex(int idx) const
{
    const std::vector<TechDrawGeometry::Vertex *> &geoms = getVertexGeometry();
    if (geoms.empty()) {
        Base::Console().Log("INFO - getProjVertexByIndex(%d) - no Vertex Geometry. Probably restoring?\n",idx);
        return NULL;
    }
    return geoms[idx];
}

//! returns existing geometry of 2D Face(idx)
//version 1 Face has 1 wire
std::vector<TechDrawGeometry::BaseGeom*> DrawViewPart::getProjFaceByIndex(int idx) const
{
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
    QRectF result(0.0,0.0,getBoxX(),getBoxY());  //this is from GO and is already scaled
    return result;
}

//used to project pt (ex SectionOrigin) onto paper plane
Base::Vector3d DrawViewPart::projectPoint(const Base::Vector3d& pt) const
{
    Base::Vector3d centeredPoint = pt - shapeCentroid;
    Base::Vector3d direction = Direction.getValue();
    Base::Vector3d xAxis = getValidXDir();
    gp_Ax2 viewAxis;
    viewAxis = gp_Ax2(gp_Pnt(0.0,0.0,0.0),
                      gp_Dir(direction.x, direction.y, direction.z),
                      gp_Dir(xAxis.x, xAxis.y, xAxis.z));
    HLRAlgo_Projector projector( viewAxis );
    gp_Pnt2d prjPnt;
    projector.Project(gp_Pnt(centeredPoint.x,centeredPoint.y,centeredPoint.z), prjPnt);
    return Base::Vector3d(prjPnt.X(),prjPnt.Y(), 0.0);
}

bool DrawViewPart::hasGeometry(void) const
{
    bool result = false;
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

Base::Vector3d DrawViewPart::getValidXDir() const
{
    Base::Vector3d X(1.0,0.0,0.0);
    Base::Vector3d Y(0.0,1.0,0.0);
    Base::Vector3d Z(0.0,0.0,1.0);
    Base::Vector3d xDir = XAxisDirection.getValue();
    if (xDir.Length() < Precision::Confusion()) {
        Base::Console().Warning("XAxisDirection has zero length - using (1,0,0)\n");
        xDir = X;
    }
    double xLength = xDir.Length();
    xDir.Normalize();
    Base::Vector3d viewDir = Direction.getValue();
    viewDir.Normalize();
    Base::Vector3d randomDir(0.0,0.0,0.0);
    if (xDir == viewDir) {
        randomDir = Y;
        if (randomDir == xDir) {
            randomDir = X;
        }
        xDir = randomDir;
        Base::Console().Warning("XAxisDirection cannot equal +/- Direction - using (%.3f,%.3f%.3f)\n",
                                xDir.x,xDir.y,xDir.z);
    } else if (xDir == (-1.0 * viewDir)) {
        randomDir = Y;
        if ((xDir == randomDir) ||
            (xDir == (-1.0 * randomDir))) {
                randomDir = X;
        }
        xDir = randomDir;
        Base::Console().Warning("XAxisDirection cannot equal +/- Direction - using (%.3f,%.3f%.3f)\n",
                                xDir.x,xDir.y,xDir.z);
    }
    return xLength * xDir;
}

void DrawViewPart::saveParamSpace(const Base::Vector3d& direction,
                                  const Base::Vector3d& xAxis)
{
    gp_Ax2 viewAxis;
    viewAxis = gp_Ax2(gp_Pnt(0, 0, 0),
                      gp_Dir(direction.x, -direction.y, direction.z),
                      gp_Dir(xAxis.x, -xAxis.y, xAxis.z)); // Y invert warning! //

    uDir = Base::Vector3d(xAxis.x, -xAxis.y, xAxis.z);
    gp_Dir ydir = viewAxis.YDirection();
    vDir = Base::Vector3d(ydir.X(),ydir.Y(),ydir.Z());
    wDir = Base::Vector3d(direction.x, -direction.y, direction.z);
}


DrawViewSection* DrawViewPart::getSectionRef(void) const
{
    DrawViewSection* result = nullptr;
    std::vector<App::DocumentObject*> inObjs = getInList();
    for (auto& o:inObjs) {
        if (o->getTypeId().isDerivedFrom(DrawViewSection::getClassTypeId())) {
            result = static_cast<TechDraw::DrawViewSection*>(o);
        }
    }
    return result;
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
