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

# include <QFile>
# include <QFileInfo>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Material.h>
#include <Base/BoundBox.h>
#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

#include "Geometry.h"
#include "GeometryObject.h"
#include "Cosmetic.h"
#include "EdgeWalker.h"
#include "DrawProjectSplit.h"
#include "DrawUtil.h"
#include "DrawViewDetail.h"
#include "DrawProjGroupItem.h"
#include "DrawViewSection.h"

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewDetail
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewDetail, TechDraw::DrawViewPart)

DrawViewDetail::DrawViewDetail()
{
    static const char *dgroup = "Detail";

    ADD_PROPERTY_TYPE(BaseView ,(0),dgroup,App::Prop_None,"2D View source for this Section");
    BaseView.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(AnchorPoint ,(0,0,0) ,dgroup,App::Prop_None,"Location of detail in BaseView");
    ADD_PROPERTY_TYPE(Radius,(10.0),dgroup, App::Prop_None, "Size of detail area");
    ADD_PROPERTY_TYPE(Reference ,("1"),dgroup,App::Prop_None,"An identifier for this detail");

    getParameters();
    m_fudge = 1.01;

    //hide Properties not relevant to DVDetail
    Direction.setStatus(App::Property::ReadOnly,true);   //Should be same as BaseView
    Rotation.setStatus(App::Property::ReadOnly,true);    //same as BaseView
}

DrawViewDetail::~DrawViewDetail()
{
}

short DrawViewDetail::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  = (AnchorPoint.isTouched() ||
                   Radius.isTouched()     ||
                   BaseView.isTouched()  ||
                   Reference.isTouched());
    }
    if (result) {
        return result;
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
    }
    DrawView::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawViewDetail::execute(void)
{
//    Base::Console().Message("DVD::execute() - %s\n", Label.getValue());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    App::DocumentObject* baseObj = BaseView.getValue();
    if (!baseObj)  {
        bool isRestoring = getDocument()->testStatus(App::Document::Status::Restoring);
        if (isRestoring) {
            Base::Console().Warning("DVD::execute - No BaseView (but document is restoring) - %s\n",
                                getNameInDocument());
        } else {
            Base::Console().Error("Error: DVD::execute - No BaseView(s) linked. - %s\n",
                                  getNameInDocument());
        }
        return DrawView::execute();
    }

    DrawViewPart* dvp = nullptr;
    if (!baseObj->getTypeId().isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        return new App::DocumentObjectExecReturn("BaseView object is not a DrawViewPart object");
    } else {
        dvp = static_cast<DrawViewPart*>(baseObj);
    }

    DrawProjGroupItem* dpgi = nullptr;
    if (dvp->isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())) {
        dpgi= static_cast<TechDraw::DrawProjGroupItem*>(dvp);
    }

    DrawViewSection* dvs = nullptr;
    if (dvp->isDerivedFrom(TechDraw::DrawViewSection::getClassTypeId())) {
        dvs= static_cast<TechDraw::DrawViewSection*>(dvp);
    }

    TopoDS_Shape shape;
    if (dvs != nullptr) {
        shape = dvs->getCutShape();
    } else if (dpgi != nullptr) {
        shape = dpgi->getSourceShapeFused();
    } else {
        shape = dvp->getSourceShapeFused();
    }

    if (shape.IsNull()) {
        bool isRestoring = getDocument()->testStatus(App::Document::Status::Restoring);
        if (isRestoring) {
            Base::Console().Warning("DVD::execute - source shape is invalid - (but document is restoring) - %s\n",
                                getNameInDocument());
        } else {
            Base::Console().Error("Error: DVD::execute - Source shape is Null. - %s\n",
                                  getNameInDocument());
        }
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
    if (ScaleType.isValue("Automatic")) {
        if (!checkFit()) {
            double newScale = autoScale();
            Scale.setValue(newScale);
            Scale.purgeTouched();
            if (geometryObject != nullptr) {
                delete geometryObject;
                geometryObject = nullptr;
                detailExec(shape, dvp, dvs);
            }
        }
    }
    dvp->requestPaint();  //to refresh detail highlight!
    return DrawView::execute();
}

void DrawViewDetail::detailExec(TopoDS_Shape shape,
                                DrawViewPart* dvp,
                                DrawViewSection* dvs)
{
    Base::Vector3d anchor = AnchorPoint.getValue();    //this is a 2D point (in unrotated coords)
    Base::Vector3d dirDetail = dvp->Direction.getValue();

    double radius = getFudgeRadius();
    double scale = getScale();

    BRepBuilderAPI_Copy BuilderCopy(shape);
    TopoDS_Shape myShape = BuilderCopy.Shape();

    gp_Pnt gpCenter = TechDraw::findCentroid(myShape,
                                             dirDetail);
    Base::Vector3d shapeCenter = Base::Vector3d(gpCenter.X(),gpCenter.Y(),gpCenter.Z());
    m_saveCentroid = shapeCenter;              //centroid of original shape

    if (dvs != nullptr) {
        //section cutShape should already be on origin
    } else {
        myShape = TechDraw::moveShape(myShape,                     //centre shape on origin
                                       -shapeCenter);
    }

    shapeCenter = Base::Vector3d(0.0, 0.0, 0.0);


    gp_Ax2 viewAxis;

    viewAxis = dvp->getProjectionCS(shapeCenter);
    anchor = Base::Vector3d(anchor.x,anchor.y, 0.0);
    Base::Vector3d anchorOffset3d = DrawUtil::toR3(viewAxis, anchor);     //anchor displacement in R3

    Bnd_Box bbxSource;
    bbxSource.SetGap(0.0);
    BRepBndLib::Add(myShape, bbxSource);
    double diag = sqrt(bbxSource.SquareExtent());

    Base::Vector3d toolPlaneOrigin = anchorOffset3d + dirDetail * diag * -1.0;    //center tool about anchor
    double extrudeLength = 2.0 * toolPlaneOrigin.Length();

    gp_Pnt gpnt(toolPlaneOrigin.x,toolPlaneOrigin.y,toolPlaneOrigin.z);
    gp_Dir gdir(dirDetail.x,dirDetail.y,dirDetail.z);
    gp_Pln gpln(gpnt,gdir);
    double hideToolRadius = radius * 1.0;
    BRepBuilderAPI_MakeFace mkFace(gpln, -hideToolRadius,hideToolRadius,-hideToolRadius,hideToolRadius);
    TopoDS_Face aProjFace = mkFace.Face();
    if(aProjFace.IsNull()) {
        Base::Console().Warning("DVD::execute - %s - failed to create tool base face\n", getNameInDocument());
        return;
    }

    Base::Vector3d extrudeVec = dirDetail * extrudeLength;
    gp_Vec extrudeDir(extrudeVec.x,extrudeVec.y,extrudeVec.z);
    TopoDS_Shape tool = BRepPrimAPI_MakePrism(aProjFace, extrudeDir, false, true).Shape();


    BRep_Builder builder;
    TopoDS_Compound pieces;
    builder.MakeCompound(pieces);
    TopExp_Explorer expl(myShape, TopAbs_SOLID);
    int indb = 0;
    int outdb = 0;
    for (; expl.More(); expl.Next()) {
        indb++;
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
        outdb++;
    }

    if (debugDetail()) {
        BRepTools::Write(tool, "DVDTool.brep");            //debug
        BRepTools::Write(myShape, "DVDCopy.brep");       //debug
        BRepTools::Write(pieces, "DVDCommon.brep");        //debug
    }

    Bnd_Box testBox;
    testBox.SetGap(0.0);
    BRepBndLib::Add(pieces, testBox);
    if (testBox.IsVoid()) {
        TechDraw::GeometryObject* go = getGeometryObject();
        if (go != nullptr) {
            go->clear();
        }
        dvp->requestPaint();
        Base::Console().Warning("DVD::execute - %s - detail area contains no geometry\n", getNameInDocument());
        return;
    }

//for debugging show compound instead of common
//    BRep_Builder builder;
//    TopoDS_Compound Comp;
//    builder.MakeCompound(Comp);
//    builder.Add(Comp, tool);
//    builder.Add(Comp, myShape);

    gp_Pnt inputCenter;
    try {
        inputCenter = TechDraw::findCentroid(tool,
                                             dirDetail);
    Base::Vector3d centroid(inputCenter.X(),
                            inputCenter.Y(),
                            inputCenter.Z());
    m_saveCentroid += centroid;              //center of massaged shape

    Base::Vector3d stdOrg(0.0,0.0,0.0);
    gp_Ax2 viewAxis = dvp->getProjectionCS(stdOrg);  //sb same CS as base view. 

    //center shape on origin
//    TopoDS_Shape centeredShape = TechDraw::moveShape(detail,
    TopoDS_Shape centeredShape = TechDraw::moveShape(pieces,
                                                     centroid * -1.0);

    TopoDS_Shape scaledShape = TechDraw::scaleShape(centeredShape,
                                                    getScale());
    if (!DrawUtil::fpCompare(Rotation.getValue(),0.0)) {
        scaledShape = TechDraw::rotateShape(scaledShape,
                                            viewAxis,
                                            Rotation.getValue());
    }

    if (debugDetail()) {
        BRepTools::Write(tool, "DVDScaled.brep");            //debug
    }

    geometryObject =  buildGeometryObject(scaledShape,viewAxis);
    geometryObject->pruneVertexGeom(Base::Vector3d(0.0,0.0,0.0),
                                    Radius.getValue() * scale);      //remove vertices beyond clipradius

#if MOD_TECHDRAW_HANDLE_FACES
    if (handleFaces()) {
        try {
            extractFaces();
        }
        catch (Standard_Failure& e4) {
            Base::Console().Log("LOG - DVD::execute - extractFaces failed for %s - %s **\n",getNameInDocument(),e4.GetMessageString());
            return;
        }
    }

#endif //#if MOD_TECHDRAW_HANDLE_FACES
    }
    catch (Standard_Failure& e1) {
        Base::Console().Message("LOG - DVD::execute - failed to create detail %s - %s **\n",getNameInDocument(),e1.GetMessageString());
        return;
    }

    addCosmeticVertexesToGeom();
    addCosmeticEdgesToGeom();
    addCenterLinesToGeom();

    addReferencesToGeom();   //what if landmarks are outside detail area??

}

double DrawViewDetail::getFudgeRadius()
{
    return Radius.getValue() * m_fudge;
}

bool DrawViewDetail::debugDetail(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/debug");

    bool result = hGrp->GetBool("debugDetail",false);
    return result;
}

void DrawViewDetail::unsetupObject()
{
//    Base::Console().Message("DVD::unsetupObject()\n");
    App::DocumentObject* baseObj = BaseView.getValue();
    DrawView* base = dynamic_cast<DrawView*>(baseObj);
    if (base != nullptr) {
        base->requestPaint();
    }

}


void DrawViewDetail::getParameters()
{
// what parameters are useful?
// handleFaces
// radiusFudge?

//    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
//        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw");
//    m_mattingStyle = hGrp->GetInt("MattingStyle", 0);

}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewDetailPython, TechDraw::DrawViewDetail)
template<> const char* TechDraw::DrawViewDetailPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderViewPart";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewDetail>;
}
