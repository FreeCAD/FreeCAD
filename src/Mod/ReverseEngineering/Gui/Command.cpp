/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <QApplication>
#include <QMessageBox>
#include <sstream>

#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <Base/CoordinateSystem.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Approximation.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Part/App/FaceMakerCheese.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/Tools.h>
#include <Mod/Points/App/Structured.h>
#include <Mod/ReverseEngineering/App/ApproxSurface.h>

#include "FitBSplineSurface.h"
#include "Poisson.h"
#include "Segmentation.h"
#include "SegmentationManual.h"


using namespace std;

DEF_STD_CMD_A(CmdApproxSurface)

CmdApproxSurface::CmdApproxSurface()
    : Command("Reen_ApproxSurface")
{
    sAppModule = "Reen";
    sGroup = QT_TR_NOOP("Reverse Engineering");
    sMenuText = QT_TR_NOOP("Approximate B-spline surface...");
    sToolTipText = QT_TR_NOOP("Approximate a B-spline surface");
    sWhatsThis = "Reen_ApproxSurface";
    sStatusTip = sToolTipText;
    sPixmap = "actions/FitSurface";
}

void CmdApproxSurface::activated(int)
{
    App::DocumentObjectT objT;
    std::vector<App::DocumentObject*> obj =
        Gui::Selection().getObjectsOfType(App::GeoFeature::getClassTypeId());
    if (obj.size() != 1
        || !(obj.at(0)->isDerivedFrom(Points::Feature::getClassTypeId())
             || obj.at(0)->isDerivedFrom(Mesh::Feature::getClassTypeId()))) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            qApp->translate("Reen_ApproxSurface", "Wrong selection"),
            qApp->translate("Reen_ApproxSurface", "Please select a point cloud or mesh."));
        return;
    }

    objT = obj.front();
    Gui::Control().showDialog(new ReenGui::TaskFitBSplineSurface(objT));
}

bool CmdApproxSurface::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

DEF_STD_CMD_A(CmdApproxPlane)

CmdApproxPlane::CmdApproxPlane()
    : Command("Reen_ApproxPlane")
{
    sAppModule = "Reen";
    sGroup = QT_TR_NOOP("Reverse Engineering");
    sMenuText = QT_TR_NOOP("Plane...");
    sToolTipText = QT_TR_NOOP("Approximate a plane");
    sWhatsThis = "Reen_ApproxPlane";
    sStatusTip = sToolTipText;
}

void CmdApproxPlane::activated(int)
{
    std::vector<App::GeoFeature*> obj = Gui::Selection().getObjectsOfType<App::GeoFeature>();
    for (const auto& it : obj) {
        std::vector<Base::Vector3d> aPoints;
        std::vector<Base::Vector3d> aNormals;

        std::vector<App::Property*> List;
        it->getPropertyList(List);
        for (const auto& jt : List) {
            if (jt->getTypeId().isDerivedFrom(App::PropertyComplexGeoData::getClassTypeId())) {
                const Data::ComplexGeoData* data =
                    static_cast<App::PropertyComplexGeoData*>(jt)->getComplexData();
                if (data) {
                    data->getPoints(aPoints, aNormals, 0.01f);
                    if (!aPoints.empty()) {
                        break;
                    }
                }
            }
        }

        if (!aPoints.empty()) {
            // get a reference normal for the plane fit
            Base::Vector3f refNormal(0, 0, 0);
            if (!aNormals.empty()) {
                refNormal = Base::convertTo<Base::Vector3f>(aNormals.front());
            }

            std::vector<Base::Vector3f> aData;
            aData.reserve(aPoints.size());
            for (const auto& jt : aPoints) {
                aData.push_back(Base::toVector<float>(jt));
            }
            MeshCore::PlaneFit fit;
            fit.AddPoints(aData);
            float sigma = fit.Fit();
            Base::Vector3f base = fit.GetBase();
            Base::Vector3f dirU = fit.GetDirU();
            Base::Vector3f dirV = fit.GetDirV();
            Base::Vector3f norm = fit.GetNormal();

            // if the dot product of the reference with the plane normal is negative
            // a flip must be done
            if (refNormal * norm < 0) {
                norm = -norm;
                dirU = -dirU;
            }

            float length, width;
            fit.Dimension(length, width);

            // move to the corner point
            base = base - (0.5f * length * dirU + 0.5f * width * dirV);

            Base::CoordinateSystem cs;
            cs.setPosition(Base::convertTo<Base::Vector3d>(base));
            cs.setAxes(Base::convertTo<Base::Vector3d>(norm),
                       Base::convertTo<Base::Vector3d>(dirU));
            Base::Placement pm = Base::CoordinateSystem().displacement(cs);
            double q0, q1, q2, q3;
            pm.getRotation().getValue(q0, q1, q2, q3);

            Base::Console().Log("RMS value for plane fit with %lu points: %.4f\n",
                                aData.size(),
                                sigma);
            Base::Console().Log("  Plane base(%.4f, %.4f, %.4f)\n", base.x, base.y, base.z);
            Base::Console().Log("  Plane normal(%.4f, %.4f, %.4f)\n", norm.x, norm.y, norm.z);

            std::stringstream str;
            str << "from FreeCAD import Base" << std::endl;
            str << "App.ActiveDocument.addObject('Part::Plane','Plane_fit')" << std::endl;
            str << "App.ActiveDocument.ActiveObject.Length = " << length << std::endl;
            str << "App.ActiveDocument.ActiveObject.Width = " << width << std::endl;
            str << "App.ActiveDocument.ActiveObject.Placement = Base.Placement("
                << "Base.Vector(" << base.x << "," << base.y << "," << base.z << "),"
                << "Base.Rotation(" << q0 << "," << q1 << "," << q2 << "," << q3 << "))"
                << std::endl;

            openCommand(QT_TRANSLATE_NOOP("Command", "Fit plane"));
            runCommand(Gui::Command::Doc, str.str().c_str());
            commitCommand();
            updateActive();
        }
    }
}

bool CmdApproxPlane::isActive()
{
    if (getSelection().countObjectsOfType(App::GeoFeature::getClassTypeId()) == 1) {
        return true;
    }
    return false;
}

DEF_STD_CMD_A(CmdApproxCylinder)

CmdApproxCylinder::CmdApproxCylinder()
    : Command("Reen_ApproxCylinder")
{
    sAppModule = "Reen";
    sGroup = QT_TR_NOOP("Reverse Engineering");
    sMenuText = QT_TR_NOOP("Cylinder");
    sToolTipText = QT_TR_NOOP("Approximate a cylinder");
    sWhatsThis = "Reen_ApproxCylinder";
    sStatusTip = sToolTipText;
}

void CmdApproxCylinder::activated(int)
{
    std::vector<Mesh::Feature*> sel = getSelection().getObjectsOfType<Mesh::Feature>();
    openCommand(QT_TRANSLATE_NOOP("Command", "Fit cylinder"));
    for (auto it : sel) {
        const Mesh::MeshObject& mesh = it->Mesh.getValue();
        const MeshCore::MeshKernel& kernel = mesh.getKernel();
        MeshCore::CylinderFit fit;
        fit.AddPoints(kernel.GetPoints());

        // get normals
        {
            std::vector<MeshCore::FacetIndex> facets(kernel.CountFacets());
            std::generate(facets.begin(), facets.end(), Base::iotaGen<MeshCore::FacetIndex>(0));
            std::vector<Base::Vector3f> normals = kernel.GetFacetNormals(facets);
            Base::Vector3f base = fit.GetGravity();
            Base::Vector3f axis = fit.GetInitialAxisFromNormals(normals);
            fit.SetInitialValues(base, axis);
        }

        if (fit.Fit() < FLOAT_MAX) {
            Base::Vector3f base, top;
            fit.GetBounding(base, top);
            float height = Base::Distance(base, top);

            Base::Rotation rot;
            rot.setValue(Base::Vector3d(0, 0, 1), Base::convertTo<Base::Vector3d>(fit.GetAxis()));
            double q0, q1, q2, q3;
            rot.getValue(q0, q1, q2, q3);

            std::stringstream str;
            str << "from FreeCAD import Base" << std::endl;
            str << "App.ActiveDocument.addObject('Part::Cylinder','Cylinder_fit')" << std::endl;
            str << "App.ActiveDocument.ActiveObject.Radius = " << fit.GetRadius() << std::endl;
            str << "App.ActiveDocument.ActiveObject.Height = " << height << std::endl;
            str << "App.ActiveDocument.ActiveObject.Placement = Base.Placement("
                << "Base.Vector(" << base.x << "," << base.y << "," << base.z << "),"
                << "Base.Rotation(" << q0 << "," << q1 << "," << q2 << "," << q3 << "))"
                << std::endl;

            runCommand(Gui::Command::Doc, str.str().c_str());
        }
    }
    commitCommand();
    updateActive();
}

bool CmdApproxCylinder::isActive()
{
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0) {
        return true;
    }
    return false;
}

DEF_STD_CMD_A(CmdApproxSphere)

CmdApproxSphere::CmdApproxSphere()
    : Command("Reen_ApproxSphere")
{
    sAppModule = "Reen";
    sGroup = QT_TR_NOOP("Reverse Engineering");
    sMenuText = QT_TR_NOOP("Sphere");
    sToolTipText = QT_TR_NOOP("Approximate a sphere");
    sWhatsThis = "Reen_ApproxSphere";
    sStatusTip = sToolTipText;
}

void CmdApproxSphere::activated(int)
{
    std::vector<Mesh::Feature*> sel = getSelection().getObjectsOfType<Mesh::Feature>();
    openCommand(QT_TRANSLATE_NOOP("Command", "Fit sphere"));
    for (auto it : sel) {
        const Mesh::MeshObject& mesh = it->Mesh.getValue();
        const MeshCore::MeshKernel& kernel = mesh.getKernel();
        MeshCore::SphereFit fit;
        fit.AddPoints(kernel.GetPoints());
        if (fit.Fit() < FLOAT_MAX) {
            Base::Vector3f base = fit.GetCenter();

            std::stringstream str;
            str << "from FreeCAD import Base" << std::endl;
            str << "App.ActiveDocument.addObject('Part::Sphere','Sphere_fit')" << std::endl;
            str << "App.ActiveDocument.ActiveObject.Radius = " << fit.GetRadius() << std::endl;
            str << "App.ActiveDocument.ActiveObject.Placement = Base.Placement("
                << "Base.Vector(" << base.x << "," << base.y << "," << base.z << "),"
                << "Base.Rotation(" << 1 << "," << 0 << "," << 0 << "," << 0 << "))" << std::endl;

            runCommand(Gui::Command::Doc, str.str().c_str());
        }
    }
    commitCommand();
    updateActive();
}

bool CmdApproxSphere::isActive()
{
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0) {
        return true;
    }
    return false;
}

DEF_STD_CMD_A(CmdApproxPolynomial)

CmdApproxPolynomial::CmdApproxPolynomial()
    : Command("Reen_ApproxPolynomial")
{
    sAppModule = "Reen";
    sGroup = QT_TR_NOOP("Reverse Engineering");
    sMenuText = QT_TR_NOOP("Polynomial surface");
    sToolTipText = QT_TR_NOOP("Approximate a polynomial surface");
    sWhatsThis = "Reen_ApproxPolynomial";
    sStatusTip = sToolTipText;
}

void CmdApproxPolynomial::activated(int)
{
    std::vector<Mesh::Feature*> sel = getSelection().getObjectsOfType<Mesh::Feature>();
    App::Document* doc = App::GetApplication().getActiveDocument();
    openCommand(QT_TRANSLATE_NOOP("Command", "Fit polynomial surface"));
    for (auto it : sel) {
        const Mesh::MeshObject& mesh = it->Mesh.getValue();
        const MeshCore::MeshKernel& kernel = mesh.getKernel();
        MeshCore::SurfaceFit fit;
        fit.AddPoints(kernel.GetPoints());
        if (fit.Fit() < FLOAT_MAX) {
            Base::BoundBox3f bbox = fit.GetBoundings();
            std::vector<Base::Vector3d> poles =
                fit.toBezier(bbox.MinX, bbox.MaxX, bbox.MinY, bbox.MaxY);
            fit.Transform(poles);

            TColgp_Array2OfPnt grid(1, 3, 1, 3);
            grid.SetValue(1, 1, Base::convertTo<gp_Pnt>(poles.at(0)));
            grid.SetValue(2, 1, Base::convertTo<gp_Pnt>(poles.at(1)));
            grid.SetValue(3, 1, Base::convertTo<gp_Pnt>(poles.at(2)));
            grid.SetValue(1, 2, Base::convertTo<gp_Pnt>(poles.at(3)));
            grid.SetValue(2, 2, Base::convertTo<gp_Pnt>(poles.at(4)));
            grid.SetValue(3, 2, Base::convertTo<gp_Pnt>(poles.at(5)));
            grid.SetValue(1, 3, Base::convertTo<gp_Pnt>(poles.at(6)));
            grid.SetValue(2, 3, Base::convertTo<gp_Pnt>(poles.at(7)));
            grid.SetValue(3, 3, Base::convertTo<gp_Pnt>(poles.at(8)));

            Handle(Geom_BezierSurface) bezier(new Geom_BezierSurface(grid));
            Part::Feature* part =
                static_cast<Part::Feature*>(doc->addObject("Part::Spline", "Bezier"));
            part->Shape.setValue(Part::GeomBezierSurface(bezier).toShape());
        }
    }
    commitCommand();
    updateActive();
}

bool CmdApproxPolynomial::isActive()
{
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0) {
        return true;
    }
    return false;
}

DEF_STD_CMD_A(CmdSegmentation)

CmdSegmentation::CmdSegmentation()
    : Command("Reen_Segmentation")
{
    sAppModule = "Reen";
    sGroup = QT_TR_NOOP("Reverse Engineering");
    sMenuText = QT_TR_NOOP("Mesh segmentation...");
    sToolTipText = QT_TR_NOOP("Create mesh segments");
    sWhatsThis = "Reen_Segmentation";
    sStatusTip = sToolTipText;
}

void CmdSegmentation::activated(int)
{
    std::vector<Mesh::Feature*> objs = Gui::Selection().getObjectsOfType<Mesh::Feature>();
    Mesh::Feature* mesh = static_cast<Mesh::Feature*>(objs.front());
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg) {
        dlg = new ReverseEngineeringGui::TaskSegmentation(mesh);
    }
    Gui::Control().showDialog(dlg);
}

bool CmdSegmentation::isActive()
{
    if (Gui::Control().activeDialog()) {
        return false;
    }
    return Gui::Selection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 1;
}

DEF_STD_CMD_A(CmdSegmentationManual)

CmdSegmentationManual::CmdSegmentationManual()
    : Command("Reen_SegmentationManual")
{
    sAppModule = "Reen";
    sGroup = QT_TR_NOOP("Reverse Engineering");
    sMenuText = QT_TR_NOOP("Manual segmentation...");
    sToolTipText = QT_TR_NOOP("Create mesh segments manually");
    sWhatsThis = "Reen_SegmentationManual";
    sStatusTip = sToolTipText;
}

void CmdSegmentationManual::activated(int)
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg) {
        dlg = new ReverseEngineeringGui::TaskSegmentationManual();
    }
    Gui::Control().showDialog(dlg);
}

bool CmdSegmentationManual::isActive()
{
    if (Gui::Control().activeDialog()) {
        return false;
    }
    return hasActiveDocument();
}

DEF_STD_CMD_A(CmdSegmentationFromComponents)

CmdSegmentationFromComponents::CmdSegmentationFromComponents()
    : Command("Reen_SegmentationFromComponents")
{
    sAppModule = "Reen";
    sGroup = QT_TR_NOOP("Reverse Engineering");
    sMenuText = QT_TR_NOOP("From components");
    sToolTipText = QT_TR_NOOP("Create mesh segments from components");
    sWhatsThis = "Reen_SegmentationFromComponents";
    sStatusTip = sToolTipText;
}

void CmdSegmentationFromComponents::activated(int)
{
    std::vector<Mesh::Feature*> sel = getSelection().getObjectsOfType<Mesh::Feature>();
    App::Document* doc = App::GetApplication().getActiveDocument();
    doc->openTransaction("Segmentation");

    for (auto it : sel) {
        std::string internalname = "Segments_";
        internalname += it->getNameInDocument();
        App::DocumentObjectGroup* group = static_cast<App::DocumentObjectGroup*>(
            doc->addObject("App::DocumentObjectGroup", internalname.c_str()));
        std::string labelname = "Segments ";
        labelname += it->Label.getValue();
        group->Label.setValue(labelname);

        const Mesh::MeshObject& mesh = it->Mesh.getValue();
        std::vector<std::vector<MeshCore::FacetIndex>> comps = mesh.getComponents();
        for (const auto& jt : comps) {
            std::unique_ptr<Mesh::MeshObject> segment(mesh.meshFromSegment(jt));
            Mesh::Feature* feaSegm =
                static_cast<Mesh::Feature*>(group->addObject("Mesh::Feature", "Segment"));
            Mesh::MeshObject* feaMesh = feaSegm->Mesh.startEditing();
            feaMesh->swap(*segment);
            feaSegm->Mesh.finishEditing();
        }
    }

    doc->commitTransaction();
    doc->recompute();
}

bool CmdSegmentationFromComponents::isActive()
{
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0) {
        return true;
    }
    return false;
}

DEF_STD_CMD_A(CmdMeshBoundary)

CmdMeshBoundary::CmdMeshBoundary()
    : Command("Reen_MeshBoundary")
{
    sAppModule = "Reen";
    sGroup = QT_TR_NOOP("Reverse Engineering");
    sMenuText = QT_TR_NOOP("Wire from mesh boundary...");
    sToolTipText = QT_TR_NOOP("Create wire from mesh boundaries");
    sWhatsThis = "Reen_Segmentation";
    sStatusTip = sToolTipText;
}

void CmdMeshBoundary::activated(int)
{
    std::vector<Mesh::Feature*> objs = Gui::Selection().getObjectsOfType<Mesh::Feature>();
    App::Document* document = App::GetApplication().getActiveDocument();
    document->openTransaction("Wire from mesh");
    for (auto it : objs) {
        const Mesh::MeshObject& mesh = it->Mesh.getValue();
        std::list<std::vector<Base::Vector3f>> bounds;
        MeshCore::MeshAlgorithm algo(mesh.getKernel());
        algo.GetMeshBorders(bounds);

        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);

        TopoDS_Shape shape;
        std::vector<TopoDS_Wire> wires;

        for (const auto& bt : bounds) {
            BRepBuilderAPI_MakePolygon mkPoly;
            for (auto it = bt.rbegin(); it != bt.rend(); ++it) {
                mkPoly.Add(gp_Pnt(it->x, it->y, it->z));
            }
            if (mkPoly.IsDone()) {
                builder.Add(compound, mkPoly.Wire());
                wires.push_back(mkPoly.Wire());
            }
        }

        try {
            shape = Part::FaceMakerCheese::makeFace(wires);
        }
        catch (...) {
        }

        if (!shape.IsNull()) {
            Part::Feature* shapeFea =
                static_cast<Part::Feature*>(document->addObject("Part::Feature", "Face from mesh"));
            shapeFea->Shape.setValue(shape);
        }
        else {
            Part::Feature* shapeFea =
                static_cast<Part::Feature*>(document->addObject("Part::Feature", "Wire from mesh"));
            shapeFea->Shape.setValue(compound);
        }
    }
    document->commitTransaction();
}

bool CmdMeshBoundary::isActive()
{
    return Gui::Selection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0;
}

DEF_STD_CMD_A(CmdPoissonReconstruction)

CmdPoissonReconstruction::CmdPoissonReconstruction()
    : Command("Reen_PoissonReconstruction")
{
    sAppModule = "Reen";
    sGroup = QT_TR_NOOP("Reverse Engineering");
    sMenuText = QT_TR_NOOP("Poisson...");
    sToolTipText = QT_TR_NOOP("Poisson surface reconstruction");
    sWhatsThis = "Reen_PoissonReconstruction";
    sStatusTip = sToolTipText;
}

void CmdPoissonReconstruction::activated(int)
{
    App::DocumentObjectT objT;
    std::vector<App::DocumentObject*> obj =
        Gui::Selection().getObjectsOfType(Points::Feature::getClassTypeId());
    if (obj.size() != 1) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            qApp->translate("Reen_ApproxSurface", "Wrong selection"),
            qApp->translate("Reen_ApproxSurface", "Please select a single point cloud."));
        return;
    }

    objT = obj.front();
    Gui::Control().showDialog(new ReenGui::TaskPoisson(objT));
}

bool CmdPoissonReconstruction::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

DEF_STD_CMD_A(CmdViewTriangulation)

CmdViewTriangulation::CmdViewTriangulation()
    : Command("Reen_ViewTriangulation")
{
    sAppModule = "Reen";
    sGroup = QT_TR_NOOP("Reverse Engineering");
    sMenuText = QT_TR_NOOP("Structured point clouds");
    sToolTipText = QT_TR_NOOP("Triangulation of structured point clouds");
    sStatusTip = QT_TR_NOOP("Triangulation of structured point clouds");
    sWhatsThis = "Reen_ViewTriangulation";
}

void CmdViewTriangulation::activated(int)
{
    std::vector<App::DocumentObject*> obj =
        Gui::Selection().getObjectsOfType(Points::Structured::getClassTypeId());
    addModule(App, "ReverseEngineering");
    openCommand(QT_TRANSLATE_NOOP("Command", "View triangulation"));
    try {
        for (const auto& it : obj) {
            App::DocumentObjectT objT(it);
            QString document = QString::fromStdString(objT.getDocumentPython());
            QString object = QString::fromStdString(objT.getObjectPython());

            QString command = QString::fromLatin1("%1.addObject('Mesh::Feature', 'View mesh').Mesh "
                                                  "= ReverseEngineering.viewTriangulation("
                                                  "Points=%2.Points,"
                                                  "Width=%2.Width,"
                                                  "Height=%2.Height)")
                                  .arg(document, object);
            runCommand(Doc, command.toLatin1());
        }

        commitCommand();
        updateActive();
    }
    catch (const Base::Exception& e) {
        abortCommand();
        QMessageBox::warning(Gui::getMainWindow(),
                             qApp->translate("Reen_ViewTriangulation", "View triangulation failed"),
                             QString::fromLatin1(e.what()));
    }
}

bool CmdViewTriangulation::isActive()
{
    return (Gui::Selection().countObjectsOfType(Points::Structured::getClassTypeId()) > 0);
}

void CreateReverseEngineeringCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdApproxSurface());
    rcCmdMgr.addCommand(new CmdApproxPlane());
    rcCmdMgr.addCommand(new CmdApproxCylinder());
    rcCmdMgr.addCommand(new CmdApproxSphere());
    rcCmdMgr.addCommand(new CmdApproxPolynomial());
    rcCmdMgr.addCommand(new CmdSegmentation());
    rcCmdMgr.addCommand(new CmdSegmentationManual());
    rcCmdMgr.addCommand(new CmdSegmentationFromComponents());
    rcCmdMgr.addCommand(new CmdMeshBoundary());
    rcCmdMgr.addCommand(new CmdPoissonReconstruction());
    rcCmdMgr.addCommand(new CmdViewTriangulation());
}
