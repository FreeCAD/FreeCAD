/***************************************************************************
 *   Copyright (c) YEAR YOUR NAME         <Your e-mail address>            *
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
#endif

#include <Base/Console.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//===========================================================================
// Import_ReadGDML
//===========================================================================
DEF_STD_CMD_A(FCCmdImportReadGDML);

FCCmdImportReadGDML::FCCmdImportReadGDML()
   : Command("Import_ReadGDML")
{
    sAppModule      = "Import";
    sGroup          = "Import";
    sMenuText       = "Read GDML";
    sToolTipText    = "Read a GDML file";
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "import-gdmlread";
}

void FCCmdImportReadGDML::activated(int iMsg)
{
    openCommand("Read GDML");
	/*
    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(), QString(), QString(), QLatin1String("GDML (*.gdml)"));
    if (fn.isEmpty()) {
        abortCommand();
        return;
    }*/
	//Base::Console().Log("Oh Yeah");
	Base::Console().Message("FCCmdImportReadGDML | ");

    //doCommand(Doc,"TopoShape = Import.ReadBREP(\"%s\")",(const char*)fn.toUtf8());
    commitCommand();
}

bool FCCmdImportReadGDML::isActive(void)
{
    return getGuiApplication()->activeDocument() != 0;
}

//===========================================================================
// Import_WriteGDML
//===========================================================================
DEF_STD_CMD_A(FCCmdImportWriteGDML);

FCCmdImportWriteGDML::FCCmdImportWriteGDML()
  : Command("Import_WriteGDML")
{
    sAppModule      = "Import";
    sGroup          = "Import";
    sMenuText       = "Write GDML";
    sToolTipText    = "Write a GDML file";
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "import-gdmlwrite";
}

void FCCmdImportWriteGDML::activated(int iMsg)
{
    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(), QString(), QString(), QLatin1String("GDML (*.gdml)"));
    if (!fn.isEmpty()) {
        openCommand("Write GDML");
        //doCommand(Doc,"f = App.document().addObject(\"ImportIges\",\"ImportIges\")");
        //doCommand(Doc,"f.FileName = \"%s\"",(const char*)fn.toUtf8());
        commitCommand();
        updateActive();
    }
}

bool FCCmdImportWriteGDML::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

void CreateGDMLCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new FCCmdImportReadGDML());
    rcCmdMgr.addCommand(new FCCmdImportWriteGDML());
}
