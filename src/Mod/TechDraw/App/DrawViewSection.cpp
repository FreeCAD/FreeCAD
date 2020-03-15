/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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
#include <BRepCheck_Wire.hxx>
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
#include <ShapeFix_Wire.hxx>
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
#include "DrawHatch.h"
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

    ADD_PROPERTY_TYPE(SectionSymbol ,(""),sgroup,App::Prop_None,"The identifier for this section");
    ADD_PROPERTY_TYPE(BaseView ,(0),sgroup,App::Prop_None,"2D View source for this Section");
    BaseView.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(SectionNormal ,(0,0,1.0) ,sgroup,App::Prop_None,
                        "Section Plane normal direction");  //direction of extrusion of cutting prism
    ADD_PROPERTY_TYPE(SectionOrigin ,(0,0,0) ,sgroup,App::Prop_None,"Section Plane Origin");
    SectionDirection.setEnums(SectionDirEnums);
    ADD_PROPERTY_TYPE(SectionDirection,((long)0),sgroup, App::Prop_None, "Direction in Base View for this Section");
    ADD_PROPERTY_TYPE(FuseBeforeCut ,(false),sgroup,App::Prop_None,"Merge Source(s) into a single shape before cutting");

    CutSurfaceDisplay.setEnums(CutSurfaceEnums);
    ADD_PROPERTY_TYPE(CutSurfaceDisplay,((long)2),fgroup, App::Prop_None, "Appearance of Cut Surface");

//initialize these to defaults
    ADD_PROPERTY_TYPE(FileHatchPattern ,(DrawHatch::prefSvgHatch()),fgroup,App::Prop_None,"The hatch pattern file for the cut surface");
    ADD_PROPERTY_TYPE(FileGeomPattern ,(DrawGeomHatch::prefGeomHatchFile()),fgroup,App::Prop_None,"The PAT pattern file for geometric hatching");

    ADD_PROPERTY_TYPE(SvgIncluded ,(""),fgroup,App::Prop_None,
                                            "Embedded Svg hatch file. System use only.");   // n/a to end users
    ADD_PROPERTY_TYPE(PatIncluded ,(""),fgroup,App::Prop_None,
                                            "Embedded Pat pattern file. System use only."); // n/a to end users
    ADD_PROPERTY_TYPE(NameGeomPattern ,(DrawGeomHatch::prefGeomHatchName()),fgroup,App::Prop_None,"The pattern name for geometric hatching");
    ADD_PROPERTY_TYPE(HatchScale,(1.0),fgroup,App::Prop_None,"Hatch pattern size adjustment");

    getParameters();

    std::string hatchFilter("Svg files (*.svg *.SVG);;All files (*)");
    FileHatchPattern.setFilter(hatchFilter);
    hatchFilter = ("PAT files (*.pat *.PAT);;All files (*)");
    FileGeomPattern.setFilter(hatchFilter);

    SvgIncluded.setStatus(App::Property::ReadOnly,true);
    PatIncluded.setStatus(App::Property::ReadOnly,true);
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
    App::Document* doc = getDocument();
//    bool docRestoring = getDocument()->testStatus(App::Document::Status::Restoring);
//    Base::Console().Message("DVS::onChanged(%s) - obj restoring: %d\n", 
//                            prop->getName(), isRestoring());

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

        if ((prop == &FileHatchPattern) &&
            (doc != nullptr) ) {
            if (!FileHatchPattern.isEmpty()) {
                Base::FileInfo fi(FileHatchPattern.getValue());
                if (fi.isReadable()) {
                    replaceSvgIncluded(FileHatchPattern.getValue());
                }
            }
        }

        if ( (prop == &FileGeomPattern) &&
             (doc != nullptr) ) {
            if (!FileGeomPattern.isEmpty()) {
                Base::FileInfo fi(FileGeomPattern.getValue());
                if (fi.isReadable()) {
                    replacePatIncluded(FileGeomPattern.getValue());
                }
            }
        }
    }

    if (prop == &FileGeomPattern    ||
        prop == &NameGeomPattern ) {
        if (!FileGeomPattern.isEmpty()) {
            std::string fileSpec = FileGeomPattern.getValue();
            Base::FileInfo fi(fileSpec);
            std::string ext = fi.extension();
            if (!fi.isReadable()) {
                Base::Console().Message("%s can not read hatch file: %s\n", getNameInDocument(), fileSpec.c_str());
                Base::Console().Message("%s using included hatch file.\n", getNameInDocument());
            } else {
                if ( (ext == "pat") ||
                     (ext == "PAT") ) {
                    if ((!fileSpec.empty())  &&
                        (!NameGeomPattern.isEmpty())) {
                        std::vector<PATLineSpec> specs = 
                                   DrawGeomHatch::getDecodedSpecsFromFile(fileSpec,
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
        }
    }
    DrawView::onChanged(prop);
}


//this could probably always use FileHatchPattern as input?
void DrawViewSection::replaceSvgIncluded(std::string newSvgFile)
{
//    Base::Console().Message("DVS::replaceSvgHatch(%s)\n", newSvgFile.c_str());
    if (SvgIncluded.isEmpty()) {
        setupSvgIncluded();
    } else {
        std::string tempName = SvgIncluded.getExchangeTempFile();
        DrawUtil::copyFile(newSvgFile, tempName);
        SvgIncluded.setValue(tempName.c_str());
    }
}

void DrawViewSection::replacePatIncluded(std::string newPatFile)
{
//    Base::Console().Message("DVS::replacePatHatch(%s)\n", newPatFile.c_str());
    if (PatIncluded.isEmpty()) {
        setupPatIncluded();
    } else {
        std::string tempName = PatIncluded.getExchangeTempFile();
        DrawUtil::copyFile(newPatFile, tempName);
        PatIncluded.setValue(tempName.c_str());
    }
}

App::DocumentObjectExecReturn *DrawViewSection::execute(void)
{
//    Base::Console().Message("DVS::execute() - %s/%s \n", getNameInDocument(), Label.getValue());
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

//    checkXDirection();
    bool haveX = checkXDirection();
    if (!haveX) {
        //block touch/onChanged stuff
        Base::Vector3d newX = getXDirection();
        XDirection.setValue(newX);
        XDirection.purgeTouched();  //don't trigger updates!
        //unblock
    }

    sectionExec(baseShape);
    addShapes2d();

    //second pass if required
    if (ScaleType.isValue("Automatic")) {
        if (!checkFit()) {
            double newScale = autoScale();
            Scale.setValue(newScale);
            Scale.purgeTouched();
            if (geometryObject != nullptr) {
                delete geometryObject;
                geometryObject = nullptr;
                sectionExec(baseShape);
            }
        }
    }


    dvp->requestPaint();  //to refresh section line
    return DrawView::execute();
}

void DrawViewSection::sectionExec(TopoDS_Shape baseShape)
{
    //is SectionOrigin valid?
    Bnd_Box centerBox;
    BRepBndLib::Add(baseShape, centerBox);
    centerBox.SetGap(0.0);

    gp_Pln pln = getSectionPlane();
    gp_Dir gpNormal = pln.Axis().Direction();
    Base::Vector3d orgPnt = SectionOrigin.getValue();

    if(!isReallyInBox(gp_Pnt(orgPnt.x,orgPnt.y,orgPnt.z), centerBox)) {
        Base::Console().Warning("DVS: SectionOrigin doesn't intersect part in %s\n",getNameInDocument());
    }

    // Make the extrusion face
    double dMax = sqrt(centerBox.SquareExtent());
    BRepBuilderAPI_MakeFace mkFace(pln, -dMax,dMax,-dMax,dMax);
    TopoDS_Face aProjFace = mkFace.Face();
    if(aProjFace.IsNull()) {
        Base::Console().Warning("DVS: Section face is NULL in %s\n",getNameInDocument());
        return;
    }
    gp_Vec extrudeDir = dMax * gp_Vec(gpNormal);
    TopoDS_Shape prism = BRepPrimAPI_MakePrism(aProjFace, extrudeDir, false, true).Shape();

    // We need to copy the shape to not modify the BRepstructure
    BRepBuilderAPI_Copy BuilderCopy(baseShape);
    TopoDS_Shape myShape = BuilderCopy.Shape();

    BRepAlgoAPI_Cut mkCut(myShape, prism);
    if (!mkCut.IsDone()) {
        Base::Console().Warning("DVS: Section cut has failed in %s\n",getNameInDocument());
        return;
    }

    TopoDS_Shape rawShape = mkCut.Shape();
    if (debugSection()) {
        BRepTools::Write(myShape, "DVSCopy.brep");            //debug
        BRepTools::Write(aProjFace, "DVSFace.brep");          //debug
        BRepTools::Write(prism, "DVSTool.brep");              //debug
        BRepTools::Write(rawShape, "DVSResult.brep");         //debug
    }

    Bnd_Box testBox;
    BRepBndLib::Add(rawShape, testBox);
    testBox.SetGap(0.0);
    if (testBox.IsVoid()) {           //prism & input don't intersect.  rawShape is garbage, don't bother.
        Base::Console().Warning("DVS::execute - prism & input don't intersect - %s\n", Label.getValue());
        return;
    }

    gp_Ax2 viewAxis;
    TopoDS_Shape centeredShape;
    try {
        Base::Vector3d sectionOrigin = SectionOrigin.getValue();
        centeredShape = TechDraw::moveShape(rawShape,
                                            sectionOrigin * -1.0);
        m_cutShape = centeredShape;
        m_saveCentroid = sectionOrigin;

        TopoDS_Shape scaledShape   = TechDraw::scaleShape(centeredShape,
                                                          getScale());

        Base::Vector3d origin(0.0, 0.0, 0.0);
        viewAxis = getProjectionCS(origin);
        if (!DrawUtil::fpCompare(Rotation.getValue(),0.0)) {
            scaledShape = TechDraw::rotateShape(scaledShape,
                                                viewAxis,
                                                Rotation.getValue());
        }
        if (debugSection()) {
            BRepTools::Write(m_cutShape, "DVSmCutShape.brep");         //debug
            BRepTools::Write(scaledShape, "DVSScaled.brep");              //debug
//            DrawUtil::dumpCS("DVS::execute - CS to GO", viewAxis);
        }

        geometryObject = buildGeometryObject(scaledShape,viewAxis);

#if MOD_TECHDRAW_HANDLE_FACES
        extractFaces();
#endif //#if MOD_TECHDRAW_HANDLE_FACES
    }
    catch (Standard_Failure& e1) {
        Base::Console().Warning("DVS::execute - failed to build base shape %s - %s **\n",
                                getNameInDocument(),e1.GetMessageString());
        return;
    }

    try {
        //sectionFaces = build sectionFaces(rawShape);
        TopoDS_Compound sectionCompound = findSectionPlaneIntersections(rawShape);
 
        Base::Vector3d sectionOrigin = SectionOrigin.getValue();
        TopoDS_Shape centeredShape = TechDraw::moveShape(sectionCompound,
                                                         sectionOrigin * -1.0);

        TopoDS_Shape scaledSection = TechDraw::scaleShape(centeredShape,
                                                          getScale());
        if (!DrawUtil::fpCompare(Rotation.getValue(),0.0)) {
            scaledSection = TechDraw::rotateShape(scaledSection,
                                                  viewAxis,
                                                  Rotation.getValue());
        }
        if (debugSection()) {
            BRepTools::Write(scaledSection, "DVSScaledFaces.brep");            //debug
        }

        sectionFaceWires.clear();
        TopoDS_Compound newFaces;
        BRep_Builder builder;
        builder.MakeCompound(newFaces);
        TopExp_Explorer expl(scaledSection, TopAbs_FACE);
        int idb = 0;
        for (; expl.More(); expl.Next()) {
            const TopoDS_Face& face = TopoDS::Face(expl.Current());
            TopoDS_Face pFace = projectFace(face,
                                            viewAxis);
            if (!pFace.IsNull()) {
                if (debugSection()) {
                    std::stringstream ss;
                    ss << "DVSScaledFace" << idb << ".brep" ;
                    std::string faceName = ss.str();
                    BRepTools::Write(pFace, faceName.c_str());            //debug
                    std::stringstream ss2;
                    ss2 << "DVSOuter" << idb << ".brep" ;
                    TopoDS_Wire owdb = ShapeAnalysis::OuterWire(pFace);
                    std::string wireName = ss2.str();
                    BRepTools::Write(owdb, wireName.c_str());               //debug
                }
                TopoDS_Wire ow = ShapeAnalysis::OuterWire(pFace);
                //this check helps prevent "ghost" faces
                BRepCheck_Wire chkWire(ow);
                TopoDS_Edge e1, e2;
                BRepCheck_Status status = chkWire.SelfIntersect(pFace, e1, e2);
                if (status == BRepCheck_NoError) {
                    builder.Add(newFaces,pFace);
                    sectionFaceWires.push_back(ShapeAnalysis::OuterWire(pFace));
                }
            }
            idb++;
        }
        //return newFaces;
        sectionFaces = newFaces;
    }
    catch (Standard_Failure& e2) {
        Base::Console().Warning("DVS::execute - failed to build section faces for %s - %s **\n",
                                getNameInDocument(),e2.GetMessageString());
        return;
    }

    addCosmeticVertexesToGeom();
    addCosmeticEdgesToGeom();
    addCenterLinesToGeom();

    addReferencesToGeom();
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
                                         const gp_Ax2 CS)
{
//    Base::Console().Message("DVS::projectFace()\n");
    if(face.IsNull()) {
        throw Base::ValueError("DrawViewSection::projectFace - input Face is NULL");
    }

    TopoDS_Shape hardEdges = geometryObject->projectFace(face, CS);

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

Base::Vector3d DrawViewSection::getXDirection(void) const
{
//    Base::Console().Message("DVS::getXDirection() - %s\n", Label.getValue());
    Base::Vector3d result(1.0, 0.0, 0.0);               //default X
    App::Property* prop = getPropertyByName("XDirection");
    if (prop != nullptr) {                              //have an XDirection property
        Base::Vector3d propVal = XDirection.getValue();
        if (DrawUtil::fpCompare(propVal.Length(), 0.0))  {   //but it has no value
            std::string sectName = SectionDirection.getValueAsString();
            gp_Ax2 cs = getCSFromBase(sectName);
            gp_Dir gXDir = cs.XDirection();
            result = Base::Vector3d(gXDir.X(),
                                    gXDir.Y(),
                                    gXDir.Z());
        } else {
            result = propVal;                               //normal case.  XDirection is set.
        }
    } else {                                                //no Property.  can this happen?
            std::string sectName = SectionDirection.getValueAsString();
            gp_Ax2 cs = getCSFromBase(sectName);
            gp_Dir gXDir = cs.XDirection();
            result = Base::Vector3d(gXDir.X(),
                                    gXDir.Y(),
                                    gXDir.Z());

    }
    return result;
}

void DrawViewSection::setCSFromBase(const std::string sectionName) 
{
//    Base::Console().Message("DVS::setCSFromBase(%s)\n", sectionName.c_str());
    gp_Ax2 CS = getCSFromBase(sectionName);
    gp_Dir gDir = CS.Direction();
    Base::Vector3d vDir(gDir.X(),
                        gDir.Y(),
                        gDir.Z());
    Direction.setValue(vDir);
    SectionNormal.setValue(vDir);
    gp_Dir gxDir = CS.XDirection();
    Base::Vector3d vXDir(gxDir.X(),
                         gxDir.Y(),
                         gxDir.Z());
    XDirection.setValue(vXDir);
}

gp_Ax2 DrawViewSection::getCSFromBase(const std::string sectionName) const
{
//    Base::Console().Message("DVS::getCSFromBase(%s)\n", sectionName.c_str());
    Base::Vector3d sectionNormal;
    Base::Vector3d sectionXDir;
    Base::Vector3d origin(0.0, 0.0, 0.0);
    Base::Vector3d sectOrigin = SectionOrigin.getValue();

    gp_Ax2 dvpCS = getBaseDVP()->getProjectionCS(sectOrigin);
    
    if (debugSection()) {
        DrawUtil::dumpCS("DVS::getCSFromBase - dvp CS", dvpCS);
    }
    gp_Dir dvpDir = dvpCS.Direction();
    gp_Dir dvpUp = dvpCS.YDirection();
    gp_Dir dvpRight = dvpCS.XDirection();
    gp_Pnt dvsLoc(sectOrigin.x,
                  sectOrigin.y,
                  sectOrigin.z);
    gp_Dir dvsDir;
    gp_Dir dvsXDir;

    if (sectionName == "Up") {      //looking up
        dvsDir = dvpUp.Reversed();
        dvsXDir = dvpRight;
    } else if (sectionName == "Down") {
        dvsDir  = dvpUp;
        dvsXDir = dvpRight;
    } else if (sectionName == "Left") {
        dvsDir = dvpRight;
        dvsXDir = dvpDir.Reversed();
    } else if (sectionName == "Right") {
        dvsDir = dvpRight.Reversed();
        dvsXDir = dvpDir;
    } else {
        Base::Console().Log("Error - DVS::getCSFromBase - bad sectionName: %s\n",sectionName.c_str());
        dvsDir = dvpRight;
        dvsXDir = dvpDir;
    }

    gp_Ax2 CS(dvsLoc,
              dvsDir,
              dvsXDir);

    if (debugSection()) {
        DrawUtil::dumpCS("DVS::getCSFromBase - sectionCS out", CS);
    }

    return CS;
}

//returns current section cs
gp_Ax2 DrawViewSection::getSectionCS(void) const
{
//    Base::Console().Message("DVS::getSectionCS()\n");
    Base::Vector3d vNormal = SectionNormal.getValue();
    gp_Dir gNormal(vNormal.x,
                   vNormal.y,
                   vNormal.z);
    Base::Vector3d vXDir   = getXDirection();
    gp_Dir gXDir(vXDir.x,
                 vXDir.y,
                 vXDir.z);
    Base::Vector3d vOrigin = SectionOrigin.getValue();
    gp_Pnt gOrigin(vOrigin.x,
                   vOrigin.y,
                   vOrigin.z);
    gp_Ax2 sectionCS(gOrigin,
                     gNormal);
    try {
        sectionCS = gp_Ax2(gOrigin, 
                           gNormal,
                           gXDir);
    }
    catch (...) {
        Base::Console().Warning("DVS::getSectionCS - %s - failed to create section CS\n", getNameInDocument());
    }
    return sectionCS;
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
//    Base::Console().Message("DVS::getParameters()\n");
    Base::Reference<ParameterGrp>hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");

    bool fuseFirst = hGrp->GetBool("SectionFuseFirst", false);
    FuseBeforeCut.setValue(fuseFirst);
}

bool DrawViewSection::debugSection(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/debug");

    bool result = hGrp->GetBool("debugSection",false);
    return result;
}


void DrawViewSection::onDocumentRestored() 
{
//    Base::Console().Message("DVS::onDocumentRestored()\n");
    if (!FileHatchPattern.isEmpty()) {
        std::string svgFileName = FileHatchPattern.getValue();
        Base::FileInfo tfi(svgFileName);
        if (tfi.isReadable()) {
            if (SvgIncluded.isEmpty()) {
                setupSvgIncluded();
            }
        }
    }

    if (!FileGeomPattern.isEmpty()) {
        std::string patFileName = FileGeomPattern.getValue();
        Base::FileInfo tfi(patFileName);
        if (tfi.isReadable()) {
            if (PatIncluded.isEmpty()) {
                setupPatIncluded();
            }
        }
    }
    DrawViewPart::onDocumentRestored();
}

void DrawViewSection::setupObject()
{
    //by this point DVS should have a name and belong to a document
    setupSvgIncluded();
    setupPatIncluded();

    DrawViewPart::setupObject();
}

void DrawViewSection::setupSvgIncluded(void)
{
//    Base::Console().Message("DVS::setupSvgIncluded()\n");
    App::Document* doc = getDocument();
    std::string special = getNameInDocument();
    special += "SvgHatch.svg";
    std::string dir = doc->TransientDir.getValue();
    std::string svgName = dir + special;
    
    //first time
    std::string svgInclude = SvgIncluded.getValue();
    if (svgInclude.empty()) {
        DrawUtil::copyFile(std::string(), svgName);
        SvgIncluded.setValue(svgName.c_str());
    }

    std::string svgFile = FileHatchPattern.getValue();
    if (!svgFile.empty()) {
        std::string exchName = SvgIncluded.getExchangeTempFile();
        DrawUtil::copyFile(svgFile, exchName);
        SvgIncluded.setValue(exchName.c_str(), special.c_str());
    }
}

void DrawViewSection::setupPatIncluded(void)
{
//    Base::Console().Message("DVS::setupPatIncluded()\n");
    App::Document* doc = getDocument();
    std::string special = getNameInDocument();
    special += "PatHatch.pat";
    std::string dir = doc->TransientDir.getValue();
    std::string patName = dir + special;
    std::string patProp = PatIncluded.getValue();
    if (patProp.empty()) {
        DrawUtil::copyFile(std::string(), patName);
        PatIncluded.setValue(patName.c_str());
    }

    if (!FileGeomPattern.isEmpty()) {
        std::string exchName = PatIncluded.getExchangeTempFile();
        DrawUtil::copyFile(FileGeomPattern.getValue(), exchName);
        PatIncluded.setValue(exchName.c_str(), special.c_str());
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
