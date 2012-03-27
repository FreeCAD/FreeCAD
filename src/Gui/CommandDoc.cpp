/***************************************************************************
 *   Copyright (c) 2002 J�rgen Riegel <juergen.riegel@web.de>              *
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
# include <QClipboard>
# include <QStatusBar>
# include <QPointer>
#endif
#include <algorithm>

#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Sequencer.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>

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

using namespace Gui;


//===========================================================================
// Std_Open
//===========================================================================

DEF_STD_CMD(StdCmdOpen);

StdCmdOpen::StdCmdOpen()
  : Command("Std_Open")
{
    // seting the
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
    // load the files with the associated modules
    SelectModule::Dict dict = SelectModule::importHandler(fileList, selectedFilter);
    for (SelectModule::Dict::iterator it = dict.begin(); it != dict.end(); ++it) {
        getGuiApplication()->open(it.key().toUtf8(), it.value().toAscii());
    }
}

//===========================================================================
// Std_Import
//===========================================================================

DEF_STD_CMD_A(StdCmdImport);

StdCmdImport::StdCmdImport()
  : Command("Std_Import")
{
    // seting the
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

    QString selectedFilter;
    QStringList fileList = FileDialog::getOpenFileNames(getMainWindow(),
        QObject::tr("Import file"), QString(), formatList, &selectedFilter);
    SelectModule::Dict dict = SelectModule::importHandler(fileList, selectedFilter);
    // load the files with the associated modules
    for (SelectModule::Dict::iterator it = dict.begin(); it != dict.end(); ++it) {
        getGuiApplication()->importFrom(it.key().toUtf8(),
            getActiveGuiDocument()->getDocument()->getName(),
            it.value().toAscii());
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
    // seting the
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

    QString selectedFilter;
    QString fileName = FileDialog::getSaveFileName(getMainWindow(),
        QObject::tr("Export file"), QString(), formatList, &selectedFilter);
    if (!fileName.isEmpty()) {
        SelectModule::Dict dict = SelectModule::exportHandler(fileName, selectedFilter);
        // export the files with the associated modules
        for (SelectModule::Dict::iterator it = dict.begin(); it != dict.end(); ++it) {
            getGuiApplication()->exportTo(it.key().toUtf8(),
                getActiveGuiDocument()->getDocument()->getName(),
                it.value().toAscii());
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
    sWhatsThis    = QT_TR_NOOP("Merge project");
    sStatusTip    = QT_TR_NOOP("Merge project");
}

void StdCmdMergeProjects::activated(int iMsg)
{
    QString exe = QString::fromUtf8(App::GetApplication().getExecutableName());
    QString project = QFileDialog::getOpenFileName(Gui::getMainWindow(),
        QString::fromUtf8(QT_TR_NOOP("Merge project")), QString(),
        QString::fromUtf8(QT_TR_NOOP("%1 document (*.fcstd)")).arg(exe));
    if (!project.isEmpty()) {
        App::Document* doc = App::GetApplication().getActiveDocument();
        QFileInfo info(QString::fromUtf8(doc->FileName.getValue()));
        QFileInfo proj(project);
        if (proj == info) {
            QMessageBox::critical(Gui::getMainWindow(),
                QString::fromUtf8(QT_TR_NOOP("Merge project")),
                QString::fromUtf8(QT_TR_NOOP("Cannot merge project with itself.")));
            return;
        }

        QString dir1 = proj.absoluteDir().filePath(proj.baseName());
        QString dir2 = info.absoluteDir().filePath(info.baseName());

        Base::FileInfo fi((const char*)project.toUtf8());
        Base::ifstream str(fi, std::ios::in | std::ios::binary);
        MergeDocuments md(doc);
        md.importObjects(str);
    }
}

bool StdCmdMergeProjects::isActive(void)
{
    return this->hasActiveDocument();
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
  doCommand(Command::Doc,"App.newDocument()");
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
}

void StdCmdSaveAs::activated(int iMsg)
{
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
// Std_ProjectInfo
//===========================================================================

DEF_STD_CMD_A(StdCmdProjectInfo);

StdCmdProjectInfo::StdCmdProjectInfo()
  :Command("Std_ProjectInfo")
{
  // seting the 
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
    // seting the 
    sGroup        = QT_TR_NOOP("Tools");
    sWhatsThis    = "Std_ProjectUtil";
    sMenuText     = QT_TR_NOOP("Project utility...");
    sToolTipText  = QT_TR_NOOP("Utility to extract or create project files");
    sStatusTip    = QT_TR_NOOP("Utility to extract or create project files");
}

void StdCmdProjectUtil::activated(int iMsg)
{
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
    if (getMainWindow()->activeWindow()) {
        getMainWindow()->statusBar()->showMessage(QObject::tr("Printing..."));
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
}

void StdCmdPrintPreview::activated(int iMsg)
{
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
    if (getMainWindow()->activeWindow()) {
        getMainWindow()->statusBar()->showMessage(QObject::tr("Exporting PDF..."));
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
  sPixmap       = "system-log-out";
#endif
  sAccel        = "Alt+F4";
}

void StdCmdQuit::activated(int iMsg)
{
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
    applyCommandData(pcAction);
    if (sPixmap)
        pcAction->setIcon(Gui::BitmapFactory().pixmap(sPixmap));
    pcAction->setShortcut(QString::fromAscii(sAccel));

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
    applyCommandData(pcAction);
    if (sPixmap)
        pcAction->setIcon(Gui::BitmapFactory().pixmap(sPixmap));
    pcAction->setShortcut(QString::fromAscii(sAccel));

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
    bool done = getGuiApplication()->sendMsgToActiveView("Copy");
    if (!done) {
        WaitCursor wc;
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

DEF_STD_CMD_A(StdCmdDDuplicateSelection);

StdCmdDDuplicateSelection::StdCmdDDuplicateSelection()
  :Command("Std_DuplicateSelection")
{
    sAppModule    = "Edit";
    sGroup        = QT_TR_NOOP("Edit");
    sMenuText     = QT_TR_NOOP("Duplicate selection");
    sToolTipText  = QT_TR_NOOP("Put duplicates of the selected objects to the active document");
    sWhatsThis    = QT_TR_NOOP("Put duplicates of the selected objects to the active document");
    sStatusTip    = QT_TR_NOOP("Put duplicates of the selected objects to the active document");
}

void StdCmdDDuplicateSelection::activated(int iMsg)
{
    App::Document* act = App::GetApplication().getActiveDocument();
    if (!act)
        return; // no active document found
    Gui::Document* doc = Gui::Application::Instance->getDocument(act);
    std::vector<Gui::SelectionSingleton::SelObj> sel = Gui::Selection().getCompleteSelection();
    for (std::vector<SelectionSingleton::SelObj>::iterator it = sel.begin(); it != sel.end(); ++it) {
        if (!it->pObject)
            continue; // should actually not happen
        // create a copy of the object
        App::DocumentObject* copy = act->copyObject(it->pObject, false);
        if (!copy) // continue if no copy could be created
            continue;
        // mark all properties of the copy as "touched" which are touched in the original object
        std::map<std::string,App::Property*> props;
        it->pObject->getPropertyMap(props);
        std::map<std::string,App::Property*> copy_props;
        copy->getPropertyMap(copy_props);
        for (std::map<std::string,App::Property*>::iterator jt = props.begin(); jt != props.end(); ++jt) {
            if (jt->second->isTouched()) {
                std::map<std::string,App::Property*>::iterator kt;
                kt = copy_props.find(jt->first);
                if (kt != copy_props.end()) {
                    kt->second->touch();
                }
            }
        }

        Gui::Document* parent = Gui::Application::Instance->getDocument(it->pObject->getDocument());
        if (!parent || !doc)
            continue; // should not happen
        // copy the properties of the associated view providers
        Gui::ViewProvider* view = parent->getViewProvider(it->pObject);
        Gui::ViewProvider* copy_view = doc->getViewProvider(copy);
        copy_view->addDynamicProperties(view);
        if (!view || !copy_view)
            continue; // should not happen

        // get the properties of the view provider
        props.clear();
        view->getPropertyMap(props);
        copy_props.clear();
        copy_view->getPropertyMap(copy_props);
        for (std::map<std::string,App::Property*>::iterator jt = props.begin(); jt != props.end(); ++jt) {
            std::map<std::string,App::Property*>::iterator kt;
            kt = copy_props.find(jt->first);
            if (kt != copy_props.end()) {
                std::auto_ptr<App::Property> data(jt->second->Copy());
                if (data.get()) {
                    kt->second->Paste(*data);
                }
            }
        }
    }
}

bool StdCmdDDuplicateSelection::isActive(void)
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
    // go through all documents
    const SelectionSingleton& rSel = Selection();
    const std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    for (std::vector<App::Document*>::const_iterator it = docs.begin(); it != docs.end(); ++it) {
        Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(*it);
        std::vector<Gui::SelectionObject> sel = rSel.getSelectionEx((*it)->getName());
        if (!sel.empty()) {
            (*it)->openTransaction("Delete");
            for (std::vector<Gui::SelectionObject>::iterator ft = sel.begin(); ft != sel.end(); ++ft) {
                Gui::ViewProvider* vp = pGuiDoc->getViewProvider(ft->getObject());
                if (vp) {
                    // ask the ViewProvider if its want to do some clean up
                    if (vp->onDelete(ft->getSubNames()))
                        doCommand(Doc,"App.getDocument(\"%s\").removeObject(\"%s\")"
                                 ,(*it)->getName(), ft->getFeatName());
                }
            }
            (*it)->commitTransaction();
        }
    }
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
  :Command("Std_Refresh")
{
  sGroup        = QT_TR_NOOP("Edit");
  sMenuText     = QT_TR_NOOP("&Refresh");
  sToolTipText  = QT_TR_NOOP("Recomputes the current active document");
  sWhatsThis    = "Std_Refresh";
  sStatusTip    = QT_TR_NOOP("Recomputes the current active document");
  sPixmap       = "view-refresh";
  sAccel        = keySequenceToAccel(QKeySequence::Refresh);
}

void StdCmdRefresh::activated(int iMsg)
{
  if ( getActiveGuiDocument() )
  {
    openCommand("Refresh active document");
    doCommand(Doc,"App.activeDocument().recompute()");
    commitCommand(); 
  }
}

bool StdCmdRefresh::isActive(void)
{
  return this->getDocument() && this->getDocument()->isTouched();
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
    Gui::Document* doc = Application::Instance->activeDocument();
    if (doc) {
        View3DInventor* mdi = qobject_cast<View3DInventor*>(doc->getActiveView());
        if (mdi) {
            style = mdi->getViewer()->navigationStyle()->getTypeId();
        }
    }

    align->setMinPoints(1);
    align->startAlignment(style);
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
  :Command("Std_Edit")
{
  sGroup        = QT_TR_NOOP("Edit");
  sMenuText     = QT_TR_NOOP("Toggle &Editmode");
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


namespace Gui {

void CreateDocCommands(void)
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();

    rcCmdMgr.addCommand(new StdCmdNew());
    rcCmdMgr.addCommand(new StdCmdOpen());
    rcCmdMgr.addCommand(new StdCmdImport());
    rcCmdMgr.addCommand(new StdCmdExport());
    rcCmdMgr.addCommand(new StdCmdMergeProjects());

    rcCmdMgr.addCommand(new StdCmdSave());
    rcCmdMgr.addCommand(new StdCmdSaveAs());
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
    rcCmdMgr.addCommand(new StdCmdDDuplicateSelection());
    rcCmdMgr.addCommand(new StdCmdSelectAll());
    rcCmdMgr.addCommand(new StdCmdDelete());
    rcCmdMgr.addCommand(new StdCmdRefresh());
    rcCmdMgr.addCommand(new StdCmdTransform());
    rcCmdMgr.addCommand(new StdCmdPlacement());
    rcCmdMgr.addCommand(new StdCmdAlignment());
    rcCmdMgr.addCommand(new StdCmdEdit());
}

} // namespace Gui

