/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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
#include <QPointer>
#endif

#include <Gui/Command.h>
#include <Gui/MainWindow.h>

#include "MaterialsEditor.h"
#include "ModelSelect.h"
#include "MaterialSave.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===========================================================================
// Material_Edit
//===========================================================================
DEF_STD_CMD_A(CmdMaterialsEdit)

CmdMaterialsEdit::CmdMaterialsEdit()
  :Command("Materials_Edit")
{
    sAppModule    = "Material";
    sGroup        = QT_TR_NOOP("Material");
    sMenuText     = QT_TR_NOOP("Edit...");
    sToolTipText  = QT_TR_NOOP("Edit material properties");
    sWhatsThis    = "Materials_Edit";
    sStatusTip    = sToolTipText;
    sPixmap       = "Materials_Edit";
}

void CmdMaterialsEdit::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Base::Console().Log("Materials_Edit\n");

    static QPointer<QDialog> dlg = nullptr;
    if (!dlg)
        dlg = new MatGui::MaterialsEditor(Gui::getMainWindow());
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

bool CmdMaterialsEdit::isActive()
{
    // return (hasActiveDocument() && !Gui::Control().activeDialog());
    return true;
}

//===========================================================================
// Material_ValueEdit
//===========================================================================
DEF_STD_CMD_A(CmdMaterialsValueEdit)

CmdMaterialsValueEdit::CmdMaterialsValueEdit()
  :Command("Materials_ValueEdit")
{
    sAppModule    = "Material";
    sGroup        = QT_TR_NOOP("Material");
    sMenuText     = QT_TR_NOOP("Value Edit...");
    sToolTipText  = QT_TR_NOOP("Edit material property values");
    sWhatsThis    = "Materials_ValueEdit";
    sStatusTip    = sToolTipText;
    sPixmap       = "Materials_Edit";
}

void CmdMaterialsValueEdit::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Base::Console().Log("Materials_ValueEdit\n");

    openCommand(QT_TRANSLATE_NOOP("Command", "Material value edit"));
    addModule(Doc,"ValueEditor");
    doCommand(Doc,"ValueEditor.openEditor()");
    commitCommand();
    // updateActive();
}

bool CmdMaterialsValueEdit::isActive()
{
    return true;
}

//===========================================================================
// Materials_ModelSelect
//===========================================================================
DEF_STD_CMD_A(CmdMaterialModelSelect)

CmdMaterialModelSelect::CmdMaterialModelSelect()
  :Command("Materials_ModelSelect")
{
    sAppModule    = "Material";
    sGroup        = QT_TR_NOOP("Material");
    sMenuText     = QT_TR_NOOP("Material model selection...");
    sToolTipText  = QT_TR_NOOP("Material model selection");
    sWhatsThis    = "Materials_ModelSelect";
    sStatusTip    = sToolTipText;
    sPixmap       = "Materials_Edit";
}

void CmdMaterialModelSelect::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Base::Console().Log("Materials_ModelSelect\n");

    static QPointer<QDialog> dlg = nullptr;
    if (!dlg)
        dlg = new MatGui::ModelSelect(Gui::getMainWindow());
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

bool CmdMaterialModelSelect::isActive()
{
    return true;
}

//===========================================================================
// Materials_MaterialSave
//===========================================================================
DEF_STD_CMD_A(CmdMaterialSave)

CmdMaterialSave::CmdMaterialSave()
  :Command("Materials_MaterialSave")
{
    sAppModule    = "Material";
    sGroup        = QT_TR_NOOP("Material");
    sMenuText     = QT_TR_NOOP("Material save...");
    sToolTipText  = QT_TR_NOOP("Material save");
    sWhatsThis    = "Materials_MaterialSave";
    sStatusTip    = sToolTipText;
    sPixmap       = "Materials_Edit";
}

void CmdMaterialSave::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Base::Console().Log("Materials_MaterialSave\n");

    static QPointer<QDialog> dlg = nullptr;
    if (!dlg)
        dlg = new MatGui::MaterialSave(Gui::getMainWindow());
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

bool CmdMaterialSave::isActive()
{
    return true;
}

//---------------------------------------------------------------

void CreateMaterialCommands()
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdMaterialsEdit());
    rcCmdMgr.addCommand(new CmdMaterialsValueEdit());
    rcCmdMgr.addCommand(new CmdMaterialModelSelect());
    rcCmdMgr.addCommand(new CmdMaterialSave());
}
