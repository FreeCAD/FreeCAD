/***************************************************************************
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
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepProj_Projection.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <QtConcurrentRun>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Solid.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <sstream>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "DrawComplexSection.h"
#include "DrawUtil.h"
#include "DrawViewDetail.h"
#include "DrawViewSection.h"
#include "GeometryObject.h"
#include "Preferences.h"
#include "ShapeUtils.h"


using namespace TechDraw;

//===========================================================================
// DrawViewDetail
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewDetail, TechDraw::DrawViewPart)

DrawViewDetail::DrawViewDetail() : m_waitingForDetail(false), m_saveDvp(nullptr), m_saveDvs(nullptr)
{
    static const char* dgroup = "Detail";

    ADD_PROPERTY_TYPE(BaseView, (nullptr), dgroup, App::Prop_None,
                      "2D View source for this Section");
    BaseView.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(AnchorPoint, (0, 0, 0), dgroup, App::Prop_None,
                      "Location of detail in BaseView");
    ADD_PROPERTY_TYPE(Radius, (10.0), dgroup, App::Prop_None, "Size of detail area");
    ADD_PROPERTY_TYPE(Reference, ("1"), dgroup, App::Prop_None, "An identifier for this detail");

    getParameters();
    m_fudge = 1.01;

    //hide Properties not relevant to DVDetail
    Direction.setStatus(App::Property::ReadOnly, true);//Should be same as BaseView
    Rotation.setStatus(App::Property::ReadOnly, true); //same as BaseView
    ScaleType.setValue("Custom");                      //dvd uses scale from BaseView
}

DrawViewDetail::~DrawViewDetail()
{
    //don't delete this object while it still has dependent tasks running
    if (m_detailFuture.isRunning()) {
        Base::Console().Message("%s is waiting for detail cut to finish\n", Label.getValue());
        m_detailFuture.waitForFinished();
    }
}

short DrawViewDetail::mustExecute() const
{
    if (isRestoring()) {
        TechDraw::DrawView::mustExecute();
    }

    if (AnchorPoint.isTouched() || Radius.isTouched() || BaseView.isTouched()) {
        return 1;
    }

    return TechDraw::DrawView::mustExecute();
}

void DrawViewDetail::onChanged(const App::Property* prop)
{
    if (isRestoring()) {
        DrawView::onChanged(prop);
        return;
    }

    if (prop == &Reference) {
        std::string lblText = "Detail " + std::string(Reference.getValue());
        Label.setValue(lblText);
    }

    DrawViewPart::onChanged(prop);
}

App::DocumentObjectExecReturn* DrawViewDetail::execute()
{
    //    Base::Console().Message("DVD::execute() - %s\n", getNameInDocument());
    if (!keepUpdated()) {
        return DrawView::execute();
    }

    App::DocumentObject* baseObj = BaseView.getValue();
    if (!baseObj) {
        return DrawView::execute();
    }

    if (!baseObj->getTypeId().isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        //this can only happen via scripting?
        return DrawView::execute();
    }

    DrawViewPart* dvp = static_cast<DrawViewPart*>(baseObj);
    TopoDS_Shape shape = dvp->getShapeForDetail();
    DrawViewSection* dvs = nullptr;
    if (dvp->isDerivedFrom(TechDraw::DrawViewSection::getClassTypeId())) {
        dvs = static_cast<TechDraw::DrawViewSection*>(dvp);
    }

    if (shape.IsNull()) {
        return DrawView::execute();
    }

    bool haveX = checkXDirection();
    if (!haveX) {
        //block touch/onChanged stuff
        Base::Vector3d newX = getXDirection();
        XDirection.setValue(newX);
        XDirection.purgeTouched();//don't trigger updates!
        //unblock
    }

    detailExec(shape, dvp, dvs);
    addShapes2d();

    dvp->requestPaint();//to refresh detail highlight in base view
    return DrawView::execute();
}

//try to create a detail of the solids & shells in shape
//if there are no solids/shells in shape, use the edges in shape
void DrawViewDetail::detailExec(TopoDS_Shape& shape, DrawViewPart* dvp, DrawViewSection* dvs)
{
    if (waitingForHlr() || waitingForDetail()) {
        return;
    }

    //note that &m_detailWatcher in the third parameter is not strictly required, but using the
    //4 parameter signature instead of the 3 parameter signature prevents clazy warning:
    //https://github.com/KDE/clazy/blob/1.11/docs/checks/README-connect-3arg-lambda.md
    connectDetailWatcher =
        QObject::connect(&m_detailWatcher, &QFutureWatcherBase::finished, &m_detailWatcher,
                         [this] { this->onMakeDetailFinished(); });

    // We create a lambda closure to hold a copy of shape.
    // This is important because this variable might be local to the calling
    // function and might get destructed before the parallel processing finishes.
    // TODO: What about dvp and dvs? Do they live past makeDetailShape?
    auto lambda = [this, shape, dvp, dvs]{this->makeDetailShape(shape, dvp, dvs);};
    m_detailFuture = QtConcurrent::run(std::move(lambda));
    m_detailWatcher.setFuture(m_detailFuture);
    waitingForDetail(true);
}

//this runs in a separate thread since it can sometimes take a long time
//make a common of the input shape and a cylinder (or prism depending on
//the matting style)
void DrawViewDetail::makeDetailShape(const TopoDS_Shape& shape, DrawViewPart* dvp, DrawViewSection* dvs)
{
    showProgressMessage(getNameInDocument(), "is making detail shape");

    Base::Vector3d dirDetail = dvp->Direction.getValue();
    double radius = getFudgeRadius();

    //make a copy of the input shape so we don't inadvertently change it
    BRepBuilderAPI_Copy BuilderCopy(shape);
    TopoDS_Shape copyShape = BuilderCopy.Shape();
    m_saveShape = copyShape;
    m_saveDvp = dvp;
    m_saveDvs = dvs;

    gp_Pnt gpCenter = ShapeUtils::findCentroid(copyShape, dirDetail);
    Base::Vector3d shapeCenter = Base::Vector3d(gpCenter.X(), gpCenter.Y(), gpCenter.Z());
    m_saveCentroid = shapeCenter;//centroid of original shape

    if (!dvs) {
        //section cutShape should already be on origin
        copyShape = ShapeUtils::moveShape(copyShape,//centre shape on origin
                                        -shapeCenter);
    }

    shapeCenter = Base::Vector3d(0.0, 0.0, 0.0);


    m_viewAxis = dvp->getProjectionCS(shapeCenter);//save the CS for later
    Base::Vector3d anchor = AnchorPoint.getValue();//this is a 2D point in base view local coords

    anchor = DrawUtil::toR3(m_viewAxis, anchor);//actual anchor coords in R3

    Bnd_Box bbxSource;
    bbxSource.SetGap(0.0);
    BRepBndLib::AddOptimal(copyShape, bbxSource);
    double diag = sqrt(bbxSource.SquareExtent());

    Base::Vector3d toolPlaneOrigin = anchor + dirDetail * diag * -1.0;//center tool about anchor
    double extrudeLength = 2.0 * toolPlaneOrigin.Length();

    gp_Pnt gpnt(toolPlaneOrigin.x, toolPlaneOrigin.y, toolPlaneOrigin.z);
    gp_Dir gdir(dirDetail.x, dirDetail.y, dirDetail.z);

    TopoDS_Face extrusionFace;
    Base::Vector3d extrudeVec = dirDetail * extrudeLength;
    gp_Vec extrudeDir(extrudeVec.x, extrudeVec.y, extrudeVec.z);
    TopoDS_Shape tool;
    if (Preferences::mattingStyle()) {
        //square mat
        gp_Pln gpln(gpnt, gdir);
        BRepBuilderAPI_MakeFace mkFace(gpln, -radius, radius, -radius, radius);
        extrusionFace = mkFace.Face();
        if (extrusionFace.IsNull()) {
            Base::Console().Warning("DVD::makeDetailShape - %s - failed to create tool base face\n",
                                    getNameInDocument());
            return;
        }
        tool = BRepPrimAPI_MakePrism(extrusionFace, extrudeDir, false, true).Shape();
        if (tool.IsNull()) {
            Base::Console().Warning("DVD::makeDetailShape - %s - failed to create tool (prism)\n",
                                    getNameInDocument());
            return;
        }
    }
    else {
        //circular mat
        gp_Ax2 cs(gpnt, gdir);
        BRepPrimAPI_MakeCylinder mkTool(cs, radius, extrudeLength);
        tool = mkTool.Shape();
        if (tool.IsNull()) {
            Base::Console().Warning("DVD::detailExec - %s - failed to create tool (cylinder)\n",
                                    getNameInDocument());
            return;
        }
    }

    BRepAlgoAPI_Common mkCommon(copyShape, tool);
    if (!mkCommon.IsDone() || mkCommon.Shape().IsNull()) {
        Base::Console().Warning("DVD::detailExec - %s - failed to create detail shape\n",
                            getNameInDocument());
        return;
    }

    // save the detail shape for further processing
    m_detailShape = mkCommon.Shape();

    if (debugDetail()) {
        BRepTools::Write(tool, "DVDTool.brep");     //debug
        BRepTools::Write(copyShape, "DVDCopy.brep");//debug
        BRepTools::Write(m_detailShape, "DVDCommon.brep"); //debug
    }

    gp_Pnt inputCenter = ShapeUtils::findCentroid(m_detailShape, dirDetail);
    Base::Vector3d centroid(inputCenter.X(), inputCenter.Y(), inputCenter.Z());
    m_saveCentroid += centroid;//center of massaged shape
    //align shape with detail anchor
    TopoDS_Shape centeredShape = ShapeUtils::moveShape(m_detailShape, anchor * -1.0);
    m_scaledShape = ShapeUtils::scaleShape(centeredShape, getScale());

    if (debugDetail()) {
        BRepTools::Write(m_scaledShape, "DVDScaled.brep");//debug
    }

    Base::Vector3d stdOrg(0.0, 0.0, 0.0);
    m_viewAxis = dvp->getProjectionCS(stdOrg);

    if (!DrawUtil::fpCompare(Rotation.getValue(), 0.0)) {
        m_scaledShape = ShapeUtils::rotateShape(m_scaledShape, m_viewAxis, Rotation.getValue());
    }

    showProgressMessage(getNameInDocument(), "has finished making detail shape");
}

void DrawViewDetail::postHlrTasks(void)
{
    //    Base::Console().Message("DVD::postHlrTasks()\n");
    DrawViewPart::postHlrTasks();

    geometryObject->pruneVertexGeom(Base::Vector3d(0.0, 0.0, 0.0),
                                    Radius.getValue()
                                        * getScale());//remove vertices beyond clipradius

    //second pass if required
    if (ScaleType.isValue("Automatic") && !checkFit()) {
        double newScale = autoScale();
        Scale.setValue(newScale);
        Scale.purgeTouched();
        detailExec(m_saveShape, m_saveDvp, m_saveDvs);
    }
    overrideKeepUpdated(false);
}

//continue processing after makeDetailShape thread is finished
void DrawViewDetail::onMakeDetailFinished(void)
{
    waitingForDetail(false);
    QObject::disconnect(connectDetailWatcher);

    //ancestor's buildGeometryObject will run HLR and face finding in a separate thread
    m_tempGeometryObject = buildGeometryObject(m_scaledShape, m_viewAxis);
}

bool DrawViewDetail::waitingForResult() const
{
    if (DrawViewPart::waitingForResult() || waitingForDetail()) {
        return true;
    }
    return false;
}
TopoDS_Shape DrawViewDetail::projectEdgesOntoFace(TopoDS_Shape& edgeShape, TopoDS_Face& projFace,
                                                  gp_Dir& projDir)
{
    BRep_Builder builder;
    TopoDS_Compound edges;
    builder.MakeCompound(edges);
    TopExp_Explorer Ex(edgeShape, TopAbs_EDGE);
    while (Ex.More()) {
        TopoDS_Edge e = TopoDS::Edge(Ex.Current());
        BRepProj_Projection mkProj(e, projFace, projDir);
        if (mkProj.IsDone()) {
            builder.Add(edges, mkProj.Shape());
        }
        Ex.Next();
    }
    if (debugDetail()) {
        BRepTools::Write(edges, "DVDEdges.brep");//debug
    }

    return TopoDS_Shape(std::move(edges));
}

//we don't want to paint detail highlights on top of detail views,
//so tell the Gui that there are no details for this view
std::vector<DrawViewDetail*> DrawViewDetail::getDetailRefs() const
{
    return std::vector<DrawViewDetail*>();
}

double DrawViewDetail::getFudgeRadius() { return Radius.getValue() * m_fudge; }

bool DrawViewDetail::debugDetail() const
{
    return Preferences::getPreferenceGroup("debug")->GetBool("debugDetail", false);
}

void DrawViewDetail::unsetupObject()
{
    //    Base::Console().Message("DVD::unsetupObject()\n");
    App::DocumentObject* baseObj = BaseView.getValue();
    DrawView* base = dynamic_cast<DrawView*>(baseObj);
    if (base) {
        base->requestPaint();
    }
}

void DrawViewDetail::getParameters() {}

// Python Drawing feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewDetailPython, TechDraw::DrawViewDetail)
template<> const char* TechDraw::DrawViewDetailPython::getViewProviderName() const
{
    return "TechDrawGui::ViewProviderViewPart";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewDetail>;
}// namespace App
