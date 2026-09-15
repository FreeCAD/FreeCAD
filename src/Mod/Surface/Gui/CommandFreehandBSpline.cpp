// SPDX-License-Identifier: LGPL-2.1-or-later

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>

DEF_STD_CMD_A(CmdSurfaceFreehandBSpline)
CmdSurfaceFreehandBSpline::CmdSurfaceFreehandBSpline()
    : Command("Surface_FreehandBSpline")
{
    sAppModule = "Surface";
    sGroup = QT_TR_NOOP("Surface");
    sMenuText = QT_TR_NOOP("Freehand B-Spline");
    sToolTipText = QT_TR_NOOP("Creates an editable B-spline through points placed in the 3D view");
    sStatusTip = sToolTipText;
    sPixmap = "Surface_FreehandBSpline";
}
bool CmdSurfaceFreehandBSpline::isActive()
{
    return hasActiveDocument() && !Gui::Control().activeDialog();
}
void CmdSurfaceFreehandBSpline::activated(int)
{
    auto name = getUniqueObjectName("FreehandBSpline");
    openCommand(QT_TRANSLATE_NOOP("Command", "Create freehand B-spline"));
    doCommand(Doc, "App.ActiveDocument.addObject('Surface::FreehandBSpline', '%s')", name.c_str());
    doCommand(Doc, "App.ActiveDocument.%s.Points = []", name.c_str());
    doCommand(Gui, "Gui.ActiveDocument.setEdit('%s')", name.c_str());
}

void CreateSurfaceFreehandBSplineCommands()
{
    auto& manager = Gui::Application::Instance->commandManager();
    manager.addCommand(new CmdSurfaceFreehandBSpline);
}
