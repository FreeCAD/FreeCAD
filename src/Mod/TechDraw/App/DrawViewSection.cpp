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

//DrawViewSection processing overview

//execute
//    sectionExec(getShapeToCut())

//sectionExec
//    makeSectionCut(baseShape)

//makeSectionCut (separate thread)
//    m_cuttingTool = makeCuttingTool (DVSTool.brep)
//    m_cutPieces = (baseShape - m_cuttingTool) (DVSCutPieces.brep)

//onSectionCutFinished
//    m_preparedShape = prepareShape(m_cutPieces) - centered, scaled, rotated
//    geometryObject = DVP::buildGeometryObject(m_preparedShape)  (HLR)

//postHlrTasks
//    faceIntersections = findSectionPlaneIntersections
//    m_sectionTopoDSFaces = alignSectionFaces(faceIntersections)
//    m_tdSectionFaces = makeTDSectionFaces(m_sectionTopoDSFaces)

#include "PreCompiled.h"

#ifndef _PreComp_
#include <chrono>
#include <sstream>
#include <QtConcurrentRun>
#include <Bnd_Box.hxx>
#include <BRep_Builder.hxx>
#include <BRepBndLib.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>

#include <Mod/Part/App/PartFeature.h>

#include "DrawGeomHatch.h"
#include "DrawHatch.h"
#include "DrawProjGroupItem.h"
#include "DrawProjGroupItem.h"
#include "DrawUtil.h"
#include "DrawUtil.h"
#include "EdgeWalker.h"
#include "Geometry.h"
#include "Geometry.h"
#include "GeometryObject.h"
#include "GeometryObject.h"
#include "HatchLine.h"

#include "DrawViewSection.h"

using namespace TechDraw;

using DU = DrawUtil;

const char* DrawViewSection::SectionDirEnums[]= {"Right",
                                            "Left",
                                            "Up",
                                            "Down",
                                            "Aligned",
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
    m_waitingForCut(false),
    m_shapeSize(0.0)
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

TopoDS_Shape DrawViewSection::getShapeToCut()
{
//    Base::Console().Message("DVS::getShapeToCut()\n");
    App::DocumentObject* base = BaseView.getValue();
    TechDraw::DrawViewPart* dvp = nullptr;
    if (!base ||
        !base->getTypeId().isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        //this can probably only happen with scripting
        return TopoDS_Shape();
    } else {
        dvp = static_cast<TechDraw::DrawViewPart*>(base);
    }
    TopoDS_Shape shapeToCut = dvp->getSourceShape();
    if (FuseBeforeCut.getValue()) {
        shapeToCut = dvp->getSourceShapeFused();
    }
    return shapeToCut;
}

App::DocumentObjectExecReturn *DrawViewSection::execute()
{
//    Base::Console().Message("DVS::execute() - %s\n", getNameInDocument());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    if (!isBaseValid()) {
        return new App::DocumentObjectExecReturn("BaseView object not found");
    }

    TopoDS_Shape baseShape = getShapeToCut();

    if (baseShape.IsNull()) {
        return DrawView::execute();
    }

    m_saveShape = baseShape;        //save shape for 2nd pass

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

bool DrawViewSection::isBaseValid() const
{
    App::DocumentObject* base = BaseView.getValue();
    if (base &&
        base->getTypeId().isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        return true;
    }
    return false;
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
    Base::Vector3d orgPnt = SectionOrigin.getValue();

    if(!isReallyInBox(gp_Pnt(orgPnt.x, orgPnt.y, orgPnt.z), centerBox)) {
        Base::Console().Warning("DVS: SectionOrigin doesn't intersect part in %s\n", getNameInDocument());
    }
   m_shapeSize = sqrt(centerBox.SquareExtent());

    // We need to copy the shape to not modify the BRepstructure
    BRepBuilderAPI_Copy BuilderCopy(baseShape);
    TopoDS_Shape myShape = BuilderCopy.Shape();
    m_saveShape = myShape;        //save shape for 2nd pass

    if (debugSection()) {
        BRepTools::Write(myShape, "DVSCopy.brep");            //debug
    }

    m_cuttingTool = makeCuttingTool(m_shapeSize);

    if (debugSection()) {
        BRepTools::Write(m_cuttingTool, "DVSTool.brep");              //debug
    }

// perform cut
    BRep_Builder builder;
    TopoDS_Compound cutPieces;
    builder.MakeCompound(cutPieces);
    TopExp_Explorer expl(myShape, TopAbs_SOLID);
    for (; expl.More(); expl.Next()) {
        const TopoDS_Solid& s = TopoDS::Solid(expl.Current());
        BRepAlgoAPI_Cut mkCut(s, m_cuttingTool);
        if (!mkCut.IsDone()) {
            Base::Console().Warning("DVS: Section cut has failed in %s\n", getNameInDocument());
            continue;
        }
        builder.Add(cutPieces, mkCut.Shape());
    }

    // cutPieces contains result of cutting each subshape in baseShape with tool
    m_cutPieces = cutPieces;

    if (debugSection()) {
        BRepTools::Write(cutPieces, "DVSCutPieces.brep");         //debug
    }

// check for error in cut
    Bnd_Box testBox;
    BRepBndLib::AddOptimal(m_cutPieces, testBox);
    testBox.SetGap(0.0);
    if (testBox.IsVoid()) {           //prism & input don't intersect.  rawShape is garbage, don't bother.
        Base::Console().Warning("DVS::makeSectionCut - prism & input don't intersect - %s\n", Label.getValue());
        return;
    }

    if (debugSection()) {
        BRepTools::Write(m_cutPieces, "DVSRawShapeAfter.brep");         //debug
    }

    waitingForCut(false);
}

//position, scale and rotate shape for  buildGeometryObject
TopoDS_Shape DrawViewSection::prepareShape(const TopoDS_Shape& rawShape,
                                           double shapeSize)
{
//    Base::Console().Message("DVS::prepareShape - %s - rawShape.IsNull: %d shapeSize: %.3f\n",
//                            getNameInDocument(), rawShape.IsNull(), shapeSize);
    (void) shapeSize;    //shapeSize is not used in this base class, but is interesting for
                         //derived classes
    // build display geometry as in DVP, with minor mods
    TopoDS_Shape preparedShape;
    try {
        Base::Vector3d origin(0.0, 0.0, 0.0);
        m_projectionCS = getProjectionCS(origin);
        gp_Pnt inputCenter;
        inputCenter = TechDraw::findCentroid(rawShape,
                                             m_projectionCS);
        Base::Vector3d centroid(inputCenter.X(),
                                inputCenter.Y(),
                                inputCenter.Z());

        preparedShape = TechDraw::moveShape(rawShape,
                                            centroid * -1.0);
        m_cutShape = preparedShape;
        m_saveCentroid = centroid;

        preparedShape   = TechDraw::scaleShape(preparedShape,
                                               getScale());

        if (!DrawUtil::fpCompare(Rotation.getValue(), 0.0)) {
            preparedShape = TechDraw::rotateShape(preparedShape,
                                                  m_projectionCS,
                                                  Rotation.getValue());
        }
        if (debugSection()) {
            BRepTools::Write(m_cutShape, "DVSmCutShape.brep");         //debug
//            DrawUtil::dumpCS("DVS::makeSectionCut - CS to GO", viewAxis);
        }

    }
    catch (Standard_Failure& e1) {
        Base::Console().Warning("DVS::prepareShape - failed to build shape %s - %s **\n",
                                getNameInDocument(), e1.GetMessageString());
    }
    return preparedShape;
}

TopoDS_Shape DrawViewSection::makeCuttingTool(double shapeSize)
{
//    Base::Console().Message("DVS::makeCuttingTool(%.3f) - %s\n", shapeSize, getNameInDocument());
    // Make the extrusion face
    gp_Pln pln = getSectionPlane();
    gp_Dir gpNormal = pln.Axis().Direction();
    BRepBuilderAPI_MakeFace mkFace(pln, -shapeSize, shapeSize, -shapeSize, shapeSize);
    TopoDS_Face aProjFace = mkFace.Face();
    if(aProjFace.IsNull()) {
        return TopoDS_Shape();
    }
    if (debugSection()){
        BRepTools::Write(aProjFace, "DVSSectionFace.brep");          //debug
    }
    gp_Vec extrudeDir = shapeSize * gp_Vec(gpNormal);
    return BRepPrimAPI_MakePrism(aProjFace, extrudeDir, false, true).Shape();
}

void DrawViewSection::onSectionCutFinished()
{
//    Base::Console().Message("DVS::onSectionCutFinished() - %s\n", getNameInDocument());
    QObject::disconnect(connectCutWatcher);

    showProgressMessage(getNameInDocument(), "has finished making section cut");

    m_preparedShape = prepareShape(getShapeToPrepare(), m_shapeSize);
    if (debugSection()) {
        BRepTools::Write(m_preparedShape, "DVSPreparedShape.brep");              //debug
    }

    postSectionCutTasks();

    //display geometry for cut shape is in geometryObject as in DVP
    m_tempGeometryObject = buildGeometryObject(m_preparedShape, getSectionCS());
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
    TopoDS_Compound faceIntersections = findSectionPlaneIntersections(getShapeToIntersect());
    if (faceIntersections.IsNull()) {
        requestPaint();
        return;
    }

    TopoDS_Shape centeredFaces = TechDraw::moveShape(faceIntersections,
                                                       m_saveCentroid * -1.0);

    TopoDS_Shape scaledSection = TechDraw::scaleShape(centeredFaces,
                                                      getScale());
    if (!DrawUtil::fpCompare(Rotation.getValue(), 0.0)) {
        scaledSection = TechDraw::rotateShape(scaledSection,
                                              getProjectionCS(),
                                              Rotation.getValue());
    }
    if (debugSection()) {
        BRepTools::Write(faceIntersections, "DVSFaceIntersections.brep");              //debug
    }

    m_sectionTopoDSFaces = alignSectionFaces(faceIntersections);
    if (debugSection()) {
        BRepTools::Write(m_sectionTopoDSFaces, "DVSTopoSectionFaces.brep");              //debug
    }
    m_tdSectionFaces = makeTDSectionFaces(m_sectionTopoDSFaces);


    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(BaseView.getValue());
    if (dvp) {
        dvp->requestPaint();  //to refresh section line
    }
    requestPaint();         //this will be a duplicate paint if we are making a standalone ComplexSection
}

//activities that depend on a valid section cut
void DrawViewSection::postSectionCutTasks()
{
//    Base::Console().Message("DVS::postSectionCutTasks()\n");
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
//! the original algo finds the intersections first then transforms them to match the centered, rotated
//! and scaled cut shape.  Piecewise complex sections need to intersect the final cut shape (which in this
//! case is a compound of individual cuts) with the "effective" (flattened) section plane.
TopoDS_Compound DrawViewSection::findSectionPlaneIntersections(const TopoDS_Shape& shape)
{
//    Base::Console().Message("DVS::findSectionPlaneIntersections() - %s\n", getNameInDocument());
    if(shape.IsNull()){
        // this shouldn't happen
//        Base::Console().Warning("DrawViewSection::findSectionPlaneInter - %s - input shape is Null\n", getNameInDocument());
        return TopoDS_Compound();
    }

    gp_Pln plnSection = getSectionPlane();
    if (debugSection()) {
        BRepBuilderAPI_MakeFace mkFace(plnSection, -m_shapeSize, m_shapeSize, -m_shapeSize, m_shapeSize);
        BRepTools::Write(mkFace.Face(), "DVSSectionPlane.brep");            //debug
        BRepTools::Write(shape, "DVSShapeToIntersect.brep)");
    }
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

//move section faces to line up with cut shape
TopoDS_Compound DrawViewSection::alignSectionFaces(TopoDS_Shape faceIntersections)
{
//    Base::Console().Message("DVS::alignSectionFaces()\n");
    TopoDS_Compound sectionFaces;
    TopoDS_Shape centeredShape = TechDraw::moveShape(faceIntersections,
                                                     getOriginalCentroid() * -1.0);

    TopoDS_Shape scaledSection = TechDraw::scaleShape(centeredShape,
                                                      getScale());
    if (!DrawUtil::fpCompare(Rotation.getValue(), 0.0)) {
        scaledSection = TechDraw::rotateShape(scaledSection,
                                              getSectionCS(),
                                              Rotation.getValue());
    }
    if (debugSection()) {
        BRepTools::Write(scaledSection, "DVSScaledFaces.brep");            //debug
    }

    return mapToPage(scaledSection);
}

TopoDS_Compound DrawViewSection::mapToPage(TopoDS_Shape& shapeToAlign)
{
    // shapeToAlign is compound of TopoDS_Face intersections, but aligned to pln(origin, sectionNormal)
    // needs to be aligned to paper plane (origin, stdZ);
    //project the faces in the shapeToAlign, build new faces from the resulting wires and
    //combine everything into a compound of faces

    BRep_Builder builder;
    TopoDS_Compound result;
    builder.MakeCompound(result);

    TopExp_Explorer expFace(shapeToAlign, TopAbs_FACE);
    for (int iFace = 1; expFace.More(); expFace.Next(), iFace++) {
        const TopoDS_Face& face = TopoDS::Face(expFace.Current());
        std::vector<TopoDS_Wire> faceWires;
        TopExp_Explorer expWires(face, TopAbs_WIRE);
        for ( ; expWires.More(); expWires.Next()) {
            const TopoDS_Wire& wire = TopoDS::Wire(expWires.Current());
            TopoDS_Shape projectedShape = GeometryObject::projectSimpleShape(wire, getSectionCS());
            std::vector<TopoDS_Edge> wireEdges;
            //projectedShape is just a bunch of edges. we have to rebuild the wire.
            TopExp_Explorer expEdges(projectedShape, TopAbs_EDGE);
            for ( ; expEdges.More(); expEdges.Next()) {
                const TopoDS_Edge& edge = TopoDS::Edge(expEdges.Current());
                wireEdges.push_back(edge);
            }
            TopoDS_Wire cleanWire = EdgeWalker::makeCleanWire(wireEdges, 2.0 * EWTOLERANCE);
            faceWires.push_back(cleanWire);
        }
        if (debugSection()) {
            std::stringstream ss;
            ss << "DVSFaceWires" << iFace << ".brep";
            BRepTools::Write(DrawUtil::vectorToCompound(faceWires), ss.str().c_str());          //debug
        }
        //first wire should be the outer boundary of the face
        BRepBuilderAPI_MakeFace mkFace(faceWires.front());
        int wireCount = faceWires.size();
        for (int iWire = 1; iWire < wireCount; iWire++) {
            //make holes in the face with the rest of the wires
            mkFace.Add(faceWires.at(iWire));
        }
        builder.Add(result, mkFace.Face());
        if (debugSection()) {
            std::stringstream ss;
            ss << "DVSFaceFromWires" << iFace << ".brep";
            BRepTools::Write(mkFace.Face(), ss.str().c_str());          //debug
        }
    }

    return result;
}

//turn OCC section faces into TD geometry
std::vector<TechDraw::FacePtr> DrawViewSection::makeTDSectionFaces(TopoDS_Compound topoDSFaces)
{
//    Base::Console().Message("DVS::makeTDSectionFaces()\n");
    std::vector<TechDraw::FacePtr> tdSectionFaces;
    TopExp_Explorer sectionExpl(topoDSFaces, TopAbs_FACE);
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

    return tdSectionFaces;
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
    } else if (sectionName == "Aligned") {
        //if aligned, we don't get our direction from the base view
        Base::Vector3d sectionNormal = SectionNormal.getValue();
        dvsDir = gp_Dir(sectionNormal.x, sectionNormal.y, sectionNormal.z);
        Base::Vector3d sectionXDir = XDirection.getValue();
        dvsXDir = gp_Dir(sectionXDir.x, sectionXDir.y, sectionXDir.z);
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
    TopExp_Explorer expl(m_sectionTopoDSFaces, TopAbs_FACE);
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
