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
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#endif

#include <limits>
#include <algorithm>
#include <cmath>
#include <GeomLib_Tool.hxx>

#include <App/Application.h>
#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>
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
    static const char *sgroup = "Show";

    //properties that affect Geometry
    ADD_PROPERTY_TYPE(Source ,(0),group,App::Prop_None,"3D Shape to view");
    ADD_PROPERTY_TYPE(Direction ,(0,0,1.0)    ,group,App::Prop_None,"Projection direction. The direction you are looking from.");
//    ADD_PROPERTY_TYPE(XAxisDirection ,(1,0,0) ,group,App::Prop_None,"Where to place projection's XAxis (rotation)");
    ADD_PROPERTY_TYPE(Tolerance,(0.05f),group,App::Prop_None,"Internal tolerance for calculations");
    Tolerance.setConstraints(&floatRange);

    //properties that affect Appearance
    //visible outline
    ADD_PROPERTY_TYPE(SmoothVisible ,(false),sgroup,App::Prop_None,"Visible Smooth lines on/off");
    ADD_PROPERTY_TYPE(SeamVisible ,(false),sgroup,App::Prop_None,"Visible Seam lines on/off");
    ADD_PROPERTY_TYPE(IsoVisible ,(false),sgroup,App::Prop_None,"Visible Iso u,v lines on/off");
    ADD_PROPERTY_TYPE(HardHidden ,(false),sgroup,App::Prop_None,"Hidden Hard lines on/off");   // and outline
    //hidden outline
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

    geometryObject = new TechDrawGeometry::GeometryObject(this);
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
        return new App::DocumentObjectExecReturn("FVP - No Source object linked");
    }

    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        return new App::DocumentObjectExecReturn("FVP - Linked object is not a Part object");
    }

    TopoDS_Shape shape = static_cast<Part::Feature*>(link)->Shape.getShape().getShape();
    if (shape.IsNull()) {
        return new App::DocumentObjectExecReturn("FVP - Linked shape object is empty");
    }

    //Base::Console().Message("TRACE - DVP::execute - %s/%s ScaleType: %s\n",getNameInDocument(),Label.getValue(),ScaleType.getValueAsString());

    (void) DrawView::execute();           //make sure Scale is up to date

    geometryObject->setTolerance(Tolerance.getValue());
    geometryObject->setScale(Scale.getValue());

    //TODO: remove these try/catch block when code is stable
    gp_Pnt inputCenter;
    try {
        inputCenter = TechDrawGeometry::findCentroid(shape,
                                                     Direction.getValue());
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
        geometryObject->setIsoCount(IsoCount.getValue());
        buildGeometryObject(mirroredShape,inputCenter);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e3 = Standard_Failure::Caught();
        Base::Console().Log("LOG - DVP::execute - buildGeometryObject failed for %s - %s **\n",getNameInDocument(),e3->GetMessageString());
        return new App::DocumentObjectExecReturn(e3->GetMessageString());
    }

#if MOD_TECHDRAW_HANDLE_FACES
    if (handleFaces()) {
        try {
            extractFaces();
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e4 = Standard_Failure::Caught();
            Base::Console().Log("LOG - DVP::execute - extractFaces failed for %s - %s **\n",getNameInDocument(),e4->GetMessageString());
            return new App::DocumentObjectExecReturn(e4->GetMessageString());
        }
    }
#endif //#if MOD_TECHDRAW_HANDLE_FACES

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
    //Base::Console().Message("TRACE - DVP::onChanged(%s) - %s\n",prop->getName(),Label.getValue());

    DrawView::onChanged(prop);

//TODO: when scale changes, any Dimensions for this View sb recalculated.  DVD should pick this up subject to topological naming issues.
}

void DrawViewPart::buildGeometryObject(TopoDS_Shape shape, gp_Pnt& inputCenter)
{
    Base::Vector3d baseProjDir = Direction.getValue();

    saveParamSpace(baseProjDir);

    geometryObject->projectShape(shape,
                            inputCenter,
                            Direction.getValue());
    geometryObject->extractGeometry(TechDrawGeometry::ecHARD,                   //always show the hard&outline visible lines
                                    true);
    geometryObject->extractGeometry(TechDrawGeometry::ecOUTLINE,
                                    true);
    if (SmoothVisible.getValue()) {
        geometryObject->extractGeometry(TechDrawGeometry::ecSMOOTH,
                                        true);
    }
    if (SeamVisible.getValue()) {
        geometryObject->extractGeometry(TechDrawGeometry::ecSEAM,
                                        true);
    }
    if ((IsoVisible.getValue()) && (IsoCount.getValue() > 0)) {
        geometryObject->extractGeometry(TechDrawGeometry::ecUVISO,
                                        true);
    }
    if (HardHidden.getValue()) {
        geometryObject->extractGeometry(TechDrawGeometry::ecHARD,
                                        false);
        geometryObject->extractGeometry(TechDrawGeometry::ecOUTLINE,
                                        false);
    }
    if (SmoothHidden.getValue()) {
        geometryObject->extractGeometry(TechDrawGeometry::ecSMOOTH,
                                        false);
    }
    if (SeamHidden.getValue()) {
        geometryObject->extractGeometry(TechDrawGeometry::ecSEAM,
                                        false);
    }
    if (IsoHidden.getValue() && (IsoCount.getValue() > 0)) {
        geometryObject->extractGeometry(TechDrawGeometry::ecUVISO,
                                        false);
    }
    bbox = geometryObject->calcBoundingBox();
}

//! make faces from the existing edge geometry
void DrawViewPart::extractFaces()
{
    geometryObject->clearFaceGeom();
    const std::vector<TechDrawGeometry::BaseGeom*>& goEdges = geometryObject->getVisibleFaceEdges();
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
    for (; itOuter != origEdges.end(); itOuter++, iOuter++) {
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
        for (; itInner != faceEdges.end(); itInner++,iInner++) {
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
            if (isOnEdge((*itInner),v1,param,false)) {
                gp_Pnt pnt1 = BRep_Tool::Pnt(v1);
                splitPoint s1;
                s1.i = iInner;
                s1.v = Base::Vector3d(pnt1.X(),pnt1.Y(),pnt1.Z());
                s1.param = param;
                splits.push_back(s1);
            }
            if (isOnEdge((*itInner),v2,param,false)) {
                gp_Pnt pnt2 = BRep_Tool::Pnt(v2);
                splitPoint s2;
                s2.i = iInner;
                s2.v = Base::Vector3d(pnt2.X(),pnt2.Y(),pnt2.Z());
                s2.param = param;
                splits.push_back(s2);
            }
        } //inner loop
    }   //outer loop

    std::vector<splitPoint> sorted = sortSplits(splits,true);
    auto last = std::unique(sorted.begin(), sorted.end(), DrawViewPart::splitEqual);  //duplicates to back
    sorted.erase(last, sorted.end());                         //remove dupls
    std::vector<TopoDS_Edge> newEdges = splitEdges(faceEdges,sorted);

    if (newEdges.empty()) {
        Base::Console().Log("LOG - DVP::extractFaces - no newEdges\n");
        return;
    }

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

//this routine is the big time consumer.  gets called many times (and is slow?))
//note param gets modified here
bool DrawViewPart::isOnEdge(TopoDS_Edge e, TopoDS_Vertex v, double& param, bool allowEnds)
{
    bool result = false;
    bool outOfBox = false;
    param = -2;

    //eliminate obvious cases
    Bnd_Box sBox;
    BRepBndLib::Add(e, sBox);
    sBox.SetGap(0.1);
    if (sBox.IsVoid()) {
        Base::Console().Message("DVP::isOnEdge - Bnd_Box is void for %s\n",getNameInDocument());
    } else {
        gp_Pnt pt = BRep_Tool::Pnt(v);
        if (sBox.IsOut(pt)) {
            outOfBox = true;
        }
    }
    if (!outOfBox) {
        if (m_interAlgo == 1) {
        //1) using projPointOnCurve.  roughly similar to dist to shape w/ bndbox.  hangs(?) w/o bndbox
            try {
                gp_Pnt pt = BRep_Tool::Pnt(v);
                BRepAdaptor_Curve adapt(e);
                Handle_Geom_Curve c = adapt.Curve().Curve();
                GeomAPI_ProjectPointOnCurve proj(pt,c);
                int n = proj.NbPoints();
                if (n > 0) {
                    if (proj.LowerDistance() < Precision::Confusion()) {
                        param = proj.LowerDistanceParameter();
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
                }
            }
            catch (Standard_Failure) {
                Handle_Standard_Failure e = Standard_Failure::Caught();     //no perp projection
            }
        } else if (m_interAlgo == 2) {                                      //can't provide param as is
            double dist = simpleMinDist(v,e);
            if (dist < 0.0) {
                Base::Console().Error("DVP::isOnEdge - simpleMinDist failed: %.3f\n",dist);
                result = false;
            } else if (dist < Precision::Confusion()) {
                const gp_Pnt pt = BRep_Tool::Pnt(v);                         //have to duplicate method 3 to get param
                BRepAdaptor_Curve adapt(e);
                const Handle_Geom_Curve c = adapt.Curve().Curve();
                double maxDist = 0.000001;     //magic number.  less than this gives false positives.
                //bool found =
                (void) GeomLib_Tool::Parameter(c,pt,maxDist,param);  //already know point it on curve
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
        } else if (m_interAlgo == 3) {
            const gp_Pnt pt = BRep_Tool::Pnt(v);
            BRepAdaptor_Curve adapt(e);
            const Handle_Geom_Curve c = adapt.Curve().Curve();
            double par = -1;
            double maxDist = 0.000001;     //magic number.  less than this gives false positives.
            bool found = GeomLib_Tool::Parameter(c,pt,maxDist,par);
            if (found) {
                result = true;
                param = par;
                TopoDS_Vertex v1 = TopExp::FirstVertex(e);
                TopoDS_Vertex v2 = TopExp::LastVertex(e);
                if (DrawUtil::isSamePoint(v,v1) || DrawUtil::isSamePoint(v,v2)) {
                    if (!allowEnds) {
                        result = false;
                    }
                }
            }
        }
    } //!outofbox
    return result;
}

std::vector<TopoDS_Edge> DrawViewPart::splitEdges(std::vector<TopoDS_Edge> edges, std::vector<splitPoint> splits)
{
    std::vector<TopoDS_Edge> result;
    std::vector<TopoDS_Edge> newEdges;
    std::vector<splitPoint> edgeSplits;      //splits for current edge
    int iEdge = 0; //current edge index
    int iSplit = 0; //current splitindex
    int ii = 0;     //i value of current split
    int endEdge = edges.size();
    int endSplit = splits.size();
    int imax = std::numeric_limits<int>::max();

    while ((iEdge < endEdge) )  {
        if (iSplit < endSplit) {
            ii = splits[iSplit].i;
        } else {
            ii = imax;
        }
        if (ii == iEdge) {
            edgeSplits.push_back(splits[iSplit]);
            iSplit++;
            continue;
        }

        if (ii > iEdge) {
            if (!edgeSplits.empty()) {                          //save *iedge's splits
                newEdges = split1Edge(edges[iEdge],edgeSplits);
                result.insert(result.end(), newEdges.begin(), newEdges.end());
                edgeSplits.clear();
            } else {
                result.push_back(edges[iEdge]);                //save *iedge
            }
            iEdge++;                                           //next edge
            continue;
        }

        if (iEdge > ii) {
            iSplit++;
            continue;
        }
    }
    if (!edgeSplits.empty()) {                                           //handle last batch
        newEdges = split1Edge(edges[iEdge],edgeSplits);
        result.insert(result.end(), newEdges.begin(), newEdges.end());
        edgeSplits.clear();
    }

    return result;
}

std::vector<TopoDS_Edge> DrawViewPart::split1Edge(TopoDS_Edge e, std::vector<splitPoint> splits)
{
    //Base::Console().Message("DVP::split1Edge - splits: %d\n",splits.size());
    std::vector<TopoDS_Edge> result;
    if (splits.empty()) {
        return result;
    }

    BRepAdaptor_Curve adapt(e);
    Handle_Geom_Curve c = adapt.Curve().Curve();
    double first = BRepLProp_CurveTool::FirstParameter(adapt);
    double last = BRepLProp_CurveTool::LastParameter(adapt);
    if (first > last) {
        //TODO parms.reverse();
        Base::Console().Message("DVP::split1Edge - edge is backwards!\n");
        return result;
    }
    std::vector<double> parms;
    parms.push_back(first);
    for (auto& s:splits) {
        parms.push_back(s.param);
    }

    parms.push_back(last);
    std::vector<double>::iterator pfirst = parms.begin();
    auto parms2 = parms.begin() + 1;
    std::vector<double>::iterator psecond = parms2;
    std::vector<double>::iterator pstop = parms.end();
    for (; psecond != pstop; pfirst++,psecond++) {
        try {
            BRepBuilderAPI_MakeEdge mkEdge(c, *pfirst, *psecond);
            if (mkEdge.IsDone()) {
                TopoDS_Edge e1 = mkEdge.Edge();
                result.push_back(e1);
            }
        }
        catch (Standard_Failure) {
            Base::Console().Message("LOG - DVP::split1Edge failed building edge segment\n");
        }
    }
    return result;
}

std::vector<splitPoint> DrawViewPart::sortSplits(std::vector<splitPoint>& s, bool ascend)
{
    std::vector<splitPoint> sorted = s;
    std::sort(sorted.begin(), sorted.end(), DrawViewPart::splitCompare);
    if (ascend) {
        std::reverse(sorted.begin(),sorted.end());
    }
    return sorted;
}

//return true if p1 "is greater than" p2
/*static*/bool DrawViewPart::splitCompare(const splitPoint& p1, const splitPoint& p2)
{
    bool result = false;
    if (p1.i > p2.i) {
        result = true;
    } else if (p1.i < p2.i) {
        result = false;
    } else if (p1.param > p2.param) {
        result = true;
    } else if (p1.param < p2.param) {
        result = false;
    }
    return result;
}

//return true if p1 "is equal to" p2
/*static*/bool DrawViewPart::splitEqual(const splitPoint& p1, const splitPoint& p2)
{
    bool result = false;
    if ((p1.i == p2.i) &&
        (fabs(p1.param - p2.param) < Precision::Confusion())) {
        result = true;
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
std::vector<TechDrawGeometry::BaseGeom*> DrawViewPart::getProjFaceByIndex(int /*idx*/) const
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
    gp_Ax2 viewAxis = TechDrawGeometry::getViewAxis(centeredPoint,direction);
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

void DrawViewPart::saveParamSpace(const Base::Vector3d& direction)
{
    Base::Vector3d origin(0.0,0.0,0.0);
    gp_Ax2 viewAxis = TechDrawGeometry::getViewAxis(origin,direction);

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
    return geometryObject->getVisibleFaceEdges();
}

void DrawViewPart::getRunControl()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/RunControl");
    m_interAlgo = hGrp->GetInt("InterAlgo", 2l);
    m_sectionEdges = hGrp->GetBool("ShowSectionEdges", 1l);
    m_handleFaces = hGrp->GetBool("HandleFaces", 1l);
//    Base::Console().Message("TRACE - DVP::getRunControl - interAlgo: %ld sectionFaces: %ld handleFaces: %ld\n",
//                             m_interAlgo,m_sectionEdges,m_handleFaces);
}

bool DrawViewPart::handleFaces(void)
{
    return m_handleFaces;
}

bool DrawViewPart::showSectionEdges(void)
{
    return m_sectionEdges;
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
