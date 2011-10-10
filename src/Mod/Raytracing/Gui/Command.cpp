 /***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <BRep_Tool.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <GeomLProp_SLProps.hxx>
# include <Poly_Triangulation.hxx>
# include <TopoDS_Face.hxx>
# include <Inventor/SoInput.h>
# include <Inventor/nodes/SoNode.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <vector>
# include <Inventor/nodes/SoPerspectiveCamera.h>
# include <QMessageBox>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Material.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Gui/FileDialog.h>
#include <Gui/View.h>
#include <Gui/ViewProvider.h>
#include <Gui/Selection.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>

#include <Mod/Raytracing/App/RayFeature.h>
#include <Mod/Raytracing/App/RaySegment.h>
#include <Mod/Raytracing/App/RayProject.h>
#include <Mod/Part/App/PartFeature.h>
  
#include "FreeCADpov.h"


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//===========================================================================
// CmdRaytracingWriteCamera
//===========================================================================
DEF_STD_CMD_A(CmdRaytracingWriteCamera);

CmdRaytracingWriteCamera::CmdRaytracingWriteCamera()
  :Command("Raytracing_WriteCamera")
{
    sAppModule    = "Raytracing";
    sGroup        = QT_TR_NOOP("Raytracing");
    sMenuText     = QT_TR_NOOP("Export camera to povray...");
    sToolTipText  = QT_TR_NOOP("Export the camera positon of the active 3D view in PovRay format to a file");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Raytrace_Camera";
}

void CmdRaytracingWriteCamera::activated(int iMsg)
{
    const char* ppReturn=0;

    getGuiApplication()->sendMsgToActiveView("GetCamera",&ppReturn);

    Base::Console().Log("GetCamera MSG send:\n%s",ppReturn);

    SoInput in;
    in.setBuffer((void*)ppReturn,std::strlen(ppReturn));

    //if (!in.openFile(filename)) { exit(1); }

    SoNode* rootNode;
    SoDB::read(&in,rootNode);

    if (!rootNode || !rootNode->getTypeId().isDerivedFrom(SoCamera::getClassTypeId()))
        throw Base::Exception("CmdRaytracingWriteCamera::activated(): Could not read "
                              "camera information from ASCII stream....\n");

    // root-node returned from SoDB::readAll() has initial zero
    // ref-count, so reference it before we start using it to
    // avoid premature destruction.
    SoCamera * Cam = static_cast<SoCamera*>(rootNode);
    Cam->ref();

    SbRotation camrot = Cam->orientation.getValue();

    SbVec3f upvec(0, 1, 0); // init to default up vector
    camrot.multVec(upvec, upvec);

    SbVec3f lookat(0, 0, -1); // init to default view direction vector
    camrot.multVec(lookat, lookat);

    SbVec3f pos = Cam->position.getValue();
    float Dist = Cam->focalDistance.getValue();

    QStringList filter;
    filter << QObject::tr("Povray(*.pov)");
    filter << QObject::tr("All Files (*.*)");
    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(), QObject::tr("Export page"), QString(), filter.join(QLatin1String(";;")));
    if (fn.isEmpty()) 
        return;
    std::string cFullName = (const char*)fn.toUtf8();

    // building up the python string
    std::stringstream out;
    out << "Raytracing.writeCameraFile(\"" << strToPython(cFullName) << "\"," 
        << "(" << pos.getValue()[0]    <<"," << pos.getValue()[1]    <<"," << pos.getValue()[2]    <<")," 
        << "(" << lookat.getValue()[0] <<"," << lookat.getValue()[1] <<"," << lookat.getValue()[2] <<")," ;
    lookat *= Dist;
    lookat += pos;
    out << "(" << lookat.getValue()[0] <<"," << lookat.getValue()[1] <<"," << lookat.getValue()[2] <<")," 
        << "(" << upvec.getValue()[0]  <<"," << upvec.getValue()[1]  <<"," << upvec.getValue()[2]  <<") )" ;

    doCommand(Doc,"import Raytracing");
    doCommand(Gui,out.str().c_str());


    // Bring ref-count of root-node back to zero to cause the
    // destruction of the camera.
    Cam->unref();
}

bool CmdRaytracingWriteCamera::isActive(void)
{
    return getGuiApplication()->sendHasMsgToActiveView("GetCamera");
}

//===========================================================================
// CmdRaytracingWritePart
//===========================================================================
DEF_STD_CMD_A(CmdRaytracingWritePart);

CmdRaytracingWritePart::CmdRaytracingWritePart()
  :Command("Raytracing_WritePart")
{
    sAppModule    = "Raytracing";
    sGroup        = QT_TR_NOOP("Raytracing");
    sMenuText     = QT_TR_NOOP("Export part to povray...");
    sToolTipText  = QT_TR_NOOP("Write the selected Part (object) as a povray file");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Raytrace_Part";
}

void CmdRaytracingWritePart::activated(int iMsg)
{
    QStringList filter;
    filter << QObject::tr("Povray(*.pov)");
    filter << QObject::tr("All Files (*.*)");
    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(), QObject::tr("Export page"), QString(), filter.join(QLatin1String(";;")));
    if (fn.isEmpty()) 
        return;
    std::string cFullName = (const char*)fn.toUtf8();

    // name of the objects in the pov file
    std::string Name = "Part";
    std::vector<App::DocumentObject*> obj = Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (obj.empty()) return;

    std::stringstream out;
    //Raytracing.writePartFile(App.document().GetActiveFeature().getShape())
    out << "Raytracing.writePartFile(\"" << strToPython(cFullName) << "\",\""
        << Name << "\",App.ActiveDocument." << obj.front()->getNameInDocument() << ".Shape)";

    doCommand(Doc,"import Raytracing");
    doCommand(Doc,out.str().c_str());
}

bool CmdRaytracingWritePart::isActive(void)
{
    return Gui::Selection().countObjectsOfType(Part::Feature::getClassTypeId()) == 1;
}

//===========================================================================
// CmdRaytracingWriteView
//===========================================================================
DEF_STD_CMD_A(CmdRaytracingWriteView);

CmdRaytracingWriteView::CmdRaytracingWriteView()
  :Command("Raytracing_WriteView")
{
    sAppModule    = "Raytracing";
    sGroup        = QT_TR_NOOP("Raytracing");
    sMenuText     = QT_TR_NOOP("Export view to povray...");
    sToolTipText  = QT_TR_NOOP("Write the active 3D view with camera and all its content to a povray file");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Raytrace_Export";
}

void CmdRaytracingWriteView::activated(int iMsg)
{
    QStringList filter;
    filter << QObject::tr("Povray(*.pov)");
    filter << QObject::tr("All Files (*.*)");
    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(), QObject::tr("Export page"), QString(), filter.join(QLatin1String(";;")));
    if (fn.isEmpty()) 
        return;
    std::string cFullName = (const char*)fn.toUtf8();


    // get all objects of the active document
    std::vector<Part::Feature*> DocObjects = getActiveGuiDocument()->getDocument()->
        getObjectsOfType<Part::Feature>();

    openCommand("Write view");
    doCommand(Doc,"import Raytracing,RaytracingGui");
    doCommand(Doc,"OutFile = open(unicode('%s','utf-8'),'w')",cFullName.c_str());
    doCommand(Doc,"OutFile.write(open(App.getResourceDir()+'Mod/Raytracing/Templates/ProjectStd.pov').read())");
    doCommand(Doc,"OutFile.write(RaytracingGui.povViewCamera())");
    // go through all document objects
    for (std::vector<Part::Feature*>::const_iterator it=DocObjects.begin();it!=DocObjects.end();++it) {
        App::PropertyColor *pcColor = dynamic_cast<App::PropertyColor *>(getActiveGuiDocument()->getViewProvider(*it)->getPropertyByName("ShapeColor"));
        App::Color col = pcColor->getValue();
        doCommand(Doc,"OutFile.write(Raytracing.getPartAsPovray('%s',App.activeDocument().%s.Shape,%f,%f,%f))",
                 (*it)->getNameInDocument(),(*it)->getNameInDocument(),col.r,col.g,col.b);
    }

    doCommand(Doc,"OutFile.close()");
    doCommand(Doc,"del OutFile");

    updateActive();
    commitCommand();
}

bool CmdRaytracingWriteView::isActive(void)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc) {
        return doc->countObjectsOfType(Part::Feature::getClassTypeId()) > 0;
    }

    return false;
}


////===========================================================================
//// CmdRaytracingNewProject
////===========================================================================
//DEF_STD_CMD_A(CmdRaytracingNewProject);
//
//CmdRaytracingNewProject::CmdRaytracingNewProject()
//  :Command("Raytracing_NewProject")
//{
//    sAppModule    = "Raytracing";
//    sGroup        = QT_TR_NOOP("Raytracing");
//    sMenuText     = QT_TR_NOOP("New project");
//    sToolTipText  = QT_TR_NOOP("Write the initial povray file to render a part");
//    sWhatsThis    = sToolTipText;
//    sStatusTip    = sToolTipText;
//    sPixmap       = "Test1";
//    sAccel        = "Ctrl+P";
//}
//
//void CmdRaytracingNewProject::activated(int iMsg)
//{
//    // getting standard parameter
//    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Raytracing");
//    std::string cDir             = hGrp->GetASCII("ProjectPath", "");
//    // xorx: The following has to be implemented as a setting
//    std::string cPovRayName      = hGrp->GetASCII("SceneFilename", "PovrayScene.pov");
//    // HACK: This is the workaround
//    //std::string cPovRayName="PovrayScene.pov";
//
//    if (cDir!="" && cDir[cDir.size()-1] != PATHSEP)
//        cDir += PATHSEP;
//    std::string cFullName = cDir+cPovRayName;
//
//    // Open RayTracing module
//    doCommand(Doc,"import Raytracing");
//    // Get the default scene file and write it to the Project directory
//    doCommand(Doc,"Raytracing.copyResource(\"FCSimple.pov\",\"%s\")",strToPython(cFullName).c_str());
//}
//
//bool CmdRaytracingNewProject::isActive(void)
//{
//    //if( getActiveDocument() )
//        return true;
//    //else
//    //  return false;
//}
//
////===========================================================================
//// CmdRaytracingQuickRender
////===========================================================================
//DEF_STD_CMD_A(CmdRaytracingQuickRender);
//
//CmdRaytracingQuickRender::CmdRaytracingQuickRender()
//  :Command("Raytracing_QuickRender")
//{
//    sAppModule    = "Raytracing";
//    sGroup        = QT_TR_NOOP("Raytracing");
//    sMenuText     = QT_TR_NOOP("Render");
//    sToolTipText  = QT_TR_NOOP("Renders the actual view");
//    sWhatsThis    = sToolTipText;
//    sStatusTip    = sToolTipText;
//    sPixmap       = "Test1";
//    sAccel        = "Ctrl+P";
//}
//
//void CmdRaytracingQuickRender::activated(int iMsg)
//{
//    // get the preferences
//    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Raytracing");
//    std::string cDir             = hGrp->GetASCII("ProjectPath", "");
//
//    //cDir = Gui::FileDialog::getExistingDirectory(cDir.c_str()).latin1();
//
//    if (cDir!="" && cDir[cDir.size()-1] != PATHSEP)
//        cDir += PATHSEP;
//
//    std::string cFullName = cDir+"FreeCAD.pov";
//    Base::Console().Log("Using file name:%s",cFullName.c_str());
//
//    // open the file and write
//    std::ofstream fout(cFullName.c_str());
//    fout << FreeCAD ;
//    fout.close();
//}
//
//bool CmdRaytracingQuickRender::isActive(void)
//{
//    //if( getActiveDocument() )
//        return true;
//    //else
//    //  return false;
//}
//

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//===========================================================================
// Raytracing_NewPovrayProject
//===========================================================================

DEF_STD_CMD_A(CmdRaytracingNewPovrayProject);

CmdRaytracingNewPovrayProject::CmdRaytracingNewPovrayProject()
  : Command("Raytracing_NewPovrayProject")
{
    sAppModule      = "Raytracing";
    sGroup          = QT_TR_NOOP("Raytracing");
    sMenuText       = QT_TR_NOOP("New Povray project");
    sToolTipText    = QT_TR_NOOP("Insert new Povray project into the document");
    sWhatsThis      = "Raytracing_NewPovrayProject";
    sStatusTip      = sToolTipText;
    sPixmap         = "Raytrace_New";
}

void CmdRaytracingNewPovrayProject::activated(int iMsg)
{
    std::string FeatName = getUniqueObjectName("PovProject");

    openCommand("Raytracing create project");
    doCommand(Doc,"App.activeDocument().addObject('Raytracing::RayProject','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Template = App.getResourceDir()+'Mod/Raytracing/Templates/ProjectStd.pov'",FeatName.c_str());
    commitCommand();
}

bool CmdRaytracingNewPovrayProject::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}


//===========================================================================
// Raytracing_NewPartView
//===========================================================================

DEF_STD_CMD(CmdRaytracingNewPartSegment);

CmdRaytracingNewPartSegment::CmdRaytracingNewPartSegment()
  : Command("Raytracing_NewPartSegment")
{
    sAppModule      = "Raytracing";
    sGroup          = QT_TR_NOOP("Raytracing");
    sMenuText       = QT_TR_NOOP("Insert part");
    sToolTipText    = QT_TR_NOOP("Insert a new part object into a Povray project");
    sWhatsThis      = "Raytracing_NewPartSegment";
    sStatusTip      = sToolTipText;
    sPixmap         = 0;
}

void CmdRaytracingNewPartSegment::activated(int iMsg)
{
    std::vector<Part::Feature*> parts = Gui::Selection().getObjectsOfType<Part::Feature>();
    if (parts.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select a Part object."));
        return;
    }

    std::vector<App::DocumentObject*> pages = App::GetApplication().getActiveDocument()
        ->getObjectsOfType(Raytracing::RayProject::getClassTypeId());
    if (pages.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No Povray project to insert"),
            QObject::tr("Create a Povray project to insert a view."));
        return;
    }

    std::string ProjName = pages.front()->getNameInDocument();

    openCommand("Create view");
    for (std::vector<Part::Feature*>::iterator it = parts.begin(); it != parts.end(); ++it) {
        std::string FeatName = (*it)->getNameInDocument();
        FeatName += "_View";
        FeatName = getUniqueObjectName(FeatName.c_str());
        doCommand(Doc,"App.activeDocument().addObject('Raytracing::RayFeature','%s')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Source = App.activeDocument().%s",FeatName.c_str(),(*it)->getNameInDocument());
        doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)",ProjName.c_str(), FeatName.c_str());
    }
    updateActive();
    commitCommand();
}

//===========================================================================
// Raytracing_ExportProject
//===========================================================================

DEF_STD_CMD_A(CmdRaytracingExportProject);

CmdRaytracingExportProject::CmdRaytracingExportProject()
  : Command("Raytracing_ExportProject")
{
    // seting the
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&Export project...");
    sToolTipText  = QT_TR_NOOP("Export the Povray project file");
    sWhatsThis    = "Raytracing_ExportProject";
    sStatusTip    = sToolTipText;
    sPixmap       = "Raytrace_Export";
}

void CmdRaytracingExportProject::activated(int iMsg)
{
    unsigned int n = getSelection().countObjectsOfType(Raytracing::RayProject::getClassTypeId());
    if (n != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one Povray project object."));
        return;
    }

    QStringList filter;
    filter << QObject::tr("Povray(*.pov)");
    filter << QObject::tr("All Files (*.*)");

    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(), QObject::tr("Export page"), QString(), filter.join(QLatin1String(";;")));
    if (!fn.isEmpty()) {
        std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
        openCommand("Raytracing export project");

        doCommand(Doc,"PageFile = open(App.activeDocument().%s.PageResult,'r')",Sel[0].FeatName);
        std::string fname = (const char*)fn.toUtf8();
        doCommand(Doc,"OutFile = open(unicode('%s','utf-8'),'w')",fname.c_str());
        doCommand(Doc,"OutFile.write(PageFile.read())");
        doCommand(Doc,"del OutFile,PageFile");

        commitCommand();
    }
}

bool CmdRaytracingExportProject::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}


void CreateRaytracingCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdRaytracingWriteCamera());
    rcCmdMgr.addCommand(new CmdRaytracingWritePart());
    rcCmdMgr.addCommand(new CmdRaytracingWriteView());
    rcCmdMgr.addCommand(new CmdRaytracingNewPovrayProject());
    rcCmdMgr.addCommand(new CmdRaytracingExportProject());
    rcCmdMgr.addCommand(new CmdRaytracingNewPartSegment());
}
