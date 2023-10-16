/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2016, 2022 WandererFan <wandererfan@gmail.com>          *
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

// DrawViewSection processing overview

// execute
//     sectionExec(getShapeToCut())

// sectionExec
//     makeSectionCut(baseShape)

// makeSectionCut (separate thread)
//     m_cuttingTool = makeCuttingTool (DVSTool.brep)
//     m_cutPieces = (baseShape - m_cuttingTool) (DVSCutPieces.brep)

// onSectionCutFinished
//     m_preparedShape = prepareShape(m_cutPieces) - centered, scaled, rotated
//     geometryObject = DVP::buildGeometryObject(m_preparedShape)  (HLR)

// postHlrTasks
//     faceIntersections = findSectionPlaneIntersections
//     m_sectionTopoDSFaces = alignSectionFaces(faceIntersections)
//     m_tdSectionFaces = makeTDSectionFaces(m_sectionTopoDSFaces)

#include "PreCompiled.h"

#ifndef _PreComp_
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <QtConcurrentRun>
#include <ShapeAnalysis.hxx>
#include <ShapeFix_Shape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <chrono>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <sstream>
#endif

#include <App/Document.h>
#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>

#include <Mod/Part/App/PartFeature.h>

#include "DrawGeomHatch.h"
#include "DrawHatch.h"
#include "DrawUtil.h"
#include "DrawViewDetail.h"
#include "EdgeWalker.h"
#include "GeometryObject.h"
#include "Preferences.h"

#include "DrawViewSection.h"

using namespace TechDraw;

using DU = DrawUtil;

// class to store geometry of points where the section line changes direction
ChangePoint::ChangePoint(QPointF location, QPointF preDirection, QPointF postDirection)
{
    m_location = location;
    m_preDirection = preDirection;
    m_postDirection = postDirection;
}

ChangePoint::ChangePoint(gp_Pnt location, gp_Dir preDirection, gp_Dir postDirection)
{
    m_location.setX(location.X());
    m_location.setY(location.Y());
    m_preDirection.setX(preDirection.X());
    m_preDirection.setY(preDirection.Y());
    m_postDirection.setX(postDirection.X());
    m_postDirection.setY(postDirection.Y());
}

void ChangePoint::scale(double scaleFactor)
{
    m_location = m_location * scaleFactor;
}

const char* DrawViewSection::SectionDirEnums[] =
    {"Right", "Left", "Up", "Down", "Aligned", nullptr};

const char* DrawViewSection::CutSurfaceEnums[] = {"Hide", "Color", "SvgHatch", "PatHatch", nullptr};

//===========================================================================
// DrawViewSection
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewSection, TechDraw::DrawViewPart)

DrawViewSection::DrawViewSection()
    : m_waitingForCut(false)
    , m_shapeSize(0.0)
{
    static const char* sgroup = "Section";
    static const char* fgroup = "Cut Surface Format";
    static const char* ggroup = "Cut Operation";
    static const char* agroup = "Appearance";

    // general section properties
    ADD_PROPERTY_TYPE(SectionSymbol,
                      (""),
                      sgroup,
                      App::Prop_Output,
                      "The identifier for this section");
    ADD_PROPERTY_TYPE(BaseView,
                      (nullptr),
                      sgroup,
                      App::Prop_None,
                      "2D View source for this Section");
    BaseView.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(SectionNormal,
                      (0, 0, 1.0),
                      sgroup,
                      App::Prop_None,
                      "Section Plane normal direction");// direction of extrusion
                                                        // of cutting prism
    ADD_PROPERTY_TYPE(SectionOrigin, (0, 0, 0), sgroup, App::Prop_None, "Section Plane Origin");

    // TODO: SectionDirection is a legacy from when SectionViews were only
    // available along cardinal directions.  It should be made obsolete and
    // replaced with Aligned sections and local unit vectors.
    SectionDirection.setEnums(SectionDirEnums);
    ADD_PROPERTY_TYPE(SectionDirection,
                      ((long)0),
                      sgroup,
                      App::Prop_None,
                      "Orientation of this Section in the Base View");

    // properties related to the cut operation
    ADD_PROPERTY_TYPE(FuseBeforeCut,
                      (false),
                      ggroup,
                      App::Prop_None,
                      "Merge Source(s) into a single shape before cutting");
    ADD_PROPERTY_TYPE(TrimAfterCut,
                      (false),
                      ggroup,
                      App::Prop_None,
                      "Trim the resulting shape after the section cut");
    ADD_PROPERTY_TYPE(UsePreviousCut,
                      (Preferences::SectionUsePreviousCut()),
                      ggroup,
                      App::Prop_None,
                      "Use the cut shape from the base view instead of the original object");

    // properties related to the display of the cut surface
    CutSurfaceDisplay.setEnums(CutSurfaceEnums);
    ADD_PROPERTY_TYPE(CutSurfaceDisplay,
                      (prefCutSurface()),
                      fgroup,
                      App::Prop_None,
                      "Appearance of Cut Surface");
    ADD_PROPERTY_TYPE(FileHatchPattern,
                      (DrawHatch::prefSvgHatch()),
                      fgroup,
                      App::Prop_None,
                      "The hatch pattern file for the cut surface");
    ADD_PROPERTY_TYPE(FileGeomPattern,
                      (DrawGeomHatch::prefGeomHatchFile()),
                      fgroup,
                      App::Prop_None,
                      "The PAT pattern file for geometric hatching");

    ADD_PROPERTY_TYPE(SvgIncluded,
                      (""),
                      fgroup,
                      App::Prop_None,
                      "Embedded Svg hatch file. System use only.");// n/a to end users
    ADD_PROPERTY_TYPE(PatIncluded,
                      (""),
                      fgroup,
                      App::Prop_None,
                      "Embedded Pat pattern file. System use only.");// n/a to end users
    ADD_PROPERTY_TYPE(NameGeomPattern,
                      (DrawGeomHatch::prefGeomHatchName()),
                      fgroup,
                      App::Prop_None,
                      "The pattern name for geometric hatching");
    ADD_PROPERTY_TYPE(HatchScale, (1.0), fgroup, App::Prop_None, "Hatch pattern size adjustment");
    ADD_PROPERTY_TYPE(HatchRotation,
                      (0.0),
                      fgroup,
                      App::Prop_None,
                      "Rotation of hatch pattern in degrees anti-clockwise");
    ADD_PROPERTY_TYPE(HatchOffset, (0.0, 0.0, 0.0), fgroup, App::Prop_None, "Hatch pattern offset");

    ADD_PROPERTY_TYPE(SectionLineStretch, (1.0), agroup, App::Prop_None,
                      "Adjusts the length of the section line.  1.0 is normal length.  1.1 would be 10% longer, 0.9 would be 10% shorter.");

    getParameters();

    std::string hatchFilter("Svg files (*.svg *.SVG);;All files (*)");
    FileHatchPattern.setFilter(hatchFilter);
    hatchFilter = ("PAT files (*.pat *.PAT);;All files (*)");
    FileGeomPattern.setFilter(hatchFilter);

    SvgIncluded.setStatus(App::Property::ReadOnly, true);
    PatIncluded.setStatus(App::Property::ReadOnly, true);
    // SectionNormal is used instead to Direction
    Direction.setStatus(App::Property::ReadOnly, true);
    SectionDirection.setStatus(App::Property::Hidden, true);
    SectionDirection.setStatus(App::Property::ReadOnly, true);
}

DrawViewSection::~DrawViewSection()
{
    // don't destroy this object while it has dependent threads running
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

    if (Scale.isTouched() || Direction.isTouched() || BaseView.isTouched()
        || SectionNormal.isTouched() || SectionOrigin.isTouched() || Rotation.isTouched()) {
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
        // tarfu
        DrawViewPart::onChanged(prop);
        return;
    }

    if (prop == &SectionNormal) {
        Direction.setValue(SectionNormal.getValue());
        return;
    }
    else if (prop == &SectionSymbol) {
        if (getBaseDVP()) {
            getBaseDVP()->requestPaint();
        }
        return;
    }
    else if (prop == &CutSurfaceDisplay) {
        if (CutSurfaceDisplay.isValue("PatHatch")) {
            makeLineSets();
        }
        requestPaint();
        return;
    }
    else if (prop == &FileHatchPattern) {
        replaceSvgIncluded(FileHatchPattern.getValue());
        requestPaint();
        return;
    }
    else if (prop == &FileGeomPattern) {
        replacePatIncluded(FileGeomPattern.getValue());
        makeLineSets();
        requestPaint();
        return;
    }
    else if (prop == &NameGeomPattern) {
        makeLineSets();
        requestPaint();
        return;
    }
    else if (prop == &BaseView) {
        // if the BaseView is a Section, then the option of using UsePreviousCut is
        // valid.
        if (BaseView.getValue() && BaseView.getValue()->getTypeId().isDerivedFrom(
                TechDraw::DrawViewSection::getClassTypeId())) {
            UsePreviousCut.setStatus(App::Property::ReadOnly, false);
        }
        else {
            UsePreviousCut.setStatus(App::Property::ReadOnly, true);
        }
    } else if (prop == &SectionLineStretch) {
        BaseView.getValue()->touch();
    }

    DrawView::onChanged(prop);
}

TopoDS_Shape DrawViewSection::getShapeToCut()
{
    //    Base::Console().Message("DVS::getShapeToCut() - %s\n",
    //    getNameInDocument());
    App::DocumentObject* base = BaseView.getValue();
    TechDraw::DrawViewPart* dvp = nullptr;
    TechDraw::DrawViewSection* dvs = nullptr;
    TechDraw::DrawViewDetail* dvd = nullptr;
    if (!base) {
        return TopoDS_Shape();
    }

    TopoDS_Shape shapeToCut;
    if (base->getTypeId().isDerivedFrom(TechDraw::DrawViewSection::getClassTypeId())) {
        dvs = static_cast<TechDraw::DrawViewSection*>(base);
        shapeToCut = dvs->getShapeToCut();
        if (UsePreviousCut.getValue()) {
            shapeToCut = dvs->getCutShapeRaw();
        }
    }
    else if (base->getTypeId().isDerivedFrom(TechDraw::DrawViewDetail::getClassTypeId())) {
        dvd = static_cast<TechDraw::DrawViewDetail*>(base);
        shapeToCut = dvd->getDetailShape();
    }
    else if (base->getTypeId().isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        dvp = static_cast<TechDraw::DrawViewPart*>(base);
        shapeToCut = dvp->getSourceShape();
        if (FuseBeforeCut.getValue()) {
            shapeToCut = dvp->getSourceShape(true);
        }
    }
    else {
        Base::Console().Message("DVS::getShapeToCut - base is weird\n");
        return TopoDS_Shape();
    }
    return shapeToCut;
}

TopoDS_Shape DrawViewSection::getShapeForDetail() const
{
    return ShapeUtils::rotateShape(getCutShape(), getProjectionCS(), Rotation.getValue());
}

App::DocumentObjectExecReturn* DrawViewSection::execute()
{
    //    Base::Console().Message("DVS::execute() - %s\n", getNameInDocument());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    if (!isBaseValid()) {
        return new App::DocumentObjectExecReturn("BaseView object not found");
    }

    if (waitingForCut() || waitingForHlr()) {
        return DrawView::execute();
    }

    TopoDS_Shape baseShape = getShapeToCut();

    if (baseShape.IsNull()) {
        return DrawView::execute();
    }

    // is SectionOrigin valid?
    Bnd_Box centerBox;
    BRepBndLib::AddOptimal(baseShape, centerBox);
    centerBox.SetGap(0.0);
    Base::Vector3d orgPnt = SectionOrigin.getValue();

    if (!isReallyInBox(gp_Pnt(orgPnt.x, orgPnt.y, orgPnt.z), centerBox)) {
        Base::Console().Warning("DVS: SectionOrigin doesn't intersect part in %s\n",
                                getNameInDocument());
    }

    // save important info for later use
    m_shapeSize = sqrt(centerBox.SquareExtent());
    m_saveShape = baseShape;

    bool haveX = checkXDirection();
    if (!haveX) {
        // block touch/onChanged stuff
        Base::Vector3d newX = getXDirection();
        XDirection.setValue(newX);
        XDirection.purgeTouched();// don't trigger updates!
                                  // unblock
    }

    sectionExec(baseShape);
    addShapes2d();

    return DrawView::execute();
}

bool DrawViewSection::isBaseValid() const
{
    App::DocumentObject* base = BaseView.getValue();
    if (base && base->getTypeId().isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        return true;
    }
    return false;
}

void DrawViewSection::sectionExec(TopoDS_Shape& baseShape)
{
    //    Base::Console().Message("DVS::sectionExec() - %s baseShape.IsNull:
    //    %d\n",
    //                            getNameInDocument(), baseShape.IsNull());

    if (waitingForHlr() || waitingForCut()) {
        return;
    }

    if (baseShape.IsNull()) {
        // should be caught before this
        return;
    }

    m_cuttingTool = makeCuttingTool(m_shapeSize);

    try {
        // note that &m_cutWatcher in the third parameter is not strictly required,
        // but using the 4 parameter signature instead of the 3 parameter signature
        // prevents clazy warning:
        // https://github.com/KDE/clazy/blob/1.11/docs/checks/README-connect-3arg-lambda.md
        connectCutWatcher =
            QObject::connect(&m_cutWatcher, &QFutureWatcherBase::finished, &m_cutWatcher, [this] {
                this->onSectionCutFinished();
            });

        // We create a lambda closure to hold a copy of baseShape.
        // This is important because this variable might be local to the calling
        // function and might get destructed before the parallel processing finishes.
        auto lambda = [this, baseShape]{this->makeSectionCut(baseShape);};
        m_cutFuture = QtConcurrent::run(std::move(lambda));
        m_cutWatcher.setFuture(m_cutFuture);
        waitingForCut(true);
    }
    catch (...) {
        Base::Console().Message("DVS::sectionExec - failed to make section cut");
        return;
    }
}

void DrawViewSection::makeSectionCut(const TopoDS_Shape& baseShape)
{
    //    Base::Console().Message("DVS::makeSectionCut() - %s - baseShape.IsNull:
    //    %d\n",
    //                            getNameInDocument(), baseShape.IsNull());

    showProgressMessage(getNameInDocument(), "is making section cut");

    // We need to copy the shape to not modify the BRepstructure
    BRepBuilderAPI_Copy BuilderCopy(baseShape);
    TopoDS_Shape myShape = BuilderCopy.Shape();
    m_saveShape = myShape;// save shape for 2nd pass

    if (debugSection()) {
        BRepTools::Write(myShape, "DVSCopy.brep");// debug
    }

    if (debugSection()) {
        BRepTools::Write(m_cuttingTool, "DVSTool.brep");// debug
    }

    // perform the cut. We cut each solid in myShape individually to avoid issues
    // where a compound BaseShape does not cut correctly.
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
        BRepTools::Write(cutPieces, "DVSCutPieces1.brep");// debug
    }

    // second cut if requested.  Sometimes the first cut includes extra uncut
    // pieces.
    if (trimAfterCut()) {
        BRepAlgoAPI_Cut mkCut2(cutPieces, m_cuttingTool);
        if (mkCut2.IsDone()) {
            m_cutPieces = mkCut2.Shape();
            if (debugSection()) {
                BRepTools::Write(m_cutPieces, "DVSCutPieces2.brep");// debug
            }
        }
    }

    // check for error in cut
    Bnd_Box testBox;
    BRepBndLib::AddOptimal(m_cutPieces, testBox);
    testBox.SetGap(0.0);
    if (testBox.IsVoid()) {// prism & input don't intersect.  rawShape is
                           // garbage, don't bother.
        Base::Console().Warning("DVS::makeSectionCut - prism & input don't intersect - %s\n",
                                Label.getValue());
        return;
    }

    waitingForCut(false);
}

//! position, scale and rotate shape for  buildGeometryObject
//! save the cut shape for further processing
TopoDS_Shape DrawViewSection::prepareShape(const TopoDS_Shape& rawShape, double shapeSize)
{
    //    Base::Console().Message("DVS::prepareShape - %s - rawShape.IsNull: %d
    //    shapeSize: %.3f\n",
    //                            getNameInDocument(), rawShape.IsNull(),
    //                            shapeSize);
    (void)shapeSize;// shapeSize is not used in this base class, but is
                    // interesting for derived classes
    // build display geometry as in DVP, with minor mods
    TopoDS_Shape preparedShape;
    try {
        Base::Vector3d origin(0.0, 0.0, 0.0);
        m_projectionCS = getProjectionCS(origin);
        gp_Pnt inputCenter;
        inputCenter = ShapeUtils::findCentroid(rawShape, m_projectionCS);
        Base::Vector3d centroid(inputCenter.X(), inputCenter.Y(), inputCenter.Z());

        m_cutShapeRaw = rawShape;
        preparedShape = ShapeUtils::moveShape(rawShape, centroid * -1.0);
        m_cutShape = preparedShape;
        m_saveCentroid = centroid;

        preparedShape = ShapeUtils::scaleShape(preparedShape, getScale());

        if (!DrawUtil::fpCompare(Rotation.getValue(), 0.0)) {
            preparedShape =
                ShapeUtils::rotateShape(preparedShape, m_projectionCS, Rotation.getValue());
        }
        if (debugSection()) {
            BRepTools::Write(m_cutShape, "DVSCutShape.brep");// debug
            //            DrawUtil::dumpCS("DVS::makeSectionCut - CS to GO",
            //            viewAxis);
        }
    }
    catch (Standard_Failure& e1) {
        Base::Console().Warning("DVS::prepareShape - failed to build shape %s - %s **\n",
                                getNameInDocument(),
                                e1.GetMessageString());
    }
    return preparedShape;
}

TopoDS_Shape DrawViewSection::makeCuttingTool(double shapeSize)
{
    //    Base::Console().Message("DVS::makeCuttingTool(%.3f) - %s\n", shapeSize,
    //    getNameInDocument());
    // Make the extrusion face
    gp_Pln pln = getSectionPlane();
    gp_Dir gpNormal = pln.Axis().Direction();
    BRepBuilderAPI_MakeFace mkFace(pln, -shapeSize, shapeSize, -shapeSize, shapeSize);
    TopoDS_Face aProjFace = mkFace.Face();
    if (aProjFace.IsNull()) {
        return TopoDS_Shape();
    }
    if (debugSection()) {
        BRepTools::Write(aProjFace, "DVSSectionFace.brep");// debug
    }
    gp_Vec extrudeDir = shapeSize * gp_Vec(gpNormal);
    return BRepPrimAPI_MakePrism(aProjFace, extrudeDir, false, true).Shape();
}

void DrawViewSection::onSectionCutFinished()
{
    //    Base::Console().Message("DVS::onSectionCutFinished() - %s\n",
    //    getNameInDocument());
    QObject::disconnect(connectCutWatcher);

    showProgressMessage(getNameInDocument(), "has finished making section cut");

    m_preparedShape = prepareShape(getShapeToPrepare(), m_shapeSize);
    if (debugSection()) {
        BRepTools::Write(m_preparedShape, "DVSPreparedShape.brep");// debug
    }

    postSectionCutTasks();

    // display geometry for cut shape is in geometryObject as in DVP
    m_tempGeometryObject = buildGeometryObject(m_preparedShape, getProjectionCS());
}

// activities that depend on updated geometry object
void DrawViewSection::postHlrTasks(void)
{
    //    Base::Console().Message("DVS::postHlrTasks() - %s\n",
    //    getNameInDocument());

    DrawViewPart::postHlrTasks();

    // second pass if required
    if (ScaleType.isValue("Automatic") && !checkFit()) {
        double newScale = autoScale();
        Scale.setValue(newScale);
        Scale.purgeTouched();
        sectionExec(m_saveShape);
    }
    overrideKeepUpdated(false);

    // build section face geometry
    TopoDS_Compound faceIntersections = findSectionPlaneIntersections(getShapeToIntersect());
    if (faceIntersections.IsNull()) {
        requestPaint();
        return;
    }
    if (debugSection()) {
        BRepTools::Write(faceIntersections, "DVSFaceIntersections.brep");// debug
    }

    TopoDS_Shape centeredFaces = ShapeUtils::moveShape(faceIntersections, m_saveCentroid * -1.0);

    TopoDS_Shape scaledSection = ShapeUtils::scaleShape(centeredFaces, getScale());
    if (!DrawUtil::fpCompare(Rotation.getValue(), 0.0)) {
        scaledSection =
            ShapeUtils::rotateShape(scaledSection, getProjectionCS(), Rotation.getValue());
    }

    m_sectionTopoDSFaces = alignSectionFaces(faceIntersections);
    if (debugSection()) {
        BRepTools::Write(m_sectionTopoDSFaces, "DVSTopoSectionFaces.brep");// debug
    }
    m_tdSectionFaces = makeTDSectionFaces(m_sectionTopoDSFaces);

    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(BaseView.getValue());
    if (dvp) {
        dvp->requestPaint();// to refresh section line
    }
    requestPaint();// this will be a duplicate paint if we are making a
                   // standalone ComplexSection
}

// activities that depend on a valid section cut
void DrawViewSection::postSectionCutTasks()
{
    //    Base::Console().Message("DVS::postSectionCutTasks()\n");
    std::vector<App::DocumentObject*> children = getInList();
    for (auto& c : children) {
        if (c->getTypeId().isDerivedFrom(DrawViewPart::getClassTypeId())) {
            // details or sections of this need cut shape
            c->recomputeFeature();
        }
    }
}

bool DrawViewSection::waitingForResult() const
{
    if (DrawViewPart::waitingForResult() || waitingForCut()) {
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

//! tries to find the intersection of the section plane with the shape giving a
//! collection of planar faces the original algo finds the intersections first
//! then transforms them to match the centered, rotated and scaled cut shape.
//! Aligned complex sections need to intersect the final cut shape (which in
//! this case is a compound of individual cuts) with the "effective" (flattened)
//! section plane.
TopoDS_Compound DrawViewSection::findSectionPlaneIntersections(const TopoDS_Shape& shape)
{
    //    Base::Console().Message("DVS::findSectionPlaneIntersections() - %s\n",
    //    getNameInDocument());
    if (shape.IsNull()) {
        // this shouldn't happen
        Base::Console().Warning(
            "DrawViewSection::findSectionPlaneInter - %s - input shape is Null\n",
            getNameInDocument());
        return TopoDS_Compound();
    }

    gp_Pln plnSection = getSectionPlane();
    if (debugSection()) {
        BRepBuilderAPI_MakeFace mkFace(plnSection,
                                       -m_shapeSize,
                                       m_shapeSize,
                                       -m_shapeSize,
                                       m_shapeSize);
        BRepTools::Write(mkFace.Face(), "DVSSectionPlane.brep");// debug
        BRepTools::Write(shape, "DVSShapeToIntersect.brep)");
    }
    BRep_Builder builder;
    TopoDS_Compound result;
    builder.MakeCompound(result);

    TopExp_Explorer expFaces(shape, TopAbs_FACE);
    for (; expFaces.More(); expFaces.Next()) {
        const TopoDS_Face& face = TopoDS::Face(expFaces.Current());
        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() == GeomAbs_Plane) {
            gp_Pln plnFace = adapt.Plane();
            if (plnSection.Contains(plnFace.Location(), Precision::Confusion())
                && plnFace.Axis().IsParallel(plnSection.Axis(), Precision::Angular())) {
                builder.Add(result, face);
            }
        }
    }
    return result;
}

// move section faces to line up with cut shape
TopoDS_Compound DrawViewSection::alignSectionFaces(TopoDS_Shape faceIntersections)
{
    //    Base::Console().Message("DVS::alignSectionFaces() - %s -
    //    faceIntersection.isnull: %d\n",
    //                            getNameInDocument(),
    //                            faceIntersections.IsNull());
    TopoDS_Compound sectionFaces;
    TopoDS_Shape centeredShape =
        ShapeUtils::moveShape(faceIntersections, getOriginalCentroid() * -1.0);

    TopoDS_Shape scaledSection = ShapeUtils::scaleShape(centeredShape, getScale());
    if (!DrawUtil::fpCompare(Rotation.getValue(), 0.0)) {
        scaledSection =
            ShapeUtils::rotateShape(scaledSection, getProjectionCS(), Rotation.getValue());
    }

    return mapToPage(scaledSection);
}

TopoDS_Compound DrawViewSection::mapToPage(TopoDS_Shape& shapeToAlign)
{
    // shapeToAlign is compound of TopoDS_Face intersections, but aligned to
    // pln(origin, sectionNormal) needs to be aligned to paper plane (origin,
    // stdZ);
    // project the faces in the shapeToAlign, build new faces from the resulting
    // wires and combine everything into a compound of faces
    //    Base::Console().Message("DVS::mapToPage() - shapeToAlign.null: %d\n",
    //    shapeToAlign.IsNull());
    if (debugSection()) {
        BRepTools::Write(shapeToAlign, "DVSShapeToAlign.brep");// debug
    }

    BRep_Builder builder;
    TopoDS_Compound result;
    builder.MakeCompound(result);

    TopExp_Explorer expFace(shapeToAlign, TopAbs_FACE);
    for (int iFace = 1; expFace.More(); expFace.Next(), iFace++) {
        const TopoDS_Face& face = TopoDS::Face(expFace.Current());
        std::vector<TopoDS_Wire> faceWires;
        TopExp_Explorer expWires(face, TopAbs_WIRE);
        for (; expWires.More(); expWires.Next()) {
            const TopoDS_Wire& wire = TopoDS::Wire(expWires.Current());
            TopoDS_Shape projectedShape =
                GeometryObject::projectSimpleShape(wire, getProjectionCS());
            std::vector<TopoDS_Edge> wireEdges;
            // projectedShape is just a bunch of edges. we have to rebuild the wire.
            TopExp_Explorer expEdges(projectedShape, TopAbs_EDGE);
            for (; expEdges.More(); expEdges.Next()) {
                const TopoDS_Edge& edge = TopoDS::Edge(expEdges.Current());
                wireEdges.push_back(edge);
            }
            TopoDS_Wire cleanWire = EdgeWalker::makeCleanWire(wireEdges, 2.0 * EWTOLERANCE);
            faceWires.push_back(cleanWire);
        }

        // validate section face wires
        std::vector<TopoDS_Wire> goodWires;
        constexpr double minWireArea = 0.000001;// arbitrary very small face size
        for (auto& wire : faceWires) {
            if (wire.IsNull()) {
                continue;
            }
            if (!BRep_Tool::IsClosed(wire)) {
                continue;// can not make a face from open wire
            }
            double area = ShapeAnalysis::ContourArea(wire);
            if (area <= minWireArea) {
                continue;// can not make a face from wire with no area
            }
            goodWires.push_back(wire);
        }

        if (goodWires.empty()) {
            // this may or may not be significant.  In the offset or noparallel
            // strategies, a profile segment that is parallel to the SectionNormal
            // will not generate a face.
            Base::Console().Log("DVS::mapToPage - %s - section face has no valid wires.\n",
                                getNameInDocument());
            continue;
        }

        TopoDS_Shape holeyShape = makeFaceFromWires(goodWires);
        if (holeyShape.IsNull()) {
            continue;
        }

        builder.Add(result, TopoDS::Face(holeyShape));
        if (debugSection()) {
            std::stringstream ss;
            ss << "DVSFaceFromWires" << iFace << ".brep";
            BRepTools::Write(holeyShape, ss.str().c_str());// debug
        }
    }

    return result;
}

// makes a [perforated] face from an outer wire and wires describing the holes.
// Open wires and wires with zero area are assumed to already have been removed.
TopoDS_Shape DrawViewSection::makeFaceFromWires(std::vector<TopoDS_Wire>& inWires)
{
    // make sure the largest wire is the first
    EdgeWalker eWalker;
    std::vector<TopoDS_Wire> goodWires = eWalker.sortWiresBySize(inWires);

    // make a face from the good wires
    // first good wire should be the outer boundary of the face
    TopoDS_Face faceToFix;
    TopoDS_Shape orientedShape = goodWires.at(0).Oriented(TopAbs_FORWARD);
    TopoDS_Wire orientedWire = TopoDS::Wire(orientedShape);
    orientedWire.Orientation(TopAbs_FORWARD);
    TopoDS_Face blankFace = BRepBuilderAPI_MakeFace(orientedWire);
    int wireCount = goodWires.size();
    if (wireCount < 2) {
        faceToFix = blankFace;
    }
    else {
        // add the holes
        BRepBuilderAPI_MakeFace mkFace(blankFace);
        for (int iWire = 1; iWire < wireCount; iWire++) {
            // make holes in the face with the rest of the wires
            orientedShape = goodWires.at(iWire).Oriented(TopAbs_REVERSED);
            orientedWire = TopoDS::Wire(orientedShape);
            mkFace.Add(orientedWire);
        }

        if (!mkFace.IsDone()) {
            Base::Console().Warning("DVS::makeFaceFromWires - %s - failed to make section face.\n",
                                    getNameInDocument());
            return TopoDS_Shape();
        }
        faceToFix = mkFace.Face();
    }

    // setting the wire orientation above should generate a valid face, but
    // sometimes does not, so we fix the shape to resolve any issues
    Handle(ShapeFix_Shape) sfs = new ShapeFix_Shape;
    sfs->Init(faceToFix);
    sfs->Perform();
    return sfs->Shape();
}

// turn OCC section faces into TD geometry
std::vector<TechDraw::FacePtr> DrawViewSection::makeTDSectionFaces(TopoDS_Compound topoDSFaces)
{
    //    Base::Console().Message("DVS::makeTDSectionFaces()\n");
    std::vector<TechDraw::FacePtr> tdSectionFaces;
    TopExp_Explorer sectionExpl(topoDSFaces, TopAbs_FACE);
    for (; sectionExpl.More(); sectionExpl.Next()) {
        const TopoDS_Face& face = TopoDS::Face(sectionExpl.Current());
        TechDraw::FacePtr sectionFace(std::make_shared<TechDraw::Face>());
        TopExp_Explorer expFace(face, TopAbs_WIRE);
        for (; expFace.More(); expFace.Next()) {
            TechDraw::Wire* w = new TechDraw::Wire();
            const TopoDS_Wire& wire = TopoDS::Wire(expFace.Current());
            TopExp_Explorer expWire(wire, TopAbs_EDGE);
            for (; expWire.More(); expWire.Next()) {
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

// calculate the ends of the section line in BaseView's coords
std::pair<Base::Vector3d, Base::Vector3d> DrawViewSection::sectionLineEnds()
{
    std::pair<Base::Vector3d, Base::Vector3d> result;
    Base::Vector3d stdZ(0.0, 0.0, 1.0);
    double baseRotation = getBaseDVP()->Rotation.getValue();// Qt degrees are clockwise
    Base::Rotation rotator(stdZ, baseRotation * M_PI / 180.0);
    Base::Rotation unrotator(stdZ, -baseRotation * M_PI / 180.0);

    auto sNorm = SectionNormal.getValue();
    auto axis = getBaseDVP()->Direction.getValue();
    Base::Vector3d stdOrg(0.0, 0.0, 0.0);
    Base::Vector3d sectionLineDir = -axis.Cross(sNorm);
    sectionLineDir.Normalize();
    sectionLineDir = getBaseDVP()->projectPoint(sectionLineDir);// convert to base view CS
    sectionLineDir.Normalize();

    Base::Vector3d sectionOrg = SectionOrigin.getValue() - getBaseDVP()->getOriginalCentroid();
    sectionOrg = getBaseDVP()->projectPoint(sectionOrg);// convert to base view
                                                        // CS
    double halfSize = (getBaseDVP()->getSizeAlongVector(sectionLineDir) / 2.0) * SectionLineStretch.getValue();
    result.first = sectionOrg + sectionLineDir * halfSize;
    result.second = sectionOrg - sectionLineDir * halfSize;

    return result;
}

// find the points and directions to make the change point marks.
ChangePointVector DrawViewSection::getChangePointsFromSectionLine()
{
    //    Base::Console().Message("Dvs::getChangePointsFromSectionLine()\n");
    ChangePointVector result;
    std::vector<gp_Pnt> allPoints;
    DrawViewPart* baseDvp = dynamic_cast<DrawViewPart*>(BaseView.getValue());
    if (baseDvp) {
        std::pair<Base::Vector3d, Base::Vector3d> lineEnds = sectionLineEnds();
        // make start and end marks
        gp_Pnt location0 = DU::togp_Pnt(lineEnds.first);
        gp_Pnt location1 = DU::togp_Pnt(lineEnds.second);
        gp_Dir postDir = gp_Dir(location1.XYZ() - location0.XYZ());
        gp_Dir preDir = postDir.Reversed();
        ChangePoint startPoint(location0, preDir, postDir);
        result.push_back(startPoint);
        preDir = gp_Dir(location0.XYZ() - location1.XYZ());
        postDir = preDir.Reversed();
        ChangePoint endPoint(location1, preDir, postDir);
        result.push_back(endPoint);
    }
    return result;
}

// this should really be in BoundBox.h
//! check if point is in box or on boundary of box
//! compare to isInBox which doesn't allow on boundary
bool DrawViewSection::isReallyInBox(const Base::Vector3d v, const Base::BoundBox3d bb) const
{
    if (v.x <= bb.MinX || v.x >= bb.MaxX) {
        return false;
    }
    if (v.y <= bb.MinY || v.y >= bb.MaxY) {
        return false;
    }
    if (v.z <= bb.MinZ || v.z >= bb.MaxZ) {
        return false;
    }
    return true;
}

bool DrawViewSection::isReallyInBox(const gp_Pnt p, const Bnd_Box& bb) const
{
    return !bb.IsOut(p);
}

Base::Vector3d DrawViewSection::getXDirection() const
{
    //    Base::Console().Message("DVS::getXDirection() - %s\n",
    //    Label.getValue());
    App::Property* prop = getPropertyByName("XDirection");
    if (!prop) {
        // No XDirection property.  can this happen?
        gp_Ax2 cs = getCSFromBase(SectionDirection.getValueAsString());
        gp_Dir gXDir = cs.XDirection();
        return Base::Vector3d(gXDir.X(), gXDir.Y(), gXDir.Z());
    }

    // we have an XDirection property
    if (DrawUtil::fpCompare(XDirection.getValue().Length(), 0.0)) {
        // but it has no value, so we make a value
        if (BaseView.getValue()) {
            gp_Ax2 cs = getCSFromBase(SectionDirection.getValueAsString());
            gp_Dir gXDir = cs.XDirection();
            return Base::Vector3d(gXDir.X(), gXDir.Y(), gXDir.Z());
        }
    }

    // XDirection is good, so we use it
    return XDirection.getValue();
}

void DrawViewSection::setCSFromBase(const std::string sectionName)
{
    //    Base::Console().Message("DVS::setCSFromBase(%s)\n",
    //    sectionName.c_str());
    gp_Dir gDir = getCSFromBase(sectionName).Direction();
    Base::Vector3d vDir(gDir.X(), gDir.Y(), gDir.Z());
    Direction.setValue(vDir);
    SectionNormal.setValue(vDir);
    gp_Dir gxDir = getCSFromBase(sectionName).XDirection();
    Base::Vector3d vXDir(gxDir.X(), gxDir.Y(), gxDir.Z());
    XDirection.setValue(vXDir);
}

// set the section CS based on an XY vector in BaseViews CS
void DrawViewSection::setCSFromBase(const Base::Vector3d localUnit)
{
    //    Base::Console().Message("DVS::setCSFromBase(%s)\n",
    //    DrawUtil::formatVector(localUnit).c_str());
    gp_Ax2 newSectionCS = getBaseDVP()->localVectorToCS(localUnit);

    Base::Vector3d vDir(newSectionCS.Direction().X(),
                        newSectionCS.Direction().Y(),
                        newSectionCS.Direction().Z());
    Direction.setValue(vDir);
    SectionNormal.setValue(vDir);
    Base::Vector3d vXDir(newSectionCS.XDirection().X(),
                         newSectionCS.XDirection().Y(),
                         newSectionCS.XDirection().Z());
    XDirection.setValue(vXDir);// XDir is for projection
}

// reset the section CS based on an XY vector in current section CS
void DrawViewSection::setCSFromLocalUnit(const Base::Vector3d localUnit)
{
    //    Base::Console().Message("DVS::setCSFromLocalUnit(%s)\n",
    //    DrawUtil::formatVector(localUnit).c_str());
    gp_Dir verticalDir = getSectionCS().YDirection();
    gp_Ax1 verticalAxis(DrawUtil::togp_Pnt(SectionOrigin.getValue()), verticalDir);
    gp_Dir oldNormal = getSectionCS().Direction();
    gp_Dir newNormal = DrawUtil::togp_Dir(projectPoint(localUnit));
    double angle = oldNormal.AngleWithRef(newNormal, verticalDir);
    gp_Ax2 newCS = getSectionCS().Rotated(verticalAxis, angle);
    SectionNormal.setValue(DrawUtil::toVector3d(newCS.Direction()));
    XDirection.setValue(DrawUtil::toVector3d(newCS.XDirection()));
}

gp_Ax2 DrawViewSection::getCSFromBase(const std::string sectionName) const
{
    //    Base::Console().Message("DVS::getCSFromBase(%s)\n",
    //    sectionName.c_str());
    Base::Vector3d origin(0.0, 0.0, 0.0);
    Base::Vector3d sectOrigin = SectionOrigin.getValue();

    gp_Ax2 dvpCS = getBaseDVP()->getProjectionCS(sectOrigin);

    if (debugSection()) {
        DrawUtil::dumpCS("DVS::getCSFromBase - dvp CS", dvpCS);
    }
    gp_Dir dvpDir = dvpCS.Direction();
    gp_Dir dvpUp = dvpCS.YDirection();
    gp_Dir dvpRight = dvpCS.XDirection();
    gp_Pnt dvsLoc(sectOrigin.x, sectOrigin.y, sectOrigin.z);
    gp_Dir dvsDir;
    gp_Dir dvsXDir;

    if (sectionName == "Up") {// looking up
        dvsDir = dvpUp.Reversed();
        dvsXDir = dvpRight;
    }
    else if (sectionName == "Down") {
        dvsDir = dvpUp;
        dvsXDir = dvpRight;
    }
    else if (sectionName == "Left") {
        dvsDir = dvpRight;          // dvpX
        dvsXDir = dvpDir.Reversed();//-dvpZ
    }
    else if (sectionName == "Right") {
        dvsDir = dvpRight.Reversed();
        dvsXDir = dvpDir;
    }
    else if (sectionName == "Aligned") {
        // if aligned, we don't get our direction from the base view
        Base::Vector3d sectionNormal = SectionNormal.getValue();
        dvsDir = gp_Dir(sectionNormal.x, sectionNormal.y, sectionNormal.z);
        Base::Vector3d sectionXDir = XDirection.getValue();
        dvsXDir = gp_Dir(sectionXDir.x, sectionXDir.y, sectionXDir.z);
    }
    else {
        dvsDir = dvpRight;
        dvsXDir = dvpDir;
    }

    gp_Ax2 CS(dvsLoc, dvsDir, dvsXDir);

    if (debugSection()) {
        DrawUtil::dumpCS("DVS::getCSFromBase - sectionCS out", CS);
    }

    return CS;
}

// returns current section cs
gp_Ax2 DrawViewSection::getSectionCS() const
{
    //    Base::Console().Message("DVS::getSectionCS()\n");
    Base::Vector3d vNormal = SectionNormal.getValue();
    gp_Dir gNormal(vNormal.x, vNormal.y, vNormal.z);
    Base::Vector3d vXDir = getXDirection();
    gp_Dir gXDir(vXDir.x, vXDir.y, vXDir.z);
    Base::Vector3d vOrigin = SectionOrigin.getValue();
    gp_Pnt gOrigin(vOrigin.x, vOrigin.y, vOrigin.z);
    gp_Ax2 sectionCS(gOrigin, gNormal);
    try {
        sectionCS = gp_Ax2(gOrigin, gNormal, gXDir);
    }
    catch (...) {
        Base::Console().Error("DVS::getSectionCS - %s - failed to create section CS\n",
                              getNameInDocument());
    }
    return sectionCS;
}

gp_Ax2 DrawViewSection::getProjectionCS(const Base::Vector3d pt) const
{
    Base::Vector3d vNormal = SectionNormal.getValue();
    gp_Dir gNormal(vNormal.x, vNormal.y, vNormal.z);
    Base::Vector3d vXDir = getXDirection();
    gp_Dir gXDir(vXDir.x, vXDir.y, vXDir.z);
    if (DrawUtil::fpCompare(fabs(gNormal.Dot(gXDir)), 1.0)) {
        // can not build a gp_Ax2 from these values
        throw Base::RuntimeError(
            "DVS::getProjectionCS - SectionNormal and XDirection are parallel");
    }
    gp_Pnt gOrigin(pt.x, pt.y, pt.z);
    return {gOrigin, gNormal, gXDir};
}

std::vector<LineSet> DrawViewSection::getDrawableLines(int i)
{
    //    Base::Console().Message("DVS::getDrawableLines(%d) - lineSets: %d\n", i,
    //    m_lineSets.size());
    std::vector<LineSet> result;
    return DrawGeomHatch::getTrimmedLinesSection(this,
                                                 m_lineSets,
                                                 getSectionTopoDSFace(i),
                                                 HatchScale.getValue(),
                                                 HatchRotation.getValue(),
                                                 HatchOffset.getValue());
}

TopoDS_Face DrawViewSection::getSectionTopoDSFace(int i)
{
    TopExp_Explorer expl(m_sectionTopoDSFaces, TopAbs_FACE);
    int count = 1;
    for (; expl.More(); expl.Next(), count++) {
        if (count == i + 1) {
            return TopoDS::Face(expl.Current());
        }
    }
    return TopoDS_Face();
}

TechDraw::DrawViewPart* DrawViewSection::getBaseDVP() const
{
    App::DocumentObject* base = BaseView.getValue();
    if (base && base->getTypeId().isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        TechDraw::DrawViewPart* baseDVP = static_cast<TechDraw::DrawViewPart*>(base);
        return baseDVP;
    }
    return nullptr;
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
    makeLineSets();
    DrawViewPart::onDocumentRestored();
}

void DrawViewSection::setupObject()
{
    // by this point DVS should have a name and belong to a document
    replaceSvgIncluded(FileHatchPattern.getValue());
    replacePatIncluded(FileGeomPattern.getValue());

    DrawViewPart::setupObject();
}

// hatch file routines

// create geometric hatch lines
void DrawViewSection::makeLineSets(void)
{
    //    Base::Console().Message("DVS::makeLineSets()\n");
    if (PatIncluded.isEmpty()) {
        return;
    }

    std::string fileSpec = PatIncluded.getValue();
    Base::FileInfo fi(fileSpec);
    if (!fi.isReadable()) {
        Base::Console().Message("%s can not read hatch file: %s\n",
                                getNameInDocument(),
                                fileSpec.c_str());
        return;
    }

    if (fi.hasExtension("pat")) {
        if (!fileSpec.empty() && !NameGeomPattern.isEmpty()) {
            m_lineSets.clear();
            m_lineSets = DrawGeomHatch::makeLineSets(fileSpec, NameGeomPattern.getValue());
        }
    }
}

void DrawViewSection::replaceSvgIncluded(std::string newSvgFile)
{
    //    Base::Console().Message("DVS::replaceSvgIncluded(%s)\n",
    //    newSvgFile.c_str());
    if (newSvgFile.empty()) {
        return;
    }

    Base::FileInfo tfi(newSvgFile);
    if (tfi.isReadable()) {
        SvgIncluded.setValue(newSvgFile.c_str());
    }
    else {
        throw Base::RuntimeError("Could not read the new Svg file");
    }
}

void DrawViewSection::replacePatIncluded(std::string newPatFile)
{
    //    Base::Console().Message("DVS::replacePatIncluded(%s)\n",
    //    newPatFile.c_str());
    if (newPatFile.empty()) {
        return;
    }

    Base::FileInfo tfi(newPatFile);
    if (tfi.isReadable()) {
        PatIncluded.setValue(newPatFile.c_str());
    }
    else {
        throw Base::RuntimeError("Could not read the new Pat file");
    }
}

// Parameter fetching routines

void DrawViewSection::getParameters()
{
    //    Base::Console().Message("DVS::getParameters()\n");
    bool fuseFirst = Preferences::getPreferenceGroup("General")->GetBool("SectionFuseFirst", false);
    FuseBeforeCut.setValue(fuseFirst);
}

bool DrawViewSection::debugSection(void) const
{
    return Preferences::getPreferenceGroup("debug")->GetBool("debugSection", false);
}

int DrawViewSection::prefCutSurface(void) const
{
    //    Base::Console().Message("DVS::prefCutSurface()\n");

    return Preferences::getPreferenceGroup("Decorations")
        ->GetInt("CutSurfaceDisplay", 2);// default to SvgHatch
}

bool DrawViewSection::showSectionEdges(void)
{
    return Preferences::getPreferenceGroup("General")->GetBool("ShowSectionEdges", true);
}

bool DrawViewSection::trimAfterCut() const
{
    return TrimAfterCut.getValue();
}
// Python Drawing feature
// ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewSectionPython, TechDraw::DrawViewSection)
template<>
const char* TechDraw::DrawViewSectionPython::getViewProviderName() const
{
    return "TechDrawGui::ViewProviderDrawingView";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewSection>;
}// namespace App
