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
# ifdef FC_OS_WIN32
# include <windows.h>
# endif
# include <qaction.h>
# include <qdir.h>
# include <qfileinfo.h>
# include <qinputdialog.h>
# include <qmessagebox.h>
# include <qstringlist.h>
//# include <gts.h>
# include <map>
#endif

#ifndef __InventorAll__
# include <Gui/InventorAll.h>
#endif

#include <Mod/Mesh/App/Core/Smoothing.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/FeatureMeshCurvature.h>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/MainWindow.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/WaitCursor.h>

#include "DlgEvaluateMeshImp.h"
#include "DlgRegularSolidImp.h"
#include "RemoveComponents.h"
#include "DlgSmoothing.h"
#include "ViewProviderMeshFaceSet.h"
#include "ViewProviderCurvature.h"
#include "MeshEditor.h"
#include "Segmentation.h"

using namespace Mesh;


DEF_STD_CMD_A(CmdMeshTransform);

CmdMeshTransform::CmdMeshTransform()
  :Command("Mesh_Transform")
{
  sAppModule    = "Mesh";
  sGroup        = QT_TR_NOOP("Mesh");
  sMenuText     = QT_TR_NOOP("Transform mesh");
  sToolTipText  = QT_TR_NOOP("Rotate or move a mesh");
  sWhatsThis    = "Mesh_Transform";
  sStatusTip    = QT_TR_NOOP("Rotate or move a mesh");
  sPixmap       = "Std_Tool1";
}

void CmdMeshTransform::activated(int iMsg)
{
  unsigned int n = getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId());
  if ( n!=1 ) return;

  std::string fName = getUniqueObjectName("Move");
  std::vector<Gui::SelectionSingleton::SelObj> cSel = getSelection().getSelection();

  openCommand("Mesh Mesh Create");
  doCommand(Doc,"App.activeDocument().addObject(\"Mesh::Transform\",\"%s\")",fName.c_str());
  doCommand(Doc,"App.activeDocument().%s.Source = App.activeDocument().%s",fName.c_str(),cSel[0].FeatName);
  doCommand(Gui,"Gui.hide(\"%s\")",cSel[0].FeatName);
  commitCommand(); 
 
  updateActive();
}

bool CmdMeshTransform::isActive(void)
{
  //return true;
  return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 1;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshDemolding);

CmdMeshDemolding::CmdMeshDemolding()
  :Command("Mesh_Demolding")
{
  sAppModule    = "Mesh";
  sGroup        = QT_TR_NOOP("Mesh");
  sMenuText     = QT_TR_NOOP("Interactive demolding direction");
  sToolTipText  = sMenuText;
  sWhatsThis    = "Mesh_Demolding";
  sStatusTip    = sMenuText;
  sPixmap       = "Std_Tool1";
}

void CmdMeshDemolding::activated(int iMsg)
{
  unsigned int n = getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId());
  if ( n!=1 ) return;

  std::string fName = getUniqueObjectName("Demolding");
  std::vector<Gui::SelectionSingleton::SelObj> cSel = getSelection().getSelection();

  openCommand("Mesh Mesh Create");
  doCommand(Doc,"App.activeDocument().addObject(\"Mesh::TransformDemolding\",\"%s\")",fName.c_str());
  doCommand(Doc,"App.activeDocument().%s.Source = App.activeDocument().%s",fName.c_str(),cSel[0].FeatName);
  doCommand(Gui,"Gui.hide(\"%s\")",cSel[0].FeatName);
  commitCommand(); 
 
  updateActive();
}

bool CmdMeshDemolding::isActive(void)
{
  //return true;
  return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 1;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshUnion);

CmdMeshUnion::CmdMeshUnion()
  :Command("Mesh_Union")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Union");
    sToolTipText  = sMenuText;
    sWhatsThis    = "Mesh_Union";
    sStatusTip    = sMenuText;
}

void CmdMeshUnion::activated(int iMsg)
{
    std::vector<App::DocumentObject*> obj = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    std::string name1 = obj.front()->getNameInDocument();
    std::string name2 = obj.back()->getNameInDocument();
    std::string name3 = getUniqueObjectName("Union");
    openCommand("Mesh Union");
    doCommand(Doc,
        "import Mesh,MeshGui\n"
        "mesh = App.ActiveDocument.%s.Mesh."
        "unite(App.ActiveDocument.%s.Mesh)\n"
        "App.activeDocument().addObject(\"Mesh::Feature\",\"%s\")\n"
        "App.activeDocument().%s.Mesh = mesh\n",
        name1.c_str(), name2.c_str(),
        name3.c_str(), name3.c_str());
 
    updateActive();
    commitCommand();
}

bool CmdMeshUnion::isActive(void)
{
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 2;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshDifference);

CmdMeshDifference::CmdMeshDifference()
  :Command("Mesh_Difference")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Difference");
    sToolTipText  = sMenuText;
    sWhatsThis    = "Mesh_Difference";
    sStatusTip    = sMenuText;
}

void CmdMeshDifference::activated(int iMsg)
{
    std::vector<App::DocumentObject*> obj = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    std::string name1 = obj.front()->getNameInDocument();
    std::string name2 = obj.back()->getNameInDocument();
    std::string name3 = getUniqueObjectName("Difference");
    openCommand("Mesh Union");
    doCommand(Doc,
        "import Mesh,MeshGui\n"
        "mesh = App.ActiveDocument.%s.Mesh."
        "difference(App.ActiveDocument.%s.Mesh)\n"
        "App.activeDocument().addObject(\"Mesh::Feature\",\"%s\")\n"
        "App.activeDocument().%s.Mesh = mesh\n",
        name1.c_str(), name2.c_str(),
        name3.c_str(), name3.c_str());

    updateActive();
    commitCommand();
}

bool CmdMeshDifference::isActive(void)
{
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 2;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshIntersection);

CmdMeshIntersection::CmdMeshIntersection()
  :Command("Mesh_Intersection")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Intersection");
    sToolTipText  = sMenuText;
    sWhatsThis    = "Mesh_Intersection";
    sStatusTip    = sMenuText;
}

void CmdMeshIntersection::activated(int iMsg)
{
    std::vector<App::DocumentObject*> obj = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    std::string name1 = obj.front()->getNameInDocument();
    std::string name2 = obj.back()->getNameInDocument();
    std::string name3 = getUniqueObjectName("Intersection");
    openCommand("Mesh Union");
    doCommand(Doc,
        "import Mesh,MeshGui\n"
        "mesh = App.ActiveDocument.%s.Mesh."
        "intersect(App.ActiveDocument.%s.Mesh)\n"
        "App.activeDocument().addObject(\"Mesh::Feature\",\"%s\")\n"
        "App.activeDocument().%s.Mesh = mesh\n",
        name1.c_str(), name2.c_str(),
        name3.c_str(), name3.c_str());

    updateActive();
    commitCommand();
}

bool CmdMeshIntersection::isActive(void)
{
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 2;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshImport);

CmdMeshImport::CmdMeshImport()
  :Command("Mesh_Import")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Import mesh...");
    sToolTipText  = QT_TR_NOOP("Imports a mesh from file");
    sWhatsThis    = "Mesh_Import";
    sStatusTip    = QT_TR_NOOP("Imports a mesh from file");
    sPixmap       = "Mesh_Import_Mesh";
}

void CmdMeshImport::activated(int iMsg)
{
    // use current path as default
    QStringList filter;
    filter << QObject::tr("All Mesh Files (*.stl *.ast *.bms *.obj *.off *.ply)");
    filter << QObject::tr("Binary STL (*.stl)");
    filter << QObject::tr("ASCII STL (*.ast)");
    filter << QObject::tr("Binary Mesh (*.bms)");
    filter << QObject::tr("Alias Mesh (*.obj)");
    filter << QObject::tr("Object File Format (*.off)");
    filter << QObject::tr("Inventor V2.1 ascii (*.iv)");
    filter << QObject::tr("Stanford Polygon (*.ply)");
    //filter << "Nastran (*.nas *.bdf)";
    filter << QObject::tr("All Files (*.*)");

    // Allow multi selection
    QStringList fn = Gui::FileDialog::getOpenFileNames(Gui::getMainWindow(),
        QObject::tr("Import mesh"), QString(), filter.join(QLatin1String(";;")));
    for (QStringList::Iterator it = fn.begin(); it != fn.end(); ++it) {
        QFileInfo fi;
        fi.setFile(*it);

        openCommand("Import Mesh");
        doCommand(Doc,"import Mesh");
        doCommand(Doc,"Mesh.insert(\"%s\")",
                 (const char*)(*it).toUtf8());
        commitCommand();
        updateActive();
    }
}

bool CmdMeshImport::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshExport);

CmdMeshExport::CmdMeshExport()
  :Command("Mesh_Export")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Export mesh...");
    sToolTipText  = QT_TR_NOOP("Exports a mesh to file");
    sWhatsThis    = "Mesh_Export";
    sStatusTip    = QT_TR_NOOP("Exports a mesh to file");
    sPixmap       = "Mesh_Export_Mesh";
}

void CmdMeshExport::activated(int iMsg)
{
    std::vector<App::DocumentObject*> docObjs = Gui::Selection().getObjectsOfType
        (Mesh::Feature::getClassTypeId());
    if (docObjs.size() != 1)
        return;

    App::DocumentObject* docObj = docObjs.front();

    QString dir = QString::fromUtf8(docObj->Label.getValue());
    QList<QPair<QString, QByteArray> > ext;
    ext << qMakePair<QString, QByteArray>(QObject::tr("Binary STL (*.stl)"), "STL");
    ext << qMakePair<QString, QByteArray>(QObject::tr("ASCII STL (*.stl)"), "AST");
    ext << qMakePair<QString, QByteArray>(QObject::tr("ASCII STL (*.ast)"), "AST");
    ext << qMakePair<QString, QByteArray>(QObject::tr("Binary Mesh (*.bms)"), "BMS");
    ext << qMakePair<QString, QByteArray>(QObject::tr("Alias Mesh (*.obj)"), "OBJ");
    ext << qMakePair<QString, QByteArray>(QObject::tr("Object File Format (*.off)"), "OFF");
    ext << qMakePair<QString, QByteArray>(QObject::tr("Inventor V2.1 ascii (*.iv)"), "IV");
    ext << qMakePair<QString, QByteArray>(QObject::tr("X3D Extensible 3D(*.x3d)"), "X3D");
    ext << qMakePair<QString, QByteArray>(QObject::tr("Standford Polygon (*.ply)"), "PLY");
    ext << qMakePair<QString, QByteArray>(QObject::tr("VRML V2.0 (*.wrl *.vrml)"), "VRML");
    ext << qMakePair<QString, QByteArray>(QObject::tr("Compressed VRML 2.0 (*.wrz)"), "WRZ");
    ext << qMakePair<QString, QByteArray>(QObject::tr("Nastran (*.nas *.bdf)"), "NAS");
    ext << qMakePair<QString, QByteArray>(QObject::tr("Python module def (*.py)"), "PY");
    ext << qMakePair<QString, QByteArray>(QObject::tr("All Files (*.*)"), ""); // Undefined
    QStringList filter;
    for (QList<QPair<QString, QByteArray> >::iterator it = ext.begin(); it != ext.end(); ++it)
        filter << it->first;

    QString format;
    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(),
        QObject::tr("Export mesh"), dir, filter.join(QLatin1String(";;")), &format);
    if (!fn.isEmpty()) {
        QFileInfo fi(fn);
        QByteArray extension = fi.suffix().toAscii();
        for (QList<QPair<QString, QByteArray> >::iterator it = ext.begin(); it != ext.end(); ++it) {
            if (it->first == format) {
                extension = it->second;
                break;
            }
        }

        //openCommand("Export Mesh");
        doCommand(Doc,"FreeCAD.ActiveDocument.getObject(\"%s\").Mesh.write(\"%s\",\"%s\")",
                 docObj->getNameInDocument(),
                 (const char*)fn.toUtf8(),
                 (const char*)extension);
        //commitCommand();
    }
}

bool CmdMeshExport::isActive(void)
{
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 1;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshFromGeometry);

CmdMeshFromGeometry::CmdMeshFromGeometry()
  :Command("Mesh_FromGeometry")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Create mesh from geometry...");
    sToolTipText  = QT_TR_NOOP("Create mesh from the selected geometry");
    sWhatsThis    = "Mesh_FromGeometry";
    sStatusTip    = QT_TR_NOOP("Create mesh from the selected geometry");
}

void CmdMeshFromGeometry::activated(int iMsg)
{
    bool ok;
    double tol = QInputDialog::getDouble(Gui::getMainWindow(), QObject::tr("Meshing Tolerance"),
        QObject::tr("Enter tolerance for meshing geometry:"), 0.1, 0.01,10.0,2,&ok);
    if (!ok)
        return;

    App::Document* doc = App::GetApplication().getActiveDocument();
    std::vector<App::DocumentObject*> geo = Gui::Selection().getObjectsOfType(App::GeoFeature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = geo.begin(); it != geo.end(); ++it) {
        if (!(*it)->getTypeId().isDerivedFrom(Mesh::Feature::getClassTypeId())) {
            // exclude meshes
            std::map<std::string, App::Property*> Map;
            (*it)->getPropertyMap(Map);
            Mesh::MeshObject mesh;
            for (std::map<std::string, App::Property*>::iterator jt = Map.begin(); jt != Map.end(); ++jt) {
                if (jt->first == "Shape" && jt->second->getTypeId().isDerivedFrom(App::PropertyComplexGeoData::getClassTypeId())) {
                    std::vector<Base::Vector3d> aPoints;
                    std::vector<Data::ComplexGeoData::Facet> aTopo;
                    static_cast<App::PropertyComplexGeoData*>(jt->second)->getFaces(aPoints, aTopo,(float)tol);
                    mesh.setFacets(aTopo, aPoints);
                }
            }

            // create a mesh feature and assign the mesh
            Mesh::Feature* mf = static_cast<Mesh::Feature*>(doc->addObject("Mesh::Feature","Mesh"));
            mf->Mesh.setValue(mesh.getKernel());
        }
    }
}

bool CmdMeshFromGeometry::isActive(void)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) return false;
    return getSelection().countObjectsOfType(App::GeoFeature::getClassTypeId()) >= 1;
}

//===========================================================================
// Mesh_FromPart
//===========================================================================
DEF_STD_CMD_A(CmdMeshFromPartShape);

CmdMeshFromPartShape::CmdMeshFromPartShape()
  : Command("Mesh_FromPartShape")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Create mesh from shape...");
    sToolTipText  = QT_TR_NOOP("Tessellate shape");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Mesh_Mesh_from_Shape.svg";
}

void CmdMeshFromPartShape::activated(int iMsg)
{
    doCommand(Doc,"import MeshPartGui, FreeCADGui\nFreeCADGui.runCommand('MeshPart_Mesher')\n");
}

bool CmdMeshFromPartShape::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshVertexCurvature);

CmdMeshVertexCurvature::CmdMeshVertexCurvature()
  : Command("Mesh_VertexCurvature")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Curvature plot");
    sToolTipText  = QT_TR_NOOP("Calculates the curvature of the vertices of a mesh");
    sWhatsThis    = "Mesh_VertexCurvature";
    sStatusTip    = QT_TR_NOOP("Calculates the curvature of the vertices of a mesh");
    sPixmap       = "Mesh_Curvature_Plot";
}

void CmdMeshVertexCurvature::activated(int iMsg)
{
    std::vector<App::DocumentObject*> meshes = getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::const_iterator it = meshes.begin(); it != meshes.end(); ++it) {
        std::string fName = (*it)->getNameInDocument();
        fName += "_Curvature";
        fName = getUniqueObjectName(fName.c_str());

        openCommand("Mesh VertexCurvature");
        App::DocumentObjectGroup* grp = App::DocumentObjectGroup::getGroupOfObject( *it );
        if (grp)
            doCommand(Doc,"App.activeDocument().getObject(\"%s\").newObject(\"Mesh::Curvature\",\"%s\")",grp->getNameInDocument(), fName.c_str());
        else
            doCommand(Doc,"App.activeDocument().addObject(\"Mesh::Curvature\",\"%s\")",fName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Source = App.activeDocument().%s",fName.c_str(),(*it)->getNameInDocument());
    }

    commitCommand();
    updateActive();
}

bool CmdMeshVertexCurvature::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshVertexCurvatureInfo);

CmdMeshVertexCurvatureInfo::CmdMeshVertexCurvatureInfo()
  :Command("Mesh_CurvatureInfo")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Curvature info");
    sToolTipText  = QT_TR_NOOP("Information about curvature");
    sWhatsThis    = "Mesh_CurvatureInfo";
    sStatusTip    = QT_TR_NOOP("Information about curvature");
}

void CmdMeshVertexCurvatureInfo::activated(int iMsg)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->setRedirectToSceneGraph(true);
        viewer->setEditingCursor(QCursor(Gui::BitmapFactory().pixmap("mesh_pipette"),4,29));
        viewer->addEventCallback(SoEvent::getClassTypeId(),
            MeshGui::ViewProviderMeshCurvature::curvatureInfoCallback);
     }
}

bool CmdMeshVertexCurvatureInfo::isActive(void)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType(Mesh::Curvature::getClassTypeId()) == 0)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshPolySegm);

CmdMeshPolySegm::CmdMeshPolySegm()
  :Command("Mesh_PolySegm")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Make segment");
    sToolTipText  = QT_TR_NOOP("Creates a mesh segment");
    sWhatsThis    = "Mesh_PolySegm";
    sStatusTip    = QT_TR_NOOP("Creates a mesh segment");
    sPixmap       = "PolygonPick";
}

void CmdMeshPolySegm::activated(int iMsg)
{
    std::vector<App::DocumentObject*> docObj = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end(); ++it) {
        if (it == docObj.begin()) {
            Gui::Document* doc = getActiveGuiDocument();
            Gui::MDIView* view = doc->getActiveView();
            if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
                Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
                viewer->setEditing(true);
                viewer->startSelection(Gui::View3DInventorViewer::Clip);
                viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), MeshGui::ViewProviderMeshFaceSet::segmMeshCallback);
            }
            else {
                return;
            }
        }

        Gui::ViewProvider* pVP = getActiveGuiDocument()->getViewProvider(*it);
        if (pVP->isVisible())
            pVP->startEditing();
    }
}

bool CmdMeshPolySegm::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshPolySelect);

CmdMeshPolySelect::CmdMeshPolySelect()
  : Command("Mesh_PolySelect")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Select mesh");
    sToolTipText  = QT_TR_NOOP("Select an area of the mesh");
    sWhatsThis    = "Mesh_PolySelect";
    sStatusTip    = QT_TR_NOOP("Select an area of the mesh");
}

void CmdMeshPolySelect::activated(int iMsg)
{
    std::vector<App::DocumentObject*> docObj = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end(); ++it) {
        if (it == docObj.begin()) {
            Gui::Document* doc = getActiveGuiDocument();
            Gui::MDIView* view = doc->getActiveView();
            if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
                Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
                viewer->setEditing(true);
                viewer->startSelection(Gui::View3DInventorViewer::Rectangle);
                viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), MeshGui::ViewProviderMeshFaceSet::selectGLCallback);
            }
            else {
                return;
            }
        }

        Gui::ViewProvider* pVP = getActiveGuiDocument()->getViewProvider(*it);
        pVP->startEditing();
    }
}

bool CmdMeshPolySelect::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshAddFacet);

CmdMeshAddFacet::CmdMeshAddFacet()
  : Command("Mesh_AddFacet")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Add triangle");
    sToolTipText  = QT_TR_NOOP("Add triangle manually to a mesh");
    sWhatsThis    = "Mesh_AddFacet";
    sStatusTip    = QT_TR_NOOP("Add triangle manually to a mesh");
}

void CmdMeshAddFacet::activated(int iMsg)
{
    std::vector<App::DocumentObject*> docObj = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end(); ++it) {
        Gui::Document* doc = Gui::Application::Instance->getDocument((*it)->getDocument());
        Gui::MDIView* view = doc->getActiveView();
        if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
            MeshGui::MeshFaceAddition* edit = new MeshGui::MeshFaceAddition
                (static_cast<Gui::View3DInventor*>(view));
            edit->startEditing(static_cast<MeshGui::ViewProviderMesh*>
                (Gui::Application::Instance->getViewProvider(*it)));
            break;
        }
    }
}

bool CmdMeshAddFacet::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) != 1)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshPolyCut);

CmdMeshPolyCut::CmdMeshPolyCut()
  : Command("Mesh_PolyCut")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Cut mesh");
    sToolTipText  = QT_TR_NOOP("Cuts a mesh with a picked polygon");
    sWhatsThis    = "Mesh_PolyCut";
    sStatusTip    = QT_TR_NOOP("Cuts a mesh with a picked polygon");
    sPixmap       = "mesh_cut";
}

void CmdMeshPolyCut::activated(int iMsg)
{
    std::vector<App::DocumentObject*> docObj = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end(); ++it) {
        if (it == docObj.begin()) {
            Gui::Document* doc = getActiveGuiDocument();
            Gui::MDIView* view = doc->getActiveView();
            if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
                Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
                viewer->setEditing(true);
                viewer->startSelection(Gui::View3DInventorViewer::Clip);
                viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), MeshGui::ViewProviderMeshFaceSet::clipMeshCallback);
            }
            else {
                return;
            }
        }

        Gui::ViewProvider* pVP = getActiveGuiDocument()->getViewProvider(*it);
        if (pVP->isVisible())
            pVP->startEditing();
    }
}

bool CmdMeshPolyCut::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshPolyTrim);

CmdMeshPolyTrim::CmdMeshPolyTrim()
  : Command("Mesh_PolyTrim")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Trim mesh");
    sToolTipText  = QT_TR_NOOP("Trims a mesh with a picked polygon");
    sWhatsThis    = "Mesh_PolyTrim";
    sStatusTip    = QT_TR_NOOP("Trims a mesh with a picked polygon");
}

void CmdMeshPolyTrim::activated(int iMsg)
{
    std::vector<App::DocumentObject*> docObj = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end(); ++it) {
        if (it == docObj.begin()) {
            Gui::Document* doc = getActiveGuiDocument();
            Gui::MDIView* view = doc->getActiveView();
            if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
                Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
                viewer->setEditing(true);
                viewer->startSelection(Gui::View3DInventorViewer::Clip);
                viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(),
                    MeshGui::ViewProviderMeshFaceSet::trimMeshCallback);
            }
            else {
                return;
            }
        }

        Gui::ViewProvider* pVP = getActiveGuiDocument()->getViewProvider(*it);
        if (pVP->isVisible())
            pVP->startEditing();
    }
}

bool CmdMeshPolyTrim::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshTrimByPlane);

CmdMeshTrimByPlane::CmdMeshTrimByPlane()
  : Command("Mesh_TrimByPlane")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Trim mesh with a plane");
    sToolTipText  = QT_TR_NOOP("Trims a mesh with a plane");
    sStatusTip    = QT_TR_NOOP("Trims a mesh with a plane");
}

void CmdMeshTrimByPlane::activated(int iMsg)
{
    Base::Type partType = Base::Type::fromName("Part::Plane");
    std::vector<App::DocumentObject*> plane = getSelection().getObjectsOfType(partType);
    if (plane.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("Mesh_TrimByPlane", "Select plane"),
            qApp->translate("Mesh_TrimByPlane", "Please select a plane at which you trim the mesh."));
        return;
    }

    Base::Placement plm = static_cast<App::GeoFeature*>(plane.front())->Placement.getValue();
    Base::Vector3d normal(0,0,1);
    plm.getRotation().multVec(normal, normal);
    Base::Vector3d view;
    if (normal == Base::Vector3d(0,0,1)) {
        view.Set(0,1,0);
    }
    else {
        Base::Vector3d dir(0,0,1);
        view = normal % dir;
    }

    Base::Vector3d base = plm.getPosition();
    Base::Vector3d up = normal % view;

    Base::Rotation rot(Base::Vector3d(0,0,1), view);
    Base::Matrix4D mat;
    rot.getValue(mat);
    Base::ViewProjMatrix proj(mat);

    openCommand("Trim with plane");
    std::vector<App::DocumentObject*> docObj = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end(); ++it) {
        Mesh::MeshObject* mesh = static_cast<Mesh::Feature*>(*it)->Mesh.startEditing();
        Base::BoundBox3d bbox = mesh->getBoundBox();
        double len = bbox.CalcDiagonalLength();
        // project center of bbox onto plane and use this as base point
        Base::Vector3d cnt = bbox.CalcCenter();
        double dist = (cnt-base)*normal;
        base = cnt - normal * dist;

        Base::Vector3d p1 = base + up * len;
        Base::Vector3d p2 = base - up * len;
        Base::Vector3d p3 = p2 + normal * len;
        Base::Vector3d p4 = p1 + normal * len;
        p1 = mat * p1;
        p2 = mat * p2;
        p3 = mat * p3;
        p4 = mat * p4;

        Base::Polygon2D polygon2d;
        polygon2d.Add(Base::Vector2D(p1.x, p1.y));
        polygon2d.Add(Base::Vector2D(p2.x, p2.y));
        polygon2d.Add(Base::Vector2D(p3.x, p3.y));
        polygon2d.Add(Base::Vector2D(p4.x, p4.y));

        Mesh::MeshObject::CutType type = Mesh::MeshObject::INNER;
        mesh->trim(polygon2d, proj, type);
        static_cast<Mesh::Feature*>(*it)->Mesh.finishEditing();
        (*it)->purgeTouched();
    }
    commitCommand();
}

bool CmdMeshTrimByPlane::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) != 1)
        return false;

    return true;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshPolySplit);

CmdMeshPolySplit::CmdMeshPolySplit()
  : Command("Mesh_PolySplit")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Split mesh");
    sToolTipText  = QT_TR_NOOP("Splits a mesh into two meshes");
    sWhatsThis    = "Mesh_PolySplit";
    sStatusTip    = QT_TR_NOOP("Splits a mesh into two meshes");
}

void CmdMeshPolySplit::activated(int iMsg)
{
    std::vector<App::DocumentObject*> docObj = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end(); ++it) {
        if (it == docObj.begin()) {
            Gui::Document* doc = getActiveGuiDocument();
            Gui::MDIView* view = doc->getActiveView();
            if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
                Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
                viewer->setEditing(true);
                viewer->startSelection(Gui::View3DInventorViewer::Clip);
                viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), MeshGui::ViewProviderMeshFaceSet::partMeshCallback);
            }
            else {
                return;
            }
        }

        Gui::ViewProvider* pVP = getActiveGuiDocument()->getViewProvider(*it);
        pVP->startEditing();
    }
}

bool CmdMeshPolySplit::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshToolMesh);

CmdMeshToolMesh::CmdMeshToolMesh()
  :Command("Mesh_ToolMesh")
{
  sAppModule    = "Mesh";
  sGroup        = QT_TR_NOOP("Mesh");
  sMenuText     = QT_TR_NOOP("Segment by tool mesh");
  sToolTipText  = QT_TR_NOOP("Creates a segment from a given tool mesh");
  sWhatsThis    = "Mesh_ToolMesh";
  sStatusTip    = QT_TR_NOOP("Creates a segment from a given tool mesh");
}

void CmdMeshToolMesh::activated(int iMsg)
{
  std::vector<App::DocumentObject*> fea = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
  if ( fea.size() == 2 )
  {
    std::string fName = getUniqueObjectName("MeshSegment");
    App::DocumentObject* mesh = fea.front();
    App::DocumentObject* tool = fea.back();

    openCommand("Segment by tool mesh");
    doCommand(Doc, "import Mesh");
    doCommand(Gui, "import MeshGui");
    doCommand(Doc,
      "App.activeDocument().addObject(\"Mesh::SegmentByMesh\",\"%s\")\n"
      "App.activeDocument().%s.Source = App.activeDocument().%s\n"
      "App.activeDocument().%s.Tool = App.activeDocument().%s\n",
      fName.c_str(), fName.c_str(),  mesh->getNameInDocument(), fName.c_str(), tool->getNameInDocument() );

    commitCommand();
    updateActive();

    App::Document* pDoc = getDocument();
    App::DocumentObject * pObj = pDoc->getObject( fName.c_str() );

    if ( pObj )
    {
      doCommand(Gui,"Gui.hide(\"%s\")", mesh->getNameInDocument());
      doCommand(Gui,"Gui.hide(\"%s\")", tool->getNameInDocument());
      getSelection().clearSelection();
    }
  }
}

bool CmdMeshToolMesh::isActive(void)
{
  // Check for the selected mesh feature (all Mesh types)
  return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 2;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshEvaluation);

CmdMeshEvaluation::CmdMeshEvaluation()
  :Command("Mesh_Evaluation")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    // needs two ampersands to display one
    sMenuText     = QT_TR_NOOP("Evaluate && Repair mesh...");
    sToolTipText  = QT_TR_NOOP("Opens a dialog to analyze and repair a mesh");
    sWhatsThis    = "Mesh_Evaluation";
    sStatusTip    = QT_TR_NOOP("Opens a dialog to analyze and repair a mesh");
}

void CmdMeshEvaluation::activated(int iMsg)
{
    if (MeshGui::DockEvaluateMeshImp::hasInstance()) {
        MeshGui::DockEvaluateMeshImp::instance()->show();
        return;
    }

    MeshGui::DlgEvaluateMeshImp* dlg = MeshGui::DockEvaluateMeshImp::instance();
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    std::vector<App::DocumentObject*> meshes = getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::const_iterator it = meshes.begin(); it != meshes.end(); ++it) {
        dlg->setMesh((Mesh::Feature*)(*it));
        break;
    }

    dlg->show();
}

bool CmdMeshEvaluation::isActive(void)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0)
        return false;
    return true;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshEvaluateFacet);

CmdMeshEvaluateFacet::CmdMeshEvaluateFacet()
  :Command("Mesh_EvaluateFacet")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Face info");
    sToolTipText  = QT_TR_NOOP("Information about face");
    sWhatsThis    = "Mesh_EvaluateFacet";
    sStatusTip    = QT_TR_NOOP("Information about face");
    sPixmap       = "mesh_pipette";
}

void CmdMeshEvaluateFacet::activated(int iMsg)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->setEditingCursor(QCursor(Gui::BitmapFactory().pixmap("mesh_pipette"),4,29));
        viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), MeshGui::ViewProviderMeshFaceSet::faceInfoCallback);
     }
}

bool CmdMeshEvaluateFacet::isActive(void)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshRemoveComponents);

CmdMeshRemoveComponents::CmdMeshRemoveComponents()
  : Command("Mesh_RemoveComponents")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Remove components...");
    sToolTipText  = QT_TR_NOOP("Remove topologic independent components from the mesh");
    sWhatsThis    = "Mesh_RemoveComponents";
    sStatusTip    = QT_TR_NOOP("Remove topologic independent components from the mesh");
    sPixmap       = "Mesh_Remove_Components";
}

void CmdMeshRemoveComponents::activated(int iMsg)
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg) {
        dlg = new MeshGui::TaskRemoveComponents();
        dlg->setButtonPosition(Gui::TaskView::TaskDialog::South);
    }
    Gui::Control().showDialog(dlg);
}

bool CmdMeshRemoveComponents::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    App::Document* doc = getDocument();
    if (!(doc && doc->countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0))
        return false;
    Gui::Document* viewDoc = Gui::Application::Instance->getDocument(doc);
    Gui::View3DInventor* view = dynamic_cast<Gui::View3DInventor*>(viewDoc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        if (viewer->isEditing())
            return false;
    }
    if (Gui::Control().activeDialog())
        return false;

    return true;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshRemoveCompByHand);

CmdMeshRemoveCompByHand::CmdMeshRemoveCompByHand()
  :Command("Mesh_RemoveCompByHand")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Remove components by hand...");
    sToolTipText  = QT_TR_NOOP("Mark a component to remove it from the mesh");
    sWhatsThis    = "Mesh_RemoveCompByHand";
    sStatusTip    = QT_TR_NOOP("Mark a component to remove it from the mesh");
}

void CmdMeshRemoveCompByHand::activated(int iMsg)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->setEditingCursor(QCursor(Qt::OpenHandCursor));
        viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), MeshGui::ViewProviderMeshFaceSet::markPartCallback);
    }
}

bool CmdMeshRemoveCompByHand::isActive(void)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0)
        return false;

    Gui::View3DInventor* view = dynamic_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshEvaluateSolid);

CmdMeshEvaluateSolid::CmdMeshEvaluateSolid()
  :Command("Mesh_EvaluateSolid")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Check solid mesh");
    sToolTipText  = QT_TR_NOOP("Checks whether the mesh is a solid");
    sWhatsThis    = "Mesh_EvaluateSolid";
    sStatusTip    = QT_TR_NOOP("Checks whether the mesh is a solid");
}

void CmdMeshEvaluateSolid::activated(int iMsg)
{
    std::vector<App::DocumentObject*> meshes = getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::const_iterator it = meshes.begin(); it != meshes.end(); ++it) {
        Mesh::Feature* mesh = (Mesh::Feature*)(*it);
        QString msg;
        if (mesh->Mesh.getValue().getKernel().HasOpenEdges())
            msg = QObject::tr("The mesh '%1' is not a solid.")
                .arg(QString::fromAscii(mesh->Label.getValue()));
        else
            msg = QObject::tr("The mesh '%1' is a solid.")
                .arg(QString::fromAscii(mesh->Label.getValue()));
        QMessageBox::information(Gui::getMainWindow(), QObject::tr("Solid Mesh"), msg);
    }
}

bool CmdMeshEvaluateSolid::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 1;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshSmoothing);

CmdMeshSmoothing::CmdMeshSmoothing()
  :Command("Mesh_Smoothing")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Smooth...");
    sToolTipText  = QT_TR_NOOP("Smooth the selected meshes");
    sWhatsThis    = "Mesh_Smoothing";
    sStatusTip    = QT_TR_NOOP("Smooth the selected meshes");
}

void CmdMeshSmoothing::activated(int iMsg)
{
#if 0
    MeshGui::SmoothingDialog dlg(Gui::getMainWindow());
    if (dlg.exec() == QDialog::Accepted) {
        Gui::WaitCursor wc;
        openCommand("Mesh Smoothing");
        std::vector<App::DocumentObject*> meshes = getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
        for (std::vector<App::DocumentObject*>::const_iterator it = meshes.begin(); it != meshes.end(); ++it) {
            Mesh::Feature* mesh = (Mesh::Feature*)*it;
            Mesh::MeshObject* mm = mesh->Mesh.startEditing();
            switch (dlg.method()) {
                case MeshGui::DlgSmoothing::Taubin:
                    {
                        MeshCore::TaubinSmoothing s(mm->getKernel());
                        s.SetLambda(dlg.lambdaStep());
                        s.SetMicro(dlg.microStep());
                        s.Smooth(dlg.iterations());
                    }   break;
                case MeshGui::DlgSmoothing::Laplace:
                    {
                        MeshCore::LaplaceSmoothing s(mm->getKernel());
                        s.SetLambda(dlg.lambdaStep());
                        s.Smooth(dlg.iterations());
                    }   break;
                default:
                    break;
            }
            mesh->Mesh.finishEditing();
        }
        commitCommand();
    }
#else
    Gui::Control().showDialog(new MeshGui::TaskSmoothing());
#endif
}

bool CmdMeshSmoothing::isActive(void)
{
#if 1
    if (Gui::Control().activeDialog())
        return false;
#endif
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshHarmonizeNormals);

CmdMeshHarmonizeNormals::CmdMeshHarmonizeNormals()
  :Command("Mesh_HarmonizeNormals")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Harmonize normals");
    sToolTipText  = QT_TR_NOOP("Harmonizes the normals of the mesh");
    sWhatsThis    = "Mesh_HarmonizeNormals";
    sStatusTip    = QT_TR_NOOP("Harmonizes the normals of the mesh");
    sPixmap       = "Mesh_Harmonize_Normals";
}

void CmdMeshHarmonizeNormals::activated(int iMsg)
{
    std::vector<App::DocumentObject*> meshes = getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    openCommand("Harmonize mesh normals");
    for (std::vector<App::DocumentObject*>::const_iterator it = meshes.begin(); it != meshes.end(); ++it) {
        doCommand(Doc,"App.activeDocument().getObject(\"%s\").Mesh.harmonizeNormals()"
                     ,(*it)->getNameInDocument());
    }
    commitCommand();
    updateActive();
}

bool CmdMeshHarmonizeNormals::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshFlipNormals);

CmdMeshFlipNormals::CmdMeshFlipNormals()
  :Command("Mesh_FlipNormals")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Flip normals");
    sToolTipText  = QT_TR_NOOP("Flips the normals of the mesh");
    sWhatsThis    = "Mesh_FlipNormals";
    sStatusTip    = QT_TR_NOOP("Flips the normals of the mesh");
    sPixmap       = "Mesh_Flip_Normals";
}

void CmdMeshFlipNormals::activated(int iMsg)
{
    std::vector<App::DocumentObject*> meshes = getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    openCommand("Flip mesh normals");
    for (std::vector<App::DocumentObject*>::const_iterator it = meshes.begin(); it != meshes.end(); ++it) {
        doCommand(Doc,"App.activeDocument().getObject(\"%s\").Mesh.flipNormals()"
                     ,(*it)->getNameInDocument());
    }
    commitCommand();
    updateActive();
}

bool CmdMeshFlipNormals::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshBoundingBox);

CmdMeshBoundingBox::CmdMeshBoundingBox()
  :Command("Mesh_BoundingBox")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Boundings info...");
    sToolTipText  = QT_TR_NOOP("Shows the boundings of the selected mesh");
    sWhatsThis    = "Mesh_BoundingBox";
    sStatusTip    = QT_TR_NOOP("Shows the boundings of the selected mesh");
}

void CmdMeshBoundingBox::activated(int iMsg)
{
    std::vector<App::DocumentObject*> meshes = getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    for (std::vector<App::DocumentObject*>::const_iterator it = meshes.begin(); it != meshes.end(); ++it) {
        const MeshCore::MeshKernel& rMesh = ((Mesh::Feature*)(*it))->Mesh.getValue().getKernel();
        const Base::BoundBox3f& box = rMesh.GetBoundBox();

        Base::Console().Message("Boundings: Min=<%f,%f,%f>, Max=<%f,%f,%f>\n",
                                box.MinX,box.MinY,box.MinZ,box.MaxX,box.MaxY,box.MaxZ);

        QString bound = QObject::tr("Min=<%1,%2,%3>\n\nMax=<%4,%5,%6>")
            .arg(box.MinX).arg(box.MinY).arg(box.MinZ)
            .arg(box.MaxX).arg(box.MaxY).arg(box.MaxZ);
        QMessageBox::information(Gui::getMainWindow(), QObject::tr("Boundings"), bound);
        break;
    }
}

bool CmdMeshBoundingBox::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) == 1;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshBuildRegularSolid);

CmdMeshBuildRegularSolid::CmdMeshBuildRegularSolid()
  :Command("Mesh_BuildRegularSolid")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Regular solid...");
    sToolTipText  = QT_TR_NOOP("Builds a regular solid");
    sWhatsThis    = "Mesh_BuildRegularSolid";
    sStatusTip    = QT_TR_NOOP("Builds a regular solid");
    sPixmap       = "Mesh_Regular_Solid";
}

void CmdMeshBuildRegularSolid::activated(int iMsg)
{
    MeshGui::SingleDlgRegularSolidImp::instance()->show();
}

bool CmdMeshBuildRegularSolid::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    return (!MeshGui::SingleDlgRegularSolidImp::hasInstance())&&hasActiveDocument();
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshFillupHoles);

CmdMeshFillupHoles::CmdMeshFillupHoles()
  :Command("Mesh_FillupHoles")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Fill holes...");
    sToolTipText  = QT_TR_NOOP("Fill holes of the mesh");
    sWhatsThis    = "Mesh_FillupHoles";
    sStatusTip    = QT_TR_NOOP("Fill holes of the mesh");
}

void CmdMeshFillupHoles::activated(int iMsg)
{
    std::vector<App::DocumentObject*> meshes = getSelection().getObjectsOfType(Mesh::Feature::getClassTypeId());
    bool ok;
    int FillupHolesOfLength = QInputDialog::getInteger(Gui::getMainWindow(), QObject::tr("Fill holes"),
                                QObject::tr("Fill holes with maximum number of edges:"), 3, 3, 10000, 1, &ok);
    if (!ok) return;
    openCommand("Fill up holes");
    for (std::vector<App::DocumentObject*>::const_iterator it = meshes.begin(); it != meshes.end(); ++it) {
        doCommand(Doc,"App.activeDocument().getObject(\"%s\").Mesh.fillupHoles(%d)"
                     ,(*it)->getNameInDocument(), FillupHolesOfLength);
    }
    commitCommand();
    updateActive();
}

bool CmdMeshFillupHoles::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    return getSelection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0;
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdMeshFillInteractiveHole);

CmdMeshFillInteractiveHole::CmdMeshFillInteractiveHole()
  :Command("Mesh_FillInteractiveHole")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Close hole");
    sToolTipText  = QT_TR_NOOP("Close holes interactively");
    sWhatsThis    = "Mesh_FillInteractiveHole";
    sStatusTip    = QT_TR_NOOP("Close holes interactively");
    sPixmap       = "mesh_boundary";
}

void CmdMeshFillInteractiveHole::activated(int iMsg)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->setEditingCursor(QCursor(Gui::BitmapFactory().pixmap("mesh_fillhole"),5,5));
        viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), MeshGui::ViewProviderMeshFaceSet::fillHoleCallback);
     }
}

bool CmdMeshFillInteractiveHole::isActive(void)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType(Mesh::Feature::getClassTypeId()) == 0)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

DEF_STD_CMD_A(CmdMeshSegmentation);

CmdMeshSegmentation::CmdMeshSegmentation()
  : Command("Mesh_Segmentation")
{
    sAppModule    = "Mesh";
    sGroup        = QT_TR_NOOP("Mesh");
    sMenuText     = QT_TR_NOOP("Create mesh segments...");
    sToolTipText  = QT_TR_NOOP("Create mesh segments");
    sWhatsThis    = "Mesh_Segmentation";
    sStatusTip    = QT_TR_NOOP("Create mesh segments");
}

void CmdMeshSegmentation::activated(int iMsg)
{
    std::vector<App::DocumentObject*> objs = Gui::Selection().getObjectsOfType
        (Mesh::Feature::getClassTypeId());
    Mesh::Feature* mesh = static_cast<Mesh::Feature*>(objs.front());
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg) {
        dlg = new MeshGui::TaskSegmentation(mesh);
    }
    Gui::Control().showDialog(dlg);
}

bool CmdMeshSegmentation::isActive(void)
{
    if (Gui::Control().activeDialog())
        return false;
    return Gui::Selection().countObjectsOfType
        (Mesh::Feature::getClassTypeId()) == 1;
}

void CreateMeshCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdMeshImport());
    rcCmdMgr.addCommand(new CmdMeshExport());
    rcCmdMgr.addCommand(new CmdMeshVertexCurvature());
    rcCmdMgr.addCommand(new CmdMeshVertexCurvatureInfo());
    rcCmdMgr.addCommand(new CmdMeshUnion());
    rcCmdMgr.addCommand(new CmdMeshDifference());
    rcCmdMgr.addCommand(new CmdMeshIntersection());
    rcCmdMgr.addCommand(new CmdMeshDemolding());
    rcCmdMgr.addCommand(new CmdMeshPolySegm());
    rcCmdMgr.addCommand(new CmdMeshPolySelect());
    rcCmdMgr.addCommand(new CmdMeshAddFacet());
    rcCmdMgr.addCommand(new CmdMeshPolyCut());
    rcCmdMgr.addCommand(new CmdMeshPolySplit());
    rcCmdMgr.addCommand(new CmdMeshPolyTrim());
    rcCmdMgr.addCommand(new CmdMeshTrimByPlane());
    rcCmdMgr.addCommand(new CmdMeshToolMesh());
    rcCmdMgr.addCommand(new CmdMeshTransform());
    rcCmdMgr.addCommand(new CmdMeshEvaluation());
    rcCmdMgr.addCommand(new CmdMeshEvaluateFacet());
    rcCmdMgr.addCommand(new CmdMeshEvaluateSolid());
    rcCmdMgr.addCommand(new CmdMeshHarmonizeNormals());
    rcCmdMgr.addCommand(new CmdMeshFlipNormals());
    rcCmdMgr.addCommand(new CmdMeshSmoothing());
    rcCmdMgr.addCommand(new CmdMeshBoundingBox());
    rcCmdMgr.addCommand(new CmdMeshBuildRegularSolid());
    rcCmdMgr.addCommand(new CmdMeshFillupHoles());
    rcCmdMgr.addCommand(new CmdMeshRemoveComponents());
    rcCmdMgr.addCommand(new CmdMeshFillInteractiveHole());
    rcCmdMgr.addCommand(new CmdMeshRemoveCompByHand());
    rcCmdMgr.addCommand(new CmdMeshFromGeometry());
    rcCmdMgr.addCommand(new CmdMeshFromPartShape());
    rcCmdMgr.addCommand(new CmdMeshSegmentation());
}
