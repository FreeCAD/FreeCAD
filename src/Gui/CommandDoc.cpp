/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QApplication>
# include <QClipboard>
# include <QEventLoop>
# include <QFileDialog>
# include <QLabel>
# include <QTextStream>
# include <QStatusBar>
# include <QPointer>
# include <QProcess>
# include <sstream>
# include <Inventor/nodes/SoCamera.h>
#endif
#include <algorithm>

#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/Sequencer.h>
#include <Base/Tools.h>
#include <Base/Console.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <App/Origin.h>

#include "Action.h"
#include "Application.h"
#include "Document.h"
#include "Command.h"
#include "Control.h"
#include "FileDialog.h"
#include "MainWindow.h"
#include "BitmapFactory.h"
#include "Selection.h"
#include "DlgProjectInformationImp.h"
#include "DlgProjectUtility.h"
#include "Transform.h"
#include "Placement.h"
#include "ManualAlignment.h"
#include "WaitCursor.h"
#include "ViewProvider.h"
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include "MergeDocuments.h"
#include "NavigationStyle.h"
#include "GraphvizView.h"
#include "DlgObjectSelection.h"

FC_LOG_LEVEL_INIT("Command", false);

using namespace Gui;


//===========================================================================
// Std_Open
//===========================================================================

DEF_STD_CMD(StdCmdOpen);

StdCmdOpen::StdCmdOpen()
  : Command("Std_Open")
{
    // setting the
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&Open...");
    sToolTipText  = QT_TR_NOOP("Open a document or import files");
    sWhatsThis    = "Std_Open";
    sStatusTip    = QT_TR_NOOP("Open a document or import files");
    sPixmap       = "document-open";
    sAccel        = keySequenceToAccel(QKeySequence::Open);
}

void StdCmdOpen::activated(int iMsg)
{
    Q_UNUSED(iMsg); 

    // fill the list of registered endings
    QString formatList;
    const char* supported = QT_TR_NOOP("Supported formats");
    const char* allFiles = QT_TR_NOOP("All files (*.*)");
    formatList = QObject::tr(supported);
    formatList += QLatin1String(" (");

    std::vector<std::string> filetypes = App::GetApplication().getImportTypes();
    std::vector<std::string>::iterator it;
    // Make sure FCStd is the very first fileformat
    it = std::find(filetypes.begin(), filetypes.end(), "FCStd");
    if (it != filetypes.end()) {
        filetypes.erase(it);
        filetypes.insert(filetypes.begin(), "FCStd");
    }
    for (it=filetypes.begin();it != filetypes.end();++it) {
        formatList += QLatin1String(" *.");
        formatList += QLatin1String(it->c_str());
    }

    formatList += QLatin1String(");;");

    std::map<std::string, std::string> FilterList = App::GetApplication().getImportFilters();
    std::map<std::string, std::string>::iterator jt;
    // Make sure the format name for FCStd is the very first in the list
    for (jt=FilterList.begin();jt != FilterList.end();++jt) {
        if (jt->first.find("*.FCStd") != std::string::npos) {
            formatList += QLatin1String(jt->first.c_str());
            formatList += QLatin1String(";;");
            FilterList.erase(jt);
            break;
        }
    }
    for (jt=FilterList.begin();jt != FilterList.end();++jt) {
        formatList += QLatin1String(jt->first.c_str());
        formatList += QLatin1String(";;");
    }
    formatList += QObject::tr(allFiles);

    QString selectedFilter;
    QStringList fileList = FileDialog::getOpenFileNames(getMainWindow(),
        QObject::tr("Open document"), QString(), formatList, &selectedFilter);
    if (fileList.isEmpty())
        return;

    // load the files with the associated modules
    SelectModule::Dict dict = SelectModule::importHandler(fileList, selectedFilter);
    if (dict.isEmpty()) {
        QMessageBox::critical(getMainWindow(),
            qApp->translate("StdCmdOpen", "Cannot open file"),
            qApp->translate("StdCmdOpen", "Loading the file %1 is not supported").arg(fileList.front()));
    }
    else {
        for (SelectModule::Dict::iterator it = dict.begin(); it != dict.end(); ++it) {
            getGuiApplication()->open(it.key().toUtf8(), it.value().toLatin1());
        }
    }
}

//===========================================================================
// Std_Import
//===========================================================================

DEF_STD_CMD_A(StdCmdImport);

StdCmdImport::StdCmdImport()
  : Command("Std_Import")
{
    // setting the
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&Import...");
    sToolTipText  = QT_TR_NOOP("Import a file in the active document");
    sWhatsThis    = "Std_Import";
    sStatusTip    = QT_TR_NOOP("Import a file in the active document");
    //sPixmap       = "Open";
    sAccel        = "Ctrl+I";
}

void StdCmdImport::activated(int iMsg)
{
    Q_UNUSED(iMsg); 

    // fill the list of registered endings
    QString formatList;
    const char* supported = QT_TR_NOOP("Supported formats");
    const char* allFiles = QT_TR_NOOP("All files (*.*)");
    formatList = QObject::tr(supported);
    formatList += QLatin1String(" (");

    std::vector<std::string> filetypes = App::GetApplication().getImportTypes();
    std::vector<std::string>::const_iterator it;
    for (it=filetypes.begin();it != filetypes.end();++it) {
        if (*it != "FCStd") {
            // ignore the project file format
            formatList += QLatin1String(" *.");
            formatList += QLatin1String(it->c_str());
        }
    }

    formatList += QLatin1String(");;");

    std::map<std::string, std::string> FilterList = App::GetApplication().getImportFilters();
    std::map<std::string, std::string>::const_iterator jt;
    for (jt=FilterList.begin();jt != FilterList.end();++jt) {
        // ignore the project file format
        if (jt->first.find("(*.FCStd)") == std::string::npos) {
            formatList += QLatin1String(jt->first.c_str());
            formatList += QLatin1String(";;");
        }
    }
    formatList += QObject::tr(allFiles);

    Base::Reference<ParameterGrp> hPath = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                               ->GetGroup("Preferences")->GetGroup("General");
    QString selectedFilter = QString::fromStdString(hPath->GetASCII("FileImportFilter"));
    QStringList fileList = FileDialog::getOpenFileNames(getMainWindow(),
        QObject::tr("Import file"), QString(), formatList, &selectedFilter);
    if (!fileList.isEmpty()) {
        hPath->SetASCII("FileImportFilter", selectedFilter.toLatin1().constData());
        SelectModule::Dict dict = SelectModule::importHandler(fileList, selectedFilter);

        bool emptyDoc = (getActiveGuiDocument()->getDocument()->countObjects() == 0);
        // load the files with the associated modules
        for (SelectModule::Dict::iterator it = dict.begin(); it != dict.end(); ++it) {
            getGuiApplication()->importFrom(it.key().toUtf8(),
                getActiveGuiDocument()->getDocument()->getName(),
                it.value().toLatin1());
        }

        if (emptyDoc) {
            // only do a view fit if the document was empty before. See also parameter 'AutoFitToView' in importFrom()
            std::list<Gui::MDIView*> views = getActiveGuiDocument()->getMDIViewsOfType(Gui::View3DInventor::getClassTypeId());
            for (std::list<MDIView*>::iterator it = views.begin(); it != views.end(); ++it) {
                (*it)->viewAll();
            }
        }
    }
}

bool StdCmdImport::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}


//===========================================================================
// Std_Export
//===========================================================================

DEF_STD_CMD_A(StdCmdExport);

StdCmdExport::StdCmdExport()
  : Command("Std_Export")
{
    // setting the
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&Export...");
    sToolTipText  = QT_TR_NOOP("Export an object in the active document");
    sWhatsThis    = "Std_Export";
    sStatusTip    = QT_TR_NOOP("Export an object in the active document");
    //sPixmap       = "Open";
    sAccel        = "Ctrl+E";
    eType         = 0;
}

void StdCmdExport::activated(int iMsg)
{
    Q_UNUSED(iMsg); 

    if (Gui::Selection().countObjectsOfType(App::DocumentObject::getClassTypeId()) == 0) {
        QMessageBox::warning(Gui::getMainWindow(),
            QString::fromUtf8(QT_TR_NOOP("No selection")),
            QString::fromUtf8(QT_TR_NOOP("Please select first the objects you want to export.")));
        return;
    }

    // fill the list of registered endings
    QString formatList;
    std::map<std::string, std::string> FilterList = App::GetApplication().getExportFilters();
    std::map<std::string, std::string>::const_iterator jt;
    for (jt=FilterList.begin();jt != FilterList.end();++jt) {
        // ignore the project file format
        if (jt->first.find("(*.FCStd)") == std::string::npos) {
            formatList += QLatin1String(jt->first.c_str());
            formatList += QLatin1String(";;");
        }
    }

    Base::Reference<ParameterGrp> hPath = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                               ->GetGroup("Preferences")->GetGroup("General");
    QString selectedFilter = QString::fromStdString(hPath->GetASCII("FileExportFilter"));

    QString fileName = FileDialog::getSaveFileName(getMainWindow(),
        QObject::tr("Export file"), QString(), formatList, &selectedFilter);
    if (!fileName.isEmpty()) {
        hPath->SetASCII("FileExportFilter", selectedFilter.toLatin1().constData());
        SelectModule::Dict dict = SelectModule::exportHandler(fileName, selectedFilter);
        // export the files with the associated modules
        for (SelectModule::Dict::iterator it = dict.begin(); it != dict.end(); ++it) {
            getGuiApplication()->exportTo(it.key().toUtf8(),
                getActiveGuiDocument()->getDocument()->getName(),
                it.value().toLatin1());
        }
    }
}

bool StdCmdExport::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}

//===========================================================================
// Std_MergeProjects
//===========================================================================

DEF_STD_CMD_A(StdCmdMergeProjects);

StdCmdMergeProjects::StdCmdMergeProjects()
  : Command("Std_MergeProjects")
{
    sAppModule    = "File";
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("Merge project...");
    sToolTipText  = QT_TR_NOOP("Merge project");
    sWhatsThis    = "Std_MergeProjects";
    sStatusTip    = QT_TR_NOOP("Merge project");
}

void StdCmdMergeProjects::activated(int iMsg)
{
    Q_UNUSED(iMsg); 

    QString exe = qApp->applicationName();
    QString project = QFileDialog::getOpenFileName(Gui::getMainWindow(),
        QString::fromUtf8(QT_TR_NOOP("Merge project")), FileDialog::getWorkingDirectory(),
        QString::fromUtf8(QT_TR_NOOP("%1 document (*.fcstd)")).arg(exe));
    if (!project.isEmpty()) {
        FileDialog::setWorkingDirectory(project);
        App::Document* doc = App::GetApplication().getActiveDocument();
        QFileInfo info(QString::fromUtf8(doc->FileName.getValue()));
        QFileInfo proj(project);
        if (proj == info) {
            QMessageBox::critical(Gui::getMainWindow(),
                QString::fromUtf8(QT_TR_NOOP("Merge project")),
                QString::fromUtf8(QT_TR_NOOP("Cannot merge project with itself.")));
            return;
        }

        doc->openTransaction("Merge project");
        Base::FileInfo fi((const char*)project.toUtf8());
        Base::ifstream str(fi, std::ios::in | std::ios::binary);
        MergeDocuments md(doc);
        md.importObjects(str);
        str.close();
        doc->commitTransaction();
    }
}

bool StdCmdMergeProjects::isActive(void)
{
    return this->hasActiveDocument();
}

//===========================================================================
// Std_ExportGraphviz
//===========================================================================

DEF_STD_CMD_A(StdCmdExportGraphviz);

StdCmdExportGraphviz::StdCmdExportGraphviz()
  : Command("Std_ExportGraphviz")
{
    // setting the
    sGroup        = QT_TR_NOOP("Tools");
    sMenuText     = QT_TR_NOOP("Dependency graph...");
    sToolTipText  = QT_TR_NOOP("Show the dependency graph of the objects in the active document");
    sStatusTip    = QT_TR_NOOP("Show the dependency graph of the objects in the active document");
    sWhatsThis    = "Std_ExportGraphviz";
    eType         = 0;
}

void StdCmdExportGraphviz::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    App::Document* doc = App::GetApplication().getActiveDocument();
    Gui::GraphvizView* view = new Gui::GraphvizView(*doc);
    view->setWindowTitle(qApp->translate("Std_ExportGraphviz","Dependency graph"));
    getMainWindow()->addWindow(view);
}

bool StdCmdExportGraphviz::isActive(void)
{
    return (getActiveGuiDocument() ? true : false);
}

//===========================================================================
// Std_New
//===========================================================================

DEF_STD_CMD(StdCmdNew);

StdCmdNew::StdCmdNew()
  :Command("Std_New")
{
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&New");
    sToolTipText  = QT_TR_NOOP("Create a new empty document");
    sWhatsThis    = "Std_New";
    sStatusTip    = QT_TR_NOOP("Create a new empty document");
    sPixmap       = "document-new";
    sAccel        = keySequenceToAccel(QKeySequence::New);
}

void StdCmdNew::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    QString cmd;
    cmd = QString::fromLatin1("App.newDocument(\"%1\")")
        .arg(qApp->translate("StdCmdNew","Unnamed"));
    runCommand(Command::Doc,cmd.toUtf8());
}

//===========================================================================
// Std_Save
//===========================================================================
DEF_STD_CMD_A(StdCmdSave);

StdCmdSave::StdCmdSave()
  :Command("Std_Save")
{
  sGroup        = QT_TR_NOOP("File");
  sMenuText     = QT_TR_NOOP("&Save");
  sToolTipText  = QT_TR_NOOP("Save the active document");
  sWhatsThis    = "Std_Save";
  sStatusTip    = QT_TR_NOOP("Save the active document");
  sPixmap       = "document-save";
  sAccel        = keySequenceToAccel(QKeySequence::Save);
  eType         = 0;
}

void StdCmdSave::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
#if 0
  Gui::Document* pActiveDoc = getActiveGuiDocument();
  if ( pActiveDoc )
    pActiveDoc->save();
  else
#endif
    doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"Save\")");
}

bool StdCmdSave::isActive(void)
{
#if 0
  if( getActiveGuiDocument() )
    return true;
  else
#endif
    return getGuiApplication()->sendHasMsgToActiveView("Save");
}

//===========================================================================
// Std_SaveAs
//===========================================================================
DEF_STD_CMD_A(StdCmdSaveAs);

StdCmdSaveAs::StdCmdSaveAs()
  :Command("Std_SaveAs")
{
  sGroup        = QT_TR_NOOP("File");
  sMenuText     = QT_TR_NOOP("Save &As...");
  sToolTipText  = QT_TR_NOOP("Save the active document under a new file name");
  sWhatsThis    = "Std_SaveAs";
  sStatusTip    = QT_TR_NOOP("Save the active document under a new file name");
#if QT_VERSION >= 0x040200
  sPixmap       = "document-save-as";
#endif
  sAccel        = keySequenceToAccel(QKeySequence::SaveAs);
  eType         = 0;
}

void StdCmdSaveAs::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
#if 0
  Gui::Document* pActiveDoc = getActiveGuiDocument();
  if ( pActiveDoc )
    pActiveDoc->saveAs();
  else
#endif
    doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"SaveAs\")");
}

bool StdCmdSaveAs::isActive(void)
{
#if 0
  if( getActiveGuiDocument() )
    return true;
  else
#endif
    return getGuiApplication()->sendHasMsgToActiveView("SaveAs");
}

//===========================================================================
// Std_SaveCopy
//===========================================================================
DEF_STD_CMD_A(StdCmdSaveCopy);

StdCmdSaveCopy::StdCmdSaveCopy()
  :Command("Std_SaveCopy")
{
  sGroup        = QT_TR_NOOP("File");
  sMenuText     = QT_TR_NOOP("Save a &Copy...");
  sToolTipText  = QT_TR_NOOP("Save a copy of the active document under a new file name");
  sWhatsThis    = "Std_SaveCopy";
  sStatusTip    = QT_TR_NOOP("Save a copy of the active document under a new file name");
  //sPixmap       = "document-save-as";
}

void StdCmdSaveCopy::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
#if 0
  Gui::Document* pActiveDoc = getActiveGuiDocument();
  if ( pActiveDoc )
    pActiveDoc->saveCopy();
  else
#endif
    doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"SaveCopy\")");
}

bool StdCmdSaveCopy::isActive(void)
{
  return ( getActiveGuiDocument() ? true : false );
}

//===========================================================================
// Std_SaveAll
//===========================================================================
DEF_STD_CMD_A(StdCmdSaveAll);

StdCmdSaveAll::StdCmdSaveAll()
  :Command("Std_SaveAll")
{
  sGroup        = QT_TR_NOOP("File");
  sMenuText     = QT_TR_NOOP("Save All");
  sToolTipText  = QT_TR_NOOP("Save all opened document");
  sWhatsThis    = "Std_SaveAll";
  sStatusTip    = QT_TR_NOOP("Save all opened document");
}

void StdCmdSaveAll::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    Gui::Document::saveAll();
}

bool StdCmdSaveAll::isActive(void)
{
  return ( getActiveGuiDocument() ? true : false );
}


//===========================================================================
// Std_Revert
//===========================================================================
DEF_STD_CMD_A(StdCmdRevert);

StdCmdRevert::StdCmdRevert()
  :Command("Std_Revert")
{
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("Revert");
    sToolTipText  = QT_TR_NOOP("Reverts to the saved version of this file");
    sWhatsThis    = "Std_Revert";
    sStatusTip    = QT_TR_NOOP("Reverts to the saved version of this file");
  //sPixmap       = "document-revert";
}

void StdCmdRevert::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    QMessageBox msgBox(Gui::getMainWindow());
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle(qApp->translate("Std_Revert","Revert document"));
    msgBox.setText(qApp->translate("Std_Revert","This will discard all the changes since last file save."));
    msgBox.setInformativeText(qApp->translate("Std_Revert","Do you want to continue?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();
    if (ret == QMessageBox::Yes)
        doCommand(Command::App,"App.ActiveDocument.restore()");
}

bool StdCmdRevert::isActive(void)
{
  return ( getActiveGuiDocument() ? true : false );
}

//===========================================================================
// Std_ProjectInfo
//===========================================================================

DEF_STD_CMD_A(StdCmdProjectInfo);

StdCmdProjectInfo::StdCmdProjectInfo()
  :Command("Std_ProjectInfo")
{
  // setting the
  sGroup        = QT_TR_NOOP("File");
  sMenuText     = QT_TR_NOOP("Project i&nformation...");
  sToolTipText  = QT_TR_NOOP("Show details of the currently active project");
  sWhatsThis    = "Std_ProjectInfo";
  sStatusTip    = QT_TR_NOOP("Show details of the currently active project");
#if QT_VERSION >= 0x040200
  sPixmap       = "document-properties";
#endif
}

void StdCmdProjectInfo::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    Gui::Dialog::DlgProjectInformationImp dlg(getActiveGuiDocument()->getDocument(), getMainWindow());
    dlg.exec();
}

bool StdCmdProjectInfo::isActive(void)
{
  return ( getActiveGuiDocument() ? true : false );
}

//===========================================================================
// Std_ProjectUtil
//===========================================================================

DEF_STD_CMD_A(StdCmdProjectUtil);

StdCmdProjectUtil::StdCmdProjectUtil()
  :Command("Std_ProjectUtil")
{
    // setting the
    sGroup        = QT_TR_NOOP("Tools");
    sWhatsThis    = "Std_ProjectUtil";
    sMenuText     = QT_TR_NOOP("Project utility...");
    sToolTipText  = QT_TR_NOOP("Utility to extract or create project files");
    sStatusTip    = QT_TR_NOOP("Utility to extract or create project files");
}

void StdCmdProjectUtil::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    Gui::Dialog::DlgProjectUtility dlg(getMainWindow());
    dlg.exec();
}

bool StdCmdProjectUtil::isActive(void)
{
    return true;
}

//===========================================================================
// Std_Print
//===========================================================================
DEF_STD_CMD_A(StdCmdPrint );

StdCmdPrint::StdCmdPrint()
  :Command("Std_Print")
{
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&Print...");
    sToolTipText  = QT_TR_NOOP("Print the document");
    sWhatsThis    = "Std_Print";
    sStatusTip    = QT_TR_NOOP("Print the document");
    sPixmap       = "document-print";
    sAccel        = keySequenceToAccel(QKeySequence::Print);
}

void StdCmdPrint::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    if (getMainWindow()->activeWindow()) {
        getMainWindow()->showMessage(QObject::tr("Printing..."));
        getMainWindow()->activeWindow()->print();
    }
}

bool StdCmdPrint::isActive(void)
{
    return getGuiApplication()->sendHasMsgToActiveView("Print");
}

//===========================================================================
// Std_PrintPreview
//===========================================================================
DEF_STD_CMD_A(StdCmdPrintPreview);

StdCmdPrintPreview::StdCmdPrintPreview()
  :Command("Std_PrintPreview")
{
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&Print preview...");
    sToolTipText  = QT_TR_NOOP("Print the document");
    sWhatsThis    = "Std_PrintPreview";
    sStatusTip    = QT_TR_NOOP("Print preview");
    sPixmap       = "document-print-preview";
}

void StdCmdPrintPreview::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    if (getMainWindow()->activeWindow()) {
        getMainWindow()->activeWindow()->printPreview();
    }
}

bool StdCmdPrintPreview::isActive(void)
{
    return getGuiApplication()->sendHasMsgToActiveView("PrintPreview");
}

//===========================================================================
// Std_PrintPdf
//===========================================================================
DEF_STD_CMD_A(StdCmdPrintPdf);

StdCmdPrintPdf::StdCmdPrintPdf()
  :Command("Std_PrintPdf")
{
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("&Export PDF...");
    sToolTipText  = QT_TR_NOOP("Export the document as PDF");
    sWhatsThis    = "Std_PrintPdf";
    sStatusTip    = QT_TR_NOOP("Export the document as PDF");
}

void StdCmdPrintPdf::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    if (getMainWindow()->activeWindow()) {
        getMainWindow()->showMessage(QObject::tr("Exporting PDF..."));
        getMainWindow()->activeWindow()->printPdf();
    }
}

bool StdCmdPrintPdf::isActive(void)
{
    return getGuiApplication()->sendHasMsgToActiveView("PrintPdf");
}

//===========================================================================
// Std_Quit
//===========================================================================

DEF_STD_CMD(StdCmdQuit );

StdCmdQuit::StdCmdQuit()
  :Command("Std_Quit")
{
  sGroup        = QT_TR_NOOP("File");
  sMenuText     = QT_TR_NOOP("E&xit");
  sToolTipText  = QT_TR_NOOP("Quits the application");
  sWhatsThis    = "Std_Quit";
  sStatusTip    = QT_TR_NOOP("Quits the application");
#if QT_VERSION >= 0x040200
  sPixmap       = "application-exit";
#endif
  sAccel        = "Alt+F4";
}

void StdCmdQuit::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    // close the main window and exit the event loop
    getMainWindow()->close();
}

//===========================================================================
// Std_Undo
//===========================================================================

DEF_STD_CMD_AC(StdCmdUndo);

StdCmdUndo::StdCmdUndo()
  :Command("Std_Undo")
{
  sGroup        = QT_TR_NOOP("Edit");
  sMenuText     = QT_TR_NOOP("&Undo");
  sToolTipText  = QT_TR_NOOP("Undo exactly one action");
  sWhatsThis    = "Std_Undo";
  sStatusTip    = QT_TR_NOOP("Undo exactly one action");
  sPixmap       = "edit-undo";
  sAccel        = keySequenceToAccel(QKeySequence::Undo);
  eType         = ForEdit;
}

void StdCmdUndo::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
//  Application::Instance->slotUndo();
    getGuiApplication()->sendMsgToActiveView("Undo");
}

bool StdCmdUndo::isActive(void)
{
  return getGuiApplication()->sendHasMsgToActiveView("Undo");
}

Action * StdCmdUndo::createAction(void)
{
    Action *pcAction;

    pcAction = new UndoAction(this,getMainWindow());
    pcAction->setShortcut(QString::fromLatin1(sAccel));
    applyCommandData(this->className(), pcAction);
    if (sPixmap)
        pcAction->setIcon(Gui::BitmapFactory().iconFromTheme(sPixmap));

    return pcAction;
}

//===========================================================================
// Std_Redo
//===========================================================================

DEF_STD_CMD_AC(StdCmdRedo );

StdCmdRedo::StdCmdRedo()
  :Command("Std_Redo")
{
  sGroup        = QT_TR_NOOP("Edit");
  sMenuText     = QT_TR_NOOP("&Redo");
  sToolTipText  = QT_TR_NOOP("Redoes a previously undone action");
  sWhatsThis    = "Std_Redo";
  sStatusTip    = QT_TR_NOOP("Redoes a previously undone action");
  sPixmap       = "edit-redo";
  sAccel        = keySequenceToAccel(QKeySequence::Redo);
  eType         = ForEdit;
}

void StdCmdRedo::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
//  Application::Instance->slotRedo();
    getGuiApplication()->sendMsgToActiveView("Redo");
}

bool StdCmdRedo::isActive(void)
{
  return getGuiApplication()->sendHasMsgToActiveView("Redo");
}

Action * StdCmdRedo::createAction(void)
{
    Action *pcAction;

    pcAction = new RedoAction(this,getMainWindow());
    pcAction->setShortcut(QString::fromLatin1(sAccel));
    applyCommandData(this->className(), pcAction);
    if (sPixmap)
        pcAction->setIcon(Gui::BitmapFactory().iconFromTheme(sPixmap));

    return pcAction;
}

//===========================================================================
// Std_Cut
//===========================================================================
DEF_STD_CMD_A(StdCmdCut);

StdCmdCut::StdCmdCut()
  : Command("Std_Cut")
{
    sGroup        = QT_TR_NOOP("Edit");
    sMenuText     = QT_TR_NOOP("&Cut");
    sToolTipText  = QT_TR_NOOP("Cut out");
    sWhatsThis    = "Std_Cut";
    sStatusTip    = QT_TR_NOOP("Cut out");
    sPixmap       = "edit-cut";
    sAccel        = keySequenceToAccel(QKeySequence::Cut);
}

void StdCmdCut::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    getGuiApplication()->sendMsgToActiveView("Cut");
}

bool StdCmdCut::isActive(void)
{
    return getGuiApplication()->sendHasMsgToActiveView("Cut");
}

//===========================================================================
// Std_Copy
//===========================================================================
DEF_STD_CMD_A(StdCmdCopy);

StdCmdCopy::StdCmdCopy()
  : Command("Std_Copy")
{
    sGroup        = QT_TR_NOOP("Edit");
    sMenuText     = QT_TR_NOOP("C&opy");
    sToolTipText  = QT_TR_NOOP("Copy operation");
    sWhatsThis    = "Std_Copy";
    sStatusTip    = QT_TR_NOOP("Copy operation");
    sPixmap       = "edit-copy";
    sAccel        = keySequenceToAccel(QKeySequence::Copy);
}

void StdCmdCopy::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    bool done = getGuiApplication()->sendMsgToActiveView("Copy");
    if (!done) {
        QMimeData * mimeData = getMainWindow()->createMimeDataFromSelection();
        QClipboard* cb = QApplication::clipboard();
        cb->setMimeData(mimeData);
    }
}

bool StdCmdCopy::isActive(void)
{
    if (getGuiApplication()->sendHasMsgToActiveView("Copy"))
        return true;
    return Selection().hasSelection();
}

//===========================================================================
// Std_Paste
//===========================================================================
DEF_STD_CMD_A(StdCmdPaste);

StdCmdPaste::StdCmdPaste()
  : Command("Std_Paste")
{
    sGroup        = QT_TR_NOOP("Edit");
    sMenuText     = QT_TR_NOOP("&Paste");
    sToolTipText  = QT_TR_NOOP("Paste operation");
    sWhatsThis    = "Std_Paste";
    sStatusTip    = QT_TR_NOOP("Paste operation");
    sPixmap       = "edit-paste";
    sAccel        = keySequenceToAccel(QKeySequence::Paste);
}

void StdCmdPaste::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    bool done = getGuiApplication()->sendMsgToActiveView("Paste");
    if (!done) {
        QClipboard* cb = QApplication::clipboard();
        const QMimeData* mimeData = cb->mimeData();
        if (mimeData) {
            WaitCursor wc;
            getMainWindow()->insertFromMimeData(mimeData);
        }
    }
}

bool StdCmdPaste::isActive(void)
{
    if (getGuiApplication()->sendHasMsgToActiveView("Paste"))
        return true;
    QClipboard* cb = QApplication::clipboard();
    const QMimeData* mime = cb->mimeData();
    if (!mime) return false;
    return getMainWindow()->canInsertFromMimeData(mime);
}

DEF_STD_CMD_A(StdCmdDuplicateSelection);

StdCmdDuplicateSelection::StdCmdDuplicateSelection()
  :Command("Std_DuplicateSelection")
{
    sAppModule    = "Edit";
    sGroup        = QT_TR_NOOP("Edit");
    sMenuText     = QT_TR_NOOP("Duplicate selection");
    sToolTipText  = QT_TR_NOOP("Put duplicates of the selected objects to the active document");
    sWhatsThis    = "Std_DuplicateSelection";
    sStatusTip    = QT_TR_NOOP("Put duplicates of the selected objects to the active document");
}

void StdCmdDuplicateSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    std::vector<App::DocumentObject*> sel;
    std::set<App::DocumentObject*> objSet;
    for(auto &s : Selection().getCompleteSelection()) {
        if(s.pObject && s.pObject->getNameInDocument() && objSet.insert(s.pObject).second)
            sel.push_back(s.pObject);
    }
    if(sel.empty())
        return;

    bool hasXLink = false;
    Base::FileInfo fi(App::Application::getTempFileName());
    {
        auto all = App::Document::getDependencyList(sel);
        if (all.size() > sel.size()) {
            DlgObjectSelection dlg(sel,getMainWindow());
            if(dlg.exec()!=QDialog::Accepted)
                return;
            sel = dlg.getSelections();
            if(sel.empty())
                return;
        }
        std::vector<App::Document*> unsaved;
        hasXLink = App::PropertyXLink::hasXLink(sel,&unsaved);
        if(unsaved.size()) {
            QMessageBox::critical(getMainWindow(), QObject::tr("Unsaved document"),
                QObject::tr("The exported object contains external link. Please save the document"
                   "at least once before exporting."));
            return;
        }

        // save stuff to file
        Base::ofstream str(fi, std::ios::out | std::ios::binary);
        App::Document* doc = sel.front()->getDocument();
        MergeDocuments mimeView(doc);
        doc->exportObjects(sel, str);
        str.close();
    }
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc) {
        bool proceed = true;
        if(hasXLink && !doc->isSaved()) {
            int ret = QMessageBox::question(getMainWindow(),
                qApp->translate("Std_DuplicateSelection","Object dependencies"),
                qApp->translate("Std_DuplicateSelection",
                "To link to external objects, the document must be saved at least once.\n"
                "Do you want to save the document now?"),
                QMessageBox::Yes,QMessageBox::No);
            if(ret == QMessageBox::Yes) 
                proceed = Application::Instance->getDocument(doc)->saveAs();
        }
        if(proceed) {
            doc->openTransaction("Duplicate");
            // restore objects from file and add to active document
            Base::ifstream str(fi, std::ios::in | std::ios::binary);
            MergeDocuments mimeView(doc);
            mimeView.importObjects(str);
            str.close();
            doc->commitTransaction();
        }
    }
    fi.deleteFile();
}

bool StdCmdDuplicateSelection::isActive(void)
{
    return Gui::Selection().hasSelection();
}

//===========================================================================
// Std_SelectAll
//===========================================================================

DEF_STD_CMD_A(StdCmdSelectAll);

StdCmdSelectAll::StdCmdSelectAll()
  : Command("Std_SelectAll")
{
    sGroup        = QT_TR_NOOP("Edit");
    sMenuText     = QT_TR_NOOP("Select &All");
    sToolTipText  = QT_TR_NOOP("Select all");
    sWhatsThis    = "Std_SelectAll";
    sStatusTip    = QT_TR_NOOP("Select all");
#if QT_VERSION >= 0x040200
    sPixmap       = "edit-select-all";
#endif
    //sAccel        = "Ctrl+A"; // superseeds shortcuts for text edits
}

void StdCmdSelectAll::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    SelectionSingleton& rSel = Selection();
    App::Document* doc = App::GetApplication().getActiveDocument();
    std::vector<App::DocumentObject*> objs = doc->getObjectsOfType(App::DocumentObject::getClassTypeId());
    rSel.setSelection(doc->getName(), objs);
}

bool StdCmdSelectAll::isActive(void)
{
    return App::GetApplication().getActiveDocument() != 0;
}

//===========================================================================
// Std_Delete
//===========================================================================
DEF_STD_CMD_A(StdCmdDelete);

StdCmdDelete::StdCmdDelete()
  :Command("Std_Delete")
{
  sGroup        = QT_TR_NOOP("Edit");
  sMenuText     = QT_TR_NOOP("&Delete");
  sToolTipText  = QT_TR_NOOP("Deletes the selected objects");
  sWhatsThis    = "Std_Delete";
  sStatusTip    = QT_TR_NOOP("Deletes the selected objects");
#if QT_VERSION >= 0x040200
  sPixmap       = "edit-delete";
#endif
  sAccel        = keySequenceToAccel(QKeySequence::Delete);
  eType         = ForEdit;
}

void StdCmdDelete::activated(int iMsg)
{
    Q_UNUSED(iMsg); 

    std::set<App::Document*> docs;
    try {
        App::GetApplication().setActiveTransaction("Delete");
        auto activeView = getMainWindow()->activeWindow();
        if(activeView) {
            bool found = false;
            for(auto focus=qApp->focusWidget();focus;focus=focus->parentWidget()) {
                if(focus == activeView) {
                    found = true;
                    break;
                }
            }
            if(found && activeView->onMsg(getName(),0)) {
                App::GetApplication().closeActiveTransaction();
                return;
            }
        }
        Gui::getMainWindow()->setUpdatesEnabled(false);
        auto editDoc = Application::Instance->editDocument();
        ViewProviderDocumentObject *vpedit = 0;
        if(editDoc)
            vpedit = dynamic_cast<ViewProviderDocumentObject*>(editDoc->getInEdit());
        if(vpedit) {
            for(auto &sel : Selection().getSelectionEx(editDoc->getDocument()->getName())) {
                if(sel.getObject() == vpedit->getObject()) {
                    if (!sel.getSubNames().empty()) {
                        vpedit->onDelete(sel.getSubNames());
                        docs.insert(editDoc->getDocument());
                    }
                    break;
                }
            }
        } else {
            std::set<QString> affectedLabels;
            bool more = false;
            auto sels = Selection().getSelectionEx();
            bool autoDeletion = true;
            for(auto &sel : sels) {
                auto obj = sel.getObject();
                for(auto parent : obj->getInList()) {
                    if(!Selection().isSelected(parent)) {
                        ViewProvider* vp = Application::Instance->getViewProvider(parent);
                        if (vp && !vp->canDelete(obj)) {
                            autoDeletion = false;
                            QString label;
                            if(parent->getDocument() != obj->getDocument())
                                label = QLatin1String(parent->getFullName().c_str());
                            else
                                label = QLatin1String(parent->getNameInDocument());
                            if(parent->Label.getStrValue() != parent->getNameInDocument())
                                label += QString::fromLatin1(" (%1)").arg(
                                        QString::fromUtf8(parent->Label.getValue()));
                            affectedLabels.insert(label);
                            if(affectedLabels.size()>=10) {
                                more = true;
                                break;
                            }
                        }
                    }
                }
                if(more)
                    break;
            }

            // The check below is not needed because we now only get selection
            // from the active document
#if 0
            //check for inactive objects in selection  Mantis #3477
            std::set<QString> inactiveLabels;
            App::Application& app = App::GetApplication();
            App::Document* actDoc = app.getActiveDocument();
            for (std::vector<Gui::SelectionObject>::iterator ft = sels.begin(); ft != sels.end(); ++ft) {
                App::DocumentObject* obj = ft->getObject();
                App::Document* objDoc = obj->getDocument();
                if (actDoc != objDoc) {
                    inactiveLabels.insert(QString::fromUtf8(obj->Label.getValue()));
                    autoDeletion = false;
                }
            }
#endif

            if (!autoDeletion) {
                QString bodyMessage;
                QTextStream bodyMessageStream(&bodyMessage);
                bodyMessageStream << qApp->translate("Std_Delete",
                                                     "The following referencing objects might break.\n\n"
                                                     "Are you sure you want to continue?\n");
                for (const auto &currentLabel : affectedLabels)
                    bodyMessageStream << '\n' << currentLabel;
                if(more)
                    bodyMessageStream << "\n...";
#if 0
                //message for inactive items
                if (!inactiveLabels.empty()) {
                    if (!affectedLabels.empty()) {
                        bodyMessageStream << "\n";
                    }
                    std::string thisDoc = pGuiDoc->getDocument()->getName();
                    bodyMessageStream << qApp->translate("Std_Delete", 
                                            "These items are selected for deletion, but are not in the active document. \n\n"); 
                    for (const auto &currentLabel : inactiveLabels)
                        bodyMessageStream << currentLabel << " / " << Base::Tools::fromStdString(thisDoc) << '\n';
                }
#endif

                int ret = QMessageBox::warning(Gui::getMainWindow(),
                    qApp->translate("Std_Delete", "Object dependencies"), bodyMessage,
                    QMessageBox::Yes, QMessageBox::No);
                if (ret == QMessageBox::Yes)
                    autoDeletion = true;
            }
            if (autoDeletion) {
                for(auto &sel : sels) {
                    auto obj = sel.getObject();
                    Gui::ViewProvider* vp = Application::Instance->getViewProvider(obj);
                    if (vp) {
                        // ask the ViewProvider if it wants to do some clean up
                        if (vp->onDelete(sel.getSubNames())) {
                            FCMD_OBJ_DOC_CMD(obj,"removeObject('" << obj->getNameInDocument() << "')");
                            docs.insert(obj->getDocument());
                        }
                    }
                }
            }
        }
        if(docs.size()) {
            const auto &outList = App::PropertyXLink::getDocumentOutList();
            for(auto it=docs.begin();it!=docs.end();++it) {
                auto itd = outList.find(*it);
                if(itd!=outList.end()) {
                    for(auto doc : itd->second) {
                        if(doc != *it)
                            docs.erase(doc);
                    }
                }
            }
            for(auto doc : docs) {
                FCMD_DOC_CMD(doc,"recompute()");
            }
        }
    } catch (const Base::Exception& e) {
        QMessageBox::critical(getMainWindow(), QObject::tr("Delete failed"),
                QString::fromLatin1(e.what()));
        e.ReportException();
    } catch (...) {
        QMessageBox::critical(getMainWindow(), QObject::tr("Delete failed"),
                QString::fromLatin1("Unknown error"));
    }
    App::GetApplication().closeActiveTransaction();
    Gui::getMainWindow()->setUpdatesEnabled(true);
    Gui::getMainWindow()->update();
}

bool StdCmdDelete::isActive(void)
{
    return Selection().getCompleteSelection().size() > 0;
}

//===========================================================================
// Std_Refresh
//===========================================================================
DEF_STD_CMD_A(StdCmdRefresh);

StdCmdRefresh::StdCmdRefresh()
  : Command("Std_Refresh")
{
    sGroup        = QT_TR_NOOP("Edit");
    sMenuText     = QT_TR_NOOP("&Refresh");
    sToolTipText  = QT_TR_NOOP("Recomputes the current active document");
    sWhatsThis    = "Std_Refresh";
    sStatusTip    = QT_TR_NOOP("Recomputes the current active document");
    sPixmap       = "view-refresh";
    sAccel        = keySequenceToAccel(QKeySequence::Refresh);
    eType         = AlterDoc | Alter3DView | AlterSelection | ForEdit;
    bCanLog        = false;
}

void StdCmdRefresh::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    if (getActiveGuiDocument()) {
        App::AutoTransaction trans("Recompute");
        try {
            doCommand(Doc,"App.activeDocument().recompute(None,True,True)");
        } catch(Base::Exception &e) {
            int ret = QMessageBox::warning(getMainWindow(), QObject::tr("Dependency error"),
                QObject::tr("The document contains dependency cycles.\n"
                            "Please check the Report View for more details.\n\n"
                            "Do you still want to proceed?"),
                    QMessageBox::Yes, QMessageBox::No);
            if(ret == QMessageBox::No)
                return;
            doCommand(Doc,"App.activeDocument().recompute(None,True)");
        }
    }
}

bool StdCmdRefresh::isActive(void)
{
    return this->getDocument() && this->getDocument()->mustExecute();
}

//===========================================================================
// Std_Transform
//===========================================================================
DEF_STD_CMD_A(StdCmdTransform);

StdCmdTransform::StdCmdTransform()
  : Command("Std_Transform")
{
    sGroup        = QT_TR_NOOP("Edit");
    sMenuText     = QT_TR_NOOP("Transform...");
    sToolTipText  = QT_TR_NOOP("Transform the geometry of selected objects");
    sStatusTip    = QT_TR_NOOP("Transform the geometry of selected objects");
    sWhatsThis    = "Std_Transform";
}

void StdCmdTransform::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    Gui::Control().showDialog(new Gui::Dialog::TaskTransform());
}

bool StdCmdTransform::isActive(void)
{
    return (Gui::Control().activeDialog()==0);
}

//===========================================================================
// Std_Placement
//===========================================================================
DEF_STD_CMD_A(StdCmdPlacement);

StdCmdPlacement::StdCmdPlacement()
  : Command("Std_Placement")
{
    sGroup        = QT_TR_NOOP("Edit");
    sMenuText     = QT_TR_NOOP("Placement...");
    sToolTipText  = QT_TR_NOOP("Place the selected objects");
    sStatusTip    = QT_TR_NOOP("Place the selected objects");
    sWhatsThis    = "Std_Placement";
}

void StdCmdPlacement::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    std::vector<App::DocumentObject*> sel = Gui::Selection().getObjectsOfType(App::GeoFeature::getClassTypeId());
    Gui::Dialog::TaskPlacement* plm = new Gui::Dialog::TaskPlacement();
    if (!sel.empty()) {
        App::Property* prop = sel.front()->getPropertyByName("Placement");
        if (prop && prop->getTypeId() == App::PropertyPlacement::getClassTypeId())
            plm->setPlacement(static_cast<App::PropertyPlacement*>(prop)->getValue());
    }
    Gui::Control().showDialog(plm);
}

bool StdCmdPlacement::isActive(void)
{
    return (Gui::Control().activeDialog()==0);
}

//===========================================================================
// Std_TransformManip
//===========================================================================
DEF_STD_CMD_A(StdCmdTransformManip);

StdCmdTransformManip::StdCmdTransformManip()
  : Command("Std_TransformManip")
{
    sGroup        = QT_TR_NOOP("Edit");
    sMenuText     = QT_TR_NOOP("Transform");
    sToolTipText  = QT_TR_NOOP("Transform the selected object in the 3d view");
    sStatusTip    = QT_TR_NOOP("Transform the selected object in the 3d view");
    sWhatsThis    = "Std_TransformManip";
}

void StdCmdTransformManip::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    if (getActiveGuiDocument()->getInEdit())
        getActiveGuiDocument()->resetEdit();
    std::vector<App::DocumentObject*> sel = Gui::Selection().getObjectsOfType(App::GeoFeature::getClassTypeId());
    Gui::ViewProvider* vp = Application::Instance->getViewProvider(sel.front());
    // FIXME: Need a way to force 'Transform' edit mode
    // #0000477: Proper interface for edit modes of view provider
    if (vp)
        getActiveGuiDocument()->setEdit(vp, Gui::ViewProvider::Transform);
}

bool StdCmdTransformManip::isActive(void)
{
    return Gui::Selection().countObjectsOfType(App::GeoFeature::getClassTypeId()) == 1;
}

//===========================================================================
// Std_Alignment
//===========================================================================
DEF_STD_CMD_A(StdCmdAlignment);

StdCmdAlignment::StdCmdAlignment()
  : Command("Std_Alignment")
{
    sGroup        = QT_TR_NOOP("Edit");
    sMenuText     = QT_TR_NOOP("Alignment...");
    sToolTipText  = QT_TR_NOOP("Align the selected objects");
    sStatusTip    = QT_TR_NOOP("Align the selected objects");
    sWhatsThis    = "Std_Alignment";
}

void StdCmdAlignment::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    std::vector<App::DocumentObject*> sel = Gui::Selection().getObjectsOfType
        (App::GeoFeature::getClassTypeId());
    ManualAlignment* align = ManualAlignment::instance();
    QObject::connect(align, SIGNAL(emitCanceled()), align, SLOT(deleteLater()));
    QObject::connect(align, SIGNAL(emitFinished()), align, SLOT(deleteLater()));

    // Get the fixed and moving meshes
    FixedGroup fixedGroup;
    std::map<int, MovableGroup> groupMap;
    fixedGroup.addView(sel[0]);
    groupMap[0].addView(sel[1]);

    // add the fixed group
    align->setFixedGroup(fixedGroup);

    // create the model of movable groups
    MovableGroupModel model;
    model.addGroups(groupMap);
    align->setModel(model);
    Base::Type style = Base::Type::fromName("Gui::CADNavigationStyle");
    Base::Vector3d upDir(0,1,0), viewDir(0,0,-1);
    Gui::Document* doc = Application::Instance->activeDocument();
    if (doc) {
        View3DInventor* mdi = qobject_cast<View3DInventor*>(doc->getActiveView());
        if (mdi) {
            View3DInventorViewer* viewer = mdi->getViewer();
            SoCamera* camera = viewer->getSoRenderManager()->getCamera();
            if (camera) {
                SbVec3f up(0,1,0), dir(0,0,-1);
                camera->orientation.getValue().multVec(dir, dir);
                viewDir.Set(dir[0],dir[1],dir[2]);
                camera->orientation.getValue().multVec(up, up);
                upDir.Set(up[0],up[1],up[2]);
            }
            style = viewer->navigationStyle()->getTypeId();
        }
    }

    align->setMinPoints(1);
    align->startAlignment(style);
    align->setViewingDirections(viewDir,upDir, viewDir,upDir);
    Gui::Selection().clearSelection();
}

bool StdCmdAlignment::isActive(void)
{
    if (ManualAlignment::hasInstance())
        return false;
    return Gui::Selection().countObjectsOfType(App::GeoFeature::getClassTypeId()) == 2;
}

//===========================================================================
// Std_Edit
//===========================================================================
DEF_STD_CMD_A(StdCmdEdit);

StdCmdEdit::StdCmdEdit()
  : Command("Std_Edit")
{
    sGroup        = QT_TR_NOOP("Edit");
    sMenuText     = QT_TR_NOOP("Toggle &Edit mode");
    sToolTipText  = QT_TR_NOOP("Toggles the selected object's edit mode");
    sWhatsThis    = "Std_Edit";
    sStatusTip    = QT_TR_NOOP("Enters or leaves the selected object's edit mode");
#if QT_VERSION >= 0x040200
    sPixmap       = "edit-edit";
#endif
    eType         = ForEdit;
}

void StdCmdEdit::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        if (viewer->isEditingViewProvider()) {
            doCommand(Command::Gui,"Gui.activeDocument().resetEdit()");
        } else {
            if (Selection().getCompleteSelection().size() > 0) {
                SelectionSingleton::SelObj obj = Selection().getCompleteSelection()[0];
                doCommand(Command::Gui,"Gui.activeDocument().setEdit(\"%s\",0)",obj.FeatName);
            }
        }
    }
}

bool StdCmdEdit::isActive(void)
{
    return (Selection().getCompleteSelection().size() > 0) || (Gui::Control().activeDialog() != 0);
}

//======================================================================
// StdCmdExpression
//===========================================================================
class StdCmdExpression : public Gui::Command
{
public:
    StdCmdExpression() : Command("Std_Expressions")
    {
        sGroup        = QT_TR_NOOP("Edit");
        sMenuText     = QT_TR_NOOP("Expression actions");
        sToolTipText  = QT_TR_NOOP("Expression actions");
        sWhatsThis    = "Std_Expressions";
        sStatusTip    = QT_TR_NOOP("Expression actions");
        eType         = ForEdit;
    }

    virtual const char* className() const {return "StdCmdExpression";}
protected:

    virtual void activated(int iMsg) {
        std::map<App::Document*, std::set<App::DocumentObject*> > objs;
        switch(iMsg) {
        case 0:
            for(auto &sel : Selection().getCompleteSelection())
                objs[sel.pObject->getDocument()].insert(sel.pObject);
            break;
        case 1:
            if(App::GetApplication().getActiveDocument()) {
                auto doc = App::GetApplication().getActiveDocument();
                auto array = doc->getObjects();
                auto &set = objs[doc];
                set.insert(array.begin(),array.end());
            }
            break;
        case 2:
            for(auto doc : App::GetApplication().getDocuments()) {
                auto &set = objs[doc];
                auto array = doc->getObjects();
                set.insert(array.begin(),array.end());
            }
            break;
        case 3:
            pasteExpressions();
            break;
        }
        copyExpressions(objs);
    }

    virtual Gui::Action * createAction(void) {
        ActionGroup* pcAction = new ActionGroup(this, getMainWindow());
        pcAction->setDropDownMenu(true);
        applyCommandData(this->className(), pcAction);

        pcActionCopySel = pcAction->addAction(QObject::tr("Copy selected"));
        pcActionCopyActive = pcAction->addAction(QObject::tr("Copy active document"));
        pcActionCopyAll = pcAction->addAction(QObject::tr("Copy all documents"));
        pcActionPaste = pcAction->addAction(QObject::tr("Paste"));

        return pcAction;
    }

    void copyExpressions(const std::map<App::Document*, std::set<App::DocumentObject*> > &objs) {
        std::ostringstream ss;
        std::vector<App::Property*> props;
        for(auto &v : objs) {
            for(auto obj : v.second) {
                props.clear();
                obj->getPropertyList(props);
                for(auto prop : props) {
                    auto p = dynamic_cast<App::PropertyExpressionContainer*>(prop);
                    if(!p) continue;
                    for(auto &v : p->getExpressions()) {
                        ss << "##@@ " << v.first.toString() << ' '
                           << obj->getFullName() << '.' << p->getName()
                           << " (" << obj->Label.getValue() << ')' << std::endl;
                        ss << "##@@";
                        if(v.second->comment.size()) {
                            if(v.second->comment[0] == '&' 
                                    || v.second->comment.find('\n') != std::string::npos
                                    || v.second->comment.find('\r') != std::string::npos)
                            {
                                std::string comment = v.second->comment;
                                boost::replace_all(comment,"&","&amp;");
                                boost::replace_all(comment,"\n","&#10;");
                                boost::replace_all(comment,"\r","&#13;");
                                ss << '&' << comment;
                            }else
                                ss << v.second->comment;
                        }
                        ss << std::endl << v.second->toStr(true,true) << std::endl << std::endl;
                    }
                }
            }
        }
        QApplication::clipboard()->setText(QString::fromUtf8(ss.str().c_str()));
    }

    void pasteExpressions() {
        std::map<App::Document*, std::map<App::PropertyExpressionContainer*, 
            std::map<App::ObjectIdentifier, App::ExpressionPtr> > > exprs;

        bool failed = false;
        std::string txt = QApplication::clipboard()->text().toUtf8().constData();
        const char *tstart = txt.c_str();
        const char *tend = tstart + txt.size();

        static boost::regex rule("^##@@ ([^ ]+) (\\w+)#(\\w+)\\.(\\w+) [^\n]+\n##@@([^\n]*)\n");
        boost::cmatch m;
        if(!boost::regex_search(tstart,m,rule)) {
            FC_WARN("No expression header found");
            return;
        }
        boost::cmatch m2;
        bool found = true;
        for(;found;m=m2) {
            found = boost::regex_search(m[0].second,tend,m2,rule);

            auto pathName = m.str(1);
            auto docName = m.str(2);
            auto objName = m.str(3);
            auto propName = m.str(4);
            auto comment = m.str(5);

            App::Document *doc = App::GetApplication().getDocument(docName.c_str());
            if(!doc) {
                FC_WARN("Cannot find document '" << docName << "'");
                continue;
            }

            auto obj = doc->getObject(objName.c_str());
            if(!obj) {
                FC_WARN("Cannot find object '" << docName << '#' << objName << "'");
                continue;
            }

            auto prop = dynamic_cast<App::PropertyExpressionContainer*>(
                    obj->getPropertyByName(propName.c_str()));
            if(!prop) {
                FC_WARN("Invalid property '" << docName << '#' << objName << '.' << propName << "'");
                continue;
            }

            size_t len = (found?m2[0].first:tend) - m[0].second;
            try {
                auto expr = App::Expression::parse(obj,m[0].second,len,true);
                if(expr && comment.size()) {
                    if(comment[0] == '&') {
                        expr->comment = comment.c_str()+1;
                        boost::replace_all(expr->comment,"&amp;","&");
                        boost::replace_all(expr->comment,"&#10;","\n");
                        boost::replace_all(expr->comment,"&#13;","\r");
                    } else
                        expr->comment = comment;
                }
                exprs[doc][prop][App::ObjectIdentifier::parse(obj,pathName)] = std::move(expr);
            } catch(Base::Exception &e) {
                FC_ERR(e.what() << std::endl << m[0].str());
                failed = true;
            }
        }
        if(failed) {
            QMessageBox::critical(getMainWindow(), QObject::tr("Expression error"),
                QObject::tr("Failed to parse some of the expressions.\n"
                            "Please check the Report View for more details."));
            return;
        }

        App::GetApplication().setActiveTransaction("Paste expressions");
        try {
            for(auto &v : exprs) {
                for(auto &v2 : v.second) {
                    auto &expressions = v2.second;
                    auto old = v2.first->getExpressions();
                    for(auto it=expressions.begin(),itNext=it;it!=expressions.end();it=itNext) {
                        ++itNext;
                        auto iter = old.find(it->first);
                        if(iter != old.end() && it->second->isSame(*iter->second))
                            expressions.erase(it);
                    }
                    if(expressions.size())
                        v2.first->setExpressions(std::move(expressions));
                }
            }
            App::GetApplication().closeActiveTransaction(false);
        } catch (const Base::Exception& e) {
            QMessageBox::critical(getMainWindow(), QObject::tr("Failed to paste expressions"),
                QString::fromLatin1(e.what()));
            App::GetApplication().closeActiveTransaction(true);
            e.ReportException();
        }
    }

    bool isActive() {
        if(!App::GetApplication().getActiveDocument()) {
            pcActionCopyAll->setEnabled(false);
            pcActionCopySel->setEnabled(false);
            pcActionCopyActive->setEnabled(false);
            pcActionPaste->setEnabled(false);
            return true;
        }
        pcActionCopyActive->setEnabled(true);
        pcActionCopyAll->setEnabled(true);
        pcActionCopySel->setEnabled(Selection().hasSelection());

        pcActionPaste->setEnabled(
                QApplication::clipboard()->text().startsWith(QLatin1String("##@@ ")));
        return true;
    }

    QAction *pcActionCopyAll;
    QAction *pcActionCopySel;
    QAction *pcActionCopyActive;
    QAction *pcActionPaste;
};

namespace Gui {

void CreateDocCommands(void)
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();

    rcCmdMgr.addCommand(new StdCmdNew());
    rcCmdMgr.addCommand(new StdCmdOpen());
    rcCmdMgr.addCommand(new StdCmdImport());
    rcCmdMgr.addCommand(new StdCmdExport());
    rcCmdMgr.addCommand(new StdCmdMergeProjects());
    rcCmdMgr.addCommand(new StdCmdExportGraphviz());

    rcCmdMgr.addCommand(new StdCmdSave());
    rcCmdMgr.addCommand(new StdCmdSaveAs());
    rcCmdMgr.addCommand(new StdCmdSaveCopy());
    rcCmdMgr.addCommand(new StdCmdSaveAll());
    rcCmdMgr.addCommand(new StdCmdRevert());
    rcCmdMgr.addCommand(new StdCmdProjectInfo());
    rcCmdMgr.addCommand(new StdCmdProjectUtil());
    rcCmdMgr.addCommand(new StdCmdUndo());
    rcCmdMgr.addCommand(new StdCmdRedo());
    rcCmdMgr.addCommand(new StdCmdPrint());
    rcCmdMgr.addCommand(new StdCmdPrintPreview());
    rcCmdMgr.addCommand(new StdCmdPrintPdf());
    rcCmdMgr.addCommand(new StdCmdQuit());
    rcCmdMgr.addCommand(new StdCmdCut());
    rcCmdMgr.addCommand(new StdCmdCopy());
    rcCmdMgr.addCommand(new StdCmdPaste());
    rcCmdMgr.addCommand(new StdCmdDuplicateSelection());
    rcCmdMgr.addCommand(new StdCmdSelectAll());
    rcCmdMgr.addCommand(new StdCmdDelete());
    rcCmdMgr.addCommand(new StdCmdRefresh());
    rcCmdMgr.addCommand(new StdCmdTransform());
    rcCmdMgr.addCommand(new StdCmdPlacement());
    rcCmdMgr.addCommand(new StdCmdTransformManip());
    rcCmdMgr.addCommand(new StdCmdAlignment());
    rcCmdMgr.addCommand(new StdCmdEdit());
    rcCmdMgr.addCommand(new StdCmdExpression());
}

} // namespace Gui

