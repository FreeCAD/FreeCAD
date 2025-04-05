// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 PaddleStroke <>                                    *
 *                 2025 wandererfan <wondererfan[at]gmail[dot]com>         *
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

// this is the multi-purpose view inserter that was originally in Command.cpp
// CmdTechDrawView::activated

#ifndef TECHDRAWGUI_SINGLEINSERTTOOL_H
#define TECHDRAWGUI_SINGLEINSERTTOOL_H

#include <Mod/TechDraw/TechDrawGlobal.h>

namespace Gui {
class Command;
}

namespace TechDraw {
class DrawPage;
}

namespace TechDrawGui {
// ?? namespace TechDrawGui::CommandHelpers {

void  singleInsertTool(Gui::Command& cmd, TechDraw::DrawPage* page);

}

#endif  // TECHDRAWGUI_SINGLEINSERTTOOL_H
