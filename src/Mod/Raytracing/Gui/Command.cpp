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
# include <QApplication>
# include <QFile>
# include <QFileInfo>
# include <QMessageBox>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Material.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
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
#include <Mod/Raytracing/App/LuxProject.h>
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
    if (ppReturn) {
        std::string str(ppReturn);
        if (str.find("PerspectiveCamera") == std::string::npos) {
            int ret = QMessageBox::warning(Gui::getMainWindow(), 
                qApp->translate("CmdRaytracingWriteView","No perspective camera"),
                qApp->translate("CmdRaytracingWriteView","The current view camera is not perspective"
                                " and thus the result of the povray image later might look different to"
                                " what you expect.\nDo you want to continue?"),
                QMessageBox::Yes|QMessageBox::No);
            if (ret != QMessageBox::Yes)
                return;
        }
    }

    SoInput in;
    in.setBuffer((void*)ppReturn,std::strlen(ppReturn));

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
    const char* ppReturn=0;
    Gui::Application::Instance->sendMsgToActiveView("GetCamera",&ppReturn);
    if (ppReturn) {
        std::string str(ppReturn);
        if (str.find("PerspectiveCamera") == std::string::npos) {
            int ret = QMessageBox::warning(Gui::getMainWindow(), 
                qApp->translate("CmdRaytracingWriteView","No perspective camera"),
                qApp->translate("CmdRaytracingWriteView","The current view camera is not perspective"
                                " and thus the result of the povray image later might look different to"
                                " what you expect.\nDo you want to continue?"),
                QMessageBox::Yes|QMessageBox::No);
            if (ret != QMessageBox::Yes)
                return;
        }
    }

    QStringList filter;
    filter << QObject::tr("Povray(*.pov)");
    filter << QObject::tr("All Files (*.*)");
    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(),
        QObject::tr("Export page"), QString(), filter.join(QLatin1String(";;")));
    if (fn.isEmpty()) 
        return;
    std::string cFullName = (const char*)fn.toUtf8();


    // get all objects of the active document
    std::vector<Part::Feature*> DocObjects = getActiveGuiDocument()->getDocument()->
        getObjectsOfType<Part::Feature>();

    openCommand("Write view");
    doCommand(Doc,"import Raytracing,RaytracingGui");
    doCommand(Doc,"OutFile = open(unicode('%s','utf-8'),'w')",cFullName.c_str());
    doCommand(Doc,"result = open(App.getResourceDir()+'Mod/Raytracing/Templates/ProjectStd.pov').read()");
    doCommand(Doc,"content = ''");
    doCommand(Doc,"content += RaytracingGui.povViewCamera()");
    // go through all document objects
    for (std::vector<Part::Feature*>::const_iterator it=DocObjects.begin();it!=DocObjects.end();++it) {
        Gui::ViewProvider* vp = getActiveGuiDocument()->getViewProvider(*it);
        if (vp && vp->isVisible()) {
            App::PropertyColor *pcColor = dynamic_cast<App::PropertyColor *>(vp->getPropertyByName("ShapeColor"));
            App::Color col = pcColor->getValue();
            doCommand(Doc,"content += Raytracing.getPartAsPovray('%s',App.activeDocument().%s.Shape,%f,%f,%f)",
                     (*it)->getNameInDocument(),(*it)->getNameInDocument(),col.r,col.g,col.b);
        }
    }
    doCommand(Doc,"result = result.replace('//RaytracingContent',content)");
    doCommand(Doc,"OutFile.write(result)");
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


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//===========================================================================
// Raytracing_NewPovrayProject
//===========================================================================

DEF_STD_CMD_AC(CmdRaytracingNewPovrayProject);

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
    const char* ppReturn=0;
    Gui::Application::Instance->sendMsgToActiveView("GetCamera",&ppReturn);
    if (ppReturn) {
        std::string str(ppReturn);
        if (str.find("PerspectiveCamera") == std::string::npos) {
            int ret = QMessageBox::warning(Gui::getMainWindow(), 
                qApp->translate("CmdRaytracingWriteView","No perspective camera"),
                qApp->translate("CmdRaytracingWriteView","The current view camera is not perspective"
                                " and thus the result of the povray image later might look different to"
                                " what you expect.\nDo you want to continue?"),
                QMessageBox::Yes|QMessageBox::No);
            if (ret != QMessageBox::Yes)
                return;
        }
    }

    std::string FeatName = getUniqueObjectName("PovProject");

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    QAction* a = pcAction->actions()[iMsg];
    QFileInfo tfi(a->property("Template").toString());
    if (tfi.isReadable()) {
        openCommand("Create Povray project");
        doCommand(Doc,"import Raytracing,RaytracingGui");
        doCommand(Doc,"App.activeDocument().addObject('Raytracing::RayProject','%s')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Template = '%s'",FeatName.c_str(), (const char*)tfi.filePath().toUtf8());
        doCommand(Doc,"App.activeDocument().%s.Camera = RaytracingGui.povViewCamera()",FeatName.c_str());
        commitCommand();
    }
    else {
        QMessageBox::critical(Gui::getMainWindow(),
            qApp->translate("CmdRaytracingNewPovrayProject","No template"),
            qApp->translate("CmdRaytracingNewPovrayProject","No template available"));
    }
}

Gui::Action * CmdRaytracingNewPovrayProject::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(pcAction);

    QAction* defaultAction = 0;
    int defaultId = 0;

    std::string path = App::Application::getResourceDir();
    path += "Mod/Raytracing/Templates/";
    QDir dir(QString::fromUtf8(path.c_str()), QString::fromAscii("*.pov"));
    for (unsigned int i=0; i<dir.count(); i++ ) {
        QFileInfo fi(dir[i]);
        QAction* a = pcAction->addAction(fi.baseName());
        a->setIcon(Gui::BitmapFactory().pixmap("Raytrace_New"));

        a->setProperty("Template", dir.absoluteFilePath(dir[i]));
    }

    _pcAction = pcAction;
    languageChange();
    if (defaultAction) {
        pcAction->setIcon(defaultAction->icon());
        pcAction->setProperty("defaultAction", QVariant(defaultId));
    }
    else if (!pcAction->actions().isEmpty()) {
        pcAction->setIcon(pcAction->actions()[0]->icon());
        pcAction->setProperty("defaultAction", QVariant(0));
    }

    return pcAction;
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

DEF_STD_CMD_A(CmdRaytracingNewPartSegment);

CmdRaytracingNewPartSegment::CmdRaytracingNewPartSegment()
  : Command("Raytracing_NewPartSegment")
{
    sAppModule      = "Raytracing";
    sGroup          = QT_TR_NOOP("Raytracing");
    sMenuText       = QT_TR_NOOP("Insert part");
    sToolTipText    = QT_TR_NOOP("Insert a new part object into a Raytracing project");
    sWhatsThis      = "Raytracing_NewPartSegment";
    sStatusTip      = sToolTipText;
    sPixmap         = "Raytrace_NewPartSegment";
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
    std::vector<App::DocumentObject*> pages2 = App::GetApplication().getActiveDocument()
        ->getObjectsOfType(Raytracing::LuxProject::getClassTypeId());
    pages.insert(pages.end(),pages2.begin(),pages2.end());
    if (pages.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No Raytracing project to insert"),
            QObject::tr("Create a Raytracing project to insert a view."));
        return;
    }

    std::string ProjName;
    if (pages.size() > 1) {
        // priority to the elders, if there is a pov project in the selection, it is used first!
        pages = Gui::Selection().getObjectsOfType(Raytracing::RayProject::getClassTypeId());
        if (pages.size() != 1) {
            pages = Gui::Selection().getObjectsOfType(Raytracing::LuxProject::getClassTypeId());
            if (pages.size() != 1) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No Raytracing project to insert"),
                    QObject::tr("Select a Raytracing project to insert the view."));
                return;
            }
        }
    }

    ProjName = pages.front()->getNameInDocument();
    const char *FeatType;
    if (pages.front()->getTypeId().isDerivedFrom(Raytracing::RayProject::getClassTypeId())) {
        FeatType = "RayFeature";
    } else {
        FeatType = "LuxFeature";
    }

    openCommand("Create view");
    for (std::vector<Part::Feature*>::iterator it = parts.begin(); it != parts.end(); ++it) {
        std::string FeatName = (*it)->getNameInDocument();
        FeatName += "_View";
        FeatName = getUniqueObjectName(FeatName.c_str());
        doCommand(Doc,"App.activeDocument().addObject('Raytracing::%s','%s')",FeatType,FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Source = App.activeDocument().%s",FeatName.c_str(),(*it)->getNameInDocument());
        doCommand(Doc,"App.activeDocument().%s.Color = Gui.activeDocument().%s.ShapeColor",FeatName.c_str(),(*it)->getNameInDocument());
        doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)",ProjName.c_str(), FeatName.c_str());
    }
    updateActive();
    commitCommand();
}

bool CmdRaytracingNewPartSegment::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
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
    sToolTipText  = QT_TR_NOOP("Export a Raytracing project to a file");
    sWhatsThis    = "Raytracing_ExportProject";
    sStatusTip    = sToolTipText;
    sPixmap       = "Raytrace_ExportProject";
}

void CmdRaytracingExportProject::activated(int iMsg)
{
    const char *filterLabel;
    unsigned int n = getSelection().countObjectsOfType(Raytracing::RayProject::getClassTypeId());
    if (n != 1) {
        n = getSelection().countObjectsOfType(Raytracing::LuxProject::getClassTypeId());
        if (n != 1) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Select one Raytracing project object."));
            return;
        } else {
            filterLabel = "Luxrender Scene (*.lxs)";
        }
    } else {
        filterLabel = "Povray Scene (*.pov)";
    }

    QStringList filter;
    filter << QObject::tr(filterLabel);
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


//===========================================================================
// Raytracing_Render
//===========================================================================

DEF_STD_CMD_A(CmdRaytracingRender);

CmdRaytracingRender::CmdRaytracingRender()
  : Command("Raytracing_Render")
{
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&Render");
    sToolTipText  = QT_TR_NOOP("Renders the current raytracing project with an external renderer");
    sWhatsThis    = "Raytracing_Render";
    sStatusTip    = sToolTipText;
    sPixmap       = "Raytrace_Render";
}

void CmdRaytracingRender::activated(int iMsg)
{
    // determining render type
    const char* renderType;
    unsigned int n1 = getSelection().countObjectsOfType(Raytracing::RayProject::getClassTypeId());
    if (n1 != 1) {
        unsigned int n2 = getSelection().countObjectsOfType(Raytracing::LuxProject::getClassTypeId());
        if (n2 != 1) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Select one Raytracing project object."));
            return;
        } else {
            renderType = "luxrender";
        }
    } else {
        renderType = "povray";
    }
    
    // checking if renderer is present
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Raytracing");
    std::string renderer;
    if (renderType == "povray") {
        renderer = hGrp->GetASCII("PovrayExecutable", "");
        if (renderer == "") {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Povray not found"),
                QObject::tr("Please set the path to the povray executable in the preferences."));
            return;
        } else {
            QFileInfo fi(QString::fromUtf8(renderer.c_str()));
            if (!fi.exists() || !fi.isFile()) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Povray not found"),
                    QObject::tr("Please correct the path to the povray executable in the preferences."));
                return;
            }
        }
    } else {
        renderer = hGrp->GetASCII("LuxrenderExecutable", "");
        if (renderer == "") {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Luxrender not found"),
                QObject::tr("Please set the path to the luxrender or luxconsole executable in the preferences."));
            return;
        } else {
            QFileInfo fi(QString::fromUtf8(renderer.c_str()));
            if (!fi.exists() || !fi.isFile()) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Luxrender not found"),
                    QObject::tr("Please correct the path to the luxrender or luxconsole executable in the preferences."));
                return;
            }
        }
    }
    
    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
    
    if (renderType == "povray") {
        QStringList filter;
        filter << QObject::tr("Rendered image(*.png)");
        filter << QObject::tr("All Files (*.*)");
        QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(), QObject::tr("Rendered image"), QString(), filter.join(QLatin1String(";;")));
        if (!fn.isEmpty()) {
            std::string fname = (const char*)fn.toUtf8();
            openCommand("Render project");
            int width = hGrp->GetInt("OutputWidth", 800);
            std::stringstream w;
            w << width;
            int height = hGrp->GetInt("OutputHeight", 600);
            std::stringstream h;
            h << height;
            std::string par = hGrp->GetASCII("OutputParameters", "+P +A");
            doCommand(Doc,"PageFile = open(App.activeDocument().%s.PageResult,'r')",Sel[0].FeatName);
            doCommand(Doc,"import subprocess,tempfile");
            doCommand(Doc,"TempFile = tempfile.mkstemp(suffix='.pov')[1]");
            doCommand(Doc,"f = open(TempFile,'wb')");
            doCommand(Doc,"f.write(PageFile.read())");
            doCommand(Doc,"f.close()");
            doCommand(Doc,"subprocess.call('\"%s\" %s +W%s +H%s +O\"%s\" '+TempFile,shell=True)",renderer.c_str(),par.c_str(),w.str().c_str(),h.str().c_str(),fname.c_str());
            doCommand(Gui,"import ImageGui");
            doCommand(Gui,"ImageGui.open('%s')",fname.c_str());
            doCommand(Doc,"del TempFile,PageFile");
            commitCommand();
        }
    } else {
        openCommand("Render project");
        doCommand(Doc,"PageFile = open(App.activeDocument().%s.PageResult,'r')",Sel[0].FeatName);
        doCommand(Doc,"import subprocess,tempfile");
        doCommand(Doc,"TempFile = tempfile.mkstemp(suffix='.lxs')[1]");
        doCommand(Doc,"f = open(TempFile,'wb')");
        doCommand(Doc,"f.write(PageFile.read())");
        doCommand(Doc,"f.close()");
        doCommand(Doc,"subprocess.Popen([\"%s\",TempFile])",renderer.c_str());
        doCommand(Doc,"del TempFile,PageFile");            
        commitCommand();
    }
}

bool CmdRaytracingRender::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}

//===========================================================================
// Raytracing_NewLuxProject
//===========================================================================

DEF_STD_CMD_AC(CmdRaytracingNewLuxProject);

CmdRaytracingNewLuxProject::CmdRaytracingNewLuxProject()
  : Command("Raytracing_NewLuxProject")
{
    sAppModule      = "Raytracing";
    sGroup          = QT_TR_NOOP("Raytracing");
    sMenuText       = QT_TR_NOOP("New Luxrender project");
    sToolTipText    = QT_TR_NOOP("Insert new Luxrender project into the document");
    sWhatsThis      = "Raytracing_NewLuxrenderProject";
    sStatusTip      = sToolTipText;
    sPixmap         = "Raytrace_Lux";
}

void CmdRaytracingNewLuxProject::activated(int iMsg)
{
    const char* ppReturn=0;
    Gui::Application::Instance->sendMsgToActiveView("GetCamera",&ppReturn);
    if (ppReturn) {
        std::string str(ppReturn);
        if (str.find("PerspectiveCamera") == std::string::npos) {
            int ret = QMessageBox::warning(Gui::getMainWindow(), 
                qApp->translate("CmdRaytracingWriteView","No perspective camera"),
                qApp->translate("CmdRaytracingWriteView","The current view camera is not perspective"
                                " and thus the result of the luxrender image later might look different to"
                                " what you expect.\nDo you want to continue?"),
                QMessageBox::Yes|QMessageBox::No);
            if (ret != QMessageBox::Yes)
                return;
        }
    }

    std::string FeatName = getUniqueObjectName("LuxProject");

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    QAction* a = pcAction->actions()[iMsg];
    QFileInfo tfi(a->property("Template").toString());
    if (tfi.isReadable()) {
        openCommand("Create LuxRender project");
        doCommand(Doc,"import Raytracing,RaytracingGui");
        doCommand(Doc,"App.activeDocument().addObject('Raytracing::LuxProject','%s')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Template = '%s'",FeatName.c_str(), (const char*)tfi.filePath().toUtf8());
        doCommand(Doc,"App.activeDocument().%s.Camera = RaytracingGui.luxViewCamera()",FeatName.c_str());
        commitCommand();
    }
    else {
        QMessageBox::critical(Gui::getMainWindow(),
            qApp->translate("CmdRaytracingNewLuxProject","No template"),
            qApp->translate("CmdRaytracingNewLuxProject","No template available"));
    }
}

Gui::Action * CmdRaytracingNewLuxProject::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(pcAction);

    QAction* defaultAction = 0;
    int defaultId = 0;

    std::string path = App::Application::getResourceDir();
    path += "Mod/Raytracing/Templates/";
    QDir dir(QString::fromUtf8(path.c_str()), QString::fromAscii("*.lxs"));
    for (unsigned int i=0; i<dir.count(); i++ ) {
        QFileInfo fi(dir[i]);
        QAction* a = pcAction->addAction(fi.baseName());
        a->setIcon(Gui::BitmapFactory().pixmap("Raytrace_Lux"));

        a->setProperty("Template", dir.absoluteFilePath(dir[i]));
    }

    _pcAction = pcAction;
    languageChange();
    if (defaultAction) {
        pcAction->setIcon(defaultAction->icon());
        pcAction->setProperty("defaultAction", QVariant(defaultId));
    }
    else if (!pcAction->actions().isEmpty()) {
        pcAction->setIcon(pcAction->actions()[0]->icon());
        pcAction->setProperty("defaultAction", QVariant(0));
    }

    return pcAction;
}

bool CmdRaytracingNewLuxProject::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Raytracing_ResetCamera
//===========================================================================

DEF_STD_CMD_A(CmdRaytracingResetCamera);

CmdRaytracingResetCamera::CmdRaytracingResetCamera()
  : Command("Raytracing_ResetCamera")
{
    // seting the
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&Reset Camera");
    sToolTipText  = QT_TR_NOOP("Sets the camera of the selected Raytracing project to match the current view");
    sWhatsThis    = "Raytracing_ResetCamera";
    sStatusTip    = sToolTipText;
    sPixmap       = "Raytrace_ResetCamera";
}

void CmdRaytracingResetCamera::activated(int iMsg)
{
    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();
    unsigned int n = getSelection().countObjectsOfType(Raytracing::RayProject::getClassTypeId());
    if (n != 1) {
        n = getSelection().countObjectsOfType(Raytracing::LuxProject::getClassTypeId());
        if (n != 1) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Select one Raytracing project object."));
            return;
        } else {
            //luxrender
            openCommand("Reset Raytracing Camera");
            doCommand(Doc,"import RaytracingGui");
            doCommand(Doc,"App.activeDocument().%s.Camera = RaytracingGui.luxViewCamera()",Sel[0].FeatName);
            commitCommand();
            doCommand(Doc,"App.activeDocument().recompute()");
        }
    } else {
        //povray
        openCommand("Reset Raytracing Camera");
        doCommand(Doc,"import RaytracingGui");
        doCommand(Doc,"App.activeDocument().%s.Camera = RaytracingGui.povViewCamera()",Sel[0].FeatName);
        commitCommand();
        doCommand(Doc,"App.activeDocument().recompute()");
    }
}

bool CmdRaytracingResetCamera::isActive(void)
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
    rcCmdMgr.addCommand(new CmdRaytracingRender());
    rcCmdMgr.addCommand(new CmdRaytracingNewLuxProject());
    rcCmdMgr.addCommand(new CmdRaytracingResetCamera());
}
