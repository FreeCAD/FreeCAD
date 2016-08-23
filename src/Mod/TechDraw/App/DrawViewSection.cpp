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

#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pln.hxx>
#include <gp_Dir.hxx>
#include <Geom_Plane.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>

#endif


#include <App/Application.h>
#include <App/Material.h>
#include <Base/BoundBox.h>
#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/Part/App/PartFeature.h>

#include "Geometry.h"
#include "GeometryObject.h"
#include "EdgeWalker.h"
#include "DrawViewSection.h"

using namespace TechDraw;
using namespace std;

//===========================================================================
// DrawViewSection
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewSection, TechDraw::DrawViewPart)

DrawViewSection::DrawViewSection()
{
    static const char *group = "Section";

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("CutSurfaceColor", 0xC8C8C800));


    ADD_PROPERTY_TYPE(SectionNormal ,(0,0,1.0)    ,group,App::Prop_None,"Section Plane normal direction");
    ADD_PROPERTY_TYPE(SectionOrigin ,(0,0,0) ,group,App::Prop_None,"Section Plane Origin");
    ADD_PROPERTY_TYPE(ShowCutSurface ,(true),group,App::Prop_None,"Show the cut surface");
    ADD_PROPERTY_TYPE(CutSurfaceColor,(fcColor),group,App::Prop_None,"The color to shade the cut surface");

    geometryObject = new TechDrawGeometry::GeometryObject();
}

DrawViewSection::~DrawViewSection()
{
}

short DrawViewSection::mustExecute() const
{
    // If Tolerance Property is touched
    if(SectionNormal.isTouched() ||
       SectionOrigin.isTouched() ||
       ShowCutSurface.isTouched() ||
       CutSurfaceColor.isTouched() )
          return 1;

    return TechDraw::DrawViewPart::mustExecute();
}

App::DocumentObjectExecReturn *DrawViewSection::execute(void)
{
    //## Get the Part Link ##/
    App::DocumentObject* link = Source.getValue();

    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");

    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");

    const Part::TopoShape &partTopo = static_cast<Part::Feature*>(link)->Shape.getShape();

    if (partTopo.getShape().IsNull())
        return new App::DocumentObjectExecReturn("Linked shape object is empty");

    gp_Pln pln = getSectionPlane();
    // Get the Axis Directions for the Plane to transform UV components again
    gp_XYZ xAxis = pln.XAxis().Direction().XYZ();
    gp_XYZ yAxis = pln.YAxis().Direction().XYZ();
    gp_XYZ origin = pln.Location().XYZ();

    Base::BoundBox3d bb = partTopo.getBoundBox();

    Base::Vector3d tmp1 = SectionOrigin.getValue();
    Base::Vector3d plnPnt(tmp1.x, tmp1.y, tmp1.z);
    Base::Vector3d tmp2 = SectionNormal.getValue();
    Base::Vector3d plnNorm(tmp2.x, tmp2.y, tmp2.z);

//    if(!bb.IsCutPlane(plnPnt, plnNorm)) {      //this test doesn't work if plane is coincident with bb!
    if(!isReallyInBox(plnPnt, bb)) {
        Base::Console().Warning("DVS: Section Plane doesn't intersect part in %s\n",getNameInDocument());
        Base::Console().Warning("DVS: Using center of bounding box.\n");
        plnPnt = bb.GetCenter();
        SectionOrigin.setValue(plnPnt);
    }

    // Gather the corner points of bbox
    std::vector<Base::Vector3d> pnts;
    pnts.push_back(Base::Vector3d(bb.MinX,bb.MinY,bb.MinZ));
    pnts.push_back(Base::Vector3d(bb.MaxX,bb.MinY,bb.MinZ));
    pnts.push_back(Base::Vector3d(bb.MinX,bb.MaxY,bb.MinZ));
    pnts.push_back(Base::Vector3d(bb.MaxX,bb.MaxY,bb.MinZ));
    pnts.push_back(Base::Vector3d(bb.MinX,bb.MinY,bb.MaxZ));
    pnts.push_back(Base::Vector3d(bb.MaxX,bb.MinY,bb.MaxZ));
    pnts.push_back(Base::Vector3d(bb.MinX,bb.MaxY,bb.MaxZ));
    pnts.push_back(Base::Vector3d(bb.MaxX,bb.MaxY,bb.MaxZ));

    double uMax = 0, vMax = 0, wMax = 0., dMax = 0;
    for(std::vector<Base::Vector3d>::const_iterator it = pnts.begin(); it != pnts.end(); ++it) {
        // Project each bounding box point onto projection plane and find largest u,v,w values
        Base::Vector3d pnt = (*it);
        pnt.ProjectToPlane(plnPnt, plnNorm);
        uMax = std::max(uMax, std::abs(plnPnt.x - pnt.x));       //one will be zero
        vMax = std::max(vMax, std::abs(plnPnt.y - pnt.y));
        wMax = std::max(wMax, std::abs(plnPnt.z - pnt.z));

        //dMax is the bounding box point furthest away from plane. used for determining extrusion length
        double dist = (*it).DistanceToPlane(plnPnt, plnNorm);
        dMax = std::max(dMax, dist);
    }

    //use largest of u,v,w to make cutting face that covers whole shape
    double maxParm = std::max(uMax,vMax);
    maxParm = std::max(maxParm,wMax);
    BRepBuilderAPI_MakePolygon mkPoly;
    gp_Pnt pn1(origin + xAxis *  maxParm  + yAxis *  maxParm);
    gp_Pnt pn2(origin + xAxis *  maxParm  + yAxis * -maxParm);
    gp_Pnt pn3(origin + xAxis * -maxParm  + yAxis  * -maxParm);
    gp_Pnt pn4(origin + xAxis * -maxParm  + yAxis  * +maxParm);

    mkPoly.Add(pn1);
    mkPoly.Add(pn2);
    mkPoly.Add(pn3);
    mkPoly.Add(pn4);
    mkPoly.Close();

    // Make the extrusion face
    BRepBuilderAPI_MakeFace mkFace(mkPoly.Wire());
    TopoDS_Face aProjFace = mkFace.Face();
    if(aProjFace.IsNull())
        return new App::DocumentObjectExecReturn("DrawViewSection - Projected face is NULL");
    TopoDS_Shape prism = BRepPrimAPI_MakePrism(aProjFace, dMax * gp_Vec(pln.Axis().Direction()), false, true).Shape();

    // We need to copy the shape to not modify the BRepstructure
    BRepBuilderAPI_Copy BuilderCopy(partTopo.getShape());
    TopoDS_Shape myShape = BuilderCopy.Shape();

    BRepAlgoAPI_Cut mkCut(myShape, prism);
    if (!mkCut.IsDone())
        return new App::DocumentObjectExecReturn("Section cut has failed");

    TopoDS_Shape rawShape = mkCut.Shape();

    geometryObject->setTolerance(Tolerance.getValue());
    geometryObject->setScale(Scale.getValue());
    try {
        gp_Pnt inputCenter = TechDrawGeometry::findCentroid(rawShape,
                                                            Direction.getValue(),
                                                            getValidXDir());
        TopoDS_Shape mirroredShape = TechDrawGeometry::mirrorShape(rawShape,
                                                    inputCenter,
                                                    Scale.getValue());
        buildGeometryObject(mirroredShape,inputCenter);                         //this is original shape after cut by section prism
#if MOD_TECHDRAW_HANDLE_FACES
        extractFaces();
#endif //#if MOD_TECHDRAW_HANDLE_FACES

        TopoDS_Compound sectionCompound = findSectionPlaneIntersections(rawShape);
        TopoDS_Shape mirroredSection = TechDrawGeometry::mirrorShape(sectionCompound,
                                                                     inputCenter,
                                                                     Scale.getValue());
        TopoDS_Compound newFaces;
        BRep_Builder builder;
        builder.MakeCompound(newFaces);
        TopExp_Explorer expl(mirroredSection, TopAbs_FACE);
        for (; expl.More(); expl.Next()) {
            const TopoDS_Face& face = TopoDS::Face(expl.Current());
            TopoDS_Face pFace = projectFace(face,
                                            inputCenter,
                                            Direction.getValue(),
                                            getValidXDir());
             builder.Add(newFaces,pFace);
        }
        sectionFaces = newFaces;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e1 = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(std::string("DVS building Section shape failed: ") +
                                                 std::string(e1->GetMessageString()));
    }

    touch();
    return DrawView::execute();
}

gp_Pln DrawViewSection::getSectionPlane() const
{
    Base::Vector3d plnPnt = SectionOrigin.getValue();
    Base::Vector3d plnNorm = SectionNormal.getValue();

    return gp_Pln(gp_Pnt(plnPnt.x, plnPnt.y, plnPnt.z), gp_Dir(plnNorm.x, plnNorm.y, plnNorm.z));
}

//! tries to find the intersection of the section plane with the shape giving a collection of planar faces
TopoDS_Compound DrawViewSection::findSectionPlaneIntersections(const TopoDS_Shape& shape)
{
    TopoDS_Compound result;
    if(shape.IsNull()){
        Base::Console().Log("DrawViewSection::getSectionSurface - Sectional View shape is Empty\n");
        return result;
    }

    gp_Pln plnSection = getSectionPlane();
    BRep_Builder builder;
    builder.MakeCompound(result);

    TopExp_Explorer expFaces(shape, TopAbs_FACE);
    int i;
    for (i = 1 ; expFaces.More(); expFaces.Next(), i++) {
        const TopoDS_Face& face = TopoDS::Face(expFaces.Current());
        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() == GeomAbs_Plane){
            gp_Pln plnFace = adapt.Plane();

            if(plnSection.Contains(plnFace.Location(), Precision::Confusion()) &&
               plnFace.Axis().IsParallel(plnSection.Axis(), Precision::Angular())) {
                builder.Add(result, face);
            }
        }
    }
    return result;
}

//! get display geometry for Section faces
std::vector<TechDrawGeometry::Face*> DrawViewSection::getFaceGeometry()
{
    std::vector<TechDrawGeometry::Face*> result;
    TopoDS_Compound c = sectionFaces;
    TopExp_Explorer faces(c, TopAbs_FACE);
    for (; faces.More(); faces.Next()) {
        TechDrawGeometry::Face* f = new TechDrawGeometry::Face();
        const TopoDS_Face& face = TopoDS::Face(faces.Current());
        TopExp_Explorer wires(face, TopAbs_WIRE);
        for (; wires.More(); wires.Next()) {
            TechDrawGeometry::Wire* w = new TechDrawGeometry::Wire();
            const TopoDS_Wire& wire = TopoDS::Wire(wires.Current());
            TopExp_Explorer edges(wire, TopAbs_EDGE);
            for (; edges.More(); edges.Next()) {
                const TopoDS_Edge& edge = TopoDS::Edge(edges.Current());
                //dumpEdge("edge",edgeCount,edge);
                TechDrawGeometry::BaseGeom* base = TechDrawGeometry::BaseGeom::baseFactory(edge);
                w->geoms.push_back(base);
            }
            f->wires.push_back(w);
        }
        result.push_back(f);
    }
    return result;
}

//! project a single face using HLR
TopoDS_Face DrawViewSection::projectFace(const TopoDS_Shape &face,
                                     gp_Pnt faceCenter,
                                     const Base::Vector3d &direction,
                                     const Base::Vector3d &xaxis)
{
    if(face.IsNull()) {
        throw Base::Exception("DrawViewSection::projectFace - input Face is NULL");
    }

    gp_Ax2 transform;
    transform = gp_Ax2(faceCenter,
                       gp_Dir(direction.x, direction.y, direction.z),
                       gp_Dir(xaxis.x, xaxis.y, xaxis.z));

    HLRBRep_Algo *brep_hlr = new HLRBRep_Algo();
    brep_hlr->Add(face);

    HLRAlgo_Projector projector( transform );
    brep_hlr->Projector(projector);
    brep_hlr->Update();
    brep_hlr->Hide();

    HLRBRep_HLRToShape hlrToShape(brep_hlr);
    TopoDS_Shape hardEdges = hlrToShape.VCompound();
    TopoDS_Shape outEdges = hlrToShape.OutLineVCompound();
    std::vector<TopoDS_Edge> faceEdges;
    TopExp_Explorer expl(hardEdges, TopAbs_EDGE);
    int i;
    for (i = 1 ; expl.More(); expl.Next(),i++) {
        TopoDS_Edge edge = TopoDS::Edge(expl.Current());
        if (edge.IsNull()) {
            Base::Console().Log("INFO - GO::projectFace - hard edge: %d is NULL\n",i);
            continue;
        }
        faceEdges.push_back(edge);
    }
    expl.Init(outEdges, TopAbs_EDGE);
    for (i = 1 ; expl.More(); expl.Next(),i++) {
        TopoDS_Edge edge = TopoDS::Edge(expl.Current());
        if (edge.IsNull()) {
            Base::Console().Log("INFO - GO::projectFace - outline edge: %d is NULL\n",i);
            continue;
        }
        faceEdges.push_back(edge);
    }

    std::vector<TopoDS_Vertex> uniqueVert = makeUniqueVList(faceEdges);
    std::vector<WalkerEdge> walkerEdges = makeWalkerEdges(faceEdges,uniqueVert);

    EdgeWalker ew;
    ew.setSize(uniqueVert.size());
    ew.loadEdges(walkerEdges);
    ew.perform();
    facelist result = ew.getResult();         //probably two Faces most of the time. Outerwire + real wires

    facelist::iterator iFace = result.begin();
    std::vector<TopoDS_Wire> fw;
    for (;iFace != result.end(); iFace++) {
        edgelist::iterator iEdge = (*iFace).begin();
        std::vector<TopoDS_Edge> fe;
        for (;iEdge != (*iFace).end(); iEdge++) {
            fe.push_back(faceEdges.at((*iEdge).idx));
        }
    TopoDS_Wire w = makeCleanWire(fe);                 //make 1 clean wire from its edges
    fw.push_back(w);
    }
    TopoDS_Face projectedFace;
    if (!fw.empty()) {
        std::vector<TopoDS_Wire> sortedWires = sortWiresBySize(fw);
        if (sortedWires.empty()) {
            return projectedFace;
        }

        //TODO: this really should have the same size checking logic as DVP
        //remove the largest wire (OuterWire of graph)
        sortedWires.erase(sortedWires.begin());

        BRepBuilderAPI_MakeFace mkFace(sortedWires.front(),true);                   //true => only want planes?
        std::vector<TopoDS_Wire>::iterator itWire = ++sortedWires.begin();          //starting with second face
        for (; itWire != sortedWires.end(); itWire++) {
            mkFace.Add(*itWire);
        }
        projectedFace = mkFace.Face();
    }
    return projectedFace;
}

//this should really be in BoundBox.h
bool DrawViewSection::isReallyInBox (const Base::Vector3d v, const Base::BoundBox3d bb) const
{
    if (v.x <= bb.MinX || v.x >= bb.MaxX)
        return false;
    if (v.y <= bb.MinY || v.y >= bb.MaxY)
        return false;
    if (v.z <= bb.MinZ || v.z >= bb.MaxZ)
        return false;
    return true;
}


// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewSectionPython, TechDraw::DrawViewSection)
template<> const char* TechDraw::DrawViewSectionPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderDrawingView";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewSection>;
}
