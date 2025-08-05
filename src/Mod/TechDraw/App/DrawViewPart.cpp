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

//===========================================================================
// DrawViewPart overview
//===========================================================================
//
// 1) get the shapes from the source objects
// 2) center, scale and rotate the shapes
// 3) project the shape using the OCC HLR algorithms
// 4) add cosmetic and other objects that don't participate in hlr
// 5) find the closed regions (faces) in the edges returned by hlr
// everything else is mostly providing services to other objects, such as the
// actual drawing routines in Gui

#include "PreCompiled.h"

#ifndef _PreComp_
#include <BRepAlgo_NormalProjection.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <HLRAlgo_Projector.hxx>
#include <QtConcurrentRun>
#include <ShapeAnalysis.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <sstream>
#endif

#include <App/Document.h>
#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Converter.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>

#include "Cosmetic.h"
#include "CenterLine.h"
#include "DrawGeomHatch.h"
#include "DrawHatch.h"
#include "DrawPage.h"
#include "DrawProjectSplit.h"
#include "DrawUtil.h"
#include "DrawViewBalloon.h"
#include "DrawViewDetail.h"
#include "DrawViewDimension.h"
#include "DrawViewPart.h"
#include "DrawViewPartPy.h"// generated from DrawViewPartPy.xml
#include "DrawViewSection.h"
#include "EdgeWalker.h"
#include "Geometry.h"
#include "GeometryObject.h"
#include "ShapeExtractor.h"
#include "Preferences.h"
#include "ShapeUtils.h"

using namespace TechDraw;
using DU = DrawUtil;

PROPERTY_SOURCE_WITH_EXTENSIONS(TechDraw::DrawViewPart, TechDraw::DrawView)

DrawViewPart::DrawViewPart()
    : geometryObject(nullptr),
      m_tempGeometryObject(nullptr),
      m_handleFaces(false),
      nowUnsetting(false),
      m_waitingForFaces(false),
      m_waitingForHlr(false)
{
    static const char* group = "Projection";
    static const char* sgroup = "HLR Parameters";

    CosmeticExtension::initExtension(this);

    //properties that affect Geometry
    ADD_PROPERTY_TYPE(Source, (nullptr), group, App::Prop_None, "3D Shape to view");
    Source.setScope(App::LinkScope::Global);
    Source.setAllowExternal(true);
    ADD_PROPERTY_TYPE(XSource, (nullptr), group, App::Prop_None, "External 3D Shape to view");

    ADD_PROPERTY_TYPE(Direction, (0.0, -1.0, 0.0), group, App::Prop_None,
                      "Projection Plane normal. View direction for the projection plane");
    ADD_PROPERTY_TYPE(XDirection, (1.0, 0.0, 0.0), group, App::Prop_None,
                      "Projection Plane X Axis in R3. Rotates/Mirrors View");
    ADD_PROPERTY_TYPE(Perspective, (false), group, App::Prop_None,
                      "Perspective(true) or Orthographic(false) projection");
    ADD_PROPERTY_TYPE(Focus, (Preferences::getPreferenceGroup("General")->GetFloat("FocusDistance", 100.0)),
                    group, App::Prop_None, "Perspective view focus distance");

    //properties that control HLR algo
    ADD_PROPERTY_TYPE(CoarseView, (Preferences::getPreferenceGroup("General")->GetBool("CoarseView", false)),
        sgroup, App::Prop_None, "Coarse View on/off");
    ADD_PROPERTY_TYPE(SmoothVisible, (Preferences::getPreferenceGroup("HLR")->GetBool("SmoothViz", true)),
        sgroup, App::Prop_None, "Show Visible Smooth lines");
    ADD_PROPERTY_TYPE(SeamVisible, (Preferences::getPreferenceGroup("HLR")->GetBool("SeamViz", false)),
        sgroup, App::Prop_None,
                      "Show Visible Seam lines");
    ADD_PROPERTY_TYPE(IsoVisible, (Preferences::getPreferenceGroup("HLR")->GetBool("IsoViz", false)),
        sgroup, App::Prop_None, "Show Visible Iso u, v lines");
    ADD_PROPERTY_TYPE(HardHidden, (Preferences::getPreferenceGroup("HLR")->GetBool("HardHid", false)),
        sgroup, App::Prop_None, "Show Hidden Hard lines");
    ADD_PROPERTY_TYPE(SmoothHidden, (Preferences::getPreferenceGroup("HLR")->GetBool("SmoothHid", false)),
        sgroup, App::Prop_None, "Show Hidden Smooth lines");
    ADD_PROPERTY_TYPE(SeamHidden, (Preferences::getPreferenceGroup("HLR")->GetBool("SeamHid", false)),
        sgroup, App::Prop_None, "Show Hidden Seam lines");
    ADD_PROPERTY_TYPE(IsoHidden, (Preferences::getPreferenceGroup("HLR")->GetBool("IsoHid", false)),
        sgroup, App::Prop_None, "Show Hidden Iso u, v lines");
    ADD_PROPERTY_TYPE(IsoCount, (Preferences::getPreferenceGroup("HLR")->GetBool("IsoCount", 0)),
        sgroup, App::Prop_None, "Number of iso parameters lines");

    ADD_PROPERTY_TYPE(ScrubCount, (Preferences::scrubCount()), sgroup, App::Prop_None,
                      "The number of times FreeCAD should try to clean the HLR result.");

    //initialize bbox to non-garbage
    bbox = Base::BoundBox3d(Base::Vector3d(0.0, 0.0, 0.0), 0.0);
}

DrawViewPart::~DrawViewPart()
{
    //don't delete this object while it still has dependent threads running
    if (m_hlrFuture.isRunning()) {
        Base::Console().message("%s is waiting for HLR to finish\n", Label.getValue());
        m_hlrFuture.waitForFinished();
    }
    if (m_faceFuture.isRunning()) {
        Base::Console().message("%s is waiting for face finding to finish\n", Label.getValue());
        m_faceFuture.waitForFinished();
    }
    removeAllReferencesFromGeom();
}

//! returns a compound of all the shapes from the DocumentObjects in the Source &
//!  XSource property lists
TopoDS_Shape DrawViewPart::getSourceShape(bool fuse, bool allow2d) const
{
//    Base::Console().message("DVP::getSourceShape()\n");
    const std::vector<App::DocumentObject*>& links = getAllSources();
    if (links.empty()) {
        return {};
    }
    if (fuse) {
        return ShapeExtractor::getShapesFused(links);
    }
    return ShapeExtractor::getShapes(links, allow2d);
}

//! deliver a shape appropriate for making a detail view based on this view
//! TODO: why does dvp do the thinking for detail, but section picks its own
//! version of the shape?  Should we have a getShapeForSection?
TopoDS_Shape DrawViewPart::getShapeForDetail() const
{
    return ShapeUtils::rotateShape(getSourceShape(false), getProjectionCS(), Rotation.getValue());
}

//! combine the regular links and xlinks into a single list
std::vector<App::DocumentObject*> DrawViewPart::getAllSources() const
{
    //    Base::Console().message("DVP::getAllSources()\n");
    std::vector<App::DocumentObject*> links = Source.getValues();
    std::vector<App::DocumentObject*> xLinks = XSource.getValues();

    std::vector<App::DocumentObject*> result = links;
    if (!xLinks.empty()) {
        result.insert(result.end(), xLinks.begin(), xLinks.end());
    }
    return result;
}

//! pick vertex objects out of the Source properties and
//! add them directly to the geometry without going through HLR
void DrawViewPart::addPoints()
{
//    Base::Console().message("DVP::addPoints()\n");
    // get all the 2d shapes in the sources, then pick through them for vertices.
    std::vector<TopoDS_Shape> shapesAll = ShapeExtractor::getShapes2d(getAllSources());
    for (auto& shape : shapesAll) {
        if (shape.ShapeType() == TopAbs_VERTEX) {
            gp_Pnt gp = BRep_Tool::Pnt(TopoDS::Vertex(shape));
            Base::Vector3d vp(gp.X(), gp.Y(), gp.Z());
            vp = vp - m_saveCentroid;
            //need to offset the point to match the big projection
            Base::Vector3d projected = projectPoint(vp * getScale());
            TechDraw::VertexPtr v1(std::make_shared<TechDraw::Vertex>(projected));
            geometryObject->addVertex(v1);
        }
    }
}

App::DocumentObjectExecReturn* DrawViewPart::execute()
{
    // Base::Console().message("DVP::execute() - %s\n", getNameInDocument());
    if (!keepUpdated()) {
        return DrawView::execute();
    }

    if (waitingForHlr()) {
        return DrawView::execute();
    }

    TopoDS_Shape shape = getSourceShape();
    if (shape.IsNull()) {
        Base::Console().message("DVP::execute - %s - Source shape is Null.\n", getNameInDocument());
        return DrawView::execute();
    }

    //make sure the XDirection property is valid. Mostly for older models.
    if (!checkXDirection()) {
        Base::Vector3d newX = getXDirection();
        XDirection.setValue(newX);
        XDirection.purgeTouched();//don't trigger updates!
    }

    partExec(shape);

    return DrawView::execute();
}

short DrawViewPart::mustExecute() const
{
    if (isRestoring()) {
        return TechDraw::DrawView::mustExecute();
    }

    if (Direction.isTouched() || Source.isTouched() || XSource.isTouched()
        || Perspective.isTouched() || Focus.isTouched() || Rotation.isTouched()
        || SmoothVisible.isTouched() || SeamVisible.isTouched() || IsoVisible.isTouched()
        || HardHidden.isTouched() || SmoothHidden.isTouched() || SeamHidden.isTouched()
        || IsoHidden.isTouched() || IsoCount.isTouched() || CoarseView.isTouched()
        || CosmeticVertexes.isTouched() || CosmeticEdges.isTouched() || CenterLines.isTouched()) {
        return 1;
    }

    return TechDraw::DrawView::mustExecute();
}

void DrawViewPart::onChanged(const App::Property* prop)
{
    // If the user has set PropertyVector Direction to zero, set it along the default value instead (Front View).
    // Otherwise bad things will happen because there'll be a normalization for direction calculations later.
    Base::Vector3d dir = Direction.getValue();
    if (DrawUtil::fpCompare(dir.Length(), 0.0)) {
        Base::Console().warning("%s Direction is null. Using (0, -1, 0).\n", Label.getValue());
        Direction.setValue(Base::Vector3d(0.0, -1.0, 0.0));
    }
    Base::Vector3d xdir = XDirection.getValue();
    if (DrawUtil::fpCompare(xdir.Length(), 0.0)) {
        Base::Console().warning("%s XDirection is null. Using (1, 0, 0).\n", Label.getValue());
        XDirection.setValue(Base::Vector3d(1.0, 0.0, 0.0));
    }

    DrawView::onChanged(prop);
}

void DrawViewPart::partExec(TopoDS_Shape& shape)
{
    if (waitingForHlr()) {
        //finish what we are already doing before starting a new cycle
        return;
    }

    //we need to keep using the old geometryObject until the new one is fully populated
    m_tempGeometryObject = makeGeometryForShape(shape);
    if (CoarseView.getValue() ||
        !DU::isGuiUp()) {
        onHlrFinished();//poly algo and console mode do not run in separate thread, so we need to invoke
                        //the post hlr processing manually
    }
}

//! prepare the shape for HLR processing by centering, scaling and rotating it
GeometryObjectPtr DrawViewPart::makeGeometryForShape(TopoDS_Shape& shape)
{
    // if we use the passed reference directly, the centering doesn't work.  Maybe the underlying OCC TShape
    // isn't modified?  using a copy works and the referenced shape (from getSourceShape in execute())
    // isn't used for anything anyway.
    bool copyGeometry = true;
    bool copyMesh = false;
    BRepBuilderAPI_Copy copier(shape, copyGeometry, copyMesh);
    TopoDS_Shape localShape = copier.Shape();

    gp_Pnt gCentroid = ShapeUtils::findCentroid(localShape, getProjectionCS());
    m_saveCentroid = Base::convertTo<Base::Vector3d>(gCentroid);
    m_saveShape = centerScaleRotate(this, localShape, m_saveCentroid);

    return buildGeometryObject(localShape, getProjectionCS());
}

//! Modify a shape by centering, scaling and rotating and return the centered (but not rotated) shape
TopoDS_Shape DrawViewPart::centerScaleRotate(const DrawViewPart *dvp, TopoDS_Shape& inOutShape,
                                             Base::Vector3d centroid)
{
    gp_Ax2 viewAxis = dvp->getProjectionCS();

    //center shape on origin
    TopoDS_Shape centeredShape = ShapeUtils::moveShape(inOutShape, centroid * -1.0);

    inOutShape = ShapeUtils::scaleShape(centeredShape, dvp->getScale());
    if (!DrawUtil::fpCompare(dvp->Rotation.getValue(), 0.0)) {
        inOutShape = ShapeUtils::rotateShape(inOutShape, viewAxis,
                                           dvp->Rotation.getValue());//conventional rotation
    }
    //    BRepTools::Write(inOutShape, "DVPScaled.brep");            //debug
    return centeredShape;
}

//! create a geometry object and trigger the HLR process in another thread
TechDraw::GeometryObjectPtr DrawViewPart::buildGeometryObject(TopoDS_Shape& shape,
                                                              const gp_Ax2& viewAxis)
{
    TechDraw::GeometryObjectPtr go(
        std::make_shared<TechDraw::GeometryObject>(getNameInDocument(), this));
    go->setIsoCount(IsoCount.getValue());
    go->isPerspective(Perspective.getValue());
    go->setFocus(Focus.getValue());
    go->usePolygonHLR(CoarseView.getValue());
    go->setScrubCount(ScrubCount.getValue());

    if (CoarseView.getValue()) {
        //the polygon approximation HLR process runs quickly, so doesn't need to be in a
        //separate thread
        go->projectShapeWithPolygonAlgo(shape, viewAxis);
        return go;
    }

    if (!DU::isGuiUp()) {
        // if the Gui is not running (actual the event loop), we cannot use the separate thread,
        // since we will never be notified of thread completion.
        go->projectShape(shape, viewAxis);
        return go;
    }

    //projectShape (the HLR process) runs in a separate thread since it can take a long time
    //note that &m_hlrWatcher in the third parameter is not strictly required, but using the
    //4 parameter signature instead of the 3 parameter signature prevents clazy warning:
    //https://github.com/KDE/clazy/blob/1.11/docs/checks/README-connect-3arg-lambda.md
    connectHlrWatcher = QObject::connect(&m_hlrWatcher, &QFutureWatcherBase::finished,
                                         &m_hlrWatcher, [this] { this->onHlrFinished(); });

    // We create a lambda closure to hold a copy of go, shape and viewAxis.
    // This is important because those variables might be local to the calling
    // function and might get destructed before the parallel processing finishes.
    auto lambda = [go, shape, viewAxis]{go->projectShape(shape, viewAxis);};
    m_hlrFuture = QtConcurrent::run(std::move(lambda));
    m_hlrWatcher.setFuture(m_hlrFuture);
    waitingForHlr(true);

    return go;
}

//! continue processing after hlr thread completes
void DrawViewPart::onHlrFinished()
{
    //now that the new GeometryObject is fully populated, we can replace the old one
    if (m_tempGeometryObject) {
        geometryObject = m_tempGeometryObject;//replace with new
        m_tempGeometryObject = nullptr;       //superfluous?
    }
    if (!geometryObject) {
        throw Base::RuntimeError("DrawViewPart has lost its geometry object");
    }

    if (!hasGeometry()) {
        Base::Console().error("TechDraw did not retrieve any geometry for %s/%s\n",
                              getNameInDocument(), Label.getValue());
    }

    //the last hlr related task is to make a bbox of the results
    bbox = geometryObject->calcBoundingBox();

    waitingForHlr(false);
    QObject::disconnect(connectHlrWatcher);
    showProgressMessage(getNameInDocument(), "has finished finding hidden lines");

    postHlrTasks();//application level tasks that depend on HLR/GO being complete

    //start face finding in a separate thread.  We don't find faces when using the polygon
    //HLR method.

    if (handleFaces() && !DU::isGuiUp()) {
        extractFaces();
        onFacesFinished();
        return;
    }

    if (handleFaces() && !CoarseView.getValue()) {
        try {
            //note that &m_faceWatcher in the third parameter is not strictly required, but using the
            //4 parameter signature instead of the 3 parameter signature prevents clazy warning:
            //https://github.com/KDE/clazy/blob/1.11/docs/checks/README-connect-3arg-lambda.md
            connectFaceWatcher =
                QObject::connect(&m_faceWatcher, &QFutureWatcherBase::finished, &m_faceWatcher,
                                 [this] { this->onFacesFinished(); });

            auto lambda = [this]{this->extractFaces();};
            m_faceFuture = QtConcurrent::run(std::move(lambda));
            m_faceWatcher.setFuture(m_faceFuture);
            waitingForFaces(true);
        }
        catch (Standard_Failure& e) {
            waitingForFaces(false);
            Base::Console().error("DVP::partExec - %s - extractFaces failed - %s **\n",
                                  getNameInDocument(), e.GetMessageString());
            throw Base::RuntimeError("DVP::onHlrFinished - error extracting faces");
        }
    }
}

//! run any tasks that need to been done after geometry is available
void DrawViewPart::postHlrTasks()
{
    //add geometry that doesn't come from HLR
    addCosmeticVertexesToGeom();
    addCosmeticEdgesToGeom();
    addReferencesToGeom();
    addPoints();

    //balloons need to be recomputed here because their
    //references will be invalid until the geometry exists
    std::vector<TechDraw::DrawViewBalloon*> balloonsAll = getBalloons();
    for (auto& balloon : balloonsAll) {
        balloon->recomputeFeature();
    }
    // Dimensions need to be recomputed now if face finding is not going to take place.
    if (!handleFaces() || CoarseView.getValue()) {
        std::vector<TechDraw::DrawViewDimension*> dimsAll = getDimensions();
        for (auto& dim : dimsAll) {
            dim->recomputeFeature();
        }
    }

    // rescale if view doesn't fit on page
    if (ScaleType.isValue("Automatic") && !checkFit()) {
        double newScale = autoScale();
        Scale.setValue(newScale);
        Scale.purgeTouched();
        partExec(m_saveShape);
    }

    overrideKeepUpdated(false);

    requestPaint();
}

// Run any tasks that need to be done after faces are available
void DrawViewPart::postFaceExtractionTasks()
{
    // Some centerlines depend on faces so we could not add CL geometry before now
    addCenterLinesToGeom();

    // Dimensions need to be recomputed because their references will be invalid
    //  until all the geometry (including centerlines dependent on faces) exists.
    std::vector<TechDraw::DrawViewDimension*> dimsAll = getDimensions();
    for (auto& dim : dimsAll) {
        dim->recomputeFeature();
    }

    requestPaint();
}


//! make faces from the edge geometry
void DrawViewPart::extractFaces()
{
    if (!geometryObject) {
        //geometry is in flux, can not make faces right now
        return;
    }

    showProgressMessage(getNameInDocument(), "is extracting faces");

    const std::vector<TechDraw::BaseGeomPtr>& goEdges =
        geometryObject->getVisibleFaceEdges(SmoothVisible.getValue(), SeamVisible.getValue());

    if (goEdges.empty()) {
        //        Base::Console().message("DVP::extractFaces - %s - no face edges available!\n", getNameInDocument());    //debug
        return;
    }

    if (newFaceFinder()) {
        findFacesNew(goEdges);
    } else {
        findFacesOld(goEdges);
    }
}

// use the revised face finder algo
void DrawViewPart::findFacesNew(const std::vector<BaseGeomPtr> &goEdges)
{
    std::vector<TopoDS_Edge> closedEdges;
    std::vector<TopoDS_Edge> cleanEdges = DrawProjectSplit::scrubEdges(goEdges, closedEdges);

    if (cleanEdges.empty() && closedEdges.empty()) {
        //how does this happen?  something wrong somewhere
        //            Base::Console().message("DVP::findFacesNew - no clean or closed wires\n");    //debug
        return;
    }

    //use EdgeWalker to make wires from edges
    EdgeWalker eWalker;
    std::vector<TopoDS_Wire> sortedWires;
    try {
        if (!cleanEdges.empty()) {
            sortedWires = eWalker.execute(cleanEdges, true);//include outer wire
        }
    }
    catch (Base::Exception& e) {
        throw Base::RuntimeError(e.what());
    }
    geometryObject->clearFaceGeom();

    std::vector<TopoDS_Wire> closedWires;
    for (auto& edge : closedEdges) {
        BRepBuilderAPI_MakeWire mkWire(edge);
        TopoDS_Wire wire = mkWire.Wire();
        closedWires.push_back(wire);
    }
    if (!closedWires.empty()) {
        sortedWires.insert(sortedWires.end(), closedWires.begin(), closedWires.end());
        //inserting the closedWires that did not go through EdgeWalker into
        //sortedWires ruins EdgeWalker's sort by size, so we have to do it again.
        sortedWires = eWalker.sortWiresBySize(sortedWires);
    }

    if (sortedWires.empty()) {
        Base::Console().warning(
            "DVP::findFacesNew - %s - Cannot make faces from projected edges\n",
            getNameInDocument());
    }
    else {
        constexpr double minWireArea = 0.000001;//arbitrary very small face size
        auto itWire = sortedWires.begin();
        for (; itWire != sortedWires.end(); itWire++) {
            if (!BRep_Tool::IsClosed(*itWire)) {
                continue;//can not make a face from open wire
            }

            double area = ShapeAnalysis::ContourArea(*itWire);
            if (area <= minWireArea) {
                continue;//can not make a face from wire with no area
            }

            TechDraw::FacePtr face(std::make_shared<TechDraw::Face>());
            const TopoDS_Wire& wire = (*itWire);
            face->wires.push_back(new TechDraw::Wire(wire));
            if (geometryObject) {
                geometryObject->addFaceGeom(face);
            }
        }
    }
}

// original face finding method.  This is retained only to produce the same face geometry in older
// documents.
void DrawViewPart::findFacesOld(const std::vector<BaseGeomPtr> &goEdges)
{
    //make a copy of the input edges so the loose tolerances of face finding are
    //not applied to the real edge geometry.  See TopoDS_Shape::TShape().
    std::vector<TopoDS_Edge> copyEdges;
    bool copyGeometry = true;
    bool copyMesh = false;
    for (const auto& e : goEdges) {
        BRepBuilderAPI_Copy copier(e->getOCCEdge(), copyGeometry, copyMesh);
        copyEdges.push_back(TopoDS::Edge(copier.Shape()));
    }
    std::vector<TopoDS_Edge> nonZero;
    for (auto& e : copyEdges) {//drop any zero edges (shouldn't be any by now!!!)
        if (!DrawUtil::isZeroEdge(e)) {
            nonZero.push_back(e);
        }
    }

    //HLR algo does not provide all edge intersections for edge endpoints.
    //need to split long edges touched by Vertex of another edge
    std::vector<splitPoint> splits;
    std::vector<TopoDS_Edge>::iterator itOuter = nonZero.begin();
    int iOuter = 0;
    for (; itOuter != nonZero.end(); ++itOuter, iOuter++) {//*** itOuter != nonZero.end() - 1
        TopoDS_Vertex v1 = TopExp::FirstVertex((*itOuter));
        TopoDS_Vertex v2 = TopExp::LastVertex((*itOuter));
        Bnd_Box sOuter;
        BRepBndLib::AddOptimal(*itOuter, sOuter);
        sOuter.SetGap(0.1);
        if (sOuter.IsVoid()) {
            continue;
        }
        if (DrawUtil::isZeroEdge(*itOuter)) {
            continue;                   //skip zero length edges. shouldn't happen ;)
        }
        int iInner = 0;
        std::vector<TopoDS_Edge>::iterator itInner = nonZero.begin();//***sb itOuter + 1;
        for (; itInner != nonZero.end(); ++itInner, iInner++) {
            if (iInner == iOuter) {
                continue;
            }
            if (DrawUtil::isZeroEdge((*itInner))) {
                continue;//skip zero length edges. shouldn't happen ;)
            }

            Bnd_Box sInner;
            BRepBndLib::AddOptimal(*itInner, sInner);
            sInner.SetGap(0.1);
            if (sInner.IsVoid()) {
                continue;
            }
            if (sOuter.IsOut(sInner)) {//bboxes of edges don't intersect, don't bother
                continue;
            }

            double param = -1;
            if (DrawProjectSplit::isOnEdge((*itInner), v1, param, false)) {
                gp_Pnt pnt1 = BRep_Tool::Pnt(v1);
                splitPoint s1;
                s1.i = iInner;
                s1.v = Base::Vector3d(pnt1.X(), pnt1.Y(), pnt1.Z());
                s1.param = param;
                splits.push_back(s1);
            }
            if (DrawProjectSplit::isOnEdge((*itInner), v2, param, false)) {
                gp_Pnt pnt2 = BRep_Tool::Pnt(v2);
                splitPoint s2;
                s2.i = iInner;
                s2.v = Base::Vector3d(pnt2.X(), pnt2.Y(), pnt2.Z());
                s2.param = param;
                splits.push_back(s2);
            }
        }//inner loop
    }    //outer loop

    std::vector<splitPoint> sorted = DrawProjectSplit::sortSplits(splits, true);
    auto last = std::unique(sorted.begin(), sorted.end(),
                            DrawProjectSplit::splitEqual);//duplicates to back
    sorted.erase(last, sorted.end());                     //remove dupl splits
    std::vector<TopoDS_Edge> newEdges = DrawProjectSplit::splitEdges(nonZero, sorted);

    if (newEdges.empty()) {
        return;
    }

    newEdges = DrawProjectSplit::removeDuplicateEdges(newEdges);

    geometryObject->clearFaceGeom();

    //find all the wires in the pile of faceEdges
    std::vector<TopoDS_Wire> sortedWires;
    EdgeWalker eWalker;
    sortedWires = eWalker.execute(newEdges);
    if (sortedWires.empty()) {
        Base::Console().warning(
            "DVP::findFacesOld - %s -Cannott make faces from projected edges\n",
            getNameInDocument());
        return;
    }
    else {
        std::vector<TopoDS_Wire>::iterator itWire = sortedWires.begin();
        for (; itWire != sortedWires.end(); itWire++) {
            //version 1: 1 wire/face - no voids in face
            TechDraw::FacePtr f(std::make_shared<TechDraw::Face>());
            const TopoDS_Wire& wire = (*itWire);
            TechDraw::Wire* w = new TechDraw::Wire(wire);
            f->wires.push_back(w);
            if (geometryObject) {
                geometryObject->addFaceGeom(f);
            }
        }
    }
}

//continue processing after extractFaces thread completes
void DrawViewPart::onFacesFinished()
{
    //    Base::Console().message("DVP::onFacesFinished() - %s\n", getNameInDocument());
    waitingForFaces(false);
    QObject::disconnect(connectFaceWatcher);
    showProgressMessage(getNameInDocument(), "has finished extracting faces");

    // Now we can recompute Dimensions and do other tasks possibly depending on Face extraction
    postFaceExtractionTasks();

    requestPaint();
}


//! returns the position of the first visible vertex within snap radius of newAnchorPoint.  newAnchorPoint
//! should be unscaled in conventional coordinates.  if no suitable vertex is found, newAnchorPoint
//! is returned. the result is unscaled and inverted?
Base::Vector3d DrawViewPart::snapHighlightToVertex(Base::Vector3d newAnchorPoint,
                                                   double radius) const
{
    if (!Preferences::snapDetailHighlights()) {
        return newAnchorPoint;
    }

    double snapRadius = radius * Preferences::detailSnapRadius();
    double dvpScale = getScale();
    std::vector<Base::Vector3d> vertexPoints;
    auto vertsAll = getVertexGeometry();
    double nearDistance{std::numeric_limits<double>::max()};
    Base::Vector3d nearPoint{newAnchorPoint};
    for (auto& vert: vertsAll) {
        if (vert->getHlrVisible()) {
            Base::Vector3d vertPointUnscaled = DU::invertY(vert->point()) / dvpScale;
            double distanceToVertex = (vertPointUnscaled - newAnchorPoint).Length();
            if (distanceToVertex < snapRadius &&
                distanceToVertex < nearDistance) {
                nearDistance = distanceToVertex;
                nearPoint = vertPointUnscaled;
            }
        }
    }
    return nearPoint;
}


//retrieve all the face hatches associated with this dvp
std::vector<TechDraw::DrawHatch*> DrawViewPart::getHatches() const
{
    std::vector<TechDraw::DrawHatch*> result;
    std::vector<App::DocumentObject*> children = getInList();
    for (auto& child : children) {
        if (child->isDerivedFrom<DrawHatch>() && !child->isRemoving()) {
            TechDraw::DrawHatch* hatch = static_cast<TechDraw::DrawHatch*>(child);
            result.push_back(hatch);
        }
    }
    return result;
}

//retrieve all the geometric hatches associated with this dvp
std::vector<TechDraw::DrawGeomHatch*> DrawViewPart::getGeomHatches() const
{
    std::vector<TechDraw::DrawGeomHatch*> result;
    std::vector<App::DocumentObject*> children = getInList();
    for (auto& child : children) {
        if (child->isDerivedFrom<DrawGeomHatch>()
            && !child->isRemoving()) {
            TechDraw::DrawGeomHatch* geom = static_cast<TechDraw::DrawGeomHatch*>(child);
            result.push_back(geom);
        }
    }
    return result;
}

//return *unique* list of Dimensions which reference this DVP
//if the dimension has two references to this dvp, it will appear twice in
//the inlist
std::vector<TechDraw::DrawViewDimension*> DrawViewPart::getDimensions() const
{
    std::vector<TechDraw::DrawViewDimension*> result;
    std::vector<App::DocumentObject*> children = getInList();
    std::sort(children.begin(), children.end(), std::less<>());
    std::vector<App::DocumentObject*>::iterator newEnd =
        std::unique(children.begin(), children.end());
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != newEnd; ++it) {
        if ((*it)->isDerivedFrom<DrawViewDimension>()) {
            TechDraw::DrawViewDimension* dim = static_cast<TechDraw::DrawViewDimension*>(*it);
            result.push_back(dim);
        }
    }
    return result;
}

std::vector<TechDraw::DrawViewBalloon*> DrawViewPart::getBalloons() const
{
    std::vector<TechDraw::DrawViewBalloon*> result;
    std::vector<App::DocumentObject*> children = getInList();
    std::sort(children.begin(), children.end(), std::less<>());
    std::vector<App::DocumentObject*>::iterator newEnd =
        std::unique(children.begin(), children.end());
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != newEnd; ++it) {
        if ((*it)->isDerivedFrom<DrawViewBalloon>()) {
            TechDraw::DrawViewBalloon* balloon = static_cast<TechDraw::DrawViewBalloon*>(*it);
            result.push_back(balloon);
        }
    }
    return result;
}

const std::vector<TechDraw::VertexPtr> DrawViewPart::getVertexGeometry() const
{
    if (geometryObject) {
        return geometryObject->getVertexGeometry();
    }
    return std::vector<TechDraw::VertexPtr>();
}


//! TechDraw vertex names run from 0 to n-1
TechDraw::VertexPtr DrawViewPart::getVertex(std::string vertexName) const
{
    // Base::Console().message("DVP::getVertex(%s)\n", vertexName.c_str());
    auto vertexIndex = DrawUtil::getIndexFromName(vertexName);
    auto vertex = getProjVertexByIndex(vertexIndex);
    return vertex;
}

//! returns existing BaseGeom of 2D Edge
//! TechDraw edge names run from 0 to n-1
TechDraw::BaseGeomPtr DrawViewPart::getEdge(std::string edgeName) const
{
    const std::vector<TechDraw::BaseGeomPtr>& geoms = getEdgeGeometry();
    if (geoms.empty()) {
        //should not happen
        return nullptr;
    }
    size_t iEdge = DrawUtil::getIndexFromName(edgeName);
    if ((unsigned)iEdge >= geoms.size()) {
        return nullptr;
    }
    return geoms.at(iEdge);
}


//! returns existing 2d Face
//! TechDraw face names run from 0 to n-1
TechDraw::FacePtr DrawViewPart::getFace(std::string faceName) const
{
    const std::vector<TechDraw::FacePtr>& faces = getFaceGeometry();
    if (faces.empty()) {
        //should not happen
        return nullptr;
    }
    size_t iFace = DrawUtil::getIndexFromName(faceName);
    if (iFace >= faces.size()) {
        return nullptr;
    }
    return faces.at(iFace);
}


const std::vector<TechDraw::FacePtr> DrawViewPart::getFaceGeometry() const
{
    std::vector<TechDraw::FacePtr> result;
    if (waitingForFaces() || !geometryObject) {
        return std::vector<TechDraw::FacePtr>();
    }
    return geometryObject->getFaceGeometry();
}

const BaseGeomPtrVector DrawViewPart::getEdgeGeometry() const
{
    if (geometryObject) {
        return geometryObject->getEdgeGeometry();
    }
    return BaseGeomPtrVector();
}

//! returns existing BaseGeom of 2D Edge(idx)
TechDraw::BaseGeomPtr DrawViewPart::getGeomByIndex(int idx) const
{
    const std::vector<TechDraw::BaseGeomPtr>& geoms = getEdgeGeometry();
    if (geoms.empty()) {
        return nullptr;
    }
    if (idx >= (int)geoms.size()) {
        return nullptr;
    }
    return geoms.at(idx);
}

//! returns existing geometry of 2D Vertex(idx)
TechDraw::VertexPtr DrawViewPart::getProjVertexByIndex(int idx) const
{
    const std::vector<TechDraw::VertexPtr>& geoms = getVertexGeometry();
    if (geoms.empty()) {
       return nullptr;
    }
    if ((unsigned)idx >= geoms.size()) {
        return nullptr;
    }
    return geoms.at(idx);
}

TechDraw::VertexPtr DrawViewPart::getProjVertexByCosTag(std::string cosTag)
{
    std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
    if (gVerts.empty()) {
        return nullptr;
    }

    for (auto& gv : gVerts) {
        if (gv->getCosmeticTag() == cosTag) {
            return gv;
        }
    }
    return nullptr;
}


//! returns existing geometry of 2D Face(idx)
std::vector<TechDraw::BaseGeomPtr> DrawViewPart::getFaceEdgesByIndex(int idx) const
{
    std::vector<TechDraw::BaseGeomPtr> result;
    const std::vector<TechDraw::FacePtr>& faces = getFaceGeometry();
    if (idx < (int)faces.size()) {
        TechDraw::FacePtr projFace = faces.at(idx);
        for (auto& w : projFace->wires) {
            for (auto& g : w->geoms) {
                if (g->getCosmetic()) {
                    //if g is cosmetic, we should skip it
                    continue;
                }
                else {
                    result.push_back(g);
                }
            }
        }
    }
    return result;
}

std::vector<TopoDS_Wire> DrawViewPart::getWireForFace(int idx) const
{
    std::vector<TopoDS_Wire> result;
    std::vector<TopoDS_Edge> edges;
    const std::vector<TechDraw::FacePtr>& faces = getFaceGeometry();
    TechDraw::FacePtr ourFace = faces.at(idx);
    for (auto& w : ourFace->wires) {
        edges.clear();
        for (auto& g : w->geoms) {
            edges.push_back(g->getOCCEdge());
        }
        TopoDS_Wire occwire = EdgeWalker::makeCleanWire(edges);
        result.push_back(occwire);
    }

    return result;
}

Base::BoundBox3d DrawViewPart::getBoundingBox() const { return bbox; }

double DrawViewPart::getBoxX() const
{
    Base::BoundBox3d bbx = getBoundingBox();//bbox is already scaled & centered!
    return (bbx.MaxX - bbx.MinX);
}

double DrawViewPart::getBoxY() const
{
    Base::BoundBox3d bbx = getBoundingBox();
    return (bbx.MaxY - bbx.MinY);
}

QRectF DrawViewPart::getRect() const
{
    //    Base::Console().message("DVP::getRect() - %s\n", getNameInDocument());
    double x = getBoxX();
    double y = getBoxY();
    return QRectF(0.0, 0.0, x, y);
}

//returns a compound of all the visible projected edges
TopoDS_Shape DrawViewPart::getEdgeCompound() const
{
    BRep_Builder builder;
    TopoDS_Compound result;
    builder.MakeCompound(result);
    if (geometryObject) {
        if (!geometryObject->getVisHard().IsNull()) {
            builder.Add(result, geometryObject->getVisHard());
        }
        if (!geometryObject->getVisOutline().IsNull()) {
            builder.Add(result, geometryObject->getVisOutline());
        }
        if (!geometryObject->getVisSeam().IsNull()) {
            builder.Add(result, geometryObject->getVisSeam());
        }
        if (!geometryObject->getVisSmooth().IsNull()) {
            builder.Add(result, geometryObject->getVisSmooth());
        }
    }
    //check for empty compound
    if (!result.IsNull() && TopoDS_Iterator(result).More()) {
        return result;
    }
    return TopoDS_Shape();
}

// returns the (unscaled) size of the visible lines along the alignment vector.
// alignment vector is already projected onto our CS, so only has X,Y components
// used in calculating the length of a section line
double DrawViewPart::getSizeAlongVector(Base::Vector3d alignmentVector)
{
    //    Base::Console().message("DVP::GetSizeAlongVector(%s)\n", DrawUtil::formatVector(alignmentVector).c_str());
    double alignmentAngle = atan2(alignmentVector.y, alignmentVector.x) * -1.0;
    gp_Ax2 OXYZ;//shape has already been projected and we will rotate around Z
    if (getEdgeCompound().IsNull()) {
        return 1.0;
    }
    TopoDS_Shape rotatedShape = ShapeUtils::rotateShape(getEdgeCompound(), OXYZ, Base::toDegrees(alignmentAngle));
    Bnd_Box shapeBox;
    shapeBox.SetGap(0.0);
    BRepBndLib::AddOptimal(rotatedShape, shapeBox);
    double xMin = 0, xMax = 0, yMin = 0, yMax = 0, zMin = 0, zMax = 0;
    shapeBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    double shapeWidth((xMax - xMin) / getScale());
    return shapeWidth;
}

//used to project a pt (ex SectionOrigin) onto paper plane
Base::Vector3d DrawViewPart::projectPoint(const Base::Vector3d& pt, bool invert) const
{
    //    Base::Console().message("DVP::projectPoint(%s, %d\n",
    //                            DrawUtil::formatVector(pt).c_str(), invert);
    Base::Vector3d stdOrg(0.0, 0.0, 0.0);
    gp_Ax2 viewAxis = getProjectionCS(stdOrg);
    gp_Pnt gPt(pt.x, pt.y, pt.z);

    HLRAlgo_Projector projector(viewAxis);
    gp_Pnt2d prjPnt;
    projector.Project(gPt, prjPnt);
    Base::Vector3d result(prjPnt.X(), prjPnt.Y(), 0.0);
    if (invert) {
        result = DrawUtil::invertY(result);
    }
    return result;
}

//project an edge onto the paper plane
BaseGeomPtr DrawViewPart::projectEdge(const TopoDS_Edge& e) const
{
    Base::Vector3d stdOrg(0.0, 0.0, 0.0);
    gp_Ax2 viewAxis = getProjectionCS(stdOrg);

    gp_Pln plane(viewAxis);
    TopoDS_Face paper = BRepBuilderAPI_MakeFace(plane);
    BRepAlgo_NormalProjection projector(paper);
    projector.Add(e);
    projector.Build();
    TopoDS_Shape s = projector.Projection();
    return BaseGeom::baseFactory(TopoDS::Edge(s));
}

bool DrawViewPart::waitingForResult() const
{
    if (waitingForHlr() || waitingForFaces()) {
        return true;
    }
    return false;
}

bool DrawViewPart::hasGeometry() const
{
    if (!geometryObject) {
        return false;
    }

    const std::vector<TechDraw::VertexPtr>& verts = getVertexGeometry();
    const std::vector<TechDraw::BaseGeomPtr>& edges = getEdgeGeometry();

    return !(verts.empty() && edges.empty());
}

//convert a vector in local XY coords into a coordinate system in global
//coordinates aligned to the vector.
//Note that this CS may not have the ideal XDirection for the derived view
//(likely a DrawViewSection) and the user may need to adjust the XDirection
//in the derived view.
gp_Ax2 DrawViewPart::localVectorToCS(const Base::Vector3d localUnit) const
{
    //    Base::Console().message("DVP::localVectorToCS(%s)\n", DU::formatVector((localUnit)).c_str());
    double angle = atan2(localUnit.y, localUnit.x);//radians
    gp_Ax1 rotateAxisDir(gp_Pnt(0.0, 0.0, 0.0), getProjectionCS().Direction());
    gp_Vec gOldX = getProjectionCS().XDirection();
    gp_Vec gNewDirection = gOldX.Rotated(rotateAxisDir, angle);
    gp_Vec gNewY = getProjectionCS().Direction();
    gp_Vec gNewX = (gNewDirection.Crossed(gNewY).Reversed());
    if (gNewX.IsParallel(gOldX, EWTOLERANCE)) {
        //if the X directions are parallel, the view is rotating around X, so
        //we should use the original X to prevent unwanted mirroring.
        //There might be a better choice of tolerance than EWTOLERANCE
        gNewX = gOldX;
    }

    return {gp_Pnt(0.0, 0.0, 0.0), gp_Dir(gNewDirection), gp_Dir(gNewX)};
}


Base::Vector3d DrawViewPart::localVectorToDirection(const Base::Vector3d localUnit) const
{
    //    Base::Console().message("DVP::localVectorToDirection() - localUnit: %s\n", DrawUtil::formatVector(localUnit).c_str());
    gp_Ax2 cs = localVectorToCS(localUnit);
    return Base::convertTo<Base::Vector3d>(cs.Direction());
}

gp_Ax2 DrawViewPart::getProjectionCS(const Base::Vector3d pt) const
{
    //    Base::Console().message("DVP::getProjectionCS() - %s - %s\n", getNameInDocument(), Label.getValue());
    Base::Vector3d direction = Direction.getValue();
    gp_Dir gDir(direction.x, direction.y, direction.z);
    Base::Vector3d xDir = getXDirection();
    gp_Dir gXDir(xDir.x, xDir.y, xDir.z);
    gp_Pnt gOrg(pt.x, pt.y, pt.z);
    gp_Ax2 viewAxis(gOrg, gDir);
    try {
        viewAxis = gp_Ax2(gOrg, gDir, gXDir);
    }
    catch (...) {
        Base::Console().warning("DVP - %s - failed to create projection CS\n", getNameInDocument());
    }
    return viewAxis;
}

gp_Ax2 DrawViewPart::getRotatedCS(const Base::Vector3d basePoint) const
{
    //    Base::Console().message("DVP::getRotatedCS() - %s - %s\n", getNameInDocument(), Label.getValue());
    gp_Ax2 unrotated = getProjectionCS(basePoint);
    gp_Ax1 rotationAxis(Base::convertTo<gp_Pnt>(basePoint), unrotated.Direction());
    double angleRad = Base::toRadians(Rotation.getValue());
    gp_Ax2 rotated = unrotated.Rotated(rotationAxis, -angleRad);
    return rotated;
}

gp_Ax2 DrawViewPart::getViewAxis(const Base::Vector3d& pt, const Base::Vector3d& direction,
                                 const bool flip) const
{
    (void)direction;
    (void)flip;
    Base::Console().message("DVP::getViewAxis - deprecated. Use getProjectionCS.\n");
    return getProjectionCS(pt);
}

//TODO: make saveShape a property

Base::Vector3d DrawViewPart::getOriginalCentroid() const { return m_saveCentroid; }

Base::Vector3d DrawViewPart::getCurrentCentroid() const
{
    TopoDS_Shape shape = getSourceShape();
    if (shape.IsNull()) {
        return Base::Vector3d(0.0, 0.0, 0.0);
    }
    gp_Ax2 cs = getProjectionCS();
    gp_Pnt gCenter = ShapeUtils::findCentroid(shape, cs);
    return Base::convertTo<Base::Vector3d>(gCenter);
}

std::vector<DrawViewSection*> DrawViewPart::getSectionRefs() const
{
    std::vector<DrawViewSection*> result;
    std::vector<App::DocumentObject*> inObjs = getInList();
    for (auto& o : inObjs) {
        if (o->isDerivedFrom<DrawViewSection>()) {
            // expressions can add extra links to this DVP so we keep only
            // objects that are BaseViews
            auto section = static_cast<TechDraw::DrawViewSection*>(o);
            auto base = section->BaseView.getValue();
            if (base == this) {
                result.push_back(section);
            }
        }
    }
    return result;
}

std::vector<DrawViewDetail*> DrawViewPart::getDetailRefs() const
{
    std::vector<DrawViewDetail*> result;
    std::vector<App::DocumentObject*> inObjs = getInList();
    for (auto& o : inObjs) {
        if (o->isDerivedFrom<DrawViewDetail>() &&
            !o->isRemoving() ) {
            // expressions can add extra links to this DVP so we keep only
            // objects that are BaseViews
            auto detail = static_cast<TechDraw::DrawViewDetail*>(o);
            auto base = detail->BaseView.getValue();
            if (base == this) {
                result.push_back(detail);
            }

        }
    }
    return result;
}

const BaseGeomPtrVector DrawViewPart::getVisibleFaceEdges() const
{
    return geometryObject->getVisibleFaceEdges(SmoothVisible.getValue(), SeamVisible.getValue());
}

bool DrawViewPart::handleFaces()
{
    return Preferences::getPreferenceGroup("General")->GetBool("HandleFaces", true);
}

bool DrawViewPart::newFaceFinder()
{
    return Preferences::getPreferenceGroup("General")->GetBool("NewFaceFinder", false);
}

//! remove features that are useless without this DVP
//! hatches, geomhatches, dimensions, ...
void DrawViewPart::unsetupObject()
{
//    Base::Console().message("DVP::unsetupObject()\n");
    nowUnsetting = true;
    App::Document* doc = getDocument();
    std::string docName = doc->getName();

    // Remove the View's Hatches from document
    std::vector<TechDraw::DrawHatch*> hatches = getHatches();
    std::vector<TechDraw::DrawHatch*>::iterator it = hatches.begin();
    for (; it != hatches.end(); it++) {
        std::string viewName = (*it)->getNameInDocument();
        Base::Interpreter().runStringArg("App.getDocument(\"%s\").removeObject(\"%s\")",
                                         docName.c_str(), viewName.c_str());
    }

    // Remove the View's GeomHatches from document
    std::vector<TechDraw::DrawGeomHatch*> gHatches = getGeomHatches();
    std::vector<TechDraw::DrawGeomHatch*>::iterator it2 = gHatches.begin();
    for (; it2 != gHatches.end(); it2++) {
        std::string viewName = (*it2)->getNameInDocument();
        Base::Interpreter().runStringArg("App.getDocument(\"%s\").removeObject(\"%s\")",
                                         docName.c_str(), viewName.c_str());
    }

    // Remove Dimensions which reference this DVP
    // must use page->removeObject first
    TechDraw::DrawPage* page = findParentPage();
    if (page) {
        std::vector<TechDraw::DrawViewDimension*> dims = getDimensions();
        std::vector<TechDraw::DrawViewDimension*>::iterator it3 = dims.begin();
        for (; it3 != dims.end(); it3++) {
            page->removeView(*it3);
            const char* name = (*it3)->getNameInDocument();
            if (name) {
                Base::Interpreter().runStringArg("App.getDocument(\"%s\").removeObject(\"%s\")",
                                                 docName.c_str(), name);
            }
        }
    }

    // Remove Balloons which reference this DVP
    // must use page->removeObject first
    page = findParentPage();
    if (page) {
        std::vector<TechDraw::DrawViewBalloon*> balloons = getBalloons();
        std::vector<TechDraw::DrawViewBalloon*>::iterator it3 = balloons.begin();
        for (; it3 != balloons.end(); it3++) {
            page->removeView(*it3);
            const char* name = (*it3)->getNameInDocument();
            if (name) {
                Base::Interpreter().runStringArg("App.getDocument(\"%s\").removeObject(\"%s\")",
                                                 docName.c_str(), name);
            }
        }
    }
}

bool DrawViewPart::checkXDirection() const
{
    //    Base::Console().message("DVP::checkXDirection()\n");
    Base::Vector3d xDir = XDirection.getValue();
    if (DrawUtil::fpCompare(xDir.Length(), 0.0)) {
        return false;
    }
    return true;
}

Base::Vector3d DrawViewPart::getXDirection() const
{
    //    Base::Console().message("DVP::getXDirection() - %s\n", Label.getValue());
    Base::Vector3d result(1.0, 0.0, 0.0);//default X
    App::Property* prop = getPropertyByName("XDirection");
    if (prop) {//have an XDirection property
        Base::Vector3d propVal = XDirection.getValue();
        if (DrawUtil::fpCompare(propVal.Length(), 0.0)) {//but it has no value
            Base::Vector3d dir = Direction.getValue();   //make a sensible default
            Base::Vector3d org(0.0, 0.0, 0.0);
            result = getLegacyX(org, dir);
        } else {
            result = propVal;	//normal case.  XDirection is set.
        }
    }
    else {                                        //no Property.  can this happen?
        Base::Vector3d dir = Direction.getValue();//make a sensible default
        Base::Vector3d org(0.0, 0.0, 0.0);
        result = getLegacyX(org, dir);
    }
    return result;
}


void DrawViewPart::rotate(const RotationMotion& motion)
{
    std::pair<Base::Vector3d, Base::Vector3d> newDirs;
    if (motion == RotationMotion::Right)
        newDirs = getDirsFromFront(ProjDirection::Left);// Front -> Right -> Rear -> Left -> Front
    else if (motion == RotationMotion::Left)
        newDirs = getDirsFromFront(ProjDirection::Right);// Front -> Left -> Rear -> Right -> Front
    else if (motion == RotationMotion::Up)
        newDirs = getDirsFromFront(ProjDirection::Bottom);// Front -> Top -> Rear -> Bottom -> Front
    else if (motion == RotationMotion::Down)
        newDirs = getDirsFromFront(ProjDirection::Top);// Front -> Bottom -> Rear -> Top -> Front

    Direction.setValue(newDirs.first);
    XDirection.setValue(newDirs.second);
    recompute();
}

void DrawViewPart::spin(const SpinDirection& spindirection)
{
    double angle;
    if (spindirection == SpinDirection::CW)
        angle = std::numbers::pi / 2.0;// Top -> Right -> Bottom -> Left -> Top
    if (spindirection == SpinDirection::CCW)
        angle = -std::numbers::pi / 2.0;// Top -> Left -> Bottom -> Right -> Top

    spin(angle);
}

void DrawViewPart::spin(double angle)
{
    Base::Vector3d org(0.0, 0.0, 0.0);
    Base::Vector3d curRot = getXDirection();
    Base::Vector3d curDir = Direction.getValue();
    Base::Vector3d newRot = DrawUtil::vecRotate(curRot, angle, curDir, org);

    XDirection.setValue(newRot);
    recompute();
}

std::pair<Base::Vector3d, Base::Vector3d> DrawViewPart::getDirsFromFront(ProjDirection viewType)
{
    //    Base::Console().message("DVP::getDirsFromFront(%s)\n", viewType.c_str());
    std::pair<Base::Vector3d, Base::Vector3d> result;

    Base::Vector3d projDir, rotVec;

    Base::Vector3d org(0.0, 0.0, 0.0);
    gp_Ax2 anchorCS = getProjectionCS(org);
    gp_Pnt gOrg(0.0, 0.0, 0.0);
    gp_Dir gDir = anchorCS.Direction();
    gp_Dir gXDir = anchorCS.XDirection();
    gp_Dir gYDir = anchorCS.YDirection();
    gp_Ax1 gUpAxis(gOrg, gYDir);
    gp_Ax2 newCS;
    gp_Dir gNewDir;
    gp_Dir gNewXDir;

    double angle = std::numbers::pi / 2.0;//90*

    if (viewType == ProjDirection::Right) {
        newCS = anchorCS.Rotated(gUpAxis, angle);
        projDir = dir2vec(newCS.Direction());
        rotVec = dir2vec(newCS.XDirection());
    }
    else if (viewType == ProjDirection::Left) {
        newCS = anchorCS.Rotated(gUpAxis, -angle);
        projDir = dir2vec(newCS.Direction());
        rotVec = dir2vec(newCS.XDirection());
    }
    else if (viewType == ProjDirection::Top) {
        projDir = dir2vec(gYDir);
        rotVec = dir2vec(gXDir);
    }
    else if (viewType == ProjDirection::Bottom) {
        projDir = dir2vec(gYDir.Reversed());
        rotVec = dir2vec(gXDir);
    }
    else if (viewType == ProjDirection::Rear) {
        projDir = dir2vec(gDir.Reversed());
        rotVec = dir2vec(gXDir.Reversed());
    }
    else if (viewType == ProjDirection::FrontTopLeft) {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) - gp_Vec(gXDir) + gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) + gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    }
    else if (viewType == ProjDirection::FrontTopRight) {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) + gp_Vec(gXDir) + gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) - gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    }
    else if (viewType == ProjDirection::FrontBottomLeft) {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) - gp_Vec(gXDir) - gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) + gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    }
    else if (viewType == ProjDirection::FrontBottomRight) {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) + gp_Vec(gXDir) - gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) - gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    }

    return std::make_pair(projDir, rotVec);
}

Base::Vector3d DrawViewPart::dir2vec(gp_Dir d)
{
    return Base::Vector3d(d.X(), d.Y(), d.Z());
}

Base::Vector3d DrawViewPart::getLegacyX(const Base::Vector3d& pt, const Base::Vector3d& axis,
                                        const bool flip) const
{
    gp_Ax2 viewAxis = ShapeUtils::legacyViewAxis1(pt, axis, flip);
    gp_Dir gXDir = viewAxis.XDirection();
    return Base::Vector3d(gXDir.X(), gXDir.Y(), gXDir.Z());
}

// reference dimensions & their vertices
// these routines may be obsolete

void DrawViewPart::updateReferenceVert(std::string tag, Base::Vector3d loc2d)
{
    for (auto& v : m_referenceVerts) {
        if (v->getTagAsString() == tag) {
            v->point(loc2d);
            break;
        }
    }
}

void DrawViewPart::addReferencesToGeom()
{
    std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
    gVerts.insert(gVerts.end(), m_referenceVerts.begin(), m_referenceVerts.end());
    getGeometryObject()->setVertexGeometry(gVerts);
}

//add a vertex that is not part of the geometry, but is used by
//ex. LandmarkDimension as a reference
std::string DrawViewPart::addReferenceVertex(Base::Vector3d v)
{
    std::string refTag;
    Base::Vector3d scaledV = v;
    TechDraw::VertexPtr ref(std::make_shared<TechDraw::Vertex>(scaledV));
    ref->isReference(true);
    refTag = ref->getTagAsString();
    m_referenceVerts.push_back(ref);
    return refTag;
}

void DrawViewPart::removeReferenceVertex(std::string tag)
{
    std::vector<TechDraw::VertexPtr> newRefVerts;
    for (auto& v : m_referenceVerts) {
        if (v->getTagAsString() != tag) {
            newRefVerts.push_back(v);
        }
        else {
            //            delete v;  //??? who deletes v?
        }
    }
    m_referenceVerts = newRefVerts;
    resetReferenceVerts();
}

//! remove reference vertexes from the view geometry
void DrawViewPart::removeAllReferencesFromGeom()
{
    //    Base::Console().message("DVP::removeAllReferencesFromGeom()\n");
    if (!m_referenceVerts.empty()) {
        std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
        std::vector<TechDraw::VertexPtr> newVerts;
        for (auto& gv : gVerts) {
            if (!gv->isReference()) {
                newVerts.push_back(gv);
            }
        }
        getGeometryObject()->setVertexGeometry(newVerts);
    }
}

void DrawViewPart::resetReferenceVerts()
{
    //    Base::Console().message("DVP::resetReferenceVerts() %s\n", getNameInDocument());
    removeAllReferencesFromGeom();
    addReferencesToGeom();
}

void DrawViewPart::handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, App::Property * prop)
{
    if (prop == &Direction) {
        // Direction was PropertyVector, then briefly PropertyDirection, now back to PropertyVector
        App::PropertyDirection tmp;
        if (strcmp(tmp.getTypeId().getName(), TypeName)==0) {
            tmp.setContainer(this);
            tmp.Restore(reader);
            auto tmpValue = tmp.getValue();
            Direction.setValue(tmpValue);
        }
        return;
    }

    if (prop == &XDirection) {
        // XDirection was PropertyVector, then briefly PropertyDirection, now back to PropertyVector
        App::PropertyDirection tmp;
        if (strcmp(tmp.getTypeId().getName(), TypeName)==0) {
            tmp.setContainer(this);
            tmp.Restore(reader);
            auto tmpValue = tmp.getValue();
            XDirection.setValue(tmpValue);
        }
        return;
    }
}

// true if owner->element is a cosmetic vertex
bool DrawViewPart::isCosmeticVertex(const std::string& element)
{
    auto vertexIndex = DrawUtil::getIndexFromName(element);
    auto vertex = getProjVertexByIndex(vertexIndex);
    if (vertex) {
        return vertex->getCosmetic();
    }
    return false;
}

// true if owner->element is a cosmetic edge
bool DrawViewPart::isCosmeticEdge(const std::string& element)
{
    auto edge = getEdge(element);
    if (edge && edge->source() == SourceType::COSMETICEDGE && edge->getCosmetic()) {
        return true;
    }
    return false;
}

// true if owner->element is a center line
bool DrawViewPart::isCenterLine(const std::string& element)
{
    auto edge = getEdge(element);
    if (edge && edge->source() == SourceType::CENTERLINE && edge->getCosmetic()) {
        return true;
    }
    return false;
}

// debugging ----------------------------------------------------------------------------

void DrawViewPart::dumpVerts(std::string text)
{
    if (!geometryObject) {
        Base::Console().message("no verts to dump yet\n");
        return;
    }
    std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
    Base::Console().message("%s - dumping %d vertGeoms\n", text.c_str(), gVerts.size());
    for (auto& gv : gVerts) {
        gv->dump();
    }
}

void DrawViewPart::dumpCosVerts(std::string text)
{
    std::vector<TechDraw::CosmeticVertex*> cVerts = CosmeticVertexes.getValues();
    Base::Console().message("%s - dumping %d CosmeticVertexes\n", text.c_str(), cVerts.size());
    for (auto& cv : cVerts) {
        cv->dump("a CV");
    }
}

void DrawViewPart::dumpCosEdges(std::string text)
{
    std::vector<TechDraw::CosmeticEdge*> cEdges = CosmeticEdges.getValues();
    Base::Console().message("%s - dumping %d CosmeticEdge\n", text.c_str(), cEdges.size());
    for (auto& ce : cEdges) {
        ce->dump("a CE");
    }
}

PyObject* DrawViewPart::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewPartPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewPartPython, TechDraw::DrawViewPart)
template<> const char* TechDraw::DrawViewPartPython::getViewProviderName() const
{
    return "TechDrawGui::ViewProviderViewPart";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewPart>;
}// namespace App
