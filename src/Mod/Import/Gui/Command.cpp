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

#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
using Gui::FileDialog;

//===========================================================================
// Import_Box
//===========================================================================
DEF_STD_CMD_A(FCCmdImportReadBREP)

FCCmdImportReadBREP::FCCmdImportReadBREP()
    : Command("Import_ReadBREP")
{
    sAppModule = "Import";
    sGroup = "Import";
    sMenuText = "Read BREP";
    sToolTipText = "Read a BREP file";
    sWhatsThis = "Import_ReadBREP";
    sStatusTip = sToolTipText;
    sPixmap = "Std_Tool1";
}

void FCCmdImportReadBREP::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    openCommand(QT_TRANSLATE_NOOP("Command", "Read BREP"));
    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
                                                  QString(),
                                                  QString(),
                                                  QLatin1String("BREP (*.brep *.rle)"));
    if (fn.isEmpty()) {
        abortCommand();
        return;
    }

    fn = Base::Tools::escapeEncodeFilename(fn);
    doCommand(Doc, "TopoShape = Import.ReadBREP(\"%s\")", (const char*)fn.toUtf8());
    commitCommand();
}

bool FCCmdImportReadBREP::isActive()
{
    return getGuiApplication()->activeDocument() != nullptr;
}

//===========================================================================
// PartImportStep
//===========================================================================
DEF_STD_CMD_A(ImportStep)

ImportStep::ImportStep()
    : Command("Part_ImportStep")
{
    sAppModule = "Part";
    sGroup = "Part";
    sMenuText = "Import STEP";
    sToolTipText = "Create or change a Import STEP feature";
    sWhatsThis = "Part_ImportStep";
    sStatusTip = sToolTipText;
    sPixmap = "Save";
}


void ImportStep::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
                                                  QString(),
                                                  QString(),
                                                  QLatin1String("STEP (*.stp *.step)"));
    if (!fn.isEmpty()) {
        openCommand(QT_TRANSLATE_NOOP("Command", "Part ImportSTEP Create"));
        doCommand(Doc, "f = App.document().addObject(\"ImportStep\",\"ImportStep\")");
        fn = Base::Tools::escapeEncodeFilename(fn);
        doCommand(Doc, "f.FileName = \"%s\"", (const char*)fn.toUtf8());
        commitCommand();
        updateActive();
    }
}

bool ImportStep::isActive()
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}


//===========================================================================
// ImportIges
//===========================================================================
DEF_STD_CMD_A(ImportIges)

ImportIges::ImportIges()
    : Command("Import_Iges")
{
    sAppModule = "Import";
    sGroup = "Part";
    sMenuText = "Import IGES";
    sToolTipText = "Create or change a Import IGES feature";
    sWhatsThis = "Import_Iges";
    sStatusTip = sToolTipText;
    sPixmap = "Save";
}

void ImportIges::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
                                                  QString(),
                                                  QString(),
                                                  QLatin1String("IGES (*.igs *.iges)"));
    if (!fn.isEmpty()) {
        openCommand(QT_TRANSLATE_NOOP("Command", "ImportIGES Create"));
        doCommand(Doc, "f = App.document().addObject(\"ImportIges\",\"ImportIges\")");
        fn = Base::Tools::escapeEncodeFilename(fn);
        doCommand(Doc, "f.FileName = \"%s\"", (const char*)fn.toUtf8());
        commitCommand();
        updateActive();
    }
}

bool ImportIges::isActive()
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}


void CreateImportCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new FCCmdImportReadBREP());
}
