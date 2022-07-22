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
# include <sstream>

#include <Bnd_Box.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRep_Builder.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrim_Cylinder.hxx>
#include <BRepProj_Projection.hxx>
#include <BRepTools.hxx>
#include <Geom_Plane.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>

#endif

#include <chrono>

#include <QFile>
#include <QFileInfo>
#include "QtConcurrent/qtconcurrentrun.h"

#include <App/Application.h>
#include <App/Document.h>
#include <App/Material.h>
#include <Base/BoundBox.h>
#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

#include "Preferences.h"
#include "Geometry.h"
#include "GeometryObject.h"
#include "Cosmetic.h"
#include "EdgeWalker.h"
#include "DrawProjectSplit.h"
#include "DrawProjGroupItem.h"
#include "DrawPage.h"
#include "DrawUtil.h"
#include "DrawViewDetail.h"
#include "DrawViewSection.h"

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewDetail
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewDetail, TechDraw::DrawViewPart)

DrawViewDetail::DrawViewDetail() :
    m_waitingForDetail(false)
{
    static const char *dgroup = "Detail";

    ADD_PROPERTY_TYPE(BaseView ,(nullptr),dgroup,App::Prop_None,"2D View source for this Section");
    BaseView.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(AnchorPoint ,(0,0,0) ,dgroup,App::Prop_None,"Location of detail in BaseView");
    ADD_PROPERTY_TYPE(Radius,(10.0),dgroup, App::Prop_None, "Size of detail area");
    ADD_PROPERTY_TYPE(Reference ,("1"),dgroup,App::Prop_None,"An identifier for this detail");

    getParameters();
    m_fudge = 1.01;

    //hide Properties not relevant to DVDetail
    Direction.setStatus(App::Property::ReadOnly,true);   //Should be same as BaseView
    Rotation.setStatus(App::Property::ReadOnly,true);    //same as BaseView
    ScaleType.setValue("Custom");                        //dvd uses scale from BaseView
}

DrawViewDetail::~DrawViewDetail()
{
}

short DrawViewDetail::mustExecute() const
{
    if (!isRestoring()) {
        if (
            AnchorPoint.isTouched() ||
            Radius.isTouched() ||
            BaseView.isTouched() ||
            Reference.isTouched()
        ) {
            return true;
        }
    }
    return TechDraw::DrawView::mustExecute();
}

void DrawViewDetail::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &Reference) {
            std::string lblText = "Detail " +
                                  std::string(Reference.getValue());
            Label.setValue(lblText);
        }
        if ((prop == &Reference) ||
            (prop == &Radius) ||
            (prop == &BaseView)) {
            requestPaint();
        }
        if (prop == &AnchorPoint)  {
            // to see AnchorPoint changes repainting is not enough, we must recompute
            recomputeFeature(true);
        }
        if (prop == &ScaleType) {
            auto page = findParentPage();
            // if ScaleType is "Page", the user cannot change it
            if (ScaleType.isValue("Page")) {
                Scale.setStatus(App::Property::ReadOnly, true);
                // apply the page-wide Scale
                if (page) {
                    if (std::abs(page->Scale.getValue() - getScale()) > FLT_EPSILON) {
                        Scale.setValue(page->Scale.getValue());
                        Scale.purgeTouched();
                    }
                }
            }
            else if (ScaleType.isValue("Custom")) {
                // allow the change Scale
                Scale.setStatus(App::Property::ReadOnly, false);
            }
            else if (ScaleType.isValue("Automatic")) {
                Scale.setStatus(App::Property::ReadOnly, true);
                // apply a Scale
                if (!checkFit(page)) {
                    double newScale = autoScale(page->getPageWidth(), page->getPageHeight());
                    if (std::abs(newScale - getScale()) > FLT_EPSILON) {           //stops onChanged/execute loop
                        Scale.setValue(newScale);
                        Scale.purgeTouched();
                    }
                }
            }
        }
    }
    DrawView::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawViewDetail::execute()
{
//    Base::Console().Message("DVD::execute() - %s\n", getNameInDocument());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    App::DocumentObject* baseObj = BaseView.getValue();
    if (!baseObj)  {
        Base::Console().Log("DVD::execute - No BaseView(s) linked. - %s\n",
                                  getNameInDocument());
        return DrawView::execute();
    }

    if (!baseObj->getTypeId().isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        Base::Console().Log("DVD::execute - %s - BaseView object is not a DrawViewPart object\n",
                             getNameInDocument());
        return DrawView::execute();
    }
    
    DrawViewPart* dvp = static_cast<DrawViewPart*>(baseObj);

    DrawProjGroupItem* dpgi = nullptr;
    if (dvp->isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())) {
        dpgi= static_cast<TechDraw::DrawProjGroupItem*>(dvp);
    }

    DrawViewSection* dvs = nullptr;
    if (dvp->isDerivedFrom(TechDraw::DrawViewSection::getClassTypeId())) {
        dvs= static_cast<TechDraw::DrawViewSection*>(dvp);
    }

    TopoDS_Shape shape;
    if (dvs) {
        shape = dvs->getCutShape();
    }
    else if (dpgi) {
        shape = dpgi->getSourceShapeFused();
    }
    else {
        shape = dvp->getSourceShapeFused();
    }

    if (shape.IsNull()) {
        Base::Console().Log("DVD::execute - %s - Source shape is Null\n",
                              getNameInDocument());
        return DrawView::execute();
    }

    bool haveX = checkXDirection();
    if (!haveX) {
        //block touch/onChanged stuff
        Base::Vector3d newX = getXDirection();
        XDirection.setValue(newX);
        XDirection.purgeTouched();  //don't trigger updates!
        //unblock
    }

    detailExec(shape, dvp, dvs);
    addShapes2d();

    //second pass if required
    if (ScaleType.isValue("Automatic") && !checkFit()) {
        double newScale = autoScale();
        Scale.setValue(newScale);
        Scale.purgeTouched();
        if (geometryObject) {
            delete geometryObject;
            geometryObject = nullptr;
            detailExec(shape, dvp, dvs);
        }
    }
    dvp->requestPaint();  //to refresh detail highlight!
    return DrawView::execute();
}

//try to create a detail of the solids & shells in shape
//if there are no solids/shells in shape, use the edges in shape
void DrawViewDetail::detailExec(TopoDS_Shape& shape,
                                DrawViewPart* dvp,
                                DrawViewSection* dvs)
{
    if (waitingForResult()) {
//        Base::Console().Message("DVD::detailExec - waiting for result\n");
        return;
    }
    QObject::connect(&m_detailWatcher, SIGNAL(finished()), this, SLOT(onMakeDetailFinished()));
    m_detailFuture = QtConcurrent::run(this, &DrawViewDetail::makeDetailShape, shape, dvp, dvs);
    m_detailWatcher.setFuture(m_detailFuture);
}

//this runs in a separate thread since it can sometimes take a long time
void DrawViewDetail::makeDetailShape(TopoDS_Shape& shape,
                                     DrawViewPart* dvp,
                                     DrawViewSection* dvs)
{
    if (waitingForDetail()) {
//        Base::Console().Message("DVD::makeDetailShape - already in progress. returning\n");
        return;
    }
    waitingForDetail(true);
    showProgressMessage(getNameInDocument(), "is making detail shape");

//    auto start = chrono::high_resolution_clock::now();

    Base::Vector3d anchor = AnchorPoint.getValue();    //this is a 2D point (in unrotated coords)
    Base::Vector3d dirDetail = dvp->Direction.getValue();

    double radius = getFudgeRadius();

    int solidCount = DrawUtil::countSubShapes(shape, TopAbs_SOLID);
    int shellCount = DrawUtil::countSubShapes(shape, TopAbs_SHELL);

    BRepBuilderAPI_Copy BuilderCopy(shape);
    TopoDS_Shape myShape = BuilderCopy.Shape();

    gp_Pnt gpCenter = TechDraw::findCentroid(myShape,
                                             dirDetail);
    Base::Vector3d shapeCenter = Base::Vector3d(gpCenter.X(),gpCenter.Y(),gpCenter.Z());
    m_saveCentroid = shapeCenter;              //centroid of original shape

    if (dvs) {
        //section cutShape should already be on origin
    } else {
        myShape = TechDraw::moveShape(myShape,                     //centre shape on origin
                                       -shapeCenter);
    }

    shapeCenter = Base::Vector3d(0.0, 0.0, 0.0);


    m_viewAxis = dvp->getProjectionCS(shapeCenter);
    anchor = Base::Vector3d(anchor.x,anchor.y, 0.0);   //anchor coord in projection CS
    Base::Vector3d anchorOffset3d = DrawUtil::toR3(m_viewAxis, anchor);     //actual anchor coords in R3

    Bnd_Box bbxSource;
    bbxSource.SetGap(0.0);
    BRepBndLib::AddOptimal(myShape, bbxSource);
    double diag = sqrt(bbxSource.SquareExtent());

    Base::Vector3d toolPlaneOrigin = anchorOffset3d + dirDetail * diag * -1.0;    //center tool about anchor
    double extrudeLength = 2.0 * toolPlaneOrigin.Length();

    gp_Pnt gpnt(toolPlaneOrigin.x,toolPlaneOrigin.y,toolPlaneOrigin.z);
    gp_Dir gdir(dirDetail.x,dirDetail.y,dirDetail.z);

    double hideToolRadius = radius * 1.0;
    TopoDS_Face aProjFace;
    Base::Vector3d extrudeVec = dirDetail * extrudeLength;
    gp_Vec extrudeDir(extrudeVec.x,extrudeVec.y,extrudeVec.z);
    TopoDS_Shape tool;
    if (Preferences::mattingStyle()) {
        //square mat
        gp_Pln gpln(gpnt,gdir);
        BRepBuilderAPI_MakeFace mkFace(gpln, -hideToolRadius,hideToolRadius,-hideToolRadius,hideToolRadius);
        aProjFace = mkFace.Face();
        if(aProjFace.IsNull()) {
            Base::Console().Warning("DVD::detailExec - %s - failed to create tool base face\n", getNameInDocument());
            return;
        }
        tool = BRepPrimAPI_MakePrism(aProjFace, extrudeDir, false, true).Shape();
        if(tool.IsNull()) {
            Base::Console().Warning("DVD::detailExec - %s - failed to create tool (prism)\n", getNameInDocument());
            return;
        }
    } else {
        //circular mat
        gp_Ax2 cs(gpnt, gdir);
        BRepPrimAPI_MakeCylinder mkTool(cs, hideToolRadius, extrudeLength);
        tool = mkTool.Shape();
        if(tool.IsNull()) {
            Base::Console().Warning("DVD::detailExec - %s - failed to create tool (cylinder)\n", getNameInDocument());
            return;
        }
    }

    BRep_Builder builder;
    TopoDS_Compound pieces;
    builder.MakeCompound(pieces);
    if (solidCount > 0)  {
        TopExp_Explorer expl(myShape, TopAbs_SOLID);
        for (; expl.More(); expl.Next()) {
            const TopoDS_Solid& s = TopoDS::Solid(expl.Current());

            BRepAlgoAPI_Common mkCommon(s,tool);
            if (!mkCommon.IsDone()) {
    //            Base::Console().Warning("DVD::execute - %s - detail cut operation failed (1)\n", getNameInDocument());
                continue;
            }
            if (mkCommon.Shape().IsNull()) {
    //            Base::Console().Warning("DVD::execute - %s - detail cut operation failed (2)\n", getNameInDocument());
                continue;
            }
            //this might be overkill for piecewise algo
            //Did we get at least 1 solid?
            TopExp_Explorer xp;
            xp.Init(mkCommon.Shape(),TopAbs_SOLID);
            if (!(xp.More() == Standard_True)) {
    //            Base::Console().Warning("DVD::execute - mkCommon.Shape is not a solid!\n");
                continue;
            }
            builder.Add(pieces, mkCommon.Shape());
        }
    }

    if (shellCount > 0) {
        TopExp_Explorer expl(myShape, TopAbs_SHELL);
        for (; expl.More(); expl.Next()) {
            const TopoDS_Shell& s = TopoDS::Shell(expl.Current());

            BRepAlgoAPI_Common mkCommon(s,tool);
            if (!mkCommon.IsDone()) {
    //            Base::Console().Warning("DVD::execute - %s - detail cut operation failed (1)\n", getNameInDocument());
                continue;
            }
            if (mkCommon.Shape().IsNull()) {
    //            Base::Console().Warning("DVD::execute - %s - detail cut operation failed (2)\n", getNameInDocument());
                continue;
            }
            //this might be overkill for piecewise algo
            //Did we get at least 1 shell?
            TopExp_Explorer xp;
            xp.Init(mkCommon.Shape(),TopAbs_SHELL);
            if (!(xp.More() == Standard_True)) {
    //            Base::Console().Warning("DVD::execute - mkCommon.Shape is not a shell!\n");
                continue;
            }
            builder.Add(pieces, mkCommon.Shape());
        }
    }

    if (debugDetail()) {
        BRepTools::Write(tool, "DVDTool.brep");            //debug
        BRepTools::Write(myShape, "DVDCopy.brep");       //debug
        BRepTools::Write(pieces, "DVDCommon.brep");        //debug
    }

//for debugging show compound instead of common
//    BRep_Builder builder;
//    TopoDS_Compound Comp;
//    builder.MakeCompound(Comp);
//    builder.Add(Comp, tool);
//    builder.Add(Comp, myShape);

    gp_Pnt inputCenter;
    try {
        //centroid of result
        inputCenter = TechDraw::findCentroid(pieces,
                                             dirDetail);
        Base::Vector3d centroid(inputCenter.X(),
                                inputCenter.Y(),
                                inputCenter.Z());
        m_saveCentroid += centroid;              //center of massaged shape

        if ((solidCount > 0) ||
            (shellCount > 0)) {
            //align shape with detail anchor
            TopoDS_Shape centeredShape = TechDraw::moveShape(pieces,
                                                             anchorOffset3d * -1.0);
            m_scaledShape = TechDraw::scaleShape(centeredShape,
                                                 getScale());
            if (debugDetail()) {
                BRepTools::Write(m_scaledShape, "DVDScaled.brep");            //debug
            }
        } else {
            //no solids, no shells, do what you can with edges
            TopoDS_Shape projectedEdges = projectEdgesOntoFace(myShape, aProjFace, gdir);
            TopoDS_Shape centeredShape = TechDraw::moveShape(projectedEdges,
                                                             anchorOffset3d * -1.0);
            if (debugDetail()) {
                BRepTools::Write(projectedEdges, "DVDProjectedEdges.brep");            //debug
                BRepTools::Write(centeredShape, "DVDCenteredShape.brep");            //debug
            }
            m_scaledShape = TechDraw::scaleShape(centeredShape,
                                                 getScale());
        }

        Base::Vector3d stdOrg(0.0,0.0,0.0);
        m_viewAxis = dvp->getProjectionCS(stdOrg);

        if (!DrawUtil::fpCompare(Rotation.getValue(),0.0)) {
            m_scaledShape = TechDraw::rotateShape(m_scaledShape,
                                                  m_viewAxis,
                                                  Rotation.getValue());
        }
    }  //end try block

    catch (Standard_Failure& e1) {
        Base::Console().Message("DVD::makeDetailShape - failed to create detail %s - %s **\n",getNameInDocument(),e1.GetMessageString());
        return;
    }

//    auto end = chrono::high_resolution_clock::now();
//    auto diff = end - start;
//    double diffOut = chrono::duration <double, milli>(diff).count();
//    Base::Console().Message("DVD::makeDetailShape - %s spent: %.3f millisecs making detail shape\n", getNameInDocument(), diffOut);
    showProgressMessage(getNameInDocument(), "has finished making detail shape");
}

void DrawViewDetail::postHlrTasks(void)
{
//    Base::Console().Message("DVD::postHlrTasks()\n");
    geometryObject->pruneVertexGeom(Base::Vector3d(0.0,0.0,0.0),
                                    Radius.getValue() * getScale());      //remove vertices beyond clipradius
    DrawViewPart::postHlrTasks();
}

//continue processing after makeDetailShape thread is finished
void DrawViewDetail::onMakeDetailFinished(void)
{
    waitingForDetail(false);
    QObject::disconnect(&m_detailWatcher, SIGNAL(finished()), this, SLOT(onMakeDetailFinished()));

    //ancestor's buildGeometryObject will run HLR and face finding in a separate thread
    geometryObject =  buildGeometryObject(m_scaledShape, m_viewAxis);

}
TopoDS_Shape DrawViewDetail::projectEdgesOntoFace(TopoDS_Shape &edgeShape,
                                                  TopoDS_Face &projFace,
                                                  gp_Dir& projDir)
{
    BRep_Builder builder;
    TopoDS_Compound edges;
    builder.MakeCompound(edges);
    TopExp_Explorer Ex(edgeShape, TopAbs_EDGE);
    while (Ex.More())
    {
        TopoDS_Edge e = TopoDS::Edge(Ex.Current());
        BRepProj_Projection mkProj(e, projFace, projDir);
        if (mkProj.IsDone()) {
            builder.Add(edges, mkProj.Shape());
        }
        Ex.Next();
    }
    if (debugDetail()) {
        BRepTools::Write(edges, "DVDEdges.brep");            //debug
    }

    return TopoDS_Shape(std::move(edges));
}

//we don't want to paint detail highlights on top of detail views,
//so tell the Gui that there are no details for this view
std::vector<DrawViewDetail*> DrawViewDetail::getDetailRefs() const
{
    return std::vector<DrawViewDetail*>();
}

double DrawViewDetail::getFudgeRadius()
{
    return Radius.getValue() * m_fudge;
}

bool DrawViewDetail::debugDetail() const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/debug");

    return hGrp->GetBool("debugDetail",false);
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

void DrawViewDetail::getParameters()
{
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewDetailPython, TechDraw::DrawViewDetail)
template<> const char* TechDraw::DrawViewDetailPython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderViewPart";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewDetail>;
}
