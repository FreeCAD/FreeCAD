/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
 *   Copyright (c) Luke Parry             (l.parry@warwick.ac.uk) 2013     *
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

#endif

#include <algorithm>

#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Mod/Part/App/PartFeature.h>

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

    ADD_PROPERTY_TYPE(Direction ,(0,0,1.0)    ,group,App::Prop_None,"Projection normal direction");
    ADD_PROPERTY_TYPE(Source ,(0),group,App::Prop_None,"3D Shape to view");
    ADD_PROPERTY_TYPE(ShowHiddenLines ,(false),group,App::Prop_None,"Hidden lines on/off");
    ADD_PROPERTY_TYPE(ShowSmoothLines ,(false),group,App::Prop_None,"Smooth lines on/off");
    ADD_PROPERTY_TYPE(ShowSeamLines ,(false),group,App::Prop_None,"Seam lines on/off");
    //ADD_PROPERTY_TYPE(ShowIsoLines ,(false),group,App::Prop_None,"Iso u,v lines on/off");
    ADD_PROPERTY_TYPE(Tolerance,(0.05f),group,App::Prop_None,"Internal tolerance");
    Tolerance.setConstraints(&floatRange);
    ADD_PROPERTY_TYPE(XAxisDirection ,(1,0,0) ,group,App::Prop_None,"Direction to use as X-axis in projection");
    ADD_PROPERTY_TYPE(LineWidth,(0.7f),fgroup,App::Prop_None,"The thickness of visible lines");
    ADD_PROPERTY_TYPE(HiddenWidth,(0.15),fgroup,App::Prop_None,"The thickness of hidden lines, if enabled");
    ADD_PROPERTY_TYPE(ShowCenters ,(true),fgroup,App::Prop_None,"Center marks on/off");
    ADD_PROPERTY_TYPE(CenterScale,(2.0),fgroup,App::Prop_None,"Center mark size adjustment, if enabled");

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
    geometryObject->setScale(Scale.getValue());
    try {
        gp_Pnt inputCenter = TechDrawGeometry::findCentroid(shape,
                                                            Direction.getValue(),
                                                            getValidXDir());
        TopoDS_Shape mirroredShape = TechDrawGeometry::mirrorShape(shape,
                                                                 inputCenter,
                                                                 Scale.getValue());
        buildGeometryObject(mirroredShape,inputCenter);
#if MOD_TECHDRAW_HANDLE_FACES
        extractFaces();
#endif //#if MOD_TECHDRAW_HANDLE_FACES

    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    // There is a guaranteed change so check any references linked to this and touch
    // We need to update all views pointing at this (ProjectionGroup, ClipGroup, etc)
    std::vector<App::DocumentObject*> parent = getInList();
    for (std::vector<App::DocumentObject*>::iterator it = parent.begin(); it != parent.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawView::getClassTypeId())) {
            TechDraw::DrawView *view = static_cast<TechDraw::DrawView *>(*it);
            view->touch();
        }
    }
    return DrawView::execute();
}

short DrawViewPart::mustExecute() const
{
    short result  = (Direction.isTouched() ||
            XAxisDirection.isTouched() ||
            Source.isTouched() ||
            Scale.isTouched() ||
            ScaleType.isTouched() ||
            ShowHiddenLines.isTouched() ||
            ShowSmoothLines.isTouched() ||
            ShowSeamLines.isTouched()   ||
            LineWidth.isTouched()       ||
            Tolerance.isTouched()       ||
            HiddenWidth.isTouched());
    return result;
}

void DrawViewPart::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &Direction ||
            prop == &XAxisDirection ||
            prop == &Source ||
            prop == &Scale ||
            prop == &ScaleType ||
            prop == &ShowHiddenLines ||
            prop == &ShowSmoothLines ||
            prop == &ShowSeamLines   ||
            prop == &LineWidth       ||
            prop == &HiddenWidth     ||
            prop == &ShowCenters     ||
            prop == &CenterScale ) {
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
        }
    }
    DrawView::onChanged(prop);

//TODO: when scale changes, any Dimensions for this View sb recalculated.  (might happen anyway if document is recomputed?)
}

void DrawViewPart::buildGeometryObject(TopoDS_Shape shape, gp_Pnt& inputCenter)
{
    geometryObject->projectShape(shape,
                            inputCenter,
                            Direction.getValue(),
                            getValidXDir());
    geometryObject->extractGeometry(TechDrawGeometry::ecHARD,
                                    true);
    geometryObject->extractGeometry(TechDrawGeometry::ecOUTLINE,
                                    true);
    if (ShowSmoothLines.getValue()) {
        geometryObject->extractGeometry(TechDrawGeometry::ecSMOOTH,
                                        true);
    }
    if (ShowSeamLines.getValue()) {
        geometryObject->extractGeometry(TechDrawGeometry::ecSEAM,
                                        true);
    }
    //if (ShowIsoLines.getValue()) {
    //    geometryObject->extractGeometry(TechDrawGeometry::ecUVISO,
    //                                    true);
    //}
    if (ShowHiddenLines.getValue()) {
        geometryObject->extractGeometry(TechDrawGeometry::ecHARD,
                                        false);
        //geometryObject->extractGeometry(TechDrawGeometry::ecOUTLINE,     //hidden outline,smooth,seam??
        //                                true);
    }
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

    std::vector<TopoDS_Edge> faceEdges = origEdges;
    std::vector<TopoDS_Edge>::iterator itOrig = origEdges.begin();

    //HLR algo does not provide all edge intersections for edge endpoints.
    //need to split long edges touched by Vertex of another edge
    int idb = 0;
    for (; itOrig != origEdges.end(); itOrig++, idb++) {
        TopoDS_Vertex v1 = TopExp::FirstVertex((*itOrig));
        TopoDS_Vertex v2 = TopExp::LastVertex((*itOrig));
        std::vector<TopoDS_Edge>::iterator itNew = faceEdges.begin();
        std::vector<size_t> deleteList;
        std::vector<TopoDS_Edge> edgesToAdd;
        int idx = 0;
        for (; itNew != faceEdges.end(); itNew++,idx++) {
            if ( itOrig->IsSame(*itNew) ){
                continue;
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
                std::vector<TopoDS_Edge> subEdges = splitEdge(splitPoints,(*itNew));
                edgesToAdd.insert(std::end(edgesToAdd), std::begin(subEdges), std::end(subEdges));
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

    std::vector<TopoDS_Vertex> uniqueVert = makeUniqueVList(faceEdges);
    std::vector<WalkerEdge> walkerEdges = makeWalkerEdges(faceEdges,uniqueVert);

    EdgeWalker ew;
    ew.setSize(uniqueVert.size());
    ew.loadEdges(walkerEdges);
    ew.perform();
    facelist result = ew.getResult();

    facelist::iterator iFace = result.begin();
    std::vector<TopoDS_Wire> fw;
    for (;iFace != result.end(); iFace++) {
        edgelist::iterator iEdge = (*iFace).begin();
        std::vector<TopoDS_Edge> fe;
        for (;iEdge != (*iFace).end(); iEdge++) {
            fe.push_back(faceEdges.at((*iEdge).idx));
        }
    TopoDS_Wire w = makeCleanWire(fe);             //make 1 clean wire from its edges
    fw.push_back(w);
    }

    std::vector<TopoDS_Wire> sortedWires = sortWiresBySize(fw,false);
    if (!sortedWires.size()) {
        Base::Console().Log("INFO - DVP::extractFaces - no sorted Wires!\n");
        return;                                     // might happen in the middle of changes?
    }

    //remove the largest wire (OuterWire of graph)
    Bnd_Box bigBox;
    if (sortedWires.size() && !sortedWires.front().IsNull()) {
        BRepBndLib::Add(sortedWires.front(), bigBox);
        bigBox.SetGap(0.0);
    }
    std::vector<std::size_t> toBeChecked;
    std::vector<TopoDS_Wire>::iterator it = sortedWires.begin() + 1;
    for (; it != sortedWires.end(); it++) {
        if (!(*it).IsNull()) {
            Bnd_Box littleBox;
            BRepBndLib::Add((*it), littleBox);
            littleBox.SetGap(0.0);
            if (bigBox.SquareExtent() > littleBox.SquareExtent()) {
                break;
            } else {
                auto position = std::distance( sortedWires.begin(), it );    //get an index from iterator
                toBeChecked.push_back(position);
            }
        }
    }
    //unfortuneately, faces can have same bbox, but not be same size.  need to weed out biggest
    if (toBeChecked.size() == 0) {
        //nobody had as big a bbox as first element of sortedWires
        sortedWires.erase(sortedWires.begin());
    } else if (toBeChecked.size() > 0) {
        BRepBuilderAPI_MakeFace mkFace(sortedWires.front());
        const TopoDS_Face& face = mkFace.Face();
        GProp_GProps props;
        BRepGProp::SurfaceProperties(face, props);
        double bigArea = props.Mass();
        unsigned int bigIndex = 0;
        for (unsigned int idx = 1; idx < toBeChecked.size(); idx++) {
            BRepBuilderAPI_MakeFace mkFace2(sortedWires.at(idx));
            const TopoDS_Face& face2 = mkFace2.Face();
            BRepGProp::SurfaceProperties(face2, props);
            double area = props.Mass();
            if (area > bigArea) {
                bigArea = area;
                bigIndex = idx;
            }
        }
        sortedWires.erase(sortedWires.begin() + bigIndex);
    }

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

std::vector<TopoDS_Vertex> DrawViewPart:: makeUniqueVList(std::vector<TopoDS_Edge> edges)
{
    std::vector<TopoDS_Vertex> uniqueVert;
    for(auto& e:edges) {
        TopoDS_Vertex v1 = TopExp::FirstVertex(e);
        TopoDS_Vertex v2 = TopExp::LastVertex(e);
        bool addv1 = true;
        bool addv2 = true;
        for (auto v:uniqueVert) {
            if (isSamePoint(v,v1))
                addv1 = false;
            if (isSamePoint(v,v2))
                addv2 = false;
        }
        if (addv1)
            uniqueVert.push_back(v1);
        if (addv2)
            uniqueVert.push_back(v2);
    }
    return uniqueVert;
}

//!make WalkerEdges (unique Vertex index pairs) from edge list
std::vector<WalkerEdge> DrawViewPart::makeWalkerEdges(std::vector<TopoDS_Edge> edges,
                                                      std::vector<TopoDS_Vertex> verts)
{
    std::vector<WalkerEdge> walkerEdges;
    for (auto e:edges) {
        TopoDS_Vertex ev1 = TopExp::FirstVertex(e);
        TopoDS_Vertex ev2 = TopExp::LastVertex(e);
        int v1dx = findUniqueVert(ev1, verts);
        int v2dx = findUniqueVert(ev2, verts);
        WalkerEdge we;
        we.v1 = v1dx;
        we.v2 = v2dx;
        walkerEdges.push_back(we);
    }
    return walkerEdges;
}

int DrawViewPart::findUniqueVert(TopoDS_Vertex vx, std::vector<TopoDS_Vertex> &uniqueVert)
{
    int idx = 0;
    int result = 0;
    for(auto& v:uniqueVert) {                    //we're always going to find vx, right?
        if (isSamePoint(v,vx)) {
            result = idx;
            break;
        }
        idx++;
    }                                           //if idx >= uniqueVert.size() TARFU
    return result;
}

double DrawViewPart::simpleMinDist(TopoDS_Shape s1, TopoDS_Shape s2)
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
        if (isSamePoint(v,v1) || isSamePoint(v,v2)) {
            if (!allowEnds) {
                result = false;
            }
        }
    }
    return result;
}

bool DrawViewPart::isSamePoint(TopoDS_Vertex v1, TopoDS_Vertex v2)
{
    bool result = false;
    gp_Pnt p1 = BRep_Tool::Pnt(v1);
    gp_Pnt p2 = BRep_Tool::Pnt(v2);
    if (p1.IsEqual(p2,Precision::Confusion())) {
        result = true;
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
    BRepBuilderAPI_MakeEdge mkBuilder1(c, vStart, splitPoints[0]);
    TopoDS_Edge e1 = mkBuilder1.Edge();
    BRepBuilderAPI_MakeEdge mkBuilder2(c, splitPoints[0], vEnd);
    TopoDS_Edge e2 = mkBuilder2.Edge();
    result.push_back(e1);
    result.push_back(e2);
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

//! make a clean wire with sorted, oriented, connected, etc edges
TopoDS_Wire DrawViewPart::makeCleanWire(std::vector<TopoDS_Edge> edges, double tol)
{
    TopoDS_Wire result;
    BRepBuilderAPI_MakeWire mkWire;
    ShapeFix_ShapeTolerance sTol;
    Handle(ShapeExtend_WireData) wireData = new ShapeExtend_WireData();

    for (auto e:edges) {
        wireData->Add(e);
    }

    Handle(ShapeFix_Wire) fixer = new ShapeFix_Wire;
    fixer->Load(wireData);
    fixer->Perform();
    fixer->FixReorder();
    fixer->SetMaxTolerance(tol);
    fixer->ClosedWireMode() = Standard_True;
    fixer->FixConnected(Precision::Confusion());
    fixer->FixClosed(Precision::Confusion());

    for (int i = 1; i <= wireData->NbEdges(); i ++) {
        TopoDS_Edge edge = fixer->WireData()->Edge(i);
        sTol.SetTolerance(edge, tol, TopAbs_VERTEX);
        mkWire.Add(edge);
    }

    result = mkWire.Wire();
    return result;
}

//! return true if w1 bbox is bigger than w2 bbox
//NOTE: this won't necessarily sort the OuterWire correctly (ex smaller wire, same bbox)
class DrawViewPart::wireCompare: public std::binary_function<const TopoDS_Wire&,
                                                            const TopoDS_Wire&, bool>
{
public:
    bool operator() (const TopoDS_Wire& w1, const TopoDS_Wire& w2)
    {
        Bnd_Box box1, box2;
        if (!w1.IsNull()) {
            BRepBndLib::Add(w1, box1);
            box1.SetGap(0.0);
        }

        if (!w2.IsNull()) {
            BRepBndLib::Add(w2, box2);
            box2.SetGap(0.0);
        }

        return box1.SquareExtent() > box2.SquareExtent();
    }
};

//sort wires in order of bbox diagonal.
std::vector<TopoDS_Wire> DrawViewPart::sortWiresBySize(std::vector<TopoDS_Wire>& w, bool ascend)
{
    std::vector<TopoDS_Wire> wires = w;
    std::sort(wires.begin(), wires.end(), wireCompare());
    if (ascend) {
        std::reverse(wires.begin(),wires.end());
    }
    return wires;
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
    Base::Vector3d Y(1.0,0.0,0.0);
    Base::Vector3d xDir = XAxisDirection.getValue();
    if (xDir.Length() < Precision::Confusion()) {
        Base::Console().Warning("XAxisDirection has zero length - using (1,0,0)\n");
        xDir = X;
    }
    Base::Vector3d viewDir = Direction.getValue();
    if ((xDir - viewDir).Length() < Precision::Confusion()) {
        if (xDir == X) {
            xDir = Y;
        }else{
            xDir = X;
        }
        Base::Console().Warning("XAxisDirection cannot equal Direction - using (%.3f,%.3f%.3f)\n",
                                 xDir.x,xDir.y,xDir.z);
    }
    return xDir;
}

void DrawViewPart::dumpVertexes(const char* text, const TopoDS_Shape& s)
{
    Base::Console().Message("DUMP - %s\n",text);
    TopExp_Explorer expl(s, TopAbs_VERTEX);
    int i;
    for (i = 1 ; expl.More(); expl.Next(),i++) {
        const TopoDS_Vertex& v = TopoDS::Vertex(expl.Current());
        gp_Pnt pnt = BRep_Tool::Pnt(v);
        Base::Console().Message("v%d: (%.3f,%.3f,%.3f)\n",i,pnt.X(),pnt.Y(),pnt.Z());
    }
}

void DrawViewPart::dump1Vertex(const char* text, const TopoDS_Vertex& v)
{
    Base::Console().Message("DUMP - DVP::dump1Vertex - %s\n",text);
    gp_Pnt pnt = BRep_Tool::Pnt(v);
    Base::Console().Message("%s: (%.3f,%.3f,%.3f)\n",text,pnt.X(),pnt.Y(),pnt.Z());
}

PyObject *DrawViewPart::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewPartPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

void DrawViewPart::dumpEdge(char* label, int i, TopoDS_Edge e)
{
    BRepAdaptor_Curve adapt(e);
    double start = BRepLProp_CurveTool::FirstParameter(adapt);
    double end = BRepLProp_CurveTool::LastParameter(adapt);
    BRepLProp_CLProps propStart(adapt,start,0,Precision::Confusion());
    const gp_Pnt& vStart = propStart.Value();
    BRepLProp_CLProps propEnd(adapt,end,0,Precision::Confusion());
    const gp_Pnt& vEnd = propEnd.Value();
    //Base::Console().Message("%s edge:%d start:(%.3f,%.3f,%.3f)/%0.3f end:(%.2f,%.3f,%.3f)/%.3f\n",label,i,
    //                        vStart.X(),vStart.Y(),vStart.Z(),start,vEnd.X(),vEnd.Y(),vEnd.Z(),end);
    Base::Console().Message("%s edge:%d start:(%.3f,%.3f,%.3f)  end:(%.2f,%.3f,%.3f)\n",label,i,
                            vStart.X(),vStart.Y(),vStart.Z(),vEnd.X(),vEnd.Y(),vEnd.Z());
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
