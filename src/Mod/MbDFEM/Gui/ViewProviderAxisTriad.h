// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/MbDFEM/MbDFEMGlobal.h>

#include <functional>

class QAction;
class QMenu;
class SoSeparator;
class SoSwitch;

namespace Gui
{
class ActionFunction;
}

namespace MbDFEMGui
{

MbDFEMGuiExport SoSeparator* createAxisTriad();
MbDFEMGuiExport SoSwitch* createAxisTriadSwitch(bool visible);
MbDFEMGuiExport void updateAxisTriadSwitch(SoSwitch* axisTriadSwitch, bool visible);
MbDFEMGuiExport QAction* addAxisTriadContextMenuAction(QMenu* menu,
                                                       Gui::ActionFunction* actionFunction,
                                                       bool checked,
                                                       std::function<void(bool)> setVisible);

}  // namespace MbDFEMGui
