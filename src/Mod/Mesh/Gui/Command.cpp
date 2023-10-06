/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#ifdef FC_OS_WIN32
#include <windows.h>
#endif
#include <map>

#include <QApplication>
#include <QPointer>
#include <qfileinfo.h>
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <qstringlist.h>
#endif

#ifndef __InventorAll__
#include <Gui/InventorAll.h>
#endif
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/MouseSelection.h>
#include <Gui/NavigationStyle.h>
#include <Gui/Selection.h>
#include <Gui/WaitCursor.h>

#include <Mod/Mesh/App/Core/Smoothing.h>
#include <Mod/Mesh/App/FeatureMeshCurvature.h>
#include <Mod/Mesh/App/MeshFeature.h>

#include "DlgDecimating.h"
#include "DlgEvaluateMeshImp.h"
#include "DlgRegularSolidImp.h"
#include "DlgSmoothing.h"
#include "MeshEditor.h"
#include "RemeshGmsh.h"
#include "RemoveComponents.h"
#include "Segmentation.h"
#include "SegmentationBestFit.h"
#include "ViewProviderCurvature.h"
#include "ViewProviderMeshFaceSet.h"


using namespace Mesh;


DEF_STD_CMD_A(CmdMeshUnion)

CmdMeshUnion::CmdMeshUnion()
    : Command("Mesh_Union")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Union");
    sToolTipText = sMenuText;
    sWhatsThis = "Mesh_Union";
    sStatusTip = sMenuText;
    sPixmap = "Mesh_Union";
}

void CmdMeshUnion::activated(int)
{
    std::vector<App::DocumentObject*> obj =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    std::string name1 = obj.front()->getNameInDocument();
    std::string name2 = obj.back()->getNameInDocument();
    std::string name3 = getUniqueObjectName("Union");

    try {
        openCommand(QT_TRANSLATE_NOOP("Command", "Mesh union"));
        doCommand(Doc,
                  "import OpenSCADUtils\n"
                  "mesh = "
                  "OpenSCADUtils.meshoptempfile('union',(App.ActiveDocument.%s.Mesh,App."
                  "ActiveDocument.%s.Mesh))\n"
                  "App.ActiveDocument.addObject(\"Mesh::Feature\",\"%s\")\n"
                  "App.ActiveDocument.%s.Mesh = mesh\n",
                  name1.c_str(),
                  name2.c_str(),
                  name3.c_str(),
                  name3.c_str());

        updateActive();
        commitCommand();
    }
    catch (...) {
        abortCommand();
        Base::PyGILStateLocker lock;
        PyObject* main = PyImport_AddModule("__main__");
        PyObject* dict = PyModule_GetDict(main);
        Py::Dict d(PyDict_Copy(dict), true);

        const char* cmd = "import OpenSCADUtils\nopenscadfilename = OpenSCADUtils.getopenscadexe()";
        PyObject* result = PyRun_String(cmd, Py_file_input, d.ptr(), d.ptr());
        Py_XDECREF(result);

        bool found = false;
        if (d.hasKey("openscadfilename")) {
            found = (bool)Py::Boolean(d.getItem("openscadfilename"));
        }

        if (found) {
            QMessageBox::critical(
                Gui::getMainWindow(),
                qApp->translate("Mesh_Union", "OpenSCAD"),
                qApp->translate("Mesh_Union", "Unknown error occurred while running OpenSCAD."));
        }
        else {
            QMessageBox::warning(
                Gui::getMainWindow(),
                qApp->translate("Mesh_Union", "OpenSCAD"),
                qApp->translate("Mesh_Union",
                                "OpenSCAD cannot be found on your system.\n"
                                "Please visit http://www.openscad.org/index.html to install it."));
        }
    }
}

bool CmdMeshUnion::isActive()
{
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 2;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshDifference)

CmdMeshDifference::CmdMeshDifference()
    : Command("Mesh_Difference")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Difference");
    sToolTipText = sMenuText;
    sWhatsThis = "Mesh_Difference";
    sStatusTip = sMenuText;
    sPixmap = "Mesh_Difference";
}

void CmdMeshDifference::activated(int)
{
    std::vector<App::DocumentObject*> obj =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    std::string name1 = obj.front()->getNameInDocument();
    std::string name2 = obj.back()->getNameInDocument();
    std::string name3 = getUniqueObjectName("Difference");
    openCommand(QT_TRANSLATE_NOOP("Command", "Mesh difference"));

    try {
        doCommand(Doc,
                  "import OpenSCADUtils\n"
                  "mesh = "
                  "OpenSCADUtils.meshoptempfile('difference',(App.ActiveDocument.%s.Mesh,App."
                  "ActiveDocument.%s.Mesh))\n"
                  "App.ActiveDocument.addObject(\"Mesh::Feature\",\"%s\")\n"
                  "App.ActiveDocument.%s.Mesh = mesh\n",
                  name1.c_str(),
                  name2.c_str(),
                  name3.c_str(),
                  name3.c_str());

        updateActive();
        commitCommand();
    }
    catch (...) {
        abortCommand();
        Base::PyGILStateLocker lock;
        PyObject* main = PyImport_AddModule("__main__");
        PyObject* dict = PyModule_GetDict(main);
        Py::Dict d(PyDict_Copy(dict), true);

        const char* cmd = "import OpenSCADUtils\nopenscadfilename = OpenSCADUtils.getopenscadexe()";
        PyObject* result = PyRun_String(cmd, Py_file_input, d.ptr(), d.ptr());
        Py_XDECREF(result);

        bool found = false;
        if (d.hasKey("openscadfilename")) {
            found = (bool)Py::Boolean(d.getItem("openscadfilename"));
        }

        if (found) {
            QMessageBox::critical(
                Gui::getMainWindow(),
                qApp->translate("Mesh_Union", "OpenSCAD"),
                qApp->translate("Mesh_Union", "Unknown error occurred while running OpenSCAD."));
        }
        else {
            QMessageBox::warning(
                Gui::getMainWindow(),
                qApp->translate("Mesh_Union", "OpenSCAD"),
                qApp->translate("Mesh_Union",
                                "OpenSCAD cannot be found on your system.\n"
                                "Please visit http://www.openscad.org/index.html to install it."));
        }
    }
}

bool CmdMeshDifference::isActive()
{
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 2;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshIntersection)

CmdMeshIntersection::CmdMeshIntersection()
    : Command("Mesh_Intersection")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Intersection");
    sToolTipText = sMenuText;
    sWhatsThis = "Mesh_Intersection";
    sStatusTip = sMenuText;
    sPixmap = "Mesh_Intersection";
}

void CmdMeshIntersection::activated(int)
{
    std::vector<App::DocumentObject*> obj =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    std::string name1 = obj.front()->getNameInDocument();
    std::string name2 = obj.back()->getNameInDocument();
    std::string name3 = getUniqueObjectName("Intersection");
    openCommand(QT_TRANSLATE_NOOP("Command", "Mesh intersection"));

    try {
        doCommand(Doc,
                  "import OpenSCADUtils\n"
                  "mesh = "
                  "OpenSCADUtils.meshoptempfile('intersection',(App.ActiveDocument.%s.Mesh,App."
                  "ActiveDocument.%s.Mesh))\n"
                  "App.ActiveDocument.addObject(\"Mesh::Feature\",\"%s\")\n"
                  "App.ActiveDocument.%s.Mesh = mesh\n",
                  name1.c_str(),
                  name2.c_str(),
                  name3.c_str(),
                  name3.c_str());

        updateActive();
        commitCommand();
    }
    catch (...) {
        abortCommand();
        Base::PyGILStateLocker lock;
        PyObject* main = PyImport_AddModule("__main__");
        PyObject* dict = PyModule_GetDict(main);
        Py::Dict d(PyDict_Copy(dict), true);

        const char* cmd = "import OpenSCADUtils\nopenscadfilename = OpenSCADUtils.getopenscadexe()";
        PyObject* result = PyRun_String(cmd, Py_file_input, d.ptr(), d.ptr());
        Py_XDECREF(result);

        bool found = false;
        if (d.hasKey("openscadfilename")) {
            found = (bool)Py::Boolean(d.getItem("openscadfilename"));
        }

        if (found) {
            QMessageBox::critical(
                Gui::getMainWindow(),
                qApp->translate("Mesh_Union", "OpenSCAD"),
                qApp->translate("Mesh_Union", "Unknown error occurred while running OpenSCAD."));
        }
        else {
            QMessageBox::warning(
                Gui::getMainWindow(),
                qApp->translate("Mesh_Union", "OpenSCAD"),
                qApp->translate("Mesh_Union",
                                "OpenSCAD cannot be found on your system.\n"
                                "Please visit http://www.openscad.org/index.html to install it."));
        }
    }
}

bool CmdMeshIntersection::isActive()
{
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 2;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshImport)

CmdMeshImport::CmdMeshImport()
    : Command("Mesh_Import")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Import mesh...");
    sToolTipText = QT_TR_NOOP("Imports a mesh from file");
    sWhatsThis = "Mesh_Import";
    sStatusTip = QT_TR_NOOP("Imports a mesh from file");
    sPixmap = "Mesh_Import";
}

void CmdMeshImport::activated(int)
{
    // use current path as default
    QStringList filter;
    filter << QString::fromLatin1("%1 (*.stl *.ast *.bms *.obj *.off *.iv *.ply *.nas *.bdf)")
                  .arg(QObject::tr("All Mesh Files"));
    filter << QString::fromLatin1("%1 (*.stl)").arg(QObject::tr("Binary STL"));
    filter << QString::fromLatin1("%1 (*.ast)").arg(QObject::tr("ASCII STL"));
    filter << QString::fromLatin1("%1 (*.bms)").arg(QObject::tr("Binary Mesh"));
    filter << QString::fromLatin1("%1 (*.obj)").arg(QObject::tr("Alias Mesh"));
    filter << QString::fromLatin1("%1 (*.off)").arg(QObject::tr("Object File Format"));
    filter << QString::fromLatin1("%1 (*.iv)").arg(QObject::tr("Inventor V2.1 ASCII"));
    filter << QString::fromLatin1("%1 (*.ply)").arg(QObject::tr("Stanford Polygon"));
    filter << QString::fromLatin1("%1 (*.nas *.bdf)").arg(QObject::tr("NASTRAN"));
    filter << QString::fromLatin1("%1 (*.*)").arg(QObject::tr("All Files"));

    // Allow multi selection
    QStringList fn = Gui::FileDialog::getOpenFileNames(Gui::getMainWindow(),
                                                       QObject::tr("Import mesh"),
                                                       QString(),
                                                       filter.join(QLatin1String(";;")));
    for (const auto& it : fn) {
        std::string unicodepath = Base::Tools::escapedUnicodeFromUtf8(it.toUtf8().data());
        unicodepath = Base::Tools::escapeEncodeFilename(unicodepath);
        openCommand(QT_TRANSLATE_NOOP("Command", "Import Mesh"));
        doCommand(Doc, "import Mesh");
        doCommand(Doc, "Mesh.insert(u\"%s\")", unicodepath.c_str());
        commitCommand();
        updateActive();
    }
}

bool CmdMeshImport::isActive()
{
    return (getActiveGuiDocument() ? true : false);
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshExport)

CmdMeshExport::CmdMeshExport()
    : Command("Mesh_Export")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Export mesh...");
    sToolTipText = QT_TR_NOOP("Exports a mesh to file");
    sWhatsThis = "Mesh_Export";
    sStatusTip = QT_TR_NOOP("Exports a mesh to file");
    sPixmap = "Mesh_Export";
}

void CmdMeshExport::activated(int)
{
    std::vector<App::DocumentObject*> docObjs =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    if (docObjs.size() != 1) {
        return;
    }

    App::DocumentObject* docObj = docObjs.front();

    // clang-format off
    QString dir = QString::fromUtf8(docObj->Label.getValue());
    QList<QPair<QString, QByteArray> > ext;
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.stl)").arg(QObject::tr("Binary STL")), "STL");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.stl)").arg(QObject::tr("ASCII STL")), "AST");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.ast)").arg(QObject::tr("ASCII STL")), "AST");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.bms)").arg(QObject::tr("Binary Mesh")), "BMS");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.obj)").arg(QObject::tr("Alias Mesh")), "OBJ");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.smf)").arg(QObject::tr("Simple Model Format")), "SMF");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.off)").arg(QObject::tr("Object File Format")), "OFF");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.iv)").arg(QObject::tr("Inventor V2.1 ascii")), "IV");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.x3d)").arg(QObject::tr("X3D Extensible 3D")), "X3D");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.x3dz)").arg(QObject::tr("Compressed X3D")), "X3DZ");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.xhtml)").arg(QObject::tr("WebGL/X3D")), "X3DOM");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.ply)").arg(QObject::tr("Stanford Polygon")), "PLY");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.wrl *.vrml)").arg(QObject::tr("VRML V2.0")), "VRML");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.wrz)").arg(QObject::tr("Compressed VRML 2.0")), "WRZ");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.nas *.bdf)").arg(QObject::tr("Nastran")), "NAS");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.py)").arg(QObject::tr("Python module def")), "PY");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.asy)").arg(QObject::tr("Asymptote Format")), "ASY");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.3mf)").arg(QObject::tr("3D Manufacturing Format")), "3MF");
    ext << qMakePair<QString, QByteArray>(QString::fromLatin1("%1 (*.*)").arg(QObject::tr("All Files")), ""); // Undefined
    // clang-format on
    QStringList filter;
    for (const auto& it : ext) {
        filter << it.first;
    }

    QString format;
    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(),
                                                  QObject::tr("Export mesh"),
                                                  dir,
                                                  filter.join(QLatin1String(";;")),
                                                  &format);
    if (!fn.isEmpty()) {
        QFileInfo fi(fn);
        QByteArray extension = fi.suffix().toLatin1();
        for (const auto& it : ext) {
            if (it.first == format) {
                extension = it.second;
                break;
            }
        }

        MeshGui::ViewProviderMesh* vp = dynamic_cast<MeshGui::ViewProviderMesh*>(
            Gui::Application::Instance->getViewProvider(docObj));
        if (vp) {
            vp->exportMesh((const char*)fn.toUtf8(), (const char*)extension);
        }
    }
}

bool CmdMeshExport::isActive()
{
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 1;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshFromGeometry)

CmdMeshFromGeometry::CmdMeshFromGeometry()
    : Command("Mesh_FromGeometry")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Create mesh from geometry...");
    sToolTipText = QT_TR_NOOP("Create mesh from the selected geometry");
    sWhatsThis = "Mesh_FromGeometry";
    sStatusTip = QT_TR_NOOP("Create mesh from the selected geometry");
}

void CmdMeshFromGeometry::activated(int)
{
    bool ok {};
    double tol = QInputDialog::getDouble(Gui::getMainWindow(),
                                         QObject::tr("Meshing Tolerance"),
                                         QObject::tr("Enter tolerance for meshing geometry:"),
                                         0.1,
                                         0.01,
                                         10.0,
                                         2,
                                         &ok,
                                         Qt::MSWindowsFixedSizeDialogHint);
    if (!ok) {
        return;
    }

    App::Document* doc = App::GetApplication().getActiveDocument();
    std::vector<App::DocumentObject*> geo =
        Gui::Selection().getObjectsOfType(App::GeoFeature::getClassTypeId());
    for (auto it : geo) {
        if (!it->getTypeId().isDerivedFrom(Mesh::Feature::getClassTypeId())) {
            // exclude meshes
            std::map<std::string, App::Property*> Map;
            it->getPropertyMap(Map);
            Mesh::MeshObject mesh;
            for (const auto& jt : Map) {
                if (jt.first == "Shape"
                    && jt.second->getTypeId().isDerivedFrom(
                        App::PropertyComplexGeoData::getClassTypeId())) {
                    std::vector<Base::Vector3d> aPoints;
                    std::vector<Data::ComplexGeoData::Facet> aTopo;
                    const Data::ComplexGeoData* data =
                        static_cast<App::PropertyComplexGeoData*>(jt.second)->getComplexData();
                    if (data) {
                        data->getFaces(aPoints, aTopo, (float)tol);
                        mesh.setFacets(aTopo, aPoints);
                    }
                }
            }

            // create a mesh feature and assign the mesh
            Mesh::Feature* mf =
                static_cast<Mesh::Feature*>(doc->addObject("Mesh::Feature", "Mesh"));
            mf->Mesh.setValue(mesh.getKernel());
        }
    }
}

bool CmdMeshFromGeometry::isActive()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        return false;
    }
    return getSelection().countObjectsOfType(App::GeoFeature::getClassTypeId()) >= 1;
}

//===========================================================================
// Mesh_FromPart
//===========================================================================
DEF_STD_CMD_A(CmdMeshFromPartShape)

CmdMeshFromPartShape::CmdMeshFromPartShape()
    : Command("Mesh_FromPartShape")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Create mesh from shape...");
    sToolTipText = QT_TR_NOOP("Tessellate shape");
    sWhatsThis = "Mesh_FromPartShape";
    sStatusTip = sToolTipText;
    sPixmap = "Mesh_FromPartShape.svg";
}

void CmdMeshFromPartShape::activated(int)
{
    doCommand(Doc, "import MeshPartGui, FreeCADGui\nFreeCADGui.runCommand('MeshPart_Mesher')\n");
}

bool CmdMeshFromPartShape::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshVertexCurvature)

CmdMeshVertexCurvature::CmdMeshVertexCurvature()
    : Command("Mesh_VertexCurvature")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Curvature plot");
    sToolTipText = QT_TR_NOOP("Calculates the curvature of the vertices of a mesh");
    sWhatsThis = "Mesh_VertexCurvature";
    sStatusTip = QT_TR_NOOP("Calculates the curvature of the vertices of a mesh");
    sPixmap = "Mesh_VertexCurvature";
}

void CmdMeshVertexCurvature::activated(int)
{
    std::vector<App::DocumentObject*> meshes =
        getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (auto it : meshes) {
        std::string fName = it->getNameInDocument();
        fName += "_Curvature";
        fName = getUniqueObjectName(fName.c_str());

        openCommand(QT_TRANSLATE_NOOP("Command", "Mesh VertexCurvature"));
        App::DocumentObject* grp = App::DocumentObjectGroup::getGroupOfObject(it);
        if (grp) {
            doCommand(
                Doc,
                "App.activeDocument().getObject(\"%s\").newObject(\"Mesh::Curvature\",\"%s\")",
                grp->getNameInDocument(),
                fName.c_str());
        }
        else {
            doCommand(Doc,
                      "App.activeDocument().addObject(\"Mesh::Curvature\",\"%s\")",
                      fName.c_str());
        }
        doCommand(Doc,
                  "App.activeDocument().%s.Source = App.activeDocument().%s",
                  fName.c_str(),
                  it->getNameInDocument());
    }

    commitCommand();
    updateActive();
}

bool CmdMeshVertexCurvature::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshVertexCurvatureInfo)

CmdMeshVertexCurvatureInfo::CmdMeshVertexCurvatureInfo()
    : Command("Mesh_CurvatureInfo")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Curvature info");
    sToolTipText = QT_TR_NOOP("Information about curvature");
    sWhatsThis = "Mesh_CurvatureInfo";
    sStatusTip = QT_TR_NOOP("Information about curvature");
    sPixmap = "Mesh_CurvatureInfo";
}

void CmdMeshVertexCurvatureInfo::activated(int)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->setRedirectToSceneGraph(true);
        viewer->setSelectionEnabled(false);
        viewer->setEditingCursor(
            QCursor(Gui::BitmapFactory().pixmapFromSvg("Mesh_Pipette", QSize(32, 32)), 4, 29));
        viewer->addEventCallback(SoEvent::getClassTypeId(),
                                 MeshGui::ViewProviderMeshCurvature::curvatureInfoCallback);
    }
}

bool CmdMeshVertexCurvatureInfo::isActive()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType(Mesh::Curvature::getClassTypeId()) == 0) {
        return false;
    }

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshPolySegm)

CmdMeshPolySegm::CmdMeshPolySegm()
    : Command("Mesh_PolySegm")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Make segment");
    sToolTipText = QT_TR_NOOP("Creates a mesh segment");
    sWhatsThis = "Mesh_PolySegm";
    sStatusTip = QT_TR_NOOP("Creates a mesh segment");
    sPixmap = "PolygonPick";
}

void CmdMeshPolySegm::activated(int)
{
    std::vector<App::DocumentObject*> docObj =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end();
         ++it) {
        if (it == docObj.begin()) {
            Gui::Document* doc = getActiveGuiDocument();
            Gui::MDIView* view = doc->getActiveView();
            if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
                Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
                viewer->setEditing(true);
                viewer->startSelection(Gui::View3DInventorViewer::Clip);
                viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(),
                                         MeshGui::ViewProviderMeshFaceSet::segmMeshCallback);
            }
            else {
                return;
            }
        }

        Gui::ViewProvider* pVP = getActiveGuiDocument()->getViewProvider(*it);
        if (pVP->isVisible()) {
            pVP->startEditing();
        }
    }
}

bool CmdMeshPolySegm::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0) {
        return false;
    }

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}


DEF_STD_CMD_A(CmdMeshAddFacet)

CmdMeshAddFacet::CmdMeshAddFacet()
    : Command("Mesh_AddFacet")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Add triangle");
    sToolTipText = QT_TR_NOOP("Add triangle manually to a mesh");
    sWhatsThis = "Mesh_AddFacet";
    sStatusTip = QT_TR_NOOP("Add triangle manually to a mesh");
    sPixmap = "Mesh_AddFacet";
}

void CmdMeshAddFacet::activated(int)
{
    std::vector<App::DocumentObject*> docObj =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (auto it : docObj) {
        Gui::Document* doc = Gui::Application::Instance->getDocument(it->getDocument());
        Gui::MDIView* view = doc->getActiveView();
        if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
            MeshGui::MeshFaceAddition* edit =
                new MeshGui::MeshFaceAddition(static_cast<Gui::View3DInventor*>(view));
            edit->startEditing(static_cast<MeshGui::ViewProviderMesh*>(
                Gui::Application::Instance->getViewProvider(it)));
            break;
        }
    }
}

bool CmdMeshAddFacet::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) != 1) {
        return false;
    }

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshPolyCut)

CmdMeshPolyCut::CmdMeshPolyCut()
    : Command("Mesh_PolyCut")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Cut mesh");
    sToolTipText = QT_TR_NOOP("Cuts a mesh with a picked polygon");
    sWhatsThis = "Mesh_PolyCut";
    sStatusTip = QT_TR_NOOP("Cuts a mesh with a picked polygon");
    sPixmap = "Mesh_PolyCut";
}

void CmdMeshPolyCut::activated(int)
{
    std::vector<App::DocumentObject*> docObj =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end();
         ++it) {
        if (it == docObj.begin()) {
            Gui::Document* doc = getActiveGuiDocument();
            Gui::MDIView* view = doc->getActiveView();
            if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
                Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
                viewer->setEditing(true);

                Gui::PolyClipSelection* clip = new Gui::PolyClipSelection();
                clip->setRole(Gui::SelectionRole::Split, true);
                clip->setColor(0.0f, 0.0f, 1.0f);
                clip->setLineWidth(1.0f);
                viewer->navigationStyle()->startSelection(clip);
                viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(),
                                         MeshGui::ViewProviderMeshFaceSet::clipMeshCallback);
            }
            else {
                return;
            }
        }

        Gui::ViewProvider* pVP = getActiveGuiDocument()->getViewProvider(*it);
        if (pVP->isVisible()) {
            pVP->startEditing();
        }
    }
}

bool CmdMeshPolyCut::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0) {
        return false;
    }

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshPolyTrim)

CmdMeshPolyTrim::CmdMeshPolyTrim()
    : Command("Mesh_PolyTrim")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Trim mesh");
    sToolTipText = QT_TR_NOOP("Trims a mesh with a picked polygon");
    sWhatsThis = "Mesh_PolyTrim";
    sStatusTip = QT_TR_NOOP("Trims a mesh with a picked polygon");
    sPixmap = "Mesh_PolyTrim";
}

void CmdMeshPolyTrim::activated(int)
{
    std::vector<App::DocumentObject*> docObj =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end();
         ++it) {
        if (it == docObj.begin()) {
            Gui::Document* doc = getActiveGuiDocument();
            Gui::MDIView* view = doc->getActiveView();
            if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
                Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
                viewer->setEditing(true);

                Gui::PolyClipSelection* clip = new Gui::PolyClipSelection();
                clip->setRole(Gui::SelectionRole::Split, true);
                clip->setColor(0.0f, 0.0f, 1.0f);
                clip->setLineWidth(1.0f);
                viewer->navigationStyle()->startSelection(clip);
                viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(),
                                         MeshGui::ViewProviderMeshFaceSet::trimMeshCallback);
            }
            else {
                return;
            }
        }

        Gui::ViewProvider* pVP = getActiveGuiDocument()->getViewProvider(*it);
        if (pVP->isVisible()) {
            pVP->startEditing();
        }
    }
}

bool CmdMeshPolyTrim::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0) {
        return false;
    }

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshTrimByPlane)

CmdMeshTrimByPlane::CmdMeshTrimByPlane()
    : Command("Mesh_TrimByPlane")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Trim mesh with a plane");
    sToolTipText = QT_TR_NOOP("Trims a mesh with a plane");
    sStatusTip = QT_TR_NOOP("Trims a mesh with a plane");
    sPixmap = "Mesh_TrimByPlane";
}

void CmdMeshTrimByPlane::activated(int)
{
    doCommand(Doc,
              "import MeshPartGui, FreeCADGui\nFreeCADGui.runCommand('MeshPart_TrimByPlane')\n");
}

bool CmdMeshTrimByPlane::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) != 1) {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshSectionByPlane)

CmdMeshSectionByPlane::CmdMeshSectionByPlane()
    : Command("Mesh_SectionByPlane")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Create section from mesh and plane");
    sToolTipText = QT_TR_NOOP("Section from mesh and plane");
    sStatusTip = QT_TR_NOOP("Section from mesh and plane");
    sPixmap = "Mesh_SectionByPlane";
}

void CmdMeshSectionByPlane::activated(int)
{
    doCommand(Doc,
              "import MeshPartGui, FreeCADGui\nFreeCADGui.runCommand('MeshPart_SectionByPlane')\n");
}

bool CmdMeshSectionByPlane::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) != 1) {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshCrossSections)

CmdMeshCrossSections::CmdMeshCrossSections()
    : Command("Mesh_CrossSections")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Cross-sections...");
    sToolTipText = QT_TR_NOOP("Cross-sections");
    sStatusTip = QT_TR_NOOP("Cross-sections");
    sPixmap = "Mesh_CrossSections";
}

void CmdMeshCrossSections::activated(int)
{
    doCommand(Doc,
              "import MeshPartGui, FreeCADGui\nFreeCADGui.runCommand('MeshPart_CrossSections')\n");
}

bool CmdMeshCrossSections::isActive()
{
    return (Gui::Selection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0
            && !Gui::Control().activeDialog());
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshPolySplit)

CmdMeshPolySplit::CmdMeshPolySplit()
    : Command("Mesh_PolySplit")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Split mesh");
    sToolTipText = QT_TR_NOOP("Splits a mesh into two meshes");
    sWhatsThis = "Mesh_PolySplit";
    sStatusTip = QT_TR_NOOP("Splits a mesh into two meshes");
}

void CmdMeshPolySplit::activated(int)
{
    std::vector<App::DocumentObject*> docObj =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end();
         ++it) {
        if (it == docObj.begin()) {
            Gui::Document* doc = getActiveGuiDocument();
            Gui::MDIView* view = doc->getActiveView();
            if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
                Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
                viewer->setEditing(true);
                viewer->startSelection(Gui::View3DInventorViewer::Clip);
                viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(),
                                         MeshGui::ViewProviderMeshFaceSet::partMeshCallback);
            }
            else {
                return;
            }
        }

        Gui::ViewProvider* pVP = getActiveGuiDocument()->getViewProvider(*it);
        pVP->startEditing();
    }
}

bool CmdMeshPolySplit::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0) {
        return false;
    }

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshEvaluation)

CmdMeshEvaluation::CmdMeshEvaluation()
    : Command("Mesh_Evaluation")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    // needs two ampersands to display one
    sMenuText = QT_TR_NOOP("Evaluate and repair mesh...");
    sToolTipText = QT_TR_NOOP("Opens a dialog to analyze and repair a mesh");
    sWhatsThis = "Mesh_Evaluation";
    sStatusTip = QT_TR_NOOP("Opens a dialog to analyze and repair a mesh");
    sPixmap = "Mesh_Evaluation";
}

void CmdMeshEvaluation::activated(int)
{
    if (MeshGui::DockEvaluateMeshImp::hasInstance()) {
        MeshGui::DockEvaluateMeshImp::instance()->show();
        return;
    }

    MeshGui::DlgEvaluateMeshImp* dlg = MeshGui::DockEvaluateMeshImp::instance();
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    std::vector<App::DocumentObject*> meshes =
        getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (auto it : meshes) {
        dlg->setMesh((Mesh::Feature*)(it));
        break;
    }

    dlg->show();
}

bool CmdMeshEvaluation::isActive()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0) {
        return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshEvaluateFacet)

CmdMeshEvaluateFacet::CmdMeshEvaluateFacet()
    : Command("Mesh_EvaluateFacet")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Face info");
    sToolTipText = QT_TR_NOOP("Information about face");
    sWhatsThis = "Mesh_EvaluateFacet";
    sStatusTip = QT_TR_NOOP("Information about face");
    sPixmap = "Mesh_EvaluateFacet";
}

void CmdMeshEvaluateFacet::activated(int)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->setEditingCursor(
            QCursor(Gui::BitmapFactory().pixmapFromSvg("Mesh_Pipette", QSize(32, 32)), 4, 29));
        viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(),
                                 MeshGui::ViewProviderMeshFaceSet::faceInfoCallback);
    }
}

bool CmdMeshEvaluateFacet::isActive()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0) {
        return false;
    }

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshRemoveComponents)

CmdMeshRemoveComponents::CmdMeshRemoveComponents()
    : Command("Mesh_RemoveComponents")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Remove components...");
    sToolTipText = QT_TR_NOOP("Remove topologic independent components from the mesh");
    sWhatsThis = "Mesh_RemoveComponents";
    sStatusTip = QT_TR_NOOP("Remove topologic independent components from the mesh");
    sPixmap = "Mesh_RemoveComponents";
}

void CmdMeshRemoveComponents::activated(int)
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg) {
        dlg = new MeshGui::TaskRemoveComponents();
        dlg->setButtonPosition(Gui::TaskView::TaskDialog::South);
    }
    Gui::Control().showDialog(dlg);
}

bool CmdMeshRemoveComponents::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    App::Document* doc = getDocument();
    if (!(doc && doc->countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0)) {
        return false;
    }
    Gui::Document* viewDoc = Gui::Application::Instance->getDocument(doc);
    Gui::View3DInventor* view = dynamic_cast<Gui::View3DInventor*>(viewDoc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        if (viewer->isEditing()) {
            return false;
        }
    }
    if (Gui::Control().activeDialog()) {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshRemeshGmsh)

CmdMeshRemeshGmsh::CmdMeshRemeshGmsh()
    : Command("Mesh_RemeshGmsh")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Refinement...");
    sToolTipText = QT_TR_NOOP("Refine existing mesh");
    sStatusTip = QT_TR_NOOP("Refine existing mesh");
    sWhatsThis = "Mesh_RemeshGmsh";
    sPixmap = "Mesh_RemeshGmsh";
}

void CmdMeshRemeshGmsh::activated(int)
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg) {
        std::vector<Mesh::Feature*> mesh = getSelection().getObjectsOfType<Mesh::Feature>();
        if (mesh.size() != 1) {
            return;
        }
        dlg = new MeshGui::TaskRemeshGmsh(mesh.front());
    }
    Gui::Control().showDialog(dlg);
}

bool CmdMeshRemeshGmsh::isActive()
{
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 1;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshRemoveCompByHand)

CmdMeshRemoveCompByHand::CmdMeshRemoveCompByHand()
    : Command("Mesh_RemoveCompByHand")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Remove components by hand...");
    sToolTipText = QT_TR_NOOP("Mark a component to remove it from the mesh");
    sWhatsThis = "Mesh_RemoveCompByHand";
    sStatusTip = QT_TR_NOOP("Mark a component to remove it from the mesh");
    sPixmap = "Mesh_RemoveCompByHand";
}

void CmdMeshRemoveCompByHand::activated(int)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->setEditingCursor(QCursor(Qt::OpenHandCursor));
        viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(),
                                 MeshGui::ViewProviderMeshFaceSet::markPartCallback);
        viewer->setSelectionEnabled(false);
    }
}

bool CmdMeshRemoveCompByHand::isActive()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0) {
        return false;
    }

    Gui::View3DInventor* view =
        dynamic_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshEvaluateSolid)

CmdMeshEvaluateSolid::CmdMeshEvaluateSolid()
    : Command("Mesh_EvaluateSolid")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Check solid mesh");
    sToolTipText = QT_TR_NOOP("Checks whether the mesh is a solid");
    sWhatsThis = "Mesh_EvaluateSolid";
    sStatusTip = QT_TR_NOOP("Checks whether the mesh is a solid");
    sPixmap = "Mesh_EvaluateSolid";
}

void CmdMeshEvaluateSolid::activated(int)
{
    std::vector<App::DocumentObject*> meshes =
        getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (auto it : meshes) {
        Mesh::Feature* mesh = (Mesh::Feature*)(it);
        QString msg;
        if (mesh->Mesh.getValue().getKernel().HasOpenEdges()) {
            msg = QObject::tr("The mesh '%1' is not a solid.")
                      .arg(QString::fromLatin1(mesh->Label.getValue()));
        }
        else {
            msg = QObject::tr("The mesh '%1' is a solid.")
                      .arg(QString::fromLatin1(mesh->Label.getValue()));
        }
        QMessageBox::information(Gui::getMainWindow(), QObject::tr("Solid Mesh"), msg);
    }
}

bool CmdMeshEvaluateSolid::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 1;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshSmoothing)

CmdMeshSmoothing::CmdMeshSmoothing()
    : Command("Mesh_Smoothing")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Smooth...");
    sToolTipText = QT_TR_NOOP("Smooth the selected meshes");
    sWhatsThis = "Mesh_Smoothing";
    sStatusTip = QT_TR_NOOP("Smooth the selected meshes");
    sPixmap = "Mesh_Smoothing";
}

void CmdMeshSmoothing::activated(int)
{
    Gui::Control().showDialog(new MeshGui::TaskSmoothing());
}

bool CmdMeshSmoothing::isActive()
{
    if (Gui::Control().activeDialog()) {
        return false;
    }
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshDecimating)

CmdMeshDecimating::CmdMeshDecimating()
    : Command("Mesh_Decimating")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Decimation...");
    sToolTipText = QT_TR_NOOP("Decimates a mesh");
    sWhatsThis = QT_TR_NOOP("Decimates a mesh");
    sStatusTip = QT_TR_NOOP("Decimates a mesh");
    sPixmap = "Mesh_Decimating";
}

void CmdMeshDecimating::activated(int)
{
    Gui::Control().showDialog(new MeshGui::TaskDecimating());
}

bool CmdMeshDecimating::isActive()
{
#if 1
    if (Gui::Control().activeDialog()) {
        return false;
    }
#endif
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshHarmonizeNormals)

CmdMeshHarmonizeNormals::CmdMeshHarmonizeNormals()
    : Command("Mesh_HarmonizeNormals")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Harmonize normals");
    sToolTipText = QT_TR_NOOP("Harmonizes the normals of the mesh");
    sWhatsThis = "Mesh_HarmonizeNormals";
    sStatusTip = QT_TR_NOOP("Harmonizes the normals of the mesh");
    sPixmap = "Mesh_HarmonizeNormals";
}

void CmdMeshHarmonizeNormals::activated(int)
{
    std::vector<App::DocumentObject*> meshes =
        getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    openCommand(QT_TRANSLATE_NOOP("Command", "Harmonize mesh normals"));
    for (auto it : meshes) {
        doCommand(Doc,
                  "App.activeDocument().getObject(\"%s\").Mesh.harmonizeNormals()",
                  it->getNameInDocument());
    }
    commitCommand();
    updateActive();
}

bool CmdMeshHarmonizeNormals::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshFlipNormals)

CmdMeshFlipNormals::CmdMeshFlipNormals()
    : Command("Mesh_FlipNormals")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Flip normals");
    sToolTipText = QT_TR_NOOP("Flips the normals of the mesh");
    sWhatsThis = "Mesh_FlipNormals";
    sStatusTip = QT_TR_NOOP("Flips the normals of the mesh");
    sPixmap = "Mesh_FlipNormals";
}

void CmdMeshFlipNormals::activated(int)
{
    std::vector<App::DocumentObject*> meshes =
        getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    openCommand(QT_TRANSLATE_NOOP("Command", "Flip mesh normals"));
    for (auto it : meshes) {
        doCommand(Doc,
                  "App.activeDocument().getObject(\"%s\").Mesh.flipNormals()",
                  it->getNameInDocument());
    }
    commitCommand();
    updateActive();
}

bool CmdMeshFlipNormals::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshBoundingBox)

CmdMeshBoundingBox::CmdMeshBoundingBox()
    : Command("Mesh_BoundingBox")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Boundings info...");
    sToolTipText = QT_TR_NOOP("Shows the boundings of the selected mesh");
    sWhatsThis = "Mesh_BoundingBox";
    sStatusTip = QT_TR_NOOP("Shows the boundings of the selected mesh");
    sPixmap = "Mesh_BoundingBox";
}

void CmdMeshBoundingBox::activated(int)
{
    std::vector<App::DocumentObject*> meshes =
        getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (auto it : meshes) {
        const MeshCore::MeshKernel& rMesh = ((Mesh::Feature*)it)->Mesh.getValue().getKernel();
        const Base::BoundBox3f& box = rMesh.GetBoundBox();

        Base::Console().Message("Boundings: Min=<%f,%f,%f>, Max=<%f,%f,%f>\n",
                                box.MinX,
                                box.MinY,
                                box.MinZ,
                                box.MaxX,
                                box.MaxY,
                                box.MaxZ);

        QString bound = qApp->translate("Mesh_BoundingBox", "Boundings of %1:")
                            .arg(QString::fromUtf8(it->Label.getValue()));
        bound += QString::fromLatin1("\n\nMin=<%1,%2,%3>\n\nMax=<%4,%5,%6>")
                     .arg(box.MinX)
                     .arg(box.MinY)
                     .arg(box.MinZ)
                     .arg(box.MaxX)
                     .arg(box.MaxY)
                     .arg(box.MaxZ);
        QMessageBox::information(Gui::getMainWindow(), QObject::tr("Boundings"), bound);
        break;
    }
}

bool CmdMeshBoundingBox::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 1;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshBuildRegularSolid)

CmdMeshBuildRegularSolid::CmdMeshBuildRegularSolid()
    : Command("Mesh_BuildRegularSolid")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Regular solid...");
    sToolTipText = QT_TR_NOOP("Builds a regular solid");
    sWhatsThis = "Mesh_BuildRegularSolid";
    sStatusTip = QT_TR_NOOP("Builds a regular solid");
    sPixmap = "Mesh_BuildRegularSolid";
}

void CmdMeshBuildRegularSolid::activated(int)
{
    static QPointer<QDialog> dlg = nullptr;
    if (!dlg) {
        dlg = new MeshGui::DlgRegularSolidImp(Gui::getMainWindow());
    }
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

bool CmdMeshBuildRegularSolid::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    return hasActiveDocument();
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshFillupHoles)

CmdMeshFillupHoles::CmdMeshFillupHoles()
    : Command("Mesh_FillupHoles")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Fill holes...");
    sToolTipText = QT_TR_NOOP("Fill holes of the mesh");
    sWhatsThis = "Mesh_FillupHoles";
    sStatusTip = QT_TR_NOOP("Fill holes of the mesh");
    sPixmap = "Mesh_FillupHoles";
}

void CmdMeshFillupHoles::activated(int)
{
    std::vector<App::DocumentObject*> meshes =
        getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    bool ok {};
    int FillupHolesOfLength =
        QInputDialog::getInt(Gui::getMainWindow(),
                             QObject::tr("Fill holes"),
                             QObject::tr("Fill holes with maximum number of edges:"),
                             3,
                             3,
                             10000,
                             1,
                             &ok,
                             Qt::MSWindowsFixedSizeDialogHint);
    if (!ok) {
        return;
    }
    openCommand(QT_TRANSLATE_NOOP("Command", "Fill up holes"));
    for (auto mesh : meshes) {
        doCommand(Doc,
                  "App.activeDocument().getObject(\"%s\").Mesh.fillupHoles(%d)",
                  mesh->getNameInDocument(),
                  FillupHolesOfLength);
    }
    commitCommand();
    updateActive();
}

bool CmdMeshFillupHoles::isActive()
{
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshFillInteractiveHole)

CmdMeshFillInteractiveHole::CmdMeshFillInteractiveHole()
    : Command("Mesh_FillInteractiveHole")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Close hole");
    sToolTipText = QT_TR_NOOP("Close holes interactively");
    sWhatsThis = "Mesh_FillInteractiveHole";
    sStatusTip = QT_TR_NOOP("Close holes interactively");
    sPixmap = "Mesh_FillInteractiveHole";
}

void CmdMeshFillInteractiveHole::activated(int)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->setEditingCursor(QCursor(Gui::BitmapFactory().pixmap("mesh_fillhole"), 5, 5));
        viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(),
                                 MeshGui::ViewProviderMeshFaceSet::fillHoleCallback);
        viewer->setSelectionEnabled(false);
    }
}

bool CmdMeshFillInteractiveHole::isActive()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0) {
        return false;
    }

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

DEF_STD_CMD_A(CmdMeshSegmentation)

CmdMeshSegmentation::CmdMeshSegmentation()
    : Command("Mesh_Segmentation")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Create mesh segments...");
    sToolTipText = QT_TR_NOOP("Create mesh segments");
    sWhatsThis = "Mesh_Segmentation";
    sStatusTip = QT_TR_NOOP("Create mesh segments");
    sPixmap = "Mesh_Segmentation";
}

void CmdMeshSegmentation::activated(int)
{
    std::vector<App::DocumentObject*> objs =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    Mesh::Feature* mesh = static_cast<Mesh::Feature*>(objs.front());
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg) {
        dlg = new MeshGui::TaskSegmentation(mesh);
    }
    Gui::Control().showDialog(dlg);
}

bool CmdMeshSegmentation::isActive()
{
    if (Gui::Control().activeDialog()) {
        return false;
    }
    return Gui::Selection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 1;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshSegmentationBestFit)

CmdMeshSegmentationBestFit::CmdMeshSegmentationBestFit()
    : Command("Mesh_SegmentationBestFit")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Create mesh segments from best-fit surfaces...");
    sToolTipText = QT_TR_NOOP("Create mesh segments from best-fit surfaces");
    sWhatsThis = "Mesh_SegmentationBestFit";
    sStatusTip = QT_TR_NOOP("Create mesh segments from best-fit surfaces");
    sPixmap = "Mesh_SegmentationBestFit";
}

void CmdMeshSegmentationBestFit::activated(int)
{
    std::vector<App::DocumentObject*> objs =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    Mesh::Feature* mesh = static_cast<Mesh::Feature*>(objs.front());
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg) {
        dlg = new MeshGui::TaskSegmentationBestFit(mesh);
    }
    Gui::Control().showDialog(dlg);
}

bool CmdMeshSegmentationBestFit::isActive()
{
    if (Gui::Control().activeDialog()) {
        return false;
    }
    return Gui::Selection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 1;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshMerge)

CmdMeshMerge::CmdMeshMerge()
    : Command("Mesh_Merge")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Merge");
    sToolTipText = QT_TR_NOOP("Merges selected meshes into one");
    sWhatsThis = "Mesh_Merge";
    sStatusTip = sToolTipText;
    sPixmap = "Mesh_Merge";
}

void CmdMeshMerge::activated(int)
{
    App::Document* pcDoc = App::GetApplication().getActiveDocument();
    if (!pcDoc) {
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Mesh merge"));
    Mesh::Feature* pcFeature =
        static_cast<Mesh::Feature*>(pcDoc->addObject("Mesh::Feature", "Mesh"));
    Mesh::MeshObject* newMesh = pcFeature->Mesh.startEditing();
    std::vector<App::DocumentObject*> objs =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (auto obj : objs) {
        const MeshObject& mesh = static_cast<Mesh::Feature*>(obj)->Mesh.getValue();
        MeshCore::MeshKernel kernel = mesh.getKernel();
        kernel.Transform(mesh.getTransform());
        newMesh->addMesh(kernel);
    }

    pcFeature->Mesh.finishEditing();
    updateActive();
    commitCommand();
}

bool CmdMeshMerge::isActive()
{
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) >= 2;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshSplitComponents)

CmdMeshSplitComponents::CmdMeshSplitComponents()
    : Command("Mesh_SplitComponents")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Split by components");
    sToolTipText = QT_TR_NOOP("Split selected mesh into its components");
    sWhatsThis = "Mesh_SplitComponents";
    sStatusTip = sToolTipText;
    sPixmap = "Mesh_SplitComponents";
}

void CmdMeshSplitComponents::activated(int)
{
    App::Document* pcDoc = App::GetApplication().getActiveDocument();
    if (!pcDoc) {
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Mesh split"));
    std::vector<App::DocumentObject*> objs =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (auto obj : objs) {
        const MeshObject& mesh = static_cast<Mesh::Feature*>(obj)->Mesh.getValue();
        std::vector<std::vector<Mesh::FacetIndex>> comps = mesh.getComponents();

        for (const auto& comp : comps) {
            std::unique_ptr<MeshObject> kernel(mesh.meshFromSegment(comp));
            kernel->setTransform(mesh.getTransform());

            Mesh::Feature* feature =
                static_cast<Mesh::Feature*>(pcDoc->addObject("Mesh::Feature", "Component"));
            feature->Mesh.setValuePtr(kernel.release());
        }
    }

    updateActive();
    commitCommand();
}

bool CmdMeshSplitComponents::isActive()
{
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 1;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshScale)

CmdMeshScale::CmdMeshScale()
    : Command("Mesh_Scale")
{
    sAppModule = "Mesh";
    sGroup = QT_TR_NOOP("Mesh");
    sMenuText = QT_TR_NOOP("Scale...");
    sToolTipText = QT_TR_NOOP("Scale selected meshes");
    sWhatsThis = "Mesh_Scale";
    sStatusTip = sToolTipText;
    sPixmap = "Mesh_Scale";
}

void CmdMeshScale::activated(int)
{
    App::Document* pcDoc = App::GetApplication().getActiveDocument();
    if (!pcDoc) {
        return;
    }

    bool ok {};
    double factor = QInputDialog::getDouble(Gui::getMainWindow(),
                                            QObject::tr("Scaling"),
                                            QObject::tr("Enter scaling factor:"),
                                            1,
                                            0,
                                            DBL_MAX,
                                            5,
                                            &ok,
                                            Qt::MSWindowsFixedSizeDialogHint);
    if (!ok || factor == 0) {
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Mesh scale"));
    std::vector<App::DocumentObject*> objs =
        Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    Base::Matrix4D mat;
    mat.scale(factor, factor, factor);
    for (auto obj : objs) {
        MeshObject* mesh = static_cast<Mesh::Feature*>(obj)->Mesh.startEditing();
        MeshCore::MeshKernel& kernel = mesh->getKernel();
        kernel.Transform(mat);
        static_cast<Mesh::Feature*>(obj)->Mesh.finishEditing();
    }

    updateActive();
    commitCommand();
}

bool CmdMeshScale::isActive()
{
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0;
}


void CreateMeshCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdMeshImport());
    rcCmdMgr.addCommand(new CmdMeshExport());
    rcCmdMgr.addCommand(new CmdMeshVertexCurvature());
    rcCmdMgr.addCommand(new CmdMeshVertexCurvatureInfo());
    rcCmdMgr.addCommand(new CmdMeshUnion());
    rcCmdMgr.addCommand(new CmdMeshDifference());
    rcCmdMgr.addCommand(new CmdMeshIntersection());
    rcCmdMgr.addCommand(new CmdMeshPolySegm());
    rcCmdMgr.addCommand(new CmdMeshAddFacet());
    rcCmdMgr.addCommand(new CmdMeshPolyCut());
    rcCmdMgr.addCommand(new CmdMeshPolySplit());
    rcCmdMgr.addCommand(new CmdMeshPolyTrim());
    rcCmdMgr.addCommand(new CmdMeshTrimByPlane());
    rcCmdMgr.addCommand(new CmdMeshSectionByPlane());
    rcCmdMgr.addCommand(new CmdMeshCrossSections());
    rcCmdMgr.addCommand(new CmdMeshEvaluation());
    rcCmdMgr.addCommand(new CmdMeshEvaluateFacet());
    rcCmdMgr.addCommand(new CmdMeshEvaluateSolid());
    rcCmdMgr.addCommand(new CmdMeshHarmonizeNormals());
    rcCmdMgr.addCommand(new CmdMeshFlipNormals());
    rcCmdMgr.addCommand(new CmdMeshSmoothing());
    rcCmdMgr.addCommand(new CmdMeshDecimating());
    rcCmdMgr.addCommand(new CmdMeshBoundingBox());
    rcCmdMgr.addCommand(new CmdMeshBuildRegularSolid());
    rcCmdMgr.addCommand(new CmdMeshFillupHoles());
    rcCmdMgr.addCommand(new CmdMeshRemoveComponents());
    rcCmdMgr.addCommand(new CmdMeshRemeshGmsh());
    rcCmdMgr.addCommand(new CmdMeshFillInteractiveHole());
    rcCmdMgr.addCommand(new CmdMeshRemoveCompByHand());
    rcCmdMgr.addCommand(new CmdMeshFromGeometry());
    rcCmdMgr.addCommand(new CmdMeshFromPartShape());
    rcCmdMgr.addCommand(new CmdMeshSegmentation());
    rcCmdMgr.addCommand(new CmdMeshSegmentationBestFit);
    rcCmdMgr.addCommand(new CmdMeshMerge());
    rcCmdMgr.addCommand(new CmdMeshSplitComponents());
    rcCmdMgr.addCommand(new CmdMeshScale());
}
