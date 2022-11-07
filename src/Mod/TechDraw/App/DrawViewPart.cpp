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
#include <sstream>
#include <QtConcurrentRun>
#include <Bnd_Box.hxx>
#include <BRep_Tool.hxx>
#include <BRepAlgo_NormalProjection.hxx>
#include <BRepBndLib.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepTools.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <HLRAlgo_Projector.hxx>
#include <ShapeAnalysis.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>

#include "DrawViewPart.h"
#include "DrawViewPartPy.h"  // generated from DrawViewPartPy.xml
#include "Cosmetic.h"
#include "DrawGeomHatch.h"
#include "DrawHatch.h"
#include "DrawPage.h"
#include "DrawProjectSplit.h"
#include "DrawUtil.h"
#include "DrawViewBalloon.h"
#include "DrawViewDetail.h"
#include "DrawViewDimension.h"
#include "DrawViewSection.h"
#include "EdgeWalker.h"
#include "Geometry.h"
#include "GeometryObject.h"
#include "ShapeExtractor.h"


using namespace TechDraw;

//===========================================================================
// DrawViewPart
//===========================================================================


//PROPERTY_SOURCE(TechDraw::DrawViewPart, TechDraw::DrawView)
PROPERTY_SOURCE_WITH_EXTENSIONS(TechDraw::DrawViewPart,
                                TechDraw::DrawView)

DrawViewPart::DrawViewPart(void) :
    geometryObject(nullptr),
    m_tempGeometryObject(nullptr),
    m_waitingForFaces(false),
    m_waitingForHlr(false)
{
    static const char *group = "Projection";
    static const char *sgroup = "HLR Parameters";
    nowUnsetting = false;
    m_handleFaces = false;

    CosmeticExtension::initExtension(this);

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().
                                         GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    double defDist = hGrp->GetFloat("FocusDistance", 100.0);

    //properties that affect Geometry
    ADD_PROPERTY_TYPE(Source ,(nullptr), group, App::Prop_None, "3D Shape to view");
    Source.setScope(App::LinkScope::Global);
    Source.setAllowExternal(true);
    ADD_PROPERTY_TYPE(XSource ,(nullptr), group, App::Prop_None, "External 3D Shape to view");


    ADD_PROPERTY_TYPE(Direction ,(0.0, -1.0, 0.0),
                      group, App::Prop_None, "Projection Plane normal. The direction you are looking from.");
    ADD_PROPERTY_TYPE(XDirection ,(0.0, 0.0, 0.0),
                      group, App::Prop_None, "Projection Plane X Axis in R3. Rotates/Mirrors View");
    ADD_PROPERTY_TYPE(Perspective ,(false), group, App::Prop_None,
                      "Perspective(true) or Orthographic(false) projection");
    ADD_PROPERTY_TYPE(Focus, (defDist), group, App::Prop_None, "Perspective view focus distance");

    //properties that control HLR algo
    bool coarseView = hGrp->GetBool("CoarseView", false);
    ADD_PROPERTY_TYPE(CoarseView, (coarseView), sgroup, App::Prop_None, "Coarse View on/off");
    ADD_PROPERTY_TYPE(SmoothVisible ,(prefSmoothViz()), sgroup, App::Prop_None, "Show Visible Smooth lines");
    ADD_PROPERTY_TYPE(SeamVisible ,(prefSeamViz()), sgroup, App::Prop_None, "Show Visible Seam lines");
    ADD_PROPERTY_TYPE(IsoVisible ,(prefIsoViz()), sgroup, App::Prop_None, "Show Visible Iso u, v lines");
    ADD_PROPERTY_TYPE(HardHidden ,(prefHardHid()), sgroup, App::Prop_None, "Show Hidden Hard lines");
    ADD_PROPERTY_TYPE(SmoothHidden ,(prefSmoothHid()), sgroup, App::Prop_None, "Show Hidden Smooth lines");
    ADD_PROPERTY_TYPE(SeamHidden ,(prefSeamHid()), sgroup, App::Prop_None, "Show Hidden Seam lines");
    ADD_PROPERTY_TYPE(IsoHidden ,(prefIsoHid()), sgroup, App::Prop_None, "Show Hidden Iso u, v lines");
    ADD_PROPERTY_TYPE(IsoCount ,(prefIsoCount()), sgroup, App::Prop_None, "Number of iso parameters lines");

    //initialize bbox to non-garbage
    bbox = Base::BoundBox3d(Base::Vector3d(0.0, 0.0, 0.0), 0.0);
}

DrawViewPart::~DrawViewPart()
{
    //don't delete this object while it still has dependent threads running
    if (m_hlrFuture.isRunning()) {
        Base::Console().Message("%s is waiting for HLR to finish\n", Label.getValue());
        m_hlrFuture.waitForFinished();
    }
    if (m_faceFuture.isRunning()) {
        Base::Console().Message("%s is waiting for face finding to finish\n", Label.getValue());
        m_faceFuture.waitForFinished();
    }
    removeAllReferencesFromGeom();
    delete geometryObject;
}

std::vector<TopoDS_Shape> DrawViewPart::getSourceShape2d() const
{
//    Base::Console().Message("DVP::getSourceShape2d()\n");
    std::vector<TopoDS_Shape> result;
    const std::vector<App::DocumentObject*>& links = getAllSources();
    result = ShapeExtractor::getShapes2d(links);
    return result;
}

TopoDS_Shape DrawViewPart::getSourceShape() const
{
//    Base::Console().Message("DVP::getSourceShape()\n");
    const std::vector<App::DocumentObject*>& links = getAllSources();
    if (links.empty())  {
        return TopoDS_Shape();
    }
    return ShapeExtractor::getShapes(links);
}

TopoDS_Shape DrawViewPart::getSourceShapeFused() const
{
//    Base::Console().Message("DVP::getSourceShapeFused()\n");
    const std::vector<App::DocumentObject*>& links = getAllSources();
    if (links.empty())  {
        return TopoDS_Shape();
    }
    return ShapeExtractor::getShapesFused(links);
}

std::vector<App::DocumentObject*> DrawViewPart::getAllSources() const
{
//    Base::Console().Message("DVP::getAllSources()\n");
    std::vector<App::DocumentObject*> links = Source.getValues();
    std::vector<DocumentObject*> xLinks = XSource.getValues();

    std::vector<App::DocumentObject*> result = links;
    if (!xLinks.empty()) {
        result.insert(result.end(), xLinks.begin(), xLinks.end());
    }
    return result;
}

//pick supported 2d shapes out of the Source properties and
//add them directly to the geometry without going through HLR
void DrawViewPart::addShapes2d(void)
{
    std::vector<TopoDS_Shape> shapes = getSourceShape2d();
    for (auto& s: shapes) {
        //just vertices for now
        if (s.ShapeType() == TopAbs_VERTEX) {
            gp_Pnt gp = BRep_Tool::Pnt(TopoDS::Vertex(s));
            Base::Vector3d vp(gp.X(), gp.Y(), gp.Z());
            vp = vp - m_saveCentroid;
            //need to offset the point to match the big projection
            Base::Vector3d projected = projectPoint(vp * getScale());
            TechDraw::VertexPtr v1(std::make_shared<TechDraw::Vertex>(projected));
            geometryObject->addVertex(v1);
        } else if (s.ShapeType() == TopAbs_EDGE) {
              //not supporting edges yet.
//            Base::Console().Message("DVP::add2dShapes - found loose edge - isNull: %d\n", s.IsNull());
//            TopoDS_Shape sTrans = TechDraw::moveShape(s,
//                                                      m_saveCentroid * -1.0);
//            TopoDS_Shape sScale = TechDraw::scaleShape(sTrans,
//                                                       getScale());
//            TopoDS_Shape sMirror = TechDraw::mirrorShape(sScale);
//            TopoDS_Edge edge = TopoDS::Edge(sMirror);
//            BaseGeomPtr bg = projectEdge(edge);

//            geometryObject->addEdge(bg);
            //save connection between source feat and this edge
        }
    }
}

App::DocumentObjectExecReturn *DrawViewPart::execute(void)
{
//    Base::Console().Message("DVP::execute() - %s\n", getNameInDocument());
    if (!keepUpdated()) {
        return DrawView::execute();
    }

    if (waitingForHlr()) {
        return DrawView::execute();
    }

    TopoDS_Shape shape = getSourceShape();
    if (shape.IsNull()) {
        Base::Console().Message("DVP::execute - %s - Source shape is Null.\n",
                            getNameInDocument());
        return DrawView::execute();
    }

    //make sure the XDirection property is valid. Mostly for older models.
    if (!checkXDirection()) {
        Base::Vector3d newX = getXDirection();
        XDirection.setValue(newX);
        XDirection.purgeTouched();  //don't trigger updates!
    }

    m_saveShape = shape;
    partExec(shape);

    return DrawView::execute();
}

short DrawViewPart::mustExecute() const
{
    if (isRestoring()) {
        return TechDraw::DrawView::mustExecute();
    }

    if (Direction.isTouched()  ||
        Source.isTouched()  ||
        XSource.isTouched()  ||
        Perspective.isTouched() ||
        Focus.isTouched() ||
        Rotation.isTouched() ||
        SmoothVisible.isTouched() ||
        SeamVisible.isTouched() ||
        IsoVisible.isTouched() ||
        HardHidden.isTouched() ||
        SmoothHidden.isTouched() ||
        SeamHidden.isTouched() ||
        IsoHidden.isTouched() ||
        IsoCount.isTouched() ||
        CoarseView.isTouched() ||
        CosmeticVertexes.isTouched() ||
        CosmeticEdges.isTouched() ||
        CenterLines.isTouched()) {
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
        Direction.setValue(Base::Vector3d(0.0, -1.0, 0.0));
    }

    DrawView::onChanged(prop);
}

void DrawViewPart::partExec(TopoDS_Shape& shape)
{
//    Base::Console().Message("DVP::partExec() - %s\n", getNameInDocument());
    if (waitingForHlr()) {
        //finish what we are already doing before starting a new cycle
        return;
    }

    //we need to keep using the old geometryObject until the new one is fully populated
    m_tempGeometryObject = makeGeometryForShape(shape);
    if (CoarseView.getValue()){
        onHlrFinished();    //poly algo does not run in separate thread, so we need to invoke
                            //the post hlr processing manually
    }
}

//prepare the shape for HLR processing by centering, scaling and rotating it
GeometryObject* DrawViewPart::makeGeometryForShape(TopoDS_Shape& shape)
{
//    Base::Console().Message("DVP::makeGeometryForShape() - %s\n", getNameInDocument());
    gp_Pnt inputCenter;
    Base::Vector3d stdOrg(0.0, 0.0, 0.0);
    gp_Ax2 viewAxis = getProjectionCS(stdOrg);
    inputCenter = TechDraw::findCentroid(shape,
                                         viewAxis);
    Base::Vector3d centroid(inputCenter.X(),
                            inputCenter.Y(),
                            inputCenter.Z());

    //center shape on origin
    TopoDS_Shape centeredShape = TechDraw::moveShape(shape,
                                                     centroid * -1.0);
    m_saveCentroid = centroid;
    m_saveShape = centeredShape;

    TopoDS_Shape scaledShape = TechDraw::scaleShape(centeredShape,
                                                    getScale());
    if (!DrawUtil::fpCompare(Rotation.getValue(), 0.0)) {
        scaledShape = TechDraw::rotateShape(scaledShape,
                                            viewAxis,
                                            Rotation.getValue());  //conventional rotation
     }
//    BRepTools::Write(scaledShape, "DVPScaled.brep");            //debug
    GeometryObject* go =  buildGeometryObject(scaledShape, viewAxis);
    return go;
}

//create a geometry object and trigger the HLR process in another thread
TechDraw::GeometryObject* DrawViewPart::buildGeometryObject(TopoDS_Shape& shape, const gp_Ax2 &viewAxis)
{
//    Base::Console().Message("DVP::buildGeometryObject() - %s\n", getNameInDocument());
    showProgressMessage(getNameInDocument(), "is finding hidden lines");

    TechDraw::GeometryObject* go = new TechDraw::GeometryObject(getNameInDocument(), this);
    go->setIsoCount(IsoCount.getValue());
    go->isPerspective(Perspective.getValue());
    go->setFocus(Focus.getValue());
    go->usePolygonHLR(CoarseView.getValue());

    if (CoarseView.getValue()){
        //the polygon approximation HLR process runs quickly, so doesn't need to be in a
        //separate thread
        go->projectShapeWithPolygonAlgo(shape,
                                        viewAxis);
    } else {
        //projectShape (the HLR process) runs in a separate thread since it can take a long time
        //note that &m_hlrWatcher in the third parameter is not strictly required, but using the
        //4 parameter signature instead of the 3 parameter signature prevents clazy warning:
        //https://github.com/KDE/clazy/blob/1.11/docs/checks/README-connect-3arg-lambda.md
        connectHlrWatcher = QObject::connect(&m_hlrWatcher, &QFutureWatcherBase::finished,
                                             &m_hlrWatcher, [this] { this->onHlrFinished(); } );
        m_hlrFuture = QtConcurrent::run(go, &GeometryObject::projectShape, shape, viewAxis);
        m_hlrWatcher.setFuture(m_hlrFuture);
        waitingForHlr(true);
    }
    return go;
}

//continue processing after hlr thread completes
void DrawViewPart::onHlrFinished(void)
{
//    Base::Console().Message("DVP::onHlrFinished() - %s\n", getNameInDocument());

    //now that the new GeometryObject is fully populated, we can replace the old one
    if (geometryObject &&
            m_tempGeometryObject) {
        delete geometryObject;      //remove the old
    }
    if (m_tempGeometryObject) {
        geometryObject = m_tempGeometryObject;  //replace with new
        m_tempGeometryObject = nullptr;     //superfluous?
    }
    if (!geometryObject) {
        throw Base::RuntimeError("DrawViewPart has lost its geometry");
    }

    //the last hlr related task is to make a bbox of the results
    bbox = geometryObject->calcBoundingBox();

    waitingForHlr(false);
    QObject::disconnect(connectHlrWatcher);
    showProgressMessage(getNameInDocument(), "has finished finding hidden lines");

    postHlrTasks();         //application level tasks that depend on HLR/GO being complete

    //start face finding in a separate thread.  We don't find faces when using the polygon
    //HLR method.
    if (handleFaces() && !CoarseView.getValue() ) {
        try {
            //note that &m_faceWatcher in the third parameter is not strictly required, but using the
            //4 parameter signature instead of the 3 parameter signature prevents clazy warning:
            //https://github.com/KDE/clazy/blob/1.11/docs/checks/README-connect-3arg-lambda.md
            connectFaceWatcher = QObject::connect(&m_faceWatcher, &QFutureWatcherBase::finished,
                                                  &m_faceWatcher, [this] { this->onFacesFinished(); });
            m_faceFuture = QtConcurrent::run(this, &DrawViewPart::extractFaces);
            m_faceWatcher.setFuture(m_faceFuture);
            waitingForFaces(true);
        }
        catch (Standard_Failure& e) {
            waitingForFaces(false);
            Base::Console().Error("DVP::partExec - %s - extractFaces failed - %s **\n", getNameInDocument(), e.GetMessageString());
            throw Base::RuntimeError("DVP::onHlrFinished - error extracting faces");
        }
    }
}

//run any tasks that need to been done after geometry is available
void DrawViewPart::postHlrTasks(void)
{
//    Base::Console().Message("DVP::postHlrTasks() - %s\n", getNameInDocument());
    //add geometry that doesn't come from HLR
    addCosmeticVertexesToGeom();
    addCosmeticEdgesToGeom();
    addReferencesToGeom();
    addShapes2d();

    //dimensions and balloons need to be recomputed here because their
    //references will be invalid until the geometry exists
    std::vector<TechDraw::DrawViewDimension*> dims = getDimensions();
    for (auto& d : dims) {
        d->recomputeFeature();
    }
    std::vector<TechDraw::DrawViewBalloon*> bals = getBalloons();
    for (auto& b : bals) {
        b->recomputeFeature();
    }

    //second pass if required
    if (ScaleType.isValue("Automatic") &&
        !checkFit()) {
        double newScale = autoScale();
        Scale.setValue(newScale);
        Scale.purgeTouched();
        partExec(m_saveShape);
    }

    overrideKeepUpdated(false);

    requestPaint();
}

//! make faces from the edge geometry
void DrawViewPart::extractFaces()
{
//    Base::Console().Message("DVP::extractFaces() - %s waitingForHlr: %d waitingForFaces: %d\n",
//                            getNameInDocument(), waitingForHlr(), waitingForFaces());
    if ( !geometryObject ) {
        //geometry is in flux, can not make faces right now
        return;
    }

    showProgressMessage(getNameInDocument(), "is extracting faces");

    const std::vector<TechDraw::BaseGeomPtr>& goEdges =
                       geometryObject->getVisibleFaceEdges(SmoothVisible.getValue(),SeamVisible.getValue());

    if (goEdges.empty()) {
//        Base::Console().Message("DVP::extractFaces - %s - no face edges available!\n", getNameInDocument());    //debug
        return;
    }

    if (newFaceFinder()) {
        std::vector<TopoDS_Edge> closedEdges;
        std::vector<TopoDS_Edge> cleanEdges = DrawProjectSplit::scrubEdges(goEdges, closedEdges);

        if (cleanEdges.empty() &&
            closedEdges.empty()) {
            //how does this happen?  something wrong somewhere
//            Base::Console().Message("DVP::extractFaces - no clean or closed wires\n");    //debug
            return;
        }

        //use EdgeWalker to make wires from edges
        EdgeWalker eWalker;
        std::vector<TopoDS_Wire> sortedWires;
        try {
            if (!cleanEdges.empty()) {
                sortedWires = eWalker.execute(cleanEdges, true); //include outer wire
            }
        }
        catch (Base::Exception &e) {
            throw Base::RuntimeError(e.what());
        }
        geometryObject->clearFaceGeom();

        std::vector<TopoDS_Wire> closedWires;
        for (auto& e: closedEdges) {
            BRepBuilderAPI_MakeWire mkWire(e);
            TopoDS_Wire w = mkWire.Wire();
            closedWires.push_back(w);
        }
        if (!closedWires.empty()) {
            sortedWires.insert(sortedWires.end(), closedWires.begin(), closedWires.end());
            //inserting the closedWires that did not go through EdgeWalker into
            //sortedWires ruins EdgeWalker's sort by size, so we have to do it again.
            sortedWires = eWalker.sortWiresBySize(sortedWires);
        }

        if (sortedWires.empty()) {
            Base::Console().Warning("DVP::extractFaces - %s - Can't make faces from projected edges\n", getNameInDocument());
        } else {
            BRepTools::Write(DrawUtil::vectorToCompound(sortedWires), "DVPSortedWires.brep");            //debug
            constexpr double minWireArea = 0.000001;  //arbitrary very small face size
            std::vector<TopoDS_Wire>::iterator itWire = sortedWires.begin();
            for (; itWire != sortedWires.end(); itWire++) {
                if (!BRep_Tool::IsClosed(*itWire)) {
                    continue;       //can not make a face from open wire
                }

                double area = ShapeAnalysis::ContourArea(*itWire);
                if (area <= minWireArea) {
                    continue;   //can not make a face from wire with no area
                }

                TechDraw::FacePtr f(std::make_shared<TechDraw::Face>());
                const TopoDS_Wire& wire = (*itWire);
                f->wires.push_back(new TechDraw::Wire(wire));
                if (geometryObject) {
                    geometryObject->addFaceGeom(f);
                }
            }
        }
    } else { //use original method
        //make a copy of the input edges so the loose tolerances of face finding are
        //not applied to the real edge geometry.  See TopoDS_Shape::TShape().
        std::vector<TopoDS_Edge> copyEdges;
        bool copyGeometry = true;
        bool copyMesh = false;
        for (const auto& e: goEdges) {
            BRepBuilderAPI_Copy copier(e->occEdge, copyGeometry, copyMesh);
            copyEdges.push_back(TopoDS::Edge(copier.Shape()));
        }
        std::vector<TopoDS_Edge> nonZero;
        for (auto& e: copyEdges) {                            //drop any zero edges (shouldn't be any by now!!!)
            if (!DrawUtil::isZeroEdge(e)) {
                nonZero.push_back(e);
            } else {
                Base::Console().Log("INFO - DVP::extractFaces for %s found ZeroEdge!\n",getNameInDocument());
            }
        }

        //HLR algo does not provide all edge intersections for edge endpoints.
        //need to split long edges touched by Vertex of another edge
        std::vector<splitPoint> splits;
        std::vector<TopoDS_Edge>::iterator itOuter = nonZero.begin();
        int iOuter = 0;
        for (; itOuter != nonZero.end(); ++itOuter, iOuter++) {    //*** itOuter != nonZero.end() - 1
            TopoDS_Vertex v1 = TopExp::FirstVertex((*itOuter));
            TopoDS_Vertex v2 = TopExp::LastVertex((*itOuter));
            Bnd_Box sOuter;
            BRepBndLib::AddOptimal(*itOuter, sOuter);
            sOuter.SetGap(0.1);
            if (sOuter.IsVoid()) {
                Base::Console().Log("DVP::Extract Faces - outer Bnd_Box is void for %s\n",getNameInDocument());
                continue;
            }
            if (DrawUtil::isZeroEdge(*itOuter)) {
                Base::Console().Log("DVP::extractFaces - outerEdge: %d is ZeroEdge\n",iOuter);   //this is not finding ZeroEdges
                continue;  //skip zero length edges. shouldn't happen ;)
            }
            int iInner = 0;
            std::vector<TopoDS_Edge>::iterator itInner = nonZero.begin();   //***sb itOuter + 1;
            for (; itInner != nonZero.end(); ++itInner,iInner++) {
                if (iInner == iOuter) {
                    continue;
                }
                if (DrawUtil::isZeroEdge((*itInner))) {
                    continue;  //skip zero length edges. shouldn't happen ;)
                }

                Bnd_Box sInner;
                BRepBndLib::AddOptimal(*itInner, sInner);
                sInner.SetGap(0.1);
                if (sInner.IsVoid()) {
                    Base::Console().Log("INFO - DVP::Extract Faces - inner Bnd_Box is void for %s\n",getNameInDocument());
                    continue;
                }
                if (sOuter.IsOut(sInner)) {      //bboxes of edges don't intersect, don't bother
                    continue;
                }

                double param = -1;
                if (DrawProjectSplit::isOnEdge((*itInner),v1,param,false)) {
                    gp_Pnt pnt1 = BRep_Tool::Pnt(v1);
                    splitPoint s1;
                    s1.i = iInner;
                    s1.v = Base::Vector3d(pnt1.X(),pnt1.Y(),pnt1.Z());
                    s1.param = param;
                    splits.push_back(s1);
                }
                if (DrawProjectSplit::isOnEdge((*itInner),v2,param,false)) {
                    gp_Pnt pnt2 = BRep_Tool::Pnt(v2);
                    splitPoint s2;
                    s2.i = iInner;
                    s2.v = Base::Vector3d(pnt2.X(),pnt2.Y(),pnt2.Z());
                    s2.param = param;
                    splits.push_back(s2);
                }
            } //inner loop
        }   //outer loop

        std::vector<splitPoint> sorted = DrawProjectSplit::sortSplits(splits,true);
        auto last = std::unique(sorted.begin(), sorted.end(), DrawProjectSplit::splitEqual);  //duplicates to back
        sorted.erase(last, sorted.end());                         //remove dupl splits
        std::vector<TopoDS_Edge> newEdges = DrawProjectSplit::splitEdges(nonZero,sorted);

        if (newEdges.empty()) {
            Base::Console().Log("DVP::extractFaces - no newEdges\n");
            return;
        }

        newEdges = DrawProjectSplit::removeDuplicateEdges(newEdges);

        geometryObject->clearFaceGeom();

        //find all the wires in the pile of faceEdges
        std::vector<TopoDS_Wire> sortedWires;
        EdgeWalker eWalker;
        sortedWires = eWalker.execute(newEdges);
        if (sortedWires.empty()) {
            Base::Console().Warning("DVP::extractFaces - %s -Can't make faces from projected edges\n", getNameInDocument());
            return;
        } else {
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
}

//continue processing after extractFaces thread completes
void DrawViewPart::onFacesFinished(void)
{
//    Base::Console().Message("DVP::onFacesFinished() - %s\n", getNameInDocument());
    waitingForFaces(false);
    QObject::disconnect(connectFaceWatcher);
    showProgressMessage(getNameInDocument(), "has finished extracting faces");

    //some centerlines depend on faces so we could not add CL geometry before now
    addCenterLinesToGeom();
    requestPaint();
}

//retrieve all the face hatches associated with this dvp
std::vector<TechDraw::DrawHatch*> DrawViewPart::getHatches() const
{
    std::vector<TechDraw::DrawHatch*> result;
    std::vector<App::DocumentObject*> children = getInList();
    for (auto& child: children) {
        if ( child->getTypeId().isDerivedFrom(DrawHatch::getClassTypeId()) &&
            !child->isRemoving() ) {
            TechDraw::DrawHatch* hatch = dynamic_cast<TechDraw::DrawHatch*>(child);
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
    for (auto& child: children) {
        if ( child->getTypeId().isDerivedFrom(DrawGeomHatch::getClassTypeId()) &&
             !child->isRemoving() ) {
            TechDraw::DrawGeomHatch* geom = dynamic_cast<TechDraw::DrawGeomHatch*>(child);
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
    std::sort(children.begin(), children.end(), std::less<App::DocumentObject*>());
    std::vector<App::DocumentObject*>::iterator newEnd = std::unique(children.begin(), children.end());
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != newEnd; ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawViewDimension::getClassTypeId())) {
            TechDraw::DrawViewDimension* dim = dynamic_cast<TechDraw::DrawViewDimension*>(*it);
            result.push_back(dim);
        }
    }
    return result;
}

std::vector<TechDraw::DrawViewBalloon*> DrawViewPart::getBalloons() const
{
    std::vector<TechDraw::DrawViewBalloon*> result;
    std::vector<App::DocumentObject*> children = getInList();
    std::sort(children.begin(), children.end(), std::less<App::DocumentObject*>());
    std::vector<App::DocumentObject*>::iterator newEnd = std::unique(children.begin(), children.end());
    for (std::vector<App::DocumentObject*>::iterator it = children.begin(); it != newEnd; ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawViewBalloon::getClassTypeId())) {
            TechDraw::DrawViewBalloon* balloon = dynamic_cast<TechDraw::DrawViewBalloon*>(*it);
            result.push_back(balloon);
        }
    }
    return result;
}

const std::vector<TechDraw::VertexPtr> DrawViewPart::getVertexGeometry() const
{
    std::vector<TechDraw::VertexPtr> result;
    if (geometryObject) {
        result = geometryObject->getVertexGeometry();
    }
    return result;
}

const std::vector<TechDraw::FacePtr> DrawViewPart::getFaceGeometry() const
{
    std::vector<TechDraw::FacePtr> result;
    if ( waitingForFaces() ||
        !geometryObject ) {
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
    const std::vector<TechDraw::BaseGeomPtr> &geoms = getEdgeGeometry();
    if (geoms.empty()) {
        Base::Console().Log("DVP::getGeomByIndex(%d) - no Edge Geometry. Probably restoring?\n", idx);
        return nullptr;
    }
    if ((unsigned)idx >= geoms.size()) {
        Base::Console().Error("DVP::getGeomByIndex(%d) - invalid index - size: %d\n", idx, geoms.size());
        return nullptr;
    }
    return geoms.at(idx);
}

//! returns existing geometry of 2D Vertex(idx)
TechDraw::VertexPtr DrawViewPart::getProjVertexByIndex(int idx) const
{
    const std::vector<TechDraw::VertexPtr> &geoms = getVertexGeometry();
    if (geoms.empty()) {
        Base::Console().Log("DVP::getProjVertexByIndex(%d) - no Vertex Geometry. Probably restoring?\n", idx);
        return nullptr;
    }
    if ((unsigned)idx >= geoms.size()) {
        Base::Console().Error("DVP::getProjVertexByIndex(%d) - invalid index\n", idx);
        return nullptr;
    }
    return geoms.at(idx);
}

TechDraw::VertexPtr DrawViewPart::getProjVertexByCosTag(std::string cosTag)
{
    TechDraw::VertexPtr result = nullptr;
    std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
    if (gVerts.empty()) {
        Base::Console().Log("INFO - getProjVertexByCosTag(%s) - no Vertex Geometry.\n");
        return result;
    }

    for (auto& gv: gVerts) {
        if (gv->cosmeticTag == cosTag) {
            result = gv;
            break;
        }
    }
    return result;
}


//! returns existing geometry of 2D Face(idx)
std::vector<TechDraw::BaseGeomPtr> DrawViewPart::getFaceEdgesByIndex(int idx) const
{
    std::vector<TechDraw::BaseGeomPtr> result;
    const std::vector<TechDraw::FacePtr>& faces = getFaceGeometry();
    if (idx < (int) faces.size()) {
        TechDraw::FacePtr projFace = faces.at(idx);
        for (auto& w: projFace->wires) {
            for (auto& g:w->geoms) {
                if (g->cosmetic) {
                    //if g is cosmetic, we should skip it
                    Base::Console().Log("DVP::getFaceEdgesByIndex - found cosmetic edge\n");
                } else {
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
    for (auto& w:ourFace->wires) {
        edges.clear();
        int i = 0;
        for (auto& g:w->geoms) {
            edges.push_back(g->occEdge);
            i++;
        }
        TopoDS_Wire occwire = EdgeWalker::makeCleanWire(edges);
        result.push_back(occwire);
    }

    return result;
}

Base::BoundBox3d DrawViewPart::getBoundingBox() const
{
    return bbox;
}

double DrawViewPart::getBoxX() const
{
    Base::BoundBox3d bbx = getBoundingBox();   //bbox is already scaled & centered!
    return (bbx.MaxX - bbx.MinX);
}

double DrawViewPart::getBoxY() const
{
    Base::BoundBox3d bbx = getBoundingBox();
    return (bbx.MaxY - bbx.MinY);
}

QRectF DrawViewPart::getRect() const
{
//    Base::Console().Message("DVP::getRect() - %s\n", getNameInDocument());
    double x = getBoxX();
    double y = getBoxY();
    QRectF result(0.0, 0.0, x, y);
    return result;
}

//returns a compound of all the visible projected edges
TopoDS_Shape DrawViewPart::getShape() const
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
    return result;
}

//returns the (unscaled) size of the visible lines along the alignment vector
double DrawViewPart::getSizeAlongVector(Base::Vector3d alignmentVector)
{
    gp_Ax3 projectedCS3(getProjectionCS());
    projectedCS3.SetXDirection(DrawUtil::togp_Dir(alignmentVector));
    gp_Ax3 stdCS;   //OXYZ

    gp_Trsf xPieceAlign;
    xPieceAlign.SetTransformation(stdCS, projectedCS3);
    BRepBuilderAPI_Transform mkTransAlign(getShape(), xPieceAlign);
    TopoDS_Shape shapeAligned = mkTransAlign.Shape();

    Bnd_Box shapeBox;
    shapeBox.SetGap(0.0);
    BRepBndLib::AddOptimal(shapeAligned, shapeBox);
    double xMin = 0, xMax = 0, yMin = 0, yMax = 0, zMin = 0, zMax = 0;
    shapeBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    double shapeWidth((xMax - xMin) / getScale());
    return shapeWidth;
}

//used to project a pt (ex SectionOrigin) onto paper plane
Base::Vector3d DrawViewPart::projectPoint(const Base::Vector3d& pt, bool invert) const
{
//    Base::Console().Message("DVP::projectPoint(%s, %d\n",
//                            DrawUtil::formatVector(pt).c_str(), invert);
    Base::Vector3d stdOrg(0.0, 0.0, 0.0);
    gp_Ax2 viewAxis = getProjectionCS(stdOrg);
    gp_Pnt gPt(pt.x, pt.y, pt.z);

    HLRAlgo_Projector projector( viewAxis );
    gp_Pnt2d prjPnt;
    projector.Project(gPt, prjPnt);
    Base::Vector3d result(prjPnt.X(), prjPnt.Y(), 0.0);
    if (invert) {
        result = DrawUtil::invertY(result);
    }
    return result;
}

//project a loose edge onto the paper plane
//TODO:: loose edges not supported yet
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

//simple projection of inWire with conversion of the result to TD geometry
BaseGeomPtrVector DrawViewPart::projectWire(const TopoDS_Wire& inWire) const
{
//    Base::Console().Message("DVP::projectWire() - inWire.IsNull: %d\n", inWire.IsNull());
    BaseGeomPtrVector result;
    Base::Vector3d stdOrg(0.0, 0.0, 0.0);

    TopoDS_Face paper = BRepBuilderAPI_MakeFace(gp_Pln(getProjectionCS(stdOrg)));
    BRepAlgo_NormalProjection projector(paper);
    projector.Add(inWire);
    projector.Build();
    BRepTools::Write(projector.Projection(), "DVPprojectedWire.brep");            //debug

    TopExp_Explorer expShape(projector.Projection(), TopAbs_EDGE);
    for (; expShape.More(); expShape.Next()) {
        BaseGeomPtr edge = BaseGeom::baseFactory(TopoDS::Edge(expShape.Current()));
        result.push_back(edge);
    }
    return result;
}

bool DrawViewPart::waitingForResult() const
{
    if (waitingForHlr() ||
        waitingForFaces()) {
        return true;
    }
    return false;
}

bool DrawViewPart::hasGeometry(void) const
{
    if (!geometryObject) {
        return false;
    }

    if (waitingForHlr()) {
        return false;
    }
    const std::vector<TechDraw::VertexPtr> &verts = getVertexGeometry();
    const std::vector<TechDraw::BaseGeomPtr> &edges = getEdgeGeometry();
    if (verts.empty() &&
        edges.empty() ) {
        return false;
    } else {
        return true;
    }
    return false;
}

//convert a vector in local XY coords into a coordinate system in global
//coordinates aligned to the vector.
//Note that this CS may not have the ideal XDirection for the derived view
//(likely a DrawViewSection) and the user may need to adjust the XDirection
//in the derived view.
gp_Ax2 DrawViewPart::localVectorToCS(const Base::Vector3d localUnit) const
{
    gp_Pnt stdOrigin(0.0, 0.0, 0.0);
    gp_Ax2 dvpCS = getProjectionCS(DrawUtil::toVector3d(stdOrigin));
    gp_Vec gLocalUnit = DrawUtil::togp_Dir(localUnit);
    gp_Vec gLocalX(-gLocalUnit.Y(), gLocalUnit.X(), 0.0);    //clockwise perp for 2d

    gp_Ax3 OXYZ;
    gp_Trsf xLocalOXYZ;
    xLocalOXYZ.SetTransformation(OXYZ, gp_Ax3(dvpCS));
    gp_Vec gLocalUnitOXYZ = gLocalUnit.Transformed(xLocalOXYZ);
    gp_Vec gLocalXOXYZ = gLocalX.Transformed(xLocalOXYZ);

    return { stdOrigin, gp_Dir(gLocalUnitOXYZ), gp_Dir(gLocalXOXYZ) };
}

Base::Vector3d DrawViewPart::localVectorToDirection(const Base::Vector3d localUnit) const
{
    Base::Console().Message("DVP::localVectorToDirection() - localUnit: %s\n", DrawUtil::formatVector(localUnit).c_str());
    gp_Ax2 cs = localVectorToCS(localUnit);
    return DrawUtil::toVector3d(cs.Direction());
}

gp_Ax2 DrawViewPart::getProjectionCS(const Base::Vector3d pt) const
{
//    Base::Console().Message("DVP::getProjectionCS() - %s - %s\n", getNameInDocument(), Label.getValue());
    Base::Vector3d direction = Direction.getValue();
    gp_Dir gDir(direction.x,
                direction.y,
                direction.z);
    Base::Vector3d xDir = getXDirection();
    gp_Dir gXDir(xDir.x,
                 xDir.y,
                 xDir.z);
    gp_Pnt gOrg(pt.x,
                pt.y,
                pt.z);
    gp_Ax2 viewAxis(gOrg,
                    gDir);
    try {
        viewAxis = gp_Ax2(gOrg,
                          gDir,
                          gXDir);
    }
    catch (...) {
        Base::Console().Warning("DVP - %s - failed to create projection CS\n", getNameInDocument());
    }
    return viewAxis;
}

gp_Ax2 DrawViewPart::getViewAxis(const Base::Vector3d& pt,
                               const Base::Vector3d& direction,
                               const bool flip) const
{
    (void) direction;
    (void) flip;
    Base::Console().Message("DVP::getViewAxis - deprecated. Use getProjectionCS.\n");
    return getProjectionCS(pt);
}

//TODO: make saveShape a property

Base::Vector3d DrawViewPart::getOriginalCentroid() const
{
    return m_saveCentroid;
}

Base::Vector3d DrawViewPart::getCurrentCentroid() const
{
    TopoDS_Shape shape = getSourceShape();
    gp_Ax2 cs = getProjectionCS(Base::Vector3d(0.0, 0.0, 0.0));
    Base::Vector3d center = TechDraw::findCentroidVec(shape, cs);
    return center;
}

std::vector<DrawViewSection*> DrawViewPart::getSectionRefs() const
{
    std::vector<DrawViewSection*> result;
    std::vector<App::DocumentObject*> inObjs = getInList();
    for (auto& o:inObjs) {
        if (o->getTypeId().isDerivedFrom(DrawViewSection::getClassTypeId())) {
            result.push_back(static_cast<TechDraw::DrawViewSection*>(o));
        }
    }
    return result;
}

std::vector<DrawViewDetail*> DrawViewPart::getDetailRefs() const
{
    std::vector<DrawViewDetail*> result;
    std::vector<App::DocumentObject*> inObjs = getInList();
    for (auto& o:inObjs) {
        if (o->getTypeId().isDerivedFrom(DrawViewDetail::getClassTypeId())) {
            if (!o->isRemoving()) {
                result.push_back(static_cast<TechDraw::DrawViewDetail*>(o));
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
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    return hGrp->GetBool("HandleFaces", 1l);
}

bool DrawViewPart::newFaceFinder(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    bool result = hGrp->GetBool("NewFaceFinder", 0l);
    return result;
}

//! remove features that are useless without this DVP
//! hatches, geomhatches, dimensions, ...
void DrawViewPart::unsetupObject()
{
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

//! is this an Isometric projection?
bool DrawViewPart::isIso() const
{
    bool result = false;
    Base::Vector3d dir = Direction.getValue();
    if ( DrawUtil::fpCompare(fabs(dir.x), fabs(dir.y))  &&
         DrawUtil::fpCompare(fabs(dir.x), fabs(dir.z)) ) {
        result = true;
    }
    return result;
}

bool DrawViewPart::checkXDirection() const
{
//    Base::Console().Message("DVP::checkXDirection()\n");
    Base::Vector3d xDir = XDirection.getValue();
    if (DrawUtil::fpCompare(xDir.Length(), 0.0))  {
        Base::Vector3d dir = Direction.getValue();
        Base::Vector3d origin(0.0, 0.0, 0.0);
        Base::Vector3d xDir = getLegacyX(origin,
                                         dir);
        Base::Console().Log("DVP - %s - XDirection property not set. Trying %s\n",
                                getNameInDocument(),
                                DrawUtil::formatVector(xDir).c_str());
        return false;
    }
    return true;
}

//
Base::Vector3d DrawViewPart::getXDirection() const
{
//    Base::Console().Message("DVP::getXDirection() - %s\n", Label.getValue());
    Base::Vector3d result(1.0, 0.0, 0.0);               //default X
    App::Property* prop = getPropertyByName("XDirection");
    if (prop) {                              //have an XDirection property
        Base::Vector3d propVal = XDirection.getValue();
        if (DrawUtil::fpCompare(propVal.Length(), 0.0))  {   //but it has no value
            Base::Vector3d dir = Direction.getValue();       //make a sensible default
            Base::Vector3d org(0.0, 0.0, 0.0);
            result = getLegacyX(org,
                                dir);
        } else {
            result = propVal;                               //normal case.  XDirection is set.
        }
    } else {                                                //no Property.  can this happen?
            Base::Vector3d dir = Direction.getValue();      //make a sensible default
            Base::Vector3d org(0.0, 0.0, 0.0);
            result = getLegacyX(org,
                                dir);
    }
    return result;
}

Base::Vector3d DrawViewPart::getLegacyX(const Base::Vector3d& pt,
                                        const Base::Vector3d& axis,
                                        const bool flip)  const
{
//    Base::Console().Message("DVP::getLegacyX() - %s\n", Label.getValue());
    gp_Ax2 viewAxis = TechDraw::legacyViewAxis1(pt, axis, flip);
    gp_Dir gXDir = viewAxis.XDirection();
    Base::Vector3d result(gXDir.X(),
                          gXDir.Y(),
                          gXDir.Z());
    return result;
}


void DrawViewPart::updateReferenceVert(std::string tag, Base::Vector3d loc2d)
{
    for (auto& v: m_referenceVerts) {
        if (v->getTagAsString() == tag) {
            v->pnt = loc2d;
            break;
        }
    }
}

void DrawViewPart::addReferencesToGeom()
{
//    Base::Console().Message("DVP::addReferencesToGeom() - %s\n", getNameInDocument());
    std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
    gVerts.insert(gVerts.end(), m_referenceVerts.begin(), m_referenceVerts.end());
    getGeometryObject()->setVertexGeometry(gVerts);
}

//add a vertex that is not part of the geometry, but is used by
//ex. LandmarkDimension as a reference
std::string DrawViewPart::addReferenceVertex(Base::Vector3d v)
{
//    Base::Console().Message("DVP::addReferenceVertex(%s) - %s\n",
//                            DrawUtil::formatVector(v).c_str(), getNameInDocument());
    std::string refTag;
//    Base::Vector3d scaledV = v * getScale();
//    TechDraw::Vertex* ref = new TechDraw::Vertex(scaledV);
    Base::Vector3d scaledV = v;
    TechDraw::VertexPtr ref(std::make_shared<TechDraw::Vertex>(scaledV));
    ref->reference = true;
    refTag = ref->getTagAsString();
    m_referenceVerts.push_back(ref);
    return refTag;
}

void DrawViewPart::removeReferenceVertex(std::string tag)
{
    std::vector<TechDraw::VertexPtr> newRefVerts;
    for (auto& v: m_referenceVerts) {
        if (v->getTagAsString() != tag) {
            newRefVerts.push_back(v);
        } else {
//            delete v;  //??? who deletes v?
        }
    }
    m_referenceVerts = newRefVerts;
    resetReferenceVerts();
}

void DrawViewPart::removeAllReferencesFromGeom()
{
//    Base::Console().Message("DVP::removeAllReferencesFromGeom()\n");
    if (!m_referenceVerts.empty()) {
        std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
        std::vector<TechDraw::VertexPtr> newVerts;
        for (auto& gv: gVerts) {
            if (!gv->reference) {
                newVerts.push_back(gv);
            }
        }
        getGeometryObject()->setVertexGeometry(newVerts);
    }
}

void DrawViewPart::resetReferenceVerts()
{
//    Base::Console().Message("DVP::resetReferenceVerts() %s\n", getNameInDocument());
    removeAllReferencesFromGeom();
    addReferencesToGeom();
}

//********
//* Cosmetics
//********

void DrawViewPart::clearCosmeticVertexes()
{
    std::vector<CosmeticVertex*> noVerts;
    CosmeticVertexes.setValues(noVerts);
}

//add the cosmetic verts to geometry vertex list
void DrawViewPart::addCosmeticVertexesToGeom()
{
//    Base::Console().Message("DVP::addCosmeticVertexesToGeom()\n");
    const std::vector<TechDraw::CosmeticVertex*> cVerts = CosmeticVertexes.getValues();
    for (auto& cv: cVerts) {
        int iGV = geometryObject->addCosmeticVertex(cv->scaled(getScale()),
                                                    cv->getTagAsString());
        cv->linkGeom = iGV;
    }
}

int DrawViewPart::add1CVToGV(std::string tag)
{
//    Base::Console().Message("DVP::add1CVToGV(%s)\n", tag.c_str());
    TechDraw::CosmeticVertex* cv = getCosmeticVertex(tag);
    if (!cv) {
        Base::Console().Message("DVP::add1CVToGV - cv %s not found\n", tag.c_str());
        return 0;
    }
    int iGV = geometryObject->addCosmeticVertex(cv->scaled(getScale()),
                                                cv->getTagAsString());
    cv->linkGeom = iGV;
    return iGV;
}

//update Vertex geometry with current CV's
void DrawViewPart::refreshCVGeoms()
{
//    Base::Console().Message("DVP::refreshCVGeoms()\n");

    std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
    std::vector<TechDraw::VertexPtr> newGVerts;
    for (auto& gv :gVerts) {
        if (gv->cosmeticTag.empty()) {       //keep only non-cv vertices
            newGVerts.push_back(gv);
        }
    }
    getGeometryObject()->setVertexGeometry(newGVerts);
    addCosmeticVertexesToGeom();
}

//what is the CV's position in the big geometry q
int DrawViewPart::getCVIndex(std::string tag)
{
//    Base::Console().Message("DVP::getCVIndex(%s)\n", tag.c_str());
    int result = -1;
    std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
    std::vector<TechDraw::CosmeticVertex*> cVerts = CosmeticVertexes.getValues();

    int i = 0;
    bool found = false;
    for (auto& gv :gVerts) {
        if (gv->cosmeticTag == tag) {
            result = i;
            found = true;
            break;
        }
        i++;
    }
    if (!found) {       //not in vertexGeoms
        int base = gVerts.size();
        int i = 0;
        for (auto& cv: cVerts) {
//        Base::Console().Message("DVP::getCVIndex - cv tag: %s\n",
//                                cv->getTagAsString().c_str());
            if (cv->getTagAsString() == tag) {
                result = base + i;
                break;
            }
            i++;
        }
    }
//    Base::Console().Message("DVP::getCVIndex - returns: %d\n", result);
    return result;
}


//CosmeticEdges -------------------------------------------------------------------

//for completeness.  not actually used anywhere?
void DrawViewPart::clearCosmeticEdges()
{
    std::vector<CosmeticEdge*> noEdges;
    CosmeticEdges.setValues(noEdges);
}

//add the cosmetic edges to geometry edge list
void DrawViewPart::addCosmeticEdgesToGeom()
{
//    Base::Console().Message("CEx::addCosmeticEdgesToGeom()\n");
    const std::vector<TechDraw::CosmeticEdge*> cEdges = CosmeticEdges.getValues();
    for (auto& ce: cEdges) {
        TechDraw::BaseGeomPtr scaledGeom = ce->scaledGeometry(getScale());
        if (!scaledGeom)
            continue;
//        int iGE =
        geometryObject->addCosmeticEdge(scaledGeom,
                                        ce->getTagAsString());
    }
}

int DrawViewPart::add1CEToGE(std::string tag)
{
//    Base::Console().Message("CEx::add1CEToGE(%s) 2\n", tag.c_str());
    TechDraw::CosmeticEdge* ce = getCosmeticEdge(tag);
    if (!ce) {
        Base::Console().Message("CEx::add1CEToGE 2 - ce %s not found\n", tag.c_str());
        return -1;
    }
    TechDraw::BaseGeomPtr scaledGeom = ce->scaledGeometry(getScale());
    int iGE = geometryObject->addCosmeticEdge(scaledGeom,
                                              tag);

    return iGE;
}

//update Edge geometry with current CE's
void DrawViewPart::refreshCEGeoms()
{
//    Base::Console().Message("DVP::refreshCEGeoms()\n");
    std::vector<TechDraw::BaseGeomPtr> gEdges = getEdgeGeometry();
    std::vector<TechDraw::BaseGeomPtr> oldGEdges;
    for (auto& ge :gEdges) {
        if (ge->source() != SourceType::COSEDGE)  {
            oldGEdges.push_back(ge);
        }
    }
    getGeometryObject()->setEdgeGeometry(oldGEdges);
    addCosmeticEdgesToGeom();
}


// CenterLines -----------------------------------------------------------------
void DrawViewPart::clearCenterLines()
{
    std::vector<CenterLine*> noLines;
    CenterLines.setValues(noLines);
}

int DrawViewPart::add1CLToGE(std::string tag)
{
//    Base::Console().Message("CEx::add1CLToGE(%s) 2\n", tag.c_str());
    TechDraw::CenterLine* cl = getCenterLine(tag);
    if (!cl) {
        Base::Console().Message("CEx::add1CLToGE 2 - cl %s not found\n", tag.c_str());
        return -1;
    }
    TechDraw::BaseGeomPtr scaledGeom = cl->scaledGeometry(this);
    int iGE = geometryObject->addCenterLine(scaledGeom,
                                            tag);

    return iGE;
}

//update Edge geometry with current CL's
void DrawViewPart::refreshCLGeoms()
{
//    Base::Console().Message("DVP::refreshCLGeoms()\n");
    std::vector<TechDraw::BaseGeomPtr> gEdges = getEdgeGeometry();
    std::vector<TechDraw::BaseGeomPtr> newGEdges;
    for (auto& ge :gEdges) {
        if (ge->source() != SourceType::CENTERLINE)  {
            newGEdges.push_back(ge);
        }
    }
    getGeometryObject()->setEdgeGeometry(newGEdges);
    addCenterLinesToGeom();
}

//add the center lines to geometry Edges list
void DrawViewPart::addCenterLinesToGeom()
{
//   Base::Console().Message("DVP::addCenterLinesToGeom()\n");
    const std::vector<TechDraw::CenterLine*> lines = CenterLines.getValues();
    for (auto& cl: lines) {
        TechDraw::BaseGeomPtr scaledGeom = cl->scaledGeometry(this);
        if (!scaledGeom) {
            Base::Console().Error("DVP::addCenterLinesToGeom - scaledGeometry is null\n");
            continue;
        }
//        int idx =
        (void) geometryObject->addCenterLine(scaledGeom, cl->getTagAsString());
    }
}

// GeomFormats -----------------------------------------------------------------

void DrawViewPart::clearGeomFormats()
{
    std::vector<GeomFormat*> noFormats;
    std::vector<GeomFormat*> fmts = GeomFormats.getValues();
    GeomFormats.setValues(noFormats);
    for (auto& f: fmts) {
        delete f;
    }
}

//------------------------------------------------------------------------------

void DrawViewPart::dumpVerts(std::string text)
{
    if (!geometryObject) {
        Base::Console().Message("no verts to dump yet\n");
        return;
    }
    std::vector<TechDraw::VertexPtr> gVerts = getVertexGeometry();
    Base::Console().Message("%s - dumping %d vertGeoms\n",
                            text.c_str(), gVerts.size());
    for (auto& gv: gVerts) {
        gv->dump();
    }
}

void DrawViewPart::dumpCosVerts(std::string text)
{
    std::vector<TechDraw::CosmeticVertex*> cVerts = CosmeticVertexes.getValues();
    Base::Console().Message("%s - dumping %d CosmeticVertexes\n",
                            text.c_str(), cVerts.size());
    for (auto& cv: cVerts) {
        cv->dump("a CV");
    }
}

void DrawViewPart::dumpCosEdges(std::string text)
{
    std::vector<TechDraw::CosmeticEdge*> cEdges = CosmeticEdges.getValues();
    Base::Console().Message("%s - dumping %d CosmeticEdge\n",
                            text.c_str(), cEdges.size());
    for (auto& ce: cEdges) {
        ce->dump("a CE");
    }
}

PyObject *DrawViewPart::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewPartPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

void DrawViewPart::handleChangedPropertyName(Base::XMLReader &reader, const char* TypeName, const char* PropName)
{
//    extHandleChangedPropertyName(reader, TypeName, PropName); // CosmeticExtension
    DrawView::handleChangedPropertyName(reader, TypeName, PropName);
}

bool DrawViewPart::prefHardViz()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("HardViz", true);
    return result;
}

bool DrawViewPart::prefSeamViz()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("SeamViz", true);
    return result;
}

bool DrawViewPart::prefSmoothViz()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("SmoothViz", true);
    return result;
}

bool DrawViewPart::prefIsoViz()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("IsoViz", false);
    return result;
}

bool DrawViewPart::prefHardHid()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("HardHid",  false);
    return result;
}

bool DrawViewPart::prefSeamHid()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("SeamHid", false);
    return result;
}

bool DrawViewPart::prefSmoothHid()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("SmoothHid", false);
    return result;
}

bool DrawViewPart::prefIsoHid()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    bool result = hGrp->GetBool("IsoHid", false);
    return result;
}

int DrawViewPart::prefIsoCount()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/HLR");
    int result = hGrp->GetBool("IsoCount", 0);
    return result;
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewPartPython, TechDraw::DrawViewPart)
template<> const char* TechDraw::DrawViewPartPython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderViewPart";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewPart>;
}
