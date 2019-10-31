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
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pln.hxx>
#include <gp_Dir.hxx>
#include <Geom_Plane.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <ShapeAnalysis.hxx>
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
#include <App/Document.h>
#include <App/Material.h>
#include <Base/BoundBox.h>
#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>

#include <Mod/Part/App/PartFeature.h>

#include "Geometry.h"
#include "GeometryObject.h"
#include "Cosmetic.h"
#include "HatchLine.h"
#include "EdgeWalker.h"
#include "DrawUtil.h"
#include "DrawProjGroupItem.h"
#include "DrawProjectSplit.h"
#include "DrawGeomHatch.h"
#include "DrawViewSection.h"

using namespace TechDraw;
using namespace std;

const char* DrawViewSection::SectionDirEnums[]= {"Right",
                                            "Left",
                                            "Up",
                                            "Down",
                                             NULL};

const char* DrawViewSection::CutSurfaceEnums[]= {"Hide",
                                            "Color",
                                            "SvgHatch",
                                            "PatHatch",
                                             NULL};


//===========================================================================
// DrawViewSection
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewSection, TechDraw::DrawViewPart)

DrawViewSection::DrawViewSection()
{
    static const char *sgroup = "Section";
    static const char *fgroup = "Cut Surface Format";

    ADD_PROPERTY_TYPE(SectionSymbol ,("A"),sgroup,App::Prop_None,"The identifier for this section");
    ADD_PROPERTY_TYPE(BaseView ,(0),sgroup,App::Prop_None,"2D View source for this Section");
    BaseView.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(SectionNormal ,(0,0,1.0) ,sgroup,App::Prop_None,"Section Plane normal direction");  //direction of extrusion of cutting prism
    ADD_PROPERTY_TYPE(SectionOrigin ,(0,0,0) ,sgroup,App::Prop_None,"Section Plane Origin");
    SectionDirection.setEnums(SectionDirEnums);
    ADD_PROPERTY_TYPE(SectionDirection,((long)0),sgroup, App::Prop_None, "Direction in Base View for this Section");
    ADD_PROPERTY_TYPE(FuseBeforeCut ,(false),sgroup,App::Prop_None,"Merge Source(s) into a single shape before cutting");

    CutSurfaceDisplay.setEnums(CutSurfaceEnums);
    ADD_PROPERTY_TYPE(CutSurfaceDisplay,((long)2),fgroup, App::Prop_None, "Appearance of Cut Surface");
    ADD_PROPERTY_TYPE(FileHatchPattern ,(""),fgroup,App::Prop_None,"The hatch pattern file for the cut surface");
    ADD_PROPERTY_TYPE(FileGeomPattern ,(""),fgroup,App::Prop_None,"The PAT pattern file for geometric hatching");
    ADD_PROPERTY_TYPE(NameGeomPattern ,(""),fgroup,App::Prop_None,"The pattern name for geometric hatching");
    ADD_PROPERTY_TYPE(HatchScale,(1.0),fgroup,App::Prop_None,"Hatch pattern size adjustment");

    getParameters();

    std::string hatchFilter("Svg files (*.svg *.SVG);;All files (*)");
    FileHatchPattern.setFilter(hatchFilter);
    hatchFilter = ("PAT files (*.pat *.PAT);;All files (*)");
    FileGeomPattern.setFilter(hatchFilter);
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
        if (prop == &SectionSymbol) {
            std::string lblText = "Section " +
                                  std::string(SectionSymbol.getValue()) +
                                  " - " +
                                  std::string(SectionSymbol.getValue());
            Label.setValue(lblText);
        } else if (prop == &SectionOrigin) {
            App::DocumentObject* base = BaseView.getValue();
            TechDraw::DrawView* dv = dynamic_cast<TechDraw::DrawView*>(base);
            if (dv != nullptr) {
                dv->requestPaint();
            }
        } else if (prop == &CutSurfaceDisplay) {
//            Base::Console().Message("DVS::onChanged(%s)\n",prop->getName());
        }
    }
    if (prop == &FileGeomPattern    ||
        prop == &NameGeomPattern ) {
        std::string fileSpec = FileGeomPattern.getValue();
        Base::FileInfo fi(fileSpec);
        std::string ext = fi.extension();
        if ( (ext == "pat") ||
             (ext == "PAT") ) {
            if ((!FileGeomPattern.isEmpty())  &&
                (!NameGeomPattern.isEmpty())) {
                std::vector<PATLineSpec> specs = 
                           DrawGeomHatch::getDecodedSpecsFromFile(FileGeomPattern.getValue(),
                                                                  NameGeomPattern.getValue());
                m_lineSets.clear();
                for (auto& hl: specs) {
                    //hl.dump("hl from section");
                    LineSet ls;
                    ls.setPATLineSpec(hl);
                    m_lineSets.push_back(ls);
                }
            }
        }
    }

    DrawView::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawViewSection::execute(void)
{
//    Base::Console().Message("DVS::execute() - %s \n", getNameInDocument());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    App::DocumentObject* base = BaseView.getValue();
    if (base == nullptr) {
        return new App::DocumentObjectExecReturn("BaseView object not found");
    }

    TechDraw::DrawViewPart* dvp = nullptr;
    if (!base->getTypeId().isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        return new App::DocumentObjectExecReturn("BaseView object is not a DrawViewPart object");
    } else {
        dvp = static_cast<TechDraw::DrawViewPart*>(base);
    }
    
    TopoDS_Shape baseShape;
    if (FuseBeforeCut.getValue()) {
        baseShape = dvp->getSourceShapeFused();
    } else {
        baseShape = dvp->getSourceShape();
    }
    
    if (baseShape.IsNull()) {
        bool isRestoring = getDocument()->testStatus(App::Document::Status::Restoring);
        if (isRestoring) {
            Base::Console().Warning("DVS::execute - base shape is invalid - (but document is restoring) - %s\n",
                                getNameInDocument());
        } else {
            Base::Console().Error("Error: DVS::execute - base shape is Null. - %s\n",
                                  getNameInDocument());
        }
        return new App::DocumentObjectExecReturn("BaseView Source object is Null");
    }

    //is SectionOrigin valid?
    Bnd_Box centerBox;
    BRepBndLib::Add(baseShape, centerBox);
    centerBox.SetGap(0.0);

    gp_Pln pln = getSectionPlane();
    gp_Dir gpNormal = pln.Axis().Direction();
    Base::Vector3d orgPnt = SectionOrigin.getValue();

    if(!isReallyInBox(gp_Pnt(orgPnt.x,orgPnt.y,orgPnt.z), centerBox)) {
        Base::Console().Warning("DVS: SectionOrigin doesn't intersect part in %s\n",getNameInDocument());
        Base::Console().Warning("DVS: Using center of bounding box.\n");
        double Xmin,Ymin,Zmin,Xmax,Ymax,Zmax;
        centerBox.Get(Xmin,Ymin,Zmin,Xmax,Ymax,Zmax);
        orgPnt = Base::Vector3d((Xmax + Xmin)/2.0,
                                (Ymax + Ymin)/2.0,
                                (Zmax + Zmin)/2.0);
        SectionOrigin.setValue(orgPnt);
    }

    // Make the extrusion face
    double dMax = sqrt(centerBox.SquareExtent());
    BRepBuilderAPI_MakeFace mkFace(pln, -dMax,dMax,-dMax,dMax);
    TopoDS_Face aProjFace = mkFace.Face();
    if(aProjFace.IsNull()) {
        return new App::DocumentObjectExecReturn("DrawViewSection - Projected face is NULL");
    }
    gp_Vec extrudeDir = dMax * gp_Vec(gpNormal);
    TopoDS_Shape prism = BRepPrimAPI_MakePrism(aProjFace, extrudeDir, false, true).Shape();

    // We need to copy the shape to not modify the BRepstructure
    BRepBuilderAPI_Copy BuilderCopy(baseShape);
    TopoDS_Shape myShape = BuilderCopy.Shape();

    BRepAlgoAPI_Cut mkCut(myShape, prism);
    if (!mkCut.IsDone()) {
        return new App::DocumentObjectExecReturn("Section cut has failed");
    }

    TopoDS_Shape rawShape = mkCut.Shape();
//    BRepTools::Write(myShape, "DVSCopy.brep");            //debug
//    BRepTools::Write(prism, "DVSTool.brep");              //debug
//    BRepTools::Write(rawShape, "DVSResult.brep");         //debug

    Bnd_Box testBox;
    BRepBndLib::Add(rawShape, testBox);
    testBox.SetGap(0.0);
    if (testBox.IsVoid()) {                        //prism & input don't intersect.  rawShape is garbage, don't bother.
        Base::Console().Message("INFO - DVS::execute - prism & input don't intersect\n");
        return DrawView::execute();
    }

    m_cutShape = rawShape;
    m_cutShape = TechDraw::moveShape(m_cutShape,                     //centre on origin
                                  -SectionOrigin.getValue());
    gp_Pnt inputCenter;
    gp_Ax2 viewAxis;
    try {
        inputCenter = TechDraw::findCentroid(rawShape,
                                             Direction.getValue());
        TopoDS_Shape mirroredShape = TechDraw::mirrorShape(rawShape,
                                                    inputCenter,
                                                    getScale());
        viewAxis = getSectionCS(SectionDirection.getValueAsString());
        if (!DrawUtil::fpCompare(Rotation.getValue(),0.0)) {
            mirroredShape = TechDraw::rotateShape(mirroredShape,
                                                          viewAxis,
                                                          Rotation.getValue());
        }
        geometryObject = buildGeometryObject(mirroredShape,viewAxis);

#if MOD_TECHDRAW_HANDLE_FACES
        extractFaces();
#endif //#if MOD_TECHDRAW_HANDLE_FACES
    }
    catch (Standard_Failure& e1) {
        Base::Console().Warning("DVS::execute - failed to build base shape %s - %s **\n",
                                getNameInDocument(),e1.GetMessageString());
        return DrawView::execute();
    }

    try {
        TopoDS_Compound sectionCompound = findSectionPlaneIntersections(rawShape);
        TopoDS_Shape mirroredSection = TechDraw::mirrorShape(sectionCompound,
                                                             inputCenter,
                                                             getScale());
        if (!DrawUtil::fpCompare(Rotation.getValue(),0.0)) {
            mirroredSection = TechDraw::rotateShape(mirroredSection,
                                                            viewAxis,
                                                            Rotation.getValue());
        }

        sectionFaceWires.clear();
        TopoDS_Compound newFaces;
        BRep_Builder builder;
        builder.MakeCompound(newFaces);
        TopExp_Explorer expl(mirroredSection, TopAbs_FACE);
        int idb = 0;
        for (; expl.More(); expl.Next()) {
            const TopoDS_Face& face = TopoDS::Face(expl.Current());
            TopoDS_Face pFace = projectFace(face,
                                            inputCenter,
                                            Direction.getValue());
             if (!pFace.IsNull()) {
                 builder.Add(newFaces,pFace);
                 sectionFaceWires.push_back(ShapeAnalysis::OuterWire(pFace));
             }
             idb++;
        }
        sectionFaces = newFaces;
    }
    catch (Standard_Failure& e2) {
        Base::Console().Warning("DVS::execute - failed to build section faces for %s - %s **\n",
                                getNameInDocument(),e2.GetMessageString());
        return DrawView::execute();
    }

    addCosmeticVertexesToGeom();
    addCosmeticEdgesToGeom();
    addCenterLinesToGeom();

    dvp->requestPaint();  //to refresh section line
    return DrawView::execute();
}

gp_Pln DrawViewSection::getSectionPlane() const
{
    gp_Ax2 viewAxis = getSectionCS();
    gp_Ax3 viewAxis3(viewAxis);

    return gp_Pln(viewAxis3);
}


//! tries to find the intersection of the section plane with the shape giving a collection of planar faces
TopoDS_Compound DrawViewSection::findSectionPlaneIntersections(const TopoDS_Shape& shape)
{
//    Base::Console().Message("DVS::findSectionPlaneIntersections()\n");
    TopoDS_Compound result;
    if(shape.IsNull()){
        Base::Console().Warning("DrawViewSection::getSectionSurface - Sectional View shape is Empty\n");
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
std::vector<TechDraw::Face*> DrawViewSection::getFaceGeometry()
{
    std::vector<TechDraw::Face*> result;
    TopoDS_Compound c = sectionFaces;
    TopExp_Explorer faces(c, TopAbs_FACE);
    for (; faces.More(); faces.Next()) {
        TechDraw::Face* f = new TechDraw::Face();
        const TopoDS_Face& face = TopoDS::Face(faces.Current());
        TopExp_Explorer wires(face, TopAbs_WIRE);
        for (; wires.More(); wires.Next()) {
            TechDraw::Wire* w = new TechDraw::Wire();
            const TopoDS_Wire& wire = TopoDS::Wire(wires.Current());
            TopExp_Explorer edges(wire, TopAbs_EDGE);
            for (; edges.More(); edges.Next()) {
                const TopoDS_Edge& edge = TopoDS::Edge(edges.Current());
                //dumpEdge("edge",edgeCount,edge);
                TechDraw::BaseGeom* base = TechDraw::BaseGeom::baseFactory(edge);
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
    (void) direction;
    if(face.IsNull()) {
        throw Base::ValueError("DrawViewSection::projectFace - input Face is NULL");
    }

    Base::Vector3d origin(faceCenter.X(),faceCenter.Y(),faceCenter.Z());
    gp_Ax2 viewAxis = getSectionCS(SectionDirection.getValueAsString());

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
//!check if point is in box or on boundary of box
//!compare to isInBox which doesn't allow on boundary
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

bool DrawViewSection::isReallyInBox (const gp_Pnt p, const Bnd_Box& bb) const
{
    return !bb.IsOut(p);
}

void DrawViewSection::setNormalFromBase(const std::string sectionName)
{
//    Base::Console().Message("DVS::setNormalFromBase(%s)\n", sectionName.c_str());
    Base::Vector3d normal = getSectionVector(sectionName);
    Direction.setValue(normal);
    SectionNormal.setValue(normal);
}

//! calculate the section Normal/Projection Direction given section name
//TODO: this should take base view rotation into account.
Base::Vector3d DrawViewSection::getSectionVector (const std::string sectionName)
{
//    Base::Console().Message("DVS::getSectionVector(%s) - %s\n", sectionName.c_str(), getNameInDocument());
    Base::Vector3d result;
    Base::Vector3d stdX(1.0,0.0,0.0);
    Base::Vector3d stdY(0.0,1.0,0.0);
    Base::Vector3d stdZ(0.0,0.0,1.0);

    Base::Vector3d view = getBaseDVP()->Direction.getValue();
    view.Normalize();
    Base::Vector3d left = view.Cross(stdZ);
    left.Normalize();
    Base::Vector3d up = view.Cross(left);
    up.Normalize();
    double dot = view.Dot(stdZ);

    if (sectionName == "Up") {
        result = up;
        if (DrawUtil::fpCompare(dot,1.0)) {            //view = stdZ
            result = (-1.0 * stdY);
        } else if (DrawUtil::fpCompare(dot,-1.0)) {    //view = -stdZ
            result = stdY;
        }
    } else if (sectionName == "Down") {
        result = up * -1.0;
        if (DrawUtil::fpCompare(dot,1.0)) {            //view = stdZ
            result = stdY;
        } else if (DrawUtil::fpCompare(dot, -1.0)) {   //view = -stdZ
            result = (-1.0 * stdY);
        }
    } else if (sectionName == "Left") {
        result = left * -1.0;
        if (DrawUtil::fpCompare(fabs(dot),1.0)) {      //view = +/- stdZ
            result = stdX;
        }
    } else if (sectionName == "Right") {
        result = left;
        if (DrawUtil::fpCompare(fabs(dot),1.0)) {
            result = -1.0 * stdX;
        }
    } else {
        Base::Console().Log("Error - DVS::getSectionVector - bad sectionName: %s\n",sectionName.c_str());
        result = stdZ;
    }
    return result;
}

//returns current section cs
gp_Ax2 DrawViewSection::getSectionCS(void) const
{
    std::string dirName = SectionDirection.getValueAsString();
    return getSectionCS(dirName);
}

//! calculate the section Projection CS given section direction name
gp_Ax2 DrawViewSection::getSectionCS (const std::string dirName) const
{
    Base::Vector3d view = getBaseDVP()->Direction.getValue();
    view.Normalize();

    Base::Vector3d sectionOrg = SectionOrigin.getValue();
    gp_Ax2 baseCS = getBaseDVP()->getViewAxis(sectionOrg,
                                              view);

    // cardinal: 0 - left, 1 - right, 2 - up, 3 - down
    int cardinal = 0;
    if (dirName == "Up") {
        cardinal = 2;
    } else if (dirName == "Down") {
        cardinal = 3;
    } else if (dirName == "Left") {
        cardinal = 0;
    } else if (dirName == "Right") {
        cardinal = 1;
    }
    gp_Ax2 newCS = rotateCSCardinal(baseCS, cardinal);
    return newCS;
}


//TODO: this should be able to handle arbitrary rotation of the CS
//TODO: this is useful beyond DVS. move to GO or DU or ???
//      or at least static in DVS. doesn't depend on DVS object
gp_Ax2 DrawViewSection::rotateCSCardinal(gp_Ax2 oldCS, int cardinal) const
{
    // cardinal: 0 - left, 1 - right, 2 - up, 3 - down
    // as in DVS::SectionDirection
    gp_Pnt oldOrg  = oldCS.Location();
    gp_Dir oldMain = oldCS.Direction();
    gp_Dir oldX    = oldCS.XDirection();
    
    gp_Ax1 rotAxis;
    gp_Dir crossed;
    gp_Ax2 newCS;
    double angle;

    //Note: cardinal refers to the the motion of the viewer's head. When the head turns to 
    // the left, the "Direction" moves to the right!  When the viewer's head moves to look up,
    // the "Direction" moves down!

    switch (cardinal) {
        case 0:         //right
            crossed = oldMain.Crossed(oldX);
            rotAxis = gp_Ax1(oldOrg, crossed);
            angle = 90.0 * M_PI / 180.0;
            newCS = oldCS.Rotated(rotAxis, angle);
            break;
        case 1:         //left
            crossed = oldMain.Crossed(oldX);
            rotAxis = gp_Ax1(oldOrg, crossed);
            angle = -90.0 * M_PI / 180.0;
            newCS = oldCS.Rotated(rotAxis, angle);
            break;
        case 2:         // head looks up, so Direction(projection normal) points down
            rotAxis = gp_Ax1(oldOrg, oldX);        
            angle = -90.0 * M_PI / 180.0;
            newCS = oldCS.Rotated(rotAxis, angle);
            break;
        case 3:         //down
            rotAxis = gp_Ax1(oldOrg, oldX);  //one of these should be -oldX?
            angle = 90.0 * M_PI / 180.0;
            newCS = oldCS.Rotated(rotAxis, angle);
            break;
        default:
            newCS = oldCS;
            break;
    }

    return newCS;
}

gp_Ax2 DrawViewSection::rotateCSArbitrary(gp_Ax2 oldCS,
                                          Base::Vector3d axis,
                                          double degAngle) const
{
    gp_Ax2 newCS;

    gp_Pnt oldOrg  = oldCS.Location();

    gp_Dir gAxis(axis.x, axis.y, axis.z);
    gp_Ax1 rotAxis = gp_Ax1(oldOrg, gAxis);

    double radAngle = degAngle * M_PI / 180.0;

    newCS = oldCS.Rotated(rotAxis, radAngle);
    return newCS;
}

std::vector<LineSet> DrawViewSection::getDrawableLines(int i)
{
    std::vector<LineSet> result;
    result = DrawGeomHatch::getTrimmedLines(this,m_lineSets,i,HatchScale.getValue());
    return result;
}

std::vector<TopoDS_Wire> DrawViewSection::getWireForFace(int idx) const
{
    std::vector<TopoDS_Wire> result;
    result.push_back(sectionFaceWires.at(idx));
    return result;
}

void DrawViewSection::unsetupObject()
{
    TechDraw::DrawViewPart* base = getBaseDVP();
    if (base != nullptr) {
        base->touch();
    }
    DrawViewPart::unsetupObject();
}

TechDraw::DrawViewPart* DrawViewSection::getBaseDVP() const
{
    TechDraw::DrawViewPart* baseDVP = nullptr;
    App::DocumentObject* base = BaseView.getValue();
    if (base != nullptr) {
        if (base->getTypeId().isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            baseDVP = static_cast<TechDraw::DrawViewPart*>(base);
        }
    }
    return baseDVP;
}

TechDraw::DrawProjGroupItem* DrawViewSection::getBaseDPGI() const
{
    TechDraw::DrawProjGroupItem* baseDPGI = nullptr;
    App::DocumentObject* base = BaseView.getValue();
    if (base != nullptr) {
        if (base->getTypeId().isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())) {
            baseDPGI = static_cast<TechDraw::DrawProjGroupItem*>(base);
        }
    }
    return baseDPGI;
}

void DrawViewSection::getParameters()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Files");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Patterns/";
    std::string defaultFileName = defaultDir + "simple.svg";
    std::string patternFileName = hGrp->GetASCII("FileHatch",defaultFileName.c_str());
    Base::FileInfo tfi(patternFileName);
    if (tfi.isReadable()) {
        FileHatchPattern.setValue(patternFileName);
    }

    defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/PAT/";
    defaultFileName = defaultDir + "FCPAT.pat";
    patternFileName = hGrp->GetASCII("FilePattern",defaultFileName.c_str());
    Base::FileInfo tfi2(patternFileName);
    if (tfi2.isReadable()) {
        FileGeomPattern.setValue(patternFileName);
    }

    std::string patternName = hGrp->GetASCII("PatternName","Diamond");
    NameGeomPattern.setValue(patternName);

    hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");

    bool fuseFirst = hGrp->GetBool("SectionFuseFirst",true);
    FuseBeforeCut.setValue(fuseFirst);
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
