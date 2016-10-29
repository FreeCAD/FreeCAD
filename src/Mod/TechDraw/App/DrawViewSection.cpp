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
#include <BRepBndLib.hxx>
//#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepAdaptor_Surface.hxx>
# include <BRep_Builder.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
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
#include <TopoDS_Compound.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>

#endif

#include <chrono>

# include <QFile>
# include <QFileInfo>

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

const char* DrawViewSection::SectionDirEnums[]= {"Right",
                                            "Left",
                                            "Up",
                                            "Down",
                                             NULL};



//===========================================================================
// DrawViewSection
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewSection, TechDraw::DrawViewPart)

DrawViewSection::DrawViewSection()
{
    static const char *sgroup = "Section";
    static const char *fgroup = "Format";
    //static const char *lgroup = "Line";


    ADD_PROPERTY_TYPE(SectionSymbol ,("A"),sgroup,App::Prop_None,"The identifier for this section");
    ADD_PROPERTY_TYPE(BaseView ,(0),sgroup,App::Prop_None,"2D View source for this Section");
    ADD_PROPERTY_TYPE(SectionNormal ,(0,0,1.0) ,sgroup,App::Prop_None,"Section Plane normal direction");  //direction of extrusion of cutting prism
    ADD_PROPERTY_TYPE(SectionOrigin ,(0,0,0) ,sgroup,App::Prop_None,"Section Plane Origin");
    SectionDirection.setEnums(SectionDirEnums);
    ADD_PROPERTY_TYPE(SectionDirection,((long)0),sgroup, App::Prop_None, "Direction in Base View for this Section");

    ADD_PROPERTY_TYPE(ShowCutSurface ,(true),fgroup,App::Prop_None,"Shade the cut surface");
    ADD_PROPERTY_TYPE(CutSurfaceColor,(0.0,0.0,0.0),fgroup,App::Prop_None,"The color to shade the cut surface");
    ADD_PROPERTY_TYPE(HatchCutSurface ,(false),fgroup,App::Prop_None,"Hatch the cut surface");
    ADD_PROPERTY_TYPE(HatchPattern ,(""),fgroup,App::Prop_None,"The hatch pattern file for the cut surface");
    ADD_PROPERTY_TYPE(HatchColor,(0.0,0.0,0.0),fgroup,App::Prop_None,"The color of the hatch pattern");

    getParameters();

}

DrawViewSection::~DrawViewSection()
{
}

short DrawViewSection::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  = (Scale.isTouched() ||
                   Direction.isTouched()     ||
                   BaseView.isTouched()  ||
                   SectionNormal.isTouched() ||
                   SectionOrigin.isTouched() );
    }
    if (result) {
        return result;
    }
    return TechDraw::DrawView::mustExecute();
}

void DrawViewSection::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        //Base::Console().Message("TRACE - DVS::onChanged(%s) - %s\n",prop->getName(),Label.getValue());
        if (prop == &SectionSymbol) {
            std::string lblText = "Section " +
                                  std::string(SectionSymbol.getValue()) +
                                  " - " +
                                  std::string(SectionSymbol.getValue());
            Label.setValue(lblText);
        }
        if (prop == &SectionOrigin) {
            App::DocumentObject* base = BaseView.getValue();
            if (base != nullptr) {
                base->touch();
            }
        }
    }
    DrawView::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawViewSection::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    App::DocumentObject* base = BaseView.getValue();
    if (!link || !base)  {
        Base::Console().Log("INFO - DVS::execute - No Source or Link - creation?\n");
        return DrawView::execute();
    }

    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Source object is not a Part object");
    if (!base->getTypeId().isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()))
        return new App::DocumentObjectExecReturn("BaseView object is not a DrawViewPart object");

    //Base::Console().Message("TRACE - DVS::execute() - %s/%s\n",getNameInDocument(),Label.getValue());

    const Part::TopoShape &partTopo = static_cast<Part::Feature*>(link)->Shape.getShape();

    if (partTopo.getShape().IsNull())
        return new App::DocumentObjectExecReturn("Linked shape object is empty");

    (void) DrawView::execute();          //make sure Scale is up to date

    gp_Pln pln = getSectionPlane();
    gp_Dir gpNormal = pln.Axis().Direction();
    Base::Vector3d orgPnt = SectionOrigin.getValue();

    Base::BoundBox3d bb = partTopo.getBoundBox();
    if(!isReallyInBox(orgPnt, bb)) {
        Base::Console().Warning("DVS: Section Plane doesn't intersect part in %s\n",getNameInDocument());
        Base::Console().Warning("DVS: Using center of bounding box.\n");
        orgPnt = bb.GetCenter();
        SectionOrigin.setValue(orgPnt);
    }

    // Make the extrusion face
    double dMax = bb.CalcDiagonalLength();
    BRepBuilderAPI_MakeFace mkFace(pln, -dMax,dMax,-dMax,dMax);
    TopoDS_Face aProjFace = mkFace.Face();
    if(aProjFace.IsNull())
        return new App::DocumentObjectExecReturn("DrawViewSection - Projected face is NULL");
    gp_Vec extrudeDir = dMax * gp_Vec(gpNormal);
    TopoDS_Shape prism = BRepPrimAPI_MakePrism(aProjFace, extrudeDir, false, true).Shape();

    // We need to copy the shape to not modify the BRepstructure
    BRepBuilderAPI_Copy BuilderCopy(partTopo.getShape());
    TopoDS_Shape myShape = BuilderCopy.Shape();

    BRepAlgoAPI_Cut mkCut(myShape, prism);
    if (!mkCut.IsDone())
        return new App::DocumentObjectExecReturn("Section cut has failed");

    TopoDS_Shape rawShape = mkCut.Shape();
    Bnd_Box testBox;
    BRepBndLib::Add(rawShape, testBox);
    testBox.SetGap(0.0);
    if (testBox.IsVoid()) {                        //prism & input don't intersect.  rawShape is garbage, don't bother.
        Base::Console().Log("INFO - DVS::execute - prism & input don't intersect\n");
        return DrawView::execute();
    }

    geometryObject->setTolerance(Tolerance.getValue());
    geometryObject->setScale(Scale.getValue());

    gp_Pnt inputCenter;
    try {
        inputCenter = TechDrawGeometry::findCentroid(rawShape,
                                                     Direction.getValue());
        TopoDS_Shape mirroredShape = TechDrawGeometry::mirrorShape(rawShape,
                                                    inputCenter,
                                                    Scale.getValue());
        buildGeometryObject(mirroredShape,inputCenter);                         //this is original shape after cut by section prism

#if MOD_TECHDRAW_HANDLE_FACES
        extractFaces();
#endif //#if MOD_TECHDRAW_HANDLE_FACES
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e1 = Standard_Failure::Caught();
        Base::Console().Log("LOG - DVS::execute - base shape failed for %s - %s **\n",getNameInDocument(),e1->GetMessageString());
        return new App::DocumentObjectExecReturn(e1->GetMessageString());
    }

    try {
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
                                            Direction.getValue());
             if (!pFace.IsNull()) {
                 builder.Add(newFaces,pFace);
             }

        }
        sectionFaces = newFaces;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e2 = Standard_Failure::Caught();
        Base::Console().Log("LOG - DVS::execute - failed building section faces for %s - %s **\n",getNameInDocument(),e2->GetMessageString());
        return new App::DocumentObjectExecReturn(e2->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

gp_Pln DrawViewSection::getSectionPlane() const
{
    Base::Vector3d plnPnt = SectionOrigin.getValue();
    Base::Vector3d plnNorm = SectionNormal.getValue();
    gp_Ax2 viewAxis = TechDrawGeometry::getViewAxis(plnPnt,plnNorm,false);
    gp_Ax3 viewAxis3(viewAxis);

    return gp_Pln(viewAxis3);
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
    int dbAdded = 0;
    for (i = 1 ; expFaces.More(); expFaces.Next(), i++) {
        const TopoDS_Face& face = TopoDS::Face(expFaces.Current());
        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() == GeomAbs_Plane){
            gp_Pln plnFace = adapt.Plane();

            if(plnSection.Contains(plnFace.Location(), Precision::Confusion()) &&
               plnFace.Axis().IsParallel(plnSection.Axis(), Precision::Angular())) {
                dbAdded++;
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

//! project a single face using HLR - used for section faces
TopoDS_Face DrawViewSection::projectFace(const TopoDS_Shape &face,
                                     gp_Pnt faceCenter,
                                     const Base::Vector3d &direction)
{
    if(face.IsNull()) {
        throw Base::Exception("DrawViewSection::projectFace - input Face is NULL");
    }

    Base::Vector3d origin(faceCenter.X(),faceCenter.Y(),faceCenter.Z());
    gp_Ax2 viewAxis = TechDrawGeometry::getViewAxis(origin,direction);

    HLRBRep_Algo *brep_hlr = new HLRBRep_Algo();
    brep_hlr->Add(face);
    HLRAlgo_Projector projector( viewAxis );
    brep_hlr->Projector(projector);
    brep_hlr->Update();
    brep_hlr->Hide();

    HLRBRep_HLRToShape hlrToShape(brep_hlr);
    TopoDS_Shape hardEdges = hlrToShape.VCompound();
//    TopoDS_Shape outEdges = hlrToShape.OutLineVCompound();
    std::vector<TopoDS_Edge> faceEdges;
    TopExp_Explorer expl(hardEdges, TopAbs_EDGE);
    int i;
    for (i = 1 ; expl.More(); expl.Next(),i++) {
        const TopoDS_Edge& edge = TopoDS::Edge(expl.Current());
        if (edge.IsNull()) {
            Base::Console().Log("INFO - DVS::projectFace - hard edge: %d is NULL\n",i);
            continue;
        }
        faceEdges.push_back(edge);
    }
    //TODO: verify that outline edges aren't required
    //if edge is both hard & outline, it will be duplicated? are hard edges enough?
//    TopExp_Explorer expl2(outEdges, TopAbs_EDGE);
//    for (i = 1 ; expl2.More(); expl2.Next(),i++) {
//        const TopoDS_Edge& edge = TopoDS::Edge(expl2.Current());
//        if (edge.IsNull()) {
//            Base::Console().Log("INFO - GO::projectFace - outline edge: %d is NULL\n",i);
//            continue;
//        }
//        bool addEdge = true;
//        //is edge already in faceEdges?  maybe need to use explorer for this for IsSame to work?
//        for (auto& e:faceEdges) {
//            if (e.IsPartner(edge)) {
//                addEdge = false;
//                Base::Console().Message("TRACE - DVS::projectFace - skipping an edge 1\n");
//            }
//        }
//        expl.ReInit();
//        for (; expl.More(); expl.Next()){
//            const TopoDS_Edge& eHard = TopoDS::Edge(expl.Current());
//            if (eHard.IsPartner(edge)) {
//                addEdge = false;
//                Base::Console().Message("TRACE - DVS::projectFace - skipping an edge 2\n");
//            }
//        }
//        if (addEdge) {
//            faceEdges.push_back(edge);
//        }
//    }

    TopoDS_Face projectedFace;

    if (faceEdges.empty()) {
        Base::Console().Log("LOG - DVS::projectFace - no faceEdges\n");
        return projectedFace;
    }


//recreate the wires for this single face
    EdgeWalker ew;
    ew.loadEdges(faceEdges);
    bool success = ew.perform();
    if (success) {
        std::vector<TopoDS_Wire> fw = ew.getResultNoDups();

        if (!fw.empty()) {
            std::vector<TopoDS_Wire> sortedWires = ew.sortStrip(fw, true);
            if (sortedWires.empty()) {
                return projectedFace;
            }

            BRepBuilderAPI_MakeFace mkFace(sortedWires.front(),true);                   //true => only want planes?
            std::vector<TopoDS_Wire>::iterator itWire = ++sortedWires.begin();          //starting with second face
            for (; itWire != sortedWires.end(); itWire++) {
                mkFace.Add(*itWire);
            }
            projectedFace = mkFace.Face();
        }
    } else {
        Base::Console().Warning("DVS::projectFace - input is not planar graph. No face detection\n");
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

void DrawViewSection::getParameters()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color cutColor = App::Color((uint32_t) hGrp->GetUnsigned("CutSurfaceColor", 0xC8C8C800));
    CutSurfaceColor.setValue(cutColor);
    App::Color hatchColor = App::Color((uint32_t) hGrp->GetUnsigned("SectionHatchColor", 0x00000000));
    HatchColor.setValue(hatchColor);

    hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/Drawing/patterns/";
    std::string defaultFileName = defaultDir + "simple.svg";
    QString patternFileName = QString::fromStdString(hGrp->GetASCII("PatternFile",defaultFileName.c_str()));
    if (patternFileName.isEmpty()) {
        patternFileName = QString::fromStdString(defaultFileName);
    }
    QFileInfo tfi(patternFileName);
        if (tfi.isReadable()) {
            HatchPattern.setValue(patternFileName.toUtf8().constData());
        }

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
