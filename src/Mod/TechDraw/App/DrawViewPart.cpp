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

#include <algorithm>

#include <HLRBRep_Algo.hxx>
#include <TopoDS_Shape.hxx>
#include <HLRTopoBRep_OutLiner.hxx>
//#include <BRepAPI_MakeOutLine.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_ShapeBounds.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
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
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>

#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Mod/Part/App/PartFeature.h>

#include "Geometry.h"
#include "DrawViewPart.h"
//#include "ProjectionAlgos.h"
#include "DrawHatch.h"
//#include "DrawViewDimension.h"

#include "DrawViewPartPy.h"  // generated from DrawViewPartPy.xml

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewPart
//===========================================================================

App::PropertyFloatConstraint::Constraints DrawViewPart::floatRange = {0.01f,5.0f,0.05f};

PROPERTY_SOURCE(TechDraw::DrawViewPart, TechDraw::DrawView)

DrawViewPart::DrawViewPart(void) : geometryObject(0)
{
    static const char *group = "Shape view";
    static const char *vgroup = "Drawing view";

    ADD_PROPERTY_TYPE(Direction ,(0,0,1.0)    ,group,App::Prop_None,"Projection normal direction");
    ADD_PROPERTY_TYPE(Source ,(0),group,App::Prop_None,"3D Shape to view");
    ADD_PROPERTY_TYPE(ShowHiddenLines ,(false),group,App::Prop_None,"Hidden lines on/off");
    ADD_PROPERTY_TYPE(ShowSmoothLines ,(false),group,App::Prop_None,"Smooth lines on/off");
    ADD_PROPERTY_TYPE(ShowSeamLines ,(false),group,App::Prop_None,"Seam lines on/off");
    //ADD_PROPERTY_TYPE(ShowIsoLines ,(false),group,App::Prop_None,"Iso u,v lines on/off");
    ADD_PROPERTY_TYPE(LineWidth,(0.7f),vgroup,App::Prop_None,"The thickness of visible lines");
    ADD_PROPERTY_TYPE(HiddenWidth,(0.15),vgroup,App::Prop_None,"The thickness of hidden lines, if enabled");
    ADD_PROPERTY_TYPE(Tolerance,(0.05f),vgroup,App::Prop_None,"The tessellation tolerance");
    Tolerance.setConstraints(&floatRange);
    ADD_PROPERTY_TYPE(XAxisDirection ,(1,0,0) ,group,App::Prop_None,"Direction to use as X-axis in projection");

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

    TopoDS_Shape shape = static_cast<Part::Feature*>(link)->Shape.getShape()._Shape;
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

    touch();

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
            prop == &ShowSeamLines  ||
            prop == &LineWidth       ||
            prop == &HiddenWidth     ) {
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
    std::vector<TopoDS_Edge> occEdges;
    for (;itEdge != goEdges.end(); itEdge++) {
        occEdges.push_back((*itEdge)->occEdge);
    }

    //almost works. :(
    std::vector<TopoDS_Wire> wires = connectEdges(occEdges);
    std::vector<TopoDS_Wire> sortedWires = sortWiresBySize(wires,true);        //smallest first

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

Base::BoundBox3d DrawViewPart::getBoundingBox() const
{
    return bbox;
}

//! build 1 or more wires from list of edges
//note disjoint edges won't be connected. have to be able to traverse all the edges
std::vector<TopoDS_Wire> DrawViewPart::connectEdges (std::vector<TopoDS_Edge>& edges)
{
    std::vector<TopoDS_Wire> result;
    Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
    Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
    std::vector<TopoDS_Edge>::const_iterator itEdge = edges.begin();
    for (; itEdge != edges.end(); itEdge++)
        hEdges->Append(*itEdge);

    //tolerance sb tolerance of DrawViewPart instead of Precision::Confusion()?
    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(hEdges, Precision::Confusion(), Standard_False, hWires);

    int len = hWires->Length();
    for(int i=1;i<=len;i++) {
        TopoDS_Wire w = TopoDS::Wire(hWires->Value(i));
        //if (BRep_Tool::IsClosed(w)) {
            result.push_back(w);
        //}
    }
    //delete hEdges;               //does Handle<> take care of this?
    //delete hWires;
    return result;
}

//! return true if w1 bbox is bigger than w2 bbox
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

//sort wires in descending order of bbox diagonal. if reversed, then ascending bbox diagonal
std::vector<TopoDS_Wire> DrawViewPart::sortWiresBySize(std::vector<TopoDS_Wire>& w, bool reverse)
{
    std::vector<TopoDS_Wire> wires = w;
    std::sort(wires.begin(), wires.end(), wireCompare());
    if (reverse) {
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
    Base::Vector3d xDir = XAxisDirection.getValue();
    if (xDir.Length() == 0) {
        Base::Console().Warning("XAxisDirection has zero length - using (1,0,0)\n");
        xDir = Base::Vector3d(1.0,0.0,0.0);
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
