 // SPDX-License-Identifier: LGPL-2.1-or-later

  /****************************************************************************
   *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>               *
   *   Copyright (c) 2023 FreeCAD Project Association                         *
   *                                                                          *
   *   This file is part of FreeCAD.                                          *
   *                                                                          *
   *   FreeCAD is free software: you can redistribute it and/or modify it     *
   *   under the terms of the GNU Lesser General Public License as            *
   *   published by the Free Software Foundation, either version 2.1 of the   *
   *   License, or (at your option) any later version.                        *
   *                                                                          *
   *   FreeCAD is distributed in the hope that it will be useful, but         *
   *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
   *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
   *   Lesser General Public License for more details.                        *
   *                                                                          *
   *   You should have received a copy of the GNU Lesser General Public       *
   *   License along with FreeCAD. If not, see                                *
   *   <https://www.gnu.org/licenses/>.                                       *
   *                                                                          *
   ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QApplication>
# include <QMessageBox>
# include <QRegularExpression>
# include <QRegularExpressionMatch>
# include <QWhatsThis>
# include <QAbstractButton>
# include <QTimer>
# include <QProcess>
#endif

#include <App/Document.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Sequencer.h>

#include "Action.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "DlgAbout.h"
#include "DlgCustomizeImp.h"
#include "DlgParameterImp.h"
#include "DlgPreferencesImp.h"
#include "DlgUnitsCalculatorImp.h"
#include "GuiConsole.h"
#include "MainWindow.h"
#include "OnlineDocumentation.h"
#include "Selection.h"
#include "WhatsThis.h"
#include "Workbench.h"
#include "WorkbenchManager.h"


using Base::Console;
using Base::Sequencer;
using namespace Gui;
namespace sp = std::placeholders;


//===========================================================================
// Std_Workbench
//===========================================================================

DEF_STD_CMD_AC(StdCmdWorkbench)

StdCmdWorkbench::StdCmdWorkbench()
  : Command("Std_Workbench")
{
    sGroup        = "View";
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
        QRegularExpression rx;
        rx.setPattern(QLatin1String(R"(^\s*<type 'exceptions.\w*'>:\s*)"));
        auto match = rx.match(msg);
        if (match.hasMatch())
            msg = msg.mid(match.capturedLength());
        QMessageBox::critical(getMainWindow(), QObject::tr("Cannot load workbench"), msg);
    }
    catch(...) {
        QMessageBox::critical(getMainWindow(), QObject::tr("Cannot load workbench"),
            QObject::tr("A general error occurred while loading the workbench"));
    }
}

bool StdCmdWorkbench::isActive()
{
    return true;
}

Action * StdCmdWorkbench::createAction()
{
    Action *pcAction;

    pcAction = new WorkbenchGroup(this,getMainWindow());
    pcAction->setShortcut(QString::fromLatin1(getAccel()));
    applyCommandData(this->className(), pcAction);
    if (getPixmap())
        pcAction->setIcon(Gui::BitmapFactory().iconFromTheme(getPixmap()));

    return pcAction;
}

//===========================================================================
// Std_RecentFiles
//===========================================================================

DEF_STD_CMD_C(StdCmdRecentFiles)

StdCmdRecentFiles::StdCmdRecentFiles()
  :Command("Std_RecentFiles")
{
    sGroup        = "File";
    sMenuText     = QT_TR_NOOP("Open Recent");
    sToolTipText  = QT_TR_NOOP("Recent file list");
    sWhatsThis    = "Std_RecentFiles";
    sStatusTip    = QT_TR_NOOP("Recent file list");
    sPixmap       = "Std_RecentFiles";
    eType         = NoTransaction;
}

/**
 * Opens the recent file at position \a iMsg in the menu.
 * If the file does not exist or cannot be loaded this item is removed
 * from the list.
 */
void StdCmdRecentFiles::activated(int iMsg)
{
    auto act = qobject_cast<RecentFilesAction*>(_pcAction);
    if (act) act->activateFile( iMsg );
}

/**
 * Creates the QAction object containing the recent files.
 */
Action * StdCmdRecentFiles::createAction()
{
    auto pcAction = new RecentFilesAction(this, getMainWindow());
    pcAction->setObjectName(QLatin1String("recentFiles"));
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);
    return pcAction;
}

//===========================================================================
// Std_RecentMacros
//===========================================================================

DEF_STD_CMD_C(StdCmdRecentMacros)

StdCmdRecentMacros::StdCmdRecentMacros()
  :Command("Std_RecentMacros")
{
    sGroup        = "Macro";
    sMenuText     = QT_TR_NOOP("Recent macros");
    sToolTipText  = QT_TR_NOOP("Recent macro list");
    sWhatsThis    = "Std_RecentMacros";
    sStatusTip    = QT_TR_NOOP("Recent macro list");
    sPixmap       = "Std_RecentMacros";
    eType         = NoTransaction;
}

/**
 * Opens the recent macro at position \a iMsg in the menu.
 * If the macro does not exist or cannot be loaded this item is removed
 * from the list.
 */
void StdCmdRecentMacros::activated(int iMsg)
{
    auto act = qobject_cast<RecentMacrosAction*>(_pcAction);
    if (act) act->activateFile( iMsg );
}

/**
 * Creates the QAction object containing the recent macros.
 */
Action * StdCmdRecentMacros::createAction()
{
    auto pcAction = new RecentMacrosAction(this, getMainWindow());
    pcAction->setObjectName(QLatin1String("recentMacros"));
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
    sGroup        = "Help";
    sMenuText     = QT_TR_NOOP("&About %1");
    sToolTipText  = QT_TR_NOOP("Displays important information About %1");
    sWhatsThis    = "Std_About";
    sStatusTip    = sToolTipText;
    eType         = 0;
}

Action * StdCmdAbout::createAction()
{
    Action *pcAction;

    QString exe = qApp->applicationName();
    pcAction = new Action(this, getMainWindow());
    pcAction->setText(QCoreApplication::translate(
        this->className(), getMenuText()).arg(exe));
    pcAction->setToolTip(QCoreApplication::translate(
        this->className(), getToolTipText()).arg(exe));
    pcAction->setStatusTip(QCoreApplication::translate(
        this->className(), getStatusTip()).arg(exe));
    pcAction->setWhatsThis(QLatin1String(getWhatsThis()));
    pcAction->setIcon(QApplication::windowIcon());
    pcAction->setShortcut(QString::fromLatin1(getAccel()));
    // Needs to have AboutRole set to avoid duplicates if adding the about action more than once on macOS
    pcAction->setMenuRole(QAction::AboutRole);
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
            this->className(), getMenuText()).arg(exe));
        _pcAction->setToolTip(QCoreApplication::translate(
            this->className(), getToolTipText()).arg(exe));
        _pcAction->setStatusTip(QCoreApplication::translate(
            this->className(), getStatusTip()).arg(exe));
        _pcAction->setWhatsThis(QLatin1String(getWhatsThis()));
    }
}

//===========================================================================
// Std_AboutQt
//===========================================================================
DEF_STD_CMD(StdCmdAboutQt)

StdCmdAboutQt::StdCmdAboutQt()
  :Command("Std_AboutQt")
{
  sGroup        = "Help";
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
DEF_STD_CMD(StdCmdWhatsThis)

StdCmdWhatsThis::StdCmdWhatsThis()
  :Command("Std_WhatsThis")
{
    sGroup        = "Help";
    sMenuText     = QT_TR_NOOP("&What's This?");
    sToolTipText  = QT_TR_NOOP("Opens the documentation corresponding to the selection");
    sWhatsThis    = "Std_WhatsThis";
    sStatusTip    = sToolTipText;
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
// Std_RestartInSafeMode
//===========================================================================
DEF_STD_CMD(StdCmdRestartInSafeMode)

StdCmdRestartInSafeMode::StdCmdRestartInSafeMode()
  :Command("Std_RestartInSafeMode")
{
    sGroup        = "Help";
    sMenuText     = QT_TR_NOOP("Restart in Safe Mode");
    sToolTipText  = QT_TR_NOOP("Starts FreeCAD without any modules or plugins loaded");
    sWhatsThis    = "Std_RestartInSafeMode";
    sStatusTip    = sToolTipText;
    sPixmap       = "safe-mode-restart";
    eType         = 0;
}

void StdCmdRestartInSafeMode::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    QMessageBox restartBox;
    restartBox.setIcon(QMessageBox::Warning);
    restartBox.setWindowTitle(QObject::tr("Restart in safe mode"));
    restartBox.setText(QObject::tr("Are you sure you want to restart FreeCAD and enter safe mode?"));
    restartBox.setInformativeText(QObject::tr("Safe mode temporarily disables your configuration and addons."));
    restartBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    restartBox.setDefaultButton(QMessageBox::No);

    if (restartBox.exec() == QMessageBox::Yes) {
        //restart FreeCAD after a delay to give time to this dialog to close
        const int ms = 1000;
        QTimer::singleShot(ms, []()
        {
            QStringList args = QApplication::arguments();
            args.pop_front();
            auto const safeModeArgument = QString::fromLatin1("--safe-mode");
            if (!args.contains(safeModeArgument)) {
                args.append(safeModeArgument);
            }
            if (getMainWindow()->close()) {
                QProcess::startDetached(QApplication::applicationFilePath(), args);
            }
        });
    }
}

//===========================================================================
// Std_DlgParameter
//===========================================================================
DEF_STD_CMD(StdCmdDlgParameter)

StdCmdDlgParameter::StdCmdDlgParameter()
  :Command("Std_DlgParameter")
{
  sGroup        = "Tools";
  sMenuText     = QT_TR_NOOP("E&dit parameters ...");
  sToolTipText  = QT_TR_NOOP("Opens a Dialog to edit the parameters");
  sWhatsThis    = "Std_DlgParameter";
  sStatusTip    = QT_TR_NOOP("Opens a Dialog to edit the parameters");
  sPixmap       = "Std_DlgParameter";
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
DEF_STD_CMD_C(StdCmdDlgPreferences)

StdCmdDlgPreferences::StdCmdDlgPreferences()
  :Command("Std_DlgPreferences")
{
    sGroup        = "Tools";
    sMenuText     = QT_TR_NOOP("&Preferences ...");
    sToolTipText  = QT_TR_NOOP("Opens a Dialog to edit the preferences");
    sWhatsThis    = "Std_DlgPreferences";
    sStatusTip    = QT_TR_NOOP("Opens a Dialog to edit the preferences");
    sPixmap     = "preferences-system";
    eType         = 0;
    sAccel        = "Ctrl+,";
}

Action * StdCmdDlgPreferences::createAction()
{
    Action *pcAction = Command::createAction();
    pcAction->setMenuRole(QAction::PreferencesRole);

    return pcAction;
}

void StdCmdDlgPreferences::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    static QString groupName{};
    static int index{};

    Gui::Dialog::DlgPreferencesImp cDlg(getMainWindow());
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences");
    if (hGrp->GetBool("RestoreGroupPage", true)) {
        cDlg.activateGroupPage(groupName, index);
    }

    if (cDlg.exec()) {
        cDlg.activeGroupPage(groupName, index);
    }
}

//===========================================================================
// Std_DlgCustomize
//===========================================================================
DEF_STD_CMD(StdCmdDlgCustomize)

StdCmdDlgCustomize::StdCmdDlgCustomize()
  :Command("Std_DlgCustomize")
{
    sGroup        = "Tools";
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
    static QPointer<QDialog> dlg = nullptr;
    if (!dlg)
        dlg = new Gui::Dialog::DlgCustomizeImp(getMainWindow());
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

//===========================================================================
// Std_CommandLine
//===========================================================================
DEF_STD_CMD(StdCmdCommandLine)

StdCmdCommandLine::StdCmdCommandLine()
  :Command("Std_CommandLine")
{
    sGroup        = "Tools";
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

DEF_STD_CMD(StdCmdOnlineHelp)

StdCmdOnlineHelp::StdCmdOnlineHelp()
  :Command("Std_OnlineHelp")
{
    sGroup        = "Help";
    sMenuText     = QT_TR_NOOP("Help");
    sToolTipText  = QT_TR_NOOP("Opens the Help documentation");
    sWhatsThis    = "Std_OnlineHelp";
    sStatusTip    = sToolTipText;
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

DEF_STD_CMD(StdCmdOnlineHelpWebsite)

StdCmdOnlineHelpWebsite::StdCmdOnlineHelpWebsite()
  :Command("Std_OnlineHelpWebsite")
{
    sGroup        = "Help";
    sMenuText     = QT_TR_NOOP("Help Website");
    sToolTipText  = QT_TR_NOOP("Opens the help documentation");
    sWhatsThis    = "Std_OnlineHelpWebsite";
    sStatusTip    = sToolTipText;
    eType         = 0;
}

void StdCmdOnlineHelpWebsite::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string defaulturl = QCoreApplication::translate(this->className(),"https://wiki.freecad.org/Online_Help_Toc").toStdString();
    ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Websites");
    std::string url = hURLGrp->GetASCII("OnlineHelp", defaulturl.c_str());
    hURLGrp->SetASCII("OnlineHelp", url.c_str());
    OpenURLInBrowser(url.c_str());
}

//===========================================================================
// Std_FreeCADDonation
//===========================================================================

DEF_STD_CMD(StdCmdFreeCADDonation)

StdCmdFreeCADDonation::StdCmdFreeCADDonation()
  :Command("Std_FreeCADDonation")
{
    sGroup        = "Help";
    sMenuText     = QT_TR_NOOP("Support FreeCAD");
    sToolTipText  = QT_TR_NOOP("Support FreeCAD development");
    sWhatsThis    = "Std_FreeCADDonation";
    sStatusTip    = sToolTipText;
    sPixmap       = "internet-web-browser";
    eType         = 0;
}

void StdCmdFreeCADDonation::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Websites");
    std::string url = hURLGrp->GetASCII("DonatePage", "https://wiki.freecad.org/Donate");
    hURLGrp->SetASCII("DonatePage", url.c_str());
    OpenURLInBrowser(url.c_str());
}

//===========================================================================
// Std_FreeCADWebsite
//===========================================================================

DEF_STD_CMD(StdCmdFreeCADWebsite)

StdCmdFreeCADWebsite::StdCmdFreeCADWebsite()
  :Command("Std_FreeCADWebsite")
{
    sGroup        = "Help";
    sMenuText     = QT_TR_NOOP("FreeCAD Website");
    sToolTipText  = QT_TR_NOOP("Navigates to the official FreeCAD website");
    sWhatsThis    = "Std_FreeCADWebsite";
    sStatusTip    = sToolTipText;
    sPixmap       = "internet-web-browser";
    eType         = 0;
}

void StdCmdFreeCADWebsite::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string defaulturl = QCoreApplication::translate(this->className(),"https://www.freecad.org").toStdString();
    ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Websites");
    std::string url = hURLGrp->GetASCII("WebPage", defaulturl.c_str());
    hURLGrp->SetASCII("WebPage", url.c_str());
    OpenURLInBrowser(url.c_str());
}

//===========================================================================
// Std_FreeCADUserHub
//===========================================================================

DEF_STD_CMD(StdCmdFreeCADUserHub)

StdCmdFreeCADUserHub::StdCmdFreeCADUserHub()
  :Command("Std_FreeCADUserHub")
{
    sGroup        = "Help";
    sMenuText     = QT_TR_NOOP("User Documentation");
    sToolTipText  = QT_TR_NOOP("Opens the documentation for users");
    sWhatsThis    = "Std_FreeCADUserHub";
    sStatusTip    = sToolTipText;
    sPixmap       = "internet-web-browser";
    eType         = 0;
}

void StdCmdFreeCADUserHub::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string defaulturl = QCoreApplication::translate(this->className(),"https://wiki.freecad.org/User_hub").toStdString();
    ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Websites");
    std::string url = hURLGrp->GetASCII("Documentation", defaulturl.c_str());
    hURLGrp->SetASCII("Documentation", url.c_str());
    OpenURLInBrowser(url.c_str());
}

//===========================================================================
// Std_FreeCADPowerUserHub
//===========================================================================

DEF_STD_CMD(StdCmdFreeCADPowerUserHub)

StdCmdFreeCADPowerUserHub::StdCmdFreeCADPowerUserHub()
  :Command("Std_FreeCADPowerUserHub")
{
    sGroup        = "Help";
    sMenuText     = QT_TR_NOOP("Python Scripting Documentation");
    sToolTipText  = QT_TR_NOOP("Opens the Python Scripting documentation");
    sWhatsThis    = "Std_FreeCADPowerUserHub";
    sStatusTip    = sToolTipText;
    sPixmap       = "applications-python";
    eType         = 0;
}

void StdCmdFreeCADPowerUserHub::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string defaulturl = QCoreApplication::translate(this->className(),"https://wiki.freecad.org/Power_users_hub").toStdString();
    ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Websites");
    std::string url = hURLGrp->GetASCII("PowerUsers", defaulturl.c_str());
    hURLGrp->SetASCII("PowerUsers", url.c_str());
    OpenURLInBrowser(url.c_str());
}

//===========================================================================
// Std_FreeCADForum
//===========================================================================

DEF_STD_CMD(StdCmdFreeCADForum)

StdCmdFreeCADForum::StdCmdFreeCADForum()
  :Command("Std_FreeCADForum")
{
    sGroup        = "Help";
    sMenuText     = QT_TR_NOOP("FreeCAD Forum");
    sToolTipText  = QT_TR_NOOP("The FreeCAD forum, where you can find help from other users");
    sWhatsThis    = "Std_FreeCADForum";
    sStatusTip    = sToolTipText;
    sPixmap       = "internet-web-browser";
    eType         = 0;
}

void StdCmdFreeCADForum::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string defaulturl = QCoreApplication::translate(this->className(),"https://forum.freecad.org").toStdString();
    ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Websites");
    std::string url = hURLGrp->GetASCII("UserForum", defaulturl.c_str());
    hURLGrp->SetASCII("UserForum", url.c_str());
    OpenURLInBrowser(url.c_str());
}

//===========================================================================
// Std_FreeCADFAQ
//===========================================================================

DEF_STD_CMD(StdCmdFreeCADFAQ)

StdCmdFreeCADFAQ::StdCmdFreeCADFAQ()
  :Command("Std_FreeCADFAQ")
{
    sGroup        = "Help";
    sMenuText     = QT_TR_NOOP("FreeCAD FAQ");
    sToolTipText  = QT_TR_NOOP("Opens the Frequently Asked Questions");
    sWhatsThis    = "Std_FreeCADFAQ";
    sStatusTip    = sToolTipText;
    sPixmap       = "internet-web-browser";
    eType         = 0;
}

void StdCmdFreeCADFAQ::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string defaulturl = QCoreApplication::translate(this->className(),"https://wiki.freecad.org/Frequently_asked_questions").toStdString();
    ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Websites");
    std::string url = hURLGrp->GetASCII("FAQ", defaulturl.c_str());
    hURLGrp->SetASCII("FAQ", url.c_str());
    OpenURLInBrowser(url.c_str());
}

//===========================================================================
// Std_PythonWebsite
//===========================================================================

DEF_STD_CMD(StdCmdPythonWebsite)

StdCmdPythonWebsite::StdCmdPythonWebsite()
  :Command("Std_PythonWebsite")
{
    sGroup        = "Help";
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
    OpenURLInBrowser("https://www.python.org");
}


//===========================================================================
// Std_ReportBug
//===========================================================================

DEF_STD_CMD(StdCmdReportBug)

StdCmdReportBug::StdCmdReportBug()
  :Command("Std_ReportBug")
{
    sGroup        = "Help";
    sMenuText     = QT_TR_NOOP("Report an Issue");
    sToolTipText  = QT_TR_NOOP("Report an issue or suggest a new feature");
    sWhatsThis    = "Std_ReportBug";
    sStatusTip    = sToolTipText;
    sPixmap       = "internet-web-browser";
    eType         = 0;
}

void StdCmdReportBug::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ParameterGrp::handle hURLGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Websites");
    std::string url = hURLGrp->GetASCII("IssuesPage", "https://github.com/FreeCAD/FreeCAD/issues");
    hURLGrp->SetASCII("IssuesPage", url.c_str());
    OpenURLInBrowser(url.c_str());
}

//===========================================================================
// Std_TextDocument
//===========================================================================

DEF_STD_CMD_A(StdCmdTextDocument)

StdCmdTextDocument::StdCmdTextDocument()
  :Command("Std_TextDocument")
{
    sGroup        = "Tools";
    sMenuText     = QT_TR_NOOP("Add text document");
    sToolTipText  = QT_TR_NOOP("Add text document to active document");
    sWhatsThis    = "Std_TextDocument";
    sStatusTip    = QT_TR_NOOP("Add text document to active document");
    sPixmap       = "TextDocument";
    eType         = 0;
}

void StdCmdTextDocument::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    openCommand(QT_TRANSLATE_NOOP("Command", "Insert text document"));
    doCommand(Doc, "App.ActiveDocument.addObject(\"App::TextDocument\",\"%s\").Label=\"%s\"","Text document","Text document");
    doCommand(Gui, "Gui.ActiveDocument.ActiveObject.doubleClicked()");
    updateActive();
    commitCommand();
}

bool StdCmdTextDocument::isActive()
{
    return hasActiveDocument();
}

//===========================================================================
// Std_UnitsCalculator
//===========================================================================
DEF_STD_CMD(StdCmdUnitsCalculator)

StdCmdUnitsCalculator::StdCmdUnitsCalculator()
  : Command("Std_UnitsCalculator")
{
    sGroup        = "Tools";
    sMenuText     = QT_TR_NOOP("&Units converter...");
    sToolTipText  = QT_TR_NOOP("Start the units converter");
    sWhatsThis    = "Std_UnitsCalculator";
    sStatusTip    = QT_TR_NOOP("Start the units converter");
    sPixmap       = "accessories-calculator";
    eType         = 0;
}

void StdCmdUnitsCalculator::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto dlg = new Gui::Dialog::DlgUnitsCalculator( getMainWindow() );
    dlg->show();
}

//===========================================================================
// StdCmdUserEditMode
//===========================================================================
class StdCmdUserEditMode : public Gui::Command
{
public:
    StdCmdUserEditMode();
    ~StdCmdUserEditMode() override = default;
    void languageChange() override;
    const char* className() const override {return "StdCmdUserEditMode";}
    void updateIcon(int mode);
protected:
    void activated(int iMsg) override;
    bool isActive() override;
    Gui::Action * createAction() override;
};

StdCmdUserEditMode::StdCmdUserEditMode()
  : Command("Std_UserEditMode")
{
    sGroup        = "Edit";
    sMenuText     = QT_TR_NOOP("Edit mode");
    sToolTipText  = QT_TR_NOOP("Defines behavior when editing an object from tree");
    sStatusTip    = QT_TR_NOOP("Defines behavior when editing an object from tree");
    sWhatsThis    = "Std_UserEditMode";
    sPixmap       = "Std_UserEditModeDefault";
    eType         = ForEdit;

    this->getGuiApplication()->signalUserEditModeChanged.connect([this](int mode) {
        this->updateIcon(mode);
    });
}

Gui::Action * StdCmdUserEditMode::createAction()
{
    auto pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    pcAction->setIsMode(true);
    applyCommandData(this->className(), pcAction);

    for (auto const &uem : Gui::Application::Instance->listUserEditModes()) {
        QAction* act = pcAction->addAction(QString());
        auto modeName = QString::fromStdString(uem.second.first);
        act->setCheckable(true);
        act->setIcon(BitmapFactory().iconFromTheme(qPrintable(QString::fromLatin1("Std_UserEditMode")+modeName)));
        act->setObjectName(QString::fromLatin1("Std_UserEditMode")+modeName);
        act->setWhatsThis(QString::fromLatin1(getWhatsThis()));
        act->setToolTip(QString::fromStdString(uem.second.second));

        if (uem.first == 0) {
            pcAction->setIcon(act->icon());
            act->setChecked(true);
        }
    }

    _pcAction = pcAction;

    int lastMode = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
        GetInt("UserEditMode", 0);
    Gui::Application::Instance->setUserEditMode(lastMode);

    languageChange();
    return pcAction;
}

void StdCmdUserEditMode::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    auto pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    for (int i = 0 ; i < a.count() ; i++) {
        auto modeName = Gui::Application::Instance->getUserEditModeUIStrings(i);
        a[i]->setText(QCoreApplication::translate(
        "EditMode", modeName.first.c_str()));
        a[i]->setToolTip(QCoreApplication::translate(
        "EditMode", modeName.second.c_str()));
    }
}

void StdCmdUserEditMode::updateIcon(int mode)
{
    auto actionGroup = dynamic_cast<Gui::ActionGroup *>(_pcAction);
    if (!actionGroup)
        return;

    actionGroup->setCheckedAction(mode);
}

void StdCmdUserEditMode::activated(int iMsg)
{
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
            SetInt("UserEditMode", iMsg);
    Gui::Application::Instance->setUserEditMode(iMsg);
}

bool StdCmdUserEditMode::isActive()
{
    return true;
}

//===========================================================================
// Std_ReloadStylesheet
//===========================================================================
DEF_STD_CMD(StdCmdReloadStyleSheet)

StdCmdReloadStyleSheet::StdCmdReloadStyleSheet()
  : Command("Std_ReloadStyleSheet")
{
    sGroup        = "View";
    sMenuText     = QT_TR_NOOP("&Reload stylesheet");
    sToolTipText  = QT_TR_NOOP("Reloads the current stylesheet");
    sWhatsThis    = "Std_ReloadStyleSheet";
    sStatusTip    = QT_TR_NOOP("Reloads the current stylesheet");
    sPixmap       = "view-refresh";
    sWhatsThis    = "Std_ReloadStyleSheet";
}

void StdCmdReloadStyleSheet::activated(int )
{
    auto mw = getMainWindow();

    auto qssFile = mw->property("fc_currentStyleSheet").toString();
    auto tiledBackground = mw->property("fc_tiledBackground").toBool();

    Gui::Application::Instance->setStyleSheet(qssFile, tiledBackground);
}

namespace Gui {

void CreateStdCommands()
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
    rcCmdMgr.addCommand(new StdCmdRecentMacros());
    rcCmdMgr.addCommand(new StdCmdWhatsThis());
    rcCmdMgr.addCommand(new StdCmdRestartInSafeMode());
    rcCmdMgr.addCommand(new StdCmdPythonHelp());
    rcCmdMgr.addCommand(new StdCmdOnlineHelp());
    rcCmdMgr.addCommand(new StdCmdOnlineHelpWebsite());
    rcCmdMgr.addCommand(new StdCmdFreeCADWebsite());
    rcCmdMgr.addCommand(new StdCmdFreeCADDonation());
    rcCmdMgr.addCommand(new StdCmdFreeCADUserHub());
    rcCmdMgr.addCommand(new StdCmdFreeCADPowerUserHub());
    rcCmdMgr.addCommand(new StdCmdFreeCADForum());
    rcCmdMgr.addCommand(new StdCmdFreeCADFAQ());
    rcCmdMgr.addCommand(new StdCmdPythonWebsite());
    rcCmdMgr.addCommand(new StdCmdReportBug());
    rcCmdMgr.addCommand(new StdCmdTextDocument());
    rcCmdMgr.addCommand(new StdCmdUnitsCalculator());
    rcCmdMgr.addCommand(new StdCmdUserEditMode());
    rcCmdMgr.addCommand(new StdCmdReloadStyleSheet());
    //rcCmdMgr.addCommand(new StdCmdDownloadOnlineHelp());
    //rcCmdMgr.addCommand(new StdCmdDescription());
}

} // namespace Gui
