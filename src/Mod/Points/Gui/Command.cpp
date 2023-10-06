/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <Inventor/events/SoMouseButtonEvent.h>
#include <QInputDialog>
#include <algorithm>
#endif
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>

#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/WaitCursor.h>

#include "../App/PointsFeature.h"
#include "../App/Properties.h"
#include "../App/Structured.h"
#include "../App/Tools.h"

#include "DlgPointsReadImp.h"
#include "ViewProvider.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===========================================================================
// CmdPointsImport
//===========================================================================
DEF_STD_CMD_A(CmdPointsImport)

CmdPointsImport::CmdPointsImport()
    : Command("Points_Import")
{
    sAppModule = "Points";
    sGroup = QT_TR_NOOP("Points");
    sMenuText = QT_TR_NOOP("Import points...");
    sToolTipText = QT_TR_NOOP("Imports a point cloud");
    sWhatsThis = "Points_Import";
    sStatusTip = QT_TR_NOOP("Imports a point cloud");
    sPixmap = "Points_Import_Point_cloud";
}

void CmdPointsImport::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    QString fn = Gui::FileDialog::getOpenFileName(
        Gui::getMainWindow(),
        QString(),
        QString(),
        QString::fromLatin1("%1 (*.asc *.pcd *.ply);;%2 (*.*)")
            .arg(QObject::tr("Point formats"), QObject::tr("All Files")));
    if (fn.isEmpty()) {
        return;
    }

    if (!fn.isEmpty()) {
        fn = Base::Tools::escapeEncodeFilename(fn);
        Gui::Document* doc = getActiveGuiDocument();
        openCommand(QT_TRANSLATE_NOOP("Command", "Import points"));
        addModule(Command::App, "Points");
        doCommand(Command::Doc,
                  "Points.insert(\"%s\", \"%s\")",
                  fn.toUtf8().data(),
                  doc->getDocument()->getName());
        commitCommand();

        updateActive();
    }
}

bool CmdPointsImport::isActive()
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}

DEF_STD_CMD_A(CmdPointsExport)

CmdPointsExport::CmdPointsExport()
    : Command("Points_Export")
{
    sAppModule = "Points";
    sGroup = QT_TR_NOOP("Points");
    sMenuText = QT_TR_NOOP("Export points...");
    sToolTipText = QT_TR_NOOP("Exports a point cloud");
    sWhatsThis = "Points_Export";
    sStatusTip = QT_TR_NOOP("Exports a point cloud");
    sPixmap = "Points_Export_Point_cloud";
}

void CmdPointsExport::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    addModule(Command::App, "Points");
    std::vector<App::DocumentObject*> points =
        getSelection().getObjectsOfType(Points::Feature::getClassTypeId());
    for (auto point : points) {
        QString fn = Gui::FileDialog::getSaveFileName(
            Gui::getMainWindow(),
            QString(),
            QString(),
            QString::fromLatin1("%1 (*.asc *.pcd *.ply);;%2 (*.*)")
                .arg(QObject::tr("Point formats"), QObject::tr("All Files")));
        if (fn.isEmpty()) {
            break;
        }

        if (!fn.isEmpty()) {
            fn = Base::Tools::escapeEncodeFilename(fn);
            doCommand(Command::Doc,
                      "Points.export([App.ActiveDocument.%s], \"%s\")",
                      point->getNameInDocument(),
                      fn.toUtf8().data());
        }
    }
}

bool CmdPointsExport::isActive()
{
    return getSelection().countObjectsOfType(Points::Feature::getClassTypeId()) > 0;
}

DEF_STD_CMD_A(CmdPointsTransform)

CmdPointsTransform::CmdPointsTransform()
    : Command("Points_Transform")
{
    sAppModule = "Points";
    sGroup = QT_TR_NOOP("Points");
    sMenuText = QT_TR_NOOP("Transform Points");
    sToolTipText = QT_TR_NOOP("Test to transform a point cloud");
    sWhatsThis = "Points_Transform";
    sStatusTip = QT_TR_NOOP("Test to transform a point cloud");
    sPixmap = "Test1";
}

void CmdPointsTransform::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // This is a test command to transform a point cloud directly written in C++ (not Python)
    Base::Placement trans;
    trans.setRotation(Base::Rotation(Base::Vector3d(0.0, 0.0, 1.0), 1.570796));

    openCommand(QT_TRANSLATE_NOOP("Command", "Transform points"));
    // std::vector<App::DocumentObject*> points =
    // getSelection().getObjectsOfType(Points::Feature::getClassTypeId()); for
    // (std::vector<App::DocumentObject*>::const_iterator it = points.begin(); it != points.end();
    // ++it) {
    //     Base::Placement p = static_cast<Points::Feature*>(*it)->Placement.getValue();
    //     p._rot *= Base::Rotation(Base::Vector3d(0.0, 0.0, 1.0), 1.570796);
    //     static_cast<Points::Feature*>(*it)->Placement.setValue(p);
    // }
    commitCommand();
}

bool CmdPointsTransform::isActive()
{
    return getSelection().countObjectsOfType(Points::Feature::getClassTypeId()) > 0;
}

DEF_STD_CMD_A(CmdPointsConvert)

CmdPointsConvert::CmdPointsConvert()
    : Command("Points_Convert")
{
    sAppModule = "Points";
    sGroup = QT_TR_NOOP("Points");
    sMenuText = QT_TR_NOOP("Convert to points...");
    sToolTipText = QT_TR_NOOP("Convert to points");
    sWhatsThis = "Points_Convert";
    sStatusTip = QT_TR_NOOP("Convert to points");
    sPixmap = "Points_Convert";
}

void CmdPointsConvert::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    double STD_OCC_TOLERANCE = 1e-6;

    int decimals = Base::UnitsApi::getDecimals();
    double tolerance_from_decimals = pow(10., -decimals);

    double minimal_tolerance =
        tolerance_from_decimals < STD_OCC_TOLERANCE ? STD_OCC_TOLERANCE : tolerance_from_decimals;

    bool ok;
    double tol = QInputDialog::getDouble(Gui::getMainWindow(),
                                         QObject::tr("Distance"),
                                         QObject::tr("Enter maximum distance:"),
                                         0.1,
                                         minimal_tolerance,
                                         10.0,
                                         decimals,
                                         &ok,
                                         Qt::MSWindowsFixedSizeDialogHint);
    if (!ok) {
        return;
    }

    Gui::WaitCursor wc;
    openCommand(QT_TRANSLATE_NOOP("Command", "Convert to points"));
    std::vector<App::GeoFeature*> geoObject = getSelection().getObjectsOfType<App::GeoFeature>();

    auto run_python = [](const std::vector<App::GeoFeature*>& geoObject, double tol) -> bool {
        Py::List list;
        for (auto it : geoObject) {
            const App::PropertyComplexGeoData* prop = it->getPropertyOfGeometry();
            if (prop) {
                list.append(Py::asObject(it->getPyObject()));
            }
        }

        if (list.size() > 0) {
            PyObject* module = PyImport_ImportModule("pointscommands.commands");
            if (!module) {
                throw Py::Exception();
            }

            Py::Module commands(module, true);
            commands.callMemberFunction("make_points_from_geometry",
                                        Py::TupleN(list, Py::Float(tol)));
            return true;
        }

        return false;
    };

    Base::PyGILStateLocker lock;
    try {
        if (run_python(geoObject, tol)) {
            commitCommand();
        }
        else {
            abortCommand();
        }
    }
    catch (const Py::Exception&) {
        abortCommand();
        Base::PyException e;
        e.ReportException();
    }
}

bool CmdPointsConvert::isActive()
{
    return getSelection().countObjectsOfType(Base::Type::fromName("App::GeoFeature")) > 0;
}

DEF_STD_CMD_A(CmdPointsPolyCut)

CmdPointsPolyCut::CmdPointsPolyCut()
    : Command("Points_PolyCut")
{
    sAppModule = "Points";
    sGroup = QT_TR_NOOP("Points");
    sMenuText = QT_TR_NOOP("Cut point cloud");
    sToolTipText = QT_TR_NOOP("Cuts a point cloud with a picked polygon");
    sWhatsThis = "Points_PolyCut";
    sStatusTip = QT_TR_NOOP("Cuts a point cloud with a picked polygon");
    sPixmap = "PolygonPick";
}

void CmdPointsPolyCut::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    std::vector<App::DocumentObject*> docObj =
        Gui::Selection().getObjectsOfType(Points::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end();
         ++it) {
        if (it == docObj.begin()) {
            Gui::Document* doc = getActiveGuiDocument();
            Gui::MDIView* view = doc->getActiveView();
            if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
                Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
                viewer->setEditing(true);
                viewer->startSelection(Gui::View3DInventorViewer::Lasso);
                viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(),
                                         PointsGui::ViewProviderPoints::clipPointsCallback);
            }
            else {
                return;
            }
        }

        Gui::ViewProvider* pVP = getActiveGuiDocument()->getViewProvider(*it);
        pVP->startEditing(Gui::ViewProvider::Cutting);
    }
}

bool CmdPointsPolyCut::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Points::Feature::getClassTypeId()) > 0;
}

DEF_STD_CMD_A(CmdPointsMerge)

CmdPointsMerge::CmdPointsMerge()
    : Command("Points_Merge")
{
    sAppModule = "Points";
    sGroup = QT_TR_NOOP("Points");
    sMenuText = QT_TR_NOOP("Merge point clouds");
    sToolTipText = QT_TR_NOOP("Merge several point clouds into one");
    sWhatsThis = "Points_Merge";
    sStatusTip = QT_TR_NOOP("Merge several point clouds into one");
    sPixmap = "Points_Merge";
}

void CmdPointsMerge::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    App::Document* doc = App::GetApplication().getActiveDocument();
    doc->openTransaction("Merge point clouds");
    Points::Feature* pts =
        static_cast<Points::Feature*>(doc->addObject("Points::Feature", "Merged Points"));
    Points::PointKernel* kernel = pts->Points.startEditing();

    std::vector<App::DocumentObject*> docObj =
        Gui::Selection().getObjectsOfType(Points::Feature::getClassTypeId());
    for (auto it : docObj) {
        const Points::PointKernel& k = static_cast<Points::Feature*>(it)->Points.getValue();
        std::size_t numPts = kernel->size();
        kernel->resize(numPts + k.size());
        for (std::size_t i = 0; i < k.size(); ++i) {
            kernel->setPoint(i + numPts, k.getPoint(i));
        }
    }

    pts->Points.finishEditing();

    // Add properties
    std::string displayMode = "Points";
    if (Points::copyProperty<App::PropertyColorList>(pts, docObj, "Color")) {
        displayMode = "Color";
    }
    if (Points::copyProperty<Points::PropertyNormalList>(pts, docObj, "Normal")) {
        displayMode = "Shaded";
    }
    if (Points::copyProperty<Points::PropertyGreyValueList>(pts, docObj, "Intensity")) {
        displayMode = "Intensity";
    }

    if (auto vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(
            Gui::Application::Instance->getViewProvider(pts))) {
        vp->DisplayMode.setValue(displayMode.c_str());
    }

    doc->commitTransaction();
    updateActive();
}

bool CmdPointsMerge::isActive()
{
    return getSelection().countObjectsOfType(Points::Feature::getClassTypeId()) > 1;
}

DEF_STD_CMD_A(CmdPointsStructure)

CmdPointsStructure::CmdPointsStructure()
    : Command("Points_Structure")
{
    sAppModule = "Points";
    sGroup = QT_TR_NOOP("Points");
    sMenuText = QT_TR_NOOP("Structured point cloud");
    sToolTipText = QT_TR_NOOP("Convert points to structured point cloud");
    sWhatsThis = "Points_Structure";
    sStatusTip = QT_TR_NOOP("Convert points to structured point cloud");
    sPixmap = "Points_Structure";
}

void CmdPointsStructure::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    App::Document* doc = App::GetApplication().getActiveDocument();
    doc->openTransaction("Structure point cloud");

    std::vector<App::DocumentObject*> docObj =
        Gui::Selection().getObjectsOfType(Points::Feature::getClassTypeId());
    for (auto it : docObj) {
        std::string name = it->Label.getValue();
        name += " (Structured)";
        Points::Structured* output =
            static_cast<Points::Structured*>(doc->addObject("Points::Structured", name.c_str()));
        output->Label.setValue(name);

        // Already sorted, so just make a copy
        if (it->getTypeId().isDerivedFrom(Points::Structured::getClassTypeId())) {
            Points::Structured* input = static_cast<Points::Structured*>(it);

            Points::PointKernel* kernel = output->Points.startEditing();
            const Points::PointKernel& k = input->Points.getValue();

            kernel->resize(k.size());
            for (std::size_t i = 0; i < k.size(); ++i) {
                kernel->setPoint(i, k.getPoint(i));
            }
            output->Points.finishEditing();
            output->Width.setValue(input->Width.getValue());
            output->Height.setValue(input->Height.getValue());
        }
        // Sort the points
        else {
            Points::Feature* input = static_cast<Points::Feature*>(it);

            Points::PointKernel* kernel = output->Points.startEditing();
            const Points::PointKernel& k = input->Points.getValue();

            Base::BoundBox3d bbox = input->Points.getBoundingBox();
            double width = bbox.LengthX();
            double height = bbox.LengthY();

            // Count the number of different x or y values to get the size
            std::set<double> countX, countY;
            for (std::size_t i = 0; i < k.size(); ++i) {
                Base::Vector3d pnt = k.getPoint(i);
                countX.insert(pnt.x);
                countY.insert(pnt.y);
            }

            long width_l = long(countX.size());
            long height_l = long(countY.size());

            double dx = width / (width_l - 1);
            double dy = height / (height_l - 1);

            // Pre-fill the vector with <nan, nan, nan> points and afterwards replace them
            // with valid point coordinates
            double nan = std::numeric_limits<double>::quiet_NaN();
            std::vector<Base::Vector3d> sortedPoints(width_l * height_l,
                                                     Base::Vector3d(nan, nan, nan));

            for (std::size_t i = 0; i < k.size(); ++i) {
                Base::Vector3d pnt = k.getPoint(i);
                double xi = (pnt.x - bbox.MinX) / dx;
                double yi = (pnt.y - bbox.MinY) / dy;

                double xx = std::fabs(xi - std::round(xi));
                double yy = std::fabs(yi - std::round(yi));
                if (xx < 0.01 && yy < 0.01) {
                    xi = std::round(xi);
                    yi = std::round(yi);
                    long index = long(yi * width_l + xi);
                    sortedPoints[index] = pnt;
                }
            }

            kernel->resize(sortedPoints.size());
            for (std::size_t index = 0; index < sortedPoints.size(); index++) {
                kernel->setPoint(index, sortedPoints[index]);
            }

            output->Points.finishEditing();
            output->Width.setValue(width_l);
            output->Height.setValue(height_l);
        }
    }

    doc->commitTransaction();
    updateActive();
}

bool CmdPointsStructure::isActive()
{
    return getSelection().countObjectsOfType(Points::Feature::getClassTypeId()) == 1;
}

void CreatePointsCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdPointsImport());
    rcCmdMgr.addCommand(new CmdPointsExport());
    rcCmdMgr.addCommand(new CmdPointsTransform());
    rcCmdMgr.addCommand(new CmdPointsConvert());
    rcCmdMgr.addCommand(new CmdPointsPolyCut());
    rcCmdMgr.addCommand(new CmdPointsMerge());
    rcCmdMgr.addCommand(new CmdPointsStructure());
}
