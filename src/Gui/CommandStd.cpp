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
# include <QMessageBox>
# include <QSharedPointer>
# include <QWhatsThis>
#if QT_VERSION >= 0x040200
# include <QDesktopServices>
# include <QUrl>
#endif
#endif

#include <boost/scoped_ptr.hpp>

#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/Sequencer.h>
#include <App/Document.h>
#include "Action.h"
#include "Application.h"
#include "Document.h"
#include "Splashscreen.h"
#include "Command.h"
#include "MainWindow.h"
#include "WhatsThis.h"
#include "DlgUndoRedo.h"
#include "BitmapFactory.h"
#include "View.h"

#include "DlgParameterImp.h"
#include "DlgPreferencesImp.h"
#include "DlgCustomizeImp.h"
#include "Widgets.h"
#include "OnlineDocumentation.h"
#include "GuiConsole.h"
#include "WorkbenchManager.h"
#include "Workbench.h"
#include "Selection.h"
#include "DlgUnitsCalculatorImp.h"

using Base::Console;
using Base::Sequencer;
using namespace Gui;


//===========================================================================
// Std_Workbench
//===========================================================================

DEF_STD_CMD_AC(StdCmdWorkbench);

StdCmdWorkbench::StdCmdWorkbench()
  : Command("Std_Workbench")
{
    sGroup        = QT_TR_NOOP("View");
    sMenuText     = QT_TR_NOOP("Workbench");
    sToolTipText  = QT_TR_NOOP("Switch between workbenches");
    sWhatsThis    = "Std_Workbench";
    sStatusTip    = QT_TR_NOOP("Switch between workbenches");
    sPixmap       = "freecad";
    eType         = 0;
}

void StdCmdWorkbench::activated(int i)
{
    try {
        Workbench* w = WorkbenchManager::instance()->active();
        QList<QAction*> items = static_cast<WorkbenchGroup*>(_pcAction)->actions();
        std::string switch_to = (const char*)items[i]->objectName().toLatin1();
        if (w) {
            std::string current_w = w->name();
            if (switch_to == current_w)
                return;
        }
        doCommand(Gui, "Gui.activateWorkbench(\"%s\")", switch_to.c_str());
    }
    catch(const Base::PyException& e) {
        QString msg(QLatin1String(e.what()));
        // ignore '<type 'exceptions.*Error'>' prefixes
        QRegExp rx;
        rx.setPattern(QLatin1String("^\\s*<type 'exceptions.\\w*'>:\\s*"));
        int pos = rx.indexIn(msg);
        if (pos != -1)
            msg = msg.mid(rx.matchedLength());
        QMessageBox::critical(getMainWindow(), QObject::tr("Cannot load workbench"), msg); 
    }
    catch(...) {
        QMessageBox::critical(getMainWindow(), QObject::tr("Cannot load workbench"), 
            QObject::tr("A general error occurred while loading the workbench")); 
    }
}

bool StdCmdWorkbench::isActive(void)
{
    return true;
}

Action * StdCmdWorkbench::createAction(void)
{
    Action *pcAction;

    pcAction = new WorkbenchGroup(this,getMainWindow());
    pcAction->setShortcut(QString::fromLatin1(sAccel));
    applyCommandData(this->className(), pcAction);
    if (sPixmap)
        pcAction->setIcon(Gui::BitmapFactory().iconFromTheme(sPixmap));

    return pcAction;
}

//===========================================================================
// Std_RecentFiles
//===========================================================================

DEF_STD_CMD_C(StdCmdRecentFiles)

StdCmdRecentFiles::StdCmdRecentFiles()
  :Command("Std_RecentFiles")
{
    sGroup        = QT_TR_NOOP("File");
    sMenuText     = QT_TR_NOOP("Recent files");
    sToolTipText  = QT_TR_NOOP("Recent file list");
    sWhatsThis    = "Std_RecentFiles";
    sStatusTip    = QT_TR_NOOP("Recent file list");
    eType         = NoTransaction;
}

/**
 * Opens the recent file at position \a iMsg in the menu.
 * If the file does not exist or cannot be loaded this item is removed
 * from the list.
 */
void StdCmdRecentFiles::activated(int iMsg)
{
    RecentFilesAction* act = qobject_cast<RecentFilesAction*>(_pcAction);
    if (act) act->activateFile( iMsg );
}

/**
 * Creates the QAction object containing the recent files.
 */
Action * StdCmdRecentFiles::createAction(void)
{
    RecentFilesAction* pcAction = new RecentFilesAction(this, getMainWindow());
    pcAction->setObjectName(QLatin1String("recentFiles"));
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);
    return pcAction;
}

//===========================================================================
// Std_About
//===========================================================================

DEF_STD_CMD_ACL(StdCmdAbout)

StdCmdAbout::StdCmdAbout()
  :Command("Std_About")
{
    sGroup        = QT_TR_NOOP("Help");
    sMenuText     = QT_TR_NOOP("&About %1");
    sToolTipText  = QT_TR_NOOP("About %1");
    sWhatsThis    = "Std_About";
    sStatusTip    = QT_TR_NOOP("About %1");
    eType         = 0;
}

Action * StdCmdAbout::createAction(void)
{
    Action *pcAction;

    QString exe = qApp->applicationName();
    pcAction = new Action(this,getMainWindow());
    pcAction->setText(QCoreApplication::translate(
        this->className(), sMenuText).arg(exe));
    pcAction->setToolTip(QCoreApplication::translate(
        this->className(), sToolTipText).arg(exe));
    pcAction->setStatusTip(QCoreApplication::translate(
        this->className(), sStatusTip).arg(exe));
    pcAction->setWhatsThis(QLatin1String(sWhatsThis));
    pcAction->setIcon(QApplication::windowIcon());
    pcAction->setShortcut(QString::fromLatin1(sAccel));
#if QT_VERSION > 0x050000
    // Needs to have AboutRole set to avoid duplicates if adding the about action more than once on macOS
    pcAction->setMenuRole(QAction::AboutRole);
#else
    // With Qt 4.8, having AboutRole set causes it to disappear when readding it: issue #0001485
    pcAction->setMenuRole(QAction::ApplicationSpecificRole);
#endif
    return pcAction;
}

bool StdCmdAbout::isActive()
{
    return true;
}

/**
 * Shows information about the application.
 */
void StdCmdAbout::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    const Gui::Dialog::AboutDialogFactory* f = Gui::Dialog::AboutDialogFactory::defaultFactory();
    boost::scoped_ptr<QDialog> dlg(f->create(getMainWindow()));
    dlg->exec();
}

void StdCmdAbout::languageChange()
{
    if (_pcAction) {
        QString exe = qApp->applicationName();
        _pcAction->setText(QCoreApplication::translate(
            this->className(), sMenuText).arg(exe));
        _pcAction->setToolTip(QCoreApplication::translate(
            this->className(), sToolTipText).arg(exe));
        _pcAction->setStatusTip(QCoreApplication::translate(
            this->className(), sStatusTip).arg(exe));
        _pcAction->setWhatsThis(QLatin1String(sWhatsThis));
    }
}

//===========================================================================
// Std_AboutQt
//===========================================================================
DEF_STD_CMD(StdCmdAboutQt);

StdCmdAboutQt::StdCmdAboutQt()
  :Command("Std_AboutQt")
{
  sGroup        = QT_TR_NOOP("Help");
  sMenuText     = QT_TR_NOOP("About &Qt");
  sToolTipText  = QT_TR_NOOP("About Qt");
  sWhatsThis    = "Std_AboutQt";
  sStatusTip    = QT_TR_NOOP("About Qt");
  eType         = 0;
}

void StdCmdAboutQt::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    qApp->aboutQt();
}

//===========================================================================
// Std_WhatsThis
//===========================================================================
DEF_STD_CMD(StdCmdWhatsThis);

StdCmdWhatsThis::StdCmdWhatsThis()
  :Command("Std_WhatsThis")
{
    sGroup        = QT_TR_NOOP("Help");
    sMenuText     = QT_TR_NOOP("&What's This?");
    sToolTipText  = QT_TR_NOOP("What's This");
    sWhatsThis    = "Std_WhatsThis";
    sStatusTip    = QT_TR_NOOP("What's This");
    sAccel        = keySequenceToAccel(QKeySequence::WhatsThis);
    sPixmap       = "WhatsThis";
    eType         = 0;
}

void StdCmdWhatsThis::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    QWhatsThis::enterWhatsThisMode();
}

//===========================================================================
// Std_DlgParameter
//===========================================================================
DEF_STD_CMD(StdCmdDlgParameter);

StdCmdDlgParameter::StdCmdDlgParameter()
  :Command("Std_DlgParameter")
{
  sGroup        = QT_TR_NOOP("Tools");
  sMenuText     = QT_TR_NOOP("E&dit parameters ...");
  sToolTipText  = QT_TR_NOOP("Opens a Dialog to edit the parameters");
  sWhatsThis    = "Std_DlgParameter";
  sStatusTip    = QT_TR_NOOP("Opens a Dialog to edit the parameters");
  //sPixmap     = "settings";
  eType         = 0;
}

void StdCmdDlgParameter::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    Gui::Dialog::DlgParameterImp cDlg(getMainWindow());
    cDlg.resize(QSize(800, 600));
    cDlg.exec();
}

//===========================================================================
// Std_DlgPreferences
//===========================================================================
DEF_STD_CMD_C(StdCmdDlgPreferences);

StdCmdDlgPreferences::StdCmdDlgPreferences()
  :Command("Std_DlgPreferences")
{
    sGroup        = QT_TR_NOOP("Tools");
    sMenuText     = QT_TR_NOOP("&Preferences ...");
    sToolTipText  = QT_TR_NOOP("Opens a Dialog to edit the preferences");
    sWhatsThis    = "Std_DlgPreferences";
    sStatusTip    = QT_TR_NOOP("Opens a Dialog to edit the preferences");
    sPixmap     = "preferences-system";
    eType         = 0;
}

Action * StdCmdDlgPreferences::createAction(void)
{
    Action *pcAction = Command::createAction();
    pcAction->setMenuRole(QAction::PreferencesRole);
    return pcAction;
}

void StdCmdDlgPreferences::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    Gui::Dialog::DlgPreferencesImp cDlg(getMainWindow());
    cDlg.exec();
}

//===========================================================================
// Std_DlgCustomize
//===========================================================================
DEF_STD_CMD(StdCmdDlgCustomize);

StdCmdDlgCustomize::StdCmdDlgCustomize()
  :Command("Std_DlgCustomize")
{
    sGroup        = QT_TR_NOOP("Tools");
    sMenuText     = QT_TR_NOOP("Cu&stomize...");
    sToolTipText  = QT_TR_NOOP("Customize toolbars and command bars");
    sWhatsThis    = "Std_DlgCustomize";
    sStatusTip    = QT_TR_NOOP("Customize toolbars and command bars");
    sPixmap       = "applications-accessories";
    eType         = 0;
}

void StdCmdDlgCustomize::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    static QPointer<QDialog> dlg = 0;
    if (!dlg)
        dlg = new Gui::Dialog::DlgCustomizeImp(getMainWindow());
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

//===========================================================================
// Std_CommandLine
//===========================================================================
DEF_STD_CMD(StdCmdCommandLine);

StdCmdCommandLine::StdCmdCommandLine()
  :Command("Std_CommandLine")
{
    sGroup        = QT_TR_NOOP("Tools");
    sMenuText     = QT_TR_NOOP("Start command &line...");
    sToolTipText  = QT_TR_NOOP("Opens the command line in the console");
    sWhatsThis    = "Std_CommandLine";
    sStatusTip    = QT_TR_NOOP("Opens the command line in the console");
    sPixmap       = "utilities-terminal";
    eType         = 0;
}

void StdCmdCommandLine::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    bool show = getMainWindow()->isMaximized ();

    // pop up the Gui command window
    GUIConsole Wnd;

    getMainWindow()->showMinimized () ;
    qApp->processEvents();

    // create temporary console sequencer
    {
          Base::ConsoleSequencer seq;
          Base::Interpreter().runCommandLine("Console mode");
    }

#ifdef Q_WS_X11
    // On X11 this may not work. For further information see QWidget::showMaximized
    //
    // workaround for X11
    getMainWindow()->hide();
    getMainWindow()->show();
#endif

    // pop up the main window
    show ? getMainWindow()->showMaximized () : getMainWindow()->showNormal () ;
    qApp->processEvents();
}

//===========================================================================
// Std_OnlineHelp
//===========================================================================

DEF_STD_CMD(StdCmdOnlineHelp);

StdCmdOnlineHelp::StdCmdOnlineHelp()
  :Command("Std_OnlineHelp")
{
    sGroup        = QT_TR_NOOP("Help");
    sMenuText     = QT_TR_NOOP("Help");
    sToolTipText  = QT_TR_NOOP("Show help to the application");
    sWhatsThis    = "Std_OnlineHelp";
    sStatusTip    = QT_TR_NOOP("Help");
    sPixmap       = "help-browser";
    sAccel        = keySequenceToAccel(QKeySequence::HelpContents);
    eType         = 0;
}

void StdCmdOnlineHelp::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    Gui::getMainWindow()->showDocumentation(QString::fromLatin1("Online_Help_Startpage"));
}

//===========================================================================
// Std_OnlineHelpWebsite
//===========================================================================

DEF_STD_CMD(StdCmdOnlineHelpWebsite);

StdCmdOnlineHelpWebsite::StdCmdOnlineHelpWebsite()
  :Command("Std_OnlineHelpWebsite")
{
    sGroup        = QT_TR_NOOP("Help");
    sMenuText     = QT_TR_NOOP("Help Website");
    sToolTipText  = QT_TR_NOOP("The website where the help is maintained");
    sWhatsThis    = "Std_OnlineHelpWebsite";
    sStatusTip    = QT_TR_NOOP("Help Website");
    eType         = 0;
}

void StdCmdOnlineHelpWebsite::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    std::string defaulturl = QCoreApplication::translate(this->className(),"http://www.freecadweb.org/wiki/Online_Help_Toc").toStdString();
    ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Websites");
    std::string url = hURLGrp->GetASCII("OnlineHelp", defaulturl.c_str());
    hURLGrp->SetASCII("OnlineHelp", url.c_str());
    OpenURLInBrowser(url.c_str());
}

//===========================================================================
// Std_FreeCADWebsite
//===========================================================================

DEF_STD_CMD(StdCmdFreeCADWebsite);

StdCmdFreeCADWebsite::StdCmdFreeCADWebsite()
  :Command("Std_FreeCADWebsite")
{
    sGroup        = QT_TR_NOOP("Help");
    sMenuText     = QT_TR_NOOP("FreeCAD Website");
    sToolTipText  = QT_TR_NOOP("The FreeCAD website");
    sWhatsThis    = "Std_FreeCADWebsite";
    sStatusTip    = QT_TR_NOOP("FreeCAD Website");
    sPixmap       = "internet-web-browser";
    eType         = 0;
}

void StdCmdFreeCADWebsite::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    std::string defaulturl = QCoreApplication::translate(this->className(),"http://www.freecadweb.org").toStdString();
    ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Websites");
    std::string url = hURLGrp->GetASCII("WebPage", defaulturl.c_str());
    hURLGrp->SetASCII("WebPage", url.c_str());
    OpenURLInBrowser(url.c_str());
}

//===========================================================================
// Std_FreeCADUserHub
//===========================================================================

DEF_STD_CMD(StdCmdFreeCADUserHub);

StdCmdFreeCADUserHub::StdCmdFreeCADUserHub()
  :Command("Std_FreeCADUserHub")
{
    sGroup        = QT_TR_NOOP("Help");
    sMenuText     = QT_TR_NOOP("Users documentation");
    sToolTipText  = QT_TR_NOOP("Documentation for users on the FreeCAD website");
    sWhatsThis    = "Std_FreeCADUserHub";
    sStatusTip    = QT_TR_NOOP("Users documentation");
    sPixmap       = "internet-web-browser";
    eType         = 0;
}

void StdCmdFreeCADUserHub::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    std::string defaulturl = QCoreApplication::translate(this->className(),"http://www.freecadweb.org/wiki/User_hub").toStdString();
    ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Websites");
    std::string url = hURLGrp->GetASCII("Documentation", defaulturl.c_str());
    hURLGrp->SetASCII("Documentation", url.c_str());
    OpenURLInBrowser(url.c_str());
}

//===========================================================================
// Std_FreeCADPowerUserHub
//===========================================================================

DEF_STD_CMD(StdCmdFreeCADPowerUserHub);

StdCmdFreeCADPowerUserHub::StdCmdFreeCADPowerUserHub()
  :Command("Std_FreeCADPowerUserHub")
{
    sGroup        = QT_TR_NOOP("Help");
    sMenuText     = QT_TR_NOOP("Python scripting documentation");
    sToolTipText  = QT_TR_NOOP("Python scripting documentation on the FreeCAD website");
    sWhatsThis    = "Std_FreeCADPowerUserHub";
    sStatusTip    = QT_TR_NOOP("PowerUsers documentation");
    sPixmap       = "internet-web-browser";
    eType         = 0;
}

void StdCmdFreeCADPowerUserHub::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    std::string defaulturl = QCoreApplication::translate(this->className(),"http://www.freecadweb.org/wiki/Power_users_hub").toStdString();
    ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Websites");
    std::string url = hURLGrp->GetASCII("PowerUsers", defaulturl.c_str());
    hURLGrp->SetASCII("PowerUsers", url.c_str());
    OpenURLInBrowser(url.c_str());
}

//===========================================================================
// Std_FreeCADForum
//===========================================================================

DEF_STD_CMD(StdCmdFreeCADForum);

StdCmdFreeCADForum::StdCmdFreeCADForum()
  :Command("Std_FreeCADForum")
{
    sGroup        = QT_TR_NOOP("Help");
    sMenuText     = QT_TR_NOOP("FreeCAD Forum");
    sToolTipText  = QT_TR_NOOP("The FreeCAD forum, where you can find help from other users");
    sWhatsThis    = "Std_FreeCADForum";
    sStatusTip    = QT_TR_NOOP("The FreeCAD Forum");
    sPixmap       = "internet-web-browser";
    eType         = 0;
}

void StdCmdFreeCADForum::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    std::string defaulturl = QCoreApplication::translate(this->className(),"http://forum.freecadweb.org").toStdString();
    ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Websites");
    std::string url = hURLGrp->GetASCII("UserForum", defaulturl.c_str());
    hURLGrp->SetASCII("UserForum", url.c_str());
    OpenURLInBrowser(url.c_str());
}

//===========================================================================
// Std_FreeCADFAQ
//===========================================================================

DEF_STD_CMD(StdCmdFreeCADFAQ);

StdCmdFreeCADFAQ::StdCmdFreeCADFAQ()
  :Command("Std_FreeCADFAQ")
{
    sGroup        = QT_TR_NOOP("Help");
    sMenuText     = QT_TR_NOOP("FreeCAD FAQ");
    sToolTipText  = QT_TR_NOOP("Frequently Asked Questions on the FreeCAD website");
    sWhatsThis    = "Std_FreeCADFAQ";
    sStatusTip    = QT_TR_NOOP("Frequently Asked Questions");
    sPixmap       = "internet-web-browser";
    eType         = 0;
}

void StdCmdFreeCADFAQ::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    std::string defaulturl = QCoreApplication::translate(this->className(),"http://www.freecadweb.org/wiki/FAQ").toStdString();
    ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Websites");
    std::string url = hURLGrp->GetASCII("FAQ", defaulturl.c_str());
    hURLGrp->SetASCII("FAQ", url.c_str());
    OpenURLInBrowser(url.c_str());
}

//===========================================================================
// Std_PythonWebsite
//===========================================================================

DEF_STD_CMD(StdCmdPythonWebsite);

StdCmdPythonWebsite::StdCmdPythonWebsite()
  :Command("Std_PythonWebsite")
{
    sGroup        = QT_TR_NOOP("Help");
    sMenuText     = QT_TR_NOOP("Python Website");
    sToolTipText  = QT_TR_NOOP("The official Python website");
    sWhatsThis    = "Std_PythonWebsite";
    sStatusTip    = QT_TR_NOOP("Python Website");
    sPixmap       = "applications-python";
    eType         = 0;
}

void StdCmdPythonWebsite::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    OpenURLInBrowser("http://python.org");
}

//===========================================================================
// Std_MeasurementSimple
//===========================================================================

DEF_STD_CMD(StdCmdMeasurementSimple);

StdCmdMeasurementSimple::StdCmdMeasurementSimple()
  :Command("Std_MeasurementSimple")
{
    sGroup        = QT_TR_NOOP("Tools");
    sMenuText     = QT_TR_NOOP("Measure distance");
    sToolTipText  = QT_TR_NOOP("Measures distance between two selected objects");
    sWhatsThis    = "Std_MeasurementSimple";
    sStatusTip    = QT_TR_NOOP("Measures distance between two selected objects");
    sPixmap       = "view-measurement";
    eType         = 0;
}

void StdCmdMeasurementSimple::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    unsigned int n = getSelection().countObjectsOfType(App::DocumentObject::getClassTypeId());
 
    if (n == 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Only one object selected. Please select two objects.\n"
                        "Be aware the point where you click matters."));
        return;
    }
    if (n < 1 || n > 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Please select two objects.\n"
                        "Be aware the point where you click matters."));
        return;
    }

    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();

    std::string name;
    name += "Dist ";
    name += Sel[0].FeatName;
    name += "-";
    name += Sel[0].SubName;
    name += " to ";
    name += Sel[1].FeatName;
    name += "-";
    name += Sel[1].SubName;

    openCommand("Insert measurement");
    doCommand(Doc,"_f = App.activeDocument().addObject(\"App::MeasureDistance\",\"%s\")","Measurement");
    doCommand(Doc,"_f.Label ='%s'",name.c_str());
    doCommand(Doc,"_f.P1 = FreeCAD.Vector(%f,%f,%f)",Sel[0].x,Sel[0].y,Sel[0].z);
    doCommand(Doc,"_f.P2 = FreeCAD.Vector(%f,%f,%f)",Sel[1].x,Sel[1].y,Sel[1].z);
    updateActive();
    commitCommand();
}
//===========================================================================
// Std_UnitsCalculator
//===========================================================================
DEF_STD_CMD(StdCmdUnitsCalculator);

StdCmdUnitsCalculator::StdCmdUnitsCalculator()
  : Command("Std_UnitsCalculator")
{
    sGroup        = QT_TR_NOOP("Tools");
    sMenuText     = QT_TR_NOOP("&Units calculator...");
    sToolTipText  = QT_TR_NOOP("Start the units calculator");
    sWhatsThis    = "Std_UnitsCalculator";
    sStatusTip    = QT_TR_NOOP("Start the units calculator");
    sPixmap       = "accessories-calculator";
    eType         = 0;
}

void StdCmdUnitsCalculator::activated(int iMsg)
{
    Q_UNUSED(iMsg); 
    Gui::Dialog::DlgUnitsCalculator *dlg = new Gui::Dialog::DlgUnitsCalculator( getMainWindow() );
    dlg->show();
}

namespace Gui {

void CreateStdCommands(void)
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();

    rcCmdMgr.addCommand(new StdCmdAbout());
    rcCmdMgr.addCommand(new StdCmdAboutQt());

    rcCmdMgr.addCommand(new StdCmdDlgParameter());
    rcCmdMgr.addCommand(new StdCmdDlgPreferences());
    rcCmdMgr.addCommand(new StdCmdDlgCustomize());
    rcCmdMgr.addCommand(new StdCmdCommandLine());
    rcCmdMgr.addCommand(new StdCmdWorkbench());
    rcCmdMgr.addCommand(new StdCmdRecentFiles());
    rcCmdMgr.addCommand(new StdCmdWhatsThis());
    rcCmdMgr.addCommand(new StdCmdPythonHelp());
    rcCmdMgr.addCommand(new StdCmdOnlineHelp());
    rcCmdMgr.addCommand(new StdCmdOnlineHelpWebsite());
    rcCmdMgr.addCommand(new StdCmdFreeCADWebsite());
    rcCmdMgr.addCommand(new StdCmdFreeCADUserHub());
    rcCmdMgr.addCommand(new StdCmdFreeCADPowerUserHub());
    rcCmdMgr.addCommand(new StdCmdFreeCADForum());
    rcCmdMgr.addCommand(new StdCmdFreeCADFAQ());
    rcCmdMgr.addCommand(new StdCmdPythonWebsite());
    rcCmdMgr.addCommand(new StdCmdUnitsCalculator());
    //rcCmdMgr.addCommand(new StdCmdMeasurementSimple());
    //rcCmdMgr.addCommand(new StdCmdDownloadOnlineHelp());
    //rcCmdMgr.addCommand(new StdCmdDescription());
}

} // namespace Gui
