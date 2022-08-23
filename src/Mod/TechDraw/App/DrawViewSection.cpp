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
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Transform.hxx>
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
#include <sstream>

#include <QFile>
#include <QFileInfo>
#include <QtConcurrentRun>

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
#include "HatchLine.h"
#include "DrawUtil.h"
#include "DrawProjGroupItem.h"
#include "DrawGeomHatch.h"
#include "DrawHatch.h"
#include "DrawViewSection.h"

using namespace TechDraw;
using namespace std;

const char* DrawViewSection::SectionDirEnums[]= {"Right",
                                            "Left",
                                            "Up",
                                            "Down",
                                             nullptr};

const char* DrawViewSection::CutSurfaceEnums[]= {"Hide",
                                            "Color",
                                            "SvgHatch",
                                            "PatHatch",
                                             nullptr};


//===========================================================================
// DrawViewSection
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewSection, TechDraw::DrawViewPart)

DrawViewSection::DrawViewSection()  :
    m_waitingForCut(false)
{
    static const char *sgroup = "Section";
    static const char *fgroup = "Cut Surface Format";

    ADD_PROPERTY_TYPE(SectionSymbol ,(""), sgroup, App::Prop_None, "The identifier for this section");
    ADD_PROPERTY_TYPE(BaseView ,(nullptr), sgroup, App::Prop_None, "2D View source for this Section");
    BaseView.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(SectionNormal ,(0, 0,1.0) ,sgroup, App::Prop_None,
                        "Section Plane normal direction");  //direction of extrusion of cutting prism
    ADD_PROPERTY_TYPE(SectionOrigin ,(0, 0,0) ,sgroup, App::Prop_None, "Section Plane Origin");
    SectionDirection.setEnums(SectionDirEnums);
    ADD_PROPERTY_TYPE(SectionDirection, ((long)0), sgroup, App::Prop_None, "Direction in Base View for this Section");
    ADD_PROPERTY_TYPE(FuseBeforeCut ,(false), sgroup, App::Prop_None, "Merge Source(s) into a single shape before cutting");

    CutSurfaceDisplay.setEnums(CutSurfaceEnums);
    ADD_PROPERTY_TYPE(CutSurfaceDisplay, (prefCutSurface()), fgroup, App::Prop_None, "Appearance of Cut Surface");

//initialize these to defaults
    ADD_PROPERTY_TYPE(FileHatchPattern ,(DrawHatch::prefSvgHatch()), fgroup, App::Prop_None, "The hatch pattern file for the cut surface");
    ADD_PROPERTY_TYPE(FileGeomPattern ,(DrawGeomHatch::prefGeomHatchFile()), fgroup, App::Prop_None, "The PAT pattern file for geometric hatching");

    ADD_PROPERTY_TYPE(SvgIncluded ,(""), fgroup, App::Prop_None,
                                            "Embedded Svg hatch file. System use only.");   // n/a to end users
    ADD_PROPERTY_TYPE(PatIncluded ,(""), fgroup, App::Prop_None,
                                            "Embedded Pat pattern file. System use only."); // n/a to end users
    ADD_PROPERTY_TYPE(NameGeomPattern ,(DrawGeomHatch::prefGeomHatchName()), fgroup, App::Prop_None, "The pattern name for geometric hatching");
    ADD_PROPERTY_TYPE(HatchScale, (1.0), fgroup, App::Prop_None, "Hatch pattern size adjustment");

    getParameters();

    std::string hatchFilter("Svg files (*.svg *.SVG);;All files (*)");
    FileHatchPattern.setFilter(hatchFilter);
    hatchFilter = ("PAT files (*.pat *.PAT);;All files (*)");
    FileGeomPattern.setFilter(hatchFilter);

    SvgIncluded.setStatus(App::Property::ReadOnly, true);
    PatIncluded.setStatus(App::Property::ReadOnly, true);
}

DrawViewSection::~DrawViewSection()
{
    //don't destroy this object while it has dependent threads running
    if (m_cutFuture.isRunning()) {
        Base::Console().Message("%s is waiting for tasks to complete\n", Label.getValue());
        m_cutFuture.waitForFinished();
    }
}

short DrawViewSection::mustExecute() const
{
    if (isRestoring()) {
        return TechDraw::DrawView::mustExecute();
    }

    if (Scale.isTouched() ||
        Direction.isTouched()     ||
        BaseView.isTouched()  ||
        SectionNormal.isTouched() ||
        SectionOrigin.isTouched() ) {
        return 1;
    }

    return TechDraw::DrawView::mustExecute();
}

void DrawViewSection::onChanged(const App::Property* prop)
{
    if (isRestoring()) {
        DrawViewPart::onChanged(prop);
        return;
    }

    App::Document* doc = getDocument();
    if (!doc) {
        //tarfu
        DrawViewPart::onChanged(prop);
        return;
    }

    if (prop == &SectionSymbol) {
        std::string lblText = "Section " +
                              std::string(SectionSymbol.getValue()) +
                              " - " +
                              std::string(SectionSymbol.getValue());
        Label.setValue(lblText);
    } else if (prop == &CutSurfaceDisplay) {
        if (CutSurfaceDisplay.isValue("PatHatch")) {
            makeLineSets();
        }
    } else if (prop == &FileHatchPattern) {
        if (!FileHatchPattern.isEmpty()) {
            Base::FileInfo fi(FileHatchPattern.getValue());
            if (fi.isReadable()) {
                replaceSvgIncluded(FileHatchPattern.getValue());
            }
        }
    } else if (prop == &FileGeomPattern) {
        if (!FileGeomPattern.isEmpty()) {
            Base::FileInfo fi(FileGeomPattern.getValue());
            if (fi.isReadable()) {
                replacePatIncluded(FileGeomPattern.getValue());
            }
        }
    } else if (prop == &FileGeomPattern    ||
        prop == &NameGeomPattern ) {
        makeLineSets();
    }

    DrawView::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawViewSection::execute()
{
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    App::DocumentObject* base = BaseView.getValue();
    if (!base) {
        return new App::DocumentObjectExecReturn("BaseView object not found");
    }

    TechDraw::DrawViewPart* dvp = nullptr;
    if (!base->getTypeId().isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        //this can probably only happen with scripting
        return new App::DocumentObjectExecReturn("BaseView object is not a DrawViewPart object");
    } else {
        dvp = static_cast<TechDraw::DrawViewPart*>(base);
    }

    TopoDS_Shape baseShape = dvp->getSourceShape();
    if (FuseBeforeCut.getValue()) {
        baseShape = dvp->getSourceShapeFused();
    }
    
    if (baseShape.IsNull()) {
        return DrawView::execute();
    }

    m_saveShape = baseShape;        //save shape for 2nd pass

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

    return DrawView::execute();
}

void DrawViewSection::sectionExec(TopoDS_Shape& baseShape)
{
//    Base::Console().Message("DVS::sectionExec() - %s baseShape.IsNull: %d\n",
//                            getNameInDocument(), baseShape.IsNull());

    if (waitingForHlr() ||
        waitingForCut()) {
        return;
    }

    if (baseShape.IsNull()) {
        //should be caught before this
        return;
    }

    try {
        //note that &m_cutWatcher in the third parameter is not strictly required, but using the
        //4 parameter signature instead of the 3 parameter signature prevents clazy warning:
        //https://github.com/KDE/clazy/blob/1.11/docs/checks/README-connect-3arg-lambda.md
        connectCutWatcher = QObject::connect(&m_cutWatcher, &QFutureWatcherBase::finished,
                                             &m_cutWatcher, [this] { this->onSectionCutFinished(); });
        m_cutFuture = QtConcurrent::run(this, &DrawViewSection::makeSectionCut, baseShape);
        m_cutWatcher.setFuture(m_cutFuture);
        waitingForCut(true);
    }
    catch (...) {
        Base::Console().Message("DVS::sectionExec - failed to make section cut");
        return;
    }
}

void DrawViewSection::makeSectionCut(TopoDS_Shape &baseShape)
{
//    Base::Console().Message("DVS::makeSectionCut() - %s - baseShape.IsNull: %d\n",
//                            getNameInDocument(), baseShape.IsNull());

    showProgressMessage(getNameInDocument(), "is making section cut");

// cut base shape with tool
    //is SectionOrigin valid?
    Bnd_Box centerBox;
    BRepBndLib::AddOptimal(baseShape, centerBox);
    centerBox.SetGap(0.0);
    gp_Pln pln = getSectionPlane();
    gp_Dir gpNormal = pln.Axis().Direction();
    Base::Vector3d orgPnt = SectionOrigin.getValue();

    if(!isReallyInBox(gp_Pnt(orgPnt.x, orgPnt.y, orgPnt.z), centerBox)) {
        Base::Console().Warning("DVS: SectionOrigin doesn't intersect part in %s\n", getNameInDocument());
    }

    // make cutting tool
    // Make the extrusion face
    double dMax = sqrt(centerBox.SquareExtent());
    BRepBuilderAPI_MakeFace mkFace(pln, -dMax, dMax, -dMax, dMax);
    TopoDS_Face aProjFace = mkFace.Face();
    if(aProjFace.IsNull()) {
        Base::Console().Warning("DVS: Section face is NULL in %s\n", getNameInDocument());
        return;
    }
    gp_Vec extrudeDir = dMax * gp_Vec(gpNormal);
    TopoDS_Shape prism = BRepPrimAPI_MakePrism(aProjFace, extrudeDir, false, true).Shape();

    // We need to copy the shape to not modify the BRepstructure
    BRepBuilderAPI_Copy BuilderCopy(baseShape);
    TopoDS_Shape myShape = BuilderCopy.Shape();
    m_saveShape = myShape;        //save shape for 2nd pass

// perform cut
    BRep_Builder builder;
    TopoDS_Compound pieces;
    builder.MakeCompound(pieces);
    TopExp_Explorer expl(myShape, TopAbs_SOLID);
    int indb = 0;
    int outdb = 0;
    for (; expl.More(); expl.Next()) {
        indb++;
        const TopoDS_Solid& s = TopoDS::Solid(expl.Current());
        BRepAlgoAPI_Cut mkCut(s, prism);
        if (!mkCut.IsDone()) {
            Base::Console().Warning("DVS: Section cut has failed in %s\n", getNameInDocument());
            continue;
        }
        TopoDS_Shape cut = mkCut.Shape();
        builder.Add(pieces, cut);
        outdb++;
    }

    // pieces contains result of cutting each subshape in baseShape with tool
    TopoDS_Shape rawShape = pieces;

    if (debugSection()) {
        BRepTools::Write(myShape, "DVSCopy.brep");            //debug
        BRepTools::Write(aProjFace, "DVSFace.brep");          //debug
        BRepTools::Write(prism, "DVSTool.brep");              //debug
        BRepTools::Write(pieces, "DVSPieces.brep");         //debug
    }

// check for error in cut
    Bnd_Box testBox;
    BRepBndLib::AddOptimal(rawShape, testBox);
    testBox.SetGap(0.0);
    if (testBox.IsVoid()) {           //prism & input don't intersect.  rawShape is garbage, don't bother.
        Base::Console().Warning("DVS::makeSectionCut - prism & input don't intersect - %s\n", Label.getValue());
        return;

    }

// build display geometry as in DVP, with minor mods
    TopoDS_Shape centeredShape;
    try {
        Base::Vector3d origin(0.0, 0.0, 0.0);
        m_viewAxis = getProjectionCS(origin);
        gp_Pnt inputCenter;
        inputCenter = TechDraw::findCentroid(rawShape,
                                             m_viewAxis);
        Base::Vector3d centroid(inputCenter.X(),
                                inputCenter.Y(),
                                inputCenter.Z());

        centeredShape = TechDraw::moveShape(rawShape,
                                            centroid * -1.0);
        m_cutShape = centeredShape;
        m_saveCentroid = centroid;

        m_scaledShape   = TechDraw::scaleShape(centeredShape,
                                             getScale());

        if (!DrawUtil::fpCompare(Rotation.getValue(), 0.0)) {
            m_scaledShape = TechDraw::rotateShape(m_scaledShape,
                                                  m_viewAxis,
                                                  Rotation.getValue());
        }
        if (debugSection()) {
            BRepTools::Write(m_cutShape, "DVSmCutShape.brep");         //debug
            BRepTools::Write(m_scaledShape, "DVSScaled.brep");              //debug
//            DrawUtil::dumpCS("DVS::makeSectionCut - CS to GO", viewAxis);
        }

        m_rawShape = rawShape;  //save for section face finding

    }
    catch (Standard_Failure& e1) {
        Base::Console().Warning("DVS::makeSectionCut - failed to build base shape %s - %s **\n",
                                getNameInDocument(), e1.GetMessageString());
        return;
    }

    waitingForCut(false);
}

void DrawViewSection::onSectionCutFinished()
{
//    Base::Console().Message("DVS::onSectionCutFinished() - %s\n", getNameInDocument());
    QObject::disconnect(connectCutWatcher);

    showProgressMessage(getNameInDocument(), "has finished making section cut");

    postSectionCutTasks();

    //display geometry for cut shape is in geometryObject as in DVP
    m_tempGeometryObject = buildGeometryObject(m_scaledShape, m_viewAxis);
}

//activities that depend on updated geometry object
void DrawViewSection::postHlrTasks(void)
{
//    Base::Console().Message("DVS::postHlrTasks() - %s\n", getNameInDocument());

    DrawViewPart::postHlrTasks();

    //second pass if required
    if (ScaleType.isValue("Automatic")) {
        if (!checkFit()) {
            double newScale = autoScale();
            Scale.setValue(newScale);
            Scale.purgeTouched();
            sectionExec(m_saveShape);
        }
    }
    overrideKeepUpdated(false);


    // build section face geometry
    TopoDS_Compound faceIntersections = findSectionPlaneIntersections(m_rawShape);
    if (faceIntersections.IsNull()) {
        requestPaint();
        return;
    }

    TopoDS_Shape centeredShapeF = TechDraw::moveShape(faceIntersections,
                                                       m_saveCentroid * -1.0);

    TopoDS_Shape scaledSection = TechDraw::scaleShape(centeredShapeF,
                                                      getScale());
    if (!DrawUtil::fpCompare(Rotation.getValue(), 0.0)) {
        scaledSection = TechDraw::rotateShape(scaledSection,
                                              m_viewAxis,
                                              Rotation.getValue());
    }
    if (debugSection()) {
        BRepTools::Write(scaledSection, "DVSScaledFaces.brep");            //debug
    }

    // scaledSection is compound of TopoDS_Face intersections, but aligned to pln(origin, sectionNormal)
    // needs to be aligned to pln (origin, stdZ);
    gp_Ax3 R3;
    gp_Ax2 projCS = getSectionCS();
    gp_Ax3 proj3 = gp_Ax3(gp_Pnt(0.0, 0.0, 0.0),
                   projCS.Direction(),
                   projCS.XDirection());
    gp_Trsf fromR3;
    fromR3.SetTransformation(R3, proj3);
    BRepBuilderAPI_Transform xformer(fromR3);
    xformer.Perform(scaledSection, true);
    if (xformer.IsDone()) {
        sectionTopoDSFaces = TopoDS::Compound(xformer.Shape());
    } else {
        Base::Console().Message("DVS::sectionExec - face xform failed\n");
    }

    sectionTopoDSFaces = TopoDS::Compound(GeometryObject::invertGeometry(sectionTopoDSFaces));     //handle Qt -y

    //turn section faces into TD geometry
    tdSectionFaces.clear();
    TopExp_Explorer sectionExpl(sectionTopoDSFaces, TopAbs_FACE);
    for (; sectionExpl.More(); sectionExpl.Next()) {
        const TopoDS_Face& face = TopoDS::Face(sectionExpl.Current());
        TechDraw::FacePtr sectionFace(std::make_shared<TechDraw::Face>());
        TopExp_Explorer expFace(face, TopAbs_WIRE);
        for ( ; expFace.More(); expFace.Next()) {
            TechDraw::Wire* w = new TechDraw::Wire();
            const TopoDS_Wire& wire = TopoDS::Wire(expFace.Current());
            TopExp_Explorer expWire(wire, TopAbs_EDGE);
            for ( ; expWire.More(); expWire.Next()) {
                const TopoDS_Edge& edge = TopoDS::Edge(expWire.Current());
                TechDraw::BaseGeomPtr e = BaseGeom::baseFactory(edge);
                if (e) {
                    w->geoms.push_back(e);
                }
            }
            sectionFace->wires.push_back(w);
        }
        tdSectionFaces.push_back(sectionFace);
    }

    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(BaseView.getValue());
    if (dvp) {
        dvp->requestPaint();  //to refresh section line
    }
    requestPaint();
}

//activities that depend on a valid section cut
void DrawViewSection::postSectionCutTasks()
{
    std::vector<App::DocumentObject*> children = getInList();
    for (auto& c: children) {
        if (c->getTypeId().isDerivedFrom(DrawViewPart::getClassTypeId())) {
            //details or sections of this need cut shape
            c->recomputeFeature();
        }
    }
}

bool DrawViewSection::waitingForResult() const
{
    if (DrawViewPart::waitingForResult() ||
        waitingForCut()) {
        return true;
    }
    return false;
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
//    Base::Console().Message("DVS::findSectionPlaneIntersections() - %s\n", getNameInDocument());
    if(shape.IsNull()){
        // this shouldn't happen
        Base::Console().Warning("DrawViewSection::findSectionPlaneInter - %s - input shape is Null\n", getNameInDocument());
        return TopoDS_Compound();
    }

    gp_Pln plnSection = getSectionPlane();
    BRep_Builder builder;
    TopoDS_Compound result;
    builder.MakeCompound(result);

    TopExp_Explorer expFaces(shape, TopAbs_FACE);
    for ( ; expFaces.More(); expFaces.Next()) {
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

//calculate the ends of the section line in BaseView's coords
std::pair<Base::Vector3d, Base::Vector3d> DrawViewSection::sectionLineEnds()
{
    std::pair<Base::Vector3d, Base::Vector3d> result;
    Base::Vector3d stdZ(0.0, 0.0, 1.0);
    double baseRotation = getBaseDVP()->Rotation.getValue();      //Qt degrees are clockwise
    Base::Rotation rotator(stdZ, baseRotation * M_PI / 180.0);
    Base::Rotation unrotator(stdZ, - baseRotation * M_PI / 180.0);

    auto sNorm  = SectionNormal.getValue();
    auto axis   = getBaseDVP()->Direction.getValue();
    Base::Vector3d stdOrg(0.0, 0.0, 0.0);
    Base::Vector3d sectionLineDir = - axis.Cross(sNorm);
    sectionLineDir.Normalize();
    sectionLineDir = getBaseDVP()->projectPoint(sectionLineDir);   //convert to base view CS
    sectionLineDir.Normalize();

    Base::Vector3d sectionOrg = SectionOrigin.getValue() - getBaseDVP()->getOriginalCentroid();
    sectionOrg = getBaseDVP()->projectPoint(sectionOrg);            //convert to base view CS

    //get the unscaled X and Y ranges of the base view geometry
    auto bbx = getBaseDVP()->getBoundingBox();
    double xRange = bbx.MaxX - bbx.MinX;
    xRange /= getBaseDVP()->getScale();
    double yRange = bbx.MaxY - bbx.MinY;
    yRange /= getBaseDVP()->getScale();

    sectionOrg = rotator.multVec(sectionOrg);
    sectionLineDir = rotator.multVec(sectionLineDir);

    result = DrawUtil::boxIntersect2d(sectionOrg, sectionLineDir, xRange, yRange);  //unscaled
    result.first = unrotator.multVec(result.first);
    result.second = unrotator.multVec(result.second);

    return result;
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

Base::Vector3d DrawViewSection::getXDirection() const
{
//    Base::Console().Message("DVS::getXDirection() - %s\n", Label.getValue());
    Base::Vector3d result(1.0, 0.0, 0.0);               //default X
    App::Property* prop = getPropertyByName("XDirection");
    if (prop) {
        //we have an XDirection property
        if (DrawUtil::fpCompare(XDirection.getValue().Length(), 0.0)) {
            //but it has no value, so we make a value
            gp_Ax2 cs = getCSFromBase(SectionDirection.getValueAsString());
            gp_Dir gXDir = cs.XDirection();
            result = Base::Vector3d(gXDir.X(),
                                    gXDir.Y(),
                                    gXDir.Z());
        } else {
            //XDirection is good, so we use it
            result = XDirection.getValue();
        }
    } else {
        //no XDirection property.  can this happen?
        gp_Ax2 cs = getCSFromBase(SectionDirection.getValueAsString());
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
    gp_Dir gDir = getCSFromBase(sectionName).Direction();
    Base::Vector3d vDir(gDir.X(),
                        gDir.Y(),
                        gDir.Z());
    Direction.setValue(vDir);
    SectionNormal.setValue(vDir);
    gp_Dir gxDir = getCSFromBase(sectionName).XDirection();
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
        Base::Console().Log("Error - DVS::getCSFromBase - bad sectionName: %s\n", sectionName.c_str());
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
gp_Ax2 DrawViewSection::getSectionCS() const
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
        Base::Console().Log("DVS::getSectionCS - %s - failed to create section CS\n", getNameInDocument());
    }
    return sectionCS;
}

std::vector<LineSet> DrawViewSection::getDrawableLines(int i)
{
//    Base::Console().Message("DVS::getDrawableLines(%d) - lineSets: %d\n", i, m_lineSets.size());
    std::vector<LineSet> result;
    result = DrawGeomHatch::getTrimmedLinesSection(this, m_lineSets,
                                                   getSectionTopoDSFace(i),
                                                   HatchScale.getValue());
    return result;
}

TopoDS_Face DrawViewSection::getSectionTopoDSFace(int i)
{
    TopoDS_Face result;
    TopExp_Explorer expl(sectionTopoDSFaces, TopAbs_FACE);
    int count = 1;
    for (; expl.More(); expl.Next(), count++) {
        if (count == i+1) {
            result = TopoDS::Face(expl.Current());
        }
    }
    return result;
}

TechDraw::DrawViewPart* DrawViewSection::getBaseDVP() const
{
    TechDraw::DrawViewPart* baseDVP = nullptr;
    App::DocumentObject* base = BaseView.getValue();
    if (base) {
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
    if (base) {
        if (base->getTypeId().isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())) {
            baseDPGI = static_cast<TechDraw::DrawProjGroupItem*>(base);
        }
    }
    return baseDPGI;
}

// setup / tear down routines

void DrawViewSection::unsetupObject()
{
    TechDraw::DrawViewPart* base = getBaseDVP();
    if (base) {
        base->touch();
    }
    DrawViewPart::unsetupObject();
}

void DrawViewSection::onDocumentRestored() 
{
//    Base::Console().Message("DVS::onDocumentRestored()\n");
    if (SvgIncluded.isEmpty()) {
        if (!FileHatchPattern.isEmpty()) {
            std::string svgFileName = FileHatchPattern.getValue();
            Base::FileInfo tfi(svgFileName);
            if (tfi.isReadable()) {
                setupSvgIncluded();
            }
        }
    }

    if (PatIncluded.isEmpty()) {
        if (!FileGeomPattern.isEmpty()) {
            std::string patFileName = FileGeomPattern.getValue();
            Base::FileInfo tfi(patFileName);
            if (tfi.isReadable()) {
                    setupPatIncluded();
            }
        }
    }

    makeLineSets();
    DrawViewPart::onDocumentRestored();
}

void DrawViewSection::setupObject()
{
    //by this point DVS should have a name and belong to a document
    setupSvgIncluded();
    setupPatIncluded();

    DrawViewPart::setupObject();
}

//hatch file routines

//create geometric hatch lines
void DrawViewSection::makeLineSets(void)
{
//    Base::Console().Message("DVS::makeLineSets()\n");
    if (PatIncluded.isEmpty()) {
        return;
    }

    std::string fileSpec = PatIncluded.getValue();
    Base::FileInfo fi(fileSpec);
    std::string ext = fi.extension();
    if (!fi.isReadable()) {
        Base::Console().Message("%s can not read hatch file: %s\n", getNameInDocument(), fileSpec.c_str());
        return;
    }

    if ( (ext == "pat") ||
         (ext == "PAT") ) {
        if ((!fileSpec.empty())  &&
            (!NameGeomPattern.isEmpty())) {
            m_lineSets.clear();
            m_lineSets = DrawGeomHatch::makeLineSets(fileSpec, NameGeomPattern.getValue());
        }
    }
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

void DrawViewSection::setupPatIncluded()
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

// Parameter fetching routines

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

    return hGrp->GetBool("debugSection", false);
}

int DrawViewSection::prefCutSurface(void) const
{
//    Base::Console().Message("DVS::prefCutSurface()\n");
    Base::Reference<ParameterGrp>hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");

    return hGrp->GetInt("CutSurfaceDisplay", 2);   //default to SvgHatch
}

bool DrawViewSection::showSectionEdges(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    return (hGrp->GetBool("ShowSectionEdges", true));
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewSectionPython, TechDraw::DrawViewSection)
template<> const char* TechDraw::DrawViewSectionPython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderDrawingView";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewSection>;
}
