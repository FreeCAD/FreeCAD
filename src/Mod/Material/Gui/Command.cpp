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
#endif

#include <Gui/Command.h>
#include <Gui/MainWindow.h>

#include "Materials.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===========================================================================
// Material_Edit
//===========================================================================
DEF_STD_CMD_A(CmdMaterialEdit)

CmdMaterialEdit::CmdMaterialEdit()
  :Command("Material_Edit")
{
    sAppModule    = "Material";
    sGroup        = QT_TR_NOOP("Material");
    sMenuText     = QT_TR_NOOP("Edit...");
    sToolTipText  = QT_TR_NOOP("Edit material properties");
    sWhatsThis    = "Material_Edit";
    sStatusTip    = sToolTipText;
    sPixmap       = "Material_Edit";
}

void CmdMaterialEdit::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Base::Console().Log("CmdMaterialEdit::activated()\n");

    static QPointer<QDialog> dlg = nullptr;
    if (!dlg)
        dlg = new MatGui::Material(Gui::getMainWindow());
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

bool CmdMaterialEdit::isActive()
{
    // return (hasActiveDocument() && !Gui::Control().activeDialog());
    return true;
}

//---------------------------------------------------------------

void CreateMaterialCommands()
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdMaterialEdit());
}
