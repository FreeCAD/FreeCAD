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
#endif

#include <HLRBRep_Algo.hxx>
#include <TopoDS_Shape.hxx>
#include <HLRTopoBRep_OutLiner.hxx>

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>

//#include <BRepAPI_MakeOutLine.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_ShapeBounds.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pln.hxx>
#include <gp_Dir.hxx>
#include <Geom_Plane.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepMesh.hxx>

#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepPrim_FaceBuilder.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <boost/concept_check.hpp>

#include <Base/BoundBox.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Console.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/Geometry.h>

#include "DrawViewSection.h"
//#include "ProjectionAlgos.h"

using namespace TechDraw;
using namespace std;

//===========================================================================
// DrawViewSection
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewSection, TechDraw::DrawViewPart)

DrawViewSection::DrawViewSection()
{
    static const char *group = "Shape view";

    ADD_PROPERTY_TYPE(SectionNormal ,(0,0,1.0)    ,group,App::Prop_None,"Section Plane normal direction");
    ADD_PROPERTY_TYPE(SectionOrigin ,(0,0,0) ,group,App::Prop_None,"Section Plane Origin");
    ADD_PROPERTY_TYPE(ShowCutSurface ,(true),group,App::Prop_None,"Show the cut surface");

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
       ShowCutSurface.isTouched())
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

    if (partTopo._Shape.IsNull())
        return new App::DocumentObjectExecReturn("Linked shape object is empty");

    gp_Pln pln = getSectionPlane();
    // Get the Axis Directions for the Plane to transform UV components again
    gp_XYZ xAxis = pln.XAxis().Direction().XYZ();
    gp_XYZ yAxis = pln.YAxis().Direction().XYZ();
    gp_XYZ origin = pln.Location().XYZ();

    Base::BoundBox3d bb = partTopo.getBoundBox();

    Base::Vector3d tmp1 = SectionOrigin.getValue();
    Base::Vector3d tmp2 = SectionNormal.getValue();

    Base::Vector3d plnPnt(tmp1.x, tmp1.y, tmp1.z);
    Base::Vector3d plnNorm(tmp2.x, tmp2.y, tmp2.z);

    if(!bb.IsCutPlane(plnPnt, plnNorm)) {
        return new App::DocumentObjectExecReturn("Section Plane doesn't intersect part");
    }

    //bb.Enlarge(1.0); // Enlarge the bounding box to prevent any clipping

    // Gather the points
    std::vector<Base::Vector3d> pnts;

    pnts.push_back(Base::Vector3d(bb.MinX,bb.MinY,bb.MinZ));
    pnts.push_back(Base::Vector3d(bb.MaxX,bb.MinY,bb.MinZ));
    pnts.push_back(Base::Vector3d(bb.MinX,bb.MaxY,bb.MinZ));
    pnts.push_back(Base::Vector3d(bb.MaxX,bb.MaxY,bb.MinZ));
    pnts.push_back(Base::Vector3d(bb.MinX,bb.MinY,bb.MaxZ));
    pnts.push_back(Base::Vector3d(bb.MaxX,bb.MinY,bb.MaxZ));
    pnts.push_back(Base::Vector3d(bb.MinX,bb.MaxY,bb.MaxZ));
    pnts.push_back(Base::Vector3d(bb.MaxX,bb.MaxY,bb.MaxZ));

    double uMax = 0, vMax = 0, wMax;
    for(std::vector<Base::Vector3d>::const_iterator it = pnts.begin(); it != pnts.end(); ++it) {
        // Project each bounding box point onto projection plane and find larges u,v values

        Base::Vector3d pnt = (*it);
        pnt.ProjToPlane(plnPnt, plnNorm);

        uMax = std::max(uMax, std::abs(plnPnt[0] - pnt[0]));
        vMax = std::max(vMax, std::abs(plnPnt[1] - pnt[1]));

        //wMax is the bounding box point furthest away used for determining extrusion length
        double dist = (*it).DistanceToPlane(plnPnt, plnNorm);
        wMax = std::max(wMax, dist);
    }

    // Build face directly onto plane
    BRepBuilderAPI_MakePolygon mkPoly;
    gp_Pnt pn1(origin + xAxis *  uMax  + yAxis *  vMax);
    gp_Pnt pn2(origin + xAxis *  uMax  + yAxis * -vMax);
    gp_Pnt pn3(origin + xAxis * -uMax  + yAxis  * -vMax);
    gp_Pnt pn4(origin + xAxis * -uMax  + yAxis  * +vMax);
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
    // Create an infinite projection (investigate if infite extrusion necessary)
//     BRepPrimAPI_MakePrism PrismMaker(from, Ltotal*gp_Vec(dir), 0,1); // finite prism
    TopoDS_Shape prism = BRepPrimAPI_MakePrism(aProjFace, wMax * gp_Vec(pln.Axis().Direction()), 0, 1).Shape();

    // We need to copy the shape to not modify the BRepstructure
    BRepBuilderAPI_Copy BuilderCopy(partTopo._Shape);
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
        buildGeometryObject(mirroredShape,inputCenter);

        TopoDS_Compound sectionCompound = findSectionPlaneIntersections(rawShape);
        TopoDS_Shape mirroredSection = TechDrawGeometry::mirrorShape(sectionCompound,
                                                                     inputCenter,
                                                                     Scale.getValue());
        TopoDS_Compound newFaces;
        BRep_Builder builder;
        builder.MakeCompound(newFaces);
        TopExp_Explorer expl(mirroredSection, TopAbs_FACE);
        for (int i = 1 ; expl.More(); expl.Next(),i++) {
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
        Base::Console().Log("DrawViewSection::execute - building Section shape failed: %s\n",e1->GetMessageString());
        return new App::DocumentObjectExecReturn(e1->GetMessageString());
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
    //TopoDS_Compound c = getSectionFaces();    //get projected section faces?
    TopoDS_Compound c = sectionFaces;
    //for face in c
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
        return TopoDS_Face();
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
        const TopoDS_Edge& edge = TopoDS::Edge(expl.Current());
        if (edge.IsNull()) {
            Base::Console().Log("INFO - GO::addGeomFromCompound - hard edge: %d is NULL\n",i);
            continue;
        }
        faceEdges.push_back(edge);
    }
    expl.Init(outEdges, TopAbs_EDGE);
    for (i = 1 ; expl.More(); expl.Next(),i++) {
        const TopoDS_Edge& edge = TopoDS::Edge(expl.Current());
        if (edge.IsNull()) {
            Base::Console().Log("INFO - GO::addGeomFromCompound - outline edge: %d is NULL\n",i);
            continue;
        }
        faceEdges.push_back(edge);
    }

    //no guarantee HLR gives edges in any particular order, so have to build back into a face.
    TopoDS_Face projectedFace;
    std::vector<TopoDS_Wire> faceWires = DrawViewSection::connectEdges(faceEdges);
    if (!faceWires.empty()) {
        std::vector<TopoDS_Wire> sortedWires = sortWiresBySize(faceWires);
        if (sortedWires.empty()) {
            return projectedFace;
        }
        BRepBuilderAPI_MakeFace mkFace(sortedWires.front(),true);                 //true => only want planes?
        std::vector<TopoDS_Wire>::iterator itWire = (sortedWires.begin()) + 1;    //starting with second face
        for (; itWire != sortedWires.end(); itWire++) {
            mkFace.Add(*itWire);
        }
        projectedFace = mkFace.Face();
    }
    return projectedFace;
}

//! connect edges into 1 or more wires
std::vector<TopoDS_Wire> DrawViewSection::connectEdges (std::vector<TopoDS_Edge>& edges)
{
    std::vector<TopoDS_Wire> result;
    Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
    Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
    std::vector<TopoDS_Edge>::const_iterator itEdge = edges.begin();
    for (; itEdge != edges.end(); itEdge++)
        hEdges->Append(*itEdge);

    //tolerance sb tolerance of DrawViewSection instead of Precision::Confusion()?
    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(hEdges, Precision::Confusion(), Standard_False, hWires);

    int len = hWires->Length();
    for(int i=1;i<=len;i++) {
        result.push_back(TopoDS::Wire(hWires->Value(i)));
    }
    //delete hEdges;               //does Handle<> take care of this?
    //delete hWires;
    return result;
}

//! return true if w1 bbox is bigger than w2 bbox
class DrawViewSection::wireCompare: public std::binary_function<const TopoDS_Wire&,
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

//sort wires in descending order of size (bbox diagonal)
std::vector<TopoDS_Wire> DrawViewSection::sortWiresBySize(std::vector<TopoDS_Wire>& w)
{
    std::vector<TopoDS_Wire> wires = w;
    std::sort(wires.begin(), wires.end(), wireCompare());
    return wires;
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
