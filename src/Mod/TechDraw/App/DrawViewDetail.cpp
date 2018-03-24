/***************************************************************************
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
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepAdaptor_Surface.hxx>
# include <BRep_Builder.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrim_Cylinder.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pln.hxx>
#include <gp_Dir.hxx>
#include <Geom_Plane.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Shell.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>

#endif

#include <chrono>

# include <QFile>
# include <QFileInfo>

#include <App/Application.h>
#include <App/Material.h>
#include <Base/BoundBox.h>
#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/Part/App/PartFeature.h>

#include "Geometry.h"
#include "GeometryObject.h"
#include "EdgeWalker.h"
#include "DrawProjectSplit.h"
#include "DrawUtil.h"
#include "DrawViewDetail.h"

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewDetail
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewDetail, TechDraw::DrawViewPart)

DrawViewDetail::DrawViewDetail()
// :
//    m_mattingStyle(0)
{
    static const char *dgroup = "Detail";

    ADD_PROPERTY_TYPE(BaseView ,(0),dgroup,App::Prop_None,"2D View source for this Section");
    ADD_PROPERTY_TYPE(AnchorPoint ,(0,0,0) ,dgroup,App::Prop_None,"Location of detail in BaseView");
    ADD_PROPERTY_TYPE(Radius,(10.0),dgroup, App::Prop_None, "Size of detail area");
    ADD_PROPERTY_TYPE(Reference ,("1"),dgroup,App::Prop_None,"An identifier for this detail");

    getParameters();
    m_fudge = 1.01;
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
        //Base::Console().Message("TRACE - DVD::onChanged(%s) - %s\n",prop->getName(),Label.getValue());
        if (prop == &Reference) {
            std::string lblText = "Detail " +
                                  std::string(Reference.getValue());
            Label.setValue(lblText);
        }
        if ((prop == &Reference)  ||
           (prop == &Radius)     ||
           (prop == &AnchorPoint))  {
            BaseView.getValue()->touch();    //hack.  sb "update graphics"
        }

    }
    DrawView::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawViewDetail::execute(void)
{
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    App::DocumentObject* baseObj = BaseView.getValue();
    if (!baseObj)  {
        Base::Console().Log("INFO - DVD::execute - No BaseView - creation?\n");
        return DrawView::execute();
    }

    DrawViewPart* dvp = nullptr;
    if (!baseObj->getTypeId().isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        return new App::DocumentObjectExecReturn("BaseView object is not a DrawViewPart object");
    } else {
        dvp = static_cast<DrawViewPart*>(baseObj);
    }

    TopoDS_Shape shape = dvp->getSourceShapeFused();
    if (shape.IsNull()) {
        return new App::DocumentObjectExecReturn("DVD - Linked shape object is invalid");
    }

    Base::Vector3d anchor = AnchorPoint.getValue();    //this is a 2D point
    anchor = Base::Vector3d(anchor.x,anchor.y, 0.0);
    double radius = getFudgeRadius();
    Base::Vector3d dirDetail = dvp->Direction.getValue();
    double scale = getScale();
    gp_Ax2 viewAxis = getViewAxis(Base::Vector3d(0.0,0.0,0.0), dirDetail, false);

    Bnd_Box bbxSource;
    BRepBndLib::Add(shape, bbxSource);
    bbxSource.SetGap(0.0);
    double diag = sqrt(bbxSource.SquareExtent());

    BRepBuilderAPI_Copy BuilderCopy(shape);
    TopoDS_Shape myShape = BuilderCopy.Shape();

    gp_Pnt gpCenter = TechDrawGeometry::findCentroid(myShape,
                                                     dirDetail);
    Base::Vector3d shapeCenter = Base::Vector3d(gpCenter.X(),gpCenter.Y(),gpCenter.Z());
    Base::Vector3d extentFar,extentNear;
    extentFar = shapeCenter + dirDetail * diag;
    extentNear = shapeCenter + dirDetail * diag * -1.0;

    //turn anchor(x,y,0) in projection plane(P) into displacement in 3D
    Base::Vector3d offsetCenter3D = DrawUtil::toR3(viewAxis, anchor);     //displacement in R3
    Base::Vector3d stdZ(0.0,0.0,1.0);
    if (DrawUtil::checkParallel(dirDetail,stdZ)) {
        extentNear = extentNear + offsetCenter3D;
    } else {
        extentNear = extentNear - offsetCenter3D;
    }

    gp_Dir cylDir(dirDetail.x,dirDetail.y,dirDetail.z);
    gp_Pnt cylPoint(extentNear.x,extentNear.y,extentNear.z);
    gp_Ax2 cylAxis(cylPoint,cylDir);

    BRepPrimAPI_MakeCylinder mkCyl(cylAxis, radius, (extentFar-extentNear).Length());
    TopoDS_Shell sh = mkCyl.Cylinder().Shell();
    BRepBuilderAPI_MakeSolid mkSol(sh);
    TopoDS_Solid tool = mkSol.Solid();

    BRepAlgoAPI_Common mkCommon(myShape,tool);
    if (!mkCommon.IsDone()) {
        Base::Console().Log("DVD::execute - mkCommon not done\n");
        return new App::DocumentObjectExecReturn("DVD::execute - mkCommon not done");
    }
    if (mkCommon.Shape().IsNull()) {
        Base::Console().Log("DVD::execute - mkCommon.Shape is Null\n");
        return new App::DocumentObjectExecReturn("DVD::execute - mkCommon.Shape is Null");
    }

    //Did we get a solid?
    TopExp_Explorer xp;
    xp.Init(mkCommon.Shape(),TopAbs_SOLID);
    if (!(xp.More() == Standard_True)) {
        Base::Console().Log("DVD::execute - mkCommon.Shape is not a solid!\n");
    }
    TopoDS_Shape detail = mkCommon.Shape();
    Bnd_Box testBox;
    testBox.SetGap(0.0);
    BRepBndLib::Add(detail, testBox);
    if (testBox.IsVoid()) {
        Base::Console().Message("INFO - DVD::execute - testBox is void\n");
    }

//for debugging show compound instead of cut
//    BRep_Builder builder;
//    TopoDS_Compound Comp;
//    builder.MakeCompound(Comp);
//    builder.Add(Comp, tool);
//    builder.Add(Comp, myShape);

    gp_Pnt inputCenter;
    try {
        inputCenter = TechDrawGeometry::findCentroid(tool,
                                                     Direction.getValue());
        TopoDS_Shape mirroredShape = TechDrawGeometry::mirrorShape(detail,
                                                    inputCenter,
                                                    scale);
        gp_Ax2 viewAxis = getViewAxis(Base::Vector3d(inputCenter.X(),inputCenter.Y(),inputCenter.Z()),Direction.getValue());
        if (!DrawUtil::fpCompare(Rotation.getValue(),0.0)) {
            mirroredShape = TechDrawGeometry::rotateShape(mirroredShape,
                                                          viewAxis,
                                                          Rotation.getValue());
        }
        geometryObject = buildGeometryObject(mirroredShape,viewAxis);
        geometryObject->pruneVertexGeom(Base::Vector3d(0.0,0.0,0.0),Radius.getValue() * scale);      //remove vertices beyond clipradius

#if MOD_TECHDRAW_HANDLE_FACES
    if (handleFaces()) {
        try {
            extractFaces();
        }
        catch (Standard_Failure& e4) {
            Base::Console().Log("LOG - DVD::execute - extractFaces failed for %s - %s **\n",getNameInDocument(),e4.GetMessageString());
            return new App::DocumentObjectExecReturn(e4.GetMessageString());
        }
    }

#endif //#if MOD_TECHDRAW_HANDLE_FACES
    }
    catch (Standard_Failure& e1) {
        Base::Console().Log("LOG - DVD::execute - base shape failed for %s - %s **\n",getNameInDocument(),e1.GetMessageString());
        return new App::DocumentObjectExecReturn(e1.GetMessageString());
    }

    requestPaint();
    dvp->requestPaint();

    return App::DocumentObject::StdReturn;
}

double DrawViewDetail::getFudgeRadius()
{
    return Radius.getValue() * m_fudge;
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
