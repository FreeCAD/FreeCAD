/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QPointer>
#endif

#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>

#include "DlgDisplayPropertiesImp.h"
#include "DlgInspectAppearance.h"
#include "DlgInspectMaterial.h"
#include "DlgMaterialImp.h"
#include "MaterialSave.h"
#include "MaterialsEditor.h"
#include "ModelSelect.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===========================================================================
// Materials_Edit
//===========================================================================
DEF_STD_CMD_A(CmdMaterialsEdit)

CmdMaterialsEdit::CmdMaterialsEdit()
    : Command("Materials_Edit")
{
    sAppModule = "Material";
    sGroup = QT_TR_NOOP("Material");
    sMenuText = QT_TR_NOOP("Edit...");
    sToolTipText = QT_TR_NOOP("Edit material properties");
    sWhatsThis = "Materials_Edit";
    sStatusTip = sToolTipText;
    sPixmap = "Materials_Edit";
}

void CmdMaterialsEdit::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Base::Console().Log("Materials_Edit\n");

    static QPointer<QDialog> dlg = nullptr;
    if (!dlg) {
        dlg = new MatGui::MaterialsEditor(Gui::getMainWindow());
    }
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

bool CmdMaterialsEdit::isActive()
{
    // return (hasActiveDocument() && !Gui::Control().activeDialog());
    return true;
}

//===========================================================================
// Std_SetAppearance
//===========================================================================
DEF_STD_CMD_A(StdCmdSetAppearance)

StdCmdSetAppearance::StdCmdSetAppearance()
    : Command("Std_SetAppearance")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Appearance...");
    sToolTipText = QT_TR_NOOP("Sets the display properties of the selected object");
    sWhatsThis = "Std_SetAppearance";
    sStatusTip = QT_TR_NOOP("Sets the display properties of the selected object");
    sPixmap = "Std_SetAppearance";
    sAccel = "Ctrl+D";
    eType = Alter3DView;
}

void StdCmdSetAppearance::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new MatGui::TaskDisplayProperties());
}

bool StdCmdSetAppearance::isActive()
{
    return (Gui::Control().activeDialog() == nullptr) && (Gui::Selection().size() != 0);
}

//===========================================================================
// Std_SetMaterial
//===========================================================================
DEF_STD_CMD_A(StdCmdSetMaterial)

StdCmdSetMaterial::StdCmdSetMaterial()
    : Command("Std_SetMaterial")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("&Material...");
    sToolTipText = QT_TR_NOOP("Sets the material of the selected object");
    sWhatsThis = "Std_SetMaterial";
    sStatusTip = QT_TR_NOOP("Sets the material of the selected object");
    sPixmap = "Materials_Edit";
    // sAccel        = "Ctrl+D";
    // eType = Alter3DView;
}

void StdCmdSetMaterial::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new MatGui::TaskMaterial());
}

bool StdCmdSetMaterial::isActive()
{
    return (Gui::Control().activeDialog() == nullptr) && (Gui::Selection().size() != 0);
}

//===========================================================================
// Materials_InspectAppearance
//===========================================================================
DEF_STD_CMD_A(CmdInspectAppearance)

CmdInspectAppearance::CmdInspectAppearance()
    : Command("Materials_InspectAppearance")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Inspect Appearance...");
    sToolTipText = QT_TR_NOOP("Inspect the appearance properties of the selected object");
    sWhatsThis = "Materials_InspectAppearance";
    sStatusTip = QT_TR_NOOP("Inspect the appearance properties of the selected object");
    // sPixmap = "Materials_Edit";
}

void CmdInspectAppearance::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new MatGui::TaskInspectAppearance());
}

bool CmdInspectAppearance::isActive()
{
    return (Gui::Control().activeDialog() == nullptr);
}

//===========================================================================
// Materials_InspectMaterial
//===========================================================================
DEF_STD_CMD_A(CmdInspectMaterial)

CmdInspectMaterial::CmdInspectMaterial()
    : Command("Materials_InspectMaterial")
{
    sGroup = "Standard-View";
    sMenuText = QT_TR_NOOP("Inspect Material...");
    sToolTipText = QT_TR_NOOP("Inspect the material properties of the selected object");
    sWhatsThis = "Materials_InspectMaterial";
    sStatusTip = QT_TR_NOOP("Inspect the material properties of the selected object");
    // sPixmap = "Materials_Edit";
}

void CmdInspectMaterial::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new MatGui::TaskInspectMaterial());
}

bool CmdInspectMaterial::isActive()
{
    return (Gui::Control().activeDialog() == nullptr);
}

//---------------------------------------------------------------

void CreateMaterialCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdMaterialsEdit());
    rcCmdMgr.addCommand(new StdCmdSetAppearance());
    rcCmdMgr.addCommand(new StdCmdSetMaterial());
    rcCmdMgr.addCommand(new CmdInspectAppearance());
    rcCmdMgr.addCommand(new CmdInspectMaterial());
}
