/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#endif

#include "Command.h"
#include "Action.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "DlgMacroExecuteImp.h"
#include "DlgMacroRecordImp.h"
#include "Macro.h"
#include "MainWindow.h"
#include "PythonDebugger.h"


using namespace Gui;


//===========================================================================
// Std_DlgMacroRecord
//===========================================================================
DEF_STD_CMD_A(StdCmdDlgMacroRecord)

StdCmdDlgMacroRecord::StdCmdDlgMacroRecord()
  : Command("Std_DlgMacroRecord")
{
    sGroup        = "Macro";
    sMenuText     = QT_TR_NOOP("&Macro recording ...");
    sToolTipText  = QT_TR_NOOP("Opens a dialog to record a macro");
    sWhatsThis    = "Std_DlgMacroRecord";
    sStatusTip    = QT_TR_NOOP("Opens a dialog to record a macro");
    sPixmap       = "media-record";
    eType         = 0;
}

void StdCmdDlgMacroRecord::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (!getGuiApplication()->macroManager()->isOpen()){
        Gui::Dialog::DlgMacroRecordImp cDlg(getMainWindow());
        if (cDlg.exec() && getAction()) {
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("media-playback-stop"));
            getAction()->setText(QCoreApplication::translate("StdCmdDlgMacroRecord", "S&top macro recording"));
            getAction()->setToolTip(QCoreApplication::translate("StdCmdDlgMacroRecord", "Stop the macro recording session"));
        }
    }
    else {
        getGuiApplication()->macroManager()->commit();
        if (getAction()) {
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("media-record"));
            getAction()->setText(QString::fromLatin1(sMenuText));
            getAction()->setToolTip(QString::fromLatin1(sToolTipText));
        }
    }
}

bool StdCmdDlgMacroRecord::isActive()
{
    return true;
}

//===========================================================================
// Std_DlgMacroExecute
//===========================================================================
DEF_STD_CMD_A(StdCmdDlgMacroExecute)

StdCmdDlgMacroExecute::StdCmdDlgMacroExecute()
  : Command("Std_DlgMacroExecute")
{
    sGroup        = "Macro";
    sMenuText     = QT_TR_NOOP("Macros ...");
    sToolTipText  = QT_TR_NOOP("Opens a dialog to let you execute a recorded macro");
    sWhatsThis    = "Std_DlgMacroExecute";
    sStatusTip    = QT_TR_NOOP("Opens a dialog to let you execute a recorded macro");
    sPixmap       = "accessories-text-editor";
    eType         = 0;
}

void StdCmdDlgMacroExecute::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Dialog::DlgMacroExecuteImp cDlg(getMainWindow());
    cDlg.exec();
}

bool StdCmdDlgMacroExecute::isActive()
{
    return ! (getGuiApplication()->macroManager()->isOpen());
}

//===========================================================================
// Std_DlgMacroExecuteDirect
//===========================================================================
DEF_STD_CMD_A(StdCmdDlgMacroExecuteDirect)

StdCmdDlgMacroExecuteDirect::StdCmdDlgMacroExecuteDirect()
  : Command("Std_DlgMacroExecuteDirect")
{
    sGroup        = "Macro";
    sMenuText     = QT_TR_NOOP("Execute macro");
    sToolTipText  = QT_TR_NOOP("Execute the macro in the editor");
    sWhatsThis    = "Std_DlgMacroExecuteDirect";
    sStatusTip    = QT_TR_NOOP("Execute the macro in the editor");
    sPixmap       = "media-playback-start";
    sAccel        = "Ctrl+F6";
    eType         = 0;
}

void StdCmdDlgMacroExecuteDirect::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"Run\")");
}

bool StdCmdDlgMacroExecuteDirect::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("Run");
}

DEF_STD_CMD_A(StdCmdMacroAttachDebugger)

StdCmdMacroAttachDebugger::StdCmdMacroAttachDebugger()
  : Command("Std_MacroAttachDebugger")
{
    sGroup        = "Macro";
    sMenuText     = QT_TR_NOOP("Attach to remote debugger...");
    sToolTipText  = QT_TR_NOOP("Attach to a remotely running debugger");
    sWhatsThis    = "Std_MacroAttachDebugger";
    sStatusTip    = QT_TR_NOOP("Attach to a remotely running debugger");
    eType         = 0;
}

void StdCmdMacroAttachDebugger::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Gui, "from freecad.gui import RemoteDebugger\n"
                   "RemoteDebugger.attachToRemoteDebugger()");
}

bool StdCmdMacroAttachDebugger::isActive()
{
    return true;
}

DEF_STD_CMD_A(StdCmdMacroStartDebug)

StdCmdMacroStartDebug::StdCmdMacroStartDebug()
  : Command("Std_MacroStartDebug")
{
    sGroup        = "Macro";
    sMenuText     = QT_TR_NOOP("Debug macro");
    sToolTipText  = QT_TR_NOOP("Start debugging of macro");
    sWhatsThis    = "Std_MacroStartDebug";
    sStatusTip    = QT_TR_NOOP("Start debugging of macro");
    sPixmap       = "debug-start";
    sAccel        = "F6";
    eType         = 0;
}

void StdCmdMacroStartDebug::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    PythonDebugger* dbg = Application::Instance->macroManager()->debugger();
    if (!dbg->isRunning())
        doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"StartDebug\")");
    else
        dbg->stepRun();
}

bool StdCmdMacroStartDebug::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("StartDebug");
}

DEF_STD_CMD_A(StdCmdMacroStopDebug)

StdCmdMacroStopDebug::StdCmdMacroStopDebug()
  : Command("Std_MacroStopDebug")
{
    sGroup        = "Macro";
    sMenuText     = QT_TR_NOOP("Stop debugging");
    sToolTipText  = QT_TR_NOOP("Stop debugging of macro");
    sWhatsThis    = "Std_MacroStopDebug";
    sStatusTip    = QT_TR_NOOP("Stop debugging of macro");
    sPixmap       = "debug-stop";
    sAccel        = "Shift+F6";
    eType         = 0;
}

void StdCmdMacroStopDebug::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Application::Instance->macroManager()->debugger()->tryStop();
}

bool StdCmdMacroStopDebug::isActive()
{
    static PythonDebugger* dbg = Application::Instance->macroManager()->debugger();
    return dbg->isRunning();
}

DEF_STD_CMD_A(StdCmdMacroStepOver)

StdCmdMacroStepOver::StdCmdMacroStepOver()
  : Command("Std_MacroStepOver")
{
    sGroup        = "Macro";
    sMenuText     = QT_TR_NOOP("Step over");
    sToolTipText  = QT_TR_NOOP("Step to the next line in this file");
    sWhatsThis    = "Std_MacroStepOver";
    sStatusTip    = QT_TR_NOOP("Step to the next line in this file");
    sPixmap       = nullptr;
    sAccel        = "F10";
    eType         = 0;
}

void StdCmdMacroStepOver::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Application::Instance->macroManager()->debugger()->stepOver();
}

bool StdCmdMacroStepOver::isActive()
{
    static PythonDebugger* dbg = Application::Instance->macroManager()->debugger();
    return dbg->isRunning();
}

DEF_STD_CMD_A(StdCmdMacroStepInto)

StdCmdMacroStepInto::StdCmdMacroStepInto()
  : Command("Std_MacroStepInto")
{
    sGroup        = "Macro";
    sMenuText     = QT_TR_NOOP("Step into");
    sToolTipText  = QT_TR_NOOP("Step to the next line executed");
    //sWhatsThis    = "Std_MacroStepOver";
    sStatusTip    = QT_TR_NOOP("Step to the next line executed");
    sPixmap       = nullptr;
    sAccel        = "F11";
    eType         = 0;
}

void StdCmdMacroStepInto::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Application::Instance->macroManager()->debugger()->stepInto();
}

bool StdCmdMacroStepInto::isActive()
{
    static PythonDebugger* dbg = Application::Instance->macroManager()->debugger();
    return dbg->isRunning();
}

DEF_STD_CMD_A(StdCmdToggleBreakpoint)

StdCmdToggleBreakpoint::StdCmdToggleBreakpoint()
  : Command("Std_ToggleBreakpoint")
{
    sGroup        = "Macro";
    sMenuText     = QT_TR_NOOP("Toggle breakpoint");
    sToolTipText  = QT_TR_NOOP("Add or remove a breakpoint at this position");
    sWhatsThis    = "Std_ToggleBreakpoint";
    sStatusTip    = QT_TR_NOOP("Add or remove a breakpoint at this position");
    sPixmap       = nullptr;
    sAccel        = "F9";
    eType         = 0;
}

void StdCmdToggleBreakpoint::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.SendMsgToActiveView(\"ToggleBreakpoint\")");
}

bool StdCmdToggleBreakpoint::isActive()
{
    return getGuiApplication()->sendHasMsgToActiveView("ToggleBreakpoint");
}

namespace Gui {

void CreateMacroCommands()
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();
    rcCmdMgr.addCommand(new StdCmdDlgMacroRecord());
    rcCmdMgr.addCommand(new StdCmdDlgMacroExecute());
    rcCmdMgr.addCommand(new StdCmdDlgMacroExecuteDirect());
    rcCmdMgr.addCommand(new StdCmdMacroAttachDebugger());
    rcCmdMgr.addCommand(new StdCmdMacroStartDebug());
    rcCmdMgr.addCommand(new StdCmdMacroStopDebug());
    rcCmdMgr.addCommand(new StdCmdMacroStepOver());
    rcCmdMgr.addCommand(new StdCmdMacroStepInto());
    rcCmdMgr.addCommand(new StdCmdToggleBreakpoint());
}

} // namespace Gui
